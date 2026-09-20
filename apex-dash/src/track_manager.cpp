#include "track_manager.h"
#include "esp_log.h"
#include "cJSON.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <dirent.h>

static const char *TAG = "TRACK_MGR";

void TrackManager::begin() {
  loadDefaultTracks();
  seedTracksToSD();
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

bool TrackManager::saveTrackToSD(const TrackDefinition &track) {
  struct stat st;
  if (stat("/sdcard/tracks", &st) != 0) return false;

  cJSON *root = cJSON_CreateObject();
  if (!root) return false;

  cJSON_AddStringToObject(root, "id", track.id);
  cJSON_AddStringToObject(root, "name", track.name);
  cJSON_AddStringToObject(root, "location", track.location);
  cJSON_AddNumberToObject(root, "length_m", track.length_m);

  cJSON *finish = cJSON_CreateObject();
  cJSON_AddNumberToObject(finish, "lat", track.finish_line.lat);
  cJSON_AddNumberToObject(finish, "lon", track.finish_line.lon);
  cJSON_AddNumberToObject(finish, "heading_deg", track.finish_line.heading_deg);
  cJSON_AddNumberToObject(finish, "width_m", track.finish_line.width_m);
  cJSON_AddItemToObject(root, "finish_line", finish);

  cJSON *splits_arr = cJSON_CreateArray();
  for (const auto &sp : track.intermediate_splits) {
    cJSON *split = cJSON_CreateObject();
    cJSON_AddNumberToObject(split, "lat", sp.lat);
    cJSON_AddNumberToObject(split, "lon", sp.lon);
    cJSON_AddNumberToObject(split, "heading_deg", sp.heading_deg);
    cJSON_AddNumberToObject(split, "width_m", sp.width_m);
    cJSON_AddItemToArray(splits_arr, split);
  }
  cJSON_AddItemToObject(root, "splits", splits_arr);

  char *json_str = cJSON_Print(root);
  cJSON_Delete(root);

  if (!json_str) return false;

  char file_path[128];
  snprintf(file_path, sizeof(file_path), "/sdcard/tracks/%s.json", track.id);

  FILE *f = fopen(file_path, "w");
  if (!f) {
    free(json_str);
    return false;
  }

  fputs(json_str, f);
  fclose(f);
  free(json_str);
  ESP_LOGI(TAG, "Saved track definition to %s", file_path);
  return true;
}

void TrackManager::seedTracksToSD() {
  struct stat st;
  if (stat("/sdcard/tracks", &st) != 0) return;

  for (const auto &track : _tracks) {
    char path[128];
    snprintf(path, sizeof(path), "/sdcard/tracks/%s.json", track.id);
    if (stat(path, &st) != 0) {
      // File does not exist, seed it
      saveTrackToSD(track);
    }
  }
}

void TrackManager::loadTracksFromSD() {
  DIR *dir = opendir("/sdcard/tracks");
  if (!dir) return;

  struct dirent *ent;
  while ((ent = readdir(dir)) != nullptr) {
    size_t len = strlen(ent->d_name);
    if (len < 5 || strcmp(ent->d_name + len - 5, ".json") != 0) {
      continue;
    }

    char file_path[300];
    snprintf(file_path, sizeof(file_path), "/sdcard/tracks/%s", ent->d_name);

    FILE *f = fopen(file_path, "r");
    if (!f) continue;

    fseek(f, 0, SEEK_END);
    long f_len = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (f_len <= 0 || f_len > 32768) {
      fclose(f);
      continue;
    }

    char *buf = (char *)malloc(f_len + 1);
    if (!buf) {
      fclose(f);
      continue;
    }

    size_t r = fread(buf, 1, f_len, f);
    fclose(f);
    buf[r] = '\0';

    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) continue;

    TrackDefinition track;
    cJSON *item = cJSON_GetObjectItem(root, "id");
    if (item && cJSON_IsString(item) && item->valuestring) {
      strncpy(track.id, item->valuestring, sizeof(track.id) - 1);
    } else {
      // Use filename stem as id
      strncpy(track.id, ent->d_name, sizeof(track.id) - 1);
      char *dot = strrchr(track.id, '.');
      if (dot) *dot = '\0';
    }

    item = cJSON_GetObjectItem(root, "name");
    if (item && cJSON_IsString(item) && item->valuestring) {
      strncpy(track.name, item->valuestring, sizeof(track.name) - 1);
    }

    item = cJSON_GetObjectItem(root, "location");
    if (item && cJSON_IsString(item) && item->valuestring) {
      strncpy(track.location, item->valuestring, sizeof(track.location) - 1);
    }

    item = cJSON_GetObjectItem(root, "length_m");
    if (item && cJSON_IsNumber(item)) {
      track.length_m = (uint16_t)item->valueint;
    }

    cJSON *finish = cJSON_GetObjectItem(root, "finish_line");
    if (finish) {
      cJSON *lat = cJSON_GetObjectItem(finish, "lat");
      cJSON *lon = cJSON_GetObjectItem(finish, "lon");
      cJSON *hdg = cJSON_GetObjectItem(finish, "heading_deg");
      cJSON *w = cJSON_GetObjectItem(finish, "width_m");
      if (lat && lon) {
        track.finish_line.lat = lat->valuedouble;
        track.finish_line.lon = lon->valuedouble;
        if (hdg) track.finish_line.heading_deg = (float)hdg->valuedouble;
        if (w) track.finish_line.width_m = (float)w->valuedouble;
      }
    }

    cJSON *splits = cJSON_GetObjectItem(root, "splits");
    if (splits && cJSON_IsArray(splits)) {
      int count = cJSON_GetArraySize(splits);
      for (int i = 0; i < count; i++) {
        cJSON *sp = cJSON_GetArrayItem(splits, i);
        if (sp) {
          cJSON *lat = cJSON_GetObjectItem(sp, "lat");
          cJSON *lon = cJSON_GetObjectItem(sp, "lon");
          cJSON *hdg = cJSON_GetObjectItem(sp, "heading_deg");
          cJSON *w = cJSON_GetObjectItem(sp, "width_m");
          if (lat && lon) {
            SplitGate gate(lat->valuedouble, lon->valuedouble,
                           hdg ? (float)hdg->valuedouble : 0.0f,
                           w ? (float)w->valuedouble : 10.0f);
            track.intermediate_splits.push_back(gate);
          }
        }
      }
    }

    cJSON_Delete(root);

    // Check if track already exists in list (by id)
    bool found = false;
    for (size_t i = 0; i < _tracks.size(); i++) {
      if (strcmp(_tracks[i].id, track.id) == 0) {
        _tracks[i] = track;
        found = true;
        break;
      }
    }
    if (!found) {
      track.is_custom_sd = true;
      _tracks.push_back(track);
      ESP_LOGI(TAG, "Loaded custom track from SD: %s (%s)", track.name, track.id);
    }
  }

  closedir(dir);
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

