/**
 * @file esp_now_transport.cpp
 * Native ESP-IDF ESP-NOW Virtual CAN-FD Transport implementation
 */

#include "esp_now_transport.h"

#ifdef ESP_PLATFORM
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"

static const char *TAG = "ESPNOW_TX";
static ChassisTelemetry *s_active_chassis = nullptr;
static uint32_t s_last_packet_time_ms = 0;

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 1, 0)
static void on_esp_now_recv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    (void)recv_info;
#else
static void on_esp_now_recv(const uint8_t *mac_addr, const uint8_t *data, int len) {
    (void)mac_addr;
#endif
    if (!s_active_chassis || !data || len < (int)sizeof(CanFdFrame)) return;

    const CanFdFrame *frame = reinterpret_cast<const CanFdFrame *>(data);
    s_last_packet_time_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);
    s_active_chassis->connected = true;
    s_active_chassis->last_packet_ms = s_last_packet_time_ms;

    switch (frame->id) {
        case CAN_ID_FAST_DYNAMICS: {
            if (frame->len >= sizeof(CanFastDynamicsPayload)) {
                const auto *p = reinterpret_cast<const CanFastDynamicsPayload *>(frame->data);
                s_active_chassis->rpm = p->rpm;
                s_active_chassis->speed_kmh = p->speed_raw * 0.05f;
                s_active_chassis->gear = p->gear;
                s_active_chassis->status_flags = p->status_flags;
                s_active_chassis->current_sector = p->current_sector;
                s_active_chassis->total_sectors = p->total_sectors;
                s_active_chassis->lap_number = p->lap_number;
                s_active_chassis->current_lap_time_ms = p->lap_time_ms;
                s_active_chassis->predictive_delta_s = p->pred_delta_ms / 1000.0f;
            }
            break;
        }

        case CAN_ID_IMU_DYNAMICS: {
            if (frame->len >= sizeof(CanImuDynamicsPayload)) {
                const auto *p = reinterpret_cast<const CanImuDynamicsPayload *>(frame->data);
                s_active_chassis->lateral_g = p->lateral_g_raw * 0.01f;
                s_active_chassis->longitudinal_g = p->longitudinal_g_raw * 0.01f;
                s_active_chassis->vertical_g = p->vertical_g_raw * 0.01f;
                s_active_chassis->yaw_rate_dps = p->yaw_rate_raw * 0.1f;
            }
            break;
        }

        case CAN_ID_ENGINE_THERMAL: {
            if (frame->len >= sizeof(CanEngineThermalPayload)) {
                const auto *p = reinterpret_cast<const CanEngineThermalPayload *>(frame->data);
                s_active_chassis->water_temp_c = p->water_temp_raw * 0.1f;
                s_active_chassis->exhaust_temp_c = (float)p->exhaust_temp_c;
                s_active_chassis->head_temp_c = p->head_temp_raw * 0.1f;
            }
            break;
        }

        case CAN_ID_GNSS_STATUS: {
            if (frame->len >= sizeof(CanGnssStatusPayload)) {
                const auto *p = reinterpret_cast<const CanGnssStatusPayload *>(frame->data);
                s_active_chassis->latitude = p->latitude_scaled * 1e-7;
                s_active_chassis->longitude = p->longitude_scaled * 1e-7;
                s_active_chassis->altitude_m = p->altitude_m_raw * 0.1f;
                s_active_chassis->satellites_visible = p->satellites;
                s_active_chassis->gps_fix = p->fix_type;
                s_active_chassis->hdop = p->hdop_raw * 0.01f;
            }
            break;
        }

        case CAN_ID_CHASSIS_HEALTH: {
            if (frame->len >= sizeof(CanChassisHealthPayload)) {
                const auto *p = reinterpret_cast<const CanChassisHealthPayload *>(frame->data);
                s_active_chassis->chassis_battery_voltage = p->battery_mv * 0.001f;
                s_active_chassis->engine_total_hours_sec = p->engine_hours_s;
                s_active_chassis->piston_hours_sec = p->piston_hours_s;
                s_active_chassis->track_error_code = p->error_flags;
            }
            break;
        }

        default:
            break;
    }
}

namespace ApexTransport {

void EspNowTransport::init(ChassisTelemetry *target_chassis) {
    _chassis = target_chassis;
    s_active_chassis = target_chassis;

    ESP_LOGI(TAG, "Initializing Native ESP-NOW WiFi stack...");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE));

    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(on_esp_now_recv));

    ESP_LOGI(TAG, "ESP-NOW Receiver initialized on Channel 1.");
}

void EspNowTransport::update() {
    if (!_chassis) return;
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000ULL);
    if (now - s_last_packet_time_ms > 1500) {
        _chassis->connected = false;
    }
}

bool EspNowTransport::isConnected() const {
    return _chassis ? _chassis->connected : false;
}

} // namespace ApexTransport

#else // Non-ESP desktop stub

namespace ApexTransport {
void EspNowTransport::init(ChassisTelemetry *) {}
void EspNowTransport::update() {}
bool EspNowTransport::isConnected() const { return false; }
}

#endif
