#include "onboard_sensors.h"
#include <esp_system.h>

#define SHTC3_ADDR      0x70
#define PCF85063_ADDR   0x51

// SHTC3 Commands
#define SHTC3_CMD_WAKEUP      0x3517
#define SHTC3_CMD_SLEEP       0xB098
#define SHTC3_CMD_MEAS_NORM   0x7866  // T first, normal power, clock stretching disabled

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
      if (crc & 0x80) {
        crc = (crc << 1) ^ 0x31;
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}

void OnboardSensors::begin() {
  // Setup Buttons with internal pull-ups
  pinMode(PIN_BTN_BOOT, INPUT_PULLUP);
  pinMode(PIN_BTN_KEY, INPUT_PULLUP);

  // Setup Battery ADC
  analogSetPinAttenuation(PIN_VBAT_ADC, ADC_11db);

  // Setup I2C
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);

  // Initialize SHTC3
  _data.shtc3_found = initSHTC3();

  // Initialize PCF85063 RTC
  _data.rtc_found = initRTC();

  // Check PSRAM
  if (psramFound()) {
    _data.total_psram_kb = ESP.getPsramSize() / 1024;
  }

  // Initial update
  update();
}

bool OnboardSensors::initSHTC3() {
  Wire.beginTransmission(SHTC3_ADDR);
  if (Wire.endTransmission() != 0) {
    return false;
  }

  // Wake up SHTC3
  Wire.beginTransmission(SHTC3_ADDR);
  Wire.write(SHTC3_CMD_WAKEUP >> 8);
  Wire.write(SHTC3_CMD_WAKEUP & 0xFF);
  Wire.endTransmission();
  delay(1);

  // Read ID
  Wire.beginTransmission(SHTC3_ADDR);
  Wire.write(0xEF);
  Wire.write(0xC8);
  Wire.endTransmission();
  if (Wire.requestFrom((uint8_t)SHTC3_ADDR, (uint8_t)3) == 3) {
    uint8_t id_h = Wire.read();
    uint8_t id_l = Wire.read();
    uint8_t crc = Wire.read();
    uint8_t buf[2] = {id_h, id_l};
    if (shtc3_crc8(buf, 2) == crc) {
      log_i("SHTC3 ID: 0x%02X%02X verified", id_h, id_l);
    }
  }

  // Put to sleep
  Wire.beginTransmission(SHTC3_ADDR);
  Wire.write(SHTC3_CMD_SLEEP >> 8);
  Wire.write(SHTC3_CMD_SLEEP & 0xFF);
  Wire.endTransmission();

  return true;
}

bool OnboardSensors::readSHTC3(float &temp, float &humidity) {
  // Wake up
  Wire.beginTransmission(SHTC3_ADDR);
  Wire.write(SHTC3_CMD_WAKEUP >> 8);
  Wire.write(SHTC3_CMD_WAKEUP & 0xFF);
  if (Wire.endTransmission() != 0) {
    return false;
  }
  delayMicroseconds(300);

  // Start measurement: Normal power, T first, clock stretching disabled
  Wire.beginTransmission(SHTC3_ADDR);
  Wire.write(SHTC3_CMD_MEAS_NORM >> 8);
  Wire.write(SHTC3_CMD_MEAS_NORM & 0xFF);
  if (Wire.endTransmission() != 0) {
    return false;
  }

  // Wait typical 13 ms for measurement
  delay(15);

  // Read 6 bytes: T_MSB, T_LSB, T_CRC, RH_MSB, RH_LSB, RH_CRC
  if (Wire.requestFrom((uint8_t)SHTC3_ADDR, (uint8_t)6) != 6) {
    return false;
  }

  uint8_t raw[6];
  for (int i = 0; i < 6; i++) {
    raw[i] = Wire.read();
  }

  // Verify CRCs
  if (shtc3_crc8(&raw[0], 2) != raw[2] || shtc3_crc8(&raw[3], 2) != raw[5]) {
    return false;
  }

  uint16_t t_raw = ((uint16_t)raw[0] << 8) | raw[1];
  uint16_t rh_raw = ((uint16_t)raw[3] << 8) | raw[4];

  temp = -45.0f + 175.0f * ((float)t_raw / 65536.0f);
  humidity = 100.0f * ((float)rh_raw / 65536.0f);

  // Put sensor back to sleep
  Wire.beginTransmission(SHTC3_ADDR);
  Wire.write(SHTC3_CMD_SLEEP >> 8);
  Wire.write(SHTC3_CMD_SLEEP & 0xFF);
  Wire.endTransmission();

  return true;
}

bool OnboardSensors::initRTC() {
  Wire.beginTransmission(PCF85063_ADDR);
  if (Wire.endTransmission() != 0) {
    return false;
  }

  // Read Control_1 (reg 0x00) and Seconds (reg 0x04)
  Wire.beginTransmission(PCF85063_ADDR);
  Wire.write(0x00);
  Wire.endTransmission();
  Wire.requestFrom((uint8_t)PCF85063_ADDR, (uint8_t)1);
  uint8_t ctrl1 = Wire.read();

  // If STOP bit is set, clear it to run the RTC
  if (ctrl1 & (1 << 5)) {
    Wire.beginTransmission(PCF85063_ADDR);
    Wire.write(0x00);
    Wire.write(0x00); // 12.5pF, 24hr, clear STOP
    Wire.endTransmission();
  }

  // Check OS (oscillator stop flag) in seconds register (0x04, bit 7)
  Wire.beginTransmission(PCF85063_ADDR);
  Wire.write(0x04);
  Wire.endTransmission();
  Wire.requestFrom((uint8_t)PCF85063_ADDR, (uint8_t)1);
  uint8_t sec_reg = Wire.read();

  if (sec_reg & 0x80) {
    // Oscillator was stopped (e.g. initial powerup). Set initial default time.
    setRtcTime(2026, 9, 15, 12, 0, 0);
  }

  return true;
}

void OnboardSensors::setRtcTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) {
  Wire.beginTransmission(PCF85063_ADDR);
  Wire.write(0x04); // Start at seconds
  Wire.write(dec2bcd(second) & 0x7F); // Bit 7 = 0 to clear OS flag
  Wire.write(dec2bcd(minute) & 0x7F);
  Wire.write(dec2bcd(hour) & 0x3F);
  Wire.write(dec2bcd(day) & 0x3F);
  Wire.write(0x02); // Weekday
  Wire.write(dec2bcd(month) & 0x1F);
  Wire.write(dec2bcd(year >= 2000 ? year - 2000 : year));
  Wire.endTransmission();
}

bool OnboardSensors::readRTC(uint16_t &year, uint8_t &month, uint8_t &day, uint8_t &hour, uint8_t &minute, uint8_t &second) {
  Wire.beginTransmission(PCF85063_ADDR);
  Wire.write(0x04);
  if (Wire.endTransmission() != 0) {
    return false;
  }

  if (Wire.requestFrom((uint8_t)PCF85063_ADDR, (uint8_t)7) != 7) {
    return false;
  }

  uint8_t s = Wire.read();
  uint8_t m = Wire.read();
  uint8_t h = Wire.read();
  uint8_t d = Wire.read();
  Wire.read(); // skip weekday
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
  // ADC1_CH3 on GPIO 4 with 200k / 100k voltage divider (1/3 ratio)
  uint32_t adc_mv = analogReadMilliVolts(PIN_VBAT_ADC);
  voltage = (adc_mv * 3.0f) / 1000.0f;

  if (voltage >= 4.15f) {
    percent = 100;
  } else if (voltage <= 3.20f) {
    percent = 0;
  } else {
    percent = (uint8_t)(((voltage - 3.20f) / (4.15f - 3.20f)) * 100.0f);
  }
}

void OnboardSensors::update() {
  // Buttons
  _data.boot_btn_pressed = (digitalRead(PIN_BTN_BOOT) == LOW);
  _data.key_btn_pressed  = (digitalRead(PIN_BTN_KEY) == LOW);

  // SHTC3
  if (_data.shtc3_found) {
    readSHTC3(_data.temperature, _data.humidity);
  }

  // RTC
  if (_data.rtc_found) {
    readRTC(_data.year, _data.month, _data.day, _data.hour, _data.minute, _data.second);
  }

  // Battery
  readBattery(_data.battery_voltage, _data.battery_percent);

  // System
  _data.free_heap_kb = ESP.getFreeHeap() / 1024;
  if (psramFound()) {
    _data.free_psram_kb = ESP.getFreePsram() / 1024;
  }
  _data.cpu_freq_mhz = ESP.getCpuFreqMHz();
  _data.uptime_sec = millis() / 1000;
}
