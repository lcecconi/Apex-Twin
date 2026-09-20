#pragma once

#include <cstdint>
#include "telemetry_data.h"

class StorageManager {
public:
  void begin();
  void loadSettings(SystemSettings &settings);
  void saveSettings(const SystemSettings &settings);
  void syncSDCard(SystemSettings &settings);
  bool loadSettingsFromSD(SystemSettings &settings);
  bool saveSettingsToSD(const SystemSettings &settings);

  uint32_t getEngineHours();
  void saveEngineHours(uint32_t seconds);
  void resetEngineHours();
};

