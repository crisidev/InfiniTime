#include <lvgl/lvgl.h>
#include "displayapp/screens/WatchFaceTokyoNight.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "components/heartrate/HeartRateController.h"
#include "components/motion/MotionController.h"
#include "components/settings/Settings.h"
#include "displayapp/InfiniTimeTheme.h"
#include "displayapp/screens/Styles.h"

using namespace Pinetime::Applications::Screens;

namespace {
  // Recolor strings use the Colors:: hex values inline (tags are #565f89 = Colors::comment)
  lv_color_t BatteryColor(int percent) {
    if (percent >= 50) {
      return Colors::green;
    }
    if (percent >= 20) {
      return Colors::yellow;
    }
    return Colors::red;
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

  // First line: "❯❯❯ ~ HH:MM:SS", the time in a font just small enough to fit
  labelTime = CreateRow(lv_scr_act(), Colors::blue);
  lv_obj_set_style_local_text_font(labelTime, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_32);
  lv_obj_set_pos(labelTime, 76, 7);

  labelPrompt = CreateRow(lv_scr_act(), Colors::fg);
  lv_label_set_text_fmt(labelPrompt, "%s #7aa2f7 ~#", promptArrows);

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

  labelDate = CreateRow(container, Colors::fg);
  batteryValue = CreateRow(container, Colors::green);
  stepValue = CreateRow(container, Colors::orange);
  heartbeatValue = CreateRow(container, Colors::red);
  connectState = CreateRow(container, Colors::cyan);
  // Empty line before the final prompt
  lv_label_set_text_static(CreateRow(container, Colors::fg), " ");
  labelPrompt2 = CreateRow(container, Colors::fg);

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
      lv_label_set_text_fmt(labelPrompt2, "%s #7aa2f7 ~# #565f89 _#", promptArrows);
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
      SetTextColor(heartbeatValue, Colors::red);
      lv_label_set_text_fmt(heartbeatValue, "#565f89 hr#    %d bpm", heartbeat.Get());
    } else {
      SetTextColor(heartbeatValue, Colors::comment);
      lv_label_set_text_static(heartbeatValue, "#565f89 hr#    ---");
    }
  }

  bleState = bleController.IsConnected();
  bleRadioEnabled = bleController.IsRadioEnabled();
  if (bleState.IsUpdated() || bleRadioEnabled.IsUpdated()) {
    if (!bleRadioEnabled.Get()) {
      SetTextColor(connectState, Colors::comment);
      lv_label_set_text_static(connectState, "#565f89 ble#   off");
    } else if (bleState.Get()) {
      SetTextColor(connectState, Colors::cyan);
      lv_label_set_text_static(connectState, "#565f89 ble#   connected");
    } else {
      SetTextColor(connectState, Colors::comment);
      lv_label_set_text_static(connectState, "#565f89 ble#   disconnected");
    }
  }
}
