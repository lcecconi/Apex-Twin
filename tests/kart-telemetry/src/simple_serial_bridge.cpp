#include <Arduino.h>
#include "gnss_commands.h"

// ESP32-S3 Hardware UART Pins connected to LC29HEA
#define GNSS_RX_PIN 1  // ESP32-S3 RX <- LC29HEA TX
#define GNSS_TX_PIN 2  // ESP32-S3 TX -> LC29HEA RX
#define GNSS_BAUD   460800
#define SERIAL_BAUD 460800*2

HardwareSerial SerialGNSS(1);

void configureNmeaOutput() {
  // Array of configuration commands

  const char* commands[] = {
    "PQTMGNSSSTOP", // Stop GNSS output before configuration
    "$PAIR050,100",   // Set 10 Hz update rate
    "$PAIR062,0,1",   // Enable GGA
    "$PAIR062,3,1",   // Enable GSV
    "$PAIR062,5,0",   // Disable VTG
    "$PAIR062,1,0",   // Disable GLL
    "$PAIR062,2,0",   // Disable GSA
    "$PAIR062,4,0",   // Disable RMC
    "PQTMCFGRCVRMODE,W,1", // Set Rover Mode
    "PAIR864,0,0,460800", // Set Baud Rate to 460800
    "PQTMSAVEPAR"      // Save Parameters to Flash
  };

  for (const char* cmd : commands) {
    sendGnssCommand(SerialGNSS, cmd);
    readCommandResponse(SerialGNSS, 500);
    delay(100);               // Brief pause between commands
  }
  
  delay(1000);
  sendGnssCommand(SerialGNSS, "PQTMGNSSSTART");
  readCommandResponse(SerialGNSS, 500);
}

void setup() {
  // 1. Initialize USB Serial connection to PC
  Serial.begin(SERIAL_BAUD);
  delay(2000); 

  // 2. Initialize Hardware Serial to LC29HEA at 921600 baud
  SerialGNSS.begin(GNSS_BAUD, SERIAL_8N1, GNSS_RX_PIN, GNSS_TX_PIN);
  
  Serial.println("==========================================");
  Serial.println("  ESP32-S3 + LC29HEA Serial Bridge SETUP  ");
  Serial.println("==========================================");

  configureNmeaOutput();
  
  Serial.println("==========================================");
  Serial.println("  ESP32-S3 + LC29HEA Serial Bridge READY  ");
  Serial.println("==========================================");
}

void loop() {
  // Relay clean ASCII data from LC29HEA to PC
  if (SerialGNSS.available()) {
    Serial.printf("[RX GNSS] %d bytes available\n", SerialGNSS.available());
  }
  while (SerialGNSS.available()) {
    Serial.write(SerialGNSS.read());
  }
  // Relay typed commands from PC to LC29HEA
  while (Serial.available()) {
    sendGnssCommand(SerialGNSS, Serial.readStringUntil('\n'));
  }
}