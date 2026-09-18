#pragma once

#include <Arduino.h>
#include "telemetry_can.h"

#define MAX_TRACK_SECTORS 5

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

// Lap Record supporting 1 to 5 dynamic sectors
struct LapRecord {
  uint16_t lap_number = 0;
  uint32_t lap_time_ms = 0;
  uint8_t  sector_count = 3;
  uint32_t sector_times_ms[MAX_TRACK_SECTORS] = {0};
  float    max_speed_kmh = 0.0f;
  uint16_t max_rpm = 0;
  uint16_t min_rpm = 0;
  float    max_water_temp = 0.0f;
  bool     is_best_lap = false;

  // Compatibility helpers
  uint32_t getSectorTime(uint8_t idx) const {
    return (idx < sector_count && idx < MAX_TRACK_SECTORS) ? sector_times_ms[idx] : 0;
  }
};

// =============================================================================
// Domain Model: Chassis Telemetry (Acquired and broadcast by Apex-Track)
// =============================================================================
struct ChassisTelemetry {
  // Fast Dynamics (CAN 0x100 & 0x110)
  uint16_t rpm = 0;
  float speed_kmh = 0.0f;
  uint8_t gear = 0;             // 0 = Neutral, 1-6 = Gears
  uint8_t status_flags = 0;
  uint8_t current_sector = 1;   // 1 to total_sectors
  uint8_t total_sectors = 3;    // Configured sectors for active circuit (1 to 5)
  uint16_t lap_number = 1;
  uint32_t current_lap_time_ms = 0;
  uint32_t last_lap_time_ms = 0;
  uint32_t best_lap_time_ms = 0;
  float predictive_delta_s = 0.0f; // Negative = faster, Positive = slower
  uint32_t last_split_delta_ms = 0;

  // IMU Dynamics (CAN 0x110)
  float lateral_g = 0.0f;
  float longitudinal_g = 0.0f;
  float vertical_g = 0.0f;
  float yaw_rate_dps = 0.0f;

  // Thermal Dynamics (CAN 0x200)
  float water_temp_c = 0.0f;
  float exhaust_temp_c = 0.0f;
  float head_temp_c = 0.0f;

  // GNSS Navigation & Quality (CAN 0x210)
  double latitude = 0.0;
  double longitude = 0.0;
  float altitude_m = 0.0f;
  float heading_deg = 0.0f;
  uint8_t satellites_visible = 0;
  uint8_t gps_fix = 0;          // 0 = None, 1 = 2D, 2 = 3D, 3 = DGPS, 4 = RTK-Float, 5 = RTK-Fixed
  float hdop = 9.9f;

  // Health & Power (CAN 0x220)
  float chassis_battery_voltage = 0.0f;
  uint32_t engine_total_hours_sec = 0;
  uint32_t piston_hours_sec = 0;
  uint16_t track_error_code = 0;

  // Transport Link Quality
  bool connected = false;
  int8_t link_rssi = -90;
  uint32_t last_packet_ms = 0;
};

// =============================================================================
// Domain Model: Dash Local State (Measured locally on Apex-Dash)
// =============================================================================
struct DashLocalState {
  float ambient_temp_c = 25.0f;      // Sensirion SHTC3
  float ambient_humidity_pct = 50.0f;// Sensirion SHTC3
  float battery_voltage = 4.0f;      // Steering unit 18650 Li-Ion cell
  uint8_t battery_percent = 90;
  uint32_t rtc_epoch_s = 0;          // NXP PCF85063A RTC
  uint32_t session_time_sec = 0;
  bool session_active = false;
  char current_track_name[32] = "South Garda (Lonato)";
};

// =============================================================================
// Unified Telemetry Snapshot (Used by Dash UI renderers & emulator)
// =============================================================================
struct TelemetrySnapshot {
  uint32_t timestamp_ms = 0;

  // Domain state blocks
  ChassisTelemetry chassis;
  DashLocalState local;

  // Flat field convenience mapping for existing UI code
  uint16_t rpm = 0;
  float speed_kmh = 0.0f;
  uint8_t gear = 0;
  float water_temp_c = 0.0f;
  float exhaust_temp_c = 0.0f;

  uint16_t lap_number = 1;
  uint32_t current_lap_time_ms = 0;
  uint32_t last_lap_time_ms = 0;
  uint32_t best_lap_time_ms = 0;
  float predictive_delta_s = 0.0f;
  uint8_t current_sector = 1;
  uint8_t total_sectors = 3;
  uint32_t last_split_delta_ms = 0;

  uint8_t satellites_visible = 0;
  uint8_t gps_fix = 0;
  float hdop = 9.9f;
  float lateral_g = 0.0f;
  float longitudinal_g = 0.0f;
  char current_track_name[32] = "South Garda (Lonato)";

  float battery_voltage = 4.0f;
  uint8_t battery_percent = 90;
  float ambient_temp_c = 25.0f;
  float ambient_humidity_pct = 50.0f;
  bool track_module_connected = false;
  int8_t link_rssi = -90;
  uint32_t engine_total_hours_sec = 0;
  uint32_t piston_hours_sec = 0;
  uint32_t session_time_sec = 0;
  bool session_active = false;
  uint16_t track_error_code = 0;

  void syncFlatFields() {
    rpm = chassis.rpm;
    speed_kmh = chassis.speed_kmh;
    gear = chassis.gear;
    water_temp_c = chassis.water_temp_c;
    exhaust_temp_c = chassis.exhaust_temp_c;
    lap_number = chassis.lap_number;
    current_lap_time_ms = chassis.current_lap_time_ms;
    last_lap_time_ms = chassis.last_lap_time_ms;
    best_lap_time_ms = chassis.best_lap_time_ms;
    predictive_delta_s = chassis.predictive_delta_s;
    current_sector = chassis.current_sector;
    total_sectors = chassis.total_sectors;
    last_split_delta_ms = chassis.last_split_delta_ms;
    satellites_visible = chassis.satellites_visible;
    gps_fix = chassis.gps_fix;
    hdop = chassis.hdop;
    lateral_g = chassis.lateral_g;
    longitudinal_g = chassis.longitudinal_g;
    track_module_connected = chassis.connected;
    link_rssi = chassis.link_rssi;
    engine_total_hours_sec = chassis.engine_total_hours_sec;
    piston_hours_sec = chassis.piston_hours_sec;
    track_error_code = chassis.track_error_code;

    battery_voltage = local.battery_voltage;
    battery_percent = local.battery_percent;
    ambient_temp_c = local.ambient_temp_c;
    ambient_humidity_pct = local.ambient_humidity_pct;
    session_time_sec = local.session_time_sec;
    session_active = local.session_active;
    strncpy(current_track_name, local.current_track_name, sizeof(current_track_name) - 1);
  }
};

enum TrackErrorCode : uint16_t {
  TRACK_ERR_NONE = 0,
  TRACK_ERR_GPS_NO_FIX = 1,
  TRACK_ERR_IMU_FAULT = 2,
  TRACK_ERR_SD_CARD = 3,
  TRACK_ERR_SENSOR_EGT = 4,
  TRACK_ERR_SENSOR_H2O = 5,
  TRACK_ERR_CAN_BUS = 6,
  TRACK_ERR_LOW_MEM = 7
};

enum AlarmType : uint8_t {
  ALARM_WATER = 0,
  ALARM_EGT = 1,
  ALARM_REV = 2,
  ALARM_BAT = 3,
  ALARM_LINK = 4,
  ALARM_COUNT = 5
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
  bool warn_trigger_water = true;
  bool warn_trigger_egt = true;
  bool warn_trigger_rev = true;
  bool warn_trigger_battery = true;
  bool warn_trigger_link = true;
  uint8_t alarm_priority[5] = {0, 1, 2, 3, 4}; // 0=Water, 1=EGT, 2=OverRev, 3=LowBat, 4=Link
};
