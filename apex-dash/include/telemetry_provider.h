#pragma once

#include <Arduino.h>
#include "telemetry_data.h"
#include "onboard_sensors.h"
#include "esp_now_receiver.h"

#define MAX_SAVED_LAPS 30

class TelemetryProvider {
public:
  void begin(const SystemSettings &settings);
  void update(const DeviceSensorsData &local_sensors, const SystemSettings &settings);
  const TelemetrySnapshot &getSnapshot() const { return _snapshot; }

  // Lap history for data recall
  uint16_t getCompletedLapCount() const { return _completed_laps_count; }
  const LapRecord *getLapRecord(uint16_t index) const;
  const LapRecord *getBestLap() const;
  void resetSession();

private:
  TelemetrySnapshot _snapshot;
  EspNowReceiver _receiver;
  LapRecord _lap_history[MAX_SAVED_LAPS];
  uint16_t _completed_laps_count = 0;
  int16_t _best_lap_index = -1;

  // Simulation state variables
  float _sim_track_progress_m = 0.0f;
  uint32_t _lap_start_ms = 0;
  uint32_t _last_sim_update_ms = 0;
  float _sim_throttle = 1.0f;

  void updateSimulation(const SystemSettings &settings);
  void onLapCompleted(uint32_t lap_time_ms, uint32_t s1_ms, uint32_t s2_ms, uint32_t s3_ms,
                      float max_spd, uint16_t max_rpm, uint16_t min_rpm, float max_temp);
};
