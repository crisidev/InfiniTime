#include "displayapp/screens/FirmwareValidation.h"
#include "displayapp/screens/Styles.h"
#include <lvgl/lvgl.h>
#include "Version.h"
#include "components/firmwarevalidator/FirmwareValidator.h"
#include "displayapp/DisplayApp.h"
#include "displayapp/InfiniTimeTheme.h"
#include "displayapp/screens/Symbols.h"

using namespace Pinetime::Applications::Screens;

namespace {
  void ButtonEventHandler(lv_obj_t* obj, lv_event_t event) {
    auto* screen = static_cast<FirmwareValidation*>(obj->user_data);
    screen->OnButtonEvent(obj, event);
  }
}

FirmwareValidation::FirmwareValidation(Pinetime::Controllers::FirmwareValidator& validator) : validator {validator} {
  CreatePromptTitle("Firmware");

  labelVersion = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(labelVersion, true);
  lv_label_set_text_fmt(labelVersion,
                        "#737aa2 Version# %lu.%lu.%lu\n"
                        "#737aa2 Short Ref# %s\n",
                        Version::Major(),
                        Version::Minor(),
                        Version::Patch(),
                        Version::GitCommitHash());
  lv_obj_align(labelVersion, nullptr, LV_ALIGN_CENTER, 0, -40);
  lv_label_set_align(labelVersion, LV_LABEL_ALIGN_CENTER);

  labelIsValidated = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(labelIsValidated, nullptr, LV_ALIGN_CENTER, 0, 10);
  lv_label_set_align(labelIsValidated, LV_LABEL_ALIGN_CENTER);
  lv_obj_set_auto_realign(labelIsValidated, true);
  lv_label_set_recolor(labelIsValidated, true);

  if (validator.IsValidated()) {
    lv_label_set_text_static(labelIsValidated, "This firmware has\nbeen #9ece6a validated#");
    lv_obj_align(labelIsValidated, nullptr, LV_ALIGN_CENTER, 0, 10);
  } else {
    lv_label_set_text_static(labelIsValidated, "Any reboot will\nrollback to last\nvalidated firmware");

    buttonValidate = lv_btn_create(lv_scr_act(), nullptr);
    buttonValidate->user_data = this;
    lv_obj_set_size(buttonValidate, 115, 50);
    lv_obj_align(buttonValidate, nullptr, LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_event_cb(buttonValidate, ButtonEventHandler);
    SetButtonColor(buttonValidate, Colors::green);

    labelButtonValidate = lv_label_create(buttonValidate, nullptr);
    lv_label_set_text_static(labelButtonValidate, "Validate");

    buttonReset = lv_btn_create(lv_scr_act(), nullptr);
    buttonReset->user_data = this;
    lv_obj_set_size(buttonReset, 115, 50);
    lv_obj_align(buttonReset, nullptr, LV_ALIGN_IN_BOTTOM_RIGHT, 0, 0);
    SetButtonColor(buttonReset, Colors::red);
    lv_obj_set_event_cb(buttonReset, ButtonEventHandler);

    labelButtonReset = lv_label_create(buttonReset, nullptr);
    lv_label_set_text_static(labelButtonReset, "Rollback");
  }
}

FirmwareValidation::~FirmwareValidation() {
  lv_obj_clean(lv_scr_act());
}

void FirmwareValidation::OnButtonEvent(lv_obj_t* object, lv_event_t event) {
  if (object == buttonValidate && event == LV_EVENT_CLICKED) {
    validator.Validate();
    running = false;
  } else if (object == buttonReset && event == LV_EVENT_CLICKED) {
    validator.Reset();
  }
}
