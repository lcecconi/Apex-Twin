/**
 * @file page_shumacher.cpp
 * Schumacher 3-Dial Speedometer HUD implementation (LVGL v9)
 * Pixel-accurate layout reproduction matching the 400x300 RLCD display
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
    lv_obj_set_size(_container, 400, 300);
    lv_obj_set_pos(_container, 0, 0);
    lv_obj_clear_flag(_container, LV_OBJ_FLAG_SCROLLABLE);

    // ==========================================
    // 1. TOP TACHO (Full width 388x26)
    // ==========================================
    _bar_rpm = lv_bar_create(_container);
    lv_obj_set_pos(_bar_rpm, 6, 4);
    lv_obj_set_size(_bar_rpm, 388, 26);
    lv_obj_set_style_bg_color(_bar_rpm, UiTheme::bg(), 0);
    lv_obj_set_style_border_color(_bar_rpm, UiTheme::fg(), 0);
    lv_obj_set_style_border_width(_bar_rpm, 1, 0);
    lv_obj_set_style_radius(_bar_rpm, 3, 0);
    lv_obj_set_style_bg_color(_bar_rpm, UiTheme::fg(), LV_PART_INDICATOR);
    lv_obj_set_style_radius(_bar_rpm, 1, LV_PART_INDICATOR);
    lv_bar_set_range(_bar_rpm, 0, 16000);

    // ==========================================
    // 2. 3 SPEEDOMETER DIALS (y = 38, h = 118)
    // ==========================================
    // Dial 1: V-MIN (APEX) (10, 38, 120, 118)
    _card_vmin = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_vmin);
    lv_obj_add_style(_card_vmin, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_vmin, 10, 38);
    lv_obj_set_size(_card_vmin, 120, 118);
    lv_obj_clear_flag(_card_vmin, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *hdr_vmin = lv_obj_create(_card_vmin);
    lv_obj_remove_style_all(hdr_vmin);
    lv_obj_add_style(hdr_vmin, &UiTheme::style_card_title, 0);
    lv_obj_set_pos(hdr_vmin, 0, 0);
    lv_obj_set_size(hdr_vmin, 120, 20);
    lv_obj_t *lbl_hdr1 = lv_label_create(hdr_vmin);
    lv_obj_add_style(lbl_hdr1, &UiTheme::style_card_title_text, 0);
    lv_obj_center(lbl_hdr1);
    lv_label_set_text(lbl_hdr1, "V-MIN (APEX)");

    _lbl_vmin = lv_label_create(_card_vmin);
    lv_obj_add_style(_lbl_vmin, &UiTheme::style_text_large, 0);
    lv_obj_set_pos(_lbl_vmin, 0, 36);
    lv_obj_set_size(_lbl_vmin, 120, 70);
    lv_obj_set_style_text_align(_lbl_vmin, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(_lbl_vmin, "--");

    // Dial 2: LIVE SPEED (138, 38, 124, 118)
    _card_vlive = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_vlive);
    lv_obj_add_style(_card_vlive, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_vlive, 138, 38);
    lv_obj_set_size(_card_vlive, 124, 118);
    lv_obj_clear_flag(_card_vlive, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *hdr_vlive = lv_obj_create(_card_vlive);
    lv_obj_remove_style_all(hdr_vlive);
    lv_obj_add_style(hdr_vlive, &UiTheme::style_card_title, 0);
    lv_obj_set_pos(hdr_vlive, 0, 0);
    lv_obj_set_size(hdr_vlive, 124, 20);
    lv_obj_t *lbl_hdr2 = lv_label_create(hdr_vlive);
    lv_obj_add_style(lbl_hdr2, &UiTheme::style_card_title_text, 0);
    lv_obj_center(lbl_hdr2);
    lv_label_set_text(lbl_hdr2, "LIVE SPEED");

    _lbl_vlive = lv_label_create(_card_vlive);
    lv_obj_add_style(_lbl_vlive, &UiTheme::style_text_large, 0);
    lv_obj_set_pos(_lbl_vlive, 0, 36);
    lv_obj_set_size(_lbl_vlive, 124, 70);
    lv_obj_set_style_text_align(_lbl_vlive, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(_lbl_vlive, "000");

    // Dial 3: V-MAX (STR) (270, 38, 120, 118)
    _card_vmax = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_vmax);
    lv_obj_add_style(_card_vmax, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_vmax, 270, 38);
    lv_obj_set_size(_card_vmax, 120, 118);
    lv_obj_clear_flag(_card_vmax, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *hdr_vmax = lv_obj_create(_card_vmax);
    lv_obj_remove_style_all(hdr_vmax);
    lv_obj_add_style(hdr_vmax, &UiTheme::style_card_title, 0);
    lv_obj_set_pos(hdr_vmax, 0, 0);
    lv_obj_set_size(hdr_vmax, 120, 20);
    lv_obj_t *lbl_hdr3 = lv_label_create(hdr_vmax);
    lv_obj_add_style(lbl_hdr3, &UiTheme::style_card_title_text, 0);
    lv_obj_center(lbl_hdr3);
    lv_label_set_text(lbl_hdr3, "V-MAX (STR)");

    _lbl_vmax = lv_label_create(_card_vmax);
    lv_obj_add_style(_lbl_vmax, &UiTheme::style_text_large, 0);
    lv_obj_set_pos(_lbl_vmax, 0, 36);
    lv_obj_set_size(_lbl_vmax, 120, 70);
    lv_obj_set_style_text_align(_lbl_vmax, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(_lbl_vmax, "--");

    // ==========================================
    // 3. SUB-PANEL A: CURRENT LAP TIME (10, 162, 185, 52)
    // ==========================================
    _card_lap = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_lap);
    lv_obj_add_style(_card_lap, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_lap, 10, 162);
    lv_obj_set_size(_card_lap, 185, 52);
    lv_obj_clear_flag(_card_lap, LV_OBJ_FLAG_SCROLLABLE);

    _lbl_lap_title = lv_label_create(_card_lap);
    lv_obj_add_style(_lbl_lap_title, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_lap_title, 6, 4);
    lv_label_set_text(_lbl_lap_title, "LAP 01  [SEC 1/3]");

    _lbl_lap_time = lv_label_create(_card_lap);
    lv_obj_add_style(_lbl_lap_time, &UiTheme::style_text_bold, 0);
    lv_obj_set_pos(_lbl_lap_time, 0, 22);
    lv_obj_set_size(_lbl_lap_time, 185, 26);
    lv_obj_set_style_text_align(_lbl_lap_time, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(_lbl_lap_time, "00:00.00");

    // ==========================================
    // 4. SUB-PANEL B: PREDICTIVE DELTA (10, 220, 185, 52)
    // ==========================================
    _card_delta = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_delta);
    lv_obj_add_style(_card_delta, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_delta, 10, 220);
    lv_obj_set_size(_card_delta, 185, 52);
    lv_obj_clear_flag(_card_delta, LV_OBJ_FLAG_SCROLLABLE);

    _lbl_delta = lv_label_create(_card_delta);
    lv_obj_add_style(_lbl_delta, &UiTheme::style_text_large, 0);
    lv_obj_center(_lbl_delta);
    lv_label_set_text(_lbl_delta, "+ 0.00");

    // ==========================================
    // 5. BOTTOM-RIGHT: SUMMARY STATS (205, 162, 185, 110)
    // ==========================================
    _card_summary = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_summary);
    lv_obj_add_style(_card_summary, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_summary, 205, 162);
    lv_obj_set_size(_card_summary, 185, 110);
    lv_obj_clear_flag(_card_summary, LV_OBJ_FLAG_SCROLLABLE);

    _lbl_summary_best = lv_label_create(_card_summary);
    lv_obj_add_style(_lbl_summary_best, &UiTheme::style_text_bold, 0);
    lv_obj_set_pos(_lbl_summary_best, 8, 12);
    lv_obj_set_size(_lbl_summary_best, 169, 24);
    lv_obj_set_style_text_align(_lbl_summary_best, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(_lbl_summary_best, "BEST: 48.42 s");

    _lbl_summary_rpm = lv_label_create(_card_summary);
    lv_obj_add_style(_lbl_summary_rpm, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_summary_rpm, 8, 46);
    lv_obj_set_size(_lbl_summary_rpm, 169, 20);
    lv_obj_set_style_text_align(_lbl_summary_rpm, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(_lbl_summary_rpm, "MAX RPM: 15850");

    _lbl_summary_temp = lv_label_create(_card_summary);
    lv_obj_add_style(_lbl_summary_temp, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_summary_temp, 8, 72);
    lv_obj_set_size(_lbl_summary_temp, 169, 20);
    lv_obj_set_style_text_align(_lbl_summary_temp, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(_lbl_summary_temp, "H2O: 58 C  |  EGT: 580 C");

    // ==========================================
    // 6. FOOTER LINE (y = 276..300)
    // ==========================================
    lv_obj_t *line_foot = lv_obj_create(_container);
    lv_obj_remove_style_all(line_foot);
    lv_obj_set_style_bg_color(line_foot, UiTheme::fg(), 0);
    lv_obj_set_style_bg_opa(line_foot, LV_OPA_COVER, 0);
    lv_obj_set_pos(line_foot, 0, 276);
    lv_obj_set_size(line_foot, 400, 1);

    lv_obj_t *lbl_foot_track = lv_label_create(_container);
    lv_obj_add_style(lbl_foot_track, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(lbl_foot_track, 8, 281);
    lv_label_set_text(lbl_foot_track, "TRACK: South Garda (Lonato)");

    lv_obj_t *lbl_foot_stat = lv_label_create(_container);
    lv_obj_add_style(lbl_foot_stat, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(lbl_foot_stat, 220, 281);
    lv_obj_set_size(lbl_foot_stat, 172, 16);
    lv_obj_set_style_text_align(lbl_foot_stat, LV_TEXT_ALIGN_RIGHT, 0);
    lv_label_set_text(lbl_foot_stat, "BAT: 4.1V (98%) | LINK OK");
}

void PageShumacher::update(const TelemetrySnapshot &t, const SystemSettings &s) {
    if (!_container) return;

    char buf[64];

    // 1. Tachometer
    lv_bar_set_range(_bar_rpm, 0, s.max_rpm);
    lv_bar_set_value(_bar_rpm, t.rpm, LV_ANIM_OFF);

    // 2. Speedometer holding algorithm
    float spd = s.use_kmh ? t.speed_kmh : (t.speed_kmh * 0.621371f);
    bool is_braking = (t.longitudinal_g < -0.25f);
    bool is_cornering = (std::abs(t.lateral_g) > 0.60f);

    if (is_braking || is_cornering) {
        if (!_in_corner) {
            if (_current_straight_max > 20.0f) {
                _held_vmax = _current_straight_max;
            }
            _current_corner_min = spd;
            _in_corner = true;
        }
        if (spd < _current_corner_min) {
            _current_corner_min = spd;
        }
    } else if (t.longitudinal_g > 0.10f || spd > _current_corner_min + 3.0f) {
        if (_in_corner) {
            if (_current_corner_min > 5.0f && _current_corner_min < 900.0f) {
                _held_vmin = _current_corner_min;
            }
            _in_corner = false;
            _current_straight_max = spd;
        }
        if (spd > _current_straight_max) {
            _current_straight_max = spd;
        }
    }

    float disp_vmin = _in_corner ? _current_corner_min : (_held_vmin > 0.0f ? _held_vmin : 48.0f);
    float disp_vmax = (!_in_corner && _current_straight_max > _held_vmax) ? _current_straight_max : (_held_vmax > 0.0f ? _held_vmax : 124.0f);

    snprintf(buf, sizeof(buf), "%03d", (int)disp_vmin);
    lv_label_set_text(_lbl_vmin, buf);

    snprintf(buf, sizeof(buf), "%03d", (int)spd);
    lv_label_set_text(_lbl_vlive, buf);

    snprintf(buf, sizeof(buf), "%03d", (int)disp_vmax);
    lv_label_set_text(_lbl_vmax, buf);

    // 3. Current Lap Time
    if (t.total_sectors > 1) {
        snprintf(buf, sizeof(buf), "LAP %02u  [SEC %u/%u]", t.lap_number, t.current_sector, t.total_sectors);
    } else {
        snprintf(buf, sizeof(buf), "LAP %02u", t.lap_number);
    }
    lv_label_set_text(_lbl_lap_title, buf);

    unsigned long lap_min = t.current_lap_time_ms / 60000;
    unsigned long lap_sec = (t.current_lap_time_ms % 60000) / 1000;
    unsigned long lap_cen = (t.current_lap_time_ms % 1000) / 10;
    snprintf(buf, sizeof(buf), "%02lu:%02lu.%02lu", lap_min, lap_sec, lap_cen);
    lv_label_set_text(_lbl_lap_time, buf);

    // 4. Predictive Delta
    float delta = t.predictive_delta_s;
    if (t.best_lap_time_ms > 0 || std::abs(delta) > 0.001f) {
        if (delta >= 0.0f) {
            snprintf(buf, sizeof(buf), "+ %.2f", delta);
        } else {
            snprintf(buf, sizeof(buf), "- %.2f", -delta);
        }
    } else {
        snprintf(buf, sizeof(buf), "+ 0.00");
    }
    lv_label_set_text(_lbl_delta, buf);

    // 5. Summary Stats
    if (t.best_lap_time_ms > 0) {
        unsigned long b_sec = (t.best_lap_time_ms % 60000) / 1000;
        unsigned long b_cen = (t.best_lap_time_ms % 1000) / 10;
        snprintf(buf, sizeof(buf), "BEST LAP: %02lu.%02lu s", b_sec, b_cen);
    } else {
        snprintf(buf, sizeof(buf), "BEST LAP: --.-- s");
    }
    lv_label_set_text(_lbl_summary_best, buf);

    float w_temp = s.use_celsius ? t.water_temp_c : (t.water_temp_c * 1.8f + 32.0f);
    float e_temp = s.use_celsius ? t.exhaust_temp_c : (t.exhaust_temp_c * 1.8f + 32.0f);
    snprintf(buf, sizeof(buf), "H2O: %.0f %s  |  EGT: %.0f %s", w_temp, s.use_celsius ? "C" : "F", e_temp, s.use_celsius ? "C" : "F");
    lv_label_set_text(_lbl_summary_temp, buf);
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
