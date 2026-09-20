#include "esp_now_broadcaster.h"
#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>

uint8_t EspNowBroadcaster::_broadcast_mac[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
uint16_t EspNowBroadcaster::_seq = 0;
uint32_t EspNowBroadcaster::_packets_sent = 0;
uint32_t EspNowBroadcaster::_packets_failed = 0;

uint32_t EspNowBroadcaster::_last_dash_packet_ms = 0;
uint16_t EspNowBroadcaster::_dash_battery_mv = 0;
uint8_t  EspNowBroadcaster::_dash_battery_pct = 0;
float    EspNowBroadcaster::_dash_ambient_temp_c = 0.0f;
uint8_t  EspNowBroadcaster::_dash_ambient_hum_pct = 0;
uint32_t EspNowBroadcaster::_dash_rtc_epoch_s = 0;

EspNowBroadcaster::EspNowBroadcaster() {}

bool EspNowBroadcaster::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Set WiFi channel to configured channel
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(ESPNOW_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESP-NOW] Initialization failed!");
    return false;
  }

  esp_now_register_send_cb(EspNowBroadcaster::onDataSent);
  esp_now_register_recv_cb(EspNowBroadcaster::onDataRecv);

  // Register broadcast peer
  esp_now_peer_info_t peer_info = {};
  memcpy(peer_info.peer_addr, _broadcast_mac, 6);
  peer_info.channel = ESPNOW_WIFI_CHANNEL;
  peer_info.encrypt = false;

  if (esp_now_add_peer(&peer_info) != ESP_OK) {
    Serial.println("[ESP-NOW] Failed to add broadcast peer!");
    return false;
  }

  Serial.println("[ESP-NOW] Virtual CAN-FD Broadcaster initialized on Channel 1.");
  return true;
}

void EspNowBroadcaster::onDataSent(const uint8_t *mac, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    _packets_sent++;
  } else {
    _packets_failed++;
  }
}

void EspNowBroadcaster::onDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
  if (len < 4) return;
  uint32_t magic = *(const uint32_t*)data;

  if (magic == APEX_CAN_FD_MAGIC) {
    const ApexCanFdPacket *pkt = (const ApexCanFdPacket*)data;
    for (uint8_t i = 0; i < pkt->frame_count && i < 3; i++) {
      const CanFdFrame &f = pkt->frames[i];
      if (f.id == CAN_ID_DASH_STATUS && f.len >= sizeof(CanDashStatusPayload)) {
        const CanDashStatusPayload *status = (const CanDashStatusPayload*)f.data;
        _dash_battery_mv = status->dash_battery_mv;
        _dash_battery_pct = status->dash_battery_pct;
        _dash_ambient_temp_c = status->ambient_temp_raw * 0.1f;
        _dash_ambient_hum_pct = status->ambient_hum_pct;
        _dash_rtc_epoch_s = status->rtc_epoch_s;
        _last_dash_packet_ms = millis();
        Serial.printf("[DASH-UPLINK] Battery: %u mV (%u%%) | Temp: %.1f C | Hum: %u%%\n",
                      _dash_battery_mv, _dash_battery_pct, _dash_ambient_temp_c, _dash_ambient_hum_pct);
      } else if (f.id == CAN_ID_DASH_COMMAND && f.len >= sizeof(CanDashCommandPayload)) {
        const CanDashCommandPayload *cmd = (const CanDashCommandPayload*)f.data;
        Serial.printf("[DASH-COMMAND] Cmd: 0x%02X | Param: %u | Track: %s\n",
                      cmd->cmd_id, cmd->param_u32, cmd->track_id);
      }
    }
  }
}

bool EspNowBroadcaster::isDashConnected() const {
  return (_last_dash_packet_ms > 0 && (millis() - _last_dash_packet_ms < 3000));
}

bool EspNowBroadcaster::sendPacket(const ApexCanFdPacket &pkt, uint8_t frame_count) {
  size_t send_len = sizeof(uint32_t) + sizeof(uint16_t) + 2 + (frame_count * sizeof(CanFdFrame));
  esp_err_t err = esp_now_send(_broadcast_mac, (const uint8_t*)&pkt, send_len);
  return (err == ESP_OK);
}

bool EspNowBroadcaster::sendFastAndImuDynamics(const CanFastDynamicsPayload &fast, const CanImuDynamicsPayload &imu) {
  ApexCanFdPacket pkt = {};
  pkt.magic = APEX_CAN_FD_MAGIC;
  pkt.seq = ++_seq;
  pkt.frame_count = 2;

  pkt.frames[0].id = CAN_ID_FAST_DYNAMICS;
  pkt.frames[0].len = sizeof(CanFastDynamicsPayload);
  memcpy(pkt.frames[0].data, &fast, sizeof(CanFastDynamicsPayload));

  pkt.frames[1].id = CAN_ID_IMU_DYNAMICS;
  pkt.frames[1].len = sizeof(CanImuDynamicsPayload);
  memcpy(pkt.frames[1].data, &imu, sizeof(CanImuDynamicsPayload));

  return sendPacket(pkt, 2);
}

bool EspNowBroadcaster::sendSectorEvent(const CanSectorEventPayload &sector) {
  ApexCanFdPacket pkt = {};
  pkt.magic = APEX_CAN_FD_MAGIC;
  pkt.seq = ++_seq;
  pkt.frame_count = 1;

  pkt.frames[0].id = CAN_ID_SECTOR_EVENT;
  pkt.frames[0].len = sizeof(CanSectorEventPayload);
  memcpy(pkt.frames[0].data, &sector, sizeof(CanSectorEventPayload));

  return sendPacket(pkt, 1);
}

bool EspNowBroadcaster::sendEngineThermal(const CanEngineThermalPayload &thermal) {
  ApexCanFdPacket pkt = {};
  pkt.magic = APEX_CAN_FD_MAGIC;
  pkt.seq = ++_seq;
  pkt.frame_count = 1;

  pkt.frames[0].id = CAN_ID_ENGINE_THERMAL;
  pkt.frames[0].len = sizeof(CanEngineThermalPayload);
  memcpy(pkt.frames[0].data, &thermal, sizeof(CanEngineThermalPayload));

  return sendPacket(pkt, 1);
}

bool EspNowBroadcaster::sendGnssStatus(const CanGnssStatusPayload &gnss) {
  ApexCanFdPacket pkt = {};
  pkt.magic = APEX_CAN_FD_MAGIC;
  pkt.seq = ++_seq;
  pkt.frame_count = 1;

  pkt.frames[0].id = CAN_ID_GNSS_STATUS;
  pkt.frames[0].len = sizeof(CanGnssStatusPayload);
  memcpy(pkt.frames[0].data, &gnss, sizeof(CanGnssStatusPayload));

  return sendPacket(pkt, 1);
}

bool EspNowBroadcaster::sendChassisHealth(const CanChassisHealthPayload &health) {
  ApexCanFdPacket pkt = {};
  pkt.magic = APEX_CAN_FD_MAGIC;
  pkt.seq = ++_seq;
  pkt.frame_count = 1;

  pkt.frames[0].id = CAN_ID_CHASSIS_HEALTH;
  pkt.frames[0].len = sizeof(CanChassisHealthPayload);
  memcpy(pkt.frames[0].data, &health, sizeof(CanChassisHealthPayload));

  return sendPacket(pkt, 1);
}
