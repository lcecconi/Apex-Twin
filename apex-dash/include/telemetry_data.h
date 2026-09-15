#pragma once

#include <Arduino.h>

enum DriveType : uint8_t {
  DRIVE_DIRECT = 0,     // Single gear (Direct Drive)
  DRIVE_CLUTCH,         // Centrifugal clutch (OKJ, Rotax, IAME X30)
  DRIVE_SHIFTER_6SPEED  // KZ / Shifter (1-6 gears)
};

enum RpmDisplayMode : uint8_t {
  RPM_DISP_BOTH = 0,      // Both LCD screen and external LED strip
  RPM_DISP_DISPLAY_ONLY, // Only on LCD screen
  RPM_DISP_LEDS_ONLY     // Only on external LED strip
};

enum TrackDetectionMode : uint8_t {
  TRACK_AUTO = 0,
  TRACK_MANUAL,
  TRACK_LEARNING
};

struct LapRecord {
  uint16_t lap_number;
  uint32_t lap_time_ms;
  uint32_t split1_ms;
  uint32_t split2_ms;
  uint32_t split3_ms;
  float max_speed_kmh;
  uint16_t max_rpm;
  uint16_t min_rpm;
  float max_water_temp;
  bool is_best_lap;
};

struct TelemetrySnapshot {
  uint32_t timestamp_ms;

  // Engine metrics
  uint16_t rpm;
  float speed_kmh;
  uint8_t gear;          // 0 = Neutral, 1-6 = Gears
  float water_temp_c;    // Radiator / Coolant
  float exhaust_temp_c;  // EGT / Exhaust gas

  // Lapping & Timing
  uint16_t lap_number;
  uint32_t current_lap_time_ms;
  uint32_t last_lap_time_ms;
  uint32_t best_lap_time_ms;
  float predictive_delta_s; // Negative = faster (e.g. -0.24s), Positive = slower
  uint8_t current_sector;   // 1, 2, or 3
  uint32_t last_split_delta_ms; // Delta at last sector split

  // GPS & Dynamics
  uint8_t satellites_visible;
  uint8_t gps_fix;          // 0 = None, 1 = 2D, 2 = 3D, 3 = DGPS/RTK
  float hdop;
  float lateral_g;
  float longitudinal_g;
  char current_track_name[32];

  // Device & Environmental
  float battery_voltage;
  uint8_t battery_percent;
  float ambient_temp_c;
  float ambient_humidity_pct;
  bool track_module_connected; // True if Apex-Track wireless link is active
  int8_t link_rssi;
  uint32_t engine_total_hours_sec;
  uint32_t piston_hours_sec;
};

struct SystemSettings {
  DriveType drive_type = DRIVE_SHIFTER_6SPEED;
  uint16_t max_rpm = 16000;
  uint16_t shift_rpm = 14000;
  uint16_t over_rev_rpm = 15500;
  float water_temp_alarm_c = 65.0f;
  float exhaust_temp_alarm_c = 640.0f;
  float low_bat_alarm_v = 3.40f;
  bool use_kmh = true;
  bool use_celsius = true;
  bool show_speed = true;
  bool inverted_display = true;
  uint8_t lap_hold_seconds = 5;
  bool simulation_mode = true; // Enabled when physical Apex-Track is offline
  TrackDetectionMode track_mode = TRACK_AUTO;
  char selected_track[32] = "South Garda (Lonato)";
  char selected_track_file[64] = "lonato.json";
  uint8_t language = 0; // 0 = LANG_EN, 1 = LANG_IT, 2 = LANG_FR, 3 = LANG_DE
  uint8_t led_brightness = 80; // 0 - 100%
  RpmDisplayMode rpm_display_mode = RPM_DISP_BOTH;
  bool led_shift_enable = true;
  bool led_alarm_enable = true;
  uint8_t backlight_percent = 0; // 0 - 100% (PWM to external driver)
};

