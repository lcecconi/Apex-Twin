/**
 * @file page_telemetry.cpp
 * Telemetry & Sensor Monitor page implementation (LVGL v9)
 * Pixel-accurate layout reproduction matching the 400x300 RLCD display
 */

#include "ui/page_telemetry.h"
#include "ui/ui_theme.h"
#include <cstdio>
#include <cmath>

namespace ApexUi {

void PageTelemetry::create(lv_obj_t *parent) {
    _container = lv_obj_create(parent);
    lv_obj_remove_style_all(_container);
    lv_obj_add_style(_container, &UiTheme::style_screen, 0);
    lv_obj_set_size(_container, 400, 300);
    lv_obj_set_pos(_container, 0, 0);
    lv_obj_clear_flag(_container, LV_OBJ_FLAG_SCROLLABLE);

    // ==========================================
    // HEADER (y = 0..24)
    // ==========================================
    _lbl_header_title = lv_label_create(_container);
    lv_obj_add_style(_lbl_header_title, &UiTheme::style_text_bold, 0);
    lv_obj_set_pos(_lbl_header_title, 10, 6);
    lv_label_set_text(_lbl_header_title, "TELEMETRY & SENSOR MONITOR");

    _lbl_header_lap = lv_label_create(_container);
    lv_obj_add_style(_lbl_header_lap, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_header_lap, 270, 8);
    lv_obj_set_size(_lbl_header_lap, 120, 16);
    lv_obj_set_style_text_align(_lbl_header_lap, LV_TEXT_ALIGN_RIGHT, 0);
    lv_label_set_text(_lbl_header_lap, "LAP 01 | SEC 1/3");

    lv_obj_t *line_top = lv_obj_create(_container);
    lv_obj_remove_style_all(line_top);
    lv_obj_set_pos(line_top, 10, 24);
    lv_obj_set_size(line_top, 380, 1);
    lv_obj_set_style_bg_color(line_top, UiTheme::fg(), 0);
    lv_obj_set_style_bg_opa(line_top, LV_OPA_COVER, 0);

    // ==========================================
    // CARD 1: ENGINE RPM & GEAR (10, 30, 185, 112)
    // ==========================================
    _card_rpm = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_rpm);
    lv_obj_add_style(_card_rpm, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_rpm, 10, 30);
    lv_obj_set_size(_card_rpm, 185, 112);
    lv_obj_clear_flag(_card_rpm, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *hdr1 = lv_obj_create(_card_rpm);
    lv_obj_remove_style_all(hdr1);
    lv_obj_add_style(hdr1, &UiTheme::style_card_title, 0);
    lv_obj_set_pos(hdr1, 0, 0);
    lv_obj_set_size(hdr1, 185, 18);
    lv_obj_t *t1 = lv_label_create(hdr1);
    lv_obj_add_style(t1, &UiTheme::style_card_title_text, 0);
    lv_obj_set_pos(t1, 6, 2);
    lv_label_set_text(t1, "ENGINE RPM & GEAR");

    _lbl_rpm_val = lv_label_create(_card_rpm);
    lv_obj_add_style(_lbl_rpm_val, &UiTheme::style_text_large, 0);
    lv_obj_set_pos(_lbl_rpm_val, 8, 26);
    lv_label_set_text(_lbl_rpm_val, "0 RPM");

    _lbl_gear_val = lv_label_create(_card_rpm);
    lv_obj_add_style(_lbl_gear_val, &UiTheme::style_text_bold, 0);
    lv_obj_set_pos(_lbl_gear_val, 115, 32);
    lv_label_set_text(_lbl_gear_val, "Gear: N");

    lv_obj_t *mini_tacho = lv_bar_create(_card_rpm);
    lv_obj_set_pos(mini_tacho, 8, 86);
    lv_obj_set_size(mini_tacho, 169, 14);
    lv_obj_set_style_bg_color(mini_tacho, UiTheme::bg(), 0);
    lv_obj_set_style_border_color(mini_tacho, UiTheme::fg(), 0);
    lv_obj_set_style_border_width(mini_tacho, 1, 0);
    lv_obj_set_style_radius(mini_tacho, 2, 0);
    lv_obj_set_style_bg_color(mini_tacho, UiTheme::fg(), LV_PART_INDICATOR);
    lv_bar_set_range(mini_tacho, 0, 16000);
    lv_bar_set_value(mini_tacho, 8500, LV_ANIM_OFF);

    // ==========================================
    // CARD 2: DUAL TEMPERATURES (205, 30, 185, 112)
    // ==========================================
    _card_temps = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_temps);
    lv_obj_add_style(_card_temps, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_temps, 205, 30);
    lv_obj_set_size(_card_temps, 185, 112);
    lv_obj_clear_flag(_card_temps, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *hdr2 = lv_obj_create(_card_temps);
    lv_obj_remove_style_all(hdr2);
    lv_obj_add_style(hdr2, &UiTheme::style_card_title, 0);
    lv_obj_set_pos(hdr2, 0, 0);
    lv_obj_set_size(hdr2, 185, 18);
    lv_obj_t *t2 = lv_label_create(hdr2);
    lv_obj_add_style(t2, &UiTheme::style_card_title_text, 0);
    lv_obj_set_pos(t2, 6, 2);
    lv_label_set_text(t2, "COOLANT & EXHAUST (EGT)");

    _lbl_water_temp = lv_label_create(_card_temps);
    lv_obj_add_style(_lbl_water_temp, &UiTheme::style_text_bold, 0);
    lv_obj_set_pos(_lbl_water_temp, 8, 36);
    lv_label_set_text(_lbl_water_temp, "H2O:  58.4 C (OK)");

    _lbl_egt_temp = lv_label_create(_card_temps);
    lv_obj_add_style(_lbl_egt_temp, &UiTheme::style_text_bold, 0);
    lv_obj_set_pos(_lbl_egt_temp, 8, 70);
    lv_label_set_text(_lbl_egt_temp, "EGT:  580 C (OK)");

    // ==========================================
    // CARD 3: G-G FRICTION DIAGRAM (10, 148, 185, 120)
    // ==========================================
    _card_gg = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_gg);
    lv_obj_add_style(_card_gg, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_gg, 10, 148);
    lv_obj_set_size(_card_gg, 185, 120);
    lv_obj_clear_flag(_card_gg, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *hdr3 = lv_obj_create(_card_gg);
    lv_obj_remove_style_all(hdr3);
    lv_obj_add_style(hdr3, &UiTheme::style_card_title, 0);
    lv_obj_set_pos(hdr3, 0, 0);
    lv_obj_set_size(hdr3, 185, 18);
    lv_obj_t *t3 = lv_label_create(hdr3);
    lv_obj_add_style(t3, &UiTheme::style_card_title_text, 0);
    lv_obj_set_pos(t3, 6, 2);
    lv_label_set_text(t3, "ACCELERATION & G-FORCE");

    // Crosshair target circle
    lv_obj_t *circle = lv_obj_create(_card_gg);
    lv_obj_remove_style_all(circle);
    lv_obj_set_style_border_color(circle, UiTheme::fg(), 0);
    lv_obj_set_style_border_width(circle, 1, 0);
    lv_obj_set_style_radius(circle, 28, 0);
    lv_obj_set_pos(circle, 10, 36);
    lv_obj_set_size(circle, 56, 56);
    lv_obj_clear_flag(circle, LV_OBJ_FLAG_SCROLLABLE);

    _lbl_gg_lat = lv_label_create(_card_gg);
    lv_obj_add_style(_lbl_gg_lat, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_gg_lat, 80, 28);
    lv_label_set_text(_lbl_gg_lat, "Lat: +0.25 G");

    _lbl_gg_lon = lv_label_create(_card_gg);
    lv_obj_add_style(_lbl_gg_lon, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_gg_lon, 80, 54);
    lv_label_set_text(_lbl_gg_lon, "Lon: -0.15 G");

    _lbl_gg_peak = lv_label_create(_card_gg);
    lv_obj_add_style(_lbl_gg_peak, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_gg_peak, 80, 80);
    lv_label_set_text(_lbl_gg_peak, "Peak: 1.85 G");

    // ==========================================
    // CARD 4: TIMING & SECTORS (205, 148, 185, 120)
    // ==========================================
    _card_timing = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_timing);
    lv_obj_add_style(_card_timing, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_timing, 205, 148);
    lv_obj_set_size(_card_timing, 185, 120);
    lv_obj_clear_flag(_card_timing, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *hdr4 = lv_obj_create(_card_timing);
    lv_obj_remove_style_all(hdr4);
    lv_obj_add_style(hdr4, &UiTheme::style_card_title, 0);
    lv_obj_set_pos(hdr4, 0, 0);
    lv_obj_set_size(hdr4, 185, 18);
    lv_obj_t *t4 = lv_label_create(hdr4);
    lv_obj_add_style(t4, &UiTheme::style_card_title_text, 0);
    lv_obj_set_pos(t4, 6, 2);
    lv_label_set_text(t4, "TIMING & SECTORS");

    _lbl_time_best = lv_label_create(_card_timing);
    lv_obj_add_style(_lbl_time_best, &UiTheme::style_text_bold, 0);
    lv_obj_set_pos(_lbl_time_best, 8, 26);
    lv_label_set_text(_lbl_time_best, "Best: 48.42 s");

    _lbl_time_last = lv_label_create(_card_timing);
    lv_obj_add_style(_lbl_time_last, &UiTheme::style_text_bold, 0);
    lv_obj_set_pos(_lbl_time_last, 8, 48);
    lv_label_set_text(_lbl_time_last, "Last: 48.68 s");

    _lbl_sector_splits = lv_label_create(_card_timing);
    lv_obj_add_style(_lbl_sector_splits, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_sector_splits, 8, 74);
    lv_label_set_text(_lbl_sector_splits, "Sector 1: 16.08s [-0.12s]\nSector 2: 16.15s [+0.05s]");

    // ==========================================
    // FOOTER (y = 276..300)
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

void PageTelemetry::update(const TelemetrySnapshot &t, const SystemSettings &s) {
    if (!_container) return;

    char buf[64];

    if (t.total_sectors > 1) {
        snprintf(buf, sizeof(buf), "LAP %02u | SEC %u/%u", t.lap_number, t.current_sector, t.total_sectors);
    } else {
        snprintf(buf, sizeof(buf), "LAP %02u", t.lap_number);
    }
    lv_label_set_text(_lbl_header_lap, buf);

    snprintf(buf, sizeof(buf), "%u RPM", t.rpm);
    lv_label_set_text(_lbl_rpm_val, buf);

    if (s.drive_type == DRIVE_SHIFTER_6SPEED) {
        if (t.gear == 0) {
            lv_label_set_text(_lbl_gear_val, "Gear: N");
        } else {
            snprintf(buf, sizeof(buf), "Gear: %u", t.gear);
            lv_label_set_text(_lbl_gear_val, buf);
        }
    } else {
        snprintf(buf, sizeof(buf), "Speed: %.1f km/h", t.speed_kmh);
        lv_label_set_text(_lbl_gear_val, buf);
    }

    float w_temp = s.use_celsius ? t.water_temp_c : (t.water_temp_c * 1.8f + 32.0f);
    float e_temp = s.use_celsius ? t.exhaust_temp_c : (t.exhaust_temp_c * 1.8f + 32.0f);
    snprintf(buf, sizeof(buf), "Water: %.1f %s", w_temp, s.use_celsius ? "C" : "F");
    lv_label_set_text(_lbl_water_temp, buf);

    snprintf(buf, sizeof(buf), "EGT:   %.0f %s", e_temp, s.use_celsius ? "C" : "F");
    lv_label_set_text(_lbl_egt_temp, buf);

    snprintf(buf, sizeof(buf), "Lat: %+0.2f G", t.lateral_g);
    lv_label_set_text(_lbl_gg_lat, buf);

    snprintf(buf, sizeof(buf), "Lon: %+0.2f G", t.longitudinal_g);
    lv_label_set_text(_lbl_gg_lon, buf);

    if (t.best_lap_time_ms > 0) {
        unsigned long b_sec = (t.best_lap_time_ms % 60000) / 1000;
        unsigned long b_cen = (t.best_lap_time_ms % 1000) / 10;
        snprintf(buf, sizeof(buf), "Best: %02lu.%02lu s", b_sec, b_cen);
        lv_label_set_text(_lbl_time_best, buf);
    }
    if (t.last_lap_time_ms > 0) {
        unsigned long l_sec = (t.last_lap_time_ms % 60000) / 1000;
        unsigned long l_cen = (t.last_lap_time_ms % 1000) / 10;
        snprintf(buf, sizeof(buf), "Last: %02lu.%02lu s", l_sec, l_cen);
        lv_label_set_text(_lbl_time_last, buf);
    }
}

void PageTelemetry::setVisible(bool visible) {
    if (!_container) return;
    if (visible) {
        lv_obj_clear_flag(_container, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(_container, LV_OBJ_FLAG_HIDDEN);
    }
}

} // namespace ApexUi
