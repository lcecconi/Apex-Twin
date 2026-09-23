#include "apex_tasks.h"
#include "apex_utils.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

QueueHandle_t gps_data_queue;

void ntrip_task(void *pvParameters)
{
    (void)pvParameters;

    int socket_fd = -1;
    gps_data_t latest_gps_data = {0};
    bool have_gps_data = false;
    TickType_t last_gga_tick = 0;
    uint8_t correction_data[NTRIP_RX_BUFFER_SIZE];

    while (true) {
        if (socket_fd < 0) {
            socket_fd = ntrip_connect();
            if (socket_fd < 0) {
                vTaskDelay(pdMS_TO_TICKS(5000));
                continue;
            }
            last_gga_tick = xTaskGetTickCount() - pdMS_TO_TICKS(NTRIP_GGA_INTERVAL_MS);
        }

        gps_data_t queued_gps_data;
        if (xQueueReceive(gps_data_queue, &queued_gps_data, pdMS_TO_TICKS(50)) == pdPASS) {
            latest_gps_data = queued_gps_data;
            have_gps_data = true;
        }

        TickType_t now = xTaskGetTickCount();
        if (have_gps_data && now - last_gga_tick >= pdMS_TO_TICKS(NTRIP_GGA_INTERVAL_MS)) {
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

void dummy_task(void *pvParameters)
{
    (void)pvParameters;

    while (true) {
        ESP_LOGI("dummy_task", "Dummy task is running...");
        vTaskDelay(pdMS_TO_TICKS(1000));
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

    ESP_ERROR_CHECK(wifi_start());

    static const char *commands[] = {
        "$PQTMGNSSSTOP",
        "$PQTMCFGFIXRATE,W,50",
        "$PQTMCFGRCVRMODE,W,1",
        "$PQTMCFGMSGRATE,W,GGA,1",
        "$PQTMCFGMSGRATE,W,GSV,0",
        "$PQTMCFGMSGRATE,W,VTG,0",
        "$PQTMCFGMSGRATE,W,GLL,0",
        "$PQTMCFGMSGRATE,W,GSA,0",
        "$PQTMCFGMSGRATE,W,RMC,0",
        "$PQTMCFGUART,W,460800",
        "$PQTMSAVEPAR",
    };

    for (size_t index = 0; index < sizeof(commands) / sizeof(commands[0]); index++) {
        esp_err_t error = gps_send_command(commands[index], true);
        if (error != ESP_OK) {
            ESP_LOGE("gps_setup", "Failed to send GPS command %s: %s", commands[index], esp_err_to_name(error));
            vTaskDelete(NULL);
        } else {
            ESP_LOGI("gps_setup", "Sent GPS command: %s", commands[index]);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }

    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_ERROR_CHECK(gps_send_command("PQTMGNSSSTART", false));

    xTaskCreate(dummy_task, "dummy_task", 2048, NULL, 5, NULL);
    xTaskCreate(gps_monitor_task, "gps_monitor", 4096, NULL, 6, NULL);
    xTaskCreate(ntrip_task, "ntrip", 4096, NULL, 7, NULL);
    vTaskDelete(NULL);
}

void gps_monitor_task(void *pvParameters)
{
    (void)pvParameters;

    char nmea_message[GPS_COMMAND_RESPONSE_BUFFER_SIZE];
    size_t message_length = 0;

    while (true) {
        uint8_t data[GPS_COMMAND_RESPONSE_BUFFER_SIZE];
        int bytes_read = uart_read_bytes(
            GPS_UART_NUM,
            data,
            sizeof(data),
            pdMS_TO_TICKS(1000));

        if (bytes_read <= 0) {
            ESP_LOGW("gps_monitor", "No GPS data received in the last second.");
            continue;
        }

        for (int index = 0; index < bytes_read; index++) {
            char character = (char)data[index];

            if (character == '\n') {
                nmea_message[message_length] = '\0';
                if (message_length > 0) {
                    char raw_nmea_message[GPS_COMMAND_RESPONSE_BUFFER_SIZE];
                    memcpy(raw_nmea_message, nmea_message, message_length + 1);

                    gps_data_t gps_data;
                    if (gps_parse_gga_message(nmea_message, &gps_data)) {
                        strncpy(gps_data.nmea_sentence, raw_nmea_message, sizeof(gps_data.nmea_sentence) - 1);
                        gps_data.nmea_sentence[sizeof(gps_data.nmea_sentence) - 1] = '\0';
#if APX_DEBUG > 0
                        ESP_LOGI(
                            "gps_monitor",
                            "Queue GPS data: time=%lu ms lat=%.6f lon=%.6f alt=%.3f m mode=%u correction_age=%.1f s",
                            (unsigned long)gps_data.utc_time_ms,
                            gps_data.latitude,
                            gps_data.longitude,
                            gps_data.altitude_m,
                            (unsigned int)gps_data.gps_mode,
                            gps_data.correction_age_s);
#endif
#if APX_DEBUG > 1
                        ESP_LOGI("gps_monitor", "NMEA: %s", gps_data.nmea_sentence);
#endif
                        xQueueOverwrite(gps_data_queue, &gps_data);
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
    }
}
