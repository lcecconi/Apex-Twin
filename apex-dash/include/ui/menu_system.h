#pragma once

#include <cstdint>
#include "U8g2lib.h"
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
  MENU_USB_MSC_SCREEN,
  MENU_WARN_TRIGGERS,
  MENU_ALARM_PRIORITY,
  MENU_OTA_SCREEN
};

class TrackManager;
class LEDStripManager;
class BacklightManager;
class USBStorageManager;
class SDManager;
class TelemetryProvider;
class StorageManager;

class MenuSystem {
public:
  void begin(TrackManager *trackMgr, LEDStripManager *ledMgr, BacklightManager *blMgr, USBStorageManager *usbMgr, SDManager *sdMgr, StorageManager *storageMgr = nullptr);
  bool handleInput(UserInputEvent event, SystemSettings &settings, TelemetryProvider *provider = nullptr);
  void render(U8G2 *u8g2, const SystemSettings &settings, const TelemetrySnapshot &telemetry);

  bool isMenuActive() const { return _active; }
  bool isMSCActive() const { return _current_state == MENU_USB_MSC_SCREEN; }
  bool isOtaActive() const { return _current_state == MENU_OTA_SCREEN; }
  void openMenu(const SystemSettings &settings) {
    _active = true;
    _current_state = MENU_ROOT;
    _cursor_idx = 0;
    _settings_on_open = settings;
  }
  void closeMenu(const SystemSettings &settings);

private:
  bool _active = false;
  bool _edit_mode = false;
  MenuState _current_state = MENU_ROOT;
  int8_t _cursor_idx = 0;

  TrackManager *_trackMgr = nullptr;
  LEDStripManager *_ledMgr = nullptr;
  BacklightManager *_blMgr = nullptr;
  USBStorageManager *_usbMgr = nullptr;
  SDManager *_sdMgr = nullptr;
  StorageManager *_storageMgr = nullptr;
  SystemSettings _settings_on_open;

  void renderRootMenu(U8G2 *u8g2);
  void renderRaceSetupMenu(U8G2 *u8g2, const SystemSettings &settings);
  void renderLedsAlarmsMenu(U8G2 *u8g2, const SystemSettings &settings);
  void renderTrackGpsMenu(U8G2 *u8g2, const SystemSettings &settings);
  void renderStoragePcMenu(U8G2 *u8g2, const SystemSettings &settings);
  void renderDisplayPwmMenu(U8G2 *u8g2, const SystemSettings &settings);
  void renderSystemLangMenu(U8G2 *u8g2, const SystemSettings &settings);
  void renderDiagnosticsCountersMenu(U8G2 *u8g2, const TelemetrySnapshot &telemetry);
  void renderUsbMscScreen(U8G2 *u8g2);
  void renderWarnTriggersMenu(U8G2 *u8g2, const SystemSettings &settings);
  void renderAlarmPriorityMenu(U8G2 *u8g2, const SystemSettings &settings);
  void renderOtaScreen(U8G2 *u8g2);
};
