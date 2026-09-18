/**
 * @file page_live_race.h
 * Live Race Predictive Lap HUD page for Apex-Dash (LVGL v9)
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

    // RPM / Tacho
    lv_obj_t *_bar_rpm = nullptr;
    lv_obj_t *_lbl_rpm = nullptr;

    // Left: Gear or Lap
    lv_obj_t *_card_gear = nullptr;
    lv_obj_t *_lbl_gear = nullptr;
    lv_obj_t *_lbl_gear_title = nullptr;

    // Right: Lap Time & Sectors
    lv_obj_t *_card_lap = nullptr;
    lv_obj_t *_lbl_lap_sector = nullptr;
    lv_obj_t *_lbl_lap_time = nullptr;
    lv_obj_t *_badge_best_lap = nullptr;
    lv_obj_t *_lbl_best_lap = nullptr;
    lv_obj_t *_lbl_last_lap = nullptr;

    // Bottom-Left: Predictive Delta
    lv_obj_t *_card_delta = nullptr;
    lv_obj_t *_lbl_delta = nullptr;
    lv_obj_t *_lbl_delta_title = nullptr;

    // Bottom-Right: Status & Alarms
    lv_obj_t *_card_status = nullptr;
    lv_obj_t *_lbl_status_temp = nullptr;
    lv_obj_t *_lbl_status_bat = nullptr;
    lv_obj_t *_lbl_status_link = nullptr;

    // Flash animation tracking
    uint8_t  _prev_sector = 0;
    uint16_t _prev_lap = 0;
    uint32_t _prev_best_lap = 0;
    float    _prev_delta = 999.0f;
    uint32_t _flash_start_ms = 0;
};

} // namespace ApexUi
