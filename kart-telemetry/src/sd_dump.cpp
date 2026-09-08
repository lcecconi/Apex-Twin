#include <Arduino.h>
#include <SD.h>
#include "gnss_commands.h"

constexpr uint32_t SERIAL_BAUD = 460800;
constexpr size_t TRANSFER_BUFFER_SIZE = 512;

void listDirectoryFiles(File& directory) {
  while (true) {
    File entry = directory.openNextFile();
    if (!entry) {
      return;
    }

    if (entry.isDirectory()) {
      listDirectoryFiles(entry);
    } else {
      Serial.printf("SDDUMP_FILE %s %u\n",
                    entry.name(),
                    static_cast<unsigned int>(entry.size()));
    }
    entry.close();
  }
}

void listFilesUnderPath(const String& requestedPath) {
  String path = requestedPath;
  path.trim();
  if (path.length() == 0) {
    path = "/";
  }

  File directory = SD.open(path, FILE_READ);
  if (!directory || !directory.isDirectory()) {
    Serial.printf("SDDUMP_ERROR not-a-directory %s\n", path.c_str());
    if (directory) {
      directory.close();
    }
    return;
  }

  Serial.printf("SDDUMP_LIST_BEGIN %s\n", path.c_str());
  listDirectoryFiles(directory);
  directory.close();
  Serial.println("SDDUMP_LIST_END");
  Serial.flush();
}

void sendFileToPc(const String& requestedPath) {
  String path = requestedPath;
  path.trim();
  if (path.length() == 0) {
    path = GNGGA_LOG_PATH;
  }

  File sourceFile = SD.open(path, FILE_READ);
  if (!sourceFile || sourceFile.isDirectory()) {
    Serial.printf("SDDUMP_ERROR %s\n", path.c_str());
    if (sourceFile) {
      sourceFile.close();
    }
    return;
  }

  const size_t fileSize = sourceFile.size();
  Serial.printf("SDDUMP_BEGIN %u\n", static_cast<unsigned int>(fileSize));
  Serial.flush();

  uint8_t buffer[TRANSFER_BUFFER_SIZE];
  size_t remaining = fileSize;
  while (remaining > 0) {
    size_t bytesRead = sourceFile.read(buffer, min(remaining, sizeof(buffer)));
    if (bytesRead == 0) {
      sourceFile.close();
      Serial.println("\nSDDUMP_ERROR read");
      return;
    }

    Serial.write(buffer, bytesRead);
    remaining -= bytesRead;
  }

  sourceFile.close();
  Serial.printf("\nSDDUMP_END %u\n", static_cast<unsigned int>(fileSize));
  Serial.flush();
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1000);

  if (!initializeSdCard()) {
    while (true) {
      delay(1000);
    }
  }

  Serial.println("SDDUMP_READY");
}

void loop() {
  if (!Serial.available()) {
    return;
  }

  String command = Serial.readStringUntil('\n');
  command.trim();
  if (command.startsWith("DUMP")) {
    sendFileToPc(command.substring(4));
  } else if (command.startsWith("LIST")) {
    listFilesUnderPath(command.substring(4));
  } else if (command == "READY") {
    Serial.println("SDDUMP_READY");
  }
}