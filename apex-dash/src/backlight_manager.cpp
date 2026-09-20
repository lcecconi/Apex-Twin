#include "backlight_manager.h"
#include "driver/ledc.h"
#include "driver/gpio.h"

void BacklightManager::begin() {
  ledc_timer_config_t ledc_timer = {};
  ledc_timer.speed_mode = LEDC_LOW_SPEED_MODE;
  ledc_timer.timer_num = LEDC_TIMER_0;
  ledc_timer.duty_resolution = LEDC_TIMER_8_BIT;
  ledc_timer.freq_hz = LEDC_BACKLIGHT_FREQ;
  ledc_timer.clk_cfg = LEDC_AUTO_CLK;
  ledc_timer_config(&ledc_timer);

  ledc_channel_config_t ledc_channel = {};
  ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
  ledc_channel.channel = LEDC_CHANNEL_0;
  ledc_channel.timer_sel = LEDC_TIMER_0;
  ledc_channel.gpio_num = PIN_BACKLIGHT_PWM;
  ledc_channel.duty = 0;
  ledc_channel.hpoint = 0;
  ledc_channel_config(&ledc_channel);

  setBrightness(_brightness_pct);
}

void BacklightManager::setBrightness(uint8_t percent) {
  _brightness_pct = (percent > 100) ? 100 : percent;
  uint32_t duty = (_brightness_pct * 255) / 100;
  ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

uint8_t BacklightManager::getBrightness() const {
  return _brightness_pct;
}
