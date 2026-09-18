/**
 * @file page_gps_paddock.cpp
 * Paddock & Pre-Race Status page implementation (LVGL v9)
 */

#include "ui/page_gps_paddock.h"
#include "ui/ui_theme.h"
#include <cstdio>

namespace ApexUi {

void PageGpsPaddock::create(lv_obj_t *parent) {
    _container = lv_obj_create(parent);
    lv_obj_remove_style_all(_container);
    lv_obj_add_style(_container, &UiTheme::style_screen, 0);
    lv_obj_set_size(_container, 400, 240);
    lv_obj_set_pos(_container, 0, 0);
    lv_obj_clear_flag(_container, LV_OBJ_FLAG_SCROLLABLE);

    // Header
    lv_obj_t *title = lv_label_create(_container);
    lv_obj_add_style(title, &UiTheme::style_text_bold, 0);
    lv_obj_set_pos(title, 10, 6);
    lv_label_set_text(title, "PADDOCK & PRE-RACE STATUS");

    lv_obj_t *sub = lv_label_create(_container);
    lv_obj_add_style(sub, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(sub, 280, 8);
    lv_label_set_text(sub, "[STANDBY MODE]");

    lv_obj_t *line = lv_obj_create(_container);
    lv_obj_remove_style_all(line);
    lv_obj_set_pos(line, 10, 24);
    lv_obj_set_size(line, 380, 1);
    lv_obj_set_style_bg_color(line, UiTheme::fg(), 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);

    // ==========================================
    // CARD 1: GNSS RADAR
    // ==========================================
    _card_gnss = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_gnss);
    lv_obj_add_style(_card_gnss, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_gnss, 10, 30);
    lv_obj_set_size(_card_gnss, 185, 120);
    lv_obj_clear_flag(_card_gnss, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *t1 = lv_label_create(_card_gnss);
    lv_obj_add_style(t1, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(t1, 4, 2);
    lv_label_set_text(t1, "GNSS SATELLITE RADAR");

    // 8 Signal Strength Bars
    uint8_t heights[8] = {42, 38, 45, 30, 48, 44, 35, 41};
    for (int i = 0; i < 8; i++) {
        _bar_sats[i] = lv_bar_create(_card_gnss);
        lv_obj_set_pos(_bar_sats[i], 12 + (i * 18), 64 - (heights[i] / 2));
        lv_obj_set_size(_bar_sats[i], 12, heights[i] / 2);
        lv_obj_set_style_bg_color(_bar_sats[i], UiTheme::bg(), 0);
        lv_obj_set_style_bg_color(_bar_sats[i], UiTheme::fg(), LV_PART_INDICATOR);
        lv_obj_set_style_border_color(_bar_sats[i], UiTheme::fg(), 0);
        lv_obj_set_style_border_width(_bar_sats[i], 1, 0);
        lv_bar_set_value(_bar_sats[i], 100, LV_ANIM_OFF);
    }

    _lbl_sats = lv_label_create(_card_gnss);
    lv_obj_add_style(_lbl_sats, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_sats, 6, 76);
    lv_label_set_text(_lbl_sats, "Sats Visible: 18 (GPS/Gal)");

    _lbl_fix = lv_label_create(_card_gnss);
    lv_obj_add_style(_lbl_fix, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_fix, 6, 94);
    lv_label_set_text(_lbl_fix, "Fix: 3D-DGPS | HDOP: 0.85");

    // ==========================================
    // CARD 2: TRACK AUTO-DETECTION
    // ==========================================
    _card_track = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_track);
    lv_obj_add_style(_card_track, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_track, 205, 30);
    lv_obj_set_size(_card_track, 185, 120);
    lv_obj_clear_flag(_card_track, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *t2 = lv_label_create(_card_track);
    lv_obj_add_style(t2, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(t2, 4, 2);
    lv_label_set_text(t2, "TRACK AUTO-DETECTION");

    _lbl_track_name = lv_label_create(_card_track);
    lv_obj_add_style(_lbl_track_name, &UiTheme::style_text_bold, 0);
    lv_obj_set_pos(_lbl_track_name, 6, 26);
    lv_label_set_text(_lbl_track_name, "South Garda (Lonato)");

    _lbl_track_dist = lv_label_create(_card_track);
    lv_obj_add_style(_lbl_track_dist, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_track_dist, 6, 52);
    lv_label_set_text(_lbl_track_dist, "Distance to S/F: 12 m");

    lv_obj_t *lbl_status = lv_label_create(_card_track);
    lv_obj_add_style(lbl_status, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(lbl_status, 6, 72);
    lv_label_set_text(lbl_status, "Status: Ready to Race");

    _lbl_coords = lv_label_create(_card_track);
    lv_obj_add_style(_lbl_coords, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_coords, 6, 92);
    lv_label_set_text(_lbl_coords, "45.3887N  10.4795E");

    // ==========================================
    // CARD 3: MAINTENANCE & ENVIRONMENT
    // ==========================================
    _card_maint = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_maint);
    lv_obj_add_style(_card_maint, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_maint, 10, 156);
    lv_obj_set_size(_card_maint, 380, 76);
    lv_obj_clear_flag(_card_maint, LV_OBJ_FLAG_SCROLLABLE);

    _lbl_ambient = lv_label_create(_card_maint);
    lv_obj_add_style(_lbl_ambient, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_ambient, 8, 6);
    lv_label_set_text(_lbl_ambient, "Ambient Weather:  24.5 C   52% RH");

    _lbl_engine_hours = lv_label_create(_card_maint);
    lv_obj_add_style(_lbl_engine_hours, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_engine_hours, 8, 28);
    lv_label_set_text(_lbl_engine_hours, "Engine Runtime:   14h 25m total");

    _lbl_piston_hours = lv_label_create(_card_maint);
    lv_obj_add_style(_lbl_piston_hours, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_piston_hours, 8, 50);
    lv_label_set_text(_lbl_piston_hours, "Piston Runtime:   2h 15m (Service in 7h 45m)");
}

void PageGpsPaddock::update(const TelemetrySnapshot &t, const SystemSettings &s) {
    if (!_container) return;

    lv_label_set_text_fmt(_lbl_sats, "Sats Visible: %u (GPS/Gal)", t.satellites_visible);
    lv_label_set_text_fmt(_lbl_fix, "Fix: %s | HDOP: %.2f", (t.gps_fix >= 2 ? "3D-DGPS" : "No Fix"), t.hdop);
    lv_label_set_text(_lbl_track_name, t.current_track_name);

    uint32_t eng_hrs = t.engine_total_hours_sec / 3600;
    uint32_t eng_min = (t.engine_total_hours_sec % 3600) / 60;
    lv_label_set_text_fmt(_lbl_engine_hours, "Engine Runtime:   %luh %02lum total", (unsigned long)eng_hrs, (unsigned long)eng_min);

    uint32_t pis_hrs = t.piston_hours_sec / 3600;
    uint32_t pis_min = (t.piston_hours_sec % 3600) / 60;
    lv_label_set_text_fmt(_lbl_piston_hours, "Piston Runtime:   %luh %02lum", (unsigned long)pis_hrs, (unsigned long)pis_min);

    lv_label_set_text_fmt(_lbl_ambient, "Ambient Weather:  %.1f C   %.0f%% RH", t.ambient_temp_c, t.ambient_humidity_pct);
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
