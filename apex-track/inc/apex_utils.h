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

#define NTRIP_HOST "crtk.net"
#define NTRIP_PORT 2101
#define NTRIP_MOUNTPOINT "JBCH"
#define NTRIP_USERNAME "c"
#define NTRIP_PASSWORD "c"
#define NTRIP_CLIENT_NAME "c"
#define NTRIP_GGA_INTERVAL_MS 1000
#define NTRIP_RX_BUFFER_SIZE 1024

typedef struct {
	char host[64];
	uint16_t port;
	char mountpoint[64];
	char username[64];
	char password[64];
	char client_name[64];
	uint32_t gga_interval_ms;
} ntrip_config_t;

esp_err_t gps_send_command(const char *command, bool wait_for_response);
esp_err_t gps_wait_for_response(const char *command);
bool gps_parse_gga_message(char *message, gps_data_t *gps_data);
bool gps_parse_rmc_message(char *message, float *speed_kmh);
bool ntrip_send_all(int socket, const char *data, size_t length);
int ntrip_connect(void);
void ntrip_get_config(ntrip_config_t *config);
esp_err_t ntrip_set_config(const ntrip_config_t *config);
esp_err_t ntrip_save_config(const ntrip_config_t *config);
esp_err_t ntrip_load_config(void);

#endif
