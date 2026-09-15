#pragma once

#include <Arduino.h>
#include "config.h"

enum UserInputEvent : uint8_t {
  INPUT_NONE = 0,
  INPUT_NEXT,      // Next page, scroll down, increment value
  INPUT_PREV,      // Previous page, scroll up, decrement value
  INPUT_SELECT,    // Enter submenu, confirm, toggle parameter
  INPUT_BACK_MENU  // Open menu, go back to previous menu / exit to HUD
};

class InputManager {
public:
  void begin();
  UserInputEvent update();

private:
  bool _last_key_state = true;
  bool _last_boot_state = true;
  uint32_t _key_press_start_ms = 0;
  uint32_t _boot_press_start_ms = 0;
  bool _key_long_handled = false;
  bool _boot_long_handled = false;
};
