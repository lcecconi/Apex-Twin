#include "telemetry_provider.h"
#include "storage_manager.h"
#include <math.h>

#define TRACK_LENGTH_METERS 1050.0f // Simulated Lonato circuit length
#define SECTOR_1_END        340.0f
#define SECTOR_2_END        710.0f

void TelemetryProvider::begin(const SystemSettings &settings, StorageManager *storage) {
  memset(&_snapshot, 0, sizeof(_snapshot));
  memset(_lap_history, 0, sizeof(_lap_history));
  _storage = storage;

  strncpy(_snapshot.current_track_name, settings.selected_track, sizeof(_snapshot.current_track_name) - 1);
  _snapshot.satellites_visible = 15;
  _snapshot.gps_fix = 3; // 3D / DGPS
  _snapshot.hdop = 0.75f;
  _snapshot.gear = 1;
  _snapshot.water_temp_c = 48.0f;
  _snapshot.exhaust_temp_c = 420.0f;
  _snapshot.lap_number = 1;
  _snapshot.current_sector = 1;
  _snapshot.track_module_connected = false; // Wireless unlinked -> running internal simulation
  _snapshot.link_rssi = -64;

  if (_storage) {
    _snapshot.engine_total_hours_sec = _storage->getEngineHours();
  } else {
    _snapshot.engine_total_hours_sec = 14 * 3600 + 18 * 60; // 14h 18m
  }
  _snapshot.piston_hours_sec = 4 * 3600 + 12 * 60;
  _snapshot.session_time_sec = 18 * 60 + 42; // 18m 42s
  _snapshot.session_active = true;
  _session_accum_ms = 0;
  _speed_low_ms = 0;

  _lap_start_ms = millis();
  _last_sim_update_ms = millis();
  _last_engine_time_ms = millis();
  _last_storage_save_ms = millis();
  _engine_accum_ms = 0;

  // Initialize wireless link
  _receiver.begin();

  // Populate initial realistic lap records so Data Recall has immediate data to display
  onLapCompleted(48420, 16120, 16210, 16090, 114.2f, 15100, 5800, 56.4f);
  onLapCompleted(47950, 15980, 16040, 15930, 117.8f, 15450, 5950, 58.1f); // Best lap
  onLapCompleted(48190, 16050, 16110, 16030, 116.1f, 15300, 5890, 58.6f);
  _snapshot.lap_number = 4;
}

void TelemetryProvider::resetEngineHours() {
  _snapshot.engine_total_hours_sec = 0;
  _engine_accum_ms = 0;
  if (_storage) {
    _storage->resetEngineHours();
  }
}

void TelemetryProvider::resetSession() {
  _completed_laps_count = 0;
  _best_lap_index = -1;
  _snapshot.best_lap_time_ms = 0;
  _snapshot.last_lap_time_ms = 0;
  _snapshot.lap_number = 1;
  _snapshot.session_time_sec = 0;
  _snapshot.session_active = false;
  _session_accum_ms = 0;
  _speed_low_ms = 0;
  _lap_start_ms = millis();
}

const LapRecord *TelemetryProvider::getLapRecord(uint16_t index) const {
  if (index < _completed_laps_count) {
    return &_lap_history[index];
  }
  return nullptr;
}

const LapRecord *TelemetryProvider::getBestLap() const {
  if (_best_lap_index >= 0 && _best_lap_index < _completed_laps_count) {
    return &_lap_history[_best_lap_index];
  }
  return nullptr;
}

void TelemetryProvider::onLapCompleted(uint32_t lap_time_ms, uint32_t s1_ms, uint32_t s2_ms, uint32_t s3_ms,
                                      float max_spd, uint16_t max_rpm, uint16_t min_rpm, float max_temp) {
  uint16_t idx = _completed_laps_count % MAX_SAVED_LAPS;
  _lap_history[idx].lap_number = _completed_laps_count + 1;
  _lap_history[idx].lap_time_ms = lap_time_ms;
  _lap_history[idx].split1_ms = s1_ms;
  _lap_history[idx].split2_ms = s2_ms;
  _lap_history[idx].split3_ms = s3_ms;
  _lap_history[idx].max_speed_kmh = max_spd;
  _lap_history[idx].max_rpm = max_rpm;
  _lap_history[idx].min_rpm = min_rpm;
  _lap_history[idx].max_water_temp = max_temp;
  _lap_history[idx].is_best_lap = false;

  if (_snapshot.best_lap_time_ms == 0 || lap_time_ms < _snapshot.best_lap_time_ms) {
    if (_best_lap_index >= 0) {
      _lap_history[_best_lap_index].is_best_lap = false;
    }
    _snapshot.best_lap_time_ms = lap_time_ms;
    _lap_history[idx].is_best_lap = true;
    _best_lap_index = idx;
  }

  _snapshot.last_lap_time_ms = lap_time_ms;
  if (_completed_laps_count < MAX_SAVED_LAPS) {
    _completed_laps_count++;
  }
}

void TelemetryProvider::updateSimulation(const SystemSettings &settings) {
  uint32_t now = millis();
  float dt_sec = (now - _last_sim_update_ms) / 1000.0f;
  if (dt_sec <= 0.0f || dt_sec > 0.5f) dt_sec = 0.05f;
  _last_sim_update_ms = now;

  _snapshot.current_lap_time_ms = now - _lap_start_ms;

  // Track progress
  float speed_mps = _snapshot.speed_kmh / 3.6f;
  _sim_track_progress_m += speed_mps * dt_sec;

  // Determine track sector
  if (_sim_track_progress_m < SECTOR_1_END) {
    _snapshot.current_sector = 1;
  } else if (_sim_track_progress_m < SECTOR_2_END) {
    _snapshot.current_sector = 2;
  } else {
    _snapshot.current_sector = 3;
  }

  // Lap Completion Trigger
  if (_sim_track_progress_m >= TRACK_LENGTH_METERS) {
    _sim_track_progress_m = 0.0f;
    uint32_t lap_time = _snapshot.current_lap_time_ms;
    _lap_start_ms = now;
    _snapshot.current_lap_time_ms = 0;

    // Realistic splits
    uint32_t s1 = lap_time / 3 - 50;
    uint32_t s2 = lap_time / 3 + 20;
    uint32_t s3 = lap_time - (s1 + s2);

    onLapCompleted(lap_time, s1, s2, s3, 117.5f, settings.max_rpm - 300, 5600, _snapshot.water_temp_c);
    _snapshot.lap_number++;
  }

  // Realistic speed profile based on track segment
  float target_speed = 75.0f;
  float pos = _sim_track_progress_m;

  // Segment 1: Main straight (0 - 280m) -> 118 km/h
  if (pos < 280.0f) {
    target_speed = 118.0f;
    _snapshot.longitudinal_g = 0.55f;
    _snapshot.lateral_g = 0.05f;
  }
  // Segment 2: Hairpin 1 (280 - 360m) -> 48 km/h heavy braking and cornering
  else if (pos < 360.0f) {
    target_speed = 48.0f;
    _snapshot.longitudinal_g = -1.4f; // heavy braking
    _snapshot.lateral_g = 1.65f;      // high lateral grip
  }
  // Segment 3: Short chute & chicane (360 - 580m) -> 88 km/h
  else if (pos < 580.0f) {
    target_speed = 88.0f;
    _snapshot.longitudinal_g = 0.35f;
    _snapshot.lateral_g = -1.35f;
  }
  // Segment 4: Fast sweeping bend (580 - 820m) -> 98 km/h
  else if (pos < 820.0f) {
    target_speed = 98.0f;
    _snapshot.longitudinal_g = 0.15f;
    _snapshot.lateral_g = 1.85f;
  }
  // Segment 5: Final hairpin & entry onto straight (820 - 1050m) -> 52 km/h
  else {
    target_speed = 52.0f;
    _snapshot.longitudinal_g = -1.2f;
    _snapshot.lateral_g = -1.55f;
  }

  // Smooth acceleration / deceleration
  if (_snapshot.speed_kmh < target_speed) {
    _snapshot.speed_kmh += 38.0f * dt_sec;
    if (_snapshot.speed_kmh > target_speed) _snapshot.speed_kmh = target_speed;
  } else {
    _snapshot.speed_kmh -= 62.0f * dt_sec;
    if (_snapshot.speed_kmh < target_speed) _snapshot.speed_kmh = target_speed;
  }

  // Gear & RPM calculations based on DriveType
  if (settings.drive_type == DRIVE_SHIFTER_6SPEED) {
    if (_snapshot.speed_kmh < 50.0f) {
      _snapshot.gear = 2;
      _snapshot.rpm = (uint16_t)(6000 + (_snapshot.speed_kmh - 40.0f) * 450);
    } else if (_snapshot.speed_kmh < 68.0f) {
      _snapshot.gear = 3;
      _snapshot.rpm = (uint16_t)(7500 + (_snapshot.speed_kmh - 50.0f) * 380);
    } else if (_snapshot.speed_kmh < 86.0f) {
      _snapshot.gear = 4;
      _snapshot.rpm = (uint16_t)(8500 + (_snapshot.speed_kmh - 68.0f) * 350);
    } else if (_snapshot.speed_kmh < 104.0f) {
      _snapshot.gear = 5;
      _snapshot.rpm = (uint16_t)(9800 + (_snapshot.speed_kmh - 86.0f) * 310);
    } else {
      _snapshot.gear = 6;
      _snapshot.rpm = (uint16_t)(11200 + (_snapshot.speed_kmh - 104.0f) * 300);
    }
  } else {
    // Direct Drive / Clutch
    _snapshot.gear = 1;
    _snapshot.rpm = (uint16_t)(5500 + (_snapshot.speed_kmh / 120.0f) * 9500);
  }
  if (_snapshot.rpm > settings.max_rpm) _snapshot.rpm = settings.max_rpm;

  // Realistic temperatures
  if (_snapshot.water_temp_c < 58.5f) {
    _snapshot.water_temp_c += 0.05f * dt_sec;
  }
  // EGT climbs under high RPM/speed
  float target_egt = 420.0f + (_snapshot.rpm / (float)settings.max_rpm) * 195.0f;
  _snapshot.exhaust_temp_c += (target_egt - _snapshot.exhaust_temp_c) * 0.4f * dt_sec;

  // Predictive delta: updates every 3.0s so animations and values are clearly visible
  if (now - _last_delta_sim_ms >= 3000) {
    _last_delta_sim_ms = now;
    float progress_ratio = _sim_track_progress_m / TRACK_LENGTH_METERS;
    _snapshot.predictive_delta_s = -0.32f + 0.55f * sinf(progress_ratio * 2.0f * 3.14159f);
  }
}

void TelemetryProvider::update(const DeviceSensorsData &local_sensors, const SystemSettings &settings) {
  uint32_t now = millis();
  _snapshot.timestamp_ms = now;

  // Incorporate onboard environmental sensors
  _snapshot.ambient_temp_c = local_sensors.ambient_temp_c;
  _snapshot.ambient_humidity_pct = local_sensors.ambient_humidity_pct;
  _snapshot.battery_voltage = local_sensors.battery_voltage;
  _snapshot.battery_percent = local_sensors.battery_percent;
  // Try receiving real telemetry from Apex-Track on chassis
  bool live_received = false;
  if (!settings.simulation_mode) {
    live_received = _receiver.applyLatestTelemetry(_snapshot);
  }

  // If unlinked or simulation mode forced, run physics simulation
  if (!live_received) {
    _snapshot.track_module_connected = false;
    updateSimulation(settings);
  }

  // Track absolute engine runtime when engine is running (rpm > 0)
  if (_last_engine_time_ms == 0) {
    _last_engine_time_ms = now;
  }
  uint32_t dt_eng_ms = now - _last_engine_time_ms;
  _last_engine_time_ms = now;

  if (_snapshot.rpm > 0) {
    _engine_accum_ms += dt_eng_ms;
    if (_engine_accum_ms >= 1000) {
      uint32_t add_sec = _engine_accum_ms / 1000;
      _snapshot.engine_total_hours_sec += add_sec;
      _engine_accum_ms %= 1000;

      // Periodically persist to flash every 60 seconds of engine run time
      if (_storage && (now - _last_storage_save_ms >= 60000)) {
        _storage->saveEngineHours(_snapshot.engine_total_hours_sec);
        _last_storage_save_ms = now;
      }
    }
  }

  // Track current session time
  // Starts with first start line crossing, ends when speed < 5 km/h for > 1 min
  if (_snapshot.session_active) {
    _session_accum_ms += dt_eng_ms;
    if (_session_accum_ms >= 1000) {
      uint32_t add_sess_sec = _session_accum_ms / 1000;
      _snapshot.session_time_sec += add_sess_sec;
      _session_accum_ms %= 1000;
    }

    if (_snapshot.speed_kmh < 5.0f) {
      _speed_low_ms += dt_eng_ms;
      if (_speed_low_ms >= 60000) { // > 1 min below 5 km/h
        _snapshot.session_active = false;
      }
    } else {
      _speed_low_ms = 0;
    }
  } else if (_snapshot.speed_kmh >= 5.0f && _snapshot.lap_number >= 1 && _snapshot.current_lap_time_ms > 0) {
    _snapshot.session_active = true;
    _speed_low_ms = 0;
  }
}
