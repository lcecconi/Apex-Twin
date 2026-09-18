/**
 * @file page_data_recall.cpp
 * Session Data Recall page implementation (LVGL v9)
 */

#include "ui/page_data_recall.h"
#include "ui/ui_theme.h"
#include <cstdio>

namespace ApexUi {

void PageDataRecall::create(lv_obj_t *parent) {
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
    lv_label_set_text(title, "SESSION DATA RECALL");

    _lbl_total_laps = lv_label_create(_container);
    lv_obj_add_style(_lbl_total_laps, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_total_laps, 280, 8);
    lv_label_set_text(_lbl_total_laps, "TOTAL LAPS: 12");

    lv_obj_t *line = lv_obj_create(_container);
    lv_obj_remove_style_all(line);
    lv_obj_set_pos(line, 10, 24);
    lv_obj_set_size(line, 380, 1);
    lv_obj_set_style_bg_color(line, UiTheme::fg(), 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);

    // ==========================================
    // 1. TOP 3 BEST LAPS TABLE
    // ==========================================
    _table_laps = lv_table_create(_container);
    lv_obj_set_pos(_table_laps, 10, 28);
    lv_obj_set_size(_table_laps, 380, 114);
    lv_obj_set_style_bg_color(_table_laps, UiTheme::bg(), 0);
    lv_obj_set_style_border_color(_table_laps, UiTheme::fg(), 0);
    lv_obj_set_style_border_width(_table_laps, 1, 0);
    lv_obj_set_style_radius(_table_laps, 3, 0);
    lv_obj_set_style_text_font(_table_laps, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(_table_laps, UiTheme::fg(), 0);

    lv_table_set_column_count(_table_laps, 6);
    lv_table_set_row_count(_table_laps, 4);

    lv_table_set_column_width(_table_laps, 0, 45);
    lv_table_set_column_width(_table_laps, 1, 45);
    lv_table_set_column_width(_table_laps, 2, 70);
    lv_table_set_column_width(_table_laps, 3, 115);
    lv_table_set_column_width(_table_laps, 4, 50);
    lv_table_set_column_width(_table_laps, 5, 55);

    // Table Header Row
    lv_table_set_cell_value(_table_laps, 0, 0, "RANK");
    lv_table_set_cell_value(_table_laps, 0, 1, "LAP");
    lv_table_set_cell_value(_table_laps, 0, 2, "TIME");
    lv_table_set_cell_value(_table_laps, 0, 3, "SECTORS");
    lv_table_set_cell_value(_table_laps, 0, 4, "TOP");
    lv_table_set_cell_value(_table_laps, 0, 5, "RPM");

    // Default placeholder data
    lv_table_set_cell_value(_table_laps, 1, 0, "#1");
    lv_table_set_cell_value(_table_laps, 1, 1, "L02");
    lv_table_set_cell_value(_table_laps, 1, 2, "48.42s");
    lv_table_set_cell_value(_table_laps, 1, 3, "16.0/16.1/16.2");
    lv_table_set_cell_value(_table_laps, 1, 4, "125.1");
    lv_table_set_cell_value(_table_laps, 1, 5, "15850");

    lv_table_set_cell_value(_table_laps, 2, 0, "#2");
    lv_table_set_cell_value(_table_laps, 2, 1, "L01");
    lv_table_set_cell_value(_table_laps, 2, 2, "48.68s");
    lv_table_set_cell_value(_table_laps, 2, 3, "16.1/16.3/16.2");
    lv_table_set_cell_value(_table_laps, 2, 4, "122.4");
    lv_table_set_cell_value(_table_laps, 2, 5, "15600");

    lv_table_set_cell_value(_table_laps, 3, 0, "#3");
    lv_table_set_cell_value(_table_laps, 3, 1, "L03");
    lv_table_set_cell_value(_table_laps, 3, 2, "48.75s");
    lv_table_set_cell_value(_table_laps, 3, 3, "16.2/16.3/16.2");
    lv_table_set_cell_value(_table_laps, 3, 4, "121.8");
    lv_table_set_cell_value(_table_laps, 3, 5, "15500");

    // ==========================================
    // 2. SESSION SUMMARY BOX
    // ==========================================
    _card_summary = lv_obj_create(_container);
    lv_obj_remove_style_all(_card_summary);
    lv_obj_add_style(_card_summary, &UiTheme::style_card, 0);
    lv_obj_set_pos(_card_summary, 10, 146);
    lv_obj_set_size(_card_summary, 380, 86);
    lv_obj_clear_flag(_card_summary, LV_OBJ_FLAG_SCROLLABLE);

    _lbl_best_lap = lv_label_create(_card_summary);
    lv_obj_add_style(_lbl_best_lap, &UiTheme::style_text_bold, 0);
    lv_obj_set_pos(_lbl_best_lap, 6, 4);
    lv_label_set_text(_lbl_best_lap, "Session Best Lap:  L02  (48.42 s)");

    _lbl_theo_lap = lv_label_create(_card_summary);
    lv_obj_add_style(_lbl_theo_lap, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_theo_lap, 6, 26);
    lv_label_set_text(_lbl_theo_lap, "Optimal Theoretical Lap:   48.35 s (Best Sectors)");

    _lbl_peak_g = lv_label_create(_card_summary);
    lv_obj_add_style(_lbl_peak_g, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_peak_g, 6, 44);
    lv_label_set_text(_lbl_peak_g, "Peak Cornering G-Force:    1.85 G (Turn 4 Chicane)");

    _lbl_consistency = lv_label_create(_card_summary);
    lv_obj_add_style(_lbl_consistency, &UiTheme::style_text_small, 0);
    lv_obj_set_pos(_lbl_consistency, 6, 62);
    lv_label_set_text(_lbl_consistency, "Session Consistency Index: 98.4 % (Within 0.5s)");
}

void PageDataRecall::update(const TelemetrySnapshot &t, const SystemSettings &s, const LapRecord *laps, uint16_t lap_count) {
    if (!_container) return;

    lv_label_set_text_fmt(_lbl_total_laps, "TOTAL LAPS: %u", (lap_count > 0 ? lap_count : 12));

    if (laps && lap_count > 0) {
        char buf_time[16], buf_split[32], buf_spd[16], buf_rpm[16], buf_lap[8], buf_rank[8];
        uint16_t limit = (lap_count < 3) ? lap_count : 3;

        for (uint16_t i = 0; i < limit; i++) {
            const LapRecord &lap = laps[i];
            snprintf(buf_rank, sizeof(buf_rank), "#%u", i + 1);
            snprintf(buf_lap, sizeof(buf_lap), "L%02u", lap.lap_number);
            snprintf(buf_time, sizeof(buf_time), "%02lu.%02lus", (unsigned long)((lap.lap_time_ms % 60000) / 1000), (unsigned long)((lap.lap_time_ms % 1000) / 10));

            // Dynamic 1-5 sector split formatting
            if (lap.sector_count <= 1) {
                snprintf(buf_split, sizeof(buf_split), "%.2f s", lap.sector_times_ms[0] / 1000.0f);
            } else if (lap.sector_count == 2) {
                snprintf(buf_split, sizeof(buf_split), "%.1f/%.1f", lap.sector_times_ms[0] / 1000.0f, lap.sector_times_ms[1] / 1000.0f);
            } else if (lap.sector_count == 3) {
                snprintf(buf_split, sizeof(buf_split), "%.1f/%.1f/%.1f", lap.sector_times_ms[0] / 1000.0f, lap.sector_times_ms[1] / 1000.0f, lap.sector_times_ms[2] / 1000.0f);
            } else {
                snprintf(buf_split, sizeof(buf_split), "%u Sectors", lap.sector_count);
            }

            snprintf(buf_spd, sizeof(buf_spd), "%.1f", lap.max_speed_kmh);
            snprintf(buf_rpm, sizeof(buf_rpm), "%u", lap.max_rpm);

            lv_table_set_cell_value(_table_laps, i + 1, 0, buf_rank);
            lv_table_set_cell_value(_table_laps, i + 1, 1, buf_lap);
            lv_table_set_cell_value(_table_laps, i + 1, 2, buf_time);
            lv_table_set_cell_value(_table_laps, i + 1, 3, buf_split);
            lv_table_set_cell_value(_table_laps, i + 1, 4, buf_spd);
            lv_table_set_cell_value(_table_laps, i + 1, 5, buf_rpm);
        }
    }
}

void PageDataRecall::setVisible(bool visible) {
    if (!_container) return;
    if (visible) {
        lv_obj_clear_flag(_container, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(_container, LV_OBJ_FLAG_HIDDEN);
    }
}

} // namespace ApexUi
