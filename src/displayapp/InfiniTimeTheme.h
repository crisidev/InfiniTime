#pragma once

#include <lvgl/lvgl.h>

namespace Colors {
  // Tokyo Night ("night" variant) palette
  static constexpr lv_color_t background = LV_COLOR_MAKE(0x1a, 0x1b, 0x26);
  static constexpr lv_color_t fg = LV_COLOR_MAKE(0xc0, 0xca, 0xf5);
  static constexpr lv_color_t fgDark = LV_COLOR_MAKE(0xa9, 0xb1, 0xd6);
  // comment is too dim for text (2.8:1 on background): keep it for borders and fills
  static constexpr lv_color_t comment = LV_COLOR_MAKE(0x56, 0x5f, 0x89);
  // Secondary text (4.1:1 on background)
  static constexpr lv_color_t muted = LV_COLOR_MAKE(0x73, 0x7a, 0xa2);
  static constexpr lv_color_t red = LV_COLOR_MAKE(0xf7, 0x76, 0x8e);
  static constexpr lv_color_t orange = LV_COLOR_MAKE(0xff, 0x9e, 0x64);
  static constexpr lv_color_t yellow = LV_COLOR_MAKE(0xe0, 0xaf, 0x68);
  static constexpr lv_color_t green = LV_COLOR_MAKE(0x9e, 0xce, 0x6a);
  static constexpr lv_color_t cyan = LV_COLOR_MAKE(0x7d, 0xcf, 0xff);
  static constexpr lv_color_t blue = LV_COLOR_MAKE(0x7a, 0xa2, 0xf7);
  static constexpr lv_color_t magenta = LV_COLOR_MAKE(0xbb, 0x9a, 0xf7);

  // Surfaces, from darkest to lightest
  static constexpr lv_color_t bgDark = LV_COLOR_MAKE(0x16, 0x16, 0x1e);
  static constexpr lv_color_t bgAlt = LV_COLOR_MAKE(0x29, 0x2e, 0x42);
  static constexpr lv_color_t bg = LV_COLOR_MAKE(0x3b, 0x42, 0x61);

  static constexpr lv_color_t deepOrange = red;
  static constexpr lv_color_t lightGray = fgDark;
  static constexpr lv_color_t gray = comment;
  // Selected and interactive state. Green is kept for success
  static constexpr lv_color_t highlight = blue;
  // Text/icons drawn on top of a bright fill such as highlight
  static constexpr lv_color_t onHighlight = background;
};

// Corner radius of boxes, buttons and bars
static constexpr lv_style_int_t cornerRadius = 8;

/**
 * Set a button's fill color, and a text color that stays readable on it
 * (dark on bright fills, light on dark ones). Labels inside the button inherit it.
 */
inline void SetButtonColor(lv_obj_t* button, lv_color_t color, lv_state_t state = LV_STATE_DEFAULT) {
  lv_obj_set_style_local_bg_color(button, LV_BTN_PART_MAIN, state, color);
  lv_obj_set_style_local_text_color(button, LV_BTN_PART_MAIN, state, lv_color_brightness(color) > 128 ? Colors::onHighlight : Colors::fg);
}

/**
 * Initialize the default
 * @param color_primary the primary color of the theme
 * @param color_secondary the secondary color for the theme
 * @param flags ORed flags starting with `LV_THEME_DEF_FLAG_...`
 * @param font_small pointer to a small font
 * @param font_normal pointer to a normal font
 * @param font_subtitle pointer to a large font
 * @param font_title pointer to a extra large font
 * @return a pointer to reference this theme later
 */
lv_theme_t* lv_pinetime_theme_init();
