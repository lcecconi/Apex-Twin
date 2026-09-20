#include "storage_manager.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <cstring>

#define PREFS_NAMESPACE "apex_dash"

static nvs_handle_t s_nvs_handle = 0;
static bool s_nvs_open = false;

void StorageManager::begin() {
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_flash_erase();
    nvs_flash_init();
  }
  if (nvs_open(PREFS_NAMESPACE, NVS_READWRITE, &s_nvs_handle) == ESP_OK) {
    s_nvs_open = true;
  }
}

void StorageManager::loadSettings(SystemSettings &settings) {
  if (!s_nvs_open) return;

  uint8_t init_val = 0;
  if (nvs_get_u8(s_nvs_handle, "init", &init_val) != ESP_OK) {
    saveSettings(settings);
    nvs_set_u8(s_nvs_handle, "init", 1);
    nvs_commit(s_nvs_handle);
    return;
  }

  uint8_t u8val = 0;
  uint16_t u16val = 0;

  if (nvs_get_u8(s_nvs_handle, "drive_type", &u8val) == ESP_OK) settings.drive_type = (DriveType)u8val;
  if (nvs_get_u16(s_nvs_handle, "max_rpm", &u16val) == ESP_OK) settings.max_rpm = u16val;
  if (nvs_get_u16(s_nvs_handle, "shift_rpm", &u16val) == ESP_OK) settings.shift_rpm = u16val;
  if (nvs_get_u16(s_nvs_handle, "over_rev_rpm", &u16val) == ESP_OK) settings.over_rev_rpm = u16val;

  if (nvs_get_u8(s_nvs_handle, "use_kmh", &u8val) == ESP_OK) settings.use_kmh = (u8val != 0);
  if (nvs_get_u8(s_nvs_handle, "use_celsius", &u8val) == ESP_OK) settings.use_celsius = (u8val != 0);
  if (nvs_get_u8(s_nvs_handle, "show_spd", &u8val) == ESP_OK) settings.show_speed = (u8val != 0);
  if (nvs_get_u8(s_nvs_handle, "inverted", &u8val) == ESP_OK) settings.inverted_display = (u8val != 0);
  if (nvs_get_u8(s_nvs_handle, "sim_mode", &u8val) == ESP_OK) settings.simulation_mode = (u8val != 0);
  if (nvs_get_u8(s_nvs_handle, "lang", &u8val) == ESP_OK) settings.language = u8val;
  if (nvs_get_u8(s_nvs_handle, "led_bright", &u8val) == ESP_OK) settings.led_brightness = u8val;
  if (nvs_get_u8(s_nvs_handle, "rpm_disp", &u8val) == ESP_OK) settings.rpm_display_mode = (RpmDisplayMode)u8val;
  if (nvs_get_u8(s_nvs_handle, "led_shift_en", &u8val) == ESP_OK) settings.led_shift_enable = (u8val != 0);
  if (nvs_get_u8(s_nvs_handle, "led_alarm_en", &u8val) == ESP_OK) settings.led_alarm_enable = (u8val != 0);
  if (nvs_get_u8(s_nvs_handle, "backlight_pct", &u8val) == ESP_OK) settings.backlight_percent = u8val;
  if (nvs_get_u8(s_nvs_handle, "w_water", &u8val) == ESP_OK) settings.warn_trigger_water = (u8val != 0);
  if (nvs_get_u8(s_nvs_handle, "w_egt", &u8val) == ESP_OK) settings.warn_trigger_egt = (u8val != 0);
  if (nvs_get_u8(s_nvs_handle, "w_rev", &u8val) == ESP_OK) settings.warn_trigger_rev = (u8val != 0);
  if (nvs_get_u8(s_nvs_handle, "w_bat", &u8val) == ESP_OK) settings.warn_trigger_battery = (u8val != 0);
  if (nvs_get_u8(s_nvs_handle, "w_link", &u8val) == ESP_OK) settings.warn_trigger_link = (u8val != 0);

  size_t prio_len = sizeof(settings.alarm_priority);
  nvs_get_blob(s_nvs_handle, "alm_prio", settings.alarm_priority, &prio_len);

  size_t str_len = sizeof(settings.selected_track);
  nvs_get_str(s_nvs_handle, "track", settings.selected_track, &str_len);
}

void StorageManager::saveSettings(const SystemSettings &settings) {
  if (!s_nvs_open) return;

  nvs_set_u8(s_nvs_handle, "drive_type", (uint8_t)settings.drive_type);
  nvs_set_u16(s_nvs_handle, "max_rpm", settings.max_rpm);
  nvs_set_u16(s_nvs_handle, "shift_rpm", settings.shift_rpm);
  nvs_set_u16(s_nvs_handle, "over_rev_rpm", settings.over_rev_rpm);

  nvs_set_u8(s_nvs_handle, "use_kmh", settings.use_kmh ? 1 : 0);
  nvs_set_u8(s_nvs_handle, "use_celsius", settings.use_celsius ? 1 : 0);
  nvs_set_u8(s_nvs_handle, "show_spd", settings.show_speed ? 1 : 0);
  nvs_set_u8(s_nvs_handle, "inverted", settings.inverted_display ? 1 : 0);
  nvs_set_u8(s_nvs_handle, "sim_mode", settings.simulation_mode ? 1 : 0);
  nvs_set_u8(s_nvs_handle, "lang", settings.language);
  nvs_set_u8(s_nvs_handle, "led_bright", settings.led_brightness);
  nvs_set_u8(s_nvs_handle, "rpm_disp", (uint8_t)settings.rpm_display_mode);
  nvs_set_u8(s_nvs_handle, "led_shift_en", settings.led_shift_enable ? 1 : 0);
  nvs_set_u8(s_nvs_handle, "led_alarm_en", settings.led_alarm_enable ? 1 : 0);
  nvs_set_u8(s_nvs_handle, "backlight_pct", settings.backlight_percent);
  nvs_set_u8(s_nvs_handle, "w_water", settings.warn_trigger_water ? 1 : 0);
  nvs_set_u8(s_nvs_handle, "w_egt", settings.warn_trigger_egt ? 1 : 0);
  nvs_set_u8(s_nvs_handle, "w_rev", settings.warn_trigger_rev ? 1 : 0);
  nvs_set_u8(s_nvs_handle, "w_bat", settings.warn_trigger_battery ? 1 : 0);
  nvs_set_u8(s_nvs_handle, "w_link", settings.warn_trigger_link ? 1 : 0);

  nvs_set_blob(s_nvs_handle, "alm_prio", settings.alarm_priority, sizeof(settings.alarm_priority));
  nvs_set_str(s_nvs_handle, "track", settings.selected_track);

  nvs_commit(s_nvs_handle);
}

uint32_t StorageManager::getEngineHours() {
  if (!s_nvs_open) return 14 * 3600 + 18 * 60;
  uint32_t hrs = 14 * 3600 + 18 * 60;
  nvs_get_u32(s_nvs_handle, "eng_hrs_sec", &hrs);
  return hrs;
}

void StorageManager::saveEngineHours(uint32_t seconds) {
  if (!s_nvs_open) return;
  nvs_set_u32(s_nvs_handle, "eng_hrs_sec", seconds);
  nvs_commit(s_nvs_handle);
}

void StorageManager::resetEngineHours() {
  if (!s_nvs_open) return;
  nvs_set_u32(s_nvs_handle, "eng_hrs_sec", 0);
  nvs_commit(s_nvs_handle);
}
