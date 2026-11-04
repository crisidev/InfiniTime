#include "Styles.h"
#include "displayapp/InfiniTimeTheme.h"

void Pinetime::Applications::Screens::SetRadioButtonStyle(lv_obj_t* checkbox) {
  lv_obj_set_style_local_radius(checkbox, LV_CHECKBOX_PART_BULLET, LV_STATE_DEFAULT, LV_RADIUS_CIRCLE);
  lv_obj_set_style_local_border_width(checkbox, LV_CHECKBOX_PART_BULLET, LV_STATE_CHECKED, 9);
  lv_obj_set_style_local_border_color(checkbox, LV_CHECKBOX_PART_BULLET, LV_STATE_CHECKED, Colors::highlight);
  lv_obj_set_style_local_bg_color(checkbox, LV_CHECKBOX_PART_BULLET, LV_STATE_CHECKED, Colors::onHighlight);
}

lv_obj_t* Pinetime::Applications::Screens::CreatePromptTitle(const char* title) {
  lv_obj_t* label = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(label, true);
  lv_obj_set_style_local_text_color(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::blue);
  lv_label_set_text_fmt(label, "%s %s", promptArrows, title);
  lv_obj_align(label, lv_scr_act(), LV_ALIGN_IN_TOP_LEFT, 0, 15);
  return label;
}
