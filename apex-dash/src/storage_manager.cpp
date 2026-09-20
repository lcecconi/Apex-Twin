#include "storage_manager.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "cJSON.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>

#define TAG "STORAGE_MGR"
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

  size_t f_len = sizeof(float);
  nvs_get_blob(s_nvs_handle, "alm_water_c", &settings.water_temp_alarm_c, &f_len);
  f_len = sizeof(float);
  nvs_get_blob(s_nvs_handle, "alm_egt_c", &settings.exhaust_temp_alarm_c, &f_len);
  f_len = sizeof(float);
  nvs_get_blob(s_nvs_handle, "alm_bat_v", &settings.low_bat_alarm_v, &f_len);

  if (nvs_get_u8(s_nvs_handle, "lap_hold", &u8val) == ESP_OK) settings.lap_hold_seconds = u8val;

  size_t str_len = sizeof(settings.selected_track);
  nvs_get_str(s_nvs_handle, "track", settings.selected_track, &str_len);

  size_t track_file_len = sizeof(settings.selected_track_file);
  nvs_get_str(s_nvs_handle, "track_file", settings.selected_track_file, &track_file_len);

  size_t ssid_len = sizeof(settings.wifi_ssid);
  nvs_get_str(s_nvs_handle, "wifi_ssid", settings.wifi_ssid, &ssid_len);

  size_t pass_len = sizeof(settings.wifi_pass);
  nvs_get_str(s_nvs_handle, "wifi_pass", settings.wifi_pass, &pass_len);
}

void StorageManager::saveSettings(const SystemSettings &settings) {
  if (s_nvs_open) {
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
    nvs_set_blob(s_nvs_handle, "alm_water_c", &settings.water_temp_alarm_c, sizeof(float));
    nvs_set_blob(s_nvs_handle, "alm_egt_c", &settings.exhaust_temp_alarm_c, sizeof(float));
    nvs_set_blob(s_nvs_handle, "alm_bat_v", &settings.low_bat_alarm_v, sizeof(float));
    nvs_set_u8(s_nvs_handle, "lap_hold", settings.lap_hold_seconds);
    nvs_set_str(s_nvs_handle, "track", settings.selected_track);
    nvs_set_str(s_nvs_handle, "track_file", settings.selected_track_file);
    nvs_set_str(s_nvs_handle, "wifi_ssid", settings.wifi_ssid);
    nvs_set_str(s_nvs_handle, "wifi_pass", settings.wifi_pass);

    nvs_commit(s_nvs_handle);
  }

  // Also save copy to SD card if mounted
  saveSettingsToSD(settings);
}

bool StorageManager::loadSettingsFromSD(SystemSettings &settings) {
  FILE *f = fopen("/sdcard/config.json", "r");
  if (!f) return false;

  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (len <= 0 || len > 32768) {
    fclose(f);
    return false;
  }

  char *buf = (char *)malloc(len + 1);
  if (!buf) {
    fclose(f);
    return false;
  }

  size_t read_bytes = fread(buf, 1, len, f);
  fclose(f);
  buf[read_bytes] = '\0';

  cJSON *root = cJSON_Parse(buf);
  free(buf);
  if (!root) {
    ESP_LOGW(TAG, "Failed to parse /sdcard/config.json");
    return false;
  }

  // Parse System block
  cJSON *system = cJSON_GetObjectItem(root, "system");
  if (system) {
    cJSON *item = cJSON_GetObjectItem(system, "language");
    if (item && cJSON_IsNumber(item)) settings.language = (uint8_t)item->valueint;
    item = cJSON_GetObjectItem(system, "simulation_mode");
    if (item && cJSON_IsBool(item)) settings.simulation_mode = cJSON_IsTrue(item);
    item = cJSON_GetObjectItem(system, "inverted_display");
    if (item && cJSON_IsBool(item)) settings.inverted_display = cJSON_IsTrue(item);
    item = cJSON_GetObjectItem(system, "backlight_percent");
    if (item && cJSON_IsNumber(item)) settings.backlight_percent = (uint8_t)item->valueint;
  }

  // Parse Drivetrain block
  cJSON *drivetrain = cJSON_GetObjectItem(root, "drivetrain");
  if (drivetrain) {
    cJSON *item = cJSON_GetObjectItem(drivetrain, "drive_type");
    if (item && cJSON_IsNumber(item)) settings.drive_type = (DriveType)item->valueint;
    item = cJSON_GetObjectItem(drivetrain, "max_rpm");
    if (item && cJSON_IsNumber(item)) settings.max_rpm = (uint16_t)item->valueint;
    item = cJSON_GetObjectItem(drivetrain, "shift_rpm");
    if (item && cJSON_IsNumber(item)) settings.shift_rpm = (uint16_t)item->valueint;
    item = cJSON_GetObjectItem(drivetrain, "over_rev_rpm");
    if (item && cJSON_IsNumber(item)) settings.over_rev_rpm = (uint16_t)item->valueint;
    item = cJSON_GetObjectItem(drivetrain, "rpm_display_mode");
    if (item && cJSON_IsNumber(item)) settings.rpm_display_mode = (RpmDisplayMode)item->valueint;
  }

  // Parse Units block
  cJSON *units = cJSON_GetObjectItem(root, "units");
  if (units) {
    cJSON *item = cJSON_GetObjectItem(units, "use_kmh");
    if (item && cJSON_IsBool(item)) settings.use_kmh = cJSON_IsTrue(item);
    item = cJSON_GetObjectItem(units, "use_celsius");
    if (item && cJSON_IsBool(item)) settings.use_celsius = cJSON_IsTrue(item);
    item = cJSON_GetObjectItem(units, "show_speed");
    if (item && cJSON_IsBool(item)) settings.show_speed = cJSON_IsTrue(item);
  }

  // Parse LEDs block
  cJSON *leds = cJSON_GetObjectItem(root, "leds");
  if (leds) {
    cJSON *item = cJSON_GetObjectItem(leds, "brightness");
    if (item && cJSON_IsNumber(item)) settings.led_brightness = (uint8_t)item->valueint;
    item = cJSON_GetObjectItem(leds, "shift_enable");
    if (item && cJSON_IsBool(item)) settings.led_shift_enable = cJSON_IsTrue(item);
    item = cJSON_GetObjectItem(leds, "alarm_enable");
    if (item && cJSON_IsBool(item)) settings.led_alarm_enable = cJSON_IsTrue(item);
  }

  // Parse Alarms block
  cJSON *alarms = cJSON_GetObjectItem(root, "alarms");
  if (alarms) {
    cJSON *item = cJSON_GetObjectItem(alarms, "water_temp_c");
    if (item && cJSON_IsNumber(item)) settings.water_temp_alarm_c = (float)item->valuedouble;
    item = cJSON_GetObjectItem(alarms, "exhaust_temp_c");
    if (item && cJSON_IsNumber(item)) settings.exhaust_temp_alarm_c = (float)item->valuedouble;
    item = cJSON_GetObjectItem(alarms, "low_battery_v");
    if (item && cJSON_IsNumber(item)) settings.low_bat_alarm_v = (float)item->valuedouble;
    item = cJSON_GetObjectItem(alarms, "trigger_water");
    if (item && cJSON_IsBool(item)) settings.warn_trigger_water = cJSON_IsTrue(item);
    item = cJSON_GetObjectItem(alarms, "trigger_egt");
    if (item && cJSON_IsBool(item)) settings.warn_trigger_egt = cJSON_IsTrue(item);
    item = cJSON_GetObjectItem(alarms, "trigger_rev");
    if (item && cJSON_IsBool(item)) settings.warn_trigger_rev = cJSON_IsTrue(item);
    item = cJSON_GetObjectItem(alarms, "trigger_battery");
    if (item && cJSON_IsBool(item)) settings.warn_trigger_battery = cJSON_IsTrue(item);
    item = cJSON_GetObjectItem(alarms, "trigger_link");
    if (item && cJSON_IsBool(item)) settings.warn_trigger_link = cJSON_IsTrue(item);

    cJSON *prio_arr = cJSON_GetObjectItem(alarms, "priority");
    if (prio_arr && cJSON_IsArray(prio_arr)) {
      int count = cJSON_GetArraySize(prio_arr);
      if (count > 5) count = 5;
      for (int i = 0; i < count; i++) {
        cJSON *elem = cJSON_GetArrayItem(prio_arr, i);
        if (elem && cJSON_IsNumber(elem)) {
          settings.alarm_priority[i] = (uint8_t)elem->valueint;
        }
      }
    }
  }

  // Parse Track block
  cJSON *track = cJSON_GetObjectItem(root, "track");
  if (track) {
    cJSON *item = cJSON_GetObjectItem(track, "track_mode");
    if (item && cJSON_IsNumber(item)) settings.track_mode = (TrackDetectionMode)item->valueint;
    item = cJSON_GetObjectItem(track, "selected_track");
    if (item && cJSON_IsString(item) && item->valuestring) {
      strncpy(settings.selected_track, item->valuestring, sizeof(settings.selected_track) - 1);
      settings.selected_track[sizeof(settings.selected_track) - 1] = '\0';
    }
    item = cJSON_GetObjectItem(track, "selected_track_file");
    if (item && cJSON_IsString(item) && item->valuestring) {
      strncpy(settings.selected_track_file, item->valuestring, sizeof(settings.selected_track_file) - 1);
      settings.selected_track_file[sizeof(settings.selected_track_file) - 1] = '\0';
    }
    item = cJSON_GetObjectItem(track, "lap_hold_seconds");
    if (item && cJSON_IsNumber(item)) settings.lap_hold_seconds = (uint8_t)item->valueint;
  }

  // Parse WiFi block
  cJSON *wifi = cJSON_GetObjectItem(root, "wifi");
  if (wifi) {
    cJSON *item = cJSON_GetObjectItem(wifi, "ssid");
    if (item && cJSON_IsString(item) && item->valuestring) {
      strncpy(settings.wifi_ssid, item->valuestring, sizeof(settings.wifi_ssid) - 1);
      settings.wifi_ssid[sizeof(settings.wifi_ssid) - 1] = '\0';
    }
    item = cJSON_GetObjectItem(wifi, "password");
    if (item && cJSON_IsString(item) && item->valuestring) {
      strncpy(settings.wifi_pass, item->valuestring, sizeof(settings.wifi_pass) - 1);
      settings.wifi_pass[sizeof(settings.wifi_pass) - 1] = '\0';
    }
  }

  cJSON_Delete(root);
  return true;
}

bool StorageManager::saveSettingsToSD(const SystemSettings &settings) {
  struct stat st;
  if (stat("/sdcard", &st) != 0) return false;

  cJSON *root = cJSON_CreateObject();
  if (!root) return false;

  // System
  cJSON *system = cJSON_CreateObject();
  cJSON_AddNumberToObject(system, "language", settings.language);
  cJSON_AddBoolToObject(system, "simulation_mode", settings.simulation_mode);
  cJSON_AddBoolToObject(system, "inverted_display", settings.inverted_display);
  cJSON_AddNumberToObject(system, "backlight_percent", settings.backlight_percent);
  cJSON_AddItemToObject(root, "system", system);

  // Drivetrain
  cJSON *drivetrain = cJSON_CreateObject();
  cJSON_AddNumberToObject(drivetrain, "drive_type", (int)settings.drive_type);
  cJSON_AddNumberToObject(drivetrain, "max_rpm", settings.max_rpm);
  cJSON_AddNumberToObject(drivetrain, "shift_rpm", settings.shift_rpm);
  cJSON_AddNumberToObject(drivetrain, "over_rev_rpm", settings.over_rev_rpm);
  cJSON_AddNumberToObject(drivetrain, "rpm_display_mode", (int)settings.rpm_display_mode);
  cJSON_AddItemToObject(root, "drivetrain", drivetrain);

  // Units
  cJSON *units = cJSON_CreateObject();
  cJSON_AddBoolToObject(units, "use_kmh", settings.use_kmh);
  cJSON_AddBoolToObject(units, "use_celsius", settings.use_celsius);
  cJSON_AddBoolToObject(units, "show_speed", settings.show_speed);
  cJSON_AddItemToObject(root, "units", units);

  // LEDs
  cJSON *leds = cJSON_CreateObject();
  cJSON_AddNumberToObject(leds, "brightness", settings.led_brightness);
  cJSON_AddBoolToObject(leds, "shift_enable", settings.led_shift_enable);
  cJSON_AddBoolToObject(leds, "alarm_enable", settings.led_alarm_enable);
  cJSON_AddItemToObject(root, "leds", leds);

  // Alarms
  cJSON *alarms = cJSON_CreateObject();
  cJSON_AddNumberToObject(alarms, "water_temp_c", settings.water_temp_alarm_c);
  cJSON_AddNumberToObject(alarms, "exhaust_temp_c", settings.exhaust_temp_alarm_c);
  cJSON_AddNumberToObject(alarms, "low_battery_v", settings.low_bat_alarm_v);
  cJSON_AddBoolToObject(alarms, "trigger_water", settings.warn_trigger_water);
  cJSON_AddBoolToObject(alarms, "trigger_egt", settings.warn_trigger_egt);
  cJSON_AddBoolToObject(alarms, "trigger_rev", settings.warn_trigger_rev);
  cJSON_AddBoolToObject(alarms, "trigger_battery", settings.warn_trigger_battery);
  cJSON_AddBoolToObject(alarms, "trigger_link", settings.warn_trigger_link);
  int prio_ints[5];
  for (int i = 0; i < 5; i++) prio_ints[i] = settings.alarm_priority[i];
  cJSON *prio_arr = cJSON_CreateIntArray(prio_ints, 5);
  cJSON_AddItemToObject(alarms, "priority", prio_arr);
  cJSON_AddItemToObject(root, "alarms", alarms);

  // Track
  cJSON *track = cJSON_CreateObject();
  cJSON_AddNumberToObject(track, "track_mode", (int)settings.track_mode);
  cJSON_AddStringToObject(track, "selected_track", settings.selected_track);
  cJSON_AddStringToObject(track, "selected_track_file", settings.selected_track_file);
  cJSON_AddNumberToObject(track, "lap_hold_seconds", settings.lap_hold_seconds);
  cJSON_AddItemToObject(root, "track", track);

  // WiFi
  cJSON *wifi = cJSON_CreateObject();
  cJSON_AddStringToObject(wifi, "ssid", settings.wifi_ssid);
  cJSON_AddStringToObject(wifi, "password", settings.wifi_pass);
  cJSON_AddItemToObject(root, "wifi", wifi);

  char *json_str = cJSON_Print(root);
  cJSON_Delete(root);

  if (!json_str) return false;

  FILE *f = fopen("/sdcard/config.json", "w");
  if (!f) {
    free(json_str);
    return false;
  }

  fputs(json_str, f);
  fclose(f);
  free(json_str);
  ESP_LOGI(TAG, "Saved configuration to /sdcard/config.json");
  return true;
}

void StorageManager::syncSDCard(SystemSettings &settings) {
  struct stat st;
  if (stat("/sdcard", &st) != 0) return;

  if (stat("/sdcard/config.json", &st) == 0) {
    ESP_LOGI(TAG, "Found /sdcard/config.json - applying overrides...");
    if (loadSettingsFromSD(settings)) {
      saveSettings(settings);
    }
  } else {
    ESP_LOGI(TAG, "Writing initial /sdcard/config.json to MicroSD...");
    saveSettingsToSD(settings);
  }
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

