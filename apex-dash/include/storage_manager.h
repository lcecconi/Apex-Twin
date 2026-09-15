#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "telemetry_data.h"

class StorageManager {
public:
  void begin();
  void loadSettings(SystemSettings &settings);
  void saveSettings(const SystemSettings &settings);

  uint32_t getEngineHours();
  void saveEngineHours(uint32_t seconds);
  void resetEngineHours();

private:
  Preferences _prefs;
};
