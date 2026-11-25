#include "apex_tasks.h"
#include "apex_utils.h"
#include "apex_logger.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

QueueHandle_t gps_data_queue;
static gps_data_t latest_gps_data;
static bool latest_gps_data_valid;
static portMUX_TYPE gps_data_lock = portMUX_INITIALIZER_UNLOCKED;

bool gps_get_latest_data(gps_data_t *gps_data)
{
    bool valid;
    portENTER_CRITICAL(&gps_data_lock);
    valid = latest_gps_data_valid;
    if (valid && gps_data != NULL) {
        *gps_data = latest_gps_data;
    }
    portEXIT_CRITICAL(&gps_data_lock);
    return valid;
}

bool gps_get_time_reference(uint32_t *utc_time_ms, int64_t *local_time_us)
{
    bool valid;
    portENTER_CRITICAL(&gps_data_lock);
    valid = latest_gps_data_valid;
    if (valid) {
        *utc_time_ms = latest_gps_data.utc_time_ms;
        *local_time_us = latest_gps_data.local_time_us;
    }
    portEXIT_CRITICAL(&gps_data_lock);
    return valid;
}

void ntrip_task(void *pvParameters)
{
    (void)pvParameters;

    int socket_fd = -1;
    gps_data_t latest_gps_data = {0};
    bool have_gps_data = false;
    TickType_t last_gga_tick = 0;
    uint8_t correction_data[NTRIP_RX_BUFFER_SIZE];

    while (true) {
        ntrip_config_t config;
        ntrip_get_config(&config);
        if (socket_fd < 0) {
            socket_fd = ntrip_connect();
            if (socket_fd < 0) {
                vTaskDelay(pdMS_TO_TICKS(5000));
                continue;
            }
            last_gga_tick = xTaskGetTickCount() - pdMS_TO_TICKS(config.gga_interval_ms);
        }

        gps_data_t queued_gps_data;
        if (xQueueReceive(gps_data_queue, &queued_gps_data, pdMS_TO_TICKS(50)) == pdPASS) {
            latest_gps_data = queued_gps_data;
            have_gps_data = true;
        }

        TickType_t now = xTaskGetTickCount();
        if (have_gps_data && now - last_gga_tick >= pdMS_TO_TICKS(config.gga_interval_ms)) {
            bool sent = ntrip_send_all(
                socket_fd,
                latest_gps_data.nmea_sentence,
                strlen(latest_gps_data.nmea_sentence));
            sent = sent && ntrip_send_all(socket_fd, "\r\n", 2);
            if (!sent) {
                ESP_LOGW("ntrip", "Failed to send GGA position");
                close(socket_fd);
                socket_fd = -1;
                continue;
            }
            last_gga_tick = now;
        }

        int bytes_read = recv(socket_fd, correction_data, sizeof(correction_data), MSG_DONTWAIT);
        if (bytes_read > 0) {
            if (uart_write_bytes(GPS_UART_NUM, (const char *)correction_data, bytes_read) < 0) {
                ESP_LOGW("ntrip", "Failed to forward RTCM corrections to GPS");
            }
        } else if (bytes_read == 0) {
            ESP_LOGW("ntrip", "Caster closed the connection");
            close(socket_fd);
            socket_fd = -1;
        }
    }
}

void gps_setup_task(void *pvParameters)
{
    (void)pvParameters;

    const uart_config_t uart_config = {
        .baud_rate = GPS_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    ESP_ERROR_CHECK(uart_param_config(GPS_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(
        GPS_UART_NUM,
        GPS_UART_TX_PIN,
        GPS_UART_RX_PIN,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(
        GPS_UART_NUM,
        GPS_UART_RX_BUFFER_SIZE,
        0,
        0,
        NULL,
        0));

    gps_data_queue = xQueueCreate(GPS_DATA_QUEUE_LENGTH, sizeof(gps_data_t));
    if (gps_data_queue == NULL) {
        ESP_LOGE("gps_setup", "Failed to create GPS data queue");
        vTaskDelete(NULL);
    }

    static const char *commands[] = {
        "$PQTMGNSSSTOP",
        "$PQTMCFGFIXRATE,W,50",
        "$PQTMCFGRCVRMODE,W,1",
        // Disable every output message on this UART, then enable only PQTMNAV (version 1)
        // once per fix: it carries position, speed and COG in a single sentence.
        "$PQTMCLRMSG",
        "$PQTMCFGMSGRATE,W,PQTMNAV,1,1",
        // PPS1 -> ICM-20948 FSYNC: 100 ms active-high pulse at each UTC second, only while
        // the receiver has a fix, so every edge the IMU sees is aligned to UTC.
        "$PQTMCFGPPS,W,1,1,100,2,1,0",
        "$PQTMCFGUART,W,460800",
        "$PQTMSAVEPAR",
    };

    for (size_t index = 0; index < sizeof(commands) / sizeof(commands[0]); index++) {
        esp_err_t error = gps_send_command(commands[index], true);
        if (error != ESP_OK) {
            ESP_LOGE("gps_setup", "Failed to send GPS command %s: %s", commands[index], esp_err_to_name(error));
            vTaskDelete(NULL);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }

    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_ERROR_CHECK(gps_send_command("PQTMGNSSSTART", false));

    xTaskCreate(gps_monitor_task, "gps_monitor", 4096, NULL, 6, NULL);
    xTaskCreate(ntrip_task, "ntrip", 8192, NULL, 7, NULL);
    vTaskDelete(NULL);
}

static void report_gps_status(TickType_t *report_tick, uint32_t *bytes_received,
                              uint32_t *sentences_received)
{
    TickType_t now = xTaskGetTickCount();
    if (now - *report_tick < pdMS_TO_TICKS(1000)) {
        return;
    }

    gps_data_t status_data = {0};
    bool status_valid = gps_get_latest_data(&status_data);
    ESP_LOGI("gps_monitor",
             "GPS stream: %lu bytes/s, %lu NMEA sentences/s; valid=%u,mode=%u,"
             "timestamp_ms=%lu,latitude=%.7f,longitude=%.7f,speed_kmh=%.2f,cog_deg=%.2f,rtk_age_s=%.2f",
             (unsigned long)*bytes_received,
             (unsigned long)*sentences_received,
             status_valid ? 1U : 0U,
             (unsigned int)status_data.gps_mode,
             (unsigned long)status_data.utc_time_ms,
             status_data.latitude,
             status_data.longitude,
             status_data.speed_kmh,
             status_data.cog_deg,
             status_data.correction_age_s);
    *bytes_received = 0;
    *sentences_received = 0;
    *report_tick = now;
}

void gps_monitor_task(void *pvParameters)
{
    (void)pvParameters;

    char nmea_message[GPS_COMMAND_RESPONSE_BUFFER_SIZE];
    size_t message_length = 0;
    uint8_t satellite_snr[10] = {0};
    uint8_t satellite_snr_count = 0;
    uint8_t gsv_message_number = 0;
    uint32_t bytes_received = 0;
    uint32_t sentences_received = 0;
    TickType_t report_tick = xTaskGetTickCount();

    while (true) {
        uint8_t data[GPS_COMMAND_RESPONSE_BUFFER_SIZE];
        int bytes_read = uart_read_bytes(
            GPS_UART_NUM,
            data,
            sizeof(data),
            // Short timeout: return partial data quickly so sentence arrival times
            // (local_time_us) are not delayed by waiting for a full buffer.
            pdMS_TO_TICKS(10));

        if (bytes_read <= 0) {
            report_gps_status(&report_tick, &bytes_received, &sentences_received);
            continue;
        }

        bytes_received += (uint32_t)bytes_read;
        for (int index = 0; index < bytes_read; index++) {
            char character = (char)data[index];

            if (character == '\n') {
                nmea_message[message_length] = '\0';
                if (message_length > 0) {
                    sentences_received++;

                    // The parsers split the buffer in place, so pick the parser by
                    // sentence name before handing the buffer over.
                    if (strncmp(nmea_message, "$PQTMNAV,", 9) == 0) {
                        gps_data_t gps_data = {0};
                        gps_data.local_time_us = esp_timer_get_time();
                        // Returns false before the first fix (SolType 0) or on a malformed sentence.
                        if (gps_parse_pqtmnav_message(nmea_message, &gps_data)) {
                            gps_data.satellite_snr_count = satellite_snr_count;
                            memcpy(gps_data.satellite_snr, satellite_snr, sizeof(satellite_snr));
                            xQueueOverwrite(gps_data_queue, &gps_data);
                            portENTER_CRITICAL(&gps_data_lock);
                            latest_gps_data = gps_data;
                            latest_gps_data_valid = true;
                            portEXIT_CRITICAL(&gps_data_lock);
                            logger_log_gps(&gps_data);
                        }
                    } else {
                        // GSV is disabled by the setup commands; this only fills the
                        // satellite SNR CAN frame if GSV output is re-enabled.
                        char *fields[32] = {0};
                        size_t field_count = 0;
                        for (char *field = nmea_message;
                             field != NULL && field_count < 32;) {
                            fields[field_count++] = field;
                            char *separator = strchr(field, ',');
                            if (separator == NULL) {
                                break;
                            }
                            *separator = '\0';
                            field = separator + 1;
                        }

                        if (field_count >= 4 && strlen(fields[0]) >= 6 &&
                            strcmp(fields[0] + 3, "GSV") == 0) {
                            uint8_t total_messages = (uint8_t)strtoul(fields[1], NULL, 10);
                            uint8_t message_number = (uint8_t)strtoul(fields[2], NULL, 10);
                            uint8_t total_satellites = (uint8_t)strtoul(fields[3], NULL, 10);
                            if (total_messages > 0 && message_number > 0 &&
                                message_number <= total_messages) {
                                if (message_number == 1) {
                                    memset(satellite_snr, 0, sizeof(satellite_snr));
                                    satellite_snr_count = 0;
                                }
                                if (message_number == gsv_message_number + 1 ||
                                    message_number == 1) {
                                    for (size_t field_index = 7;
                                         field_index + 3 < field_count;
                                         field_index += 4) {
                                        if (satellite_snr_count >= sizeof(satellite_snr)) {
                                            break;
                                        }
                                        if (fields[field_index + 3][0] != '\0') {
                                            satellite_snr[satellite_snr_count++] =
                                                (uint8_t)strtoul(fields[field_index + 3], NULL, 10);
                                        }
                                    }
                                    gsv_message_number = message_number;
                                    if (message_number == total_messages &&
                                        total_satellites < satellite_snr_count) {
                                        satellite_snr_count = total_satellites;
                                    }
                                }
                            }
                        }
                    }
                }
                message_length = 0;
            } else if (character != '\r') {
                if (message_length < sizeof(nmea_message) - 1) {
                    nmea_message[message_length++] = character;
                } else {
                    ESP_LOGW("gps_monitor", "Discarding oversized NMEA message.");
                    message_length = 0;
                }
            }
        }

        report_gps_status(&report_tick, &bytes_received, &sentences_received);
    }
}
