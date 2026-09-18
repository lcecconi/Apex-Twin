/**
 * @file ui_theme.cpp
 * Theme implementation for Apex-Dash RLCD
 */

#include "ui/ui_theme.h"

namespace ApexUi {

bool UiTheme::_inverted = true;

lv_style_t UiTheme::style_screen;
lv_style_t UiTheme::style_card;
lv_style_t UiTheme::style_card_title;
lv_style_t UiTheme::style_card_title_text;
lv_style_t UiTheme::style_badge_inverted;
lv_style_t UiTheme::style_text_small;
lv_style_t UiTheme::style_text_normal;
lv_style_t UiTheme::style_text_bold;
lv_style_t UiTheme::style_text_large;
lv_style_t UiTheme::style_text_huge;

lv_color_t UiTheme::bg() {
    return _inverted ? color_inverted_bg() : color_bg();
}

lv_color_t UiTheme::fg() {
    return _inverted ? color_inverted_fg() : color_fg();
}

void UiTheme::init(bool inverted) {
    _inverted = inverted;
    lv_color_t c_bg = bg();
    lv_color_t c_fg = fg();

    // 1. Root Screen Style
    lv_style_init(&style_screen);
    lv_style_set_bg_color(&style_screen, c_bg);
    lv_style_set_bg_opa(&style_screen, LV_OPA_COVER);
    lv_style_set_text_color(&style_screen, c_fg);
    lv_style_set_pad_all(&style_screen, 0);
    lv_style_set_border_width(&style_screen, 0);

    // 2. Card / Framed Panel Style
    lv_style_init(&style_card);
    lv_style_set_bg_color(&style_card, c_bg);
    lv_style_set_bg_opa(&style_card, LV_OPA_COVER);
    lv_style_set_border_color(&style_card, c_fg);
    lv_style_set_border_width(&style_card, 1);
    lv_style_set_radius(&style_card, 4);
    lv_style_set_pad_all(&style_card, 4);

    // 3. Card Header Bar
    lv_style_init(&style_card_title);
    lv_style_set_bg_color(&style_card_title, c_fg);
    lv_style_set_bg_opa(&style_card_title, LV_OPA_COVER);
    lv_style_set_radius(&style_card_title, 2);
    lv_style_set_pad_hor(&style_card_title, 4);
    lv_style_set_pad_ver(&style_card_title, 2);

    lv_style_init(&style_card_title_text);
    lv_style_set_text_color(&style_card_title_text, c_bg);
    lv_style_set_text_font(&style_card_title_text, &lv_font_montserrat_12);

    // 4. Inverted Badge
    lv_style_init(&style_badge_inverted);
    lv_style_set_bg_color(&style_badge_inverted, c_fg);
    lv_style_set_bg_opa(&style_badge_inverted, LV_OPA_COVER);
    lv_style_set_text_color(&style_badge_inverted, c_bg);
    lv_style_set_radius(&style_badge_inverted, 3);
    lv_style_set_pad_hor(&style_badge_inverted, 6);
    lv_style_set_pad_ver(&style_badge_inverted, 2);

    // 5. Typography Styles
    lv_style_init(&style_text_small);
    lv_style_set_text_color(&style_text_small, c_fg);
    lv_style_set_text_font(&style_text_small, &lv_font_montserrat_10);

    lv_style_init(&style_text_normal);
    lv_style_set_text_color(&style_text_normal, c_fg);
    lv_style_set_text_font(&style_text_normal, &lv_font_montserrat_14);

    lv_style_init(&style_text_bold);
    lv_style_set_text_color(&style_text_bold, c_fg);
    lv_style_set_text_font(&style_text_bold, &lv_font_montserrat_16);

    lv_style_init(&style_text_large);
    lv_style_set_text_color(&style_text_large, c_fg);
    lv_style_set_text_font(&style_text_large, &lv_font_montserrat_28);

    lv_style_init(&style_text_huge);
    lv_style_set_text_color(&style_text_huge, c_fg);
    lv_style_set_text_font(&style_text_huge, &lv_font_montserrat_48);
}

} // namespace ApexUi
