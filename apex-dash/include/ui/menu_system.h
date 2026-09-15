#pragma once

#include <Arduino.h>
#include <U8g2lib.h>
#include "telemetry_data.h"
#include "input_manager.h"
#include "i18n.h"

enum MenuState : uint8_t {
  MENU_ROOT = 0,
  MENU_RACE_SETUP,
  MENU_LEDS_ALARMS,
  MENU_TRACK_GPS,
  MENU_STORAGE_PC,
  MENU_DISPLAY_PWM,
  MENU_SYSTEM_LANG,
  MENU_DIAGNOSTICS_COUNTERS,
  MENU_USB_MSC_SCREEN
};

class TrackManager;
class LEDStripManager;
class BacklightManager;
class USBStorageManager;
class SDManager;

class MenuSystem {
public:
  void begin(TrackManager *trackMgr, LEDStripManager *ledMgr, BacklightManager *blMgr, USBStorageManager *usbMgr, SDManager *sdMgr);
  bool handleInput(UserInputEvent event, SystemSettings &settings);
  void render(U8G2 *u8g2, const SystemSettings &settings, const TelemetrySnapshot &telemetry);

  bool isMenuActive() const { return _active; }
  bool isMSCActive() const { return _current_state == MENU_USB_MSC_SCREEN; }
  void openMenu() { _active = true; _current_state = MENU_ROOT; _cursor_idx = 0; }
  void closeMenu() { _active = false; }

private:
  bool _active = false;
  MenuState _current_state = MENU_ROOT;
  int8_t _cursor_idx = 0;

  TrackManager *_trackMgr = nullptr;
  LEDStripManager *_ledMgr = nullptr;
  BacklightManager *_blMgr = nullptr;
  USBStorageManager *_usbMgr = nullptr;
  SDManager *_sdMgr = nullptr;

  void renderRootMenu(U8G2 *u8g2);
  void renderRaceSetupMenu(U8G2 *u8g2, const SystemSettings &settings);
  void renderLedsAlarmsMenu(U8G2 *u8g2, const SystemSettings &settings);
  void renderTrackGpsMenu(U8G2 *u8g2, const SystemSettings &settings);
  void renderStoragePcMenu(U8G2 *u8g2, const SystemSettings &settings);
  void renderDisplayPwmMenu(U8G2 *u8g2, const SystemSettings &settings);
  void renderSystemLangMenu(U8G2 *u8g2, const SystemSettings &settings);
  void renderDiagnosticsCountersMenu(U8G2 *u8g2, const TelemetrySnapshot &telemetry);
  void renderUsbMscScreen(U8G2 *u8g2);
};
