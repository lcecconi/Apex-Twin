#include <Arduino.h>
#include <WiFi.h>
#include <ESP32Ping.h>
#include <TinyGPSPlus.h>
#include <vector>
#include "gnss_commands.h"

// --- Hardware Setup ---
#define GNSS_RX_PIN 1  // ESP32-S3 RX <- LC29HEA TX
#define GNSS_TX_PIN 2  // ESP32-S3 TX -> LC29HEA RX
#define GNSS_BAUD   460800 // Confirmed LC29HEA Default Baud Rate
#define SERIAL_BAUD 921600
// Satellite report mode:
// 0 = full per-satellite report
// 1 = compact RTK-ready summary
#ifndef GPS_SATELLITE_REPORT_MODE
#define GPS_SATELLITE_REPORT_MODE 1
#endif

// Performance monitor:
// 0 = disabled
// 1 = print loop() timing stats with the periodic status report
#ifndef GPS_PERF_MONITOR
#define GPS_PERF_MONITOR 1
#endif

// --- Network & NTRIP Settings ---
const char* WIFI_SSID     = "Antenna 5G";
const char* WIFI_PASS     = "01234567";

const char* NTRIP_HOST    = "crtk.net"; // e.g., rtk2go.com or caster.centipede.fr
const int   NTRIP_PORT    = 2101;
const char* NTRIP_MOUNT   = "LAJ39"; // Nearest mountpoint
const char* NTRIP_USER    = "c";
const char* NTRIP_PASS    = "c";

// --- Satellite Tracker Structure ---
struct SatSignal {
  String constell; // GPS, GAL, GLO, BDS
  int id;
  int snr;         // dB-Hz
};

struct RtkCorrectionAgeSample {
  unsigned long timestampMs;
  float ageSeconds;
};

std::vector<SatSignal> satelliteList;
std::vector<RtkCorrectionAgeSample> rtkCorrectionAgeSamples;

// --- Timing Configuration ---
const unsigned long SAT_REPORT_INTERVAL = 1000;
const unsigned long NTRIP_UPDATE_GGA_INTERVAL = 1000;
const unsigned long RTK_CORRECTION_AGE_WINDOW_MS = 10000;
const uint16_t GNGGA_BATCH_SIZE = 100;
unsigned long lastSatReportTime = 0;
unsigned long totalRtcmBytesReceived = 0;
unsigned long lastRtcmPacketTime     = 0;
unsigned long prevTotalRtcmBytes     = 0;

#if GPS_PERF_MONITOR
unsigned long loopCount = 0;
unsigned long loopTimeTotalMs = 0;
unsigned long loopTimeMaxMs = 0;
unsigned long lastLoopDurationMs = 0;
#endif

HardwareSerial SerialGNSS(1);
WiFiClient ntripClient;
TinyGPSPlus gps;
TinyGPSCustom ggaQuality(gps, "GNGGA", 6);
bool sdCardAvailable = false;
String gnggaLogPath = GNGGA_LOG_PATH;
String gnggaBatch;
uint16_t gnggaBatchCount = 0;

String currentGGA = "";
unsigned long lastGgaSentTime = 0;
unsigned long prevGgaSentTime = 0;

void connectToNtrip();
void checkInternetConnectivity();
String base64Encode(String input);
bool extractGgaCorrectionAge(const String& line, float& ageSeconds);
void recordRtkCorrectionAge(float ageSeconds);
float getRollingRtkCorrectionAgeAverage();
void parseGsvSentence(const String& line);
void printSatelliteStatus();
String getFixTypeString(int qualityCode);
String getSignalBar(int snr);
void queryModuleStatus();

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(3000);

  sdCardAvailable = initializeSdCard();
  if (sdCardAvailable) {
    gnggaBatch.reserve(12000);
    String newLogPath = createGnggaLogPath();
    if (newLogPath.length() > 0) {
      gnggaLogPath = newLogPath;
      Serial.println("[SD] GNGGA log: " + gnggaLogPath);
    } else {
      Serial.println("[SD] Using fallback GNGGA log: " + gnggaLogPath);
    }
  }

  // 1. EXPAND RX BUFFER: Expands buffer to 2048 bytes to stop line corruption/overflows
  SerialGNSS.setRxBufferSize(2048);
  SerialGNSS.begin(GNSS_BAUD, SERIAL_8N1, GNSS_RX_PIN, GNSS_TX_PIN);
  delay(500);
  
  // 2. THROTTLE RATE & FILTER SENTENCES:
  const char* commands[] = {
    "$PQTMGNSSSTOP", // Stop GNSS output before configuration
    "$PQTMCFGFIXRATE,W,50"
    "$PQTMCFGRCVRMODE,W,1", // Set Rover Mode
    "$PQTMCFGMSGRATE,W,GGA,1",   // Enable GGA
    "$PQTMCFGMSGRATE,W,GSV,20",   // Enable GSV
    "$PQTMCFGMSGRATE,W,VTG,0",   // Disable VTG
    "$PQTMCFGMSGRATE,W,GLL,0",   // Disable GLL
    "$PQTMCFGMSGRATE,W,GSA,0",   // Disable GSA
    "$PQTMCFGMSGRATE,W,RMC,0",   // Disable RMC
    "$PQTMCFGUART,W,460800", // Set Baud Rate to 460800
    "$PQTMSAVEPAR"      // Save Parameters to Flash
  };

  for (const char* cmd : commands) {
    sendGnssCommand(SerialGNSS, cmd);
    readCommandResponse(SerialGNSS, 500);
    delay(100);               // Brief pause between commands
  }
  
  delay(2000);
  sendGnssCommand(SerialGNSS, "PQTMGNSSSTART");
  readCommandResponse(SerialGNSS, 500);
  
  queryModuleStatus();
  // Flush buffer of any boot noise before WiFi starts
  while (SerialGNSS.available()) SerialGNSS.read();
  
  delay(1000);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("[WIFI] Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[WIFI] Connected! IP: " + WiFi.localIP().toString());

  checkInternetConnectivity();

  connectToNtrip();
}
void loop() {
  unsigned long loopStartMs = millis();

  // 1. Read UART stream & feed parsers
  static String lineBuffer = "";
  while (SerialGNSS.available()) {
    char c = SerialGNSS.read();
    gps.encode(c);

    if (c == '\n') {
      lineBuffer.trim();
      if (lineBuffer.startsWith("$GNGGA") || lineBuffer.startsWith("$GPGGA")) {
        currentGGA = lineBuffer;
        float correctionAgeSeconds = 0.0f;
        if (extractGgaCorrectionAge(lineBuffer, correctionAgeSeconds)) {
          recordRtkCorrectionAge(correctionAgeSeconds);
        }
        if (sdCardAvailable && lineBuffer.startsWith("$GNGGA")) {
          gnggaBatch += lineBuffer;
          gnggaBatch += '\n';
          gnggaBatchCount++;

          if (gnggaBatchCount >= GNGGA_BATCH_SIZE) {
            bool written = appendGnggaBatch(gnggaBatch, gnggaLogPath);
            if (!written) {
              Serial.println("[SD] GNGGA batch write failed");
            }
            gnggaBatch = "";
            gnggaBatchCount = 0;
          }
        }
      } else if (lineBuffer.indexOf("GSV") != -1) {
        parseGsvSentence(lineBuffer); // Intercept and parse GSV sentences
      }
      lineBuffer = "";
    } else if (c != '\r') {
      lineBuffer += c;
    }
  }

  // 2. Stream RTCM3 bytes directly to LC29HEA
  // --- Optimized High-Speed RTCM Buffer Pipe ---
  if (ntripClient.connected()) {
    uint8_t rtcmBuffer[512];
    int bytesAvailable = ntripClient.available();
    
    if (bytesAvailable > 0) {
      int bytesToRead = min(bytesAvailable, (int)sizeof(rtcmBuffer));
      int bytesRead = ntripClient.read(rtcmBuffer, bytesToRead);
      
      // Write entire block directly to LC29HEA hardware UART
      SerialGNSS.write(rtcmBuffer, bytesRead);
      
      totalRtcmBytesReceived += bytesRead;
      // Serial.printf("[NTRIP] Time between last RTCM packet: %lu ms | Total bytes received: %lu\n", millis() - lastRtcmPacketTime, totalRtcmBytesReceived);
      lastRtcmPacketTime = millis();
    }
  }

  // 3. Periodic position back-report to NTRIP caster
  if (millis() - lastGgaSentTime >= NTRIP_UPDATE_GGA_INTERVAL) {

    // 1. Verify that a GGA sentence exists and contains actual coordinates (not empty commas)
    bool isValidGga = (currentGGA.length() > 30) && 
                      (currentGGA.indexOf(",,,") == -1) && 
                      (gps.location.isValid());

    if (!isValidGga) {
      Serial.println("[TX -> CASTER SKIPPED] No valid 3D position lock to send yet.");
    } 
    else if (!ntripClient.connected()) {
      Serial.println("[TX -> CASTER ERROR] Wi-Fi client is not connected to the caster!");
    } 
    else {
      // 2. Transmit string over socket (println automatically appends mandatory \r\n)
      size_t bytesSent = ntripClient.println(currentGGA);
      prevGgaSentTime = lastGgaSentTime;
      lastGgaSentTime = millis();

      // 3. Output confirmation to Console
      // Serial.println("--------------------------------------------------");
      // Serial.printf("[TX -> CASTER] %d bytes sent successfully:\n", bytesSent);
      // Serial.printf("  %s\n", currentGGA.c_str());
      // Serial.println("--------------------------------------------------");
    }
  }
  // 4. Periodic Status & SNR Report
  if (millis() - lastSatReportTime >= SAT_REPORT_INTERVAL) {
    lastSatReportTime = millis();
    printSatelliteStatus();
  }

#if GPS_PERF_MONITOR
  lastLoopDurationMs = millis() - loopStartMs;
  loopTimeTotalMs += lastLoopDurationMs;
  loopCount++;
  if (lastLoopDurationMs > loopTimeMaxMs) {
    loopTimeMaxMs = lastLoopDurationMs;
  }
#endif
}

// --- GSV Sentence Parser for Satellite SNRs ---
void parseGsvSentence(const String& line) {
  int commas[20];
  int count = 0;
  for (int i = 0; i < line.length() && count < 20; i++) {
    if (line.charAt(i) == ',') commas[count++] = i;
  }
  if (count < 3) return;

  String header = line.substring(0, commas[0]);
  
  // FIXED: Field 1 is TotalMsgs, Field 2 (commas[1] to commas[2]) is MsgNum!
  int totalMsgs = line.substring(commas[0] + 1, commas[1]).toInt();
  int msgNum    = line.substring(commas[1] + 1, commas[2]).toInt(); 

  String constell = "GNSS";
  if (header.startsWith("$GP")) constell = "GPS";
  else if (header.startsWith("$GA")) constell = "GAL";
  else if (header.startsWith("$GB") || header.startsWith("$BD")) constell = "BDS";
  else if (header.startsWith("$GL")) constell = "GLO";

  // Reset constellation entries when sequence restarts at Message 1
  if (msgNum == 1) {
    for (int i = satelliteList.size() - 1; i >= 0; i--) {
      if (satelliteList[i].constell == constell) {
        satelliteList.erase(satelliteList.begin() + i);
      }
    }
  }

  // Extract up to 4 satellites per GSV sentence...
  for (int s = 0; s < 4; s++) {
    int prnIdx = 3 + (s * 4);
    int snrIdx = 6 + (s * 4);
    if (snrIdx < count) {
      int id = line.substring(commas[prnIdx] + 1, commas[prnIdx + 1]).toInt();
      String snrStr = (snrIdx + 1 < count) ? 
                      line.substring(commas[snrIdx] + 1, commas[snrIdx + 1]) : 
                      line.substring(commas[snrIdx] + 1, line.indexOf('*', commas[snrIdx]));
      int snr = snrStr.toInt();

      if (id > 0) {
        satelliteList.push_back({constell, id, snr});
      }
    }
  }
}
// --- Status & Diagnostic Printer ---
void printSatelliteStatus() {
  int fixQuality = atoi(ggaQuality.value());
  int numSatellites = gps.satellites.value();
  float hdopVal = gps.hdop.isValid() ? gps.hdop.hdop() : 99.9;

  float rtcmDataRate = (totalRtcmBytesReceived - prevTotalRtcmBytes) / (SAT_REPORT_INTERVAL / 1000.0);
  prevTotalRtcmBytes = totalRtcmBytesReceived;

  Serial.println("\n==================================================");
  Serial.println("           RTK DIAGNOSTIC & SNR REPORT            ");
  Serial.println("==================================================");
  Serial.printf(" Fix Status    : [%d] %s\n", fixQuality, getFixTypeString(fixQuality).c_str());
  Serial.printf(" Satellites    : %d in fix (%d in view)\n", numSatellites, (int)satelliteList.size());
  Serial.printf(" HDOP          : %.2f\n", hdopVal);
  float avgCorrectionAgeSeconds = getRollingRtkCorrectionAgeAverage();
  if (avgCorrectionAgeSeconds >= 0.0f) {
    Serial.printf(" RTK Corr Age  : %.2f s avg over last 10 s\n", avgCorrectionAgeSeconds);
  } else {
    Serial.println(" RTK Corr Age  : n/a (waiting for GGA correction age)");
  }
  Serial.printf(" RTCM Throughput: %.1f B/s (Total: %lu B)\n", rtcmDataRate, totalRtcmBytesReceived);
  
  Serial.println("--------------------------------------------------");
  Serial.println(" DETAILED SATELLITE SIGNAL STRENGTHS (C/N0):");

#if GPS_SATELLITE_REPORT_MODE == 0
  if (satelliteList.empty()) {
    Serial.println(" No satellite signal data parsed yet...");
  } else {
    int strongCount = 0;
    for (const auto& sat : satelliteList) {
      if (sat.snr >= 38) strongCount++;
      Serial.printf(" [%-3s %02d] %2d dB-Hz %s\n",
                    sat.constell.c_str(), sat.id, sat.snr, getSignalBar(sat.snr).c_str());
    }
    Serial.printf("\n L1/L5 RTK Ready Signals (>=38 dB-Hz): %d / %d\n", strongCount, (int)satelliteList.size());
  }
#else
  int strongCount = 0;
  int trackedCount = 0;

  for (const auto& sat : satelliteList) {
    if (sat.snr > 0) {
      trackedCount++;
    }
    if (sat.snr >= 38) {
      strongCount++;
    }
  }

  if (trackedCount == 0) {
    Serial.println(" No satellite signal data parsed yet...");
  } else {
    Serial.printf(" RTK-ready signals (>=38 dB-Hz): %d / %d\n", strongCount, trackedCount);
    Serial.println(strongCount > 0 ? " RTK signal strength looks usable." : " RTK signal strength is not there yet.");
  }
#endif
  Serial.println("--------------------------------------------------");
  Serial.println(" NTRIP MONITOR");
  Serial.printf(" Caster Connected: %s\n", ntripClient.connected() ? "YES" : "NO");
  Serial.printf(" Last RTCM Packet: %lu ms ago\n", millis() - lastRtcmPacketTime);
  Serial.printf(" Last GGA Sent    : %lu ms ago\n", millis() - prevGgaSentTime);
  Serial.println("--------------------------------------------------");

#if GPS_PERF_MONITOR
  if (loopCount > 0) {
    float avgLoopMs = (float)loopTimeTotalMs / (float)loopCount;
    Serial.println("--------------------------------------------------");
    Serial.println(" LOOP PERFORMANCE MONITOR");
    Serial.printf(" Last loop   : %lu ms\n", lastLoopDurationMs);
    Serial.printf(" Average loop: %.2f ms\n", avgLoopMs);
    Serial.printf(" Max loop    : %lu ms\n", loopTimeMaxMs);
    Serial.printf(" Samples     : %lu\n", loopCount);
  }
#endif

  Serial.println("==================================================\n");
  // Clear buffer so entries don't accumulate indefinitely
  satelliteList.clear();
}

String getSignalBar(int snr) {
  if (snr == 0)   return "[░░░░] No Signal";
  if (snr < 25)  return "[█░░░] Weak";
  if (snr < 35)  return "[██░░] Moderate";
  if (snr < 42)  return "[███░] Good";
  return                 "[████] Strong (Ideal for RTK)";
}

String getFixTypeString(int qualityCode) {
  switch (qualityCode) {
    case 0: return "NO FIX";
    case 1: return "3D FIX";
    case 2: return "DGPS";
    case 4: return "RTK FIXED";
    case 5: return "RTK FLOAT";
    default: return "UNKNOWN";
  }
}

bool extractGgaCorrectionAge(const String& line, float& ageSeconds) {
  int fieldStart = line.indexOf(',');
  if (fieldStart < 0) {
    return false;
  }

  fieldStart++;
  for (int fieldIndex = 1; fieldIndex <= 13; ++fieldIndex) {
    int fieldEnd = line.indexOf(',', fieldStart);
    int checksumIdx = line.indexOf('*', fieldStart);

    if (fieldEnd < 0 || (checksumIdx >= 0 && checksumIdx < fieldEnd)) {
      fieldEnd = checksumIdx;
    }
    if (fieldEnd < 0) {
      fieldEnd = line.length();
    }

    if (fieldIndex == 13) {
      String fieldValue = line.substring(fieldStart, fieldEnd);
      fieldValue.trim();
      if (fieldValue.length() == 0) {
        return false;
      }

      ageSeconds = fieldValue.toFloat();
      return true;
    }

    if (fieldEnd >= line.length()) {
      break;
    }

    fieldStart = fieldEnd + 1;
  }

  return false;
}

void recordRtkCorrectionAge(float ageSeconds) {
  unsigned long now = millis();
  rtkCorrectionAgeSamples.push_back({now, ageSeconds});

  while (!rtkCorrectionAgeSamples.empty() &&
         now - rtkCorrectionAgeSamples.front().timestampMs > RTK_CORRECTION_AGE_WINDOW_MS) {
    rtkCorrectionAgeSamples.erase(rtkCorrectionAgeSamples.begin());
  }
}

float getRollingRtkCorrectionAgeAverage() {
  unsigned long now = millis();

  while (!rtkCorrectionAgeSamples.empty() &&
         now - rtkCorrectionAgeSamples.front().timestampMs > RTK_CORRECTION_AGE_WINDOW_MS) {
    rtkCorrectionAgeSamples.erase(rtkCorrectionAgeSamples.begin());
  }

  if (rtkCorrectionAgeSamples.empty()) {
    return -1.0f;
  }

  float ageSum = 0.0f;
  for (const auto& sample : rtkCorrectionAgeSamples) {
    ageSum += sample.ageSeconds;
  }

  return ageSum / static_cast<float>(rtkCorrectionAgeSamples.size());
}

void connectToNtrip() {
  if (!ntripClient.connect(NTRIP_HOST, NTRIP_PORT)) return;
  String authEncoded = base64Encode(String(NTRIP_USER) + ":" + String(NTRIP_PASS));
  ntripClient.print(String("GET /") + NTRIP_MOUNT + " HTTP/1.0\r\n" +
                   "User-Agent: NTRIP ESP32S3_LC29HEA\r\n" +
                   "Authorization: Basic " + authEncoded + "\r\n" +
                   "Accept: */*\r\n" +
                   "Connection: close\r\n\r\n");
}

void checkInternetConnectivity() {
  const char* pingHost = "www.google.com";
  Serial.print("[NET] Pinging ");
  Serial.print(pingHost);
  Serial.print(" ... ");

  bool reachable = Ping.ping(pingHost, 3);
  if (reachable) {
    Serial.printf("OK (avg %.1f ms)\n", Ping.averageTime());
  } else {
    Serial.println("FAILED");
  }
}

String base64Encode(String input) {
  const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  String enc = "";
  int val = 0, valb = -6;
  for (unsigned char c : input) {
    val = (val << 8) + c;
    valb += 8;
    while (valb >= 0) {
      enc += b64[(val >> valb) & 0x3F];
      valb -= 6;
    }
  }
  if (valb > -6) enc += b64[((val << 8) >> (valb + 8)) & 0x3F];
  while (enc.length() % 4) enc += '=';
  return enc;
}

void queryModuleStatus() {
  Serial.println("\n--- QUERYING LC29H HARDWARE & MODE ---");
  
  // Flush rx buffer
  while(SerialGNSS.available()) SerialGNSS.read();

  // Query Firmware Version
  sendGnssCommand(SerialGNSS, "$PQTMVERNO");
  readCommandResponse(SerialGNSS, 1000);
  // Query Receiver Mode
  sendGnssCommand(SerialGNSS, "$PQTMCFGRCVRMODE,R");
  readCommandResponse(SerialGNSS, 1000);  
  Serial.println("---------------------------------------\n");
}

