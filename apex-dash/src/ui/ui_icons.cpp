#include "ui/ui_icons.h"
#include "ui/icons_xbm.h"

static uint8_t buf_water[256];
static uint8_t buf_egt[256];
static uint8_t buf_rev[256];
static uint8_t buf_bat[256];
static uint8_t buf_link[256];
static uint8_t buf_gear[256];
static uint8_t buf_lightbulb[256];
static uint8_t buf_flag[256];
static uint8_t buf_sdcard[256];
static uint8_t buf_sun[256];
static uint8_t buf_globe[256];
static uint8_t buf_wrench[256];
static uint8_t buf_back[256];
static uint8_t buf_engine[256];
static uint8_t buf_stopwatch[256];
static uint8_t buf_warn[576];

lv_image_dsc_t img_icon_water_16x16;
lv_image_dsc_t img_icon_egt_16x16;
lv_image_dsc_t img_icon_rev_16x16;
lv_image_dsc_t img_icon_bat_16x16;
lv_image_dsc_t img_icon_link_16x16;
lv_image_dsc_t img_icon_gear_16x16;
lv_image_dsc_t img_icon_lightbulb_16x16;
lv_image_dsc_t img_icon_flag_16x16;
lv_image_dsc_t img_icon_sdcard_16x16;
lv_image_dsc_t img_icon_sun_16x16;
lv_image_dsc_t img_icon_globe_16x16;
lv_image_dsc_t img_icon_wrench_16x16;
lv_image_dsc_t img_icon_back_16x16;
lv_image_dsc_t img_icon_engine_16x16;
lv_image_dsc_t img_icon_stopwatch_16x16;
lv_image_dsc_t img_icon_warn_24x24;

static void xbm_to_a8(const uint8_t *xbm, uint8_t *a8, uint32_t w, uint32_t h, lv_image_dsc_t &dsc) {
    uint32_t stride = (w + 7) / 8;
    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w; x++) {
            uint32_t byte_idx = y * stride + (x / 8);
            uint32_t bit_idx = x % 8;
            bool set = (xbm[byte_idx] & (1 << bit_idx)) != 0;
            a8[y * w + x] = set ? 0xFF : 0x00;
        }
    }
    dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    dsc.header.cf = LV_COLOR_FORMAT_A8;
    dsc.header.flags = 0;
    dsc.header.w = w;
    dsc.header.h = h;
    dsc.header.stride = w;
    dsc.data_size = w * h;
    dsc.data = a8;
    dsc.reserved = nullptr;
}

void init_ui_icons() {
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

    xbm_to_a8(icon_water_16x16, buf_water, 16, 16, img_icon_water_16x16);
    xbm_to_a8(icon_egt_16x16, buf_egt, 16, 16, img_icon_egt_16x16);
    xbm_to_a8(icon_rev_16x16, buf_rev, 16, 16, img_icon_rev_16x16);
    xbm_to_a8(icon_bat_16x16, buf_bat, 16, 16, img_icon_bat_16x16);
    xbm_to_a8(icon_link_16x16, buf_link, 16, 16, img_icon_link_16x16);
    xbm_to_a8(icon_gear_16x16, buf_gear, 16, 16, img_icon_gear_16x16);
    xbm_to_a8(icon_lightbulb_16x16, buf_lightbulb, 16, 16, img_icon_lightbulb_16x16);
    xbm_to_a8(icon_flag_16x16, buf_flag, 16, 16, img_icon_flag_16x16);
    xbm_to_a8(icon_sdcard_16x16, buf_sdcard, 16, 16, img_icon_sdcard_16x16);
    xbm_to_a8(icon_sun_16x16, buf_sun, 16, 16, img_icon_sun_16x16);
    xbm_to_a8(icon_globe_16x16, buf_globe, 16, 16, img_icon_globe_16x16);
    xbm_to_a8(icon_wrench_16x16, buf_wrench, 16, 16, img_icon_wrench_16x16);
    xbm_to_a8(icon_back_16x16, buf_back, 16, 16, img_icon_back_16x16);
    xbm_to_a8(icon_engine_16x16, buf_engine, 16, 16, img_icon_engine_16x16);
    xbm_to_a8(icon_stopwatch_16x16, buf_stopwatch, 16, 16, img_icon_stopwatch_16x16);
    xbm_to_a8(icon_warn_24x24, buf_warn, 24, 24, img_icon_warn_24x24);
}
