#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "apex_tasks.h"
#include "apex_utils.h"
#include "apex_comms.h"
#include "apex_logger.h"
#include "esp_log.h"

void app_main(void)
{   
    ESP_LOGI("app", "Starting Apex-Track tasks");
    BaseType_t result = xTaskCreate(logger_setup_task, "logger_setup", 4096, NULL, 5, NULL);
    if (result != pdPASS) {
        ESP_LOGE("app", "Failed to create logger task");
    }
    result = xTaskCreate(wifi_setup_task, "wifi_setup", 4096, NULL, 6, NULL);
    if (result != pdPASS) {
        ESP_LOGE("app", "Failed to create Wi-Fi task");
    }
    result = xTaskCreate(imu_setup_task, "imu_setup", 4096, NULL, 5, NULL);
    if (result != pdPASS) {
        ESP_LOGE("app", "Failed to create IMU task");
    }
    result = xTaskCreate(gps_setup_task, "gps_setup", 4096, NULL, 5, NULL);
    if (result != pdPASS) {
        ESP_LOGE("app", "Failed to create GPS task");
    }
}