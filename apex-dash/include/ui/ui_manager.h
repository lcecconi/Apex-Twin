#pragma once

#include <Arduino.h>
#include <U8g2lib.h>
#include "telemetry_data.h"
#include "telemetry_provider.h"
#include "input_manager.h"
#include "ui/menu_system.h"
#include "ui/page_live_race.h"
#include "ui/page_telemetry.h"
#include "ui/page_gps_paddock.h"
#include "ui/page_data_recall.h"

enum UiViewMode : uint8_t {
  VIEW_LIVE_RACE = 0,    // Predictive Lap Time HUD
  VIEW_TELEMETRY,        // Tachometer, Dual Temps, G-G Diagram
  VIEW_GPS_PADDOCK,      // Satellite radar, Track detect, Maintenance
  VIEW_DATA_RECALL,      // Best 3 laps, sector breakdown
  VIEW_COUNT
};

class TrackManager;
class LEDStripManager;
class BacklightManager;
class USBStorageManager;
class SDManager;

class UiManager {
public:
  void begin(TrackManager *trackMgr, LEDStripManager *ledMgr, BacklightManager *blMgr, USBStorageManager *usbMgr, SDManager *sdMgr);
  void handleInput(UserInputEvent event, SystemSettings &settings, TelemetryProvider &provider);
  void render(U8G2 *u8g2, const TelemetrySnapshot &telemetry, const TelemetryProvider &provider, const SystemSettings &settings);

  UiViewMode getViewMode() const { return _current_view; }
  void setViewMode(UiViewMode mode) { _current_view = mode; }
  bool isMSCActive() const { return _menu.isMSCActive(); }

private:
  UiViewMode _current_view = VIEW_LIVE_RACE;
  MenuSystem _menu;

  PageLiveRace   _page_live_race;
  PageTelemetry  _page_telemetry;
  PageGpsPaddock _page_gps_paddock;
  PageDataRecall _page_data_recall;

  void renderFooter(U8G2 *u8g2, const TelemetrySnapshot &telemetry);
};
