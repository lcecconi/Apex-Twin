#include "storage_manager.h"

#define PREFS_NAMESPACE "apex_dash"

void StorageManager::begin() {
  _prefs.begin(PREFS_NAMESPACE, false);
}

void StorageManager::loadSettings(SystemSettings &settings) {
  if (!_prefs.isKey("init")) {
    saveSettings(settings); // First run: save factory defaults
    _prefs.putBool("init", true);
    return;
  }

  settings.drive_type = (DriveType)_prefs.getUChar("drive_type", (uint8_t)settings.drive_type);
  settings.max_rpm = _prefs.getUShort("max_rpm", settings.max_rpm);
  settings.shift_rpm = _prefs.getUShort("shift_rpm", settings.shift_rpm);
  settings.over_rev_rpm = _prefs.getUShort("over_rev_rpm", settings.over_rev_rpm);
  settings.water_temp_alarm_c = _prefs.getFloat("w_temp_alarm", settings.water_temp_alarm_c);
  settings.exhaust_temp_alarm_c = _prefs.getFloat("egt_alarm", settings.exhaust_temp_alarm_c);
  settings.use_kmh = _prefs.getBool("use_kmh", settings.use_kmh);
  settings.use_celsius = _prefs.getBool("use_celsius", settings.use_celsius);
  settings.show_speed = _prefs.getBool("show_spd", settings.show_speed);
  settings.inverted_display = _prefs.getBool("inverted", settings.inverted_display);
  settings.simulation_mode = _prefs.getBool("sim_mode", settings.simulation_mode);
  settings.language = _prefs.getUChar("lang", settings.language);
  settings.led_brightness = _prefs.getUChar("led_bright", settings.led_brightness);
  settings.rpm_display_mode = (RpmDisplayMode)_prefs.getUChar("rpm_disp", (uint8_t)settings.rpm_display_mode);
  settings.led_shift_enable = _prefs.getBool("led_shift_en", settings.led_shift_enable);
  settings.led_alarm_enable = _prefs.getBool("led_alarm_en", settings.led_alarm_enable);
  settings.backlight_percent = _prefs.getUChar("backlight_pct", settings.backlight_percent);
  settings.warn_trigger_water = _prefs.getBool("w_water", settings.warn_trigger_water);
  settings.warn_trigger_egt = _prefs.getBool("w_egt", settings.warn_trigger_egt);
  settings.warn_trigger_rev = _prefs.getBool("w_rev", settings.warn_trigger_rev);
  settings.warn_trigger_battery = _prefs.getBool("w_bat", settings.warn_trigger_battery);
  settings.warn_trigger_link = _prefs.getBool("w_link", settings.warn_trigger_link);

  String track = _prefs.getString("track", String(settings.selected_track));
  strncpy(settings.selected_track, track.c_str(), sizeof(settings.selected_track) - 1);

  String trackFile = _prefs.getString("track_file", String(settings.selected_track_file));
  strncpy(settings.selected_track_file, trackFile.c_str(), sizeof(settings.selected_track_file) - 1);
}

void StorageManager::saveSettings(const SystemSettings &settings) {
  _prefs.putUChar("drive_type", (uint8_t)settings.drive_type);
  _prefs.putUShort("max_rpm", settings.max_rpm);
  _prefs.putUShort("shift_rpm", settings.shift_rpm);
  _prefs.putUShort("over_rev_rpm", settings.over_rev_rpm);
  _prefs.putFloat("w_temp_alarm", settings.water_temp_alarm_c);
  _prefs.putFloat("egt_alarm", settings.exhaust_temp_alarm_c);
  _prefs.putBool("use_kmh", settings.use_kmh);
  _prefs.putBool("use_celsius", settings.use_celsius);
  _prefs.putBool("show_spd", settings.show_speed);
  _prefs.putBool("inverted", settings.inverted_display);
  _prefs.putBool("sim_mode", settings.simulation_mode);
  _prefs.putUChar("lang", settings.language);
  _prefs.putUChar("led_bright", settings.led_brightness);
  _prefs.putUChar("rpm_disp", (uint8_t)settings.rpm_display_mode);
  _prefs.putBool("led_shift_en", settings.led_shift_enable);
  _prefs.putBool("led_alarm_en", settings.led_alarm_enable);
  _prefs.putUChar("backlight_pct", settings.backlight_percent);
  _prefs.putBool("w_water", settings.warn_trigger_water);
  _prefs.putBool("w_egt", settings.warn_trigger_egt);
  _prefs.putBool("w_rev", settings.warn_trigger_rev);
  _prefs.putBool("w_bat", settings.warn_trigger_battery);
  _prefs.putBool("w_link", settings.warn_trigger_link);
  _prefs.putString("track", String(settings.selected_track));
  _prefs.putString("track_file", String(settings.selected_track_file));
}


