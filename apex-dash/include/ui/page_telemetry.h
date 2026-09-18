/**
 * @file page_telemetry.h
 * Telemetry & Sensor Monitor page (G-G Diagram, Dual Temps, RPM) for Apex-Dash (LVGL v9)
 */

#pragma once

#include <lvgl.h>
#include "telemetry_data.h"

namespace ApexUi {

class PageTelemetry {
public:
    void create(lv_obj_t *parent);
    void update(const TelemetrySnapshot &telemetry, const SystemSettings &settings);
    void setVisible(bool visible);

private:
    lv_obj_t *_container = nullptr;

    // Header
    lv_obj_t *_lbl_header_title = nullptr;
    lv_obj_t *_lbl_header_lap = nullptr;

    // Card 1: Engine RPM & Gear
    lv_obj_t *_card_rpm = nullptr;
    lv_obj_t *_lbl_rpm_val = nullptr;
    lv_obj_t *_lbl_gear_val = nullptr;

    // Card 2: Dual Temperatures
    lv_obj_t *_card_temps = nullptr;
    lv_obj_t *_lbl_water_temp = nullptr;
    lv_obj_t *_lbl_egt_temp = nullptr;

    // Card 3: G-G Diagram (Friction Circle)
    lv_obj_t *_card_gg = nullptr;
    lv_obj_t *_lbl_gg_lat = nullptr;
    lv_obj_t *_lbl_gg_lon = nullptr;
    lv_obj_t *_lbl_gg_peak = nullptr;

    // Card 4: Timing & Sectors
    lv_obj_t *_card_timing = nullptr;
    lv_obj_t *_lbl_time_best = nullptr;
    lv_obj_t *_lbl_time_last = nullptr;
    lv_obj_t *_lbl_sector_splits = nullptr;
};

} // namespace ApexUi
