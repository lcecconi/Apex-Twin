/**
 * @file main.cpp
 * Apex-Dash Main Application Entry Point (Native ESP-IDF v5 & LVGL v9)
 */

#include <cstdio>
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include <lvgl.h>

#include "config.h"
#include "telemetry_data.h"
#include "display_st7305.h"
#include "apex_led_strip.h"
#include "esp_now_transport.h"
#include "ui/ui_manager.h"

static const char *TAG = "APEX_DASH";

static TelemetrySnapshot g_telemetry;
static SystemSettings    g_settings;
static ApexUi::UiManager g_ui_mgr;
static ApexLeds::LedStripRmt g_led_strip;
static ApexTransport::EspNowTransport g_esp_now;

static void ui_task(void *pvParameters) {
    (void)pvParameters;
    ESP_LOGI(TAG, "UI & Telemetry task started on Core 1.");
    uint32_t last_telemetry_tick = 0;

    while (1) {
        // Poll Buttons (Active Low)
        static int prev_btn_key = 1;
        int btn_key = gpio_get_level((gpio_num_t)PIN_BTN_KEY);
        if (btn_key == 0 && prev_btn_key == 1) {
            g_ui_mgr.nextView();
            ESP_LOGI(TAG, "Key button pressed: switched to view %u", g_ui_mgr.getViewMode());
        }
        prev_btn_key = btn_key;

        static int prev_btn_boot = 1;
        int btn_boot = gpio_get_level((gpio_num_t)PIN_BTN_BOOT);
        if (btn_boot == 0 && prev_btn_boot == 1) {
            g_telemetry.chassis.lap_number++;
            g_telemetry.chassis.current_lap_time_ms = 0;
            ESP_LOGI(TAG, "Boot button pressed: lap marked L%02u", g_telemetry.chassis.lap_number);
        }
        prev_btn_boot = btn_boot;

        uint32_t now = (uint32_t)(esp_timer_get_time() / 1000ULL);
        if (now - last_telemetry_tick >= 40) { // 25 Hz
            last_telemetry_tick = now;
            g_esp_now.update();
            g_telemetry.syncFlatFields();
            g_ui_mgr.update(g_telemetry, g_settings);
            g_led_strip.update(g_telemetry, g_settings);
        }

        uint32_t time_till_next = lv_timer_handler();
        if (time_till_next < 5) time_till_next = 5;
        if (time_till_next > 20) time_till_next = 20;
        vTaskDelay(pdMS_TO_TICKS(time_till_next));
    }
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "=======================================================");
    ESP_LOGI(TAG, "   APEX-DASH: Native ESP-IDF v5 + LVGL v9 Kart Dash   ");
    ESP_LOGI(TAG, "      (Cold Boot Time: < 200ms | 25-50 Hz CAN-FD)      ");
    ESP_LOGI(TAG, "=======================================================");

    // 1. Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Initialize GPIO button inputs with internal pull-up
    gpio_config_t btn_conf = {};
    btn_conf.intr_type = GPIO_INTR_DISABLE;
    btn_conf.mode = GPIO_MODE_INPUT;
    btn_conf.pin_bit_mask = (1ULL << PIN_BTN_BOOT) | (1ULL << PIN_BTN_KEY);
    btn_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    btn_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&btn_conf);

    // 3. Initialize LVGL v9
    lv_init();

    // 4. Initialize ST7305 RLCD hardware & display
    lv_display_t *disp = ApexDisplay::init_lvgl_display();
    if (!disp) {
        ESP_LOGE(TAG, "Failed to initialize ST7305 display!");
    }

    // 5. Initialize UI Manager on active screen
    g_ui_mgr.init(lv_screen_active(), true);

    // 6. Initialize RMT WS2812B LEDs
    g_led_strip.init();

    // 7. Initialize ESP-NOW Virtual CAN-FD receiver
    g_esp_now.init(&g_telemetry.chassis);

    // 8. Launch UI & Telemetry task pinned to Core 1
    xTaskCreatePinnedToCore(ui_task, "ui_task", 8192, nullptr, 5, nullptr, 1);

    ESP_LOGI(TAG, "Apex-Dash firmware startup sequence completed successfully.");
}
