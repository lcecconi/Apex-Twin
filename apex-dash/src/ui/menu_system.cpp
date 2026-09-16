#include "ui/menu_system.h"
#include "ui/icons_xbm.h"
#include "telemetry_provider.h"
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

static void cycleAlarmPriority(SystemSettings &settings, uint8_t rank) {
  if (rank >= 5) return;
  uint8_t curr = settings.alarm_priority[rank];
  uint8_t next = (curr + 1) % 5;
  for (int i = 0; i < 5; i++) {
    if (i != rank && settings.alarm_priority[i] == next) {
      settings.alarm_priority[i] = curr;
      break;
    }
  }
  settings.alarm_priority[rank] = next;
}

bool MenuSystem::handleInput(UserInputEvent event, SystemSettings &settings, TelemetryProvider *provider) {
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

  // Handle Edit Mode Adjustments
  if (_edit_mode) {
    if (event == INPUT_NEXT) {
      // Increase parameter (+500 RPM / +step)
      if (_current_state == MENU_RACE_SETUP) {
        if (_cursor_idx == 1) { // Max RPM (8000..22000 in 500 steps)
          if (settings.max_rpm < 22000) settings.max_rpm += 500;
        } else if (_cursor_idx == 2) { // Shift RPM (6000..20000 in 500 steps)
          if (settings.shift_rpm < 20000) settings.shift_rpm += 500;
        } else if (_cursor_idx == 3) { // Over-Rev Limit (8000..22000 in 500 steps)
          if (settings.over_rev_rpm < 22000) settings.over_rev_rpm += 500;
        }
      } else if (_current_state == MENU_LEDS_ALARMS) {
        if (_cursor_idx == 0) { // LED Brightness (10..100 in 10 steps)
          if (settings.led_brightness < 100) settings.led_brightness += 10;
          if (_ledMgr) _ledMgr->setBrightness(settings.led_brightness);
        } else if (_cursor_idx == 5) { // Water Temp Alarm (40..95 in 5 steps)
          if (settings.water_temp_alarm_c < 95.0f) settings.water_temp_alarm_c += 5.0f;
        } else if (_cursor_idx == 6) { // Over-Rev Alarm (8000..22000 in 500 steps)
          if (settings.over_rev_rpm < 22000) settings.over_rev_rpm += 500;
        }
      } else if (_current_state == MENU_DISPLAY_PWM) {
        if (_cursor_idx == 0) { // Backlight PWM (0..100 in 10 steps)
          if (settings.backlight_percent < 100) settings.backlight_percent += 10;
          if (_blMgr) _blMgr->setBrightness(settings.backlight_percent);
        }
      }
      return true;
    } else if (event == INPUT_PREV) {
      // Decrease parameter (-500 RPM / -step)
      if (_current_state == MENU_RACE_SETUP) {
        if (_cursor_idx == 1) { // Max RPM
          if (settings.max_rpm > 8000) settings.max_rpm -= 500;
        } else if (_cursor_idx == 2) { // Shift RPM
          if (settings.shift_rpm > 6000) settings.shift_rpm -= 500;
        } else if (_cursor_idx == 3) { // Over-Rev Limit
          if (settings.over_rev_rpm > 8000) settings.over_rev_rpm -= 500;
        }
      } else if (_current_state == MENU_LEDS_ALARMS) {
        if (_cursor_idx == 0) { // LED Brightness
          if (settings.led_brightness > 10) settings.led_brightness -= 10;
          if (_ledMgr) _ledMgr->setBrightness(settings.led_brightness);
        } else if (_cursor_idx == 5) { // Water Temp Alarm
          if (settings.water_temp_alarm_c > 40.0f) settings.water_temp_alarm_c -= 5.0f;
        } else if (_cursor_idx == 6) { // Over-Rev Alarm
          if (settings.over_rev_rpm > 8000) settings.over_rev_rpm -= 500;
        }
      } else if (_current_state == MENU_DISPLAY_PWM) {
        if (_cursor_idx == 0) { // Backlight PWM
          if (settings.backlight_percent > 0) settings.backlight_percent -= 10;
          if (_blMgr) _blMgr->setBrightness(settings.backlight_percent);
        }
      }
      return true;
    } else if (event == INPUT_SELECT || event == INPUT_BACK_MENU) {
      _edit_mode = false;
      return true;
    }
  }

  // Handle Global Back
  if (event == INPUT_BACK_MENU) {
    if (_current_state == MENU_ROOT) {
      closeMenu();
    } else if (_current_state == MENU_WARN_TRIGGERS) {
      _current_state = MENU_LEDS_ALARMS;
      _cursor_idx = 7;
    } else if (_current_state == MENU_ALARM_PRIORITY) {
      _current_state = MENU_LEDS_ALARMS;
      _cursor_idx = 8;
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
    int max_items = 5;
    if (event == INPUT_NEXT) {
      _cursor_idx = (_cursor_idx + 1) % max_items;
    } else if (event == INPUT_PREV) {
      _cursor_idx = (_cursor_idx - 1 + max_items) % max_items;
    } else if (event == INPUT_SELECT) {
      if (_cursor_idx == 0) {
        settings.drive_type = (DriveType)((settings.drive_type + 1) % 3);
      } else if (_cursor_idx == 1 || _cursor_idx == 2 || _cursor_idx == 3) {
        _edit_mode = true;
      } else {
        _current_state = MENU_ROOT;
        _cursor_idx = 0;
      }
    }
    return true;
  }

  // --- 2. SHIFT LIGHTS & ALARMS ---
  if (_current_state == MENU_LEDS_ALARMS) {
    int max_items = 10;
    if (event == INPUT_NEXT) {
      _cursor_idx = (_cursor_idx + 1) % max_items;
    } else if (event == INPUT_PREV) {
      _cursor_idx = (_cursor_idx - 1 + max_items) % max_items;
    } else if (event == INPUT_SELECT) {
      if (_cursor_idx == 0) {
        _edit_mode = true;
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
        _edit_mode = true;
      } else if (_cursor_idx == 6) {
        _edit_mode = true;
      } else if (_cursor_idx == 7) {
        _current_state = MENU_WARN_TRIGGERS;
        _cursor_idx = 0;
      } else if (_cursor_idx == 8) {
        _current_state = MENU_ALARM_PRIORITY;
        _cursor_idx = 0;
      } else {
        _current_state = MENU_ROOT;
        _cursor_idx = 1;
      }
    }
    return true;
  }

  // --- 2b. WARN BLINKING TRIGGERS SUBMENU ---
  if (_current_state == MENU_WARN_TRIGGERS) {
    int max_items = 8;
    if (event == INPUT_NEXT) {
      _cursor_idx = (_cursor_idx + 1) % max_items;
    } else if (event == INPUT_PREV) {
      _cursor_idx = (_cursor_idx - 1 + max_items) % max_items;
    } else if (event == INPUT_SELECT) {
      if (_cursor_idx == 0) {
        settings.warn_trigger_water = !settings.warn_trigger_water;
      } else if (_cursor_idx == 1) {
        settings.warn_trigger_egt = !settings.warn_trigger_egt;
      } else if (_cursor_idx == 2) {
        settings.warn_trigger_rev = !settings.warn_trigger_rev;
      } else if (_cursor_idx == 3) {
        settings.warn_trigger_battery = !settings.warn_trigger_battery;
      } else if (_cursor_idx == 4) {
        settings.warn_trigger_link = !settings.warn_trigger_link;
      } else if (_cursor_idx == 5) {
        // Enable All
        settings.warn_trigger_water = true;
        settings.warn_trigger_egt = true;
        settings.warn_trigger_rev = true;
        settings.warn_trigger_battery = true;
        settings.warn_trigger_link = true;
      } else if (_cursor_idx == 6) {
        // Disable All
        settings.warn_trigger_water = false;
        settings.warn_trigger_egt = false;
        settings.warn_trigger_rev = false;
        settings.warn_trigger_battery = false;
        settings.warn_trigger_link = false;
      } else {
        _current_state = MENU_LEDS_ALARMS;
        _cursor_idx = 7;
      }
    }
    return true;
  }

  // --- 2c. ALARM PRIORITY / SEVERITY SUBMENU ---
  if (_current_state == MENU_ALARM_PRIORITY) {
    int max_items = 7;
    if (event == INPUT_NEXT) {
      _cursor_idx = (_cursor_idx + 1) % max_items;
    } else if (event == INPUT_PREV) {
      _cursor_idx = (_cursor_idx - 1 + max_items) % max_items;
    } else if (event == INPUT_SELECT) {
      if (_cursor_idx >= 0 && _cursor_idx < 5) {
        cycleAlarmPriority(settings, (uint8_t)_cursor_idx);
      } else if (_cursor_idx == 5) {
        for (uint8_t i = 0; i < 5; i++) {
          settings.alarm_priority[i] = i;
        }
      } else {
        _current_state = MENU_LEDS_ALARMS;
        _cursor_idx = 8;
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
    int max_items = 6;
    if (event == INPUT_NEXT) {
      _cursor_idx = (_cursor_idx + 1) % max_items;
    } else if (event == INPUT_PREV) {
      _cursor_idx = (_cursor_idx - 1 + max_items) % max_items;
    } else if (event == INPUT_SELECT) {
      if (_cursor_idx == 0) {
        _edit_mode = true;
      } else if (_cursor_idx == 1) {
        settings.show_speed = !settings.show_speed;
      } else if (_cursor_idx == 2) {
        settings.inverted_display = !settings.inverted_display;
      } else if (_cursor_idx == 3) {
        settings.use_kmh = !settings.use_kmh;
      } else if (_cursor_idx == 4) {
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
    int max_items = 2;
    if (event == INPUT_NEXT) {
      _cursor_idx = (_cursor_idx + 1) % max_items;
    } else if (event == INPUT_PREV) {
      _cursor_idx = (_cursor_idx - 1 + max_items) % max_items;
    } else if (event == INPUT_SELECT) {
      if (_cursor_idx == 0) {
        if (provider) provider->resetEngineHours();
      } else {
        _current_state = MENU_ROOT;
        _cursor_idx = 6;
      }
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
    case MENU_WARN_TRIGGERS: renderWarnTriggersMenu(u8g2, settings); break;
    case MENU_ALARM_PRIORITY: renderAlarmPriorityMenu(u8g2, settings); break;
  }


  // Footer Navigation
  if (_current_state != MENU_USB_MSC_SCREEN) {
    u8g2->drawHLine(0, 276, 400);
    u8g2->setFont(u8g2_font_6x10_tr);
    if (_edit_mode) {
      u8g2->drawStr(8, 292, "KEY: + / Inc (+500) | BOOT: - / Dec (-500) | Long: Save");
    } else {
      u8g2->drawStr(8, 292, "KEY: Next | BOOT: Prev | Long KEY: Edit/Select | Long BOOT: Back");
    }
  }
}

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

  const uint8_t *icons[ROOT_MENU_COUNT] = {
    icon_gear_16x16,
    icon_lightbulb_16x16,
    icon_flag_16x16,
    icon_sdcard_16x16,
    icon_sun_16x16,
    icon_globe_16x16,
    icon_wrench_16x16,
    icon_back_16x16
  };

  for (int i = 0; i < ROOT_MENU_COUNT; i++) {
    int y = 50 + (i * 28);
    if (i == _cursor_idx) {
      u8g2->drawRBox(10, y - 18, 380, 24, 3);
      u8g2->setDrawColor(0);
      u8g2->drawXBMP(18, y - 14, 16, 16, icons[i]);
      u8g2->setFont(u8g2_font_helvB10_tr);
      u8g2->drawStr(42, y, items[i]);
      u8g2->drawStr(365, y, ">");
      u8g2->setDrawColor(1);
    } else {
      u8g2->drawXBMP(18, y - 14, 16, 16, icons[i]);
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

  char b0[64], b1[64], b2[64], b3[64];
  snprintf(b0, sizeof(b0), "%s: %s", I18n::get(STR_DRIVE_TYPE), drive_str);
  if (_edit_mode && _cursor_idx == 1) {
    snprintf(b1, sizeof(b1), "%s: [- %u RPM +]", I18n::get(STR_MAX_RPM), settings.max_rpm);
  } else {
    snprintf(b1, sizeof(b1), "%s: %u RPM", I18n::get(STR_MAX_RPM), settings.max_rpm);
  }
  if (_edit_mode && _cursor_idx == 2) {
    snprintf(b2, sizeof(b2), "%s: [- %u RPM +]", I18n::get(STR_SHIFT_RPM), settings.shift_rpm);
  } else {
    snprintf(b2, sizeof(b2), "%s: %u RPM", I18n::get(STR_SHIFT_RPM), settings.shift_rpm);
  }
  if (_edit_mode && _cursor_idx == 3) {
    snprintf(b3, sizeof(b3), "Over-Rev Alarm: [- %u RPM +]", settings.over_rev_rpm);
  } else {
    snprintf(b3, sizeof(b3), "Over-Rev Alarm: %u RPM", settings.over_rev_rpm);
  }

  const char *items[5] = { b0, b1, b2, b3, "< Return >" };

  for (int i = 0; i < 5; i++) {
    int y = 74 + (i * 32);
    if (i == _cursor_idx) {
      u8g2->drawRBox(12, y - 20, 376, 26, 4);
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
  u8g2->drawStr(12, 44, I18n::get(STR_CAT_RPM_ALARM));

  const char *rpm_mode_str = (settings.rpm_display_mode == RPM_DISP_BOTH) ? I18n::get(STR_RPM_DISP_BOTH) :
                             ((settings.rpm_display_mode == RPM_DISP_DISPLAY_ONLY) ? I18n::get(STR_RPM_DISP_DISPLAY) : I18n::get(STR_RPM_DISP_LEDS));

  char b0[64], b1[64], b2[64], b3[64], b4[64], b5[64], b6[64];
  if (_edit_mode && _cursor_idx == 0) {
    snprintf(b0, sizeof(b0), "%s: [- %d%% +]", I18n::get(STR_LED_BRIGHTNESS), settings.led_brightness);
  } else {
    snprintf(b0, sizeof(b0), "%s: [%d%%]", I18n::get(STR_LED_BRIGHTNESS), settings.led_brightness);
  }
  snprintf(b1, sizeof(b1), "%s: [%s]", I18n::get(STR_RPM_DISP_MODE), rpm_mode_str);
  snprintf(b2, sizeof(b2), "%s [Click to run]", I18n::get(STR_LED_TEST));
  snprintf(b3, sizeof(b3), "Shift LEDs (5x): [%s]", settings.led_shift_enable ? "ENABLED" : "OFF");
  snprintf(b4, sizeof(b4), "Alarm LEDs (2x): [%s]", settings.led_alarm_enable ? "ENABLED" : "OFF");
  if (_edit_mode && _cursor_idx == 5) {
    snprintf(b5, sizeof(b5), "%s: [- %.0f \xb0\x43 +]", I18n::get(STR_WATER_ALARM), settings.water_temp_alarm_c);
  } else {
    snprintf(b5, sizeof(b5), "%s: [%.0f \xb0\x43]", I18n::get(STR_WATER_ALARM), settings.water_temp_alarm_c);
  }
  if (_edit_mode && _cursor_idx == 6) {
    snprintf(b6, sizeof(b6), "Over-Rev Alarm: [- %u RPM +]", settings.over_rev_rpm);
  } else {
    snprintf(b6, sizeof(b6), "Over-Rev Alarm: [%u RPM]", settings.over_rev_rpm);
  }

  const char *items[10] = { b0, b1, b2, b3, b4, b5, b6, "WARN Alert Triggers >", "Alarm Priority / Severity >", "< Return >" };

  for (int i = 0; i < 10; i++) {
    int y = 62 + (i * 21);
    if (i == _cursor_idx) {
      u8g2->drawRBox(12, y - 15, 376, 19, 3);
      u8g2->setDrawColor(0);
      u8g2->drawStr(24, y, items[i]);
      u8g2->setDrawColor(1);
    } else {
      u8g2->drawStr(24, y, items[i]);
    }
  }
}

void MenuSystem::renderWarnTriggersMenu(U8G2 *u8g2, const SystemSettings &settings) {
  u8g2->setFont(u8g2_font_helvB10_tr);
  u8g2->drawStr(12, 44, "WARN BLINKING TRIGGERS");

  char b0[64], b1[64], b2[64], b3[64], b4[64];
  snprintf(b0, sizeof(b0), "WARN on Water Temp:   [%s]", settings.warn_trigger_water ? "ENABLED" : "OFF");
  snprintf(b1, sizeof(b1), "WARN on Exhaust (EGT): [%s]", settings.warn_trigger_egt ? "ENABLED" : "OFF");
  snprintf(b2, sizeof(b2), "WARN on Over-Rev:      [%s]", settings.warn_trigger_rev ? "ENABLED" : "OFF");
  snprintf(b3, sizeof(b3), "WARN on Low Battery:   [%s]", settings.warn_trigger_battery ? "ENABLED" : "OFF");
  snprintf(b4, sizeof(b4), "WARN on Link Lost:     [%s]", settings.warn_trigger_link ? "ENABLED" : "OFF");

  const char *items[8] = {
    b0, b1, b2, b3, b4,
    "[ Enable All Triggers ]",
    "[ Disable All Triggers ]",
    "< Return to Alarms Menu >"
  };

  for (int i = 0; i < 8; i++) {
    int y = 66 + (i * 26);
    if (i == _cursor_idx) {
      u8g2->drawRBox(12, y - 17, 376, 22, 3);
      u8g2->setDrawColor(0);
      u8g2->drawStr(24, y, items[i]);
      u8g2->setDrawColor(1);
    } else {
      u8g2->drawStr(24, y, items[i]);
    }
  }
}

void MenuSystem::renderAlarmPriorityMenu(U8G2 *u8g2, const SystemSettings &settings) {
  u8g2->setFont(u8g2_font_helvB10_tr);
  u8g2->drawStr(12, 44, "ALARM SEVERITY PRIORITY");

  static const char* const alarm_names[5] = {
    "Water Temp (H2O)",
    "Exhaust Temp (EGT)",
    "Over-Rev (RPM)",
    "Low Battery",
    "Link Lost"
  };

  char b0[64], b1[64], b2[64], b3[64], b4[64];
  char* bufs[5] = { b0, b1, b2, b3, b4 };
  for (int i = 0; i < 5; i++) {
    uint8_t aid = settings.alarm_priority[i];
    const char *name = (aid < 5) ? alarm_names[aid] : "Unknown";
    snprintf(bufs[i], 64, "#%d (Priority %d): [%s]", i + 1, i + 1, name);
  }

  const char *items[7] = {
    b0, b1, b2, b3, b4,
    "[ Reset Priority Order ]",
    "< Return to Alarms Menu >"
  };

  for (int i = 0; i < 7; i++) {
    int y = 68 + (i * 28);
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

  char b0[64], b1[64], b2[64], b3[64], b4[64];
  if (_edit_mode && _cursor_idx == 0) {
    snprintf(b0, sizeof(b0), "%s: [- %d%% +]", I18n::get(STR_BACKLIGHT_PWM), settings.backlight_percent);
  } else {
    snprintf(b0, sizeof(b0), "%s (GPIO 2): [%d%%]", I18n::get(STR_BACKLIGHT_PWM), settings.backlight_percent);
  }
  snprintf(b1, sizeof(b1), "%s: [%s]", I18n::get(STR_SHOW_SPEED), settings.show_speed ? "ENABLED" : "OFF");
  snprintf(b2, sizeof(b2), "%s: [%s]", I18n::get(STR_INVERT_DISP), settings.inverted_display ? "Black on Silver" : "Silver on Black");
  snprintf(b3, sizeof(b3), "%s: [%s]", I18n::get(STR_UNITS_SPEED), settings.use_kmh ? "KM/H" : "MPH");
  snprintf(b4, sizeof(b4), "%s: [%s]", I18n::get(STR_UNITS_TEMP), settings.use_celsius ? "\xb0\x43" : "\xb0\x46");

  const char *items[6] = { b0, b1, b2, b3, b4, "< Return >" };

  for (int i = 0; i < 6; i++) {
    int y = 70 + (i * 28);
    if (i == _cursor_idx) {
      u8g2->drawRBox(12, y - 18, 376, 22, 3);
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

  // Button 0: Reset Engine Hours
  if (_cursor_idx == 0) {
    u8g2->drawRBox(12, 192, 376, 26, 4);
    u8g2->setDrawColor(0);
    u8g2->setFont(u8g2_font_helvB10_tr);
    u8g2->drawStr(120, 210, "[ Reset Engine Hours ]");
    u8g2->setDrawColor(1);
  } else {
    u8g2->drawRFrame(12, 192, 376, 26, 4);
    u8g2->setFont(u8g2_font_helvB10_tr);
    u8g2->drawStr(120, 210, "Reset Engine Hours");
  }

  // Button 1: Return to Main Menu
  if (_cursor_idx == 1) {
    u8g2->drawRBox(12, 222, 376, 26, 4);
    u8g2->setDrawColor(0);
    u8g2->setFont(u8g2_font_helvB10_tr);
    u8g2->drawStr(120, 240, "< Return to Main Menu >");
    u8g2->setDrawColor(1);
  } else {
    u8g2->drawRFrame(12, 222, 376, 26, 4);
    u8g2->setFont(u8g2_font_helvB10_tr);
    u8g2->drawStr(120, 240, "< Return to Main Menu >");
  }
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
