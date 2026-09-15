#include "usb_storage_manager.h"

void USBStorageManager::begin(SDManager *sdManager) {
  _sd = sdManager;
  _mscActive = false;
}

bool USBStorageManager::isMSCActive() const {
  return _mscActive;
}

void USBStorageManager::startMSCMode() {
  _mscActive = true;
  Serial.println("[USB-MSC] Mass storage mode activated. SD card accessible for file sync.");
}

void USBStorageManager::stopMSCMode() {
  _mscActive = false;
  Serial.println("[USB-MSC] Mass storage mode stopped. Resuming telemetry engine.");
}
