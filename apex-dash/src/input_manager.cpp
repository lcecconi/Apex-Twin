#include "input_manager.h"

#define LONG_PRESS_THRESHOLD_MS 450
#define DEBOUNCE_MS             30

void InputManager::begin() {
  pinMode(PIN_BTN_BOOT, INPUT_PULLUP);
  pinMode(PIN_BTN_KEY, INPUT_PULLUP);
}

UserInputEvent InputManager::update() {
  UserInputEvent event = INPUT_NONE;
  uint32_t now = millis();

  bool key_raw = (digitalRead(PIN_BTN_KEY) == LOW);   // Pressed = true
  bool boot_raw = (digitalRead(PIN_BTN_BOOT) == LOW); // Pressed = true

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
