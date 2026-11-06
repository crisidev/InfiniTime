#include "displayapp/screens/Tile.h"
#include "displayapp/screens/Styles.h"
#include "displayapp/screens/BatteryIcon.h"
#include "components/ble/BleController.h"
#include "displayapp/InfiniTimeTheme.h"
#include <cstdio>

using namespace Pinetime::Applications::Screens;

namespace {
  constexpr lv_coord_t statusBarHeight = 28;

  void event_handler(lv_obj_t* obj, lv_event_t event) {
    if (event != LV_EVENT_VALUE_CHANGED) {
      return;
    }

    Tile* screen = static_cast<Tile*>(obj->user_data);
    auto* eventDataPtr = (uint32_t*) lv_event_get_data();
    uint32_t eventData = *eventDataPtr;
    screen->OnValueChangedEvent(obj, eventData);
  }
}

Tile::Tile(uint8_t screenID,
           uint8_t numScreens,
           DisplayApp* app,
           Controllers::Settings& settingsController,
           const Controllers::Battery& batteryController,
           const Controllers::Ble& bleController,
           const Controllers::AlarmController& alarmController,
           Controllers::DateTime& dateTimeController,
           std::array<Applications, 6>& applications)
  : app {app},
    dateTimeController {dateTimeController},
    statusIcons(batteryController, bleController, alarmController) {

  settingsController.SetAppMenu(screenID);

  // tmux-style status bar: "1* 2 3" on the left, then the time and status icons on the right
  lv_obj_t* statusBar = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_size(statusBar, LV_HOR_RES, statusBarHeight);
  lv_obj_set_pos(statusBar, 0, 0);
  lv_obj_set_style_local_radius(statusBar, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
  lv_obj_set_style_local_bg_color(statusBar, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, Colors::bgAlt);

  lv_obj_t* label_pages = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(label_pages, true);
  char pages[64] = {};
  size_t length = 0;
  for (uint8_t i = 0; i < numScreens && length < sizeof(pages); i++) {
    length += snprintf(pages + length, sizeof(pages) - length, i == screenID ? "#7aa2f7 %d*# " : "#737aa2 %d# ", i + 1);
  }
  lv_label_set_text(label_pages, pages);
  lv_obj_align(label_pages, statusBar, LV_ALIGN_IN_LEFT_MID, 6, 0);

  statusIcons.Create();
  lv_obj_align(statusIcons.GetObject(), statusBar, LV_ALIGN_IN_RIGHT_MID, -2, 0);

  label_time = lv_label_create(lv_scr_act(), nullptr);

  uint8_t btIndex = 0;
  for (uint8_t i = 0; i < 6; i++) {
    if (i == 3) {
      btnmMap[btIndex++] = "\n";
    }
    if (applications[i].application == Apps::None) {
      btnmMap[btIndex] = " ";
    } else {
      btnmMap[btIndex] = applications[i].icon;
    }
    btIndex++;
    apps[i] = applications[i].application;
  }
  btnmMap[btIndex] = "";

  btnm1 = lv_btnmatrix_create(lv_scr_act(), nullptr);
  lv_btnmatrix_set_map(btnm1, btnmMap);
  lv_obj_set_size(btnm1, LV_HOR_RES - 16, LV_VER_RES - statusBarHeight - 16);
  lv_obj_align(btnm1, nullptr, LV_ALIGN_IN_BOTTOM_MID, 0, -8);

  lv_obj_set_style_local_radius(btnm1, LV_BTNMATRIX_PART_BTN, LV_STATE_DEFAULT, cornerRadius);
  lv_obj_set_style_local_bg_color(btnm1, LV_BTNMATRIX_PART_BTN, LV_STATE_DEFAULT, Colors::bgAlt);
  lv_obj_set_style_local_bg_opa(btnm1, LV_BTNMATRIX_PART_BTN, LV_STATE_DISABLED, LV_OPA_50);
  lv_obj_set_style_local_bg_color(btnm1, LV_BTNMATRIX_PART_BTN, LV_STATE_DISABLED, Colors::bgDark);
  lv_obj_set_style_local_pad_all(btnm1, LV_BTNMATRIX_PART_BG, LV_STATE_DEFAULT, 0);
  lv_obj_set_style_local_pad_inner(btnm1, LV_BTNMATRIX_PART_BG, LV_STATE_DEFAULT, 10);

  for (uint8_t i = 0; i < 6; i++) {
    lv_btnmatrix_set_btn_ctrl(btnm1, i, LV_BTNMATRIX_CTRL_CLICK_TRIG);
    if (applications[i].application == Apps::None || !applications[i].enabled) {
      lv_btnmatrix_set_btn_ctrl(btnm1, i, LV_BTNMATRIX_CTRL_DISABLED);
    }
  }

  btnm1->user_data = this;
  lv_obj_set_event_cb(btnm1, event_handler);

  Refresh();
  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
}

Tile::~Tile() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
}

void Tile::Refresh() {
  statusIcons.Update();
  currentDateTime = std::chrono::time_point_cast<std::chrono::minutes>(dateTimeController.CurrentDateTime());
  if (currentDateTime.IsUpdated()) {
    // No AM/PM: it doesn't fit next to the pages when all the status icons are showing
    std::string time = dateTimeController.FormattedTime();
    lv_label_set_text(label_time, time.substr(0, time.find(' ')).c_str());
  }
  // Status icons come and go, so the time follows them
  lv_obj_align(label_time, statusIcons.GetObject(), LV_ALIGN_OUT_LEFT_MID, -6, 0);
}

void Tile::OnValueChangedEvent(lv_obj_t* obj, uint32_t buttonId) {
  if (obj != btnm1) {
    return;
  }

  app->StartApp(apps[buttonId], DisplayApp::FullRefreshDirections::Up);
  running = false;
}
