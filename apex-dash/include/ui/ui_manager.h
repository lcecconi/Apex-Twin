/**
 * @file ui_manager.h
 * Master UI Coordinator for Apex-Dash (LVGL v9)
 */

#pragma once

#include <lvgl.h>
#include <cstdint>
#include "telemetry_data.h"
#include "ui/ui_theme.h"
#include "ui/page_live_race.h"
#include "ui/page_shumacher.h"
#include "ui/page_telemetry.h"
#include "ui/page_gps_paddock.h"
#include "ui/page_data_recall.h"

namespace ApexUi {

enum UiViewMode : uint8_t {
    VIEW_LIVE_RACE = 0,    // Predictive Lap Time HUD
    VIEW_SHUMACHER,        // Schumacher 3-Speedometer Benetton HUD
    VIEW_TELEMETRY,        // Tachometer, Dual Temps, G-G Diagram
    VIEW_GPS_PADDOCK,      // Satellite radar, Track detect, Maintenance
    VIEW_DATA_RECALL,      // Best 3 laps, dynamic sector breakdown
    VIEW_COUNT
};

class UiManager {
public:
    void init(lv_obj_t *root_screen = nullptr, bool inverted = true);
    void update(const TelemetrySnapshot &telemetry, const SystemSettings &settings, const LapRecord *laps = nullptr, uint16_t lap_count = 0);

    void setViewMode(UiViewMode mode);
    void nextView();
    void prevView();
    UiViewMode getViewMode() const { return _current_view; }

private:
    UiViewMode _current_view = VIEW_LIVE_RACE;

    PageLiveRace   _page_live_race;
    PageShumacher  _page_shumacher;
    PageTelemetry  _page_telemetry;
    PageGpsPaddock _page_gps_paddock;
    PageDataRecall _page_data_recall;

    // Global Top Status Indicator (Battery & RF link)
    lv_obj_t *_lbl_status_bar = nullptr;
};

} // namespace ApexUi
