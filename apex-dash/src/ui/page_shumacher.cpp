/**
 * @file page_shumacher.cpp
 * Schumacher 3-Dial Speedometer HUD implementation (LVGL v9)
 */

#include "ui/page_shumacher.h"
#include "ui/ui_theme.h"
#include <cstdio>
#include <cmath>

namespace ApexUi {

void PageShumacher::create(lv_obj_t *parent) {
    _container = lv_obj_create(parent);
    lv_obj_remove_style_all(_container);
    lv_obj_add_style(_container, &UiTheme::style_screen, 0);
    lv_obj_set_size(_container, 400, 240);
    lv_obj_set_pos(_container, 0, 0);
    lv_obj_clear_flag(_container, LV_OBJ_FLAG_SCROLLABLE);

    // ==========================================
    // 1. TOP TACHO
    // ==========================================
    _bar_rpm = lv_bar_create(_container);
    lv_obj_set_pos(_bar_rpm, 10, 8);
    lv_obj_set_size(_bar_rpm, 275, 18);
    lv_obj_set_style_bg_color(_bar_rpm, UiTheme::bg(), 0);
    lv_obj_set_style_border_color(_bar_rpm, UiTheme::fg(), 0);
    lv_obj_set_style_border_width(_bar_rpm, 1, 0);
    lv_obj_set_style_radius(_bar_rpm, 2, 0);
    lv_obj_set_style_bg_color(_bar_rpm, UiTheme::fg(), LV_PART_INDICATOR);
    lv_obj_set_style_radius(_bar_rpm, 1, LV_PART_INDICATOR);
    lv_bar_set_range(_bar_rpm, 0, 16000);

    _lbl_rpm = lv_label_create(_container);
    lv_obj_add_style(_lbl_rpm, &UiTheme::style_text_bold, 0);
    lv_obj_set_pos(_lbl_rpm, 292, 8);
    lv_label_set_text(_lbl_rpm, "0 RPM");

    // ==========================================
    // 2. 3 SPEEDOMETER DIALS
    // ==========================================
    // Dial 1: V-MIN (APX)
    _card_vmin = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_vmin);
    lv_obj_add_style(_card_vmin, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_vmin, 10, 32);
    lv_obj_set_size(_card_vmin, 122, 120);
    lv_obj_clear_flag(_card_vmin, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title1 = lv_label_create(_card_vmin);
    lv_obj_add_style(title1, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(title1, 4, 4);
    lv_label_set_text(title1, "V-MIN (APX)");

    _lbl_vmin = lv_label_create(_card_vmin);
    lv_obj_add_style(_lbl_vmin, &UiTheme::style_text_huge, 0);
    lv_obj_align(_lbl_vmin, LV_ALIGN_CENTER, 0, 8);
    lv_label_set_text(_lbl_vmin, "--");

    // Dial 2: LIVE SPEED
    _card_vlive = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_vlive);
    lv_obj_add_style(_card_vlive, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_vlive, 138, 32);
    lv_obj_set_size(_card_vlive, 124, 120);
    lv_obj_clear_flag(_card_vlive, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title2 = lv_label_create(_card_vlive);
    lv_obj_add_style(title2, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(title2, 4, 4);
    lv_label_set_text(title2, "LIVE SPEED");

    _lbl_vlive = lv_label_create(_card_vlive);
    lv_obj_add_style(_lbl_vlive, &UiTheme::style_text_huge, 0);
    lv_obj_align(_lbl_vlive, LV_ALIGN_CENTER, 0, 8);
    lv_label_set_text(_lbl_vlive, "0");

    // Dial 3: V-MAX (STR)
    _card_vmax = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_vmax);
    lv_obj_add_style(_card_vmax, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_vmax, 268, 32);
    lv_obj_set_size(_card_vmax, 122, 120);
    lv_obj_clear_flag(_card_vmax, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title3 = lv_label_create(_card_vmax);
    lv_obj_add_style(title3, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(title3, 4, 4);
    lv_label_set_text(title3, "V-MAX (STR)");

    _lbl_vmax = lv_label_create(_card_vmax);
    lv_obj_add_style(_lbl_vmax, &UiTheme::style_text_huge, 0);
    lv_obj_align(_lbl_vmax, LV_ALIGN_CENTER, 0, 8);
    lv_label_set_text(_lbl_vmax, "--");

    // ==========================================
    // 3. BOTTOM-LEFT: LAP & DELTA
    // ==========================================
    _card_lap = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_lap);
    lv_obj_add_style(_card_lap, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_lap, 10, 158);
    lv_obj_set_size(_card_lap, 185, 36);
    lv_obj_clear_flag(_card_lap, LV_OBJ_FLAG_SCROLLABLE);

    _lbl_lap_title = lv_label_create(_card_lap);
    lv_obj_add_style(_lbl_lap_title, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_lap_title, 4, 2);
    lv_label_set_text(_lbl_lap_title, "LAP 01");

    _lbl_lap_time = lv_label_create(_card_lap);
    lv_obj_add_style(_lbl_lap_time, &UiTheme::style_text_bold, 0);
    lv_obj_set_pos(_lbl_lap_time, 90, 8);
    lv_label_set_text(_lbl_lap_time, "00:00.00");

    _card_delta = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_delta);
    lv_obj_add_style(_card_delta, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_delta, 10, 198);
    lv_obj_set_size(_card_delta, 185, 36);
    lv_obj_clear_flag(_card_delta, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title_delta = lv_label_create(_card_delta);
    lv_obj_add_style(title_delta, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(title_delta, 4, 2);
    lv_label_set_text(title_delta, "BEST DELTA");

    _lbl_delta = lv_label_create(_card_delta);
    lv_obj_add_style(_lbl_delta, &UiTheme::style_text_bold, 0);
    lv_obj_set_pos(_lbl_delta, 100, 8);
    lv_label_set_text(_lbl_delta, "+ 0.00");

    // ==========================================
    // 4. BOTTOM-RIGHT: SUMMARY
    // ==========================================
    _card_summary = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_summary);
    lv_obj_add_style(_card_summary, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_summary, 202, 158);
    lv_obj_set_size(_card_summary, 188, 76);
    lv_obj_clear_flag(_card_summary, LV_OBJ_FLAG_SCROLLABLE);

    _lbl_summary_best = lv_label_create(_card_summary);
    lv_obj_add_style(_lbl_summary_best, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_summary_best, 6, 6);
    lv_label_set_text(_lbl_summary_best, "BEST LAP: --.-- s");

    _lbl_summary_temp = lv_label_create(_card_summary);
    lv_obj_add_style(_lbl_summary_temp, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_summary_temp, 6, 26);
    lv_label_set_text(_lbl_summary_temp, "WATER: --.- C  |  EGT: --- C");

    _lbl_summary_rpm = lv_label_create(_card_summary);
    lv_obj_add_style(_lbl_summary_rpm, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_summary_rpm, 6, 46);
    lv_label_set_text(_lbl_summary_rpm, "PEAK RPM: 15,850");
}

void PageShumacher::update(const TelemetrySnapshot &t, const SystemSettings &s) {
    if (!_container) return;

    // Tacho
    lv_bar_set_range(_bar_rpm, 0, s.max_rpm);
    lv_bar_set_value(_bar_rpm, t.rpm, LV_ANIM_OFF);
    lv_label_set_text_fmt(_lbl_rpm, "%u RPM", t.rpm);

    // Speed Tracking logic
    float spd = t.speed_kmh;
    float lat = std::fabs(t.lateral_g);
    float lon = t.longitudinal_g;

    if (lat > 0.65f || lon < -0.30f) {
        _in_corner = true;
        if (spd < _current_corner_min) {
            _current_corner_min = spd;
        }
    } else if (_in_corner && lon > 0.15f) {
        _in_corner = false;
        if (_current_corner_min > 5.0f && _current_corner_min < 900.0f) {
            _held_vmin = _current_corner_min;
        }
        _current_corner_min = 999.0f;
    }

    if (lon > 0.20f && lat < 0.35f && spd > _current_straight_max) {
        _current_straight_max = spd;
    } else if (lon < -0.40f && _current_straight_max > 20.0f) {
        _held_vmax = _current_straight_max;
        _current_straight_max = 0.0f;
    }

    // 3 Speeds
    if (_held_vmin > 0.0f) {
        lv_label_set_text_fmt(_lbl_vmin, "%.0f", _held_vmin);
    } else {
        lv_label_set_text(_lbl_vmin, "--");
    }

    lv_label_set_text_fmt(_lbl_vlive, "%.0f", spd);

    if (_held_vmax > 0.0f) {
        lv_label_set_text_fmt(_lbl_vmax, "%.0f", _held_vmax);
    } else {
        lv_label_set_text(_lbl_vmax, "--");
    }

    // Lap & Delta
    if (t.total_sectors > 1) {
        lv_label_set_text_fmt(_lbl_lap_title, "LAP %02u [%u/%u]", t.lap_number, t.current_sector, t.total_sectors);
    } else {
        lv_label_set_text_fmt(_lbl_lap_title, "LAP %02u", t.lap_number);
    }

    uint32_t active_ms = t.current_lap_time_ms;
    unsigned long lap_min = (unsigned long)(active_ms / 60000);
    unsigned long lap_sec = (unsigned long)((active_ms % 60000) / 1000);
    unsigned long lap_cen = (unsigned long)((active_ms % 1000) / 10);
    lv_label_set_text_fmt(_lbl_lap_time, "%02lu:%02lu.%02lu", lap_min, lap_sec, lap_cen);

    float delta = t.predictive_delta_s;
    if (t.best_lap_time_ms > 0 || std::fabs(delta) > 0.001f) {
        if (delta >= 0.0f) {
            lv_label_set_text_fmt(_lbl_delta, "+ %.2f", delta);
        } else {
            lv_label_set_text_fmt(_lbl_delta, "- %.2f", -delta);
        }
    } else {
        lv_label_set_text(_lbl_delta, "+ 0.00");
    }

    // Summary
    if (t.best_lap_time_ms > 0) {
        unsigned long b_sec = (unsigned long)((t.best_lap_time_ms % 60000) / 1000);
        unsigned long b_cen = (unsigned long)((t.best_lap_time_ms % 1000) / 10);
        lv_label_set_text_fmt(_lbl_summary_best, "BEST LAP: %02lu.%02lu s", b_sec, b_cen);
    } else {
        lv_label_set_text(_lbl_summary_best, "BEST LAP: --.-- s");
    }

    lv_label_set_text_fmt(_lbl_summary_temp, "WATER: %.1f C  |  EGT: %.0f C", t.water_temp_c, t.exhaust_temp_c);
}

void PageShumacher::setVisible(bool visible) {
    if (!_container) return;
    if (visible) {
        lv_obj_clear_flag(_container, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(_container, LV_OBJ_FLAG_HIDDEN);
    }
}

} // namespace ApexUi
