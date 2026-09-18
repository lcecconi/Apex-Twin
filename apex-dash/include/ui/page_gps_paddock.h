/**
 * @file page_gps_paddock.h
 * Paddock & Pre-Race Status page (GNSS Radar, Track Detection) for Apex-Dash (LVGL v9)
 */

#pragma once

#include <lvgl.h>
#include "telemetry_data.h"

namespace ApexUi {

class PageGpsPaddock {
public:
    void create(lv_obj_t *parent);
    void update(const TelemetrySnapshot &telemetry, const SystemSettings &settings);
    void setVisible(bool visible);

private:
    lv_obj_t *_container = nullptr;

    // Card 1: GNSS Radar
    lv_obj_t *_card_gnss = nullptr;
    lv_obj_t *_lbl_sats = nullptr;
    lv_obj_t *_lbl_fix = nullptr;
    lv_obj_t *_bar_sats[8] = {nullptr};

    // Card 2: Track Detection
    lv_obj_t *_card_track = nullptr;
    lv_obj_t *_lbl_track_name = nullptr;
    lv_obj_t *_lbl_track_dist = nullptr;
    lv_obj_t *_lbl_coords = nullptr;

    // Card 3: Maintenance & Environment
    lv_obj_t *_card_maint = nullptr;
    lv_obj_t *_lbl_ambient = nullptr;
    lv_obj_t *_lbl_engine_hours = nullptr;
    lv_obj_t *_lbl_piston_hours = nullptr;
};

} // namespace ApexUi
