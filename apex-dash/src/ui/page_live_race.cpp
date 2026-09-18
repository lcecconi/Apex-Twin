/**
 * @file page_live_race.cpp
 * Live Race Predictive Lap HUD page implementation (LVGL v9)
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
    lv_obj_set_size(_container, 400, 240);
    lv_obj_set_pos(_container, 0, 0);
    lv_obj_clear_flag(_container, LV_OBJ_FLAG_SCROLLABLE);

    // ==========================================
    // 1. TOP TACHOMETER (RPM Progress Bar & Text)
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
    // 2. MAIN CENTER PANES (Gear & Lap Time)
    // ==========================================
    // Left: Gear Indicator Box
    _card_gear = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_gear);
    lv_obj_add_style(_card_gear, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_gear, 10, 32);
    lv_obj_set_size(_card_gear, 155, 122);
    lv_obj_clear_flag(_card_gear, LV_OBJ_FLAG_SCROLLABLE);

    _lbl_gear_title = lv_label_create(_card_gear);
    lv_obj_add_style(_lbl_gear_title, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_gear_title, 4, 4);
    lv_label_set_text(_lbl_gear_title, "GEAR");

    _lbl_gear = lv_label_create(_card_gear);
    lv_obj_add_style(_lbl_gear, &UiTheme::style_text_huge, 0);
    lv_obj_align(_lbl_gear, LV_ALIGN_CENTER, 0, 8);
    lv_label_set_text(_lbl_gear, "N");

    // Right: Lap Time, Sector & References Box
    _card_lap = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_lap);
    lv_obj_add_style(_card_lap, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_lap, 172, 32);
    lv_obj_set_size(_card_lap, 218, 122);
    lv_obj_clear_flag(_card_lap, LV_OBJ_FLAG_SCROLLABLE);

    _lbl_lap_sector = lv_label_create(_card_lap);
    lv_obj_add_style(_lbl_lap_sector, &UiTheme::style_text_bold, 0);
    lv_obj_set_pos(_lbl_lap_sector, 6, 4);
    lv_label_set_text(_lbl_lap_sector, "LAP 01");

    _lbl_lap_time = lv_label_create(_card_lap);
    lv_obj_add_style(_lbl_lap_time, &UiTheme::style_text_large, 0);
    lv_obj_set_pos(_lbl_lap_time, 6, 28);
    lv_label_set_text(_lbl_lap_time, "00:00.00");

    // Inverted Best Lap Badge
    _badge_best_lap = lv_obj_create(_card_lap);
    lv_obj_remove_style_all(_badge_best_lap);
    lv_obj_add_style(_badge_best_lap, &UiTheme::style_badge_inverted, 0);
    lv_obj_set_pos(_badge_best_lap, 6, 88);
    lv_obj_set_height(_badge_best_lap, 22);

    _lbl_best_lap = lv_label_create(_badge_best_lap);
    lv_obj_add_style(_lbl_best_lap, &UiTheme::style_text_small, 0);
    lv_obj_set_style_text_color(_lbl_best_lap, UiTheme::bg(), 0);
    lv_obj_center(_lbl_best_lap);
    lv_label_set_text(_lbl_best_lap, "BEST --.--");

    _lbl_last_lap = lv_label_create(_card_lap);
    lv_obj_add_style(_lbl_last_lap, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_last_lap, 120, 92);
    lv_label_set_text(_lbl_last_lap, "LAST: --.--");

    // ==========================================
    // 3. BOTTOM-LEFT: PREDICTIVE LAP DELTA
    // ==========================================
    _card_delta = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_delta);
    lv_obj_add_style(_card_delta, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_delta, 10, 160);
    lv_obj_set_size(_card_delta, 175, 54);
    lv_obj_clear_flag(_card_delta, LV_OBJ_FLAG_SCROLLABLE);

    _lbl_delta_title = lv_label_create(_card_delta);
    lv_obj_add_style(_lbl_delta_title, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_delta_title, 4, 2);
    lv_label_set_text(_lbl_delta_title, "PRED DELTA");

    _lbl_delta = lv_label_create(_card_delta);
    lv_obj_add_style(_lbl_delta, &UiTheme::style_text_large, 0);
    lv_obj_align(_lbl_delta, LV_ALIGN_CENTER, 0, 6);
    lv_label_set_text(_lbl_delta, "+ 0.00");

    // ==========================================
    // 4. BOTTOM-RIGHT: STATUS & ALARMS
    // ==========================================
    _card_status = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_status);
    lv_obj_add_style(_card_status, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_status, 192, 160);
    lv_obj_set_size(_card_status, 198, 54);
    lv_obj_clear_flag(_card_status, LV_OBJ_FLAG_SCROLLABLE);

    _lbl_status_temp = lv_label_create(_card_status);
    lv_obj_add_style(_lbl_status_temp, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_status_temp, 6, 4);
    lv_label_set_text(_lbl_status_temp, "H2O: --.- C  |  EGT: --- C");

    _lbl_status_bat = lv_label_create(_card_status);
    lv_obj_add_style(_lbl_status_bat, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_status_bat, 6, 20);
    lv_label_set_text(_lbl_status_bat, "BAT: 100% (4.1V)");

    _lbl_status_link = lv_label_create(_card_status);
    lv_obj_add_style(_lbl_status_link, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_status_link, 6, 36);
    lv_label_set_text(_lbl_status_link, "TRACK: LINK OK (-65dB)");
}

void PageLiveRace::update(const TelemetrySnapshot &t, const SystemSettings &s) {
    if (!_container) return;

    // 1. Tachometer
    lv_bar_set_range(_bar_rpm, 0, s.max_rpm);
    lv_bar_set_value(_bar_rpm, t.rpm, LV_ANIM_OFF);
    lv_label_set_text_fmt(_lbl_rpm, "%u RPM", t.rpm);

    // 2. Gear Indicator
    if (s.drive_type == DRIVE_SHIFTER_6SPEED) {
        if (t.gear == 0) {
            lv_label_set_text(_lbl_gear, "N");
        } else {
            lv_label_set_text_fmt(_lbl_gear, "%u", t.gear);
        }
    } else {
        lv_label_set_text_fmt(_lbl_gear, "%.0f", t.speed_kmh);
        lv_label_set_text(_lbl_gear_title, "KM/H");
    }

    // 3. Lap & Dynamic 1-5 Sector Badge
    if (t.total_sectors > 1) {
        lv_label_set_text_fmt(_lbl_lap_sector, "LAP %02u  [SEC %u/%u]", t.lap_number, t.current_sector, t.total_sectors);
    } else {
        lv_label_set_text_fmt(_lbl_lap_sector, "LAP %02u", t.lap_number);
    }

    // 4. Current Lap Time
    uint32_t active_ms = t.current_lap_time_ms;
    unsigned long lap_min = (unsigned long)(active_ms / 60000);
    unsigned long lap_sec = (unsigned long)((active_ms % 60000) / 1000);
    unsigned long lap_cen = (unsigned long)((active_ms % 1000) / 10);
    lv_label_set_text_fmt(_lbl_lap_time, "%02lu:%02lu.%02lu", lap_min, lap_sec, lap_cen);

    // 5. Best & Last Lap
    if (t.best_lap_time_ms > 0) {
        unsigned long b_sec = (unsigned long)((t.best_lap_time_ms % 60000) / 1000);
        unsigned long b_cen = (unsigned long)((t.best_lap_time_ms % 1000) / 10);
        lv_label_set_text_fmt(_lbl_best_lap, "BEST %02lu.%02lu", b_sec, b_cen);
    } else {
        lv_label_set_text(_lbl_best_lap, "BEST --.--");
    }

    if (t.last_lap_time_ms > 0) {
        unsigned long l_sec = (unsigned long)((t.last_lap_time_ms % 60000) / 1000);
        unsigned long l_cen = (unsigned long)((t.last_lap_time_ms % 1000) / 10);
        lv_label_set_text_fmt(_lbl_last_lap, "LAST: %02lu.%02lu", l_sec, l_cen);
    } else {
        lv_label_set_text(_lbl_last_lap, "LAST: --.--");
    }

    // 6. Predictive Delta
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

    // 7. Status & Alarms
    lv_label_set_text_fmt(_lbl_status_temp, "H2O: %.1f C  |  EGT: %.0f C", t.water_temp_c, t.exhaust_temp_c);
    lv_label_set_text_fmt(_lbl_status_bat, "BAT: %u%% (%.2fV)", t.battery_percent, t.battery_voltage);
    if (t.track_module_connected) {
        lv_label_set_text_fmt(_lbl_status_link, "TRACK: LINK OK (%ddB)", t.link_rssi);
    } else {
        lv_label_set_text(_lbl_status_link, "TRACK: DISCONNECTED");
    }
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
