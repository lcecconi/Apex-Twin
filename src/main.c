#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "apex_tasks.h"

void app_main(void)
{
    xTaskCreate(gps_setup_task, "gps_setup", 4096, NULL, 5, NULL);
}