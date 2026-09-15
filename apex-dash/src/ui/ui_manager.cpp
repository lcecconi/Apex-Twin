#include "ui/ui_manager.h"
#include "i18n.h"

void UiManager::begin(TrackManager *trackMgr, LEDStripManager *ledMgr, BacklightManager *blMgr, USBStorageManager *usbMgr, SDManager *sdMgr) {
  _current_view = VIEW_LIVE_RACE;
  _menu.begin(trackMgr, ledMgr, blMgr, usbMgr, sdMgr);
}

void UiManager::handleInput(UserInputEvent event, SystemSettings &settings, TelemetryProvider &provider) {
  if (event == INPUT_NONE) return;

  // If menu is open, it consumes all inputs
  if (_menu.isMenuActive()) {
    _menu.handleInput(event, settings, &provider);
    return;
  }

  // Open Menu on Back/Menu button event (BOOT long press)
  if (event == INPUT_BACK_MENU) {
    _menu.openMenu();
    return;
  }

  // Navigation across racing pages
  if (event == INPUT_NEXT) {
    _current_view = (UiViewMode)((_current_view + 1) % VIEW_COUNT);
  } else if (event == INPUT_PREV) {
    _current_view = (UiViewMode)((_current_view - 1 + VIEW_COUNT) % VIEW_COUNT);
  } else if (event == INPUT_SELECT) {
    // Quick action: toggle display color inversion
    settings.inverted_display = !settings.inverted_display;
  }
}

void UiManager::render(U8G2 *u8g2, const TelemetrySnapshot &telemetry, const TelemetryProvider &provider, const SystemSettings &settings) {
  // If menu is open, render menu
  if (_menu.isMenuActive()) {
    _menu.render(u8g2, settings, telemetry);
    return;
  }

  u8g2->clearBuffer();

  // Render current active racing page
  switch (_current_view) {
    case VIEW_LIVE_RACE:
      _page_live_race.render(u8g2, telemetry, settings);
      break;
    case VIEW_TELEMETRY:
      _page_telemetry.render(u8g2, telemetry, settings);
      break;
    case VIEW_GPS_PADDOCK:
      _page_gps_paddock.render(u8g2, telemetry, settings);
      break;
    case VIEW_DATA_RECALL:
      _page_data_recall.render(u8g2, provider, settings);
      break;
    default:
      _page_live_race.render(u8g2, telemetry, settings);
      break;
  }

  renderFooter(u8g2, telemetry);
}

void UiManager::renderFooter(U8G2 *u8g2, const TelemetrySnapshot &telemetry) {
  if (_current_view == VIEW_LIVE_RACE) {
    return; // PageLiveRace renders Track info & status on the bottom line
  }

  u8g2->drawHLine(0, 276, 400);
  u8g2->setFont(u8g2_font_6x10_tr);

  const char *view_names[VIEW_COUNT] = {
    "RACE HUD",
    "TELEMETRY",
    "PADDOCK",
    "DATA RECALL"
  };

  char buf[80];
  snprintf(buf, sizeof(buf), "KEY: Page [%s %d/4] | BOOT (Long): Menu | Select: Invert",
           view_names[_current_view], _current_view + 1);
  u8g2->drawStr(6, 292, buf);
}

