/**
 * @file page_telemetry.cpp
 * Telemetry & Sensor Monitor page implementation (LVGL v9)
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
    lv_obj_set_size(_container, 400, 240);
    lv_obj_set_pos(_container, 0, 0);
    lv_obj_clear_flag(_container, LV_OBJ_FLAG_SCROLLABLE);

    // Header Title
    _lbl_header_title = lv_label_create(_container);
    lv_obj_add_style(_lbl_header_title, &UiTheme::style_text_bold, 0);
    lv_obj_set_pos(_lbl_header_title, 10, 6);
    lv_label_set_text(_lbl_header_title, "TELEMETRY & SENSOR MONITOR");

    _lbl_header_lap = lv_label_create(_container);
    lv_obj_add_style(_lbl_header_lap, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_header_lap, 280, 8);
    lv_label_set_text(_lbl_header_lap, "LAP 01 | SEC 1/3");

    // Divider Line
    lv_obj_t *line = lv_obj_create(_container);
    lv_obj_remove_style_all(line);
    lv_obj_set_pos(line, 10, 24);
    lv_obj_set_size(line, 380, 1);
    lv_obj_set_style_bg_color(line, UiTheme::fg(), 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);

    // ==========================================
    // CARD 1: ENGINE RPM & GEAR
    // ==========================================
    _card_rpm = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_rpm);
    lv_obj_add_style(_card_rpm, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_rpm, 10, 30);
    lv_obj_set_size(_card_rpm, 185, 96);
    lv_obj_clear_flag(_card_rpm, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title1 = lv_label_create(_card_rpm);
    lv_obj_add_style(title1, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(title1, 4, 2);
    lv_label_set_text(title1, "ENGINE TACHO & GEAR");

    _lbl_rpm_val = lv_label_create(_card_rpm);
    lv_obj_add_style(_lbl_rpm_val, &UiTheme::style_text_large, 0);
    lv_obj_set_pos(_lbl_rpm_val, 6, 26);
    lv_label_set_text(_lbl_rpm_val, "0 RPM");

    _lbl_gear_val = lv_label_create(_card_rpm);
    lv_obj_add_style(_lbl_gear_val, &UiTheme::style_text_bold, 0);
    lv_obj_set_pos(_lbl_gear_val, 6, 66);
    lv_label_set_text(_lbl_gear_val, "Gear: N");

    // ==========================================
    // CARD 2: DUAL TEMPERATURES
    // ==========================================
    _card_temps = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_temps);
    lv_obj_add_style(_card_temps, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_temps, 205, 30);
    lv_obj_set_size(_card_temps, 185, 96);
    lv_obj_clear_flag(_card_temps, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title2 = lv_label_create(_card_temps);
    lv_obj_add_style(title2, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(title2, 4, 2);
    lv_label_set_text(title2, "THERMAL DYNAMICS");

    _lbl_water_temp = lv_label_create(_card_temps);
    lv_obj_add_style(_lbl_water_temp, &UiTheme::style_text_bold, 0);
    lv_obj_set_pos(_lbl_water_temp, 6, 28);
    lv_label_set_text(_lbl_water_temp, "Water: 58.4 C (OK)");

    _lbl_egt_temp = lv_label_create(_card_temps);
    lv_obj_add_style(_lbl_egt_temp, &UiTheme::style_text_bold, 0);
    lv_obj_set_pos(_lbl_egt_temp, 6, 56);
    lv_label_set_text(_lbl_egt_temp, "EGT:   580 C (OK)");

    // ==========================================
    // CARD 3: G-G FRICTION DIAGRAM
    // ==========================================
    _card_gg = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_gg);
    lv_obj_add_style(_card_gg, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_gg, 10, 134);
    lv_obj_set_size(_card_gg, 185, 98);
    lv_obj_clear_flag(_card_gg, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title3 = lv_label_create(_card_gg);
    lv_obj_add_style(title3, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(title3, 4, 2);
    lv_label_set_text(title3, "G-G ACCELEROMETER");

    _lbl_gg_lat = lv_label_create(_card_gg);
    lv_obj_add_style(_lbl_gg_lat, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_gg_lat, 6, 24);
    lv_label_set_text(_lbl_gg_lat, "Lat: +0.00 G");

    _lbl_gg_lon = lv_label_create(_card_gg);
    lv_obj_add_style(_lbl_gg_lon, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_gg_lon, 6, 44);
    lv_label_set_text(_lbl_gg_lon, "Lon: +0.00 G");

    _lbl_gg_peak = lv_label_create(_card_gg);
    lv_obj_add_style(_lbl_gg_peak, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_gg_peak, 6, 64);
    lv_label_set_text(_lbl_gg_peak, "Peak: 1.85 G");

    // ==========================================
    // CARD 4: TIMING & SECTORS
    // ==========================================
    _card_timing = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_timing);
    lv_obj_add_style(_card_timing, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_timing, 205, 134);
    lv_obj_set_size(_card_timing, 185, 98);
    lv_obj_clear_flag(_card_timing, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title4 = lv_label_create(_card_timing);
    lv_obj_add_style(title4, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(title4, 4, 2);
    lv_label_set_text(title4, "TIMING & SECTORS");

    _lbl_time_best = lv_label_create(_card_timing);
    lv_obj_add_style(_lbl_time_best, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_time_best, 6, 24);
    lv_label_set_text(_lbl_time_best, "Best: 48.42 s");

    _lbl_time_last = lv_label_create(_card_timing);
    lv_obj_add_style(_lbl_time_last, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_time_last, 6, 44);
    lv_label_set_text(_lbl_time_last, "Last: 48.68 s");

    _lbl_sector_splits = lv_label_create(_card_timing);
    lv_obj_add_style(_lbl_sector_splits, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_sector_splits, 6, 64);
    lv_label_set_text(_lbl_sector_splits, "S1: 16.08s | S2: 16.15s");
}

void PageTelemetry::update(const TelemetrySnapshot &t, const SystemSettings &s) {
    if (!_container) return;

    if (t.total_sectors > 1) {
        lv_label_set_text_fmt(_lbl_header_lap, "LAP %02u | SEC %u/%u", t.lap_number, t.current_sector, t.total_sectors);
    } else {
        lv_label_set_text_fmt(_lbl_header_lap, "LAP %02u", t.lap_number);
    }

    lv_label_set_text_fmt(_lbl_rpm_val, "%u RPM", t.rpm);

    if (s.drive_type == DRIVE_SHIFTER_6SPEED) {
        if (t.gear == 0) {
            lv_label_set_text(_lbl_gear_val, "Gear: N");
        } else {
            lv_label_set_text_fmt(_lbl_gear_val, "Gear: %u", t.gear);
        }
    } else {
        lv_label_set_text_fmt(_lbl_gear_val, "Speed: %.1f km/h", t.speed_kmh);
    }

    lv_label_set_text_fmt(_lbl_water_temp, "Water: %.1f C", t.water_temp_c);
    lv_label_set_text_fmt(_lbl_egt_temp, "EGT:   %.0f C", t.exhaust_temp_c);

    lv_label_set_text_fmt(_lbl_gg_lat, "Lat: %+0.2f G", t.lateral_g);
    lv_label_set_text_fmt(_lbl_gg_lon, "Lon: %+0.2f G", t.longitudinal_g);

    if (t.best_lap_time_ms > 0) {
        lv_label_set_text_fmt(_lbl_time_best, "Best: %02lu.%02lu s", (unsigned long)((t.best_lap_time_ms % 60000) / 1000), (unsigned long)((t.best_lap_time_ms % 1000) / 10));
    }
    if (t.last_lap_time_ms > 0) {
        lv_label_set_text_fmt(_lbl_time_last, "Last: %02lu.%02lu s", (unsigned long)((t.last_lap_time_ms % 60000) / 1000), (unsigned long)((t.last_lap_time_ms % 1000) / 10));
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
