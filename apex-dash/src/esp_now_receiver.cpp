#include "esp_now_receiver.h"

ApexTrackTelemetryPacket EspNowReceiver::_latest_packet;
uint32_t EspNowReceiver::_last_packet_ms = 0;
bool EspNowReceiver::_new_packet_ready = false;
int8_t EspNowReceiver::_last_rssi = -65;

void EspNowReceiver::onDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
  if (len != sizeof(ApexTrackTelemetryPacket)) {
    return;
  }

  const ApexTrackTelemetryPacket *pkt = (const ApexTrackTelemetryPacket *)data;
  if (pkt->magic != APEX_PACKET_MAGIC) {
    return;
  }

  memcpy(&_latest_packet, pkt, sizeof(ApexTrackTelemetryPacket));
  _last_packet_ms = millis();
  _new_packet_ready = true;
}

void EspNowReceiver::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() == ESP_OK) {
    esp_now_register_recv_cb(EspNowReceiver::onDataRecv);
    log_i("ESP-NOW Telemetry Receiver initialized on channel %d", WiFi.channel());
  } else {
    log_e("ESP-NOW Init Failed");
  }
}

bool EspNowReceiver::isConnected() const {
  return (_last_packet_ms > 0 && (millis() - _last_packet_ms < 2500));
}

int8_t EspNowReceiver::getRssi() const {
  return _last_rssi;
}

bool EspNowReceiver::applyLatestTelemetry(TelemetrySnapshot &target) {
  if (!isConnected()) {
    target.track_module_connected = false;
    return false;
  }

  target.track_module_connected = true;
  target.link_rssi = _last_rssi;

  target.rpm = _latest_packet.rpm;
  target.speed_kmh = _latest_packet.speed_kmh;
  target.gear = _latest_packet.gear;
  target.water_temp_c = _latest_packet.water_temp_c;
  target.exhaust_temp_c = _latest_packet.exhaust_temp_c;
  target.lateral_g = _latest_packet.lateral_g;
  target.longitudinal_g = _latest_packet.longitudinal_g;
  target.satellites_visible = _latest_packet.satellites;
  target.gps_fix = _latest_packet.fix_type;
  target.hdop = _latest_packet.hdop;
  target.lap_number = _latest_packet.lap_number;
  target.current_lap_time_ms = _latest_packet.current_lap_time_ms;
  target.last_lap_time_ms = _latest_packet.last_lap_time_ms;
  target.best_lap_time_ms = _latest_packet.best_lap_time_ms;
  target.predictive_delta_s = _latest_packet.predictive_delta_s;
  target.current_sector = _latest_packet.sector;
  target.track_error_code = _latest_packet.error_code;

  return true;
}
