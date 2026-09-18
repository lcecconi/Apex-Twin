/**
 * @file page_data_recall.h
 * Session Data Recall & 1-5 Sector Lap Analysis page (LVGL v9)
 */

#pragma once

#include <lvgl.h>
#include "telemetry_data.h"

namespace ApexUi {

class PageDataRecall {
public:
    void create(lv_obj_t *parent);
    void update(const TelemetrySnapshot &telemetry, const SystemSettings &settings, const LapRecord *laps = nullptr, uint16_t lap_count = 0);
    void setVisible(bool visible);

private:
    lv_obj_t *_container = nullptr;

    lv_obj_t *_lbl_total_laps = nullptr;
    lv_obj_t *_table_laps = nullptr;

    // Summary Box
    lv_obj_t *_card_summary = nullptr;
    lv_obj_t *_lbl_best_lap = nullptr;
    lv_obj_t *_lbl_theo_lap = nullptr;
    lv_obj_t *_lbl_peak_g = nullptr;
    lv_obj_t *_lbl_consistency = nullptr;
};

} // namespace ApexUi
