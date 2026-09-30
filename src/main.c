#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "apex_tasks.h"
#include "apex_utils.h"
#include "apex_comms.h"

void app_main(void)
{   
    xTaskCreate(wifi_setup_task, "wifi_setup", 4096, NULL, 6, NULL);
    xTaskCreate(gps_setup_task, "gps_setup", 4096, NULL, 5, NULL);
}