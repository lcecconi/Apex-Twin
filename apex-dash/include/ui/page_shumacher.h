/**
 * @file page_shumacher.h
 * Schumacher 3-Dial Speedometer HUD (V-MIN, LIVE, V-MAX STR) for Apex-Dash (LVGL v9)
 */

#pragma once

#include <lvgl.h>
#include "telemetry_data.h"

namespace ApexUi {

class PageShumacher {
public:
    void create(lv_obj_t *parent);
    void update(const TelemetrySnapshot &telemetry, const SystemSettings &settings);
    void setVisible(bool visible);

private:
    lv_obj_t *_container = nullptr;

    // RPM Bar
    lv_obj_t *_bar_rpm = nullptr;
    lv_obj_t *_lbl_rpm = nullptr;

    // 3 Speedometers
    lv_obj_t *_card_vmin = nullptr;
    lv_obj_t *_lbl_vmin = nullptr;

    lv_obj_t *_card_vlive = nullptr;
    lv_obj_t *_lbl_vlive = nullptr;

    lv_obj_t *_card_vmax = nullptr;
    lv_obj_t *_lbl_vmax = nullptr;

    // Bottom-Left Sub-Panels
    lv_obj_t *_card_lap = nullptr;
    lv_obj_t *_lbl_lap_title = nullptr;
    lv_obj_t *_lbl_lap_time = nullptr;

    lv_obj_t *_card_delta = nullptr;
    lv_obj_t *_lbl_delta = nullptr;

    // Bottom-Right Summary
    lv_obj_t *_card_summary = nullptr;
    lv_obj_t *_lbl_summary_rpm = nullptr;
    lv_obj_t *_lbl_summary_temp = nullptr;
    lv_obj_t *_lbl_summary_best = nullptr;

    // Speed holding tracking logic
    float _held_vmin = 0.0f;
    float _held_vmax = 0.0f;
    float _current_corner_min = 999.0f;
    float _current_straight_max = 0.0f;
    bool  _in_corner = false;
};

} // namespace ApexUi
