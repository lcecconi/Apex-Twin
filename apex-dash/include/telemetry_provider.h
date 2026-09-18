#pragma once

#include <Arduino.h>
#include "telemetry_data.h"
#include "onboard_sensors.h"
#include "esp_now_receiver.h"

#define MAX_SAVED_LAPS 30

class StorageManager;

class TelemetryProvider {
public:
  void begin(const SystemSettings &settings, StorageManager *storage = nullptr);
  void update(const DeviceSensorsData &local_sensors, const SystemSettings &settings);
  const TelemetrySnapshot &getSnapshot() const { return _snapshot; }

  // Lap history for data recall
  uint16_t getCompletedLapCount() const { return _completed_laps_count; }
  const LapRecord *getLapRecord(uint16_t index) const;
  const LapRecord *getBestLap() const;
  void resetSession();
  void resetEngineHours();

private:
  TelemetrySnapshot _snapshot;
  EspNowReceiver _receiver;
  LapRecord _lap_history[MAX_SAVED_LAPS];
  uint16_t _completed_laps_count = 0;
  int16_t _best_lap_index = -1;
  StorageManager *_storage = nullptr;

  // Runtime tracking
  uint32_t _last_engine_time_ms = 0;
  uint32_t _engine_accum_ms = 0;
  uint32_t _last_storage_save_ms = 0;
  uint32_t _session_accum_ms = 0;
  uint32_t _speed_low_ms = 0;

  // Simulation state variables
  float _sim_track_progress_m = 0.0f;
  uint32_t _lap_start_ms = 0;
  uint32_t _last_sim_update_ms = 0;
  uint32_t _last_delta_sim_ms = 0;
  float _sim_throttle = 1.0f;

  void updateSimulation(const SystemSettings &settings);
  void onLapCompleted(uint32_t lap_time_ms, const uint32_t *sector_times, uint8_t sector_count,
                      float max_spd, uint16_t max_rpm, uint16_t min_rpm, float max_temp);
  void onLapCompleted(uint32_t lap_time_ms, uint32_t s1_ms, uint32_t s2_ms, uint32_t s3_ms,
                      float max_spd, uint16_t max_rpm, uint16_t min_rpm, float max_temp);
};
