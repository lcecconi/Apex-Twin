#include "apex_imu.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdint.h>

#define IMU_I2C_PORT I2C_NUM_0
#define IMU_SDA_PIN 11
#define IMU_SCL_PIN 12
#define IMU_I2C_FREQUENCY_HZ 400000
#define IMU_ADDRESS 0x69
#define IMU_FIFO_FRAME_SIZE 22
#define IMU_FRAME_LOG_INTERVAL 20

#define REG_BANK_SEL 0x7F
#define BANK_0 0x00
#define BANK_2 0x20
#define BANK_3 0x30

#define USER_CTRL 0x03
#define PWR_MGMT_1 0x06
#define INT_PIN_CFG 0x0F
#define WHO_AM_I 0x00
#define FIFO_EN_1 0x66
#define FIFO_EN_2 0x67
#define FIFO_RST 0x68
#define FIFO_MODE 0x69
#define FIFO_COUNT_H 0x70
#define FIFO_R_W 0x72

#define GYRO_SMPLRT_DIV 0x00
#define GYRO_CONFIG_1 0x01
#define ACCEL_SMPLRT_DIV_1 0x10
#define ACCEL_SMPLRT_DIV_2 0x11
#define ACCEL_CONFIG 0x14
#define FSYNC_CONFIG 0x52
#define FSYNC_CONFIG_TEMP_LSB 0x01
#define FSYNC_CONFIG_DELAY_TIME_EN 0x80

#define I2C_MST_ODR_CONFIG 0x00
#define I2C_MST_CTRL 0x01
#define I2C_SLV0_ADDR 0x03
#define I2C_SLV0_REG 0x04
#define I2C_SLV0_CTRL 0x05
#define I2C_SLV0_DO 0x06

#define AK09916_ADDRESS 0x0C
#define AK09916_ST1 0x10
#define AK09916_CNTL2 0x31
#define AK09916_CONTINUOUS_100HZ 0x08
#define AK09916_READ_LENGTH 8
#define FIFO_TEMP_LSB_OFFSET 13

// Calibration values:
//   - TEMPERATURE
static const uint16_t room_temp_offset = 0; // Offset in LSB at room temperature (21.0°C)
static const float room_temp_lsb = 333.87f; // LSB per degree Celsius
//  - ACCELEROMETER
static float accel_sensitivity = 8192.0f; // LSB per g for +/- 4 g range
// - GYROSCOPE
static float gyro_sensitivity = 65.5f; // LSB per degree per second for 500 dps range

//
static const char *TAG = "imu";

static esp_err_t imu_write_raw(uint8_t reg, uint8_t value)
{
    // ESP-IDF expects the unshifted 7-bit I2C address.
    uint8_t address = IMU_ADDRESS;
    uint8_t data[] = {reg, value};
    esp_err_t error = i2c_master_write_to_device(
        IMU_I2C_PORT,
        address,
        data,
        sizeof(data),
        pdMS_TO_TICKS(100));
    if (error != ESP_OK) {
        ESP_LOGE(TAG, "I2C write reg 0x%02X value 0x%02X failed: %s",
                 reg, value, esp_err_to_name(error));
    }
    return error;
}

static esp_err_t imu_write(uint8_t bank, uint8_t reg, uint8_t value)
{
    if (reg != REG_BANK_SEL) {
        esp_err_t error = imu_write_raw(REG_BANK_SEL, bank);
        if (error != ESP_OK) {
            ESP_LOGE(TAG, "Selecting bank 0x%02X for register 0x%02X failed: %s",
                     bank, reg, esp_err_to_name(error));
            return error;
        }
    }
    esp_err_t error = imu_write_raw(reg, value);
    if (error != ESP_OK) {
        ESP_LOGE(TAG, "Writing bank 0x%02X register 0x%02X failed: %s",
                 bank, reg, esp_err_to_name(error));
    }
    return error;
}

static esp_err_t imu_read(uint8_t bank, uint8_t reg, uint8_t *data, size_t length)
{
    esp_err_t error = imu_write_raw(REG_BANK_SEL, bank);
    if (error != ESP_OK) {
        ESP_LOGE(TAG, "Selecting bank 0x%02X for register 0x%02X failed: %s",
                 bank, reg, esp_err_to_name(error));
        return error;
    }

    uint8_t address = IMU_ADDRESS;
    error = i2c_master_write_read_device(
        IMU_I2C_PORT,
        address,
        &reg,
        1,
        data,
        length,
        pdMS_TO_TICKS(100));
    if (error != ESP_OK) {
        ESP_LOGE(TAG, "I2C read reg 0x%02X length %u failed: %s",
                 reg, (unsigned int)length, esp_err_to_name(error));
    }
    return error;
}

static esp_err_t imu_select_bank(uint8_t bank)
{
    return imu_write(BANK_0, REG_BANK_SEL, bank);
}

static esp_err_t configure_magnetometer(void)
{
    esp_err_t error;

    ESP_LOGI(TAG, "Configuring internal I2C master and AK09916");
    error = imu_select_bank(BANK_3);
    if (error != ESP_OK) {
        return error;
    }

    error = imu_write(BANK_3, I2C_MST_CTRL, 0x07);
    if (error != ESP_OK) {
        return error;
    }
    error = imu_write(BANK_3, I2C_MST_ODR_CONFIG, 0x00);
    if (error != ESP_OK) {
        return error;
    }

    // Write AK09916 CNTL2 = continuous measurement at 100 Hz.
    error = imu_write(BANK_3, I2C_SLV0_REG, AK09916_CNTL2);
    if (error != ESP_OK) {
        return error;
    }
    error = imu_write(BANK_3, I2C_SLV0_DO, AK09916_CONTINUOUS_100HZ);
    if (error != ESP_OK) {
        return error;
    }
    error = imu_write(BANK_3, I2C_SLV0_ADDR, AK09916_ADDRESS);
    if (error != ESP_OK) {
        return error;
    }
    vTaskDelay(pdMS_TO_TICKS(20));

    // Read ST1 through ST2 (status, XYZ, and overflow/status bytes) into EXT_SLV data.
    error = imu_write(BANK_3, I2C_SLV0_REG, AK09916_ST1);
    if (error != ESP_OK) {
        return error;
    }
    error = imu_write(BANK_3, I2C_SLV0_ADDR, AK09916_ADDRESS | 0x80);
    if (error != ESP_OK) {
        return error;
    }
    return imu_write(BANK_3, I2C_SLV0_CTRL, 0x80 | AK09916_READ_LENGTH);
}

static esp_err_t reset_fifo(void)
{
    esp_err_t error = imu_write(BANK_0, FIFO_RST, 0x1F);
    if (error != ESP_OK) {
        return error;
    }
    vTaskDelay(pdMS_TO_TICKS(2));
    return imu_write(BANK_0, FIFO_RST, 0x00);
}

static esp_err_t configure_imu(void)
{
    esp_err_t error;
    uint8_t who_am_i = 0;

    error = imu_read(BANK_0, WHO_AM_I, &who_am_i, 1);
    if (error != ESP_OK) {
        return error;
    } else {
        ESP_LOGI(TAG, "WHO_AM_I: 0x%02X", who_am_i);
    }
    if (who_am_i != 0xEA) {
        ESP_LOGE(TAG, "Unexpected WHO_AM_I: 0x%02X (expected 0xEA)", who_am_i);
        return ESP_ERR_INVALID_RESPONSE;
    }

    error = imu_write(BANK_0, PWR_MGMT_1, 0x01);
    if (error != ESP_OK) {
        return error;
    }
    vTaskDelay(pdMS_TO_TICKS(50));
    error = imu_write(BANK_0, USER_CTRL, 0x20); // Internal I2C master; DMP remains disabled.
    if (error != ESP_OK) {
        return error;
    }

    error = imu_select_bank(BANK_2);
    if (error != ESP_OK) {
        return error;
    }
    error = imu_write(BANK_2, GYRO_SMPLRT_DIV, 0x0A); // 1.125 kHz / (10 + 1) = 102.3 Hz.
    if (error != ESP_OK) {
        return error;
    }
    error = imu_write(BANK_2, GYRO_CONFIG_1, 0x13); // Continuous mode, 500 dps, DPLF BW 23.9 Hz.
    if (error != ESP_OK) {
        return error;
    }
    error = imu_write(BANK_2, ACCEL_SMPLRT_DIV_1, 0x00);
    if (error != ESP_OK) {
        return error;
    }
    error = imu_write(BANK_2, ACCEL_SMPLRT_DIV_2, 0x0A); // 1.125 kHz / (10 + 1) ~= 102.3 Hz.
    if (error != ESP_OK) {
        return error;
    }
    error = imu_write(BANK_2, ACCEL_CONFIG, 0x3B); // +/- 4 g, DLPF enabled at 473 Hz.
    if (error != ESP_OK) {
        return error;
    }
    error = imu_write(
        BANK_2,
        FSYNC_CONFIG,
        FSYNC_CONFIG_TEMP_LSB | FSYNC_CONFIG_DELAY_TIME_EN); // FSYNC is written to TEMP_OUT_LSB bit 0, and delay time is written to regs.
    if (error != ESP_OK) {
        return error;
    }

    error = configure_magnetometer();
    if (error != ESP_OK) {
        ESP_LOGE(TAG, "Magnetometer configuration failed: %s", esp_err_to_name(error));
        return error;
    }

    ESP_LOGI(TAG, "Configuring FIFO");
    error = imu_select_bank(BANK_0);
    if (error != ESP_OK) {
        return error;
    }
    error = imu_write(BANK_0, FIFO_EN_1, 0x01); // FIFO includes external sensor 0 data.
    if (error != ESP_OK) {
        return error;
    }
    ESP_LOGI(TAG, "Magnetometer FIFO routing enabled: FIFO_EN_1.PERIPH_0_FIFO_EN=1");
    error = imu_write(BANK_0, FIFO_EN_2, 0x1F); // Temperature, gyro XYZ, and accel.
    if (error != ESP_OK) {
        return error;
    }
    error = imu_write(BANK_0, FIFO_MODE, 0x00); // Stream mode: FIFO overwrites old data.
    if (error != ESP_OK) {
        return error;
    }
    error = reset_fifo();
    if (error != ESP_OK) {
        return error;
    }
    return imu_write(BANK_0, USER_CTRL, 0x60); // FIFO and internal I2C master enabled; no DMP.
}

static int16_t frame_int16(const uint8_t *frame, size_t offset)
{
    return (int16_t)(((uint16_t)frame[offset] << 8) | frame[offset + 1]);
}

static float calc_temp_c(int16_t temp_uint)
{
    return ((temp_uint - room_temp_offset) / room_temp_lsb) + 21.0f;
}

static float calc_accel_g(int16_t accel_int)
{
    return ((float)accel_int) / accel_sensitivity;
}

static float calc_gyro_dps(int16_t gyro_int)
{
    return ((float)gyro_int) / gyro_sensitivity;
}

static void log_frame(const uint8_t *frame)
{
    float temp_c = calc_temp_c(frame_int16(frame, 12));
    bool fsync_asserted = (frame[FIFO_TEMP_LSB_OFFSET] & 0x01U) != 0;
    float accel_x_g = calc_accel_g(frame_int16(frame, 0));
    float accel_y_g = calc_accel_g(frame_int16(frame, 2));
    float accel_z_g = calc_accel_g(frame_int16(frame, 4));
    float gyro_x_dps = calc_gyro_dps(frame_int16(frame, 6));
    float gyro_y_dps = calc_gyro_dps(frame_int16(frame, 8));
    float gyro_z_dps = calc_gyro_dps(frame_int16(frame, 10));
    ESP_LOGI(
        "imu_monitor",
        "IMU frame contents: accel=(%.4f,%.4f,%.4f) gyro=(%.2f,%.2f,%.2f) temp_c=%.2f "
        "F_SYNCH=%s",
        accel_x_g,
        accel_y_g,
        accel_z_g,
        gyro_x_dps,
        gyro_y_dps,
        gyro_z_dps,
        temp_c,
        (fsync_asserted ? "true" : "false"));
}

void imu_monitor_task(void *pvParameters)
{
    (void)pvParameters;
    uint8_t count_data[2];
    uint8_t fifo_data[220];
    uint8_t frame[IMU_FIFO_FRAME_SIZE];
    size_t frame_length = 0;
    uint32_t bytes_received = 0;
    uint32_t frames_received = 0;
    uint32_t frame_log_counter = 0;
    uint32_t frames_since_fsync = 0;
    uint32_t fsync_intervals = 0;
    uint32_t fsync_frames_between_sum = 0;
    uint32_t last_fsync_frames_between = 0;
    bool have_fsync_reference = false;
    bool previous_fsync = false;
    TickType_t report_tick = xTaskGetTickCount();

    while (true) {
        if (imu_read(BANK_0, FIFO_COUNT_H, count_data, sizeof(count_data)) != ESP_OK) {
            ESP_LOGW("imu_monitor", "Failed to read FIFO count");
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        uint16_t fifo_count = ((uint16_t)count_data[0] << 8) | count_data[1];
        if (fifo_count >= 220) {
            ESP_LOGW("imu_monitor", "IMU FIFO data length: %u bytes", (unsigned int)fifo_count);
        }
        while ((fifo_count % IMU_FIFO_FRAME_SIZE) == 0 && fifo_count > 0) {
            size_t read_length = fifo_count < sizeof(fifo_data) ? fifo_count : sizeof(fifo_data);
            if (imu_read(BANK_0, FIFO_R_W, fifo_data, read_length) != ESP_OK) {
                ESP_LOGW("imu_monitor", "Failed to read FIFO data");
                break;
            }
            bytes_received += read_length;
            fifo_count -= read_length;

            for (size_t index = 0; index < read_length; index++) {
                frame[frame_length++] = fifo_data[index];
                if (frame_length == sizeof(frame)) {
                    bool fsync_asserted = (frame[FIFO_TEMP_LSB_OFFSET] & 0x01U) != 0;
                    frames_received++;
                    frame_log_counter++;
                    if (fsync_asserted && !previous_fsync) {
                        ESP_LOGI("imu_monitor", "FSYNC signal detected:");
                        if (have_fsync_reference) {
                            last_fsync_frames_between = frames_since_fsync;
                            fsync_frames_between_sum += last_fsync_frames_between;
                            fsync_intervals++;
                        }
                        have_fsync_reference = true;
                        frames_since_fsync = 0;
                    } else if (have_fsync_reference) {
                        frames_since_fsync++;
                    }
                    if (frame_log_counter % IMU_FRAME_LOG_INTERVAL == 0) {
                        log_frame(frame);
                    }
                    previous_fsync = fsync_asserted;
                    frame_length = 0;
                }
            }
        }

        TickType_t now = xTaskGetTickCount();
        if (now - report_tick >= pdMS_TO_TICKS(1000)) {
            ESP_LOGI(
                "imu_monitor",
                "FIFO stream: %lu bytes/s, %lu frames/s, FSYNC intervals: %lu, "
                "frames between: last=%lu avg=%.2f",
                (unsigned long)bytes_received,
                (unsigned long)frames_received,
                (unsigned long)fsync_intervals,
                (unsigned long)last_fsync_frames_between,
                fsync_intervals > 0
                    ? (double)fsync_frames_between_sum / fsync_intervals
                    : 0.0);
            bytes_received = 0;
            frames_received = 0;
            fsync_intervals = 0;
            fsync_frames_between_sum = 0;
            report_tick = now;
        }
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

void imu_setup_task(void *pvParameters)
{
    (void)pvParameters;

    const i2c_config_t config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = IMU_SDA_PIN,
        .scl_io_num = IMU_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = IMU_I2C_FREQUENCY_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(IMU_I2C_PORT, &config));
    ESP_ERROR_CHECK(i2c_driver_install(IMU_I2C_PORT, config.mode, 0, 0, 0));

    esp_err_t error = configure_imu();
    if (error != ESP_OK) {
        ESP_LOGE(TAG, "IMU configuration failed: %s (0x%02X)",
                 esp_err_to_name(error), (unsigned int)error);
        vTaskDelete(NULL);
    }

    ESP_LOGI(TAG, "ICM-20948 configured: accel/gyro/magnetometer FIFO at 100 Hz, DMP disabled");
    ESP_LOGI(TAG, "FSYNC source: TEMP_OUT_LSB bit 0 (FSYNC_CONFIG=0x%02X)", FSYNC_CONFIG_TEMP_LSB);
    ESP_ERROR_CHECK(reset_fifo());
    xTaskCreate(imu_monitor_task, "imu_monitor", 4096, NULL, 5, NULL);
    vTaskDelete(NULL);
}
