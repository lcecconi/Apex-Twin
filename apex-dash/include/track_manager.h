#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

struct SplitGate {
  double lat;
  double lon;
  float heading_deg;
  float width_m;

  SplitGate(double _lat = 0.0, double _lon = 0.0, float _h = 0.0f, float _w = 12.0f)
    : lat(_lat), lon(_lon), heading_deg(_h), width_m(_w) {}
};

struct TrackDefinition {
  char id[32];
  char name[48];
  char location[48];
  uint16_t length_m;
  SplitGate finish_line;
  SplitGate split1;
  SplitGate split2;
  bool is_custom_sd;

  TrackDefinition() : length_m(0), is_custom_sd(false) {
    id[0] = '\0';
    name[0] = '\0';
    location[0] = '\0';
  }
};

class TrackManager {
public:
  void begin();
  void loadTracksFromSD();
  size_t getTrackCount() const;
  const TrackDefinition* getTrack(size_t index) const;
  const TrackDefinition* getTrackById(const char *id) const;
  const TrackDefinition* getActiveTrack() const;
  bool setActiveTrack(size_t index);
  bool setActiveTrackById(const char *id);

private:
  void loadDefaultTracks();
  std::vector<TrackDefinition> _tracks;
  size_t _activeTrackIndex = 0;
};
