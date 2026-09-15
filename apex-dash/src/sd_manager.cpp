#include "sd_manager.h"

bool SDManager::begin() {
  pinMode(PIN_SD_CLK, INPUT_PULLUP);
  pinMode(PIN_SD_CMD, INPUT_PULLUP);
  pinMode(PIN_SD_DAT0, INPUT_PULLUP);

  if (!SD_MMC.setPins(PIN_SD_CLK, PIN_SD_CMD, PIN_SD_DAT0)) {
    Serial.println("[SD] Failed to set SD_MMC pins");
    _initialized = false;
    return false;
  }

  // 1-bit mode (mode1bit = true)
  if (!SD_MMC.begin("/sd", true)) {
    Serial.println("[SD] Card Mount Failed (No SD card inserted or bad format)");
    _initialized = false;
    return false;
  }

  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("[SD] No SD_MMC card attached");
    _initialized = false;
    return false;
  }

  _cardSizeMB = SD_MMC.cardSize() / (1024 * 1024);
  _initialized = true;
  Serial.printf("[SD] Card initialized successfully! Size: %llu MB\n", _cardSizeMB);

  ensureDirectories();
  return true;
}

bool SDManager::isAvailable() const {
  return _initialized;
}

uint64_t SDManager::getCardSizeMB() const {
  return _cardSizeMB;
}

uint64_t SDManager::getUsedBytes() const {
  if (!_initialized) return 0;
  return SD_MMC.usedBytes();
}

void SDManager::ensureDirectories() {
  if (!_initialized) return;

  if (!SD_MMC.exists("/tracks")) {
    SD_MMC.mkdir("/tracks");
    Serial.println("[SD] Created /tracks directory");
  }
  if (!SD_MMC.exists("/logs")) {
    SD_MMC.mkdir("/logs");
    Serial.println("[SD] Created /logs directory");
  }
}
