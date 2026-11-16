#ifndef APEX_LOGGER_H
#define APEX_LOGGER_H

#include "apex_imu.h"
#include "apex_tasks.h"
#include "freertos/FreeRTOS.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define LOGGER_FILE_NAME_MAX 32

typedef struct {
    char name[LOGGER_FILE_NAME_MAX];
    size_t size;
    bool current;
} logger_file_info_t;

#ifdef __cplusplus
extern "C" {
#endif

void logger_setup_task(void *pvParameters);
bool logger_log_gps(const gps_data_t *data);
bool logger_log_imu(const imu_data_t *data, const imu_orientation_t *orientation,
                    bool fsync_asserted);
bool logger_is_ready(void);
bool logger_list_files(logger_file_info_t *files, size_t capacity, size_t offset, size_t *count,
                       size_t *total_count);
bool logger_archive_current(char *archive_name, size_t archive_name_capacity);
bool logger_get_file_size(const char *name, size_t *size);
bool logger_read_file(const char *name, size_t offset, void *buffer, size_t capacity,
                      size_t *bytes_read);

#ifdef __cplusplus
}
#endif

#endif
