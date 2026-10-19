#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_http_server.h"
#include "esp_now.h"
#include "mdns.h"
#include "apex_tasks.h"
#include "apex_utils.h"

#include "apex_comms.h"

#define AP_SSID "APEX-TWIN-Setup"
#define NVS_NAMESPACE "wifi_config"
#define APEX_CAN_FD_MAGIC 0x41434644
#define CAN_ID_FAST_DYNAMICS 0x100
#define CAN_ID_GNSS_STATUS 0x210
#define CAN_ID_GNSS_SATELLITES 0x211

static const char *TAG = "WIFI_PROV";
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static httpd_handle_t server = NULL;

#define MDNS_HOSTNAME "apex-twin"
static httpd_handle_t sta_server = NULL;

typedef struct __attribute__((packed)) {
    uint32_t id;
    uint8_t len;
    uint8_t flags;
    uint8_t reserved[2];
    uint32_t timestamp_us;
    uint8_t data[64];
} can_fd_frame_t;

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint16_t seq;
    uint8_t frame_count;
    uint8_t reserved;
    can_fd_frame_t frames[3];
} apex_can_fd_packet_t;

typedef struct __attribute__((packed)) {
    uint16_t rpm;
    uint16_t speed_raw;
    uint8_t gear;
    uint8_t status_flags;
    uint8_t current_sector;
    uint8_t total_sectors;
    uint16_t lap_number;
    uint32_t lap_time_ms;
    int16_t pred_delta_ms;
} can_fast_dynamics_payload_t;

typedef struct __attribute__((packed)) {
    int32_t latitude_scaled;
    int32_t longitude_scaled;
    int16_t altitude_m_raw;
    uint16_t heading_raw;
    uint8_t satellites;
    uint8_t fix_type;
    uint16_t hdop_raw;
    uint8_t reserved[8];
} can_gnss_status_payload_t;

typedef struct __attribute__((packed)) {
    uint8_t count;
    uint8_t snr[10];
    uint8_t reserved[5];
} can_gnss_satellites_payload_t;

static uint16_t espnow_sequence;
static bool espnow_ready;
static const uint8_t espnow_broadcast_mac[ESP_NOW_ETH_ALEN] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static const char *control_html =
"<!doctype html><html><head><meta name=viewport content='width=device-width,initial-scale=1'>"
"<title>APEX-TWIN Control</title><style>"
"*{box-sizing:border-box}body{margin:0;background:#111820;color:#e7edf2;font:15px system-ui,sans-serif}"
"header{padding:22px 18px;background:#18242e;border-bottom:1px solid #30404b}h1{margin:0;font-size:22px}"
"nav{display:flex;gap:6px;padding:10px;background:#202f39;overflow:auto}nav button{border:0;background:#31434e;color:#b9c6cc;padding:10px 14px;border-radius:5px;white-space:nowrap}nav button.active{background:#f0a34a;color:#182029}"
"main{max-width:900px;margin:auto;padding:18px}.tab{display:none}.tab.active{display:block}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(180px,1fr));gap:12px}"
"section{background:#1c2933;border:1px solid #30404b;border-radius:6px;padding:16px;margin-bottom:14px}h2{font-size:18px;margin:0 0 14px}label{display:block;color:#9fb0ba;margin:10px 0 4px}input,select,textarea{width:100%;padding:10px;border:1px solid #41535e;border-radius:4px;background:#111820;color:#eef3f5}textarea{min-height:90px;font-family:monospace}button.action{margin-top:12px;padding:10px 14px;border:0;border-radius:4px;background:#f0a34a;color:#182029;font-weight:700}"
".metric{font-size:25px;color:#f0a34a}.muted{color:#9fb0ba}.ok{color:#7bd89a}.error{color:#ff8c80}pre{white-space:pre-wrap;word-break:break-word}" 
"</style></head><body><header><h1>APEX-TWIN module control</h1><div class=muted>GPS, RTK and communications console</div></header>"
"<nav><button class=active data-tab=overview>Overview</button><button data-tab=position>Position</button><button data-tab=gps>GPS</button><button data-tab=rtk>RTK</button><button data-tab=wifi>Wi-Fi</button></nav><main>"
"<div id=overview class='tab active'><section><h2>Module status</h2><div class=grid><div><div class=muted>GPS fix</div><div id=fix class=metric>--</div></div><div><div class=muted>Speed</div><div id=ospeed class=metric>--</div></div><div><div class=muted>RTK age</div><div id=age class=metric>--</div></div></div><p id=network class=muted>Loading...</p></section></div>"
"<div id=position class=tab><section><h2>Position and speed</h2><div class=grid><div><div class=muted>Latitude</div><div id=lat class=metric>--</div></div><div><div class=muted>Longitude</div><div id=lon class=metric>--</div></div><div><div class=muted>Altitude</div><div id=alt class=metric>--</div></div><div><div class=muted>Speed</div><div id=speed class=metric>--</div></div></div><p id=positionState class=muted>Waiting for GPS data...</p></section></div>"
"<div id=gps class=tab><section><h2>GPS command console</h2><label>Command (with or without checksum)</label><textarea id=command placeholder='$PQTMCFGUART,W,460800'></textarea><button class=action onclick=sendCommand()>Send command</button><pre id=commandResult></pre></section><section><h2>Quick configuration</h2><label>Communication rate</label><select id=baud><option value=115200>115200 baud</option><option value=460800 selected>460800 baud</option></select><label>GPS update rate</label><select id=rate><option value=1000>1 Hz</option><option value=200>5 Hz</option><option value=100>10 Hz</option><option value=50>20 Hz</option></select><button class=action onclick=applyRates()>Apply and save GPS rates</button></section></div>"
"<div id=rtk class=tab><section><h2>NTRIP / RTK</h2><form id=rtkForm><label>Host</label><input name=host><label>Port</label><input name=port type=number><label>Mountpoint</label><input name=mountpoint><label>Username</label><input name=username><label>Password</label><input name=password type=password><label>Client name</label><input name=client_name><label>GGA interval (ms)</label><input name=gga_interval_ms type=number></form><button class=action onclick=saveRtk()>Save RTK settings</button><p id=rtkResult class=muted></p></section></div>"
"<div id=wifi class=tab><section><h2>Network</h2><p id=wifiState>Loading...</p><p class=muted>Connected network</p><form action='/save' method=post><label>New SSID</label><input name=ssid required maxlength=32><label>New password</label><input name=pass type=password maxlength=63><button class=action type=submit>Save and reconnect</button></form><p class=muted>Saving restarts the module and reconnects using the new credentials. Provisioning mode remains available at 192.168.4.1 if the connection fails.</p></section></div>"
"</main><script>"
"const q=s=>document.querySelector(s);document.querySelectorAll('nav button').forEach(b=>b.onclick=()=>{document.querySelectorAll('nav button,.tab').forEach(x=>x.classList.remove('active'));b.classList.add('active');q('#'+b.dataset.tab).classList.add('active');});"
"async function json(url,opt){let r=await fetch(url,opt);return await r.json()}"
"function gpsModeName(mode){return ({0:'No fix',1:'GPS',2:'DGPS',3:'PPS',4:'RTK fixed',5:'RTK float',6:'Estimated',7:'Manual',8:'Simulation'})[mode]||'Unknown ('+mode+')'}"
"async function refresh(){try{let d=await json('/api/position');q('#lat').textContent=d.valid?d.latitude.toFixed(6):'--';q('#lon').textContent=d.valid?d.longitude.toFixed(6):'--';q('#alt').textContent=d.valid?d.altitude_m.toFixed(1)+' m':'--';q('#speed').textContent=d.valid?d.speed_kmh.toFixed(1)+' km/h':'--';q('#ospeed').textContent=d.valid?d.speed_kmh.toFixed(1)+' km/h':'--';q('#fix').textContent=gpsModeName(d.gps_mode);q('#age').textContent=d.valid?d.correction_age_s.toFixed(1)+' s':'--';q('#positionState').textContent=d.valid?'UTC '+d.utc_time_ms+' ms':'Waiting for GPS data';let s=await json('/api/status');q('#network').textContent=s.connected?'Connected to '+s.ssid+' ('+s.ip+')':'Not connected';q('#wifiState').textContent=q('#network').textContent}catch(e){q('#network').textContent='Status unavailable'}}"
"async function sendCommand(){let body='command='+encodeURIComponent(q('#command').value);let d=await json('/api/command',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});q('#commandResult').textContent=d.message||d.error}"
"async function applyRates(){let body='baud='+q('#baud').value+'&rate='+q('#rate').value;let d=await json('/api/rates',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});q('#commandResult').textContent=d.message||d.error}"
"async function loadRtk(){let d=await json('/api/rtk');for(let k in d){let e=document.querySelector('[name='+k+']');if(e)e.value=d[k]}}async function saveRtk(){let f=new FormData(q('#rtkForm')),body=new URLSearchParams(f);let d=await json('/api/rtk',{method:'POST',body});q('#rtkResult').textContent=d.message||d.error}loadRtk();refresh();setInterval(refresh,1000);</script></body></html>";

// HTML Configuration Page
static const char *index_html = 
"<!DOCTYPE HTML><html><head>"
"<title>APEX-TWIN Wi-Fi Setup</title>"
"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
"<style>"
"body { font-family: Arial; margin: 20px; background-color: #f4f4f9; }"
".card { max-width: 380px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.15); }"
"label { display: block; margin-top: 12px; font-weight: bold; }"
"input[type=text], input[type=password] { width: 100%; padding: 10px; margin-top: 5px; box-sizing: border-box; }"
"input[type=submit] { background: #28a745; color: white; border: none; padding: 12px; width: 100%; margin-top: 20px; border-radius: 4px; font-size: 16px; cursor: pointer; }"
"</style></head><body>"
"<div class=\"card\">"
"<h2>Wi-Fi Setup</h2>"
"<form action=\"/save\" method=\"POST\">"
"<label>SSID:</label><input type=\"text\" name=\"ssid\" required>"
"<label>Password:</label><input type=\"password\" name=\"pass\">"
"<input type=\"submit\" value=\"Save & Connect\">"
"</form></div></body></html>";

// Helper function to extract POST form fields
static void url_decode(char *dst, const char *src, size_t max_len) {
    char a, b;
    size_t length = 0;
    while (*src && length + 1 < max_len) {
        if ((*src == '%') && ((a = src[1]) && (b = src[2])) && (isxdigit(a) && isxdigit(b))) {
            if (a >= 'a' && a <= 'f') a -= 'a' - 'A';
            if (a >= 'A' && a <= 'F') a -= ('A' - 10);
            else a -= '0';
            if (b >= 'a' && b <= 'f') b -= 'a' - 'A';
            if (b >= 'A' && b <= 'F') b -= ('A' - 10);
            else b -= '0';
            *dst++ = 16 * a + b;
            length++;
            src += 3;
        } else if (*src == '+') {
            *dst++ = ' ';
            length++;
            src++;
        } else {
            *dst++ = *src++;
            length++;
        }
    }
    *dst = '\0';
}

static void parse_form_value(const char *buf, const char *key, char *out_val, size_t max_len) {
    char key_match[32];
    snprintf(key_match, sizeof(key_match), "%s=", key);
    char *start = strstr(buf, key_match);
    if (start) {
        start += strlen(key_match);
        char *end = strchr(start, '&');
        char raw[64] = {0};
        if (end) {
            size_t len = end - start;
            if (len >= sizeof(raw)) len = sizeof(raw) - 1;
            strncpy(raw, start, len);
        } else {
            strncpy(raw, start, sizeof(raw) - 1);
        }
        url_decode(out_val, raw, max_len);
    }
}

// Save credentials to NVS
static esp_err_t save_wifi_credentials(const char *ssid, const char *pass) {
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK) return err;

    nvs_set_str(nvs, "ssid", ssid);
    nvs_set_str(nvs, "pass", pass);
    err = nvs_commit(nvs);
    nvs_close(nvs);
    return err;
}

// Read credentials from NVS
static bool load_wifi_credentials(char *ssid, size_t ssid_len, char *pass, size_t pass_len) {
    nvs_handle_t nvs;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs) != ESP_OK) return false;

    esp_err_t err_s = nvs_get_str(nvs, "ssid", ssid, &ssid_len);
    esp_err_t err_p = nvs_get_str(nvs, "pass", pass, &pass_len);
    nvs_close(nvs);

    return (err_s == ESP_OK && strlen(ssid) > 0);
}

// HTTP GET Handler for setup page
static esp_err_t root_get_handler(httpd_req_t *req) {
    httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// HTTP POST Handler for submitting credentials
static esp_err_t save_post_handler(httpd_req_t *req) {
    char buf[256] = {0};
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) return ESP_FAIL;

    char ssid[33] = {0};
    char pass[64] = {0};

    parse_form_value(buf, "ssid", ssid, sizeof(ssid));
    parse_form_value(buf, "pass", pass, sizeof(pass));

    if (strlen(ssid) > 0) {
        ESP_LOGI(TAG, "Received SSID: %s", ssid);
        save_wifi_credentials(ssid, pass);

        const char *resp = "<h3>Settings saved! Restarting ESP32...</h3>";
        httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);

        vTaskDelay(pdMS_TO_TICKS(1500));
        esp_restart();
    } else {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid SSID");
    }

    return ESP_OK;
}

// Start Web Server
void start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t root_uri = { .uri = "/", .method = HTTP_GET, .handler = root_get_handler };
        httpd_register_uri_handler(server, &root_uri);

        httpd_uri_t save_uri = { .uri = "/save", .method = HTTP_POST, .handler = save_post_handler };
        httpd_register_uri_handler(server, &save_uri);
    }
}

// Start Access Point Mode
static void start_ap_mode(void) {
    ESP_LOGI(TAG, "Starting SoftAP mode...");
    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = AP_SSID,
            .ssid_len = strlen(AP_SSID),
            .channel = 6,
            .max_connection = 4,
            .authmode = WIFI_AUTH_OPEN
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Access Point running! SSID: %s, Connect and go to 192.168.4.1", AP_SSID);

    start_webserver();
}

static void enable_ap_alongside_sta(void)
{
    esp_netif_create_default_wifi_ap();
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = AP_SSID,
            .ssid_len = strlen(AP_SSID),
            .channel = 6,
            .max_connection = 4,
            .authmode = WIFI_AUTH_OPEN
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_LOGI(TAG, "Control AP available at http://192.168.4.1");
}

// Event handler for Station connection attempts
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    static int retry_num = 0;
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (retry_num < 10) {
            esp_wifi_connect();
            retry_num++;
            ESP_LOGI(TAG, "Retrying connection to AP...");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP Address: " IPSTR, IP2STR(&event->ip_info.ip));
        retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// Connect to target router in STA Mode
static bool connect_sta(const char *ssid, const char *pass) {
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

    wifi_config_t wifi_config = {0};
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Connecting to SSID: %s...", ssid);

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Successfully connected to Wi-Fi!");
        return true;
    } else {
        ESP_LOGE(TAG, "Failed to connect to Wi-Fi.");
        return false;
    }
}


// 1. Initialize mDNS Service
static void start_mdns_service(void) {
    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set(MDNS_HOSTNAME));
    ESP_ERROR_CHECK(mdns_instance_name_set("ESP32 Web Control"));

    // Register HTTP service on port 80
    ESP_ERROR_CHECK(mdns_service_add("ESP32 Web Server", "_http", "_tcp", 80, NULL, 0));
    ESP_LOGI(TAG, "mDNS started. Hostname: http://%s.local", MDNS_HOSTNAME);
}

static bool espnow_init_sender(void)
{
    // The station must remain on the channel selected by its access point.
    // Calling esp_wifi_set_channel() here races with STA connection handling
    // and fails with WIFI_REASON_ASSOC_TOOMANY or ESP_FAIL.
    uint8_t channel = 0;
    wifi_second_chan_t secondary_channel = WIFI_SECOND_CHAN_NONE;
    esp_err_t error = esp_wifi_get_channel(&channel, &secondary_channel);
    if (error != ESP_OK || channel == 0) {
        ESP_LOGE(TAG, "Could not determine the active Wi-Fi channel: %s",
                 esp_err_to_name(error));
        return false;
    }

    error = esp_now_init();
    if (error != ESP_OK && error != ESP_ERR_ESPNOW_EXIST) {
        ESP_LOGE(TAG, "ESP-NOW initialization failed: %s", esp_err_to_name(error));
        return false;
    }

    esp_now_peer_info_t peer = {0};
    memcpy(peer.peer_addr, espnow_broadcast_mac, ESP_NOW_ETH_ALEN);
    // Channel 0 means the current station channel.
    peer.channel = 0;
    peer.ifidx = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF") != NULL
        ? WIFI_IF_STA : WIFI_IF_AP;
    peer.encrypt = false;
    if (!esp_now_is_peer_exist(peer.peer_addr)) {
        error = esp_now_add_peer(&peer);
        if (error != ESP_OK && error != ESP_ERR_ESPNOW_EXIST) {
            ESP_LOGE(TAG, "Could not add ESP-NOW broadcast peer: %s", esp_err_to_name(error));
            return false;
        }
    }

    espnow_ready = true;
    uint8_t mac[6] = {0};
    esp_wifi_get_mac(peer.ifidx, mac);
    ESP_LOGI(TAG,
             "ESP-NOW telemetry sender ready: channel=%d MAC=%02X:%02X:%02X:%02X:%02X:%02X",
             channel, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return true;
}

static void fill_telemetry_packet(apex_can_fd_packet_t *packet, const gps_data_t *gps,
                                  bool include_satellites)
{
    memset(packet, 0, sizeof(*packet));
    packet->magic = APEX_CAN_FD_MAGIC;
    packet->seq = ++espnow_sequence;
    packet->frame_count = 2;
    uint32_t timestamp_us = gps->utc_time_ms * 1000U;

    can_fast_dynamics_payload_t *fast = (can_fast_dynamics_payload_t *)packet->frames[0].data;
    packet->frames[0].id = CAN_ID_FAST_DYNAMICS;
    packet->frames[0].len = sizeof(*fast);
    packet->frames[0].timestamp_us = timestamp_us;
    fast->speed_raw = (uint16_t)(gps->speed_kmh > 0.0f ? gps->speed_kmh * 20.0f : 0.0f);
    fast->gear = 0;
    fast->current_sector = 1;
    fast->total_sectors = 3;
    fast->lap_number = 1;

    can_gnss_status_payload_t *gnss = (can_gnss_status_payload_t *)packet->frames[1].data;
    packet->frames[1].id = CAN_ID_GNSS_STATUS;
    packet->frames[1].len = sizeof(*gnss);
    packet->frames[1].timestamp_us = timestamp_us;
    gnss->latitude_scaled = (int32_t)(gps->latitude * 10000000.0);
    gnss->longitude_scaled = (int32_t)(gps->longitude * 10000000.0);
    gnss->altitude_m_raw = (int16_t)(gps->altitude_m * 10.0f);
    gnss->satellites = gps->satellites;
    gnss->fix_type = gps->gps_mode;
    gnss->hdop_raw = (uint16_t)(gps->hdop * 100.0f);

    if (include_satellites) {
        can_gnss_satellites_payload_t *satellites =
            (can_gnss_satellites_payload_t *)packet->frames[2].data;
        packet->frames[2].id = CAN_ID_GNSS_SATELLITES;
        packet->frames[2].len = sizeof(*satellites);
        packet->frames[2].timestamp_us = timestamp_us;
        satellites->count = gps->satellite_snr_count > 10 ? 10 : gps->satellite_snr_count;
        memcpy(satellites->snr, gps->satellite_snr, sizeof(satellites->snr));
        packet->frame_count = 3;
    }
}

void espnow_telemetry_task(void *pvParameters)
{
    (void)pvParameters;
    while (!espnow_ready) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    while (true) {
        gps_data_t gps = {0};
        gps_get_latest_data(&gps);
        apex_can_fd_packet_t packet;
        static TickType_t last_satellite_send_tick;
        TickType_t now = xTaskGetTickCount();
        bool include_satellites =
            now - last_satellite_send_tick >= pdMS_TO_TICKS(10000);
        fill_telemetry_packet(&packet, &gps, include_satellites);
        if (include_satellites) {
            last_satellite_send_tick = now;
        }
        uint8_t frame_count = packet.frame_count;
        esp_err_t error = esp_now_send(espnow_broadcast_mac, (const uint8_t *)&packet,
                                       sizeof(packet.magic) + sizeof(packet.seq) +
                                       2 + frame_count * sizeof(can_fd_frame_t));
        if (error != ESP_OK) {
            ESP_LOGW(TAG, "ESP-NOW telemetry send failed: %s", esp_err_to_name(error));
        }
        vTaskDelay(pdMS_TO_TICKS(40));
    }
}

// 2. Web Page Handler for STA Mode
static esp_err_t sta_root_get_handler(httpd_req_t *req) {
    httpd_resp_send(req, control_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t status_get_handler(httpd_req_t *req);
static esp_err_t position_get_handler(httpd_req_t *req);
static esp_err_t command_post_handler(httpd_req_t *req);
static esp_err_t rates_post_handler(httpd_req_t *req);
static esp_err_t rtk_get_handler(httpd_req_t *req);
static esp_err_t rtk_post_handler(httpd_req_t *req);

// 3. Start Web Server in STA Mode
static void start_sta_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&sta_server, &config) == ESP_OK) {
        httpd_uri_t root_uri = {
            .uri      = "/",
            .method   = HTTP_GET,
            .handler  = sta_root_get_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(sta_server, &root_uri);
        httpd_uri_t status_uri = { .uri = "/api/status", .method = HTTP_GET, .handler = status_get_handler };
        httpd_register_uri_handler(sta_server, &status_uri);
        httpd_uri_t position_uri = { .uri = "/api/position", .method = HTTP_GET, .handler = position_get_handler };
        httpd_register_uri_handler(sta_server, &position_uri);
        httpd_uri_t command_uri = { .uri = "/api/command", .method = HTTP_POST, .handler = command_post_handler };
        httpd_register_uri_handler(sta_server, &command_uri);
        httpd_uri_t rates_uri = { .uri = "/api/rates", .method = HTTP_POST, .handler = rates_post_handler };
        httpd_register_uri_handler(sta_server, &rates_uri);
        httpd_uri_t rtk_get_uri = { .uri = "/api/rtk", .method = HTTP_GET, .handler = rtk_get_handler };
        httpd_register_uri_handler(sta_server, &rtk_get_uri);
        httpd_uri_t rtk_post_uri = { .uri = "/api/rtk", .method = HTTP_POST, .handler = rtk_post_handler };
        httpd_register_uri_handler(sta_server, &rtk_post_uri);
        httpd_uri_t save_uri = { .uri = "/save", .method = HTTP_POST, .handler = save_post_handler };
        httpd_register_uri_handler(sta_server, &save_uri);
        ESP_LOGI(TAG, "STA Web Server started on port %d", config.server_port);
    }
}

void wifi_setup_task(void *pvParameters) {
    // 1. Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ntrip_load_config();

    // 2. Network and Event Loop Initialization
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 3. Read saved credentials
    char ssid[33] = {0};
    char pass[64] = {0};
    bool has_creds = load_wifi_credentials(ssid, sizeof(ssid), pass, sizeof(pass));

    // 4. Connect or start AP setup mode
    if (has_creds && connect_sta(ssid, pass)) {
        ESP_LOGI(TAG, "Connected to network!");
        enable_ap_alongside_sta();

        // Start mDNS service (allows accessing http://myesp32.local)
        start_mdns_service();

        // Start the web application server
        start_sta_webserver();

    } else {
        start_ap_mode();
    }

    if (espnow_init_sender()) {
        xTaskCreate(espnow_telemetry_task, "espnow_telemetry", 4096, NULL, 6, NULL);
    }

    // Task finished running initialization steps
    vTaskDelete(NULL);
}

static esp_err_t send_json(httpd_req_t *req, const char *json)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t position_get_handler(httpd_req_t *req)
{
    gps_data_t data = {0};
    bool valid = gps_get_latest_data(&data);
    char response[256];
    snprintf(response, sizeof(response),
             "{\"valid\":%s,\"latitude\":%.7f,\"longitude\":%.7f,\"altitude_m\":%.2f,\"speed_kmh\":%.2f,\"gps_mode\":%u,\"correction_age_s\":%.2f,\"utc_time_ms\":%lu}",
             valid ? "true" : "false", data.latitude, data.longitude, data.altitude_m,
             data.speed_kmh, data.gps_mode, data.correction_age_s,
             (unsigned long)data.utc_time_ms);
    return send_json(req, response);
}

static esp_err_t status_get_handler(httpd_req_t *req)
{
    wifi_ap_record_t ap_info = {0};
    bool connected = esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK;
    esp_netif_ip_info_t ip_info = {0};
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif != NULL) {
        esp_netif_get_ip_info(netif, &ip_info);
    }
    char response[256];
    snprintf(response, sizeof(response),
             "{\"connected\":%s,\"ssid\":\"%s\",\"ip\":\"%u.%u.%u.%u\",\"rssi\":%d}",
             connected ? "true" : "false", connected ? (char *)ap_info.ssid : "",
             IP2STR(&ip_info.ip), connected ? ap_info.rssi : 0);
    return send_json(req, response);
}

static esp_err_t command_post_handler(httpd_req_t *req)
{
    char body[192] = {0};
    int received = httpd_req_recv(req, body, sizeof(body) - 1);
    if (received <= 0) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Command is required");
    }
    char command[128] = {0};
    parse_form_value(body, "command", command, sizeof(command));
    if (command[0] == '\0' || gps_send_command(command, false) != ESP_OK) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Command could not be sent");
    }
    return send_json(req, "{\"message\":\"GPS command sent\"}");
}

static esp_err_t rates_post_handler(httpd_req_t *req)
{
    char body[128] = {0};
    if (httpd_req_recv(req, body, sizeof(body) - 1) <= 0) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Rate settings are required");
    }
    char baud[12] = {0};
    char rate[12] = {0};
    parse_form_value(body, "baud", baud, sizeof(baud));
    parse_form_value(body, "rate", rate, sizeof(rate));
    char command[64];
    snprintf(command, sizeof(command), "$PQTMCFGUART,W,%s", baud);
    if (gps_send_command(command, false) != ESP_OK) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Could not set communication rate");
    }
    snprintf(command, sizeof(command), "$PQTMCFGFIXRATE,W,%s", rate);
    gps_send_command(command, false);
    gps_send_command("$PQTMSAVEPAR", false);
    return send_json(req, "{\"message\":\"GPS rates applied\"}");
}

static esp_err_t rtk_get_handler(httpd_req_t *req)
{
    ntrip_config_t config;
    ntrip_get_config(&config);
    char response[512];
    snprintf(response, sizeof(response),
             "{\"host\":\"%s\",\"port\":%u,\"mountpoint\":\"%s\",\"username\":\"%s\",\"client_name\":\"%s\",\"gga_interval_ms\":%lu}",
             config.host, config.port, config.mountpoint, config.username, config.client_name,
             (unsigned long)config.gga_interval_ms);
    return send_json(req, response);
}

static esp_err_t rtk_post_handler(httpd_req_t *req)
{
    char body[512] = {0};
    if (httpd_req_recv(req, body, sizeof(body) - 1) <= 0) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "RTK settings are required");
    }
    ntrip_config_t config;
    ntrip_get_config(&config);
    parse_form_value(body, "host", config.host, sizeof(config.host));
    parse_form_value(body, "mountpoint", config.mountpoint, sizeof(config.mountpoint));
    parse_form_value(body, "username", config.username, sizeof(config.username));
    char password[64] = {0};
    parse_form_value(body, "password", password, sizeof(password));
    if (password[0] != '\0') {
        strncpy(config.password, password, sizeof(config.password) - 1);
    }
    parse_form_value(body, "client_name", config.client_name, sizeof(config.client_name));
    char value[16] = {0};
    parse_form_value(body, "port", value, sizeof(value));
    config.port = (uint16_t)strtoul(value, NULL, 10);
    memset(value, 0, sizeof(value));
    parse_form_value(body, "gga_interval_ms", value, sizeof(value));
    config.gga_interval_ms = (uint32_t)strtoul(value, NULL, 10);
    if (ntrip_save_config(&config) != ESP_OK) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid RTK settings");
    }
    return send_json(req, "{\"message\":\"RTK settings saved\"}");
}