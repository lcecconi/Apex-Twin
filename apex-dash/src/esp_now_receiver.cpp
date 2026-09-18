#include "esp_now_receiver.h"

ChassisTelemetry EspNowReceiver::_chassis;
uint32_t EspNowReceiver::_last_packet_ms = 0;
bool EspNowReceiver::_new_packet_ready = false;
int8_t EspNowReceiver::_last_rssi = -65;
uint8_t EspNowReceiver::_peer_mac[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
bool EspNowReceiver::_peer_registered = false;
uint16_t EspNowReceiver::_tx_seq = 0;
CanFrameReceiveCallback EspNowReceiver::_rx_callback = nullptr;
void *EspNowReceiver::_rx_callback_arg = nullptr;

EspNowReceiver::EspNowReceiver() {
}

bool EspNowReceiver::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    log_e("ESP-NOW Init Failed");
    return false;
  }

  esp_now_register_recv_cb(EspNowReceiver::onDataRecv);

  // Register broadcast peer
  esp_now_peer_info_t peerInfo = {};
  memset(peerInfo.peer_addr, 0xFF, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  log_i("Virtual CAN-FD ESP-NOW Receiver initialized on channel %d", WiFi.channel());
  return true;
}

bool EspNowReceiver::isConnected() const {
  return (_last_packet_ms > 0 && (millis() - _last_packet_ms < 2500));
}

int8_t EspNowReceiver::getRssi() const {
  return _last_rssi;
}

void EspNowReceiver::setReceiveCallback(CanFrameReceiveCallback cb, void *user_arg) {
  _rx_callback = cb;
  _rx_callback_arg = user_arg;
}

bool EspNowReceiver::sendFrame(const CanFdFrame &frame) {
  ApexCanFdPacket pkt = {};
  pkt.magic = APEX_CAN_FD_MAGIC;
  pkt.seq = ++_tx_seq;
  pkt.frame_count = 1;
  memcpy(&pkt.frames[0], &frame, sizeof(CanFdFrame));

  const uint8_t *target_mac = _peer_registered ? _peer_mac : (const uint8_t*)"\xFF\xFF\xFF\xFF\xFF\xFF";
  size_t send_len = sizeof(uint32_t) + sizeof(uint16_t) + 2 + sizeof(CanFdFrame);
  return (esp_now_send(target_mac, (const uint8_t*)&pkt, send_len) == ESP_OK);
}

bool EspNowReceiver::sendDashCommand(uint8_t cmd_id, uint8_t sub_param, uint32_t param, const char *track_id) {
  CanFdFrame f = {};
  f.id = CAN_ID_DASH_COMMAND;
  f.len = sizeof(CanDashCommandPayload);
  CanDashCommandPayload *p = (CanDashCommandPayload*)f.data;
  p->cmd_id = cmd_id;
  p->sub_param = sub_param;
  p->param_u32 = param;
  if (track_id) {
    strncpy(p->track_id, track_id, sizeof(p->track_id) - 1);
  }
  return sendFrame(f);
}

bool EspNowReceiver::sendDashStatus(uint16_t battery_mv, uint8_t battery_pct, float amb_temp_c, uint8_t amb_hum_pct, uint32_t epoch_s) {
  CanFdFrame f = {};
  f.id = CAN_ID_DASH_STATUS;
  f.len = sizeof(CanDashStatusPayload);
  CanDashStatusPayload *p = (CanDashStatusPayload*)f.data;
  p->dash_battery_mv = battery_mv;
  p->dash_battery_pct = battery_pct;
  p->ambient_temp_raw = (int16_t)roundf(amb_temp_c * 10.0f);
  p->ambient_hum_pct = amb_hum_pct;
  p->rtc_epoch_s = epoch_s;
  return sendFrame(f);
}

void EspNowReceiver::processIncomingCanFrame(const CanFdFrame &frame) {
  switch (frame.id) {
    case CAN_ID_FAST_DYNAMICS: {
      if (frame.len >= sizeof(CanFastDynamicsPayload)) {
        const CanFastDynamicsPayload *p = (const CanFastDynamicsPayload*)frame.data;
        _chassis.rpm = p->rpm;
        _chassis.speed_kmh = p->speed_raw * 0.05f;
        _chassis.gear = p->gear;
        _chassis.status_flags = p->status_flags;
        _chassis.current_sector = p->current_sector ? p->current_sector : 1;
        _chassis.total_sectors = p->total_sectors ? p->total_sectors : 3;
        _chassis.lap_number = p->lap_number;
        _chassis.current_lap_time_ms = p->lap_time_ms;
        _chassis.predictive_delta_s = p->pred_delta_ms / 1000.0f;
      }
      break;
    }

    case CAN_ID_IMU_DYNAMICS: {
      if (frame.len >= sizeof(CanImuDynamicsPayload)) {
        const CanImuDynamicsPayload *p = (const CanImuDynamicsPayload*)frame.data;
        _chassis.lateral_g = p->lateral_g_raw * 0.01f;
        _chassis.longitudinal_g = p->longitudinal_g_raw * 0.01f;
        _chassis.vertical_g = p->vertical_g_raw * 0.01f;
        _chassis.yaw_rate_dps = p->yaw_rate_raw * 0.1f;
      }
      break;
    }

    case CAN_ID_SECTOR_EVENT: {
      if (frame.len >= sizeof(CanSectorEventPayload)) {
        const CanSectorEventPayload *p = (const CanSectorEventPayload*)frame.data;
        _chassis.last_split_delta_ms = p->split_delta_ms;
        if (p->gate_type == 1) { // S/F Crossing (New lap)
          _chassis.last_lap_time_ms = p->lap_time_ms;
          if (p->is_new_best || _chassis.best_lap_time_ms == 0 || p->lap_time_ms < _chassis.best_lap_time_ms) {
            _chassis.best_lap_time_ms = p->lap_time_ms;
          }
        }
      }
      break;
    }

    case CAN_ID_ENGINE_THERMAL: {
      if (frame.len >= sizeof(CanEngineThermalPayload)) {
        const CanEngineThermalPayload *p = (const CanEngineThermalPayload*)frame.data;
        _chassis.water_temp_c = p->water_temp_raw * 0.1f;
        _chassis.exhaust_temp_c = (float)p->exhaust_temp_c;
        _chassis.head_temp_c = p->head_temp_raw * 0.1f;
      }
      break;
    }

    case CAN_ID_GNSS_STATUS: {
      if (frame.len >= sizeof(CanGnssStatusPayload)) {
        const CanGnssStatusPayload *p = (const CanGnssStatusPayload*)frame.data;
        _chassis.latitude = p->latitude_scaled / 1e7;
        _chassis.longitude = p->longitude_scaled / 1e7;
        _chassis.altitude_m = p->altitude_m_raw * 0.1f;
        _chassis.heading_deg = p->heading_raw * 0.01f;
        _chassis.satellites_visible = p->satellites;
        _chassis.gps_fix = p->fix_type;
        _chassis.hdop = p->hdop_raw * 0.01f;
      }
      break;
    }

    case CAN_ID_CHASSIS_HEALTH: {
      if (frame.len >= sizeof(CanChassisHealthPayload)) {
        const CanChassisHealthPayload *p = (const CanChassisHealthPayload*)frame.data;
        _chassis.chassis_battery_voltage = p->battery_mv / 1000.0f;
        _chassis.track_error_code = p->error_flags;
        _chassis.engine_total_hours_sec = p->engine_hours_s;
        _chassis.piston_hours_sec = p->piston_hours_s;
      }
      break;
    }

    default:
      break;
  }

  // Trigger optional user callback
  if (_rx_callback) {
    _rx_callback(frame, _rx_callback_arg);
  }
}

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
void EspNowReceiver::onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  const uint8_t *mac = info->src_addr;
#else
void EspNowReceiver::onDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
#endif
  if (len < 4) return;

  uint32_t magic = *(const uint32_t*)data;

  // Track sender MAC address for direct unicast responses
  if (!_peer_registered || memcmp(_peer_mac, mac, 6) != 0) {
    memcpy(_peer_mac, mac, 6);
    _peer_registered = true;
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, mac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    if (esp_now_is_peer_exist(mac)) {
      esp_now_del_peer(mac);
    }
    esp_now_add_peer(&peerInfo);
  }

  // Case 1: Bundled Virtual CAN-FD packet
  if (magic == APEX_CAN_FD_MAGIC) {
    const ApexCanFdPacket *pkt = (const ApexCanFdPacket*)data;
    uint8_t count = pkt->frame_count;
    if (count > 3) count = 3;

    for (uint8_t i = 0; i < count; i++) {
      processIncomingCanFrame(pkt->frames[i]);
    }

    _last_packet_ms = millis();
    _new_packet_ready = true;
    _chassis.connected = true;
    _chassis.last_packet_ms = _last_packet_ms;
    return;
  }

  // Case 2: Single raw CanFdFrame
  if (len == sizeof(CanFdFrame)) {
    const CanFdFrame *frame = (const CanFdFrame*)data;
    processIncomingCanFrame(*frame);
    _last_packet_ms = millis();
    _new_packet_ready = true;
    _chassis.connected = true;
    _chassis.last_packet_ms = _last_packet_ms;
    return;
  }
}

bool EspNowReceiver::applyLatestTelemetry(TelemetrySnapshot &target) {
  if (!isConnected()) {
    _chassis.connected = false;
    target.track_module_connected = false;
    target.chassis.connected = false;
    return false;
  }

  _chassis.connected = true;
  _chassis.link_rssi = _last_rssi;
  target.chassis = _chassis;
  target.syncFlatFields();
  return true;
}
