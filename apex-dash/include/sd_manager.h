#pragma once

#include <cstdint>
#include "config.h"
#include "driver/sdmmc_types.h"

class SDManager {
public:
  bool begin();
  bool probe();
  bool isAvailable() const;
  uint64_t getCardSizeMB() const;
  uint64_t getUsedBytes() const;
  void ensureDirectories();

  // USB MSC support
  sdmmc_card_t *getCard() const;
  void pauseForUSB();
  void resumeAfterUSB();

private:
  bool tryMount();

  bool _initialized = false;
  bool _probeDropped = false;
  uint8_t _probeAttempts = 0;
  uint64_t _cardSizeMB = 0;
  mutable uint32_t _lastProbeMs = 0;
};
