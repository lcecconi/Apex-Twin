#include "usb_storage_manager.h"
#include <cstdio>

void USBStorageManager::begin(SDManager *sdManager) {
  _sd = sdManager;
  _mscActive = false;
}

bool USBStorageManager::isMSCActive() const {
  return _mscActive;
}

void USBStorageManager::startMSCMode() {
  _mscActive = true;
  printf("[USB-MSC] Mass storage mode activated.\n");
}

void USBStorageManager::stopMSCMode() {
  _mscActive = false;
  printf("[USB-MSC] Mass storage mode stopped.\n");
}
