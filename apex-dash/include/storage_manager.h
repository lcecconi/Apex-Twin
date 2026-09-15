#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "telemetry_data.h"

class StorageManager {
public:
  void begin();
  void loadSettings(SystemSettings &settings);
  void saveSettings(const SystemSettings &settings);

private:
  Preferences _prefs;
};
