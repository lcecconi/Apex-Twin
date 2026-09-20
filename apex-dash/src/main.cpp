#include "config.h"
#include "ST7305_U8g2.h"
#include "onboard_sensors.h"
#include "input_manager.h"
#include "telemetry_data.h"
#include "telemetry_provider.h"
#include "storage_manager.h"
#include "ui/ui_manager.h"
#include "i18n.h"
#include "led_strip_manager.h"
#include "backlight_manager.h"
#include "sd_manager.h"
#include "ota_manager.h"
#include "track_manager.h"
#include "usb_storage_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <cstdio>

static const char *TAG = "APEX_DASH";

// Hardware and Subsystem instances
static ST7305_U8g2 lcd(PIN_LCD_SCK, PIN_LCD_MOSI, PIN_LCD_DC, PIN_LCD_CS, PIN_LCD_RST);
static U8G2 *u8g2 = nullptr;
static OnboardSensors onboard_sensors;
static InputManager input_manager;
static StorageManager storage_manager;
static TelemetryProvider telemetry_provider;
static LEDStripManager led_manager;
static BacklightManager backlight_manager;
static SDManager sd_manager;
static TrackManager track_manager;
static USBStorageManager usb_storage_manager;
static UiManager ui_manager;
static SystemSettings settings;

// Performance & state tracking
static uint32_t last_serial_log_ms = 0;
static uint32_t last_fps_calc_ms = 0;
static uint32_t frame_count = 0;
static float current_fps = 0.0f;
static bool last_applied_invert = true;
static uint8_t last_applied_lang = 0;

static inline uint32_t get_millis() {
  return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

extern "C" void app_main(void) {
  vTaskDelay(pdMS_TO_TICKS(500));

  printf("\n=======================================================\n");
  printf("   APEX-DASH: Open-Source Kart Racing Display Module   \n");
  printf("      (Native ESP-IDF C++ & U8g2 Telemetry System)      \n");
  printf("=======================================================\n");

  // 0. Load persisted settings from NVS
  ESP_LOGI(TAG, "Loading persisted settings from NVS...");
  storage_manager.begin();
  storage_manager.loadSettings(settings);

  // Apply localized language
  I18n::setLanguage((Language)settings.language);
  last_applied_lang = settings.language;
  ESP_LOGI(TAG, "System language configured: %s", I18n::getLanguageName((Language)settings.language));

  // 1. Initialize Inputs
  ESP_LOGI(TAG, "Setting up input controls (BOOT: G0, KEY: G18)...");
  input_manager.begin();

  // 2. Initialize Onboard Sensors
  ESP_LOGI(TAG, "Probing onboard I2C sensors (SHTC3: 0x70, PCF85063: 0x51)...");
  onboard_sensors.begin();
  const DeviceSensorsData &sens = onboard_sensors.getData();
  ESP_LOGI(TAG, "SHTC3 Sensor: %s | PCF85063 RTC: %s", sens.shtc3_found ? "ONLINE" : "NOT FOUND", sens.rtc_found ? "ONLINE" : "NOT FOUND");
  ESP_LOGI(TAG, "Steering Battery: %.2f V (%d%%)", sens.battery_voltage, sens.battery_percent);

  // 3. Initialize MicroSD & Track Database
  ESP_LOGI(TAG, "Initializing MicroSD card & Open Track Database...");
  sd_manager.begin();
  if (sd_manager.isAvailable()) {
    storage_manager.syncSDCard(settings);
    I18n::setLanguage((Language)settings.language);
    last_applied_lang = settings.language;
  }
  track_manager.begin();
  track_manager.setActiveTrackById(settings.selected_track_file);
  ESP_LOGI(TAG, "Tracks loaded: %u circuits available", (unsigned int)track_manager.getTrackCount());


  // 3b. Initialize OTA Manager (MicroSD & Wi-Fi Web Portal)
  ESP_LOGI(TAG, "Initializing OTA Manager...");
  OtaManager::instance().begin(&sd_manager);

  // 4. Initialize RGB Shift/Alarm LEDs and PWM Backlight
  ESP_LOGI(TAG, "Initializing WS2812 RGB LED strip (GPIO 1) & Backlight PWM (GPIO 2)...");
  led_manager.begin();
  led_manager.setBrightness(settings.led_brightness);
  backlight_manager.begin();
  backlight_manager.setBrightness(settings.backlight_percent);

  // 5. Initialize USB Mass Storage Manager
  usb_storage_manager.begin(&sd_manager);

  // 6. Initialize Telemetry Provider
  ESP_LOGI(TAG, "Initializing Telemetry Provider (Physics Simulation Active)...");
  telemetry_provider.begin(settings, &storage_manager, &sd_manager);


  // 7. Initialize UI Subsystem & Menu System
  ESP_LOGI(TAG, "Initializing UI Manager & Menus...");
  ui_manager.begin(&track_manager, &led_manager, &backlight_manager, &usb_storage_manager, &sd_manager, &storage_manager);

  // 8. Initialize ST7305 RLCD Display
  ESP_LOGI(TAG, "Initializing ST7305 4.2\" Reflective LCD (400x300)...");
  lcd.begin(0, U8G2_R1); // Landscape mode 400x300
  u8g2 = lcd.getU8g2();
  lcd.setInvert(settings.inverted_display);
  last_applied_invert = settings.inverted_display;

  ESP_LOGI(TAG, "Apex-Dash initialized successfully. Starting race loop.\n");
  last_fps_calc_ms = get_millis();
  last_serial_log_ms = get_millis();

  while (true) {
    // 1. Process User Inputs
    UserInputEvent event = input_manager.update();
    ui_manager.handleInput(event, settings, telemetry_provider);

    // Apply display inversion if changed in settings
    if (settings.inverted_display != last_applied_invert) {
      lcd.setInvert(settings.inverted_display);
      last_applied_invert = settings.inverted_display;
      storage_manager.saveSettings(settings);
      ESP_LOGI(TAG, "Display polarity toggled: %s", settings.inverted_display ? "INVERTED (Black on Silver)" : "NORMAL (Silver on Black)");
    }

    // Apply language if changed in settings
    if (settings.language != last_applied_lang) {
      I18n::setLanguage((Language)settings.language);
      last_applied_lang = settings.language;
      storage_manager.saveSettings(settings);
      ESP_LOGI(TAG, "Language changed: %s", I18n::getLanguageName((Language)settings.language));
    }

    // Sync SD card if detected after boot retry
    static bool s_sd_synced = sd_manager.isAvailable();
    if (sd_manager.isAvailable() && !s_sd_synced) {
      s_sd_synced = true;
      storage_manager.syncSDCard(settings);
      track_manager.seedTracksToSD();
      track_manager.loadTracksFromSD();
      I18n::setLanguage((Language)settings.language);
      last_applied_lang = settings.language;
    }

    // 2. Poll Onboard Hardware Sensors
    onboard_sensors.update();
    const DeviceSensorsData &local_sensors = onboard_sensors.getData();


    // 3. Update Telemetry State
    telemetry_provider.update(local_sensors, settings);
    const TelemetrySnapshot &telemetry = telemetry_provider.getSnapshot();

    // 4. Update RGB Shift Lights & Alarm LEDs (25 Hz)
    if (!ui_manager.isMSCActive() && !ui_manager.isOtaActive()) {
      led_manager.update(telemetry, settings);
    }

    // 5. Render Active View / Menu
    ui_manager.render(u8g2, telemetry, telemetry_provider, settings);
    u8g2->sendBuffer();

    // 6. Performance Monitoring
    frame_count++;
    uint32_t now = get_millis();
    if (now - last_fps_calc_ms >= 1000) {
      current_fps = (float)frame_count * 1000.0f / (float)(now - last_fps_calc_ms);
      frame_count = 0;
      last_fps_calc_ms = now;
    }

    // 7. Serial Telemetry Logging (1 Hz)
    if (now - last_serial_log_ms >= 1000) {
      printf("[TELEMETRY] Lap: L%02d | Spd: %03.0f km/h | RPM: %05d | Gear: %d | H2O: %04.1f C | EGT: %03.0f C | Delta: %+0.2fs | FPS: %.1f\n",
             telemetry.lap_number, telemetry.speed_kmh, telemetry.rpm, telemetry.gear,
             telemetry.water_temp_c, telemetry.exhaust_temp_c, telemetry.predictive_delta_s, current_fps);
      last_serial_log_ms = now;
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
