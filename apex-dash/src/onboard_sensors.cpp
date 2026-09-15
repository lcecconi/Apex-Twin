#include "onboard_sensors.h"
#include <esp_system.h>

#define SHTC3_ADDR      0x70
#define PCF85063_ADDR   0x51

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
  analogSetPinAttenuation(PIN_VBAT_ADC, ADC_11db);
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, I2C_BUS_SPEED);

  _data.shtc3_found = initSHTC3();
  _data.rtc_found = initRTC();

  if (psramFound()) {
    _data.total_psram_kb = ESP.getPsramSize() / 1024;
  }
  update();
}

bool OnboardSensors::initSHTC3() {
  Wire.beginTransmission(SHTC3_ADDR);
  if (Wire.endTransmission() != 0) return false;

  Wire.beginTransmission(SHTC3_ADDR);
  Wire.write(SHTC3_CMD_WAKEUP >> 8);
  Wire.write(SHTC3_CMD_WAKEUP & 0xFF);
  Wire.endTransmission();
  delay(1);

  Wire.beginTransmission(SHTC3_ADDR);
  Wire.write(SHTC3_CMD_SLEEP >> 8);
  Wire.write(SHTC3_CMD_SLEEP & 0xFF);
  Wire.endTransmission();
  return true;
}

bool OnboardSensors::readSHTC3(float &temp, float &humidity) {
  Wire.beginTransmission(SHTC3_ADDR);
  Wire.write(SHTC3_CMD_WAKEUP >> 8);
  Wire.write(SHTC3_CMD_WAKEUP & 0xFF);
  if (Wire.endTransmission() != 0) return false;
  delayMicroseconds(300);

  Wire.beginTransmission(SHTC3_ADDR);
  Wire.write(SHTC3_CMD_MEAS_NORM >> 8);
  Wire.write(SHTC3_CMD_MEAS_NORM & 0xFF);
  if (Wire.endTransmission() != 0) return false;

  delay(15);

  if (Wire.requestFrom((uint8_t)SHTC3_ADDR, (uint8_t)6) != 6) return false;

  uint8_t raw[6];
  for (int i = 0; i < 6; i++) raw[i] = Wire.read();

  if (shtc3_crc8(&raw[0], 2) != raw[2] || shtc3_crc8(&raw[3], 2) != raw[5]) {
    return false;
  }

  uint16_t t_raw = ((uint16_t)raw[0] << 8) | raw[1];
  uint16_t rh_raw = ((uint16_t)raw[3] << 8) | raw[4];

  temp = -45.0f + 175.0f * ((float)t_raw / 65536.0f);
  humidity = 100.0f * ((float)rh_raw / 65536.0f);

  Wire.beginTransmission(SHTC3_ADDR);
  Wire.write(SHTC3_CMD_SLEEP >> 8);
  Wire.write(SHTC3_CMD_SLEEP & 0xFF);
  Wire.endTransmission();
  return true;
}

bool OnboardSensors::initRTC() {
  Wire.beginTransmission(PCF85063_ADDR);
  if (Wire.endTransmission() != 0) return false;

  Wire.beginTransmission(PCF85063_ADDR);
  Wire.write(0x00);
  Wire.endTransmission();
  Wire.requestFrom((uint8_t)PCF85063_ADDR, (uint8_t)1);
  uint8_t ctrl1 = Wire.read();

  if (ctrl1 & (1 << 5)) {
    Wire.beginTransmission(PCF85063_ADDR);
    Wire.write(0x00);
    Wire.write(0x00);
    Wire.endTransmission();
  }

  Wire.beginTransmission(PCF85063_ADDR);
  Wire.write(0x04);
  Wire.endTransmission();
  Wire.requestFrom((uint8_t)PCF85063_ADDR, (uint8_t)1);
  uint8_t sec_reg = Wire.read();

  if (sec_reg & 0x80) {
    setRtcTime(2026, 9, 15, 12, 0, 0);
  }
  return true;
}

void OnboardSensors::setRtcTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) {
  Wire.beginTransmission(PCF85063_ADDR);
  Wire.write(0x04);
  Wire.write(dec2bcd(second) & 0x7F);
  Wire.write(dec2bcd(minute) & 0x7F);
  Wire.write(dec2bcd(hour) & 0x3F);
  Wire.write(dec2bcd(day) & 0x3F);
  Wire.write(0x02);
  Wire.write(dec2bcd(month) & 0x1F);
  Wire.write(dec2bcd(year >= 2000 ? year - 2000 : year));
  Wire.endTransmission();
}

bool OnboardSensors::readRTC(uint16_t &year, uint8_t &month, uint8_t &day, uint8_t &hour, uint8_t &minute, uint8_t &second) {
  Wire.beginTransmission(PCF85063_ADDR);
  Wire.write(0x04);
  if (Wire.endTransmission() != 0) return false;
  if (Wire.requestFrom((uint8_t)PCF85063_ADDR, (uint8_t)7) != 7) return false;

  uint8_t s = Wire.read();
  uint8_t m = Wire.read();
  uint8_t h = Wire.read();
  uint8_t d = Wire.read();
  Wire.read();
  uint8_t mo = Wire.read();
  uint8_t y = Wire.read();

  second = bcd2dec(s & 0x7F);
  minute = bcd2dec(m & 0x7F);
  hour   = bcd2dec(h & 0x3F);
  day    = bcd2dec(d & 0x3F);
  month  = bcd2dec(mo & 0x1F);
  year   = 2000 + bcd2dec(y);
  return true;
}

void OnboardSensors::readBattery(float &voltage, uint8_t &percent) {
  uint32_t adc_mv = analogReadMilliVolts(PIN_VBAT_ADC);
  voltage = (adc_mv * 3.0f) / 1000.0f;

  if (voltage >= 4.15f) percent = 100;
  else if (voltage <= 3.20f) percent = 0;
  else percent = (uint8_t)(((voltage - 3.20f) / (4.15f - 3.20f)) * 100.0f);
}

void OnboardSensors::update() {
  if (_data.shtc3_found) {
    readSHTC3(_data.ambient_temp_c, _data.ambient_humidity_pct);
  }
  if (_data.rtc_found) {
    readRTC(_data.year, _data.month, _data.day, _data.hour, _data.minute, _data.second);
  }
  readBattery(_data.battery_voltage, _data.battery_percent);

  _data.free_heap_kb = ESP.getFreeHeap() / 1024;
  if (psramFound()) {
    _data.free_psram_kb = ESP.getFreePsram() / 1024;
  }
}
