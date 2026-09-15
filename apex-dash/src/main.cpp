#include <Arduino.h>
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
#include "track_manager.h"
#include "usb_storage_manager.h"

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

void setup() {
  Serial.begin(115200);
  delay(800);

  Serial.println("\n=======================================================");
  Serial.println("   APEX-DASH: Open-Source Kart Racing Display Module   ");
  Serial.println("      (Inspired by  Apex-Dash Telemetry Systems)     ");
  Serial.println("=======================================================");

  // 0. Load persisted settings from NVS
  Serial.println("[INIT] Loading persisted settings from NVS...");
  storage_manager.begin();
  storage_manager.loadSettings(settings);

  // Apply localized language
  I18n::setLanguage((Language)settings.language);
  last_applied_lang = settings.language;
  Serial.printf("[INIT] System language configured: %s\n", I18n::getLanguageName((Language)settings.language));

  // 1. Initialize Inputs
  Serial.println("[INIT] Setting up input controls (BOOT: G0, KEY: G18)...");
  input_manager.begin();

  // 2. Initialize Onboard Sensors
  Serial.println("[INIT] Probing onboard I2C sensors (SHTC3: 0x70, PCF85063: 0x51)...");
  onboard_sensors.begin();
  const DeviceSensorsData &sens = onboard_sensors.getData();
  Serial.printf("[INIT] SHTC3 Sensor: %s\n", sens.shtc3_found ? "ONLINE" : "NOT FOUND");
  Serial.printf("[INIT] PCF85063 RTC: %s\n", sens.rtc_found ? "ONLINE" : "NOT FOUND");
  Serial.printf("[INIT] Steering Battery: %.2f V (%d%%)\n", sens.battery_voltage, sens.battery_percent);
  Serial.printf("[INIT] PSRAM Total: %lu KB | Heap Free: %lu KB\n", 
                (unsigned long)sens.total_psram_kb, (unsigned long)sens.free_heap_kb);

  // 3. Initialize MicroSD & Track Database
  Serial.println("[INIT] Initializing MicroSD card & Open Track Database...");
  sd_manager.begin();
  track_manager.begin();
  track_manager.setActiveTrackById(settings.selected_track_file);
  Serial.printf("[INIT] Tracks loaded: %u circuits available\n", (unsigned int)track_manager.getTrackCount());

  // 4. Initialize RGB Shift/Alarm LEDs and PWM Backlight
  Serial.println("[INIT] Initializing WS2812 RGB LED strip (GPIO 1) & Backlight PWM (GPIO 2)...");
  led_manager.begin();
  led_manager.setBrightness(settings.led_brightness);
  backlight_manager.begin();
  backlight_manager.setBrightness(settings.backlight_percent);

  // 5. Initialize USB Mass Storage Manager
  usb_storage_manager.begin(&sd_manager);

  // 6. Initialize Telemetry Provider
  Serial.println("[INIT] Initializing Telemetry Provider (Physics Simulation Active)...");
  telemetry_provider.begin(settings);

  // 7. Initialize UI Subsystem & Menu System
  Serial.println("[INIT] Initializing UI Manager & Menus...");
  ui_manager.begin(&track_manager, &led_manager, &backlight_manager, &usb_storage_manager, &sd_manager);

  // 8. Initialize ST7305 RLCD Display
  Serial.println("[INIT] Initializing ST7305 4.2\" Reflective LCD (400x300)...");
  lcd.begin(0, U8G2_R1); // Landscape mode 400x300
  u8g2 = lcd.getU8g2();
  lcd.setInvert(settings.inverted_display);
  last_applied_invert = settings.inverted_display;

  Serial.println("[INIT] Apex-Dash initialized successfully. Starting race loop.\n");
  last_fps_calc_ms = millis();
  last_serial_log_ms = millis();
}

void loop() {
  // 1. Process User Inputs
  UserInputEvent event = input_manager.update();
  ui_manager.handleInput(event, settings, telemetry_provider);

  // Apply display inversion if changed in settings
  if (settings.inverted_display != last_applied_invert) {
    lcd.setInvert(settings.inverted_display);
    last_applied_invert = settings.inverted_display;
    storage_manager.saveSettings(settings);
    Serial.printf("[UI] Display color polarity toggled & saved to NVS: %s\n", 
                  settings.inverted_display ? "INVERTED (Black on Silver)" : "NORMAL (Silver on Black)");
  }

  // Apply language if changed in settings
  if (settings.language != last_applied_lang) {
    I18n::setLanguage((Language)settings.language);
    last_applied_lang = settings.language;
    storage_manager.saveSettings(settings);
    Serial.printf("[UI] Language changed & saved to NVS: %s\n", I18n::getLanguageName((Language)settings.language));
  }

  // 2. Poll Onboard Hardware Sensors
  onboard_sensors.update();
  const DeviceSensorsData &local_sensors = onboard_sensors.getData();

  // 3. Update Telemetry State
  telemetry_provider.update(local_sensors, settings);
  const TelemetrySnapshot &telemetry = telemetry_provider.getSnapshot();

  // 4. Update RGB Shift Lights & Alarm LEDs (25 Hz)
  if (!ui_manager.isMSCActive()) {
    led_manager.update(telemetry, settings);
  }

  // 5. Render Active View / Menu
  ui_manager.render(u8g2, telemetry, telemetry_provider, settings);
  u8g2->sendBuffer();

  // 6. Performance Monitoring
  frame_count++;
  uint32_t now = millis();
  if (now - last_fps_calc_ms >= 1000) {
    current_fps = (float)frame_count * 1000.0f / (float)(now - last_fps_calc_ms);
    frame_count = 0;
    last_fps_calc_ms = now;
  }

  // 7. USB CDC Telemetry Logging (1 Hz)
  if (now - last_serial_log_ms >= 1000) {
    Serial.printf("[TELEMETRY] Lap: L%02d | Spd: %03.0f km/h | RPM: %05d | Gear: %d | H2O: %04.1f C | EGT: %03d C | Delta: %+0.2fs | FPS: %.1f\n",
                  telemetry.lap_number, telemetry.speed_kmh, telemetry.rpm, telemetry.gear,
                  telemetry.water_temp_c, (int)telemetry.exhaust_temp_c,
                  telemetry.predictive_delta_s, current_fps);
    last_serial_log_ms = now;
  }

  delay(5);
}
