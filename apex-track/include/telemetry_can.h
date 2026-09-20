#pragma once

#include <stdint.h>
#include <string.h>

#define CAN_FD_MAX_DLC 64

// Standard 11-bit CAN Identifier definitions
enum CanMessageId : uint32_t {
  // Downlink: Apex-Track (Chassis) -> Apex-Dash (Steering)
  CAN_ID_FAST_DYNAMICS    = 0x100, // 25-50 Hz: RPM, Speed, Gear, Lap Time, Pred Delta, Sectors
  CAN_ID_IMU_DYNAMICS     = 0x110, // 25-50 Hz: 3-axis Accel (G), 3-axis Gyro (deg/s)
  CAN_ID_SECTOR_EVENT     = 0x120, // Aperiodic: Split or S/F crossing event
  CAN_ID_ENGINE_THERMAL   = 0x200, // 2 Hz: Water, EGT, Head temps
  CAN_ID_GNSS_STATUS      = 0x210, // 1 Hz: Lat, Lon, Altitude, Sats, Fix, HDOP
  CAN_ID_CHASSIS_HEALTH   = 0x220, // 1 Hz: 12V Battery mV, Fault flags, Engine/Piston hours

  // Uplink: Apex-Dash (Steering) -> Apex-Track (Chassis)
  CAN_ID_DASH_COMMAND     = 0x300, // Aperiodic: Track selection, zero IMU, session start/stop
  CAN_ID_DASH_STATUS      = 0x310  // 1 Hz: 18650 Battery mV, SHTC3 ambient weather, RTC epoch
};

// Generic CAN-FD Frame Container
struct __attribute__((packed)) CanFdFrame {
  uint32_t id;            // 11-bit CAN ID
  uint8_t  len;           // Payload length in bytes (0 - 64)
  uint8_t  flags;         // CAN-FD flags (bit 0: BRS, bit 1: ESI)
  uint8_t  reserved[2];
  uint32_t timestamp_us;  // Microsecond arrival / dispatch timestamp
  uint8_t  data[CAN_FD_MAX_DLC];
};

// =============================================================================
// Payload Structures (Packed binary layout matching docs/telemetry_protocol.md)
// =============================================================================

// 0x100: Fast Dynamics (16 Bytes)
struct __attribute__((packed)) CanFastDynamicsPayload {
  uint16_t rpm;             // 1 RPM (0 - 25000)
  uint16_t speed_raw;       // Speed * 20 (0.05 km/h per count) -> speed_kmh = speed_raw * 0.05f
  uint8_t  gear;            // 0=N, 1-6
  uint8_t  status_flags;    // bit0: shift light, bit1: over rev, bit2: session active
  uint8_t  current_sector;  // 1 - 5
  uint8_t  total_sectors;   // 1 - 5
  uint16_t lap_number;
  uint32_t lap_time_ms;
  int16_t  pred_delta_ms;   // Signed ms: -32768 .. +32767
};

// 0x110: IMU Dynamics (16 Bytes)
struct __attribute__((packed)) CanImuDynamicsPayload {
  int16_t lateral_g_raw;      // G * 100
  int16_t longitudinal_g_raw; // G * 100
  int16_t vertical_g_raw;     // G * 100
  int16_t yaw_rate_raw;       // deg/s * 10
  int16_t pitch_deg_raw;      // deg * 20
  int16_t roll_deg_raw;       // deg * 20
  uint32_t reserved;
};

// 0x120: Sector / Lap Gate Event (24 Bytes)
struct __attribute__((packed)) CanSectorEventPayload {
  uint8_t  gate_type;       // 1 = S/F line, 2 = Intermediate split
  uint8_t  sector_index;    // 1 - 5
  uint16_t lap_number;
  uint32_t split_time_ms;
  uint32_t lap_time_ms;
  int16_t  split_delta_ms;
  uint16_t top_speed_raw;   // Speed * 20
  uint16_t max_rpm;
  uint8_t  is_new_best;     // 1 = True
  uint8_t  reserved[5];
};

// 0x200: Engine Thermal (12 Bytes)
struct __attribute__((packed)) CanEngineThermalPayload {
  int16_t  water_temp_raw;  // deg C * 10 (-20.0 .. 140.0)
  uint16_t exhaust_temp_c;  // 1 deg C (0 .. 1100)
  int16_t  head_temp_raw;   // deg C * 10
  int16_t  intake_temp_raw; // deg C * 10
  uint32_t reserved;
};

// 0x210: GNSS Status (24 Bytes)
struct __attribute__((packed)) CanGnssStatusPayload {
  int32_t  latitude_scaled;  // deg * 1e7
  int32_t  longitude_scaled; // deg * 1e7
  int16_t  altitude_m_raw;   // meters * 10
  uint16_t heading_raw;      // deg * 100
  uint8_t  satellites;
  uint8_t  fix_type;         // 0=None, 1=2D, 2=3D, 3=DGPS, 4=RTK-Float, 5=RTK-Fixed
  uint16_t hdop_raw;         // HDOP * 100
  uint8_t  reserved[8];
};

// 0x220: Chassis Health & Power (16 Bytes)
struct __attribute__((packed)) CanChassisHealthPayload {
  uint16_t battery_mv;       // 1 mV (e.g. 12600 = 12.60V)
  uint16_t error_flags;      // Bitfield
  uint32_t engine_hours_s;   // Cumulative running seconds
  uint32_t piston_hours_s;   // Running seconds since rebuild
  uint32_t log_file_index;
};

// 0x300: Dash Command to Track (16 Bytes)
struct __attribute__((packed)) CanDashCommandPayload {
  uint8_t  cmd_id;           // 0x01: Set Track, 0x02: Start, 0x03: Stop, 0x04: Tare IMU
  uint8_t  sub_param;        // e.g. sector count
  uint32_t param_u32;
  char     track_id[10];     // Null-terminated track ID string
};

// 0x310: Dash Status to Track (16 Bytes)
struct __attribute__((packed)) CanDashStatusPayload {
  uint16_t dash_battery_mv;  // 1 mV (e.g. 4050 = 4.05V)
  uint8_t  dash_battery_pct; // 0 - 100%
  int16_t  ambient_temp_raw; // deg C * 10 (from SHTC3)
  uint8_t  ambient_hum_pct;  // % RH (from SHTC3)
  uint32_t rtc_epoch_s;      // Unix timestamp (from PCF85063A)
  uint8_t  reserved[6];
};
