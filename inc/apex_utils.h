#ifndef APEX_UTILS_H
#define APEX_UTILS_H

#include "apex_tasks.h"
#include "driver/uart.h"
#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define GPS_UART_NUM UART_NUM_1
#define GPS_UART_TX_PIN 2
#define GPS_UART_RX_PIN 1
#define GPS_UART_BAUD_RATE 460800
#define GPS_UART_RX_BUFFER_SIZE 2048
#define GPS_COMMAND_RESPONSE_TIMEOUT_MS 500
#define GPS_COMMAND_RESPONSE_BUFFER_SIZE 256
#define GPS_DATA_QUEUE_LENGTH 1

#define WIFI_SSID "Formula WAN"
#define WIFI_PASSWORD "Framba2025"

#define NTRIP_HOST "crtk.net"
#define NTRIP_PORT 2101
#define NTRIP_MOUNTPOINT "JBCH"
#define NTRIP_USERNAME "c"
#define NTRIP_PASSWORD "c"
#define NTRIP_CLIENT_NAME "c"
#define NTRIP_GGA_INTERVAL_MS 1000
#define NTRIP_RX_BUFFER_SIZE 1024

esp_err_t wifi_start(void);
esp_err_t gps_send_command(const char *command, bool wait_for_response);
esp_err_t gps_wait_for_response(const char *command);
bool gps_parse_gga_message(char *message, gps_data_t *gps_data);
bool ntrip_send_all(int socket, const char *data, size_t length);
int ntrip_connect(void);

#endif
