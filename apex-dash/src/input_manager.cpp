#include "input_manager.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define LONG_PRESS_THRESHOLD_MS 450
#define DEBOUNCE_MS             30

void InputManager::begin() {
  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = (1ULL << PIN_BTN_BOOT) | (1ULL << PIN_BTN_KEY);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  gpio_config(&io_conf);
}

UserInputEvent InputManager::update() {
  UserInputEvent event = INPUT_NONE;
  uint32_t now = (uint32_t)(esp_timer_get_time() / 1000ULL);

  bool key_raw = (gpio_get_level((gpio_num_t)PIN_BTN_KEY) == 0);   // Pressed = true
  bool boot_raw = (gpio_get_level((gpio_num_t)PIN_BTN_BOOT) == 0); // Pressed = true

  // --- KEY BUTTON (GPIO 18) ---
  if (key_raw && !_last_key_state) {
    _key_press_start_ms = now;
    _key_long_handled = false;
  } else if (key_raw && _last_key_state) {
    if (!_key_long_handled && (now - _key_press_start_ms >= LONG_PRESS_THRESHOLD_MS)) {
      event = INPUT_SELECT; // Long press = Select / Enter
      _key_long_handled = true;
    }
  } else if (!key_raw && _last_key_state) {
    if (!_key_long_handled && (now - _key_press_start_ms >= DEBOUNCE_MS)) {
      event = INPUT_NEXT; // Short press = Next / Down
    }
  }
  _last_key_state = key_raw;

  if (event != INPUT_NONE) {
    return event;
  }

  // --- BOOT BUTTON (GPIO 0) ---
  if (boot_raw && !_last_boot_state) {
    _boot_press_start_ms = now;
    _boot_long_handled = false;
  } else if (boot_raw && _last_boot_state) {
    if (!_boot_long_handled && (now - _boot_press_start_ms >= LONG_PRESS_THRESHOLD_MS)) {
      event = INPUT_BACK_MENU; // Long press = Menu / Back
      _boot_long_handled = true;
    }
  } else if (!boot_raw && _last_boot_state) {
    if (!_boot_long_handled && (now - _boot_press_start_ms >= DEBOUNCE_MS)) {
      event = INPUT_PREV; // Short press = Prev / Up
    }
  }
  _last_boot_state = boot_raw;

  return event;
}
