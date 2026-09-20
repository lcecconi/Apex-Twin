#include "sd_manager.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/gpio.h"
#include <sys/stat.h>
#include <cstdio>

bool SDManager::begin() {
  _initialized = false;
  _cardSizeMB = 0;
  return false;
}

bool SDManager::isAvailable() const {
  return _initialized;
}

uint64_t SDManager::getCardSizeMB() const {
  return _cardSizeMB;
}

uint64_t SDManager::getUsedBytes() const {
  return 0;
}

void SDManager::ensureDirectories() {
}
