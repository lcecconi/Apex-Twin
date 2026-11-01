#include "apex_logger.h"
#include "apex_utils.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/spi_master.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "freertos/queue.h"
#include <stdio.h>

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
uint32_t dropped_measurements;
bool first_measurement_received;

void write_header()
{
    fprintf(measurement_file,
        "timestamp_ms,source,utc_time_ms,latitude,longitude,altitude_m,hdop,satellites,gps_mode,"
        "correction_age_s,accel_x_g,accel_y_g,accel_z_g,gyro_x_dps,gyro_y_dps,"
        "gyro_z_dps,mag_x_ut,mag_y_ut,mag_z_ut,temp_c,pitch_deg,roll_deg,yaw_deg,fsync\n");
    fflush(measurement_file);
}

void write_gps(const gps_data_t &data)
{
    fprintf(measurement_file,
        "%lu,GPS,%lu,%.9f,%.9f,%.3f,%.3f,%u,%u,%.3f,,,,,,,,,,,,,,\n",
        (unsigned long)data.utc_time_ms,
        (unsigned long)data.utc_time_ms,
        data.latitude,
        data.longitude,
        data.altitude_m,
        data.hdop,
        data.satellites,
        data.gps_mode,
        data.correction_age_s);
}

void write_imu(const Measurement &measurement)
{
    const imu_data_t &data = measurement.imu;
    const imu_orientation_t &orientation = measurement.orientation;
    fprintf(measurement_file,
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
        measurement.fsync_asserted ? 1U : 0U);
}
}

extern "C" bool logger_log_gps(const gps_data_t *data)
{
    if (data == nullptr || measurement_queue == nullptr) {
        return false;
    }

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
    mount_config.max_files = 1;
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
            if (ftell(measurement_file) == 0) {
                write_header();
            }
            ESP_LOGI("logger", "Logging measurements to %s", kLogPath);
        }
    }

    Measurement measurement;
    while (true) {
        if (xQueueReceive(measurement_queue, &measurement, portMAX_DELAY) != pdPASS) {
            continue;
        }
        if (measurement_file == nullptr) {
            continue;
        }
        if (!first_measurement_received) {
            ESP_LOGI("logger", "Received first %s measurement",
                     measurement.type == MeasurementType::GPS ? "GPS" : "IMU");
            first_measurement_received = true;
        }
        if (measurement.type == MeasurementType::GPS) {
            write_gps(measurement.gps);
        } else {
            write_imu(measurement);
        }
        if (fflush(measurement_file) != 0 || ferror(measurement_file) != 0) {
            ESP_LOGE("logger", "Failed writing %s", kLogPath);
            clearerr(measurement_file);
        }
    }
}
