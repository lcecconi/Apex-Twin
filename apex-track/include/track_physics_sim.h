#pragma once

#include <stdint.h>
#include "telemetry_can.h"

struct SectorEventInfo {
  bool triggered = false;
  uint8_t gate_type = 0;        // 1 = S/F line, 2 = Intermediate split
  uint8_t sector_index = 1;     // 1 to 3
  uint16_t lap_number = 1;
  uint32_t split_time_ms = 0;
  uint32_t lap_time_ms = 0;
  int16_t split_delta_ms = 0;
  uint16_t top_speed_raw = 0;
  uint16_t max_rpm = 0;
  bool is_new_best = false;
};

class TrackPhysicsSim {
public:
  TrackPhysicsSim();
  void begin(uint32_t now_ms);
  void update(uint32_t now_ms);

  void fillFastDynamics(CanFastDynamicsPayload &payload) const;
  void fillImuDynamics(CanImuDynamicsPayload &payload) const;
  void fillEngineThermal(CanEngineThermalPayload &payload) const;
  void fillGnssStatus(CanGnssStatusPayload &payload) const;
  void fillChassisHealth(CanChassisHealthPayload &payload) const;

  bool checkAndClearSectorEvent(SectorEventInfo &event);

  // Getters for debug serial logging
  uint16_t getRpm() const { return _rpm; }
  float getSpeedKmh() const { return _speed_kmh; }
  uint8_t getGear() const { return _gear; }
  uint16_t getLapNumber() const { return _lap_number; }
  uint8_t getCurrentSector() const { return _current_sector; }
  float getPredictiveDelta() const { return _pred_delta_ms / 1000.0f; }
  float getWaterTemp() const { return _water_temp_c; }
  float getExhaustTemp() const { return _exhaust_temp_c; }

private:
  uint32_t _last_update_ms = 0;
  float _track_progress_m = 0.0f;
  float _speed_kmh = 45.0f;
  uint16_t _rpm = 6000;
  uint8_t _gear = 2;
  uint8_t _status_flags = 0x04; // bit 2: Session active
  uint8_t _current_sector = 1;
  uint16_t _lap_number = 1;
  uint32_t _lap_start_ms = 0;
  uint32_t _current_lap_time_ms = 0;
  uint32_t _last_lap_time_ms = 48420;
  uint32_t _best_lap_time_ms = 47950;
  int16_t _pred_delta_ms = -220;

  // IMU Dynamics
  float _lateral_g = 0.0f;
  float _longitudinal_g = 0.0f;
  float _vertical_g = 0.0f;
  float _yaw_rate_dps = 0.0f;

  // Thermal
  float _water_temp_c = 54.0f;
  float _exhaust_temp_c = 480.0f;
  float _head_temp_c = 68.0f;

  // Engine hours
  uint32_t _engine_hours_s = 51480;
  uint32_t _piston_hours_s = 15120;
  uint32_t _engine_accum_ms = 0;

  // Sector triggers
  SectorEventInfo _pending_event;
  bool _s1_triggered = false;
  bool _s2_triggered = false;
  uint32_t _sector_1_time_ms = 0;
  uint32_t _sector_2_time_ms = 0;
};
