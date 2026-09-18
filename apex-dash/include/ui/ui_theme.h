/**
 * @file ui_theme.h
 * High-contrast Reflective Memory LCD (RLCD) theme for Apex-Dash (LVGL v9)
 */

#pragma once

#include <lvgl.h>
#include <cstdint>

namespace ApexUi {

// Authentic Reflective LCD Colors
static inline lv_color_t color_bg() {
    return lv_color_hex(0xD5DDD2); // Reflective silver-green paper
}

static inline lv_color_t color_fg() {
    return lv_color_hex(0x141C14); // Deep charcoal/black liquid crystal
}

static inline lv_color_t color_inverted_bg() {
    return lv_color_hex(0x141C14);
}

static inline lv_color_t color_inverted_fg() {
    return lv_color_hex(0xD5DDD2);
}

class UiTheme {
public:
    static void init(bool inverted = true);

    static lv_color_t bg();
    static lv_color_t fg();

    static lv_style_t style_screen;
    static lv_style_t style_card;
    static lv_style_t style_card_title;
    static lv_style_t style_card_title_text;
    static lv_style_t style_badge_inverted;
    static lv_style_t style_text_small;
    static lv_style_t style_text_normal;
    static lv_style_t style_text_bold;
    static lv_style_t style_text_large;
    static lv_style_t style_text_huge;

private:
    static bool _inverted;
};

} // namespace ApexUi
