/**
 * @file page_live_race.cpp
 * Live Race Predictive Lap HUD page implementation (LVGL v9)
 * Pixel-accurate layout reproduction matching the 400x300 RLCD display
 */

#include "ui/page_live_race.h"
#include "ui/ui_theme.h"
#include <cstdio>
#include <cmath>

namespace ApexUi {

void PageLiveRace::create(lv_obj_t *parent) {
    _container = lv_obj_create(parent);
    lv_obj_remove_style_all(_container);
    lv_obj_add_style(_container, &UiTheme::style_screen, 0);
    lv_obj_set_size(_container, 400, 300);
    lv_obj_set_pos(_container, 0, 0);
    lv_obj_clear_flag(_container, LV_OBJ_FLAG_SCROLLABLE);

    // ==========================================
    // 1. TOP TACHOMETER (Full-width bar 388x26)
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
    // 2. LEFT PANE: GEAR & SPEED (160x118)
    // ==========================================
    _card_gear = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_gear);
    lv_obj_add_style(_card_gear, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_gear, 10, 38);
    lv_obj_set_size(_card_gear, 160, 118);
    lv_obj_clear_flag(_card_gear, LV_OBJ_FLAG_SCROLLABLE);

    // Sub-box for Gear
    _box_gear = lv_obj_create(_card_gear);
    lv_obj_remove_style_all(_box_gear);
    lv_obj_set_style_border_color(_box_gear, UiTheme::fg(), 0);
    lv_obj_set_style_border_width(_box_gear, 1, 0);
    lv_obj_set_style_radius(_box_gear, 4, 0);
    lv_obj_set_pos(_box_gear, 4, 4);
    lv_obj_set_size(_box_gear, 62, 98);
    lv_obj_clear_flag(_box_gear, LV_OBJ_FLAG_SCROLLABLE);

    _lbl_gear_val = lv_label_create(_box_gear);
    lv_obj_add_style(_lbl_gear_val, &UiTheme::style_text_huge, 0);
    lv_obj_center(_lbl_gear_val);
    lv_label_set_text(_lbl_gear_val, "N");

    // Speed display on right of gear
    _lbl_speed_unit = lv_label_create(_card_gear);
    lv_obj_add_style(_lbl_speed_unit, &UiTheme::style_text_bold, 0);
    lv_obj_set_pos(_lbl_speed_unit, 74, 12);
    lv_obj_set_size(_lbl_speed_unit, 74, 20);
    lv_obj_set_style_text_align(_lbl_speed_unit, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(_lbl_speed_unit, "KM/H");

    _lbl_speed_val = lv_label_create(_card_gear);
    lv_obj_add_style(_lbl_speed_val, &UiTheme::style_text_large, 0);
    lv_obj_set_pos(_lbl_speed_val, 74, 42);
    lv_obj_set_size(_lbl_speed_val, 74, 50);
    lv_obj_set_style_text_align(_lbl_speed_val, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(_lbl_speed_val, "000");

    // ==========================================
    // 3. RIGHT PANE: LAP TIME & SECTORS (214x118)
    // ==========================================
    _card_lap = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_lap);
    lv_obj_add_style(_card_lap, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_lap, 176, 38);
    lv_obj_set_size(_card_lap, 214, 118);
    lv_obj_clear_flag(_card_lap, LV_OBJ_FLAG_SCROLLABLE);

    _lbl_lap_title = lv_label_create(_card_lap);
    lv_obj_add_style(_lbl_lap_title, &UiTheme::style_text_bold, 0);
    lv_obj_set_pos(_lbl_lap_title, 8, 6);
    lv_label_set_text(_lbl_lap_title, "LAP 01  [SEC 1/3]");

    _lbl_lap_time = lv_label_create(_card_lap);
    lv_obj_add_style(_lbl_lap_time, &UiTheme::style_text_large, 0);
    lv_obj_set_pos(_lbl_lap_time, 8, 30);
    lv_label_set_text(_lbl_lap_time, "00:00.00");

    // Inverted Best Lap Badge
    _badge_best = lv_obj_create(_card_lap);
    lv_obj_remove_style_all(_badge_best);
    lv_obj_add_style(_badge_best, &UiTheme::style_badge_inverted, 0);
    lv_obj_set_pos(_badge_best, 6, 80);
    lv_obj_set_size(_badge_best, 96, 24);
    lv_obj_clear_flag(_badge_best, LV_OBJ_FLAG_SCROLLABLE);

    _lbl_best_lap = lv_label_create(_badge_best);
    lv_obj_add_style(_lbl_best_lap, &UiTheme::style_text_small, 0);
    lv_obj_set_style_text_color(_lbl_best_lap, UiTheme::bg(), 0);
    lv_obj_center(_lbl_best_lap);
    lv_label_set_text(_lbl_best_lap, "BEST --.--");

    _lbl_last_lap = lv_label_create(_card_lap);
    lv_obj_add_style(_lbl_last_lap, &UiTheme::style_text_bold, 0);
    lv_obj_set_pos(_lbl_last_lap, 108, 84);
    lv_obj_set_size(_lbl_last_lap, 96, 20);
    lv_obj_set_style_text_align(_lbl_last_lap, LV_TEXT_ALIGN_RIGHT, 0);
    lv_label_set_text(_lbl_last_lap, "LAST: --.--");

    // ==========================================
    // 4. BOTTOM-LEFT 1: PREDICTIVE DELTA (185x52)
    // ==========================================
    _card_delta = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_delta);
    lv_obj_add_style(_card_delta, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_delta, 10, 162);
    lv_obj_set_size(_card_delta, 185, 52);
    lv_obj_clear_flag(_card_delta, LV_OBJ_FLAG_SCROLLABLE);

    _lbl_delta = lv_label_create(_card_delta);
    lv_obj_add_style(_lbl_delta, &UiTheme::style_text_large, 0);
    lv_obj_center(_lbl_delta);
    lv_label_set_text(_lbl_delta, "+ 0.00");

    // ==========================================
    // 5. BOTTOM-LEFT 2: ENGINE & RUNTIMES (185x50)
    // ==========================================
    _card_engine = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_engine);
    lv_obj_add_style(_card_engine, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_engine, 10, 220);
    lv_obj_set_size(_card_engine, 185, 50);
    lv_obj_clear_flag(_card_engine, LV_OBJ_FLAG_SCROLLABLE);

    _lbl_water = lv_label_create(_card_engine);
    lv_obj_add_style(_lbl_water, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_water, 4, 3);
    lv_label_set_text(_lbl_water, "H2O: -- C");

    _lbl_egt = lv_label_create(_card_engine);
    lv_obj_add_style(_lbl_egt, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_egt, 4, 23);
    lv_label_set_text(_lbl_egt, "EGT: --- C");

    // Separator line
    lv_obj_t *line_sep = lv_obj_create(_card_engine);
    lv_obj_remove_style_all(line_sep);
    lv_obj_set_style_bg_color(line_sep, UiTheme::fg(), 0);
    lv_obj_set_style_bg_opa(line_sep, LV_OPA_COVER, 0);
    lv_obj_set_pos(line_sep, 88, 3);
    lv_obj_set_size(line_sep, 1, 38);

    _lbl_eng_hours = lv_label_create(_card_engine);
    lv_obj_add_style(_lbl_eng_hours, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_eng_hours, 94, 3);
    lv_label_set_text(_lbl_eng_hours, "ENG: --h--m");

    _lbl_session_time = lv_label_create(_card_engine);
    lv_obj_add_style(_lbl_session_time, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_session_time, 94, 23);
    lv_label_set_text(_lbl_session_time, "SES: --h--m");

    // ==========================================
    // 6. BOTTOM-RIGHT: STATUS & ALARMS (185x108)
    // ==========================================
    _card_status = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_status);
    lv_obj_add_style(_card_status, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_status, 205, 162);
    lv_obj_set_size(_card_status, 185, 108);
    lv_obj_clear_flag(_card_status, LV_OBJ_FLAG_SCROLLABLE);

    _lbl_status_main = lv_label_create(_card_status);
    lv_obj_add_style(_lbl_status_main, &UiTheme::style_text_bold, 0);
    lv_obj_set_pos(_lbl_status_main, 8, 12);
    lv_obj_set_size(_lbl_status_main, 165, 24);
    lv_obj_set_style_text_align(_lbl_status_main, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(_lbl_status_main, "SYSTEM OK");

    _lbl_status_sub1 = lv_label_create(_card_status);
    lv_obj_add_style(_lbl_status_sub1, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_status_sub1, 8, 46);
    lv_obj_set_size(_lbl_status_sub1, 165, 18);
    lv_obj_set_style_text_align(_lbl_status_sub1, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(_lbl_status_sub1, "BAT: 4.12V (98%)");

    _lbl_status_sub2 = lv_label_create(_card_status);
    lv_obj_add_style(_lbl_status_sub2, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_status_sub2, 8, 68);
    lv_obj_set_size(_lbl_status_sub2, 165, 18);
    lv_obj_set_style_text_align(_lbl_status_sub2, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(_lbl_status_sub2, "LINK OK (-64dB)");

    // ==========================================
    // 7. FOOTER LINE & STATUS (y = 276..300)
    // ==========================================
    _line_footer = lv_obj_create(_container);
    lv_obj_remove_style_all(_line_footer);
    lv_obj_set_style_bg_color(_line_footer, UiTheme::fg(), 0);
    lv_obj_set_style_bg_opa(_line_footer, LV_OPA_COVER, 0);
    lv_obj_set_pos(_line_footer, 0, 276);
    lv_obj_set_size(_line_footer, 400, 1);

    _lbl_footer_track = lv_label_create(_container);
    lv_obj_add_style(_lbl_footer_track, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_footer_track, 8, 281);
    lv_label_set_text(_lbl_footer_track, "TRACK: South Garda (Lonato)");

    _lbl_footer_status = lv_label_create(_container);
    lv_obj_add_style(_lbl_footer_status, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_footer_status, 220, 281);
    lv_obj_set_size(_lbl_footer_status, 172, 16);
    lv_obj_set_style_text_align(_lbl_footer_status, LV_TEXT_ALIGN_RIGHT, 0);
    lv_label_set_text(_lbl_footer_status, "BAT: 4.1V (98%) | LINK OK");
}

void PageLiveRace::update(const TelemetrySnapshot &t, const SystemSettings &s) {
    if (!_container) return;

    char buf[64];

    // 1. Tachometer
    lv_bar_set_range(_bar_rpm, 0, s.max_rpm);
    lv_bar_set_value(_bar_rpm, t.rpm, LV_ANIM_OFF);

    // 2. Gear & Speed
    if (s.drive_type == DRIVE_SHIFTER_6SPEED) {
        lv_obj_clear_flag(_box_gear, LV_OBJ_FLAG_HIDDEN);
        if (t.gear == 0) {
            lv_label_set_text(_lbl_gear_val, "N");
        } else {
            snprintf(buf, sizeof(buf), "%u", t.gear);
            lv_label_set_text(_lbl_gear_val, buf);
        }
        float disp_speed = s.use_kmh ? t.speed_kmh : (t.speed_kmh * 0.621371f);
        snprintf(buf, sizeof(buf), "%03d", (int)disp_speed);
        lv_label_set_text(_lbl_speed_val, buf);
        lv_label_set_text(_lbl_speed_unit, s.use_kmh ? "KM/H" : "MPH");
    } else {
        // Direct Drive / Single Speed
        lv_obj_add_flag(_box_gear, LV_OBJ_FLAG_HIDDEN);
        float disp_speed = s.use_kmh ? t.speed_kmh : (t.speed_kmh * 0.621371f);
        snprintf(buf, sizeof(buf), "%03d", (int)disp_speed);
        lv_label_set_text(_lbl_speed_val, buf);
        lv_label_set_text(_lbl_speed_unit, s.use_kmh ? "KM/H" : "MPH");
    }

    // 3. Lap Time & Sectors
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

    if (t.best_lap_time_ms > 0) {
        unsigned long b_sec = (t.best_lap_time_ms % 60000) / 1000;
        unsigned long b_cen = (t.best_lap_time_ms % 1000) / 10;
        snprintf(buf, sizeof(buf), " BEST %02lu.%02lu ", b_sec, b_cen);
    } else {
        snprintf(buf, sizeof(buf), " BEST --.-- ");
    }
    lv_label_set_text(_lbl_best_lap, buf);

    if (t.last_lap_time_ms > 0) {
        unsigned long l_sec = (t.last_lap_time_ms % 60000) / 1000;
        unsigned long l_cen = (t.last_lap_time_ms % 1000) / 10;
        snprintf(buf, sizeof(buf), "LAST: %02lu.%02lu", l_sec, l_cen);
    } else {
        snprintf(buf, sizeof(buf), "LAST: --.--");
    }
    lv_label_set_text(_lbl_last_lap, buf);

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

    // 5. Engine Temps & Runtimes
    float w_temp = s.use_celsius ? t.water_temp_c : (t.water_temp_c * 1.8f + 32.0f);
    float e_temp = s.use_celsius ? t.exhaust_temp_c : (t.exhaust_temp_c * 1.8f + 32.0f);
    snprintf(buf, sizeof(buf), "H2O: %.0f %s", w_temp, s.use_celsius ? "C" : "F");
    lv_label_set_text(_lbl_water, buf);

    snprintf(buf, sizeof(buf), "EGT: %.0f %s", e_temp, s.use_celsius ? "C" : "F");
    lv_label_set_text(_lbl_egt, buf);

    unsigned long eng_hrs = t.engine_total_hours_sec / 3600;
    unsigned long eng_min = (t.engine_total_hours_sec % 3600) / 60;
    snprintf(buf, sizeof(buf), "ENG: %luh%02lum", eng_hrs, eng_min);
    lv_label_set_text(_lbl_eng_hours, buf);

    unsigned long ses_hrs = t.session_time_sec / 3600;
    unsigned long ses_min = (t.session_time_sec % 3600) / 60;
    snprintf(buf, sizeof(buf), "SES: %luh%02lum", ses_hrs, ses_min);
    lv_label_set_text(_lbl_session_time, buf);

    // 6. Alarms / Status
    bool water_alarm = (t.water_temp_c >= s.water_temp_alarm_c && s.water_temp_alarm_c > 0);
    bool egt_alarm   = (t.exhaust_temp_c >= s.exhaust_temp_alarm_c && s.exhaust_temp_alarm_c > 0);
    bool rev_alarm   = (t.rpm >= s.over_rev_rpm && s.over_rev_rpm > 0);
    bool bat_alarm   = (t.battery_voltage < s.low_bat_alarm_v && t.battery_voltage > 1.0f);
    bool link_alarm  = !t.track_module_connected;

    if (water_alarm) {
        lv_label_set_text(_lbl_status_main, "! H2O HIGH !");
    } else if (egt_alarm) {
        lv_label_set_text(_lbl_status_main, "! EGT HIGH !");
    } else if (rev_alarm) {
        lv_label_set_text(_lbl_status_main, "! OVER-REV !");
    } else if (bat_alarm) {
        lv_label_set_text(_lbl_status_main, "! LOW BAT !");
    } else if (link_alarm) {
        lv_label_set_text(_lbl_status_main, "! NO LINK !");
    } else {
        lv_label_set_text(_lbl_status_main, "SYSTEM OK");
    }

    snprintf(buf, sizeof(buf), "BAT: %.2fV (%u%%)", t.battery_voltage, t.battery_percent);
    lv_label_set_text(_lbl_status_sub1, buf);

    snprintf(buf, sizeof(buf), "%s (%ddB)", t.track_module_connected ? "LINK OK" : "NO LINK", t.link_rssi);
    lv_label_set_text(_lbl_status_sub2, buf);

    // 7. Footer
    if (t.track_error_code != 0) {
        snprintf(buf, sizeof(buf), "ERR: CODE #%u", t.track_error_code);
    } else {
        snprintf(buf, sizeof(buf), "TRACK: %s", t.current_track_name);
    }
    lv_label_set_text(_lbl_footer_track, buf);

    snprintf(buf, sizeof(buf), "BAT: %.1fV (%u%%) | %s", t.battery_voltage, t.battery_percent, t.track_module_connected ? "LINK OK" : "NO LINK");
    lv_label_set_text(_lbl_footer_status, buf);
}

void PageLiveRace::setVisible(bool visible) {
    if (!_container) return;
    if (visible) {
        lv_obj_clear_flag(_container, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(_container, LV_OBJ_FLAG_HIDDEN);
    }
}

} // namespace ApexUi
