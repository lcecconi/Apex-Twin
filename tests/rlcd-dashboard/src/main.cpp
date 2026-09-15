#include <Arduino.h>
#include "ST7305_U8g2.h"
#include "onboard_sensors.h"

#define LCD_WIDTH   400
#define LCD_HEIGHT  300

#define RLCD_SCK_PIN   11
#define RLCD_MOSI_PIN  12
#define RLCD_DC_PIN    5
#define RLCD_CS_PIN    40
#define RLCD_RST_PIN   41

static ST7305_U8g2 lcd(RLCD_SCK_PIN, RLCD_MOSI_PIN, RLCD_DC_PIN, RLCD_CS_PIN, RLCD_RST_PIN);
static U8G2 *u8g2 = nullptr;
static OnboardSensors sensors;

// Display state
static uint8_t view_mode = 0; // 0 = Sensor Overview, 1 = Kart Telemetry Preview
static bool last_key_state = false;
static bool last_boot_state = false;
static bool colors_inverted = true; // Default to inverted colors as requested
static uint32_t frame_count = 0;
static uint32_t last_fps_calc_ms = 0;
static uint32_t last_serial_log_ms = 0;
static float current_fps = 0.0f;
static uint32_t frame_duration_ms = 0;

// Telemetry mock variables for Mode 1
static int mock_speed = 78;
static int mock_rpm = 9800;
static int mock_gear = 4;
static int mock_speed_dir = 1;

static void drawHeader(const SensorData &data) {
  // Header background bar
  u8g2->drawBox(0, 0, LCD_WIDTH, 24);
  u8g2->setDrawColor(0); // White text on black box

  // Title
  u8g2->setFont(u8g2_font_helvB10_tr);
  u8g2->drawStr(6, 17, "APEX-TWIN");
  u8g2->setFont(u8g2_font_6x10_tr);
  u8g2->drawStr(92, 17, "[DASH-TEST]");

  // RTC Time
  char time_str[24];
  if (data.rtc_found) {
    snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", data.hour, data.minute, data.second);
  } else {
    snprintf(time_str, sizeof(time_str), "--:--:--");
  }
  int time_w = u8g2->getStrWidth(time_str);
  u8g2->setFont(u8g2_font_helvB10_tr);
  u8g2->drawStr((LCD_WIDTH - time_w) / 2, 17, time_str);

  // Battery info
  char bat_str[32];
  snprintf(bat_str, sizeof(bat_str), "%.2fV %d%%", data.battery_voltage, data.battery_percent);
  int bat_w = u8g2->getStrWidth(bat_str);
  u8g2->drawStr(LCD_WIDTH - bat_w - 6, 17, bat_str);

  // Restore draw color to black
  u8g2->setDrawColor(1);
}

static void drawSensorDashboard(const SensorData &data) {
  char buf[48];

  // ===== CARD 1: SHTC3 Environmental Sensor (Top-Left) =====
  u8g2->drawRFrame(4, 28, 192, 116, 4);
  u8g2->drawBox(4, 28, 192, 18);
  u8g2->setDrawColor(0);
  u8g2->setFont(u8g2_font_6x10_tr);
  u8g2->drawStr(10, 41, "ENVIRONMENT (SHTC3)");
  u8g2->setDrawColor(1);

  if (data.shtc3_found) {
    u8g2->setFont(u8g2_font_helvB14_tr);
    snprintf(buf, sizeof(buf), "%+.1f \xb0\x43", data.temperature); // °C
    u8g2->drawStr(14, 72, buf);

    snprintf(buf, sizeof(buf), "%.1f %% RH", data.humidity);
    u8g2->drawStr(14, 100, buf);

    u8g2->setFont(u8g2_font_6x10_tr);
    u8g2->drawStr(14, 126, "I2C 0x70: ONLINE");
  } else {
    u8g2->setFont(u8g2_font_helvB12_tr);
    u8g2->drawStr(14, 80, "SHTC3 Not Found");
    u8g2->setFont(u8g2_font_6x10_tr);
    u8g2->drawStr(14, 110, "Check I2C SDA:13 SCL:14");
  }

  // ===== CARD 2: Power & Battery (Top-Right) =====
  u8g2->drawRFrame(202, 28, 194, 116, 4);
  u8g2->drawBox(202, 28, 194, 18);
  u8g2->setDrawColor(0);
  u8g2->setFont(u8g2_font_6x10_tr);
  u8g2->drawStr(208, 41, "POWER & BATTERY");
  u8g2->setDrawColor(1);

  u8g2->setFont(u8g2_font_helvB14_tr);
  snprintf(buf, sizeof(buf), "%.2f V", data.battery_voltage);
  u8g2->drawStr(212, 72, buf);

  snprintf(buf, sizeof(buf), "Level: %d %%", data.battery_percent);
  u8g2->drawStr(212, 100, buf);

  // Battery gauge progress bar
  u8g2->drawFrame(212, 112, 140, 10);
  int bar_fill = (data.battery_percent * 136) / 100;
  if (bar_fill > 136) bar_fill = 136;
  if (bar_fill > 0) {
    u8g2->drawBox(214, 114, bar_fill, 6);
  }
  u8g2->drawBox(352, 115, 3, 4); // positive terminal

  // ===== CARD 3: ESP32-S3 System Info (Bottom-Left) =====
  u8g2->drawRFrame(4, 148, 192, 116, 4);
  u8g2->drawBox(4, 148, 192, 18);
  u8g2->setDrawColor(0);
  u8g2->setFont(u8g2_font_6x10_tr);
  u8g2->drawStr(10, 161, "ESP32-S3 SYSTEM");
  u8g2->setDrawColor(1);

  u8g2->setFont(u8g2_font_6x10_tr);
  snprintf(buf, sizeof(buf), "CPU: %lu MHz Dual-Core", (unsigned long)data.cpu_freq_mhz);
  u8g2->drawStr(12, 182, buf);

  snprintf(buf, sizeof(buf), "Heap Free: %lu KB", (unsigned long)data.free_heap_kb);
  u8g2->drawStr(12, 202, buf);

  if (data.total_psram_kb > 0) {
    snprintf(buf, sizeof(buf), "PSRAM: %lu / %lu KB", (unsigned long)data.free_psram_kb, (unsigned long)data.total_psram_kb);
  } else {
    snprintf(buf, sizeof(buf), "PSRAM: Disabled / N/A");
  }
  u8g2->drawStr(12, 222, buf);

  snprintf(buf, sizeof(buf), "Flash: 16 MB Octal/Quad");
  u8g2->drawStr(12, 242, buf);

  // ===== CARD 4: RTC & Hardware I/O (Bottom-Right) =====
  u8g2->drawRFrame(202, 148, 194, 116, 4);
  u8g2->drawBox(202, 148, 194, 18);
  u8g2->setDrawColor(0);
  u8g2->setFont(u8g2_font_6x10_tr);
  u8g2->drawStr(208, 161, "HARDWARE I/O & RTC");
  u8g2->setDrawColor(1);

  u8g2->setFont(u8g2_font_6x10_tr);
  if (data.rtc_found) {
    snprintf(buf, sizeof(buf), "RTC Date: %04d-%02d-%02d", data.year, data.month, data.day);
  } else {
    snprintf(buf, sizeof(buf), "RTC: Not Detected (0x51)");
  }
  u8g2->drawStr(210, 182, buf);

  // Button States
  snprintf(buf, sizeof(buf), "BOOT Btn (G0):  %s", data.boot_btn_pressed ? "[PRESSED]" : "RELEASED");
  u8g2->drawStr(210, 204, buf);

  snprintf(buf, sizeof(buf), "KEY  Btn (G18): %s", data.key_btn_pressed ? "[PRESSED]" : "RELEASED");
  u8g2->drawStr(210, 224, buf);

  snprintf(buf, sizeof(buf), "FPS: %.1f (%lu ms/f)", current_fps, (unsigned long)frame_duration_ms);
  u8g2->drawStr(210, 244, buf);
}

static void drawTelemetryPreview(const SensorData &data) {
  char buf[48];

  // RPM Bar across the top
  u8g2->drawFrame(10, 32, 380, 18);
  int rpm_fill = (mock_rpm * 376) / 14000;
  if (rpm_fill > 376) rpm_fill = 376;
  if (rpm_fill > 0) {
    u8g2->drawBox(12, 34, rpm_fill, 14);
  }
  u8g2->setFont(u8g2_font_6x10_tr);
  snprintf(buf, sizeof(buf), "RPM: %d / 14,000", mock_rpm);
  u8g2->drawStr(14, 64, buf);

  // Big Speed Display in Center
  u8g2->setFont(u8g2_font_logisoso50_tn);
  snprintf(buf, sizeof(buf), "%03d", mock_speed);
  int spd_w = u8g2->getStrWidth(buf);
  int spd_x = (LCD_WIDTH - spd_w) / 2 - 30;
  u8g2->drawStr(spd_x, 150, buf);

  u8g2->setFont(u8g2_font_helvB14_tr);
  u8g2->drawStr(spd_x + spd_w + 10, 140, "KM/H");

  // Gear indicator
  u8g2->drawRFrame(300, 75, 75, 90, 6);
  u8g2->setFont(u8g2_font_6x10_tr);
  u8g2->drawStr(322, 92, "GEAR");
  u8g2->setFont(u8g2_font_logisoso38_tn);
  snprintf(buf, sizeof(buf), "%d", mock_gear);
  u8g2->drawStr(326, 145, buf);

  // Lower Section: Lap Times & Live Sensor readout
  u8g2->drawHLine(10, 180, 380);

  u8g2->setFont(u8g2_font_helvB12_tr);
  u8g2->drawStr(14, 204, "LAST LAP: 48.32s");
  u8g2->drawStr(214, 204, "BEST LAP: 47.95s");

  u8g2->setFont(u8g2_font_6x12_tr);
  snprintf(buf, sizeof(buf), "DELTA: -0.37s  |  SECTOR 2  |  LAP 07 / 15");
  u8g2->drawStr(14, 230, buf);

  snprintf(buf, sizeof(buf), "SHTC3 Temp: %+.1f C | Bat: %.2fV (%d%%)", 
           data.temperature, data.battery_voltage, data.battery_percent);
  u8g2->drawStr(14, 252, buf);
}

static void drawFooter(const SensorData &data) {
  u8g2->drawHLine(0, 274, LCD_WIDTH);
  u8g2->setFont(u8g2_font_6x10_tr);

  uint32_t sec = data.uptime_sec % 60;
  uint32_t min = (data.uptime_sec / 60) % 60;
  uint32_t hr  = data.uptime_sec / 3600;

  char buf[80];
  snprintf(buf, sizeof(buf), "KEY (G18): Mode [%d/2] | BOOT (G0): Invert Color | Up: %02lu:%02lu:%02lu",
           view_mode + 1, (unsigned long)hr, (unsigned long)min, (unsigned long)sec);
  u8g2->drawStr(6, 290, buf);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n==========================================");
  Serial.println("  Waveshare ESP32-S3-RLCD-4.2 Dashboard   ");
  Serial.println("==========================================");

  // Initialize sensors and hardware
  Serial.println("[INIT] Initializing I2C bus & onboard sensors...");
  sensors.begin();
  const SensorData &d = sensors.getData();
  Serial.printf("[INIT] SHTC3 Status: %s\n", d.shtc3_found ? "DETECTED (0x70)" : "NOT FOUND");
  Serial.printf("[INIT] PCF85063 RTC Status: %s\n", d.rtc_found ? "DETECTED (0x51)" : "NOT FOUND");
  Serial.printf("[INIT] Battery Voltage: %.2f V (%d%%)\n", d.battery_voltage, d.battery_percent);
  Serial.printf("[INIT] PSRAM Size: %lu KB\n", (unsigned long)d.total_psram_kb);

  // Initialize ST7305 RLCD Display
  Serial.println("[INIT] Initializing ST7305 4.2\" Reflective LCD...");
  lcd.begin(0, U8G2_R1); // Landscape mode 400x300
  u8g2 = lcd.getU8g2();
  Serial.println("[INIT] Display initialized successfully!");

  last_fps_calc_ms = millis();
  last_serial_log_ms = millis();
}

void loop() {
  uint32_t frame_start_us = micros();

  // Read sensors
  sensors.update();
  const SensorData &data = sensors.getData();

  // Handle KEY button press to toggle views
  if (data.key_btn_pressed && !last_key_state) {
    view_mode = (view_mode + 1) % 2;
    Serial.printf("[BUTTON] KEY pressed! Switched to View Mode %d\n", view_mode);
  }
  last_key_state = data.key_btn_pressed;

  // Handle BOOT button (toggle display color inversion)
  if (data.boot_btn_pressed && !last_boot_state) {
    colors_inverted = !colors_inverted;
    lcd.setInvert(colors_inverted);
    Serial.printf("[BUTTON] BOOT pressed! Inverted colors: %s\n", colors_inverted ? "ENABLED" : "DISABLED");
  }
  last_boot_state = data.boot_btn_pressed;

  // Update telemetry simulation
  mock_speed += mock_speed_dir * 2;
  if (mock_speed > 115) { mock_speed_dir = -1; mock_gear = 5; }
  if (mock_speed < 45)  { mock_speed_dir = 1;  mock_gear = 2; }
  mock_rpm = 5000 + (mock_speed * 75);

  // Render UI
  u8g2->clearBuffer();

  drawHeader(data);
  if (view_mode == 0) {
    drawSensorDashboard(data);
  } else {
    drawTelemetryPreview(data);
  }
  drawFooter(data);

  // Push frame buffer to ST7305
  u8g2->sendBuffer();

  // Measure performance
  uint32_t frame_end_us = micros();
  frame_duration_ms = (frame_end_us - frame_start_us) / 1000;
  frame_count++;

  uint32_t now_ms = millis();
  if (now_ms - last_fps_calc_ms >= 1000) {
    current_fps = (float)frame_count * 1000.0f / (float)(now_ms - last_fps_calc_ms);
    frame_count = 0;
    last_fps_calc_ms = now_ms;
  }

  // Serial logging every 1 second
  if (now_ms - last_serial_log_ms >= 1000) {
    Serial.printf("[DASH] Temp: %+.2f C | Humi: %.1f%% | Bat: %.2fV (%d%%) | RTC: %02d:%02d:%02d | Heap: %luKB | PSRAM: %luKB | FPS: %.1f\n",
                  data.temperature, data.humidity, data.battery_voltage, data.battery_percent,
                  data.hour, data.minute, data.second,
                  (unsigned long)data.free_heap_kb, (unsigned long)data.free_psram_kb, current_fps);
    last_serial_log_ms = now_ms;
  }

  // Brief yield for system tasks
  delay(10);
}
