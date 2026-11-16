#include "apex_logger.h"
#include "apex_utils.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/spi_master.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

namespace {
constexpr char kMountPoint[] = "/sdcard";
constexpr char kLogPath[] = "/sdcard/measurements.csv";
constexpr size_t kQueueLength = 64;

enum class MeasurementType : uint8_t {
    GPS,
    IMU,
};

struct Measurement {
    MeasurementType type;
    gps_data_t gps;
    imu_data_t imu;
    imu_orientation_t orientation;
    bool fsync_asserted;
};

QueueHandle_t measurement_queue;
FILE *measurement_file;
SemaphoreHandle_t storage_mutex;
bool storage_ready;
uint32_t dropped_measurements;
bool first_measurement_received;

bool write_header()
{
    if (fprintf(measurement_file,
        "timestamp_ms,source,utc_time_ms,latitude,longitude,altitude_m,hdop,satellites,gps_mode,"
        "correction_age_s,accel_x_g,accel_y_g,accel_z_g,gyro_x_dps,gyro_y_dps,"
        "gyro_z_dps,mag_x_ut,mag_y_ut,mag_z_ut,temp_c,pitch_deg,roll_deg,yaw_deg,fsync\n") < 0) {
        return false;
    }
    return fflush(measurement_file) == 0 && ferror(measurement_file) == 0;
}

bool write_gps(const gps_data_t &data)
{
    return fprintf(measurement_file,
        "%lu,GPS,%lu,%.9f,%.9f,%.3f,%.3f,%u,%u,%.3f,,,,,,,,,,,,,,\n",
        (unsigned long)data.utc_time_ms,
        (unsigned long)data.utc_time_ms,
        data.latitude,
        data.longitude,
        data.altitude_m,
        data.hdop,
        data.satellites,
        data.gps_mode,
        data.correction_age_s) >= 0;
}

bool write_imu(const Measurement &measurement)
{
    const imu_data_t &data = measurement.imu;
    const imu_orientation_t &orientation = measurement.orientation;
    return fprintf(measurement_file,
        "%lu,IMU,,,,,,,,,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,"
        "%.6f,%.3f,%.3f,%.3f,%u\n",
        (unsigned long)data.timestamp_ms,
        data.accel_x_g,
        data.accel_y_g,
        data.accel_z_g,
        data.gyro_x_dps,
        data.gyro_y_dps,
        data.gyro_z_dps,
        data.mag_x_ut,
        data.mag_y_ut,
        data.mag_z_ut,
        data.temp_c,
        orientation.pitch_deg,
        orientation.roll_deg,
        orientation.yaw_deg,
        measurement.fsync_asserted ? 1U : 0U) >= 0;
}

bool valid_log_file_name(const char *name)
{
    if (name == nullptr) {
        return false;
    }
    if (strcmp(name, "measurements.csv") == 0) {
        return true;
    }
    if (strlen(name) != 21 || strncmp(name, "measurements_", 13) != 0 ||
        strcmp(name + 17, ".csv") != 0) {
        return false;
    }
    for (size_t index = 13; index < 17; index++) {
        if (name[index] < '0' || name[index] > '9') {
            return false;
        }
    }
    return true;
}

bool is_archived_log_file_name(const char *name)
{
    return valid_log_file_name(name) && strcmp(name, "measurements.csv") != 0;
}

int compare_log_files(const void *left, const void *right)
{
    const logger_file_info_t *left_file = static_cast<const logger_file_info_t *>(left);
    const logger_file_info_t *right_file = static_cast<const logger_file_info_t *>(right);
    if (left_file->current != right_file->current) {
        return left_file->current ? -1 : 1;
    }
    return strcmp(left_file->name, right_file->name);
}
}

extern "C" bool logger_log_gps(const gps_data_t *data)
{
    if (data == nullptr || measurement_queue == nullptr) {
        return false;
    }
    ESP_LOGI("logger", "Logging GPS data: utc_time_ms=%lu, lat=%.9f, lon=%.9f, alt=%.3f, hdop=%.3f, satellites=%u, gps_mode=%u, correction_age_s=%.3f",
             (unsigned long)data->utc_time_ms,
             data->latitude,
             data->longitude,
             data->altitude_m,
             data->hdop,
             (unsigned int)data->satellites,
             (unsigned int)data->gps_mode,
             data->correction_age_s);

    Measurement measurement = {};
    measurement.type = MeasurementType::GPS;
    measurement.gps = *data;
    if (xQueueSend(measurement_queue, &measurement, 0) != pdPASS) {
        dropped_measurements++;
        if (dropped_measurements % 100 == 1) {
            ESP_LOGW("logger", "Measurement queue full; dropped %lu rows",
                     (unsigned long)dropped_measurements);
        }
        return false;
    }
    return true;
}

extern "C" bool logger_log_imu(const imu_data_t *data,
                                const imu_orientation_t *orientation,
                                bool fsync_asserted)
{
    if (data == nullptr || orientation == nullptr || measurement_queue == nullptr) {
        return false;
    }

    Measurement measurement = {};
    measurement.type = MeasurementType::IMU;
    measurement.imu = *data;
    measurement.orientation = *orientation;
    measurement.fsync_asserted = fsync_asserted;
    if (xQueueSend(measurement_queue, &measurement, 0) != pdPASS) {
        dropped_measurements++;
        if (dropped_measurements % 100 == 1) {
            ESP_LOGW("logger", "Measurement queue full; dropped %lu rows",
                     (unsigned long)dropped_measurements);
        }
        return false;
    }
    return true;
}

extern "C" bool logger_is_ready(void)
{
    if (storage_mutex == nullptr || xSemaphoreTake(storage_mutex, portMAX_DELAY) != pdTRUE) {
        return false;
    }
    bool ready = storage_ready;
    xSemaphoreGive(storage_mutex);
    return ready;
}

extern "C" bool logger_list_files(logger_file_info_t *files, size_t capacity, size_t offset,
                                   size_t *count, size_t *total_count)
{
    if (files == nullptr || count == nullptr || total_count == nullptr ||
        storage_mutex == nullptr ||
        xSemaphoreTake(storage_mutex, portMAX_DELAY) != pdTRUE) {
        return false;
    }

    *count = 0;
    *total_count = 0;
    bool success = storage_ready;
    DIR *directory = success ? opendir(kMountPoint) : nullptr;
    if (success && directory == nullptr) {
        success = false;
    }
    struct dirent *entry;
    while (success && (entry = readdir(directory)) != nullptr) {
        bool is_current = strcmp(entry->d_name, "measurements.csv") == 0;
        if (!is_current && !is_archived_log_file_name(entry->d_name)) {
            continue;
        }
        size_t file_index = (*total_count)++;
        if (file_index < offset || *count == capacity) {
            continue;
        }

        char path[sizeof(kMountPoint) + LOGGER_FILE_NAME_MAX] = {0};
        size_t mount_point_length = strlen(kMountPoint);
        size_t file_name_length = strlen(entry->d_name);
        memcpy(path, kMountPoint, mount_point_length);
        path[mount_point_length] = '/';
        memcpy(path + mount_point_length + 1, entry->d_name, file_name_length + 1);
        struct stat file_status;
        if (stat(path, &file_status) != 0 || !S_ISREG(file_status.st_mode)) {
            success = false;
            break;
        }
        logger_file_info_t *file = &files[(*count)++];
        strncpy(file->name, entry->d_name, sizeof(file->name) - 1);
        file->name[sizeof(file->name) - 1] = '\0';
        file->size = (size_t)file_status.st_size;
        file->current = is_current;
    }
    if (directory != nullptr) {
        closedir(directory);
    }
    if (success) {
        qsort(files, *count, sizeof(files[0]), compare_log_files);
    }
    xSemaphoreGive(storage_mutex);
    return success;
}

extern "C" bool logger_archive_current(char *archive_name, size_t archive_name_capacity)
{
    if (archive_name == nullptr || archive_name_capacity < LOGGER_FILE_NAME_MAX ||
        storage_mutex == nullptr ||
        xSemaphoreTake(storage_mutex, portMAX_DELAY) != pdTRUE) {
        return false;
    }

    bool success = storage_ready && measurement_file != nullptr;
    bool reopen_active_file = false;
    char candidate[LOGGER_FILE_NAME_MAX] = {0};
    struct stat file_status;
    for (unsigned int index = 1; success && index <= 9999; index++) {
        snprintf(candidate, sizeof(candidate), "measurements_%04u.csv", index);
        char candidate_path[sizeof(kMountPoint) + LOGGER_FILE_NAME_MAX] = {0};
        snprintf(candidate_path, sizeof(candidate_path), "%s/%s", kMountPoint, candidate);
        if (stat(candidate_path, &file_status) != 0) {
            if (errno == ENOENT) {
                break;
            }
            success = false;
        } else if (index == 9999) {
            success = false;
        }
    }

    if (success && (fflush(measurement_file) != 0 || ferror(measurement_file) != 0)) {
        success = false;
    }
    if (success) {
        if (fclose(measurement_file) != 0) {
            measurement_file = nullptr;
            success = false;
        } else {
            measurement_file = nullptr;
        }
        reopen_active_file = true;
    }

    char archive_path[sizeof(kMountPoint) + LOGGER_FILE_NAME_MAX] = {0};
    if (success) {
        snprintf(archive_path, sizeof(archive_path), "%s/%s", kMountPoint, candidate);
        if (rename(kLogPath, archive_path) != 0) {
            success = false;
        }
    }

    if (success) {
        measurement_file = fopen(kLogPath, "w");
        if (measurement_file == nullptr || !write_header()) {
            if (measurement_file != nullptr) {
                fclose(measurement_file);
                measurement_file = nullptr;
            }
            remove(kLogPath);
            if (rename(archive_path, kLogPath) != 0) {
                ESP_LOGE("logger", "Could not restore active log after archive failure");
                storage_ready = false;
                reopen_active_file = false;
            }
            success = false;
        } else {
            reopen_active_file = false;
        }
    }

    if (!success && reopen_active_file && measurement_file == nullptr) {
        measurement_file = fopen(kLogPath, "a+");
        if (measurement_file == nullptr) {
            storage_ready = false;
            ESP_LOGE("logger", "Could not reopen %s after archive failure", kLogPath);
        }
    }
    if (success) {
        strncpy(archive_name, candidate, archive_name_capacity - 1);
        archive_name[archive_name_capacity - 1] = '\0';
    }
    xSemaphoreGive(storage_mutex);
    return success;
}

extern "C" bool logger_get_file_size(const char *name, size_t *size)
{
    if (!valid_log_file_name(name) || size == nullptr || storage_mutex == nullptr ||
        xSemaphoreTake(storage_mutex, portMAX_DELAY) != pdTRUE) {
        return false;
    }
    bool success = storage_ready;
    if (success && strcmp(name, "measurements.csv") == 0 &&
        (measurement_file == nullptr || fflush(measurement_file) != 0)) {
        success = false;
    }
    char path[sizeof(kMountPoint) + LOGGER_FILE_NAME_MAX] = {0};
    struct stat file_status;
    if (success) {
        snprintf(path, sizeof(path), "%s/%s", kMountPoint, name);
        success = stat(path, &file_status) == 0 && S_ISREG(file_status.st_mode);
    }
    if (success) {
        *size = (size_t)file_status.st_size;
    }
    xSemaphoreGive(storage_mutex);
    return success;
}

extern "C" bool logger_read_file(const char *name, size_t offset, void *buffer,
                                  size_t capacity, size_t *bytes_read)
{
    if (!valid_log_file_name(name) || buffer == nullptr || bytes_read == nullptr ||
        offset > (size_t)LONG_MAX || storage_mutex == nullptr ||
        xSemaphoreTake(storage_mutex, portMAX_DELAY) != pdTRUE) {
        return false;
    }
    *bytes_read = 0;
    bool success = storage_ready;
    if (success && strcmp(name, "measurements.csv") == 0 &&
        (measurement_file == nullptr || fflush(measurement_file) != 0)) {
        success = false;
    }
    char path[sizeof(kMountPoint) + LOGGER_FILE_NAME_MAX] = {0};
    if (success) {
        snprintf(path, sizeof(path), "%s/%s", kMountPoint, name);
    }
    FILE *file = success ? fopen(path, "rb") : nullptr;
    if (success && file == nullptr) {
        success = false;
    }
    if (success && fseek(file, (long)offset, SEEK_SET) != 0) {
        success = false;
    }
    if (success) {
        *bytes_read = fread(buffer, 1, capacity, file);
        success = ferror(file) == 0;
    }
    if (file != nullptr && fclose(file) != 0) {
        success = false;
    }
    xSemaphoreGive(storage_mutex);
    return success;
}

extern "C" void logger_setup_task(void *pvParameters)
{
    (void)pvParameters;
    ESP_LOGI("logger", "Logger task started");
    measurement_queue = xQueueCreate(kQueueLength, sizeof(Measurement));
    if (measurement_queue == nullptr) {
        ESP_LOGE("logger", "Failed to create measurement queue");
        vTaskDelete(nullptr);
        return;
    }
    storage_mutex = xSemaphoreCreateMutex();
    if (storage_mutex == nullptr) {
        ESP_LOGE("logger", "Failed to create storage mutex");
        vTaskDelete(nullptr);
        return;
    }

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    spi_bus_config_t bus_config = {};
    bus_config.mosi_io_num = SD_MOSI_PIN;
    bus_config.miso_io_num = SD_MISO_PIN;
    bus_config.sclk_io_num = SD_SCK_PIN;
    bus_config.quadwp_io_num = -1;
    bus_config.quadhd_io_num = -1;
    bus_config.max_transfer_sz = 4000;
    esp_err_t error = spi_bus_initialize(
        static_cast<spi_host_device_t>(host.slot), &bus_config, SPI_DMA_CH_AUTO);
    if (error != ESP_OK) {
        ESP_LOGE("logger", "Failed to initialize SD SPI bus: %s", esp_err_to_name(error));
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = static_cast<gpio_num_t>(SD_CS_PIN);
    slot_config.host_id = static_cast<spi_host_device_t>(host.slot);
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {};
    mount_config.format_if_mount_failed = false;
    mount_config.max_files = 2;
    mount_config.allocation_unit_size = 16 * 1024;
    sdmmc_card_t *card = nullptr;
    esp_err_t mount_error = error;
    if (mount_error == ESP_OK) {
        mount_error = esp_vfs_fat_sdspi_mount(
            kMountPoint, &host, &slot_config, &mount_config, &card);
    }
    if (mount_error != ESP_OK) {
        ESP_LOGE("logger", "Failed to mount SD card: %s; measurements will not be saved",
                 esp_err_to_name(mount_error));
    } else {
        measurement_file = fopen(kLogPath, "a+");
        if (measurement_file == nullptr) {
            ESP_LOGE("logger", "Failed to open %s", kLogPath);
        } else {
            fseek(measurement_file, 0, SEEK_END);
            long file_size = ftell(measurement_file);
            if (file_size < 0) {
                ESP_LOGE("logger", "Failed to determine the size of %s", kLogPath);
                fclose(measurement_file);
                measurement_file = nullptr;
            } else if (file_size == 0 && !write_header()) {
                ESP_LOGE("logger", "Failed to write the header to %s", kLogPath);
                fclose(measurement_file);
                measurement_file = nullptr;
            }
            if (measurement_file != nullptr) {
                xSemaphoreTake(storage_mutex, portMAX_DELAY);
                storage_ready = true;
                xSemaphoreGive(storage_mutex);
                ESP_LOGI("logger", "Logging measurements to %s", kLogPath);
            }
        }
    }

    Measurement measurement;
    uint16_t num_meas = 0;
    while (true) {
        if (xQueueReceive(measurement_queue, &measurement, portMAX_DELAY) != pdPASS) {
            continue;
        }
        xSemaphoreTake(storage_mutex, portMAX_DELAY);
        if (measurement_file == nullptr) {
            ESP_LOGW("logger", "Measurement file not open; dropped measurement");
            xSemaphoreGive(storage_mutex);
            continue;
        }
        if (!first_measurement_received) {
            ESP_LOGI("logger", "Received first %s measurement",
                     measurement.type == MeasurementType::GPS ? "GPS" : "IMU");
            first_measurement_received = true;
        }
        bool write_succeeded = measurement.type == MeasurementType::GPS
            ? write_gps(measurement.gps)
            : write_imu(measurement);
        if (!write_succeeded || fflush(measurement_file) != 0 || ferror(measurement_file) != 0) {
            ESP_LOGE("logger", "Failed writing %s", kLogPath);
            clearerr(measurement_file);
            fclose(measurement_file);
            measurement_file = nullptr;
            storage_ready = false;
            xSemaphoreGive(storage_mutex);
            continue;
        }
        num_meas++;
        if (num_meas == 1000) {
            fclose(measurement_file);
            measurement_file = fopen(kLogPath, "a+");
            if (measurement_file == nullptr) {
                ESP_LOGE("logger", "Failed reopening %s", kLogPath);
                storage_ready = false;
            }
            num_meas = 0;
        }
        xSemaphoreGive(storage_mutex);
    }
}
