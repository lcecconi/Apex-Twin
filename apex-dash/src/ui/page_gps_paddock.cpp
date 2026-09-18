/**
 * @file page_gps_paddock.cpp
 * Paddock & Pre-Race Status page implementation (LVGL v9)
 * Pixel-accurate layout reproduction matching the 400x300 RLCD display
 */

#include "ui/page_gps_paddock.h"
#include "ui/ui_theme.h"
#include <cstdio>

namespace ApexUi {

void PageGpsPaddock::create(lv_obj_t *parent) {
    _container = lv_obj_create(parent);
    lv_obj_remove_style_all(_container);
    lv_obj_add_style(_container, &UiTheme::style_screen, 0);
    lv_obj_set_size(_container, 400, 300);
    lv_obj_set_pos(_container, 0, 0);
    lv_obj_clear_flag(_container, LV_OBJ_FLAG_SCROLLABLE);

    // ==========================================
    // HEADER (y = 0..24)
    // ==========================================
    lv_obj_t *title = lv_label_create(_container);
    lv_obj_add_style(title, &UiTheme::style_text_bold, 0);
    lv_obj_set_pos(title, 10, 6);
    lv_label_set_text(title, "PADDOCK & PRE-RACE STATUS");

    lv_obj_t *sub = lv_label_create(_container);
    lv_obj_add_style(sub, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(sub, 270, 8);
    lv_obj_set_size(sub, 120, 16);
    lv_obj_set_style_text_align(sub, LV_TEXT_ALIGN_RIGHT, 0);
    lv_label_set_text(sub, "[STANDBY MODE]");

    lv_obj_t *line = lv_obj_create(_container);
    lv_obj_remove_style_all(line);
    lv_obj_set_pos(line, 10, 24);
    lv_obj_set_size(line, 380, 1);
    lv_obj_set_style_bg_color(line, UiTheme::fg(), 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);

    // ==========================================
    // CARD 1: GNSS SATELLITE RADAR (10, 30, 185, 130)
    // ==========================================
    _card_gnss = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_gnss);
    lv_obj_add_style(_card_gnss, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_gnss, 10, 30);
    lv_obj_set_size(_card_gnss, 185, 130);
    lv_obj_clear_flag(_card_gnss, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *hdr1 = lv_obj_create(_card_gnss);
    lv_obj_remove_style_all(hdr1);
    lv_obj_add_style(hdr1, &UiTheme::style_card_title, 0);
    lv_obj_set_pos(hdr1, 0, 0);
    lv_obj_set_size(hdr1, 185, 18);
    lv_obj_t *t1 = lv_label_create(hdr1);
    lv_obj_add_style(t1, &UiTheme::style_card_title_text, 0);
    lv_obj_set_pos(t1, 6, 2);
    lv_label_set_text(t1, "GNSS SATELLITE RADAR");

    // 8 Signal Strength Bars
    uint8_t heights[8] = {42, 38, 45, 30, 48, 44, 35, 41};
    for (int i = 0; i < 8; i++) {
        _bar_sats[i] = lv_bar_create(_card_gnss);
        lv_obj_set_pos(_bar_sats[i], 12 + (i * 20), 62 - (heights[i] / 2));
        lv_obj_set_size(_bar_sats[i], 12, heights[i] / 2);
        lv_obj_set_style_bg_color(_bar_sats[i], UiTheme::bg(), 0);
        lv_obj_set_style_bg_color(_bar_sats[i], UiTheme::fg(), LV_PART_INDICATOR);
        lv_obj_set_style_border_color(_bar_sats[i], UiTheme::fg(), 0);
        lv_obj_set_style_border_width(_bar_sats[i], 1, 0);
        lv_bar_set_value(_bar_sats[i], 100, LV_ANIM_OFF);
    }

    _lbl_sats = lv_label_create(_card_gnss);
    lv_obj_add_style(_lbl_sats, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_sats, 6, 72);
    lv_label_set_text(_lbl_sats, "Sats Visible: 18 (GPS/Gal)");

    _lbl_fix = lv_label_create(_card_gnss);
    lv_obj_add_style(_lbl_fix, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_fix, 6, 90);
    lv_label_set_text(_lbl_fix, "Fix: 3D-DGPS | HDOP: 0.85");

    lv_obj_t *lbl_ant = lv_label_create(_card_gnss);
    lv_obj_add_style(lbl_ant, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(lbl_ant, 6, 108);
    lv_label_set_text(lbl_ant, "Antenna: Active Helix L1/L5");

    // ==========================================
    // CARD 2: TRACK AUTO-DETECTION (205, 30, 185, 130)
    // ==========================================
    _card_track = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_track);
    lv_obj_add_style(_card_track, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_track, 205, 30);
    lv_obj_set_size(_card_track, 185, 130);
    lv_obj_clear_flag(_card_track, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *hdr2 = lv_obj_create(_card_track);
    lv_obj_remove_style_all(hdr2);
    lv_obj_add_style(hdr2, &UiTheme::style_card_title, 0);
    lv_obj_set_pos(hdr2, 0, 0);
    lv_obj_set_size(hdr2, 185, 18);
    lv_obj_t *t2 = lv_label_create(hdr2);
    lv_obj_add_style(t2, &UiTheme::style_card_title_text, 0);
    lv_obj_set_pos(t2, 6, 2);
    lv_label_set_text(t2, "TRACK AUTO-DETECTION");

    _lbl_track_name = lv_label_create(_card_track);
    lv_obj_add_style(_lbl_track_name, &UiTheme::style_text_bold, 0);
    lv_obj_set_pos(_lbl_track_name, 6, 26);
    lv_label_set_text(_lbl_track_name, "South Garda (Lonato)");

    _lbl_track_dist = lv_label_create(_card_track);
    lv_obj_add_style(_lbl_track_dist, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_track_dist, 6, 50);
    lv_label_set_text(_lbl_track_dist, "Distance to S/F: 12 m");

    lv_obj_t *lbl_status = lv_label_create(_card_track);
    lv_obj_add_style(lbl_status, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(lbl_status, 6, 68);
    lv_label_set_text(lbl_status, "Status: Ready to Race");

    lv_obj_t *lbl_trip = lv_label_create(_card_track);
    lv_obj_add_style(lbl_trip, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(lbl_trip, 6, 86);
    lv_label_set_text(lbl_trip, "Auto-Trip: Speed > 15 km/h");

    lv_obj_t *lbl_coords = lv_label_create(_card_track);
    lv_obj_add_style(lbl_coords, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(lbl_coords, 6, 106);
    lv_label_set_text(lbl_coords, "45.3887N 10.4795E");

    // ==========================================
    // CARD 3: WEATHER & MAINTENANCE (10, 166, 380, 102)
    // ==========================================
    _card_maint = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_maint);
    lv_obj_add_style(_card_maint, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_maint, 10, 166);
    lv_obj_set_size(_card_maint, 380, 102);
    lv_obj_clear_flag(_card_maint, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *hdr3 = lv_obj_create(_card_maint);
    lv_obj_remove_style_all(hdr3);
    lv_obj_add_style(hdr3, &UiTheme::style_card_title, 0);
    lv_obj_set_pos(hdr3, 0, 0);
    lv_obj_set_size(hdr3, 380, 18);
    lv_obj_t *t3 = lv_label_create(hdr3);
    lv_obj_add_style(t3, &UiTheme::style_card_title_text, 0);
    lv_obj_set_pos(t3, 6, 2);
    lv_label_set_text(t3, "WEATHER & ENGINE MAINTENANCE");

    _lbl_ambient = lv_label_create(_card_maint);
    lv_obj_add_style(_lbl_ambient, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_ambient, 8, 26);
    lv_label_set_text(_lbl_ambient, "Ambient Temp:   +24.5 C\nHumidity:       52.0 % RH\nBattery:        4.12 V (98%)");

    _lbl_engine_hours = lv_label_create(_card_maint);
    lv_obj_add_style(_lbl_engine_hours, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_engine_hours, 210, 26);
    lv_label_set_text(_lbl_engine_hours, "Engine Total: 14h 25m");

    _lbl_piston_hours = lv_label_create(_card_maint);
    lv_obj_add_style(_lbl_piston_hours, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_piston_hours, 210, 48);
    lv_label_set_text(_lbl_piston_hours, "Piston Run:   2h 15m");

    lv_obj_t *lbl_link_tech = lv_label_create(_card_maint);
    lv_obj_add_style(lbl_link_tech, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(lbl_link_tech, 210, 70);
    lv_label_set_text(lbl_link_tech, "Apex-Track:   2.4GHz Link OK");

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

void PageGpsPaddock::update(const TelemetrySnapshot &t, const SystemSettings &s) {
    if (!_container) return;

    char buf[128];

    snprintf(buf, sizeof(buf), "Sats Visible: %u (GPS/Gal)", t.satellites_visible);
    lv_label_set_text(_lbl_sats, buf);

    snprintf(buf, sizeof(buf), "Fix: %s | HDOP: %.2f", (t.gps_fix >= 2 ? "3D-DGPS" : "No Fix"), t.hdop);
    lv_label_set_text(_lbl_fix, buf);

    lv_label_set_text(_lbl_track_name, t.current_track_name);

    unsigned long eng_hrs = t.engine_total_hours_sec / 3600;
    unsigned long eng_min = (t.engine_total_hours_sec % 3600) / 60;
    snprintf(buf, sizeof(buf), "Engine Total: %luh %02lum", eng_hrs, eng_min);
    lv_label_set_text(_lbl_engine_hours, buf);

    unsigned long pis_hrs = t.piston_hours_sec / 3600;
    unsigned long pis_min = (t.piston_hours_sec % 3600) / 60;
    snprintf(buf, sizeof(buf), "Piston Run:   %luh %02lum", pis_hrs, pis_min);
    lv_label_set_text(_lbl_piston_hours, buf);

    float amb_temp = s.use_celsius ? t.ambient_temp_c : (t.ambient_temp_c * 1.8f + 32.0f);
    snprintf(buf, sizeof(buf), "Ambient Temp:   %+.1f %s\nHumidity:       %.1f %% RH\nBattery:        %.2f V (%u%%)",
             amb_temp, s.use_celsius ? "C" : "F", t.ambient_humidity_pct, t.battery_voltage, t.battery_percent);
    lv_label_set_text(_lbl_ambient, buf);
}

void PageGpsPaddock::setVisible(bool visible) {
    if (!_container) return;
    if (visible) {
        lv_obj_clear_flag(_container, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(_container, LV_OBJ_FLAG_HIDDEN);
    }
}

} // namespace ApexUi
