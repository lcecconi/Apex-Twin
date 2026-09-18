/**
 * @file ui_manager.cpp
 * Master UI Coordinator implementation (LVGL v9)
 */

#include "ui/ui_manager.h"
#include "ui/ui_theme.h"
#include "ui/ui_icons.h"
#include <cstdio>

namespace ApexUi {

void UiManager::init(lv_obj_t *root_screen, bool inverted) {
    if (!root_screen) {
        root_screen = lv_screen_active();
    }

    init_ui_icons();
    UiTheme::init(inverted);
    lv_obj_add_style(root_screen, &UiTheme::style_screen, 0);

    // Instantiate and create all 5 pages
    _page_live_race.create(root_screen);
    _page_shumacher.create(root_screen);
    _page_telemetry.create(root_screen);
    _page_gps_paddock.create(root_screen);
    _page_data_recall.create(root_screen);

    // Initial View setup
    setViewMode(VIEW_LIVE_RACE);
}

void UiManager::setViewMode(UiViewMode mode) {
    _current_view = mode;

    _page_live_race.setVisible(_current_view == VIEW_LIVE_RACE);
    _page_shumacher.setVisible(_current_view == VIEW_SHUMACHER);
    _page_telemetry.setVisible(_current_view == VIEW_TELEMETRY);
    _page_gps_paddock.setVisible(_current_view == VIEW_GPS_PADDOCK);
    _page_data_recall.setVisible(_current_view == VIEW_DATA_RECALL);
}

void UiManager::nextView() {
    uint8_t next = ((uint8_t)_current_view + 1) % VIEW_COUNT;
    setViewMode((UiViewMode)next);
}

void UiManager::prevView() {
    uint8_t prev = (_current_view == 0) ? (VIEW_COUNT - 1) : ((uint8_t)_current_view - 1);
    setViewMode((UiViewMode)prev);
}

void UiManager::update(const TelemetrySnapshot &t, const SystemSettings &s, const LapRecord *laps, uint16_t lap_count) {
    switch (_current_view) {
        case VIEW_LIVE_RACE:
            _page_live_race.update(t, s);
            break;
        case VIEW_SHUMACHER:
            _page_shumacher.update(t, s);
            break;
        case VIEW_TELEMETRY:
            _page_telemetry.update(t, s);
            break;
        case VIEW_GPS_PADDOCK:
            _page_gps_paddock.update(t, s);
            break;
        case VIEW_DATA_RECALL:
            _page_data_recall.update(t, s, laps, lap_count);
            break;
        default:
            break;
    }
}

} // namespace ApexUi
