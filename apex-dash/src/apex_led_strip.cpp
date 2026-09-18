#include "apex_led_strip.h"
#include "config.h"

#ifdef ESP_PLATFORM
#include "led_strip.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "LED_RMT";

namespace ApexLeds {

void LedStripRmt::init() {
    ESP_LOGI(TAG, "Initializing RMT WS2812B LED strip (Pin %d, %d LEDs)...", PIN_RGB_LED_STRIP, NUM_TOTAL_LEDS);

    led_strip_config_t strip_config = {};
    strip_config.strip_gpio_num = PIN_RGB_LED_STRIP;
    strip_config.max_leds = NUM_TOTAL_LEDS;
    strip_config.led_pixel_format = LED_PIXEL_FORMAT_GRB;
    strip_config.led_model = LED_MODEL_WS2812;

    led_strip_rmt_config_t rmt_config = {};
    rmt_config.resolution_hz = 10 * 1000 * 1000; // 10 MHz

    led_strip_handle_t handle = nullptr;
    esp_err_t ret = led_strip_new_rmt_device(&strip_config, &rmt_config, &handle);
    if (ret == ESP_OK) {
        _strip_handle = (void *)handle;
        clear();
        ESP_LOGI(TAG, "RMT LED strip initialized successfully.");
    } else {
        ESP_LOGE(TAG, "Failed to initialize RMT LED strip: %s", esp_err_to_name(ret));
    }
}

void LedStripRmt::setPixel(uint32_t index, uint8_t r, uint8_t g, uint8_t b) {
    if (!_strip_handle || index >= NUM_TOTAL_LEDS) return;
    led_strip_set_pixel((led_strip_handle_t)_strip_handle, index, r, g, b);
}

void LedStripRmt::refresh() {
    if (!_strip_handle) return;
    led_strip_refresh((led_strip_handle_t)_strip_handle);
}

void LedStripRmt::clear() {
    if (!_strip_handle) return;
    led_strip_clear((led_strip_handle_t)_strip_handle);
}

void LedStripRmt::update(const TelemetrySnapshot &t, const SystemSettings &s) {
    if (!_strip_handle) return;

    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000ULL);
    if (now - _flash_timer_ms >= 120) {
        _flash_timer_ms = now;
        _flash_state = !_flash_state;
    }

    float brightness_scale = (float)s.led_brightness / 100.0f;
    uint8_t br_r = (uint8_t)(255 * brightness_scale);
    uint8_t br_g = (uint8_t)(255 * brightness_scale);
    uint8_t br_b = (uint8_t)(255 * brightness_scale);

    // 1. Shift RPM Bar (LEDs 0..4)
    if (t.rpm >= s.over_rev_rpm) {
        // Flash all blue/magenta
        uint8_t val = _flash_state ? br_b : 0;
        for (int i = 0; i < NUM_SHIFT_LEDS; i++) {
            setPixel(i, val, 0, val);
        }
    } else if (t.rpm >= s.shift_rpm) {
        // Flash all red
        uint8_t val = _flash_state ? br_r : 0;
        for (int i = 0; i < NUM_SHIFT_LEDS; i++) {
            setPixel(i, val, 0, 0);
        }
    } else {
        uint16_t rpm_span = s.shift_rpm - 8000;
        float progress = (t.rpm > 8000) ? ((float)(t.rpm - 8000) / (float)rpm_span) : 0.0f;
        int active_leds = (int)(progress * NUM_SHIFT_LEDS);

        for (int i = 0; i < NUM_SHIFT_LEDS; i++) {
            if (i < active_leds) {
                if (i < 2) {
                    setPixel(i, 0, br_g, 0); // Green
                } else if (i < 4) {
                    setPixel(i, br_r, (uint8_t)(br_g * 0.7f), 0); // Amber/Yellow
                } else {
                    setPixel(i, br_r, 0, 0); // Red
                }
            } else {
                setPixel(i, 0, 0, 0);
            }
        }
    }

    // 2. Alarm LEDs (LED 5: Left, LED 6: Right)
    bool water_alarm = (t.water_temp_c >= s.water_temp_alarm_c && s.water_temp_alarm_c > 0);
    bool egt_alarm   = (t.exhaust_temp_c >= s.exhaust_temp_alarm_c && s.exhaust_temp_alarm_c > 0);

    if (water_alarm) {
        setPixel(5, _flash_state ? br_r : 0, 0, 0);
    } else {
        setPixel(5, 0, 0, 0);
    }

    if (egt_alarm) {
        setPixel(6, _flash_state ? br_r : 0, 0, 0);
    } else {
        setPixel(6, 0, 0, 0);
    }

    refresh();
}

} // namespace ApexLeds

#else // Non-ESP desktop stub

namespace ApexLeds {
void LedStripRmt::init() {}
void LedStripRmt::update(const TelemetrySnapshot &, const SystemSettings &) {}
void LedStripRmt::clear() {}
void LedStripRmt::setPixel(uint32_t, uint8_t, uint8_t, uint8_t) {}
void LedStripRmt::refresh() {}
}

#endif
