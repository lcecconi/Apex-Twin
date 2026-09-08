#pragma once

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <Preferences.h>

constexpr uint8_t SD_MISO_PIN = 4;
constexpr uint8_t SD_MOSI_PIN = 5;
constexpr uint8_t SD_SCK_PIN = 6;
constexpr uint8_t SD_CS_PIN = 7;
constexpr char GNGGA_LOG_PATH[] = "/gngga.log";

inline String createGnggaLogPath() {
  Preferences preferences;
  if (!preferences.begin("gnss", false)) {
    Serial.println("[SD] Could not open GNSS boot counter");
    return String();
  }

  uint32_t bootNumber = preferences.getUInt("boot", 0) + 1;
  preferences.putUInt("boot", bootNumber);
  preferences.end();

  char path[32];
  snprintf(path, sizeof(path), "/gngga_%06lu.log", static_cast<unsigned long>(bootNumber));
  return String(path);
}

inline bool initializeSdCard() {
  SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);

  if (!SD.begin(SD_CS_PIN, SPI)) {
    Serial.println("[SD] Card initialization failed");
    return false;
  }

  Serial.println("[SD] Card initialized");
  return true;
}

inline bool appendGnggaMessage(const String& message, const String& path) {
  File logFile = SD.open(path, FILE_APPEND);
  if (!logFile) {
    Serial.printf("[SD] Could not open GNGGA log: %s\n", path.c_str());
    return false;
  }

  bool written = logFile.println(message);
  logFile.close();
  return written;
}

inline bool appendGnggaBatch(const String& messages, const String& path) {
  if (messages.length() == 0) {
    return true;
  }

  File logFile = SD.open(path, FILE_APPEND);
  if (!logFile) {
    Serial.printf("[SD] Could not open GNGGA log: %s\n", path.c_str());
    return false;
  }

  size_t bytesWritten = logFile.print(messages);
  logFile.close();
  return bytesWritten == messages.length();
}

inline bool appendGnggaMessage(const String& message) {
  return appendGnggaMessage(message, String(GNGGA_LOG_PATH));
}

inline void sendGnssCommand(HardwareSerial& serialGnss, const String& cmd) {
  String command = cmd;

  // Strip leading '$' or trailing checksum marker if present.
  if (command.startsWith("$")) command = command.substring(1);
  int starIdx = command.indexOf('*');
  if (starIdx != -1) command = command.substring(0, starIdx);

  uint8_t checksum = 0;
  for (size_t i = 0; i < command.length(); i++) {
    checksum ^= command.charAt(i);
  }

  char formattedCmd[128];
  snprintf(formattedCmd, sizeof(formattedCmd), "$%s*%02X\r\n", command.c_str(), checksum);

  Serial.printf("[TX GNSS] %s", formattedCmd);
  serialGnss.flush();
  serialGnss.print(formattedCmd);
}

inline void readCommandResponse(HardwareSerial& serialGnss, uint32_t timeoutMs) {
  uint32_t start = millis();
  bool receivedData = false;

  while (millis() - start < timeoutMs) {
    while (serialGnss.available()) {
      char c = serialGnss.read();
      Serial.write(c);
      receivedData = true;
    }
  }

  if (!receivedData) {
    Serial.println(" ❌ NO RESPONSE (Check Wiring/Baud Rate)");
  }
}