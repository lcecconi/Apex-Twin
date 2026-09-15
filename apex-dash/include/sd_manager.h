#pragma once

#include <Arduino.h>
#include <FS.h>
#include <SD_MMC.h>
#include "config.h"

class SDManager {
public:
  bool begin();
  bool isAvailable() const;
  uint64_t getCardSizeMB() const;
  uint64_t getUsedBytes() const;
  void ensureDirectories();

private:
  bool _initialized = false;
  uint64_t _cardSizeMB = 0;
};
