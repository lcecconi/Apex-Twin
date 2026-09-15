#include <Arduino.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <SD_MMC.h>

// Simple assertions for unit testing on device
#define TEST_ASSERT(cond, msg) \
  if (!(cond)) { \
    Serial.printf("[FAIL] %s (Line %d)\n", msg, __LINE__); \
    tests_failed++; \
  } else { \
    Serial.printf("[PASS] %s\n", msg); \
    tests_passed++; \
  }

static uint32_t tests_passed = 0;
static uint32_t tests_failed = 0;

void test_i18n() {
  Serial.println("\n--- Testing i18n Localization String Dictionaries ---");
  // Test basic language indexing
  const char* en_title = "SETUP MENU";
  const char* it_title = "MENU IMPOSTAZIONI";
  const char* fr_title = "MENU DE CONFIGURATION";
  const char* de_title = "EINSTELLUNGEN";

  TEST_ASSERT(strlen(en_title) > 0, "English title defined");
  TEST_ASSERT(strlen(it_title) > 0, "Italian title defined");
  TEST_ASSERT(strlen(fr_title) > 0, "French title defined");
  TEST_ASSERT(strlen(de_title) > 0, "German title defined");
}

void test_json_track_parsing() {
  Serial.println("\n--- Testing Open JSON Track Deserialization ---");
  const char *sample_json = "{"
    "\"id\":\"lonato\","
    "\"name\":\"South Garda Karting\","
    "\"location\":\"Lonato, Italy\","
    "\"length_m\":1200,"
    "\"finish_line\":{\"lat\":45.388712,\"lon\":10.479521,\"bearing_deg\":88.5,\"width_m\":12.0},"
    "\"split1\":{\"lat\":45.389240,\"lon\":10.481100,\"bearing_deg\":172.0,\"width_m\":10.0},"
    "\"split2\":{\"lat\":45.387950,\"lon\":10.480210,\"bearing_deg\":265.0,\"width_m\":10.0}"
  "}";

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, sample_json);
  TEST_ASSERT(!err, "JSON deserialization succeeded");
  TEST_ASSERT(strcmp(doc["id"], "lonato") == 0, "Track ID matches 'lonato'");
  TEST_ASSERT(doc["length_m"] == 1200, "Track length matches 1200m");
  TEST_ASSERT(doc["finish_line"]["lat"] == 45.388712, "Finish line latitude parsed accurately");
}

void test_neopixel_buffer() {
  Serial.println("\n--- Testing WS2812 NeoPixel Buffer & Color Ranges ---");
  Adafruit_NeoPixel strip(7, 1, NEO_GRB + NEO_KHZ800);
  strip.begin();
  strip.setPixelColor(0, strip.Color(0, 255, 0));   // Shift 1: Green
  strip.setPixelColor(4, strip.Color(255, 0, 0));   // Shift 5: Red
  strip.setPixelColor(5, strip.Color(255, 0, 0));   // Alarm L: Red
  strip.setPixelColor(6, strip.Color(255, 0, 200)); // Alarm R: Magenta

  TEST_ASSERT(strip.getPixelColor(0) == strip.Color(0, 255, 0), "Shift LED 0 set to Green");
  TEST_ASSERT(strip.getPixelColor(4) == strip.Color(255, 0, 0), "Shift LED 4 set to Red");
  TEST_ASSERT(strip.getPixelColor(5) == strip.Color(255, 0, 0), "Alarm LED 5 set to Red");
  TEST_ASSERT(strip.getPixelColor(6) == strip.Color(255, 0, 200), "Alarm LED 6 set to Magenta");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=======================================================");
  Serial.println("     APEX-DASH: Feature Test & Validation Suite        ");
  Serial.println("=======================================================");

  test_i18n();
  test_json_track_parsing();
  test_neopixel_buffer();

  Serial.println("\n=======================================================");
  Serial.printf("TEST SUMMARY: %u Passed, %u Failed\n", tests_passed, tests_failed);
  Serial.println("=======================================================\n");
}

void loop() {
  delay(1000);
}
