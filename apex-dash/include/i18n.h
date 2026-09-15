#pragma once

#include <Arduino.h>

enum Language : uint8_t {
  LANG_EN = 0, // English (Default)
  LANG_IT = 1, // Italian
  LANG_FR = 2, // French
  LANG_DE = 3, // German
  LANG_COUNT = 4
};

enum StrId : uint16_t {
  // Menu Navigation & Headers
  STR_MENU_TITLE = 0,
  STR_CAT_RACE_CONFIG,
  STR_CAT_RPM_ALARM,
  STR_CAT_TRACK_GPS,
  STR_CAT_STORAGE_PC,
  STR_CAT_DISPLAY_PWM,
  STR_CAT_SYSTEM_LANG,
  STR_CAT_SENSORS_INFO,

  // Menu Items & Options
  STR_DRIVE_TYPE,
  STR_SHIFT_RPM,
  STR_MAX_RPM,
  STR_OVERREV_RPM,
  STR_WATER_ALARM,
  STR_EGT_ALARM,
  STR_LED_BRIGHTNESS,
  STR_LED_TEST,
  STR_RPM_DISP_MODE,
  STR_RPM_DISP_BOTH,
  STR_RPM_DISP_DISPLAY,
  STR_RPM_DISP_LEDS,
  STR_BACKLIGHT_PWM,
  STR_TRACK_SELECT,
  STR_TRACK_SD_LOAD,
  STR_USB_MSC_START,
  STR_LANGUAGE,
  STR_INVERT_DISP,
  STR_UNITS_SPEED,
  STR_UNITS_TEMP,
  STR_RESET_CONFIG,

  // Drive Types
  STR_DRIVE_DIRECT,
  STR_DRIVE_CLUTCH,
  STR_DRIVE_SHIFTER,

  // HUD & Telemetry Labels
  STR_LABEL_SPEED,
  STR_LABEL_RPM,
  STR_LABEL_GEAR,
  STR_LABEL_LAP,
  STR_LABEL_BEST,
  STR_LABEL_LAST,
  STR_LABEL_DELTA,
  STR_LABEL_PRED,
  STR_LABEL_SECTOR,
  STR_LABEL_WATER,
  STR_LABEL_EGT,
  STR_LABEL_AIR,
  STR_LABEL_RH,
  STR_LABEL_SATS,
  STR_LABEL_BAT,
  STR_LABEL_HRS,

  // Warnings & Alarms
  STR_WARN_SHIFT,
  STR_WARN_OVERHEAT,
  STR_WARN_EGT,
  STR_WARN_OVERREV,
  STR_WARN_LOW_BAT,

  // System States
  STR_STATUS_SIMULATION,
  STR_STATUS_WIRELESS_OK,
  STR_STATUS_NO_SIGNAL,
  STR_STATUS_USB_MSC_ACTIVE,

  STR_MAX_STRINGS
};

class I18n {
public:
  static void setLanguage(Language lang);
  static Language getLanguage();
  static const char* getLanguageName(Language lang);
  static const char* get(StrId id);

private:
  static Language _currentLang;
};
