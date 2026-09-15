#pragma once

#include <Arduino.h>
#include "sd_manager.h"

class USBStorageManager {
public:
  void begin(SDManager *sdManager);
  bool isMSCActive() const;
  void startMSCMode();
  void stopMSCMode();

private:
  SDManager *_sd = nullptr;
  bool _mscActive = false;
};
