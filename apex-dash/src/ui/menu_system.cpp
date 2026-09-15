#include "ui/menu_system.h"
#include "track_manager.h"
#include "led_strip_manager.h"
#include "backlight_manager.h"
#include "usb_storage_manager.h"
#include "sd_manager.h"
#include <esp_system.h>

#define ROOT_MENU_COUNT 8

void MenuSystem::begin(TrackManager *trackMgr, LEDStripManager *ledMgr, BacklightManager *blMgr, USBStorageManager *usbMgr, SDManager *sdMgr) {
  _trackMgr = trackMgr;
  _ledMgr = ledMgr;
  _blMgr = blMgr;
  _usbMgr = usbMgr;
  _sdMgr = sdMgr;
  _active = false;
  _current_state = MENU_ROOT;
  _cursor_idx = 0;
}

bool MenuSystem::handleInput(UserInputEvent event, SystemSettings &settings) {
  if (!_active) return false;

  // Handle USB MSC active screen escape
  if (_current_state == MENU_USB_MSC_SCREEN) {
    if (event == INPUT_SELECT || event == INPUT_BACK_MENU) {
      if (_usbMgr) _usbMgr->stopMSCMode();
      _current_state = MENU_STORAGE_PC;
      _cursor_idx = 0;
      return true;
    }
    return true;
  }

  // Handle Global Back
  if (event == INPUT_BACK_MENU) {
    if (_current_state == MENU_ROOT) {
      closeMenu();
    } else {
      _current_state = MENU_ROOT;
      _cursor_idx = 0;
    }
    return true;
  }

  // --- ROOT MENU ---
  if (_current_state == MENU_ROOT) {
    if (event == INPUT_NEXT) {
      _cursor_idx = (_cursor_idx + 1) % ROOT_MENU_COUNT;
    } else if (event == INPUT_PREV) {
      _cursor_idx = (_cursor_idx - 1 + ROOT_MENU_COUNT) % ROOT_MENU_COUNT;
    } else if (event == INPUT_SELECT) {
      switch (_cursor_idx) {
        case 0: _current_state = MENU_RACE_SETUP; _cursor_idx = 0; break;
        case 1: _current_state = MENU_LEDS_ALARMS; _cursor_idx = 0; break;
        case 2: _current_state = MENU_TRACK_GPS; _cursor_idx = 0; break;
        case 3: _current_state = MENU_STORAGE_PC; _cursor_idx = 0; break;
        case 4: _current_state = MENU_DISPLAY_PWM; _cursor_idx = 0; break;
        case 5: _current_state = MENU_SYSTEM_LANG; _cursor_idx = 0; break;
        case 6: _current_state = MENU_DIAGNOSTICS_COUNTERS; _cursor_idx = 0; break;
        case 7: closeMenu(); break;
      }
    }
    return true;
  }

  // --- 1. RACE & KART SETUP ---
  if (_current_state == MENU_RACE_SETUP) {
    int max_items = 4;
    if (event == INPUT_NEXT) {
      _cursor_idx = (_cursor_idx + 1) % max_items;
    } else if (event == INPUT_PREV) {
      _cursor_idx = (_cursor_idx - 1 + max_items) % max_items;
    } else if (event == INPUT_SELECT) {
      if (_cursor_idx == 0) {
        settings.drive_type = (DriveType)((settings.drive_type + 1) % 3);
      } else if (_cursor_idx == 1) {
        settings.max_rpm = (settings.max_rpm >= 20000) ? 14000 : (settings.max_rpm + 2000);
      } else if (_cursor_idx == 2) {
        settings.shift_rpm = (settings.shift_rpm >= 16000) ? 12000 : (settings.shift_rpm + 500);
      } else {
        _current_state = MENU_ROOT;
        _cursor_idx = 0;
      }
    }
    return true;
  }

  // --- 2. SHIFT LIGHTS & ALARMS ---
  if (_current_state == MENU_LEDS_ALARMS) {
    int max_items = 7;
    if (event == INPUT_NEXT) {
      _cursor_idx = (_cursor_idx + 1) % max_items;
    } else if (event == INPUT_PREV) {
      _cursor_idx = (_cursor_idx - 1 + max_items) % max_items;
    } else if (event == INPUT_SELECT) {
      if (_cursor_idx == 0) {
        // LED Brightness: 20% -> 40% -> 60% -> 80% -> 100%
        settings.led_brightness = (settings.led_brightness >= 100) ? 20 : (settings.led_brightness + 20);
        if (_ledMgr) _ledMgr->setBrightness(settings.led_brightness);
      } else if (_cursor_idx == 1) {
        // RPM Display Mode: Both -> Display Only -> LEDs Only
        settings.rpm_display_mode = (RpmDisplayMode)((settings.rpm_display_mode + 1) % 3);
      } else if (_cursor_idx == 2) {
        // Run test pattern on RGB LEDs
        if (_ledMgr) _ledMgr->runTestPattern();
      } else if (_cursor_idx == 3) {
        settings.led_shift_enable = !settings.led_shift_enable;
      } else if (_cursor_idx == 4) {
        settings.led_alarm_enable = !settings.led_alarm_enable;
      } else if (_cursor_idx == 5) {
        // Water temp alarm threshold: 55 -> 60 -> 65 -> 70 -> 75
        settings.water_temp_alarm_c = (settings.water_temp_alarm_c >= 75.0f) ? 55.0f : (settings.water_temp_alarm_c + 5.0f);
      } else {
        _current_state = MENU_ROOT;
        _cursor_idx = 1;
      }
    }
    return true;
  }


  // --- 3. TRACK & GPS DATABASE ---
  if (_current_state == MENU_TRACK_GPS) {
    size_t track_count = _trackMgr ? _trackMgr->getTrackCount() : 0;
    int max_items = track_count + 2; // tracks + Reload SD + Return
    if (event == INPUT_NEXT) {
      _cursor_idx = (_cursor_idx + 1) % max_items;
    } else if (event == INPUT_PREV) {
      _cursor_idx = (_cursor_idx - 1 + max_items) % max_items;
    } else if (event == INPUT_SELECT) {
      if (_cursor_idx < (int)track_count) {
        if (_trackMgr) {
          _trackMgr->setActiveTrack(_cursor_idx);
          const TrackDefinition *t = _trackMgr->getActiveTrack();
          if (t) {
            strncpy(settings.selected_track, t->name, sizeof(settings.selected_track) - 1);
          }
        }
        _current_state = MENU_ROOT;
        _cursor_idx = 2;
      } else if (_cursor_idx == (int)track_count) {
        // Reload SD tracks
        if (_trackMgr) _trackMgr->loadTracksFromSD();
      } else {
        _current_state = MENU_ROOT;
        _cursor_idx = 2;
      }
    }
    return true;
  }

  // --- 4. STORAGE & PC SYNC ---
  if (_current_state == MENU_STORAGE_PC) {
    int max_items = 2;
    if (event == INPUT_NEXT) {
      _cursor_idx = (_cursor_idx + 1) % max_items;
    } else if (event == INPUT_PREV) {
      _cursor_idx = (_cursor_idx - 1 + max_items) % max_items;
    } else if (event == INPUT_SELECT) {
      if (_cursor_idx == 0) {
        // Start USB MSC Mode
        if (_usbMgr) _usbMgr->startMSCMode();
        _current_state = MENU_USB_MSC_SCREEN;
      } else {
        _current_state = MENU_ROOT;
        _cursor_idx = 3;
      }
    }
    return true;
  }

  // --- 5. DISPLAY & BACKLIGHT ---
  if (_current_state == MENU_DISPLAY_PWM) {
    int max_items = 5;
    if (event == INPUT_NEXT) {
      _cursor_idx = (_cursor_idx + 1) % max_items;
    } else if (event == INPUT_PREV) {
      _cursor_idx = (_cursor_idx - 1 + max_items) % max_items;
    } else if (event == INPUT_SELECT) {
      if (_cursor_idx == 0) {
        // Backlight PWM: 0% -> 25% -> 50% -> 75% -> 100%
        settings.backlight_percent = (settings.backlight_percent >= 100) ? 0 : (settings.backlight_percent + 25);
        if (_blMgr) _blMgr->setBrightness(settings.backlight_percent);
      } else if (_cursor_idx == 1) {
        settings.inverted_display = !settings.inverted_display;
      } else if (_cursor_idx == 2) {
        settings.use_kmh = !settings.use_kmh;
      } else if (_cursor_idx == 3) {
        settings.use_celsius = !settings.use_celsius;
      } else {
        _current_state = MENU_ROOT;
        _cursor_idx = 4;
      }
    }
    return true;
  }

  // --- 6. SYSTEM & LANGUAGE ---
  if (_current_state == MENU_SYSTEM_LANG) {
    int max_items = 3;
    if (event == INPUT_NEXT) {
      _cursor_idx = (_cursor_idx + 1) % max_items;
    } else if (event == INPUT_PREV) {
      _cursor_idx = (_cursor_idx - 1 + max_items) % max_items;
    } else if (event == INPUT_SELECT) {
      if (_cursor_idx == 0) {
        // Cycle Language: ENG -> ITA -> FRA -> GER
        settings.language = (settings.language + 1) % LANG_COUNT;
        I18n::setLanguage((Language)settings.language);
      } else if (_cursor_idx == 1) {
        // Reset defaults
        settings = SystemSettings();
        I18n::setLanguage((Language)settings.language);
        if (_ledMgr) _ledMgr->setBrightness(settings.led_brightness);
        if (_blMgr) _blMgr->setBrightness(settings.backlight_percent);
      } else {
        _current_state = MENU_ROOT;
        _cursor_idx = 5;
      }
    }
    return true;
  }

  // --- 7. DIAGNOSTICS & COUNTERS ---
  if (_current_state == MENU_DIAGNOSTICS_COUNTERS) {
    if (event == INPUT_SELECT) {
      _current_state = MENU_ROOT;
      _cursor_idx = 6;
    }
    return true;
  }

  return true;
}

void MenuSystem::render(U8G2 *u8g2, const SystemSettings &settings, const TelemetrySnapshot &telemetry) {
  u8g2->clearBuffer();

  // Header Title Bar
  u8g2->drawBox(0, 0, 400, 24);
  u8g2->setDrawColor(0);
  u8g2->setFont(u8g2_font_helvB10_tr);
  u8g2->drawStr(8, 17, I18n::get(STR_MENU_TITLE));
  u8g2->setFont(u8g2_font_6x10_tr);
  u8g2->drawStr(300, 17, "[APEX-DASH]");
  u8g2->setDrawColor(1);

  switch (_current_state) {
    case MENU_ROOT: renderRootMenu(u8g2); break;
    case MENU_RACE_SETUP: renderRaceSetupMenu(u8g2, settings); break;
    case MENU_LEDS_ALARMS: renderLedsAlarmsMenu(u8g2, settings); break;
    case MENU_TRACK_GPS: renderTrackGpsMenu(u8g2, settings); break;
    case MENU_STORAGE_PC: renderStoragePcMenu(u8g2, settings); break;
    case MENU_DISPLAY_PWM: renderDisplayPwmMenu(u8g2, settings); break;
    case MENU_SYSTEM_LANG: renderSystemLangMenu(u8g2, settings); break;
    case MENU_DIAGNOSTICS_COUNTERS: renderDiagnosticsCountersMenu(u8g2, telemetry); break;
    case MENU_USB_MSC_SCREEN: renderUsbMscScreen(u8g2); break;
  }

  // Footer Navigation
  if (_current_state != MENU_USB_MSC_SCREEN) {
    u8g2->drawHLine(0, 276, 400);
    u8g2->setFont(u8g2_font_6x10_tr);
    u8g2->drawStr(8, 292, "KEY: Next/Change | BOOT: Select | Long BOOT: Back");
  }
}

struct MenuIconGlyph {
  const uint8_t *font;
  uint16_t glyph;
};

void MenuSystem::renderRootMenu(U8G2 *u8g2) {
  const char *items[ROOT_MENU_COUNT] = {
    I18n::get(STR_CAT_RACE_CONFIG),
    I18n::get(STR_CAT_RPM_ALARM),
    I18n::get(STR_CAT_TRACK_GPS),
    I18n::get(STR_CAT_STORAGE_PC),
    I18n::get(STR_CAT_DISPLAY_PWM),
    I18n::get(STR_CAT_SYSTEM_LANG),
    I18n::get(STR_CAT_SENSORS_INFO),
    "< Exit Menu >"
  };

  const MenuIconGlyph icons[ROOT_MENU_COUNT] = {
    { u8g2_font_open_iconic_embedded_2x_t, 66 }, // Gear / Setup
    { u8g2_font_open_iconic_thing_2x_t,    65 }, // Lightbulb / LED
    { u8g2_font_open_iconic_thing_2x_t,    66 }, // Flag
    { u8g2_font_open_iconic_embedded_2x_t, 69 }, // Storage / Disk
    { u8g2_font_open_iconic_weather_2x_t,  69 }, // Sun / Contrast
    { u8g2_font_open_iconic_app_2x_t,      68 }, // Globe / System
    { u8g2_font_open_iconic_app_2x_t,      77 }, // Wrench / Tools
    { u8g2_font_open_iconic_gui_2x_t,      69 }  // Exit / Back
  };

  for (int i = 0; i < ROOT_MENU_COUNT; i++) {
    int y = 50 + (i * 28);
    if (i == _cursor_idx) {
      u8g2->drawRBox(10, y - 18, 380, 24, 3);
      u8g2->setDrawColor(0);
      u8g2->setFont(icons[i].font);
      u8g2->drawGlyph(18, y + 1, icons[i].glyph);
      u8g2->setFont(u8g2_font_helvB10_tr);
      u8g2->drawStr(42, y, items[i]);
      u8g2->drawStr(365, y, ">");
      u8g2->setDrawColor(1);
    } else {
      u8g2->setFont(icons[i].font);
      u8g2->drawGlyph(18, y + 1, icons[i].glyph);
      u8g2->setFont(u8g2_font_helvB10_tr);
      u8g2->drawStr(42, y, items[i]);
    }
  }
}


void MenuSystem::renderRaceSetupMenu(U8G2 *u8g2, const SystemSettings &settings) {
  u8g2->setFont(u8g2_font_helvB10_tr);
  u8g2->drawStr(12, 46, I18n::get(STR_CAT_RACE_CONFIG));

  const char *drive_str = (settings.drive_type == DRIVE_SHIFTER_6SPEED) ? I18n::get(STR_DRIVE_SHIFTER) :
                          ((settings.drive_type == DRIVE_CLUTCH) ? I18n::get(STR_DRIVE_CLUTCH) : I18n::get(STR_DRIVE_DIRECT));

  char b0[64], b1[64], b2[64];
  snprintf(b0, sizeof(b0), "%s: %s", I18n::get(STR_DRIVE_TYPE), drive_str);
  snprintf(b1, sizeof(b1), "%s: %u RPM", I18n::get(STR_MAX_RPM), settings.max_rpm);
  snprintf(b2, sizeof(b2), "%s: %u RPM", I18n::get(STR_SHIFT_RPM), settings.shift_rpm);

  const char *items[4] = { b0, b1, b2, "< Return >" };

  for (int i = 0; i < 4; i++) {
    int y = 84 + (i * 38);
    if (i == _cursor_idx) {
      u8g2->drawRBox(12, y - 22, 376, 32, 4);
      u8g2->setDrawColor(0);
      u8g2->drawStr(24, y, items[i]);
      u8g2->setDrawColor(1);
    } else {
      u8g2->drawStr(24, y, items[i]);
    }
  }
}

void MenuSystem::renderLedsAlarmsMenu(U8G2 *u8g2, const SystemSettings &settings) {
  u8g2->setFont(u8g2_font_helvB10_tr);
  u8g2->drawStr(12, 46, I18n::get(STR_CAT_RPM_ALARM));

  const char *rpm_mode_str = (settings.rpm_display_mode == RPM_DISP_BOTH) ? I18n::get(STR_RPM_DISP_BOTH) :
                             ((settings.rpm_display_mode == RPM_DISP_DISPLAY_ONLY) ? I18n::get(STR_RPM_DISP_DISPLAY) : I18n::get(STR_RPM_DISP_LEDS));

  char b0[64], b1[64], b2[64], b3[64], b4[64], b5[64];
  snprintf(b0, sizeof(b0), "%s: [%d%%]", I18n::get(STR_LED_BRIGHTNESS), settings.led_brightness);
  snprintf(b1, sizeof(b1), "%s: [%s]", I18n::get(STR_RPM_DISP_MODE), rpm_mode_str);
  snprintf(b2, sizeof(b2), "%s [Click to run]", I18n::get(STR_LED_TEST));
  snprintf(b3, sizeof(b3), "Shift LEDs (5x): [%s]", settings.led_shift_enable ? "ENABLED" : "OFF");
  snprintf(b4, sizeof(b4), "Alarm LEDs (2x): [%s]", settings.led_alarm_enable ? "ENABLED" : "OFF");
  snprintf(b5, sizeof(b5), "%s: [%.0f \xb0\x43]", I18n::get(STR_WATER_ALARM), settings.water_temp_alarm_c);

  const char *items[7] = { b0, b1, b2, b3, b4, b5, "< Return >" };

  for (int i = 0; i < 7; i++) {
    int y = 70 + (i * 28);
    if (i == _cursor_idx) {
      u8g2->drawRBox(12, y - 18, 376, 24, 3);
      u8g2->setDrawColor(0);
      u8g2->drawStr(24, y, items[i]);
      u8g2->setDrawColor(1);
    } else {
      u8g2->drawStr(24, y, items[i]);
    }
  }
}


void MenuSystem::renderTrackGpsMenu(U8G2 *u8g2, const SystemSettings &settings) {
  u8g2->setFont(u8g2_font_helvB10_tr);
  u8g2->drawStr(12, 46, I18n::get(STR_CAT_TRACK_GPS));

  size_t count = _trackMgr ? _trackMgr->getTrackCount() : 0;
  int total_items = count + 2;

  for (int i = 0; i < total_items; i++) {
    int y = 74 + (i * 26);
    if (y > 270) break; // Screen limit

    char label[64];
    bool is_active = false;

    if (i < (int)count) {
      const TrackDefinition *t = _trackMgr->getTrack(i);
      snprintf(label, sizeof(label), "%s (%s)", t->name, t->is_custom_sd ? "SD" : "ROM");
      is_active = (strcmp(settings.selected_track, t->name) == 0);
    } else if (i == (int)count) {
      snprintf(label, sizeof(label), "[%s]", I18n::get(STR_TRACK_SD_LOAD));
    } else {
      snprintf(label, sizeof(label), "< Return >");
    }

    if (i == _cursor_idx) {
      u8g2->drawRBox(12, y - 18, 376, 22, 3);
      u8g2->setDrawColor(0);
      u8g2->drawStr(24, y, label);
      if (is_active) u8g2->drawStr(340, y, "[*]");
      u8g2->setDrawColor(1);
    } else {
      u8g2->drawStr(24, y, label);
      if (is_active) u8g2->drawStr(340, y, "[*]");
    }
  }
}

void MenuSystem::renderStoragePcMenu(U8G2 *u8g2, const SystemSettings &settings) {
  u8g2->setFont(u8g2_font_helvB10_tr);
  u8g2->drawStr(12, 46, I18n::get(STR_CAT_STORAGE_PC));

  u8g2->setFont(u8g2_font_6x12_tr);
  char buf[64];
  if (_sdMgr && _sdMgr->isAvailable()) {
    snprintf(buf, sizeof(buf), "MicroSD Status:  INSERTED (%llu MB)", _sdMgr->getCardSizeMB());
    u8g2->drawStr(24, 75, buf);
    u8g2->drawStr(24, 98, "Filesystem:      FAT32 (/tracks, /logs)");
  } else {
    u8g2->drawStr(24, 75, "MicroSD Status:  NO CARD DETECTED");
    u8g2->drawStr(24, 98, "Fallback:        Internal 16MB Flash Partition");
  }

  const char *items[2] = {
    I18n::get(STR_USB_MSC_START),
    "< Return to Main Menu >"
  };

  u8g2->setFont(u8g2_font_helvB10_tr);
  for (int i = 0; i < 2; i++) {
    int y = 145 + (i * 45);
    if (i == _cursor_idx) {
      u8g2->drawRBox(12, y - 22, 376, 36, 4);
      u8g2->setDrawColor(0);
      u8g2->drawStr(24, y, items[i]);
      u8g2->setDrawColor(1);
    } else {
      u8g2->drawStr(24, y, items[i]);
    }
  }
}

void MenuSystem::renderDisplayPwmMenu(U8G2 *u8g2, const SystemSettings &settings) {
  u8g2->setFont(u8g2_font_helvB10_tr);
  u8g2->drawStr(12, 46, I18n::get(STR_CAT_DISPLAY_PWM));

  char b0[64], b1[64], b2[64], b3[64];
  snprintf(b0, sizeof(b0), "%s (GPIO 2): [%d%%]", I18n::get(STR_BACKLIGHT_PWM), settings.backlight_percent);
  snprintf(b1, sizeof(b1), "%s: [%s]", I18n::get(STR_INVERT_DISP), settings.inverted_display ? "Black on Silver" : "Silver on Black");
  snprintf(b2, sizeof(b2), "%s: [%s]", I18n::get(STR_UNITS_SPEED), settings.use_kmh ? "KM/H" : "MPH");
  snprintf(b3, sizeof(b3), "%s: [%s]", I18n::get(STR_UNITS_TEMP), settings.use_celsius ? "\xb0\x43" : "\xb0\x46");

  const char *items[5] = { b0, b1, b2, b3, "< Return >" };

  for (int i = 0; i < 5; i++) {
    int y = 78 + (i * 35);
    if (i == _cursor_idx) {
      u8g2->drawRBox(12, y - 20, 376, 28, 3);
      u8g2->setDrawColor(0);
      u8g2->drawStr(24, y, items[i]);
      u8g2->setDrawColor(1);
    } else {
      u8g2->drawStr(24, y, items[i]);
    }
  }
}

void MenuSystem::renderSystemLangMenu(U8G2 *u8g2, const SystemSettings &settings) {
  u8g2->setFont(u8g2_font_helvB10_tr);
  u8g2->drawStr(12, 46, I18n::get(STR_CAT_SYSTEM_LANG));

  char b0[64];
  snprintf(b0, sizeof(b0), "%s: [%s]", I18n::get(STR_LANGUAGE), I18n::getLanguageName((Language)settings.language));

  const char *items[3] = {
    b0,
    I18n::get(STR_RESET_CONFIG),
    "< Return to Main Menu >"
  };

  for (int i = 0; i < 3; i++) {
    int y = 90 + (i * 45);
    if (i == _cursor_idx) {
      u8g2->drawRBox(12, y - 22, 376, 36, 4);
      u8g2->setDrawColor(0);
      u8g2->drawStr(24, y, items[i]);
      u8g2->setDrawColor(1);
    } else {
      u8g2->drawStr(24, y, items[i]);
    }
  }
}

void MenuSystem::renderDiagnosticsCountersMenu(U8G2 *u8g2, const TelemetrySnapshot &telemetry) {
  u8g2->setFont(u8g2_font_helvB10_tr);
  u8g2->drawStr(12, 46, I18n::get(STR_CAT_SENSORS_INFO));

  uint32_t eng_hrs = telemetry.engine_total_hours_sec / 3600;
  uint32_t eng_min = (telemetry.engine_total_hours_sec % 3600) / 60;
  uint32_t pis_hrs = telemetry.piston_hours_sec / 3600;
  uint32_t pis_min = (telemetry.piston_hours_sec % 3600) / 60;

  char buf[64];
  u8g2->setFont(u8g2_font_6x12_tr);
  snprintf(buf, sizeof(buf), "Engine Hours:    %lu hrs %02lu min", (unsigned long)eng_hrs, (unsigned long)eng_min);
  u8g2->drawStr(24, 75, buf);

  snprintf(buf, sizeof(buf), "Piston Rebuild:  %lu hrs %02lu min (15h limit)", (unsigned long)pis_hrs, (unsigned long)pis_min);
  u8g2->drawStr(24, 98, buf);

  snprintf(buf, sizeof(buf), "Battery:         %.2f V (%d%%) on GPIO 4", telemetry.battery_voltage, telemetry.battery_percent);
  u8g2->drawStr(24, 121, buf);

  snprintf(buf, sizeof(buf), "Sensirion SHTC3: %+.1f \xb0\x43  |  %0.1f %% RH", telemetry.ambient_temp_c, telemetry.ambient_humidity_pct);
  u8g2->drawStr(24, 144, buf);

  snprintf(buf, sizeof(buf), "ESP32-S3 Memory: Heap %luKB | PSRAM %luKB",
           (unsigned long)(ESP.getFreeHeap() / 1024),
           (unsigned long)(ESP.getFreePsram() / 1024));
  u8g2->drawStr(24, 167, buf);

  u8g2->drawRBox(12, 215, 376, 32, 4);
  u8g2->setDrawColor(0);
  u8g2->setFont(u8g2_font_helvB10_tr);
  u8g2->drawStr(120, 237, "< Return to Main Menu >");
  u8g2->setDrawColor(1);
}

void MenuSystem::renderUsbMscScreen(U8G2 *u8g2) {
  u8g2->drawRFrame(20, 45, 360, 200, 8);

  u8g2->setFont(u8g2_font_helvB12_tr);
  u8g2->drawStr(60, 80, I18n::get(STR_STATUS_USB_MSC_ACTIVE));

  u8g2->setFont(u8g2_font_6x12_tr);
  u8g2->drawStr(40, 115, "1. Connect USB-C cable to your PC / Laptop");
  u8g2->drawStr(40, 140, "2. Access /tracks/*.json and /logs/ CSV files");
  u8g2->drawStr(40, 165, "3. Safely Eject drive when finished");

  u8g2->drawRBox(50, 195, 300, 32, 4);
  u8g2->setDrawColor(0);
  u8g2->setFont(u8g2_font_helvB10_tr);
  u8g2->drawStr(80, 217, "PRESS BOOT/KEY TO EXIT");
  u8g2->setDrawColor(1);
}
