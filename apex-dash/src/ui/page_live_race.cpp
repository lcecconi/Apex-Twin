/**
 * @file page_live_race.cpp
 * Live Race Predictive Lap HUD page implementation (LVGL v9)
 * 100% pixel-accurate layout reproduction matching the 400x300 RLCD display
 */

#include "ui/page_live_race.h"
#include "ui/ui_theme.h"
#include "ui/ui_icons.h"
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

    // Sub-box for Gear (60x96)
    _box_gear = lv_obj_create(_card_gear);
    lv_obj_remove_style_all(_box_gear);
    lv_obj_set_style_border_color(_box_gear, UiTheme::fg(), 0);
    lv_obj_set_style_border_width(_box_gear, 1, 0);
    lv_obj_set_style_radius(_box_gear, 4, 0);
    lv_obj_set_pos(_box_gear, 6, 8);
    lv_obj_set_size(_box_gear, 60, 96);
    lv_obj_clear_flag(_box_gear, LV_OBJ_FLAG_SCROLLABLE);

    _lbl_gear_val = lv_label_create(_box_gear);
    lv_obj_add_style(_lbl_gear_val, &UiTheme::style_text_huge, 0);
    lv_obj_center(_lbl_gear_val);
    lv_label_set_text(_lbl_gear_val, "N");

    // Speed display on right of gear
    _lbl_speed_unit = lv_label_create(_card_gear);
    lv_obj_add_style(_lbl_speed_unit, &UiTheme::style_text_bold, 0);
    lv_obj_set_pos(_lbl_speed_unit, 74, 14);
    lv_obj_set_size(_lbl_speed_unit, 74, 20);
    lv_obj_set_style_text_align(_lbl_speed_unit, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(_lbl_speed_unit, "KM/H");

    _lbl_speed_val = lv_label_create(_card_gear);
    lv_obj_add_style(_lbl_speed_val, &UiTheme::style_text_large, 0);
    lv_obj_set_pos(_lbl_speed_val, 70, 42);
    lv_obj_set_size(_lbl_speed_val, 82, 50);
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
    lv_obj_set_pos(_lbl_lap_title, 8, 8);
    lv_label_set_text(_lbl_lap_title, "LAP 01  [SEC 1/3]");

    _lbl_lap_time = lv_label_create(_card_lap);
    lv_obj_add_style(_lbl_lap_time, &UiTheme::style_text_large, 0);
    lv_obj_set_pos(_lbl_lap_time, 8, 36);
    lv_label_set_text(_lbl_lap_time, "00:00.00");

    // Inverted Best Lap Badge
    _badge_best = lv_obj_create(_card_lap);
    lv_obj_remove_style_all(_badge_best);
    lv_obj_add_style(_badge_best, &UiTheme::style_badge_inverted, 0);
    lv_obj_set_pos(_badge_best, 6, 82);
    lv_obj_set_size(_badge_best, 94, 22);
    lv_obj_clear_flag(_badge_best, LV_OBJ_FLAG_SCROLLABLE);

    _lbl_best_lap = lv_label_create(_badge_best);
    lv_obj_add_style(_lbl_best_lap, &UiTheme::style_text_small, 0);
    lv_obj_set_style_text_color(_lbl_best_lap, UiTheme::bg(), 0);
    lv_obj_center(_lbl_best_lap);
    lv_label_set_text(_lbl_best_lap, "BEST --.--");

    _lbl_last_lap = lv_label_create(_card_lap);
    lv_obj_add_style(_lbl_last_lap, &UiTheme::style_text_bold, 0);
    lv_obj_set_pos(_lbl_last_lap, 104, 84);
    lv_obj_set_size(_lbl_last_lap, 100, 20);
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
    lv_obj_set_pos(_card_engine, 10, 222);
    lv_obj_set_size(_card_engine, 185, 50);
    lv_obj_clear_flag(_card_engine, LV_OBJ_FLAG_SCROLLABLE);

    // Water Temp Icon & Label
    _img_water = lv_image_create(_card_engine);
    lv_image_set_src(_img_water, &img_icon_water_16x16);
    lv_obj_set_pos(_img_water, 4, 6);
    lv_obj_set_style_image_recolor(_img_water, UiTheme::fg(), 0);
    lv_obj_set_style_image_recolor_opa(_img_water, LV_OPA_COVER, 0);

    _lbl_water = lv_label_create(_card_engine);
    lv_obj_add_style(_lbl_water, &UiTheme::style_text_bold, 0);
    lv_obj_set_pos(_lbl_water, 24, 6);
    lv_label_set_text(_lbl_water, "0");

    // EGT Flame Icon & Label
    _img_egt = lv_image_create(_card_engine);
    lv_image_set_src(_img_egt, &img_icon_egt_16x16);
    lv_obj_set_pos(_img_egt, 4, 27);
    lv_obj_set_style_image_recolor(_img_egt, UiTheme::fg(), 0);
    lv_obj_set_style_image_recolor_opa(_img_egt, LV_OPA_COVER, 0);

    _lbl_egt = lv_label_create(_card_engine);
    lv_obj_add_style(_lbl_egt, &UiTheme::style_text_bold, 0);
    lv_obj_set_pos(_lbl_egt, 24, 27);
    lv_label_set_text(_lbl_egt, "0");

    // Separator line at x=88
    lv_obj_t *line_sep = lv_obj_create(_card_engine);
    lv_obj_remove_style_all(line_sep);
    lv_obj_set_style_bg_color(line_sep, UiTheme::fg(), 0);
    lv_obj_set_style_bg_opa(line_sep, LV_OPA_COVER, 0);
    lv_obj_set_pos(line_sep, 86, 4);
    lv_obj_set_size(line_sep, 1, 40);

    // Engine Hours Icon & Label
    _img_eng_hours = lv_image_create(_card_engine);
    lv_image_set_src(_img_eng_hours, &img_icon_engine_16x16);
    lv_obj_set_pos(_img_eng_hours, 94, 6);
    lv_obj_set_style_image_recolor(_img_eng_hours, UiTheme::fg(), 0);
    lv_obj_set_style_image_recolor_opa(_img_eng_hours, LV_OPA_COVER, 0);

    _lbl_eng_hours = lv_label_create(_card_engine);
    lv_obj_add_style(_lbl_eng_hours, &UiTheme::style_text_bold, 0);
    lv_obj_set_pos(_lbl_eng_hours, 114, 6);
    lv_label_set_text(_lbl_eng_hours, "00h00");

    // Session Time Icon & Label
    _img_session_time = lv_image_create(_card_engine);
    lv_image_set_src(_img_session_time, &img_icon_stopwatch_16x16);
    lv_obj_set_pos(_img_session_time, 94, 27);
    lv_obj_set_style_image_recolor(_img_session_time, UiTheme::fg(), 0);
    lv_obj_set_style_image_recolor_opa(_img_session_time, LV_OPA_COVER, 0);

    _lbl_session_time = lv_label_create(_card_engine);
    lv_obj_add_style(_lbl_session_time, &UiTheme::style_text_bold, 0);
    lv_obj_set_pos(_lbl_session_time, 114, 27);
    lv_label_set_text(_lbl_session_time, "00h00");

    // ==========================================
    // 6. BOTTOM-RIGHT: UNIFIED WARNING / STATUS PANEL (185x110)
    // ==========================================
    _card_status = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_status);
    lv_obj_add_style(_card_status, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_status, 205, 162);
    lv_obj_set_size(_card_status, 185, 110);
    lv_obj_clear_flag(_card_status, LV_OBJ_FLAG_SCROLLABLE);

    // Normal State Label
    _lbl_status_ok = lv_label_create(_card_status);
    lv_obj_add_style(_lbl_status_ok, &UiTheme::style_text_large, 0);
    lv_obj_center(_lbl_status_ok);
    lv_label_set_text(_lbl_status_ok, "SYSTEM OK");

    // Alarm Icon (Warning triangle / sensor icon)
    _img_alarm_icon = lv_image_create(_card_status);
    lv_image_set_src(_img_alarm_icon, &img_icon_warn_24x24);
    lv_obj_set_pos(_img_alarm_icon, 80, 16);
    lv_obj_add_flag(_img_alarm_icon, LV_OBJ_FLAG_HIDDEN);

    // Alarm Label ("WARN", "H2O HIGH", etc.)
    _lbl_alarm_title = lv_label_create(_card_status);
    lv_obj_add_style(_lbl_alarm_title, &UiTheme::style_text_large, 0);
    lv_obj_set_pos(_lbl_alarm_title, 10, 56);
    lv_obj_set_size(_lbl_alarm_title, 165, 40);
    lv_obj_set_style_text_align(_lbl_alarm_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_add_flag(_lbl_alarm_title, LV_OBJ_FLAG_HIDDEN);

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
    lv_obj_add_style(_lbl_footer_track, &UiTheme::style_text_bold, 0);
    lv_obj_set_pos(_lbl_footer_track, 8, 281);
    lv_label_set_text(_lbl_footer_track, "TRACK: South Garda (Lonato)");

    _lbl_footer_status = lv_label_create(_container);
    lv_obj_add_style(_lbl_footer_status, &UiTheme::style_text_bold, 0);
    lv_obj_set_pos(_lbl_footer_status, 200, 281);
    lv_obj_set_size(_lbl_footer_status, 192, 16);
    lv_obj_set_style_text_align(_lbl_footer_status, LV_TEXT_ALIGN_RIGHT, 0);
    lv_label_set_text(_lbl_footer_status, "BAT: 4.1V (98%) | LINK OK");
}

void PageLiveRace::update(const TelemetrySnapshot &t, const SystemSettings &s) {
    if (!_container) return;

    char buf[64];
    uint32_t now_ms = lv_tick_get();

    // 1. Tachometer
    lv_bar_set_range(_bar_rpm, 0, s.max_rpm > 0 ? s.max_rpm : 16000);
    lv_bar_set_value(_bar_rpm, t.rpm, LV_ANIM_OFF);

    // 2. Gear & Speed
    bool is_shift = (t.rpm >= s.shift_rpm && s.shift_rpm > 0);
    bool shift_blink = is_shift && (((now_ms / 150) % 2) == 0);

    if (s.drive_type == DRIVE_SHIFTER_6SPEED) {
        lv_obj_clear_flag(_box_gear, LV_OBJ_FLAG_HIDDEN);
        if (t.gear == 0) {
            lv_label_set_text(_lbl_gear_val, "N");
        } else {
            snprintf(buf, sizeof(buf), "%u", t.gear);
            lv_label_set_text(_lbl_gear_val, buf);
        }

        if (shift_blink) {
            lv_obj_set_style_bg_color(_box_gear, UiTheme::fg(), 0);
            lv_obj_set_style_bg_opa(_box_gear, LV_OPA_COVER, 0);
            lv_obj_set_style_text_color(_lbl_gear_val, UiTheme::bg(), 0);
        } else {
            lv_obj_set_style_bg_color(_box_gear, UiTheme::bg(), 0);
            lv_obj_set_style_bg_opa(_box_gear, LV_OPA_TRANSP, 0);
            lv_obj_set_style_text_color(_lbl_gear_val, UiTheme::fg(), 0);
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

    // 4. Predictive Delta & Inverted Flash Animation
    float delta = t.predictive_delta_s;
    if ((t.current_sector != _prev_sector && _prev_sector != 0) ||
        (t.lap_number != _prev_lap && _prev_lap != 0) ||
        (t.best_lap_time_ms != _prev_best_lap && _prev_best_lap != 0) ||
        (std::abs(delta - _prev_delta) > 0.001f && _prev_delta < 900.0f)) {
        _delta_flash_start_ms = now_ms;
    }
    _prev_sector = t.current_sector;
    _prev_lap = t.lap_number;
    _prev_best_lap = t.best_lap_time_ms;
    _prev_delta = delta;

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

    uint32_t flash_elapsed = now_ms - _delta_flash_start_ms;
    bool is_flashing = (flash_elapsed < 1500 && _delta_flash_start_ms > 0);
    bool is_inverted = is_flashing && (((flash_elapsed / 250) % 2) == 0);

    if (is_inverted) {
        lv_obj_set_style_bg_color(_card_delta, UiTheme::fg(), 0);
        lv_obj_set_style_bg_opa(_card_delta, LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(_lbl_delta, UiTheme::bg(), 0);
    } else {
        lv_obj_set_style_bg_color(_card_delta, UiTheme::bg(), 0);
        lv_obj_set_style_bg_opa(_card_delta, LV_OPA_TRANSP, 0);
        lv_obj_set_style_text_color(_lbl_delta, UiTheme::fg(), 0);
    }

    // 5. Engine Temps & Runtimes
    float w_temp = s.use_celsius ? t.water_temp_c : (t.water_temp_c * 1.8f + 32.0f);
    float e_temp = s.use_celsius ? t.exhaust_temp_c : (t.exhaust_temp_c * 1.8f + 32.0f);
    snprintf(buf, sizeof(buf), "%d", (int)std::round(w_temp));
    lv_label_set_text(_lbl_water, buf);

    snprintf(buf, sizeof(buf), "%d", (int)std::round(e_temp));
    lv_label_set_text(_lbl_egt, buf);

    unsigned long eng_hrs = t.engine_total_hours_sec / 3600;
    unsigned long eng_min = (t.engine_total_hours_sec % 3600) / 60;
    snprintf(buf, sizeof(buf), "%02luh%02lu", eng_hrs, eng_min);
    lv_label_set_text(_lbl_eng_hours, buf);

    unsigned long ses_hrs = t.session_time_sec / 3600;
    unsigned long ses_min = (t.session_time_sec % 3600) / 60;
    snprintf(buf, sizeof(buf), "%02luh%02lu", ses_hrs, ses_min);
    lv_label_set_text(_lbl_session_time, buf);

    // 6. Unified Flashing Warning / Status Panel (185x110)
    bool alm_active[5] = {
        (t.water_temp_c >= s.water_temp_alarm_c && s.water_temp_alarm_c > 0),
        (t.exhaust_temp_c >= s.exhaust_temp_alarm_c && s.exhaust_temp_alarm_c > 0),
        (t.rpm >= s.over_rev_rpm && s.over_rev_rpm > 0),
        (t.battery_voltage < s.low_bat_alarm_v && t.battery_voltage > 1.0f),
        (!t.track_module_connected)
    };

    bool alm_warn[5] = {
        alm_active[0] && s.warn_trigger_water,
        alm_active[1] && s.warn_trigger_egt,
        alm_active[2] && s.warn_trigger_rev,
        alm_active[3] && s.warn_trigger_battery,
        alm_active[4] && s.warn_trigger_link
    };

    int8_t top_alarm_id = -1;
    for (int i = 0; i < 5; i++) {
        uint8_t aid = s.alarm_priority[i];
        if (aid < 5 && alm_warn[aid]) {
            top_alarm_id = aid;
            break;
        }
    }

    if (top_alarm_id >= 0) {
        bool flash_phase = ((now_ms / 350) % 2) == 0;

        // Inverted solid background
        lv_obj_set_style_bg_color(_card_status, UiTheme::fg(), 0);
        lv_obj_set_style_bg_opa(_card_status, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(_card_status, 0, 0);

        lv_obj_add_flag(_lbl_status_ok, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(_img_alarm_icon, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(_lbl_alarm_title, LV_OBJ_FLAG_HIDDEN);

        if (flash_phase) {
            // Phase A: Warning triangle icon + "WARN"
            lv_image_set_src(_img_alarm_icon, &img_icon_warn_24x24);
            lv_obj_set_pos(_img_alarm_icon, 80, 16);
            lv_obj_set_style_image_recolor(_img_alarm_icon, UiTheme::bg(), 0);
            lv_obj_set_style_image_recolor_opa(_img_alarm_icon, LV_OPA_COVER, 0);

            lv_obj_set_style_text_font(_lbl_alarm_title, &lv_font_montserrat_24, 0);
            lv_obj_set_style_text_color(_lbl_alarm_title, UiTheme::bg(), 0);
            lv_obj_set_pos(_lbl_alarm_title, 10, 52);
            lv_obj_set_size(_lbl_alarm_title, 165, 40);
            lv_obj_set_style_text_align(_lbl_alarm_title, LV_TEXT_ALIGN_CENTER, 0);
            lv_label_set_text(_lbl_alarm_title, "WARN");
        } else {
            // Phase B: Specific alarm icon + label
            static const lv_image_dsc_t * const alarm_icons[5] = {
                &img_icon_water_16x16,
                &img_icon_egt_16x16,
                &img_icon_rev_16x16,
                &img_icon_bat_16x16,
                &img_icon_link_16x16
            };
            static const char * const alarm_labels[5] = {
                "H2O HIGH",
                "EGT HIGH",
                "OVER-REV",
                "LOW BATT",
                "NO LINK"
            };

            lv_image_set_src(_img_alarm_icon, alarm_icons[top_alarm_id]);
            lv_obj_set_pos(_img_alarm_icon, 84, 18);
            lv_obj_set_style_image_recolor(_img_alarm_icon, UiTheme::bg(), 0);
            lv_obj_set_style_image_recolor_opa(_img_alarm_icon, LV_OPA_COVER, 0);

            lv_obj_set_style_text_font(_lbl_alarm_title, &lv_font_montserrat_18, 0);
            lv_obj_set_style_text_color(_lbl_alarm_title, UiTheme::bg(), 0);
            lv_obj_set_pos(_lbl_alarm_title, 10, 56);
            lv_obj_set_size(_lbl_alarm_title, 165, 36);
            lv_obj_set_style_text_align(_lbl_alarm_title, LV_TEXT_ALIGN_CENTER, 0);
            lv_label_set_text(_lbl_alarm_title, alarm_labels[top_alarm_id]);
        }
    } else {
        // Normal OK status
        lv_obj_set_style_bg_color(_card_status, UiTheme::bg(), 0);
        lv_obj_set_style_bg_opa(_card_status, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_color(_card_status, UiTheme::fg(), 0);
        lv_obj_set_style_border_width(_card_status, 1, 0);

        lv_obj_clear_flag(_lbl_status_ok, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(_img_alarm_icon, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(_lbl_alarm_title, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(_lbl_status_ok, "SYSTEM OK");
    }

    // 7. Footer Status
    if (t.track_error_code != 0) {
        static const char* const err_strings[] = {
            "ERR: NO GPS FIX",
            "ERR: IMU FAULT",
            "ERR: SD CARD FAIL",
            "ERR: EGT SENSOR",
            "ERR: H2O SENSOR",
            "ERR: CAN BUS FAIL",
            "ERR: LOW MEMORY"
        };
        if (t.track_error_code >= 1 && t.track_error_code <= 7) {
            snprintf(buf, sizeof(buf), "%s", err_strings[t.track_error_code - 1]);
        } else {
            snprintf(buf, sizeof(buf), "ERR: CODE #%u", t.track_error_code);
        }
    } else {
        snprintf(buf, sizeof(buf), "TRACK: %s", t.current_track_name);
    }
    lv_label_set_text(_lbl_footer_track, buf);

    bool blink_1hz = ((now_ms / 500) % 2) == 0;
    bool show_bat = (t.battery_percent >= 10) || blink_1hz;
    bool show_link = t.track_module_connected || blink_1hz;

    char bat_str[32];
    snprintf(bat_str, sizeof(bat_str), "BAT: %.1fV (%d%%)", t.battery_voltage, t.battery_percent);
    const char *link_str = t.track_module_connected ? "LINK OK" : "NO LINK";

    if (show_bat && show_link) {
        snprintf(buf, sizeof(buf), "%s | %s", bat_str, link_str);
    } else if (show_bat) {
        snprintf(buf, sizeof(buf), "%s | ", bat_str);
    } else if (show_link) {
        snprintf(buf, sizeof(buf), " | %s", link_str);
    } else {
        snprintf(buf, sizeof(buf), " | ");
    }
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
