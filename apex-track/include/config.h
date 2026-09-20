#pragma once

#include <stdint.h>

// =============================================================================
// Radio & ESP-NOW Configuration
// =============================================================================
#define ESPNOW_WIFI_CHANNEL         1     // Primary 2.4 GHz channel
#define APEX_CAN_FD_MAGIC           0x41434644 // "ACFD"

// =============================================================================
// Telemetry Transmission Frequencies
// =============================================================================
#define DYNAMICS_DISPATCH_INTERVAL_MS   40    // 25 Hz: Fast Dynamics & IMU
#define THERMAL_DISPATCH_INTERVAL_MS    500   // 2 Hz: Water & EGT temps
#define GNSS_DISPATCH_INTERVAL_MS       1000  // 1 Hz: Lat, Lon, Alt, Fix
#define HEALTH_DISPATCH_INTERVAL_MS     1000  // 1 Hz: 12V Battery & Engine Hours
#define SERIAL_LOG_INTERVAL_MS          1000  // 1 Hz: Serial console debug

// =============================================================================
// Simulated Kart & Lonato Circuit Configuration
// =============================================================================
#define SIM_TRACK_ID                "lonato"
#define SIM_TRACK_NAME              "South Garda (Lonato)"
#define SIM_TRACK_LENGTH_M          1050.0f
#define SIM_SECTOR_1_END_M          340.0f
#define SIM_SECTOR_2_END_M          710.0f
#define SIM_TOTAL_SECTORS           3

#define SIM_MAX_RPM                 16000
#define SIM_SHIFT_RPM               14200
#define SIM_OVER_REV_RPM            15500
