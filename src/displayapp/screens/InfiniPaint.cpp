#include "displayapp/screens/InfiniPaint.h"
#include "displayapp/DisplayApp.h"
#include "displayapp/LittleVgl.h"
#include "displayapp/InfiniTimeTheme.h"

#include <algorithm> // std::fill

using namespace Pinetime::Applications::Screens;

InfiniPaint::InfiniPaint(Pinetime::Components::LittleVgl& lvgl, Pinetime::Controllers::MotorController& motor)
  : lvgl {lvgl}, motor {motor} {
  std::fill(b, b + bufferSize, selectColor);
}

InfiniPaint::~InfiniPaint() {
  lv_obj_clean(lv_scr_act());
}

bool InfiniPaint::OnTouchEvent(Pinetime::Applications::TouchEvents event) {
  switch (event) {
    case Pinetime::Applications::TouchEvents::LongTap:
      color = (color + 1) % 8;
      switch (color) {
        case 0:
          selectColor = Colors::magenta;
          break;
        case 1:
          selectColor = Colors::green;
          break;
        case 2:
          selectColor = Colors::fg;
          break;
        case 3:
          selectColor = Colors::red;
          break;
        case 4:
          selectColor = Colors::cyan;
          break;
        case 5:
          selectColor = Colors::yellow;
          break;
        case 6:
          selectColor = Colors::blue;
          break;
        case 7:
          selectColor = Colors::background;
          break;

        default:
          color = 0;
          break;
      }

      std::fill(b, b + bufferSize, selectColor);
      motor.RunForDuration(35);
      return true;
    default:
      return true;
  }
  return true;
}

bool InfiniPaint::OnTouchEvent(uint16_t x, uint16_t y) {
  // If currently scrolling in or out of InfiniPaint, don't paint anything!
  // Since InfiniPaint writes directly to the display bypassing LVGL, painting
  // while scrolling is happening causes bad behaviour
  if (lvgl.IsScrolling()) {
    return false;
  }
  lv_area_t area;
  area.x1 = x - (width / 2);
  area.y1 = y - (height / 2);
  area.x2 = x + (width / 2) - 1;
  area.y2 = y + (height / 2) - 1;
  lvgl.FlushDisplay(&area, b);
  return true;
}
