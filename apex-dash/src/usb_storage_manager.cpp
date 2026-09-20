#include "usb_storage_manager.h"
#include "sd_manager.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "tinyusb_msc.h"
#include "esp_log.h"
#include <cstdio>

static const char *TAG = "USB_MSC";
static tinyusb_msc_storage_handle_t s_msc_handle = nullptr;
static bool s_driver_installed = false;

void USBStorageManager::begin(SDManager *sdManager) {
  _sd = sdManager;
  _mscActive = false;
}

bool USBStorageManager::isMSCActive() const {
  return _mscActive;
}

void USBStorageManager::startMSCMode() {
  if (_mscActive) return;
  if (!_sd || !_sd->isAvailable()) {
    ESP_LOGW(TAG, "Cannot start MSC: No SD card available");
    return;
  }

  sdmmc_card_t *card = _sd->getCard();
  if (!card) {
    ESP_LOGW(TAG, "Cannot start MSC: Invalid SD card handle");
    return;
  }

  ESP_LOGI(TAG, "Pausing local SD VFS for USB Mass Storage...");
  _sd->pauseForUSB();

  // 1. Install TinyUSB Base Driver with proper task & PHY defaults
  if (!s_driver_installed) {
    tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();
    esp_err_t err = tinyusb_driver_install(&tusb_cfg);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
      ESP_LOGE(TAG, "Failed to install TinyUSB driver: 0x%x (%s)", err, esp_err_to_name(err));
      _sd->resumeAfterUSB();
      return;
    }
    s_driver_installed = true;
  }

  // 2. Install TinyUSB MSC Driver
  tinyusb_msc_driver_config_t msc_driver_cfg = {};
  esp_err_t err = tinyusb_msc_install_driver(&msc_driver_cfg);
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    ESP_LOGE(TAG, "Failed to install TinyUSB MSC driver: 0x%x (%s)", err, esp_err_to_name(err));
    _sd->resumeAfterUSB();
    return;
  }

  // 3. Register SDMMC Card Storage LUN
  tinyusb_msc_storage_config_t storage_cfg = {};
  storage_cfg.medium.card = card;
  storage_cfg.mount_point = TINYUSB_MSC_STORAGE_MOUNT_USB;
  storage_cfg.fat_fs.base_path = NULL;
  storage_cfg.fat_fs.do_not_format = true;

  err = tinyusb_msc_new_storage_sdmmc(&storage_cfg, &s_msc_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to register SDMMC storage for MSC: 0x%x (%s)", err, esp_err_to_name(err));
    tinyusb_msc_uninstall_driver();
    _sd->resumeAfterUSB();
    return;
  }

  _mscActive = true;
  ESP_LOGI(TAG, "USB Mass Storage mode activated. SD card exposed to PC.");
}

void USBStorageManager::stopMSCMode() {
  if (!_mscActive) return;

  ESP_LOGI(TAG, "Stopping USB Mass Storage mode...");

  if (s_msc_handle) {
    tinyusb_msc_delete_storage(s_msc_handle);
    s_msc_handle = nullptr;
  }

  tinyusb_msc_uninstall_driver();

  if (s_driver_installed) {
    tinyusb_driver_uninstall();
    s_driver_installed = false;
  }

  if (_sd) {
    _sd->resumeAfterUSB();
  }

  _mscActive = false;
  ESP_LOGI(TAG, "USB Mass Storage stopped. SD card returned to local system.");
}
