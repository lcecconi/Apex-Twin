/**
 * @file page_live_race.h
 * Live Race Predictive Lap HUD page for Apex-Dash (LVGL v9)
 * Pixel-accurate reproduction of the 400x300 RLCD HUD layout
 */

#pragma once

#include <lvgl.h>
#include "telemetry_data.h"

namespace ApexUi {

class PageLiveRace {
public:
    void create(lv_obj_t *parent);
    void update(const TelemetrySnapshot &telemetry, const SystemSettings &settings);
    void setVisible(bool visible);

private:
    lv_obj_t *_container = nullptr;

    // 1. Top Tachometer
    lv_obj_t *_bar_rpm = nullptr;

    // 2. Left: Gear & Speed Pane
    lv_obj_t *_card_gear = nullptr;
    lv_obj_t *_box_gear = nullptr;
    lv_obj_t *_lbl_gear_val = nullptr;
    lv_obj_t *_lbl_speed_unit = nullptr;
    lv_obj_t *_lbl_speed_val = nullptr;

    // 3. Right: Lap Time & Sectors Pane
    lv_obj_t *_card_lap = nullptr;
    lv_obj_t *_lbl_lap_title = nullptr;
    lv_obj_t *_lbl_lap_time = nullptr;
    lv_obj_t *_badge_best = nullptr;
    lv_obj_t *_lbl_best_lap = nullptr;
    lv_obj_t *_lbl_last_lap = nullptr;

    // 4. Middle-Left: Predictive Delta Pane
    lv_obj_t *_card_delta = nullptr;
    lv_obj_t *_lbl_delta = nullptr;

    // 5. Bottom-Left: Engine Temps & Hours Pane
    lv_obj_t *_card_engine = nullptr;
    lv_obj_t *_img_water = nullptr;
    lv_obj_t *_lbl_water = nullptr;
    lv_obj_t *_img_egt = nullptr;
    lv_obj_t *_lbl_egt = nullptr;
    lv_obj_t *_img_eng_hours = nullptr;
    lv_obj_t *_lbl_eng_hours = nullptr;
    lv_obj_t *_img_session_time = nullptr;
    lv_obj_t *_lbl_session_time = nullptr;

    // 6. Bottom-Right: Unified Flashing Warning / Status Panel (185x110)
    lv_obj_t *_card_status = nullptr;
    lv_obj_t *_lbl_status_ok = nullptr;
    lv_obj_t *_img_alarm_icon = nullptr;
    lv_obj_t *_lbl_alarm_title = nullptr;

    // 7. Footer Status Line
    lv_obj_t *_line_footer = nullptr;
    lv_obj_t *_lbl_footer_track = nullptr;
    lv_obj_t *_lbl_footer_status = nullptr;

    // Flash & timing animation tracking
    uint8_t  _prev_sector = 0;
    uint16_t _prev_lap = 0;
    uint32_t _prev_best_lap = 0;
    float    _prev_delta = 999.0f;
    uint32_t _delta_flash_start_ms = 0;
};

} // namespace ApexUi
