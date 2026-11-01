#ifndef APEX_LOGGER_H
#define APEX_LOGGER_H

#include "apex_imu.h"
#include "apex_tasks.h"
#include "freertos/FreeRTOS.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void logger_setup_task(void *pvParameters);
bool logger_log_gps(const gps_data_t *data);
bool logger_log_imu(const imu_data_t *data, const imu_orientation_t *orientation,
                    bool fsync_asserted);

#ifdef __cplusplus
}
#endif

#endif
