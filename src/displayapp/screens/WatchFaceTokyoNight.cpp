#include <lvgl/lvgl.h>
#include "displayapp/screens/WatchFaceTokyoNight.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "components/heartrate/HeartRateController.h"
#include "components/motion/MotionController.h"
#include "components/settings/Settings.h"

using namespace Pinetime::Applications::Screens;

namespace {
  // Tokyo Night ("night" variant) palette
  constexpr lv_color_t bg = LV_COLOR_MAKE(0x1a, 0x1b, 0x26);
  constexpr lv_color_t fg = LV_COLOR_MAKE(0xc0, 0xca, 0xf5);
  constexpr lv_color_t comment = LV_COLOR_MAKE(0x56, 0x5f, 0x89);
  constexpr lv_color_t blue = LV_COLOR_MAKE(0x7a, 0xa2, 0xf7);
  constexpr lv_color_t cyan = LV_COLOR_MAKE(0x7d, 0xcf, 0xff);
  constexpr lv_color_t green = LV_COLOR_MAKE(0x9e, 0xce, 0x6a);
  constexpr lv_color_t yellow = LV_COLOR_MAKE(0xe0, 0xaf, 0x68);
  constexpr lv_color_t orange = LV_COLOR_MAKE(0xff, 0x9e, 0x64);
  constexpr lv_color_t red = LV_COLOR_MAKE(0xf7, 0x76, 0x8e);

  // Recolor strings below use the same hex values inline (tags are #565f89 = comment)
  constexpr const char* prompt = "#f7768e ❯##e0af68 ❯##9ece6a ❯# #7aa2f7 ~#";

  lv_color_t BatteryColor(int percent) {
    if (percent >= 50) {
      return green;
    }
    if (percent >= 20) {
      return yellow;
    }
    return red;
  }

  void SetTextColor(lv_obj_t* label, lv_color_t color) {
    lv_obj_set_style_local_text_color(label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color);
  }

  lv_obj_t* CreateRow(lv_obj_t* parent, lv_color_t color) {
    lv_obj_t* label = lv_label_create(parent, nullptr);
    lv_label_set_recolor(label, true);
    SetTextColor(label, color);
    return label;
  }
}

WatchFaceTokyoNight::WatchFaceTokyoNight(Controllers::DateTime& dateTimeController,
                                         const Controllers::Battery& batteryController,
                                         const Controllers::Ble& bleController,
                                         Controllers::NotificationManager& notificationManager,
                                         Controllers::Settings& settingsController,
                                         Controllers::HeartRateController& heartRateController,
                                         Controllers::MotionController& motionController)
  : currentDateTime {{}},
    dateTimeController {dateTimeController},
    batteryController {batteryController},
    bleController {bleController},
    notificationManager {notificationManager},
    settingsController {settingsController},
    heartRateController {heartRateController},
    motionController {motionController} {

  // A full-screen object instead of styling lv_scr_act(), so the background
  // goes away with lv_obj_clean() and doesn't leak into the next screen
  background = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_size(background, LV_HOR_RES, LV_VER_RES);
  lv_obj_set_style_local_bg_color(background, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, bg);
  lv_obj_set_style_local_radius(background, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
  lv_obj_set_style_local_border_width(background, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);

  // First line: "❯❯❯ ~ HH:MM:SS", the time in a font just small enough to fit
  labelTime = CreateRow(lv_scr_act(), blue);
  lv_obj_set_style_local_text_font(labelTime, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_32);
  lv_obj_set_pos(labelTime, 76, 7);

  labelPrompt = CreateRow(lv_scr_act(), fg);
  lv_label_set_text_static(labelPrompt, prompt);

  // Everything else, one empty line below the time
  container = lv_cont_create(lv_scr_act(), nullptr);
  lv_cont_set_layout(container, LV_LAYOUT_COLUMN_LEFT);
  lv_cont_set_fit(container, LV_FIT_TIGHT);
  lv_obj_set_style_local_pad_inner(container, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 2);
  lv_obj_set_style_local_pad_top(container, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
  lv_obj_set_style_local_pad_bottom(container, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
  lv_obj_set_style_local_pad_left(container, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 4);
  lv_obj_set_style_local_pad_right(container, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 4);
  lv_obj_set_style_local_bg_opa(container, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);

  labelDate = CreateRow(container, fg);
  batteryValue = CreateRow(container, green);
  stepValue = CreateRow(container, orange);
  heartbeatValue = CreateRow(container, red);
  connectState = CreateRow(container, cyan);
  // Empty line before the final prompt
  lv_label_set_text_static(CreateRow(container, fg), " ");
  labelPrompt2 = CreateRow(container, fg);

  lv_obj_set_pos(container, 0, 57);

  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
  Refresh();

  // After Refresh() so the time label has its final size
  lv_obj_align(labelPrompt, labelTime, LV_ALIGN_OUT_LEFT_BOTTOM, -12, 0);
}

WatchFaceTokyoNight::~WatchFaceTokyoNight() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
}

void WatchFaceTokyoNight::Refresh() {
  notificationState = notificationManager.AreNewNotificationsAvailable();
  if (notificationState.IsUpdated()) {
    if (notificationState.Get()) {
      lv_label_set_text_static(labelPrompt2, "#bb9af7 You have new mail.#");
    } else {
      lv_label_set_text_fmt(labelPrompt2, "%s #565f89 _#", prompt);
    }
  }

  currentDateTime = std::chrono::time_point_cast<std::chrono::seconds>(dateTimeController.CurrentDateTime());
  if (currentDateTime.IsUpdated()) {
    uint8_t hour = dateTimeController.Hours();
    uint8_t minute = dateTimeController.Minutes();
    uint8_t second = dateTimeController.Seconds();

    if (settingsController.GetClockType() == Controllers::Settings::ClockType::H12) {
      const char* ampm = "AM";
      if (hour == 0) {
        hour = 12;
      } else if (hour == 12) {
        ampm = "PM";
      } else if (hour > 12) {
        hour = hour - 12;
        ampm = "PM";
      }
      // No room for both seconds and AM/PM on the first line
      lv_label_set_text_fmt(labelTime, "%02d:%02d #565f89 %s#", hour, minute, ampm);
    } else {
      // Seconds are dimmed so HH:MM is what the eye lands on
      lv_label_set_text_fmt(labelTime, "%02d:%02d#565f89 :%02d#", hour, minute, second);
    }

    currentDate = std::chrono::time_point_cast<std::chrono::days>(currentDateTime.Get());
    if (currentDate.IsUpdated()) {
      lv_label_set_text_fmt(labelDate,
                            "#565f89 date#  %s %02d %s",
                            Controllers::DateTime::DayOfWeekShortToStringLow(dateTimeController.DayOfWeek()),
                            dateTimeController.Day(),
                            Controllers::DateTime::MonthShortToStringLow(dateTimeController.Month()));
    }
  }

  powerPresent = batteryController.IsPowerPresent();
  batteryPercentRemaining = batteryController.PercentRemaining();
  if (batteryPercentRemaining.IsUpdated() || powerPresent.IsUpdated()) {
    SetTextColor(batteryValue, BatteryColor(batteryPercentRemaining.Get()));
    lv_label_set_text_fmt(batteryValue,
                          "#565f89 batt#  %d%%%s",
                          batteryPercentRemaining.Get(),
                          batteryController.IsCharging() ? " #e0af68 chg#" : "");
  }

  stepCount = motionController.NbSteps();
  if (stepCount.IsUpdated()) {
    lv_label_set_text_fmt(stepValue, "#565f89 step#  %lu", stepCount.Get());
  }

  heartbeat = heartRateController.HeartRate();
  heartbeatRunning = heartRateController.State() != Controllers::HeartRateController::States::Stopped;
  if (heartbeat.IsUpdated() || heartbeatRunning.IsUpdated()) {
    if (heartbeatRunning.Get()) {
      SetTextColor(heartbeatValue, red);
      lv_label_set_text_fmt(heartbeatValue, "#565f89 hr#    %d bpm", heartbeat.Get());
    } else {
      SetTextColor(heartbeatValue, comment);
      lv_label_set_text_static(heartbeatValue, "#565f89 hr#    ---");
    }
  }

  bleState = bleController.IsConnected();
  bleRadioEnabled = bleController.IsRadioEnabled();
  if (bleState.IsUpdated() || bleRadioEnabled.IsUpdated()) {
    if (!bleRadioEnabled.Get()) {
      SetTextColor(connectState, comment);
      lv_label_set_text_static(connectState, "#565f89 ble#   off");
    } else if (bleState.Get()) {
      SetTextColor(connectState, cyan);
      lv_label_set_text_static(connectState, "#565f89 ble#   connected");
    } else {
      SetTextColor(connectState, comment);
      lv_label_set_text_static(connectState, "#565f89 ble#   disconnected");
    }
  }
}
