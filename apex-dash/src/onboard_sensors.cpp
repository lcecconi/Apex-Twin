#include "onboard_sensors.h"
#include "driver/i2c.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>
#include <cmath>

#define I2C_MASTER_NUM    I2C_NUM_0
#define SHTC3_ADDR        0x70
#define PCF85063_ADDR     0x51

#define SHTC3_CMD_WAKEUP      0x3517
#define SHTC3_CMD_SLEEP       0xB098
#define SHTC3_CMD_MEAS_NORM   0x7866

static inline uint8_t bcd2dec(uint8_t bcd) {
  return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

static inline uint8_t dec2bcd(uint8_t dec) {
  return ((dec / 10) << 4) | (dec % 10);
}

static uint8_t shtc3_crc8(const uint8_t *data, size_t len) {
  uint8_t crc = 0xFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (int b = 0; b < 8; b++) {
      if (crc & 0x80) crc = (crc << 1) ^ 0x31;
      else crc <<= 1;
    }
  }
  return crc;
}

void OnboardSensors::begin() {
  i2c_config_t conf = {};
  conf.mode = I2C_MODE_MASTER;
  conf.sda_io_num = (gpio_num_t)PIN_I2C_SDA;
  conf.scl_io_num = (gpio_num_t)PIN_I2C_SCL;
  conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
  conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
  conf.master.clk_speed = I2C_BUS_SPEED;
  i2c_param_config(I2C_MASTER_NUM, &conf);
  i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);

  _data.shtc3_found = initSHTC3();
  _data.rtc_found = initRTC();
  update();
}

bool OnboardSensors::initSHTC3() {
  uint8_t wake_cmd[2] = { (uint8_t)(SHTC3_CMD_WAKEUP >> 8), (uint8_t)(SHTC3_CMD_WAKEUP & 0xFF) };
  if (i2c_master_write_to_device(I2C_MASTER_NUM, SHTC3_ADDR, wake_cmd, 2, pdMS_TO_TICKS(50)) != ESP_OK) {
    return false;
  }
  vTaskDelay(pdMS_TO_TICKS(1));

  uint8_t sleep_cmd[2] = { (uint8_t)(SHTC3_CMD_SLEEP >> 8), (uint8_t)(SHTC3_CMD_SLEEP & 0xFF) };
  i2c_master_write_to_device(I2C_MASTER_NUM, SHTC3_ADDR, sleep_cmd, 2, pdMS_TO_TICKS(50));
  return true;
}

bool OnboardSensors::readSHTC3(float &temp, float &humidity) {
  uint8_t wake_cmd[2] = { (uint8_t)(SHTC3_CMD_WAKEUP >> 8), (uint8_t)(SHTC3_CMD_WAKEUP & 0xFF) };
  if (i2c_master_write_to_device(I2C_MASTER_NUM, SHTC3_ADDR, wake_cmd, 2, pdMS_TO_TICKS(50)) != ESP_OK) {
    return false;
  }
  esp_rom_delay_us(300);

  uint8_t meas_cmd[2] = { (uint8_t)(SHTC3_CMD_MEAS_NORM >> 8), (uint8_t)(SHTC3_CMD_MEAS_NORM & 0xFF) };
  if (i2c_master_write_to_device(I2C_MASTER_NUM, SHTC3_ADDR, meas_cmd, 2, pdMS_TO_TICKS(50)) != ESP_OK) {
    return false;
  }
  vTaskDelay(pdMS_TO_TICKS(15));

  uint8_t raw[6];
  if (i2c_master_read_from_device(I2C_MASTER_NUM, SHTC3_ADDR, raw, 6, pdMS_TO_TICKS(50)) != ESP_OK) {
    return false;
  }

  if (shtc3_crc8(&raw[0], 2) != raw[2] || shtc3_crc8(&raw[3], 2) != raw[5]) {
    return false;
  }

  uint16_t t_raw = ((uint16_t)raw[0] << 8) | raw[1];
  uint16_t rh_raw = ((uint16_t)raw[3] << 8) | raw[4];

  temp = -45.0f + 175.0f * ((float)t_raw / 65536.0f);
  humidity = 100.0f * ((float)rh_raw / 65536.0f);

  uint8_t sleep_cmd[2] = { (uint8_t)(SHTC3_CMD_SLEEP >> 8), (uint8_t)(SHTC3_CMD_SLEEP & 0xFF) };
  i2c_master_write_to_device(I2C_MASTER_NUM, SHTC3_ADDR, sleep_cmd, 2, pdMS_TO_TICKS(50));
  return true;
}

bool OnboardSensors::initRTC() {
  uint8_t reg = 0x00;
  uint8_t ctrl1 = 0;
  if (i2c_master_write_read_device(I2C_MASTER_NUM, PCF85063_ADDR, &reg, 1, &ctrl1, 1, pdMS_TO_TICKS(50)) != ESP_OK) {
    return false;
  }

  if (ctrl1 & (1 << 5)) {
    uint8_t clear_stop[2] = { 0x00, 0x00 };
    i2c_master_write_to_device(I2C_MASTER_NUM, PCF85063_ADDR, clear_stop, 2, pdMS_TO_TICKS(50));
  }

  reg = 0x04;
  uint8_t sec_reg = 0;
  i2c_master_write_read_device(I2C_MASTER_NUM, PCF85063_ADDR, &reg, 1, &sec_reg, 1, pdMS_TO_TICKS(50));

  if (sec_reg & 0x80) {
    setRtcTime(2026, 9, 15, 12, 0, 0);
  }
  return true;
}

void OnboardSensors::setRtcTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) {
  uint8_t buf[8];
  buf[0] = 0x04; // Register start: seconds
  buf[1] = dec2bcd(second) & 0x7F;
  buf[2] = dec2bcd(minute) & 0x7F;
  buf[3] = dec2bcd(hour) & 0x3F;
  buf[4] = dec2bcd(day) & 0x3F;
  buf[5] = 0x02; // Weekday
  buf[6] = dec2bcd(month) & 0x1F;
  buf[7] = dec2bcd(year >= 2000 ? year - 2000 : year);
  i2c_master_write_to_device(I2C_MASTER_NUM, PCF85063_ADDR, buf, 8, pdMS_TO_TICKS(50));
}

bool OnboardSensors::readRTC(uint16_t &year, uint8_t &month, uint8_t &day, uint8_t &hour, uint8_t &minute, uint8_t &second) {
  uint8_t reg = 0x04;
  uint8_t raw[7];
  if (i2c_master_write_read_device(I2C_MASTER_NUM, PCF85063_ADDR, &reg, 1, raw, 7, pdMS_TO_TICKS(50)) != ESP_OK) {
    return false;
  }

  second = bcd2dec(raw[0] & 0x7F);
  minute = bcd2dec(raw[1] & 0x7F);
  hour   = bcd2dec(raw[2] & 0x3F);
  day    = bcd2dec(raw[3] & 0x3F);
  month  = bcd2dec(raw[5] & 0x1F);
  year   = 2000 + bcd2dec(raw[6]);
  return true;
}

void OnboardSensors::readBattery(float &voltage, uint8_t &percent) {
  voltage = 4.12f; // Nominal or battery ADC reading
  percent = 95;
}

void OnboardSensors::update() {
  if (_data.shtc3_found) {
    readSHTC3(_data.ambient_temp_c, _data.ambient_humidity_pct);
  }
  if (_data.rtc_found) {
    readRTC(_data.year, _data.month, _data.day, _data.hour, _data.minute, _data.second);
  }
  readBattery(_data.battery_voltage, _data.battery_percent);

  _data.free_heap_kb = esp_get_free_heap_size() / 1024;
}
