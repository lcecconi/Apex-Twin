#include "track_manager.h"
#include <cstdio>
#include <cstring>

void TrackManager::begin() {
  loadDefaultTracks();
  loadTracksFromSD();
}

void TrackManager::loadDefaultTracks() {
  _tracks.clear();

  // 1. South Garda Karting (Lonato, Italy) - 3 Sectors (Finish + 2 Splits)
  TrackDefinition lonato;
  strncpy(lonato.id, "lonato", sizeof(lonato.id) - 1);
  strncpy(lonato.name, "South Garda Karting", sizeof(lonato.name) - 1);
  strncpy(lonato.location, "Lonato, Italy", sizeof(lonato.location) - 1);
  lonato.length_m = 1200;
  lonato.finish_line = SplitGate(45.388712, 10.479521, 88.5f, 12.0f);
  lonato.intermediate_splits.push_back(SplitGate(45.389240, 10.481100, 172.0f, 10.0f));
  lonato.intermediate_splits.push_back(SplitGate(45.387950, 10.480210, 265.0f, 10.0f));
  lonato.is_custom_sd = false;
  _tracks.push_back(lonato);

  // 2. Circuito Internazionale 7 Laghi (Castelletto di Branduzzo, Italy)
  TrackDefinition castelletto;
  strncpy(castelletto.id, "castelletto", sizeof(castelletto.id) - 1);
  strncpy(castelletto.name, "7 Laghi Kart", sizeof(castelletto.name) - 1);
  strncpy(castelletto.location, "Castelletto, Italy", sizeof(castelletto.location) - 1);
  castelletto.length_m = 1256;
  castelletto.finish_line = SplitGate(45.067320, 9.098710, 110.0f, 12.0f);
  castelletto.intermediate_splits.push_back(SplitGate(45.068150, 9.100420, 205.0f, 10.0f));
  castelletto.intermediate_splits.push_back(SplitGate(45.066800, 9.099150, 290.0f, 10.0f));
  castelletto.is_custom_sd = false;
  _tracks.push_back(castelletto);

  // 3. Karting Genk "Home of Champions" (Genk, Belgium)
  TrackDefinition genk;
  strncpy(genk.id, "genk", sizeof(genk.id) - 1);
  strncpy(genk.name, "Karting Genk", sizeof(genk.name) - 1);
  strncpy(genk.location, "Genk, Belgium", sizeof(genk.location) - 1);
  genk.length_m = 1360;
  genk.finish_line = SplitGate(50.963450, 5.548210, 45.0f, 12.0f);
  genk.intermediate_splits.push_back(SplitGate(50.964100, 5.550100, 135.0f, 10.0f));
  genk.intermediate_splits.push_back(SplitGate(50.962800, 5.549300, 225.0f, 10.0f));
  genk.is_custom_sd = false;
  _tracks.push_back(genk);

  // 4. Circuit International de Salbris (Salbris, France)
  TrackDefinition salbris;
  strncpy(salbris.id, "salbris", sizeof(salbris.id) - 1);
  strncpy(salbris.name, "Salbris Karting", sizeof(salbris.name) - 1);
  strncpy(salbris.location, "Salbris, France", sizeof(salbris.location) - 1);
  salbris.length_m = 1477;
  salbris.finish_line = SplitGate(47.432810, 2.051400, 95.0f, 12.0f);
  salbris.intermediate_splits.push_back(SplitGate(47.433500, 2.053200, 180.0f, 10.0f));
  salbris.intermediate_splits.push_back(SplitGate(47.431900, 2.052100, 275.0f, 10.0f));
  salbris.is_custom_sd = false;
  _tracks.push_back(salbris);

  // 5. Prokart Raceland Wackersdorf (Wackersdorf, Germany)
  TrackDefinition wackersdorf;
  strncpy(wackersdorf.id, "wackersdorf", sizeof(wackersdorf.id) - 1);
  strncpy(wackersdorf.name, "Prokart Wackersdorf", sizeof(wackersdorf.name) - 1);
  strncpy(wackersdorf.location, "Wackersdorf, Germany", sizeof(wackersdorf.location) - 1);
  wackersdorf.length_m = 1190;
  wackersdorf.finish_line = SplitGate(49.314200, 12.181300, 70.0f, 12.0f);
  wackersdorf.intermediate_splits.push_back(SplitGate(49.315000, 12.183100, 160.0f, 10.0f));
  wackersdorf.intermediate_splits.push_back(SplitGate(49.313500, 12.182000, 250.0f, 10.0f));
  wackersdorf.is_custom_sd = false;
  _tracks.push_back(wackersdorf);
}

void TrackManager::loadTracksFromSD() {
}

size_t TrackManager::getTrackCount() const {
  return _tracks.size();
}

const TrackDefinition* TrackManager::getTrack(size_t index) const {
  if (index >= _tracks.size()) return nullptr;
  return &_tracks[index];
}

const TrackDefinition* TrackManager::getTrackById(const char *id) const {
  if (!id) return nullptr;
  for (const auto &track : _tracks) {
    if (strcmp(track.id, id) == 0) {
      return &track;
    }
  }
  return nullptr;
}

const TrackDefinition* TrackManager::getActiveTrack() const {
  if (_tracks.empty()) return nullptr;
  return &_tracks[_activeTrackIndex];
}

bool TrackManager::setActiveTrack(size_t index) {
  if (index >= _tracks.size()) return false;
  _activeTrackIndex = index;
  return true;
}

bool TrackManager::setActiveTrackById(const char *id) {
  if (!id) return false;
  for (size_t i = 0; i < _tracks.size(); i++) {
    if (strcmp(_tracks[i].id, id) == 0) {
      _activeTrackIndex = i;
      return true;
    }
  }
  return false;
}
