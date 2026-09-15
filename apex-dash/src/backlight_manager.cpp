#include "backlight_manager.h"

void BacklightManager::begin() {
  pinMode(PIN_BACKLIGHT_PWM, OUTPUT);
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
  ledcAttachChannel(PIN_BACKLIGHT_PWM, LEDC_BACKLIGHT_FREQ, LEDC_BACKLIGHT_RES, LEDC_BACKLIGHT_CH);
#else
  ledcSetup(LEDC_BACKLIGHT_CH, LEDC_BACKLIGHT_FREQ, LEDC_BACKLIGHT_RES);
  ledcAttachPin(PIN_BACKLIGHT_PWM, LEDC_BACKLIGHT_CH);
#endif
  setBrightness(_brightness_pct);
}

void BacklightManager::setBrightness(uint8_t percent) {
  _brightness_pct = constrain(percent, (uint8_t)0, (uint8_t)100);
  uint32_t duty = (_brightness_pct * 255) / 100;
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
  ledcWrite(PIN_BACKLIGHT_PWM, duty);
#else
  ledcWrite(LEDC_BACKLIGHT_CH, duty);
#endif
}

uint8_t BacklightManager::getBrightness() const {
  return _brightness_pct;
}
