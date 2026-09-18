/**
 * @file display_st7305.cpp
 * Native ESP-IDF SPI Driver implementation for Sitronix ST7305 with LVGL v9
 */

#include "display_st7305.h"
#include "config.h"

#ifdef ESP_PLATFORM
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

static const char *TAG = "ST7305";

static spi_device_handle_t s_spi_dev = nullptr;
static uint8_t *s_lvgl_draw_buf1 = nullptr;
static uint8_t *s_lvgl_draw_buf2 = nullptr;

static void st7305_cmd(uint8_t cmd) {
    gpio_set_level((gpio_num_t)PIN_LCD_DC, 0);
    gpio_set_level((gpio_num_t)PIN_LCD_CS, 0);
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.tx_buffer = &cmd;
    spi_device_polling_transmit(s_spi_dev, &t);
    gpio_set_level((gpio_num_t)PIN_LCD_CS, 1);
}

[[maybe_unused]] static void st7305_data(const uint8_t *data, size_t len) {
    if (len == 0) return;
    gpio_set_level((gpio_num_t)PIN_LCD_DC, 1);
    gpio_set_level((gpio_num_t)PIN_LCD_CS, 0);
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = len * 8;
    t.tx_buffer = data;
    spi_device_polling_transmit(s_spi_dev, &t);
    gpio_set_level((gpio_num_t)PIN_LCD_CS, 1);
}

static void st7305_cmd_data(uint8_t cmd, const uint8_t *data, size_t len) {
    gpio_set_level((gpio_num_t)PIN_LCD_DC, 0);
    gpio_set_level((gpio_num_t)PIN_LCD_CS, 0);
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.tx_buffer = &cmd;
    spi_device_polling_transmit(s_spi_dev, &t);
    if (len > 0) {
        gpio_set_level((gpio_num_t)PIN_LCD_DC, 1);
        memset(&t, 0, sizeof(t));
        t.length = len * 8;
        t.tx_buffer = data;
        spi_device_polling_transmit(s_spi_dev, &t);
    }
    gpio_set_level((gpio_num_t)PIN_LCD_CS, 1);
}

static void st7305_reset() {
    gpio_set_level((gpio_num_t)PIN_LCD_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level((gpio_num_t)PIN_LCD_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level((gpio_num_t)PIN_LCD_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
}

static void st7305_full_init() {
    st7305_reset();

    const uint8_t d6[] = {0x17, 0x02};
    const uint8_t d1[] = {0x01};
    const uint8_t c0[] = {0x11, 0x04};
    const uint8_t c1[] = {0x69, 0x69, 0x69, 0x69};
    const uint8_t c2[] = {0x19, 0x19, 0x19, 0x19};
    const uint8_t c4[] = {0x4B, 0x4B, 0x4B, 0x4B};
    const uint8_t d8[] = {0x80, 0xE9};
    const uint8_t b2[] = {0x02};
    const uint8_t b3[] = {0xE5, 0xF6, 0x05, 0x46, 0x77, 0x77, 0x77, 0x77, 0x76, 0x45};
    const uint8_t b4[] = {0x05, 0x46, 0x77, 0x77, 0x77, 0x77, 0x76, 0x45};
    const uint8_t g_timing[] = {0x32, 0x03, 0x1F};
    const uint8_t b7[] = {0x13};
    const uint8_t b0[] = {0x64};
    const uint8_t c9[] = {0x00};
    const uint8_t m36[] = {0x48};
    const uint8_t m3a[] = {0x11};
    const uint8_t b9[] = {0x20};
    const uint8_t b8[] = {0x29};
    const uint8_t win_a[] = {0x12, 0x2A};
    const uint8_t win_b[] = {0x00, 0xC7};
    const uint8_t m35[] = {0x00};
    const uint8_t d0[] = {0xFF};

    st7305_cmd_data(0xD6, d6, sizeof(d6));
    st7305_cmd_data(0xD1, d1, sizeof(d1));
    st7305_cmd_data(0xC0, c0, sizeof(c0));
    st7305_cmd_data(0xC1, c1, sizeof(c1));
    st7305_cmd_data(0xC2, c2, sizeof(c2));
    st7305_cmd_data(0xC4, c4, sizeof(c4));
    st7305_cmd_data(0xC5, c2, sizeof(c2));
    st7305_cmd_data(0xD8, d8, sizeof(d8));
    st7305_cmd_data(0xB2, b2, sizeof(b2));
    st7305_cmd_data(0xB3, b3, sizeof(b3));
    st7305_cmd_data(0xB4, b4, sizeof(b4));
    st7305_cmd_data(0x62, g_timing, sizeof(g_timing));
    st7305_cmd_data(0xB7, b7, sizeof(b7));
    st7305_cmd_data(0xB0, b0, sizeof(b0));
    st7305_cmd(0x11);
    vTaskDelay(pdMS_TO_TICKS(120));
    st7305_cmd_data(0xC9, c9, sizeof(c9));
    st7305_cmd_data(0x36, m36, sizeof(m36));
    st7305_cmd_data(0x3A, m3a, sizeof(m3a));
    st7305_cmd_data(0xB9, b9, sizeof(b9));
    st7305_cmd_data(0xB8, b8, sizeof(b8));
    st7305_cmd(0x20); // INVOFF
    st7305_cmd_data(0x2A, win_a, sizeof(win_a));
    st7305_cmd_data(0x2B, win_b, sizeof(win_b));
    st7305_cmd_data(0x35, m35, sizeof(m35));
    st7305_cmd_data(0xD0, d0, sizeof(d0));
    st7305_cmd(0x38);
    st7305_cmd(0x29);
}

namespace ApexDisplay {

void set_invert(bool invert) {
    st7305_cmd(invert ? 0x20 : 0x21);
}

void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    // ST7305 column and row address windowing
    // Area mapped to landscape 400x240
    int x1 = area->x1;
    int y1 = area->y1;
    int x2 = area->x2;
    int y2 = area->y2;

    int addr_start = 0x12 + (x1 / 12);
    int addr_end   = 0x12 + (x2 / 12);

    uint8_t col_bounds[] = {(uint8_t)(0x3C - addr_end), (uint8_t)(0x3C - addr_start)};
    st7305_cmd_data(0x2A, col_bounds, sizeof(col_bounds));

    uint8_t row_bounds[] = {(uint8_t)y1, (uint8_t)y2};
    st7305_cmd_data(0x2B, row_bounds, sizeof(row_bounds));

    // Send pixel data directly through DMA
    size_t byte_count = (size_t)(x2 - x1 + 1) * (size_t)(y2 - y1 + 1) * 2; // RGB565 / monochrome
    st7305_cmd_data(0x2C, px_map, byte_count);

    lv_display_flush_ready(disp);
}

lv_display_t *init_lvgl_display() {
    ESP_LOGI(TAG, "Configuring ST7305 SPI Master & GPIOs...");

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << PIN_LCD_DC) | (1ULL << PIN_LCD_CS) | (1ULL << PIN_LCD_RST);
    gpio_config(&io_conf);

    gpio_set_level((gpio_num_t)PIN_LCD_CS, 1);
    gpio_set_level((gpio_num_t)PIN_LCD_DC, 1);
    gpio_set_level((gpio_num_t)PIN_LCD_RST, 1);

    spi_bus_config_t buscfg = {};
    buscfg.miso_io_num = -1;
    buscfg.mosi_io_num = PIN_LCD_MOSI;
    buscfg.sclk_io_num = PIN_LCD_SCK;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = LCD_HOR_RES * LCD_VER_RES * 2;

    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return nullptr;
    }

    spi_device_interface_config_t devcfg = {};
    devcfg.clock_speed_hz = 24 * 1000 * 1000;
    devcfg.mode = 0;
    devcfg.spics_io_num = -1; // Manual CS control
    devcfg.queue_size = 7;

    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &s_spi_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        return nullptr;
    }

    st7305_full_init();
    ESP_LOGI(TAG, "ST7305 initialized successfully.");

    // Allocate LVGL display draw buffers
    size_t buf_size = LCD_HOR_RES * 40 * sizeof(lv_color_t);
    s_lvgl_draw_buf1 = (uint8_t *)heap_caps_malloc(buf_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    s_lvgl_draw_buf2 = (uint8_t *)heap_caps_malloc(buf_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

    lv_display_t *disp = lv_display_create(LCD_HOR_RES, LCD_VER_RES);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(disp, lvgl_flush_cb);
    lv_display_set_buffers(disp, s_lvgl_draw_buf1, s_lvgl_draw_buf2, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);

    return disp;
}

} // namespace ApexDisplay

#else // Non-ESP target stub

namespace ApexDisplay {
lv_display_t *init_lvgl_display() { return nullptr; }
void set_invert(bool) {}
void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *, uint8_t *) {
    if (disp) lv_display_flush_ready(disp);
}
}

#endif
