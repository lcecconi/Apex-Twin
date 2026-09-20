#include <Arduino.h>
#include "config.h"
#include "track_physics_sim.h"
#include "esp_now_broadcaster.h"

static TrackPhysicsSim sim;
static EspNowBroadcaster broadcaster;

static uint32_t last_25hz_ms = 0;
static uint32_t last_2hz_ms = 0;
static uint32_t last_1hz_ms = 0;
static uint32_t last_serial_ms = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=======================================================");
  Serial.println("   APEX-TRACK: Kart Chassis Telemetry & ESP-NOW Link   ");
  Serial.println("   (ESP32-WROOM Hardware Dummy / Telemetry Emulator)   ");
  Serial.println("=======================================================");

  uint32_t now = millis();
  sim.begin(now);

  if (!broadcaster.begin()) {
    Serial.println("[ERROR] Failed to start ESP-NOW Broadcaster!");
  } else {
    Serial.println("[OK] Ready! Broadcasting Virtual CAN-FD telemetry at 25 Hz...");
  }
}

void loop() {
  uint32_t now = millis();

  // 1. 25 Hz Cycle (every 40 ms): Physics update + Fast Dynamics & IMU broadcast
  if (now - last_25hz_ms >= DYNAMICS_DISPATCH_INTERVAL_MS) {
    last_25hz_ms = now;

    sim.update(now);

    CanFastDynamicsPayload fast_payload = {};
    CanImuDynamicsPayload imu_payload = {};
    sim.fillFastDynamics(fast_payload);
    sim.fillImuDynamics(imu_payload);

    broadcaster.sendFastAndImuDynamics(fast_payload, imu_payload);

    // Check for intermediate split or finish line event
    SectorEventInfo event;
    if (sim.checkAndClearSectorEvent(event)) {
      CanSectorEventPayload sector_payload = {};
      sector_payload.gate_type = event.gate_type;
      sector_payload.sector_index = event.sector_index;
      sector_payload.lap_number = event.lap_number;
      sector_payload.split_time_ms = event.split_time_ms;
      sector_payload.lap_time_ms = event.lap_time_ms;
      sector_payload.split_delta_ms = event.split_delta_ms;
      sector_payload.top_speed_raw = event.top_speed_raw;
      sector_payload.max_rpm = event.max_rpm;
      sector_payload.is_new_best = event.is_new_best ? 1 : 0;

      broadcaster.sendSectorEvent(sector_payload);

      Serial.printf("[EVENT] Gate: %s | Sector: %d | Time: %u ms | Delta: %+d ms | NewBest: %s\n",
                    (event.gate_type == 1) ? "FINISH LINE (New Lap)" : "INTERMEDIATE SPLIT",
                    event.sector_index, (event.gate_type == 1) ? event.lap_time_ms : event.split_time_ms,
                    event.split_delta_ms, event.is_new_best ? "YES!" : "NO");
    }
  }

  // 2. 2 Hz Cycle (every 500 ms): Engine thermal broadcast
  if (now - last_2hz_ms >= THERMAL_DISPATCH_INTERVAL_MS) {
    last_2hz_ms = now;

    CanEngineThermalPayload thermal_payload = {};
    sim.fillEngineThermal(thermal_payload);
    broadcaster.sendEngineThermal(thermal_payload);
  }

  // 3. 1 Hz Cycle (every 1000 ms): GNSS status & Chassis health broadcast
  if (now - last_1hz_ms >= GNSS_DISPATCH_INTERVAL_MS) {
    last_1hz_ms = now;

    CanGnssStatusPayload gnss_payload = {};
    CanChassisHealthPayload health_payload = {};
    sim.fillGnssStatus(gnss_payload);
    sim.fillChassisHealth(health_payload);

    broadcaster.sendGnssStatus(gnss_payload);
    broadcaster.sendChassisHealth(health_payload);
  }

  // 4. 1 Hz Serial Diagnostics Console Output
  if (now - last_serial_ms >= SERIAL_LOG_INTERVAL_MS) {
    last_serial_ms = now;

    Serial.printf("[TRACK-TX] Lap: L%02d [S%d] | Spd: %03.0f km/h | RPM: %05d (G%d) | Delta: %+0.2fs | H2O: %.1fC | EGT: %.0fC | TX: %u (Fail: %u)%s\n",
                  sim.getLapNumber(), sim.getCurrentSector(), sim.getSpeedKmh(), sim.getRpm(), sim.getGear(),
                  sim.getPredictiveDelta(), sim.getWaterTemp(), sim.getExhaustTemp(),
                  broadcaster.getPacketsSent(), broadcaster.getPacketsFailed(),
                  broadcaster.isDashConnected() ? " | [DASH LINKED]" : "");
  }

  delay(2);
}
