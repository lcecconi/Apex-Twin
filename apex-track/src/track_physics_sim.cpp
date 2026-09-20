#include "track_physics_sim.h"
#include "config.h"
#include <cmath>

TrackPhysicsSim::TrackPhysicsSim() {}

void TrackPhysicsSim::begin(uint32_t now_ms) {
  _last_update_ms = now_ms;
  _lap_start_ms = now_ms;
  _track_progress_m = 0.0f;
  _speed_kmh = 45.0f;
  _rpm = 6000;
  _gear = 2;
  _status_flags = 0x04; // Session active
  _current_sector = 1;
  _lap_number = 1;
  _current_lap_time_ms = 0;
  _last_lap_time_ms = 48420;
  _best_lap_time_ms = 47950;
  _pred_delta_ms = -220;
  _s1_triggered = false;
  _s2_triggered = false;
  _pending_event.triggered = false;
}

void TrackPhysicsSim::update(uint32_t now_ms) {
  float dt_sec = (now_ms - _last_update_ms) / 1000.0f;
  if (dt_sec <= 0.0f || dt_sec > 0.5f) dt_sec = 0.04f;
  _last_update_ms = now_ms;

  // Track progress
  float speed_mps = _speed_kmh / 3.6f;
  _track_progress_m += speed_mps * dt_sec;
  _current_lap_time_ms = now_ms - _lap_start_ms;

  // Determine active sector
  if (_track_progress_m < SIM_SECTOR_1_END_M) {
    _current_sector = 1;
  } else if (_track_progress_m < SIM_SECTOR_2_END_M) {
    _current_sector = 2;
  } else {
    _current_sector = 3;
  }

  // Sector 1 Gate Crossing
  if (_track_progress_m >= SIM_SECTOR_1_END_M && !_s1_triggered) {
    _s1_triggered = true;
    _sector_1_time_ms = _current_lap_time_ms;
    _pending_event.triggered = true;
    _pending_event.gate_type = 2; // Split
    _pending_event.sector_index = 1;
    _pending_event.lap_number = _lap_number;
    _pending_event.split_time_ms = _sector_1_time_ms;
    _pending_event.split_delta_ms = (int16_t)(_sector_1_time_ms - 16120);
    _pending_event.top_speed_raw = (uint16_t)(118.0f * 20.0f);
    _pending_event.max_rpm = 15200;
    _pending_event.is_new_best = false;
  }

  // Sector 2 Gate Crossing
  if (_track_progress_m >= SIM_SECTOR_2_END_M && !_s2_triggered) {
    _s2_triggered = true;
    _sector_2_time_ms = _current_lap_time_ms - _sector_1_time_ms;
    _pending_event.triggered = true;
    _pending_event.gate_type = 2; // Split
    _pending_event.sector_index = 2;
    _pending_event.lap_number = _lap_number;
    _pending_event.split_time_ms = _sector_2_time_ms;
    _pending_event.split_delta_ms = (int16_t)(_sector_2_time_ms - 16210);
    _pending_event.top_speed_raw = (uint16_t)(98.0f * 20.0f);
    _pending_event.max_rpm = 14800;
    _pending_event.is_new_best = false;
  }

  // Lap Completion Trigger (Start/Finish Line)
  if (_track_progress_m >= SIM_TRACK_LENGTH_M) {
    _track_progress_m = 0.0f;
    _s1_triggered = false;
    _s2_triggered = false;
    _current_sector = 1;

    uint32_t lap_time = _current_lap_time_ms;
    _last_lap_time_ms = lap_time;
    bool is_best = (_best_lap_time_ms == 0 || lap_time < _best_lap_time_ms);
    if (is_best) _best_lap_time_ms = lap_time;

    _pending_event.triggered = true;
    _pending_event.gate_type = 1; // Start/Finish Line
    _pending_event.sector_index = 3;
    _pending_event.lap_number = _lap_number;
    _pending_event.lap_time_ms = lap_time;
    _pending_event.split_time_ms = lap_time - (_sector_1_time_ms + _sector_2_time_ms);
    _pending_event.split_delta_ms = (int16_t)(lap_time - 48000);
    _pending_event.top_speed_raw = (uint16_t)(118.5f * 20.0f);
    _pending_event.max_rpm = 15450;
    _pending_event.is_new_best = is_best;

    _lap_number++;
    _lap_start_ms = now_ms;
    _current_lap_time_ms = 0;
  }

  // Realistic track speed & cornering G-profile based on Lonato circuit
  float target_speed = 75.0f;
  float pos = _track_progress_m;

  if (pos < 280.0f) {
    // Main Straight
    target_speed = 118.0f;
    _longitudinal_g = 0.55f;
    _lateral_g = 0.05f;
    _yaw_rate_dps = 1.0f;
  } else if (pos < 360.0f) {
    // Turn 1-2 Heavy Braking & Chicane Entry
    target_speed = 48.0f;
    _longitudinal_g = -1.45f;
    _lateral_g = 1.65f;
    _yaw_rate_dps = 48.0f;
  } else if (pos < 580.0f) {
    // Infield Esses & Hairpin
    target_speed = 88.0f;
    _longitudinal_g = 0.35f;
    _lateral_g = -1.35f;
    _yaw_rate_dps = -35.0f;
  } else if (pos < 820.0f) {
    // Fast Sweeper
    target_speed = 98.0f;
    _longitudinal_g = 0.20f;
    _lateral_g = 1.85f;
    _yaw_rate_dps = 28.0f;
  } else {
    // Final Corner & Straight Approach
    target_speed = 52.0f;
    _longitudinal_g = -1.25f;
    _lateral_g = -1.55f;
    _yaw_rate_dps = -42.0f;
  }

  // Smooth acceleration / braking dynamics
  if (_speed_kmh < target_speed) {
    _speed_kmh += 38.0f * dt_sec;
    if (_speed_kmh > target_speed) _speed_kmh = target_speed;
  } else {
    _speed_kmh -= 62.0f * dt_sec;
    if (_speed_kmh < target_speed) _speed_kmh = target_speed;
  }

  // Shifter (KZ 6-Speed) engine simulation
  if (_speed_kmh < 50.0f) {
    _gear = 2;
    _rpm = (uint16_t)(6000 + (_speed_kmh - 40.0f) * 450);
  } else if (_speed_kmh < 68.0f) {
    _gear = 3;
    _rpm = (uint16_t)(7500 + (_speed_kmh - 50.0f) * 380);
  } else if (_speed_kmh < 86.0f) {
    _gear = 4;
    _rpm = (uint16_t)(8500 + (_speed_kmh - 68.0f) * 350);
  } else if (_speed_kmh < 104.0f) {
    _gear = 5;
    _rpm = (uint16_t)(9800 + (_speed_kmh - 86.0f) * 310);
  } else {
    _gear = 6;
    _rpm = (uint16_t)(11200 + (_speed_kmh - 104.0f) * 300);
  }

  if (_rpm > 15800) _rpm = 15800;
  if (_rpm < 5200) _rpm = 5200;

  // Status flags (Shift point & Over-rev)
  _status_flags = 0x04; // Session running
  if (_rpm >= SIM_SHIFT_RPM) {
    _status_flags |= 0x01; // Shift light active
  }
  if (_rpm >= SIM_OVER_REV_RPM) {
    _status_flags |= 0x02; // Over-rev warning
  }

  // Predictive Delta calculation against personal best
  float expected_time_ms = (_track_progress_m / SIM_TRACK_LENGTH_M) * (float)_best_lap_time_ms;
  _pred_delta_ms = (int16_t)((float)_current_lap_time_ms - expected_time_ms);

  // Thermals
  _water_temp_c = 55.0f + ((float)_rpm / 16000.0f) * 4.5f;
  _exhaust_temp_c = 440.0f + ((float)_rpm / 16000.0f) * 190.0f;
  _head_temp_c = 62.0f + (_water_temp_c * 0.2f);

  // Engine hour counter accumulation
  _engine_accum_ms += (uint32_t)(dt_sec * 1000.0f);
  if (_engine_accum_ms >= 1000) {
    uint32_t sec = _engine_accum_ms / 1000;
    _engine_hours_s += sec;
    _piston_hours_s += sec;
    _engine_accum_ms %= 1000;
  }
}

void TrackPhysicsSim::fillFastDynamics(CanFastDynamicsPayload &p) const {
  p.rpm = _rpm;
  p.speed_raw = (uint16_t)roundf(_speed_kmh * 20.0f);
  p.gear = _gear;
  p.status_flags = _status_flags;
  p.current_sector = _current_sector;
  p.total_sectors = SIM_TOTAL_SECTORS;
  p.lap_number = _lap_number;
  p.lap_time_ms = _current_lap_time_ms;
  p.pred_delta_ms = _pred_delta_ms;
}

void TrackPhysicsSim::fillImuDynamics(CanImuDynamicsPayload &p) const {
  p.lateral_g_raw = (int16_t)roundf(_lateral_g * 100.0f);
  p.longitudinal_g_raw = (int16_t)roundf(_longitudinal_g * 100.0f);
  p.vertical_g_raw = (int16_t)roundf(_vertical_g * 100.0f);
  p.yaw_rate_raw = (int16_t)roundf(_yaw_rate_dps * 10.0f);
  p.pitch_deg_raw = (int16_t)roundf(_longitudinal_g * 15.0f);
  p.roll_deg_raw = (int16_t)roundf(_lateral_g * 20.0f);
  p.reserved = 0;
}

void TrackPhysicsSim::fillEngineThermal(CanEngineThermalPayload &p) const {
  p.water_temp_raw = (int16_t)roundf(_water_temp_c * 10.0f);
  p.exhaust_temp_c = (uint16_t)roundf(_exhaust_temp_c);
  p.head_temp_raw = (int16_t)roundf(_head_temp_c * 10.0f);
  p.intake_temp_raw = (int16_t)240; // 24.0 C
  p.reserved = 0;
}

void TrackPhysicsSim::fillGnssStatus(CanGnssStatusPayload &p) const {
  // Lonato coordinates with track offset
  double lat = 45.388712 + ((_track_progress_m / SIM_TRACK_LENGTH_M) * 0.0012);
  double lon = 10.479521 + ((_track_progress_m / SIM_TRACK_LENGTH_M) * 0.0018);

  p.latitude_scaled = (int32_t)round(lat * 1e7);
  p.longitude_scaled = (int32_t)round(lon * 1e7);
  p.altitude_m_raw = (int16_t)1240; // 124.0 m
  p.heading_raw = (uint16_t)roundf((_track_progress_m / SIM_TRACK_LENGTH_M) * 36000.0f);
  p.satellites = 16;
  p.fix_type = 5; // RTK-Fixed
  p.hdop_raw = 75; // 0.75
  memset(p.reserved, 0, sizeof(p.reserved));
}

void TrackPhysicsSim::fillChassisHealth(CanChassisHealthPayload &p) const {
  p.battery_mv = 12650; // 12.65V chassis battery
  p.error_flags = 0;    // System OK
  p.engine_hours_s = _engine_hours_s;
  p.piston_hours_s = _piston_hours_s;
  p.log_file_index = 42;
}

bool TrackPhysicsSim::checkAndClearSectorEvent(SectorEventInfo &event) {
  if (_pending_event.triggered) {
    event = _pending_event;
    _pending_event.triggered = false;
    return true;
  }
  return false;
}
