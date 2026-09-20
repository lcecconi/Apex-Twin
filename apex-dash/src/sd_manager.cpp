#include "sd_manager.h"
#include "config.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <sys/stat.h>
#include <cstdio>

static const char *TAG = "SD_MGR";
static sdmmc_card_t *s_card = nullptr;

static inline uint32_t get_time_ms() {
  return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

bool SDManager::tryMount() {
  if (s_card != nullptr) {
    esp_vfs_fat_sdcard_unmount("/sdcard", s_card);
    s_card = nullptr;
  }
  sdmmc_host_deinit();

  // Stabilization delay
  vTaskDelay(pdMS_TO_TICKS(50));

  gpio_set_pull_mode((gpio_num_t)PIN_SD_CMD, GPIO_PULLUP_ONLY);
  gpio_set_pull_mode((gpio_num_t)PIN_SD_DAT0, GPIO_PULLUP_ONLY);
  gpio_set_pull_mode((gpio_num_t)PIN_SD_CLK, GPIO_FLOATING);

  esp_vfs_fat_sdmmc_mount_config_t mount_config = {};
  mount_config.format_if_mount_failed = false;
  mount_config.max_files = 5;
  mount_config.allocation_unit_size = 0;

  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  host.flags = SDMMC_HOST_FLAG_1BIT;
  host.max_freq_khz = SDMMC_FREQ_DEFAULT;

  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
  slot_config.width = 1;
  slot_config.clk = (gpio_num_t)PIN_SD_CLK;
  slot_config.cmd = (gpio_num_t)PIN_SD_CMD;
  slot_config.d0  = (gpio_num_t)PIN_SD_DAT0;
  slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

  esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &s_card);
  if (ret != ESP_OK) {
    ESP_LOGW(TAG, "SD mount attempt %u failed: 0x%x (%s)", (unsigned int)_probeAttempts, ret, esp_err_to_name(ret));
    if (s_card != nullptr) {
      esp_vfs_fat_sdcard_unmount("/sdcard", s_card);
      s_card = nullptr;
    }
    sdmmc_host_deinit();
    return false;
  }

  _cardSizeMB = ((uint64_t)s_card->csd.capacity) * s_card->csd.sector_size / (1024 * 1024);
  _initialized = true;
  _probeDropped = false;
  ESP_LOGI(TAG, "MicroSD card mounted at /sdcard (Capacity: %llu MB)", _cardSizeMB);
  ensureDirectories();
  return true;
}

bool SDManager::begin() {
  _initialized = false;
  _probeDropped = false;
  _cardSizeMB = 0;
  _probeAttempts = 1;
  _lastProbeMs = get_time_ms();

  ESP_LOGI(TAG, "Initializing SDMMC in 1-bit mode (Attempt 1/3, CLK: %d, CMD: %d, D0: %d)...",
           PIN_SD_CLK, PIN_SD_CMD, PIN_SD_DAT0);

  if (tryMount()) {
    return true;
  }

  return false;
}

bool SDManager::probe() {
  if (_initialized) return true;
  if (_probeDropped) return false;
  return isAvailable();
}

bool SDManager::isAvailable() const {
  if (_initialized) return true;
  if (_probeDropped || _probeAttempts >= 3) return false;

  uint32_t now = get_time_ms();
  if (now - _lastProbeMs >= 2000) {
    _lastProbeMs = now;
    auto *mutable_this = const_cast<SDManager*>(this);
    mutable_this->_probeAttempts++;
    ESP_LOGI(TAG, "Retrying SD card mount (Attempt %u/3)...", (unsigned int)_probeAttempts);

    if (mutable_this->tryMount()) {
      return true;
    }

    if (mutable_this->_probeAttempts >= 3) {
      mutable_this->_probeDropped = true;
      ESP_LOGW(TAG, "SD card not detected after 3 attempts. Dropping SD routine.");
    }
  }

  return _initialized;
}

uint64_t SDManager::getCardSizeMB() const {
  return _cardSizeMB;
}

uint64_t SDManager::getUsedBytes() const {
  if (!_initialized) return 0;
  FATFS *fs;
  DWORD fre_clust;
  if (f_getfree("0:", &fre_clust, &fs) == FR_OK) {
    uint64_t total_sectors = (fs->n_fatent - 2) * fs->csize;
    uint64_t free_sectors = fre_clust * fs->csize;
    return (total_sectors - free_sectors) * 512;
  }
  return 0;
}

void SDManager::ensureDirectories() {
  if (!_initialized) return;
  mkdir("/sdcard/tracks", 0755);
  mkdir("/sdcard/logs", 0755);
}

sdmmc_card_t *SDManager::getCard() const {
  return s_card;
}

void SDManager::pauseForUSB() {
  // Do not free s_card! TinyUSB storage_sdmmc needs valid sdmmc_card_t handle
  _initialized = false;
}

void SDManager::resumeAfterUSB() {
  tryMount();
}
