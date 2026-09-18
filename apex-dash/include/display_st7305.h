/**
 * @file display_st7305.h
 * Native ESP-IDF SPI Driver for Sitronix ST7305 400x240 Reflective LCD with LVGL v9
 */

#pragma once

#include <cstdint>
#include <lvgl.h>

#ifdef ESP_PLATFORM
#include "esp_err.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#endif

namespace ApexDisplay {

#define LCD_HOR_RES 400
#define LCD_VER_RES 240

/**
 * Initialize ST7305 SPI hardware, GPIOs, and register LVGL v9 display.
 * @return lv_display_t* pointer on success, nullptr on failure.
 */
lv_display_t *init_lvgl_display();

/**
 * Toggle display hardware color inversion (High-contrast Black on Silver vs Silver on Black).
 */
void set_invert(bool invert);

/**
 * LVGL v9 flush callback for rendering dirty rectangles onto ST7305.
 */
void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);

} // namespace ApexDisplay
