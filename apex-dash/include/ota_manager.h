#pragma once

#include <cstdint>
#include <cstddef>
#include "esp_ota_ops.h"
#include "esp_http_server.h"

enum OtaStatus : uint8_t {
  OTA_IDLE = 0,
  OTA_SD_IN_PROGRESS,
  OTA_SD_COMPLETED,
  OTA_SD_FAILED,
  MAINTENANCE_CONNECTING,
  MAINTENANCE_READY,
  MAINTENANCE_RECEIVING,
  MAINTENANCE_COMPLETED,
  MAINTENANCE_FAILED,

  // Compatibility aliases
  OTA_WIFI_PORTAL_ACTIVE = MAINTENANCE_READY,
  OTA_WIFI_RECEIVING     = MAINTENANCE_RECEIVING,
  OTA_WIFI_COMPLETED     = MAINTENANCE_COMPLETED,
  OTA_WIFI_FAILED        = MAINTENANCE_FAILED
};

class SDManager;

class OtaManager {
public:
  static OtaManager& instance();

  void begin(SDManager *sdMgr);

  // MicroSD Offline OTA
  bool checkSdFirmwareAvailable(char *out_filename = nullptr, size_t max_len = 0);
  bool startSdUpdate(const char *filename = nullptr);

  // Wi-Fi Maintenance Mode OTA (STA Mode)
  bool startMaintenanceMode(const char *ssid = nullptr, const char *pass = nullptr);
  void stopMaintenanceMode();

  // Backward compatibility forwarders
  bool startWiFiPortal() { return startMaintenanceMode(); }
  void stopWiFiPortal() { stopMaintenanceMode(); }

  // Status & Progress
  OtaStatus getStatus() const { return _status; }
  uint8_t getProgress() const { return _progress_pct; }
  const char* getErrorMessage() const { return _err_msg; }
  const char* getActiveFilename() const { return _active_file; }
  const char* getConnectedIP() const { return _ip_addr; }
  const char* getTargetSSID() const { return _active_ssid; }
  bool isBusy() const;

  // Cancel running operation / exit maintenance mode
  void cancel();

  // MicroSD background worker
  void executeSdUpdateWorker();

  // Wi-Fi / IP internal event callbacks
  void onWiFiConnected(const char *ip);
  void onWiFiConnectFailed(const char *reason);

  // Internal HTTP request handlers
  esp_err_t onHttpUploadChunk(const uint8_t *data, size_t len, size_t total_len);
  void onHttpUploadFinish(bool success, const char *err_msg = nullptr);

private:
  OtaManager();
  ~OtaManager();

  bool startHttpServer();
  void loadWiFiCredentialsFromSD();

  SDManager *_sd_mgr = nullptr;
  OtaStatus _status = OTA_IDLE;
  uint8_t _progress_pct = 0;
  char _err_msg[128] = {0};
  char _active_file[64] = {0};
  char _active_ssid[33] = {0};
  char _active_pass[65] = {0};
  char _ip_addr[32] = {0};

  // Flash OTA handle
  esp_ota_handle_t _ota_handle = 0;
  const esp_partition_t *_update_partition = nullptr;
  size_t _binary_size = 0;
  size_t _bytes_written = 0;

  // HTTP server handle
  httpd_handle_t _http_server = nullptr;
  bool _maintenance_active = false;
};
