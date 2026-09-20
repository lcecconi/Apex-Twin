#include "ota_manager.h"
#include "sd_manager.h"
#include "config.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_app_format.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

#include "esp_now.h"
#include "esp_now_receiver.h"

static const char *TAG = "OTA_MGR";

static const char INDEX_HTML[] = 
"<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>Apex-Dash Maintenance & Firmware Update</title><style>"
"body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:#0d1117;color:#c9d1d9;margin:0;padding:24px;display:flex;justify-content:center}"
".card{background:#161b22;border:1px solid #30363d;border-radius:10px;max-width:480px;width:100%;padding:24px;box-shadow:0 8px 24px rgba(0,0,0,0.5)}"
"h1{font-size:20px;color:#58a6ff;margin-top:0;display:flex;align-items:center;gap:8px}"
"p{font-size:14px;color:#8b949e;line-height:1.5}"
".info-box{background:#21262d;border-radius:6px;padding:12px;margin-bottom:20px;font-size:13px}"
".info-row{display:flex;justify-content:space-between;margin-bottom:6px}"
".info-label{color:#8b949e}.info-val{color:#f0f6fc;font-weight:600}"
".drop-zone{border:2px dashed #30363d;border-radius:6px;padding:24px;text-align:center;cursor:pointer;transition:0.2s;margin-bottom:20px}"
".drop-zone:hover{border-color:#58a6ff;background:#1f242c}"
"input[type='file']{display:none}"
".btn{background:#238636;color:#ffffff;border:none;border-radius:6px;padding:12px 16px;font-size:14px;font-weight:600;cursor:pointer;width:100%;transition:0.2s}"
".btn:hover{background:#2ea043}.btn:disabled{background:#21262d;color:#484f58;cursor:not-allowed}"
".progress-bar{height:12px;background:#21262d;border-radius:6px;overflow:hidden;margin-top:16px;display:none}"
".progress-fill{height:100%;background:#238636;width:0%;transition:width 0.2s}"
"#status{font-size:13px;margin-top:14px;text-align:center;font-weight:500}"
"</style></head><body><div class='card'>"
"<h1>🏎️ Apex-Dash Maintenance OTA</h1>"
"<p>Upload a compiled <code>firmware.bin</code> to flash the steering display over your Wi-Fi network.</p>"
"<div class='info-box'>"
"<div class='info-row'><span class='info-label'>Target Hardware:</span><span class='info-val'>Waveshare ESP32-S3-RLCD-4.2</span></div>"
"<div class='info-row'><span class='info-label'>Flash Capacity:</span><span class='info-val'>16 MB (Dual 6 MB OTA Slots)</span></div>"
"<div class='info-row'><span class='info-label'>Mode:</span><span class='info-val'>Station Mode (Local Network)</span></div>"
"</div>"
"<form id='upload-form'>"
"<div class='drop-zone' onclick=\"document.getElementById('file-input').click()\">"
"<span id='file-name'>Click here to select firmware.bin</span>"
"<input type='file' id='file-input' accept='.bin' onchange='fileSelected(this)'>"
"</div>"
"<button type='button' id='btn-upload' class='btn' disabled onclick='uploadFirmware()'>Start Wireless Update</button>"
"<div class='progress-bar' id='p-bar'><div class='progress-fill' id='p-fill'></div></div>"
"<div id='status'></div>"
"</form></div>"
"<script>"
"function fileSelected(input){"
"if(input.files&&input.files[0]){"
"document.getElementById('file-name').innerText=input.files[0].name+' ('+(input.files[0].size/1024).toFixed(1)+' KB)';"
"document.getElementById('btn-upload').disabled=false;"
"}}"
"function uploadFirmware(){"
"var file=document.getElementById('file-input').files[0];"
"if(!file)return;"
"var btn=document.getElementById('btn-upload');"
"btn.disabled=true;"
"document.getElementById('p-bar').style.display='block';"
"var status=document.getElementById('status');"
"status.style.color='#58a6ff';"
"status.innerText='Uploading and writing to flash partition...';"
"var xhr=new XMLHttpRequest();"
"xhr.open('POST','/update',true);"
"xhr.upload.onprogress=function(e){"
"if(e.lengthComputable){"
"var pct=Math.round((e.loaded/e.total)*100);"
"document.getElementById('p-fill').style.width=pct+'%';"
"status.innerText='Flashing: '+pct+'% (Do not turn off display power)';"
"}};"
"xhr.onload=function(){"
"if(xhr.status===200){"
"status.style.color='#3fb950';"
"status.innerText='✔ Update Successful! Dashboard is rebooting...';"
"document.getElementById('p-fill').style.background='#3fb950';"
"}else{"
"status.style.color='#f85149';"
"status.innerText='✖ Upload failed: '+xhr.responseText;"
"btn.disabled=false;"
"}};"
"xhr.onerror=function(){"
"status.style.color='#f85149';"
"status.innerText='✖ Connection error during update.';"
"btn.disabled=false;"
"};"
"xhr.send(file);"
"}"
"</script></body></html>";

static void delayed_reboot_task(void *param) {
  vTaskDelay(pdMS_TO_TICKS(1500));
  ESP_LOGI(TAG, "Rebooting into new firmware...");
  esp_restart();
}

static esp_err_t index_get_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, INDEX_HTML, HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

static esp_err_t update_post_handler(httpd_req_t *req) {
  char buf[2048];
  int remaining = req->content_len;
  int total_len = req->content_len;

  while (remaining > 0) {
    int to_read = (remaining > (int)sizeof(buf)) ? (int)sizeof(buf) : remaining;
    int received = httpd_req_recv(req, buf, to_read);
    if (received <= 0) {
      if (received == HTTPD_SOCK_ERR_TIMEOUT) continue;
      OtaManager::instance().onHttpUploadFinish(false, "Socket receive error");
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Socket error");
      return ESP_FAIL;
    }

    esp_err_t err = OtaManager::instance().onHttpUploadChunk((const uint8_t*)buf, received, total_len);
    if (err != ESP_OK) {
      OtaManager::instance().onHttpUploadFinish(false, "Flash write error");
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA write failed");
      return ESP_FAIL;
    }

    remaining -= received;
  }

  OtaManager::instance().onHttpUploadFinish(true);
  httpd_resp_set_type(req, "text/plain");
  httpd_resp_sendstr(req, "OK - Update Successful");

  xTaskCreate(delayed_reboot_task, "ota_reboot", 2048, NULL, 5, NULL);
  return ESP_OK;
}

static esp_netif_t *s_sta_netif = nullptr;
static esp_event_handler_instance_t s_wifi_event_handler = nullptr;
static esp_event_handler_instance_t s_ip_event_handler = nullptr;
static int s_retry_count = 0;

static void wifi_sta_event_handler(void* arg, esp_event_base_t event_base,
                                   int32_t event_id, void* event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    if (s_retry_count < 6) {
      s_retry_count++;
      ESP_LOGI(TAG, "Retrying Wi-Fi connection (%d/6)...", s_retry_count);
      vTaskDelay(pdMS_TO_TICKS(500));
      esp_wifi_connect();
    } else {
      OtaManager::instance().onWiFiConnectFailed("Wi-Fi connection failed (Check SSID/Password)");
    }
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
    char ip_str[32];
    esp_ip4addr_ntoa(&event->ip_info.ip, ip_str, sizeof(ip_str));
    s_retry_count = 0;
    OtaManager::instance().onWiFiConnected(ip_str);
  }
}

OtaManager& OtaManager::instance() {
  static OtaManager s_instance;
  return s_instance;
}

OtaManager::OtaManager() {
  strncpy(_active_ssid, DEFAULT_WIFI_SSID, sizeof(_active_ssid) - 1);
  strncpy(_active_pass, DEFAULT_WIFI_PASS, sizeof(_active_pass) - 1);
}

OtaManager::~OtaManager() {
  cancel();
}

void OtaManager::loadWiFiCredentialsFromSD() {
  FILE *f = fopen("/sdcard/wifi.cfg", "r");
  if (!f) return;

  char line[128];
  while (fgets(line, sizeof(line), f)) {
    char *cr = strchr(line, '\r'); if (cr) *cr = '\0';
    char *nl = strchr(line, '\n'); if (nl) *nl = '\0';

    if (strncmp(line, "SSID=", 5) == 0 || strncmp(line, "ssid=", 5) == 0) {
      strncpy(_active_ssid, line + 5, sizeof(_active_ssid) - 1);
      _active_ssid[sizeof(_active_ssid) - 1] = '\0';
    } else if (strncmp(line, "PASS=", 5) == 0 || strncmp(line, "pass=", 5) == 0) {
      strncpy(_active_pass, line + 5, sizeof(_active_pass) - 1);
      _active_pass[sizeof(_active_pass) - 1] = '\0';
    }
  }
  fclose(f);
  ESP_LOGI(TAG, "Loaded Wi-Fi credentials from /sdcard/wifi.cfg (SSID: %s)", _active_ssid);
}

void OtaManager::begin(SDManager *sdMgr) {
  _sd_mgr = sdMgr;
  _status = OTA_IDLE;
  _progress_pct = 0;
  _err_msg[0] = '\0';
  _active_file[0] = '\0';
  loadWiFiCredentialsFromSD();
}

bool OtaManager::isBusy() const {
  return (_status == OTA_SD_IN_PROGRESS || _status == MAINTENANCE_CONNECTING ||
          _status == MAINTENANCE_READY || _status == MAINTENANCE_RECEIVING);
}

void OtaManager::cancel() {
  if (_ota_handle) {
    esp_ota_abort(_ota_handle);
    _ota_handle = 0;
  }
  stopMaintenanceMode();
  _status = OTA_IDLE;
  _progress_pct = 0;
}

bool OtaManager::checkSdFirmwareAvailable(char *out_filename, size_t max_len) {
  const char *candidates[] = {
    "/sdcard/firmware.bin",
    "/sdcard/apex_dash.bin",
    "/sdcard/update.bin"
  };

  struct stat st;
  for (const char *path : candidates) {
    if (stat(path, &st) == 0 && st.st_size > 32768) {
      if (out_filename && max_len > 0) {
        strncpy(out_filename, path, max_len - 1);
        out_filename[max_len - 1] = '\0';
      }
      return true;
    }
  }
  return false;
}

static void sd_ota_task(void *param) {
  OtaManager::instance().executeSdUpdateWorker();
  vTaskDelete(NULL);
}

bool OtaManager::startSdUpdate(const char *filename) {
  if (isBusy()) return false;

  char chosen_path[64] = {0};
  if (filename && strlen(filename) > 0) {
    strncpy(chosen_path, filename, sizeof(chosen_path) - 1);
  } else if (!checkSdFirmwareAvailable(chosen_path, sizeof(chosen_path))) {
    strncpy(_err_msg, "No firmware.bin found on MicroSD", sizeof(_err_msg) - 1);
    _status = OTA_SD_FAILED;
    return false;
  }

  strncpy(_active_file, chosen_path, sizeof(_active_file) - 1);
  _status = OTA_SD_IN_PROGRESS;
  _progress_pct = 0;
  _err_msg[0] = '\0';

  BaseType_t res = xTaskCreate(sd_ota_task, "sd_ota", 8192, NULL, 5, NULL);
  if (res != pdPASS) {
    strncpy(_err_msg, "Failed to create SD task", sizeof(_err_msg) - 1);
    _status = OTA_SD_FAILED;
    return false;
  }
  return true;
}

void OtaManager::executeSdUpdateWorker() {
  FILE *f = fopen(_active_file, "rb");
  if (!f) {
    strncpy(_err_msg, "Cannot open SD file", sizeof(_err_msg) - 1);
    _status = OTA_SD_FAILED;
    return;
  }

  fseek(f, 0, SEEK_END);
  _binary_size = ftell(f);
  fseek(f, 0, SEEK_SET);

  if (_binary_size < 32768) {
    fclose(f);
    strncpy(_err_msg, "Firmware file too small", sizeof(_err_msg) - 1);
    _status = OTA_SD_FAILED;
    return;
  }

  _update_partition = esp_ota_get_next_update_partition(NULL);
  if (!_update_partition) {
    fclose(f);
    strncpy(_err_msg, "No available OTA partition", sizeof(_err_msg) - 1);
    _status = OTA_SD_FAILED;
    return;
  }

  ESP_LOGI(TAG, "Writing MicroSD OTA to partition: %s (size %u bytes)", 
           _update_partition->label, (unsigned int)_binary_size);

  esp_err_t err = esp_ota_begin(_update_partition, _binary_size, &_ota_handle);
  if (err != ESP_OK) {
    fclose(f);
    snprintf(_err_msg, sizeof(_err_msg), "OTA begin failed: %s", esp_err_to_name(err));
    _status = OTA_SD_FAILED;
    return;
  }

  uint8_t buffer[4096];
  _bytes_written = 0;

  while (_bytes_written < _binary_size) {
    size_t to_read = sizeof(buffer);
    if (_binary_size - _bytes_written < to_read) {
      to_read = _binary_size - _bytes_written;
    }

    size_t bytes_read = fread(buffer, 1, to_read, f);
    if (bytes_read == 0) {
      fclose(f);
      esp_ota_abort(_ota_handle);
      _ota_handle = 0;
      strncpy(_err_msg, "SD read error", sizeof(_err_msg) - 1);
      _status = OTA_SD_FAILED;
      return;
    }

    err = esp_ota_write(_ota_handle, buffer, bytes_read);
    if (err != ESP_OK) {
      fclose(f);
      esp_ota_abort(_ota_handle);
      _ota_handle = 0;
      snprintf(_err_msg, sizeof(_err_msg), "Flash write error: %s", esp_err_to_name(err));
      _status = OTA_SD_FAILED;
      return;
    }

    _bytes_written += bytes_read;
    _progress_pct = (uint8_t)((_bytes_written * 100) / _binary_size);
    vTaskDelay(pdMS_TO_TICKS(1));
  }

  fclose(f);

  if (_bytes_written == _binary_size &&
      esp_ota_end(_ota_handle) == ESP_OK &&
      esp_ota_set_boot_partition(_update_partition) == ESP_OK) {
    _status = OTA_SD_COMPLETED;
    _progress_pct = 100;
    _ota_handle = 0;
    ESP_LOGI(TAG, "MicroSD OTA finished successfully! Restarting...");
    xTaskCreate(delayed_reboot_task, "ota_reboot", 2048, NULL, 5, NULL);
    return;
  }

  if (_ota_handle) {
    esp_ota_abort(_ota_handle);
    _ota_handle = 0;
  }
  strncpy(_err_msg, "OTA finalization failed", sizeof(_err_msg) - 1);
  _status = OTA_SD_FAILED;
}

bool OtaManager::startHttpServer() {
  if (_http_server) return true;

  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 8;
  config.stack_size = 8192;

  httpd_uri_t uri_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = index_get_handler,
    .user_ctx = NULL
  };

  httpd_uri_t uri_post = {
    .uri = "/update",
    .method = HTTP_POST,
    .handler = update_post_handler,
    .user_ctx = NULL
  };

  if (httpd_start(&_http_server, &config) == ESP_OK) {
    httpd_register_uri_handler(_http_server, &uri_get);
    httpd_register_uri_handler(_http_server, &uri_post);
    ESP_LOGI(TAG, "Maintenance HTTP OTA Server listening on http://%s:80", _ip_addr);
    return true;
  }
  return false;
}

bool OtaManager::startMaintenanceMode(const char *ssid, const char *pass) {
  if (_maintenance_active) return true;

  if (ssid && strlen(ssid) > 0) {
    strncpy(_active_ssid, ssid, sizeof(_active_ssid) - 1);
  }
  if (pass) {
    strncpy(_active_pass, pass, sizeof(_active_pass) - 1);
  }

  ESP_LOGI(TAG, "Entering Maintenance Mode: connecting to Wi-Fi SSID '%s'...", _active_ssid);

  // Deinit ESP-NOW to give full radio control to Wi-Fi STA
  esp_now_unregister_recv_cb();
  esp_now_deinit();
  esp_wifi_stop();

  if (!s_sta_netif) {
    s_sta_netif = esp_netif_create_default_wifi_sta();
  }

  s_retry_count = 0;
  if (!s_wifi_event_handler) {
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_sta_event_handler, NULL, &s_wifi_event_handler);
  }
  if (!s_ip_event_handler) {
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_sta_event_handler, NULL, &s_ip_event_handler);
  }

  wifi_config_t sta_config = {};
  strncpy((char*)sta_config.sta.ssid, _active_ssid, sizeof(sta_config.sta.ssid) - 1);
  strncpy((char*)sta_config.sta.password, _active_pass, sizeof(sta_config.sta.password) - 1);
  sta_config.sta.threshold.authmode = (strlen(_active_pass) > 0) ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;

  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_config(WIFI_IF_STA, &sta_config);
  esp_wifi_start();

  _maintenance_active = true;
  _status = MAINTENANCE_CONNECTING;
  _progress_pct = 0;
  _err_msg[0] = '\0';
  _ip_addr[0] = '\0';

  return true;
}

void OtaManager::stopMaintenanceMode() {
  if (_http_server) {
    httpd_stop(_http_server);
    _http_server = nullptr;
  }

  if (s_wifi_event_handler) {
    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, s_wifi_event_handler);
    s_wifi_event_handler = nullptr;
  }
  if (s_ip_event_handler) {
    esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, s_ip_event_handler);
    s_ip_event_handler = nullptr;
  }

  if (_maintenance_active) {
    _maintenance_active = false;
    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();

    // Re-register ESP-NOW callback and broadcast peer
    esp_now_init();
    esp_now_register_recv_cb(EspNowReceiver::onDataRecv);
    esp_now_peer_info_t peerInfo = {};
    memset(peerInfo.peer_addr, 0xFF, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
    ESP_LOGI(TAG, "Exited Maintenance Mode. Restored ESP-NOW Telemetry link.");
  }
  _status = OTA_IDLE;
}

void OtaManager::onWiFiConnected(const char *ip) {
  strncpy(_ip_addr, ip, sizeof(_ip_addr) - 1);
  ESP_LOGI(TAG, "Connected to Wi-Fi SSID '%s'! Assigned IP: %s", _active_ssid, _ip_addr);
  if (startHttpServer()) {
    _status = MAINTENANCE_READY;
    _progress_pct = 0;
    _err_msg[0] = '\0';
  } else {
    strncpy(_err_msg, "Failed to start HTTP server", sizeof(_err_msg) - 1);
    _status = MAINTENANCE_FAILED;
  }
}

void OtaManager::onWiFiConnectFailed(const char *reason) {
  ESP_LOGE(TAG, "Wi-Fi connection failed: %s", reason);
  snprintf(_err_msg, sizeof(_err_msg), "Could not connect to '%s'", _active_ssid);
  _status = MAINTENANCE_FAILED;
}

esp_err_t OtaManager::onHttpUploadChunk(const uint8_t *data, size_t len, size_t total_len) {
  if (_status == MAINTENANCE_READY || _status == MAINTENANCE_CONNECTING) {
    _status = MAINTENANCE_RECEIVING;
    _binary_size = total_len;
    _bytes_written = 0;
    _progress_pct = 0;

    _update_partition = esp_ota_get_next_update_partition(NULL);
    if (!_update_partition) {
      strncpy(_err_msg, "No available OTA partition", sizeof(_err_msg) - 1);
      return ESP_FAIL;
    }

    esp_err_t err = esp_ota_begin(_update_partition, total_len, &_ota_handle);
    if (err != ESP_OK) {
      snprintf(_err_msg, sizeof(_err_msg), "OTA begin err: %s", esp_err_to_name(err));
      return err;
    }
  }

  esp_err_t err = esp_ota_write(_ota_handle, data, len);
  if (err != ESP_OK) {
    snprintf(_err_msg, sizeof(_err_msg), "OTA write err: %s", esp_err_to_name(err));
    return err;
  }

  _bytes_written += len;
  if (_binary_size > 0) {
    _progress_pct = (uint8_t)((_bytes_written * 100) / _binary_size);
  }
  return ESP_OK;
}

void OtaManager::onHttpUploadFinish(bool success, const char *err_msg) {
  if (success) {
    if (esp_ota_end(_ota_handle) == ESP_OK && esp_ota_set_boot_partition(_update_partition) == ESP_OK) {
      _status = MAINTENANCE_COMPLETED;
      _progress_pct = 100;
      _ota_handle = 0;
      ESP_LOGI(TAG, "Maintenance OTA update successful! Reboot scheduled.");
      return;
    }
  }

  if (_ota_handle) {
    esp_ota_abort(_ota_handle);
    _ota_handle = 0;
  }

  _status = MAINTENANCE_FAILED;
  if (err_msg) {
    strncpy(_err_msg, err_msg, sizeof(_err_msg) - 1);
  } else {
    strncpy(_err_msg, "Upload failed or corrupted", sizeof(_err_msg) - 1);
  }
}
