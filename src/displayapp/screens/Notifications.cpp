#include "displayapp/screens/Notifications.h"
#include "displayapp/screens/Styles.h"
#include "displayapp/DisplayApp.h"
#include "components/ble/MusicService.h"
#include "components/ble/AlertNotificationService.h"
#include "displayapp/screens/Symbols.h"
#include <algorithm>
#include <cstring>
#include "displayapp/InfiniTimeTheme.h"

using namespace Pinetime::Applications::Screens;
extern lv_font_t jetbrains_mono_extrabold_compressed;
extern lv_font_t jetbrains_mono_bold_20;
extern lv_font_t jetbrains_mono_bold_24;

namespace {
  struct EmojiAlias {
    const char* alias;
    const char* glyph;
  };

  // Gadgetbridge sends emoji the watch can't draw as ":alias:" (gemoji names, e.g. ":fire:").
  // These are drawn with a matching Font Awesome glyph instead, which jetbrains_mono_bold_20 and
  // jetbrains_mono_bold_24 include (see fonts.json).
  constexpr EmojiAlias emojiAliases[] = {
    {"+1", "\xEF\x85\xA4"},
    {"-1", "\xEF\x85\xA5"},
    {"angry", "\xEF\x95\x96"},
    {"astonished", "\xEF\x97\x82"},
    {"ballot_box_with_check", "\xEF\x80\x8C"},
    {"beer", "\xEF\x83\xBC"},
    {"beers", "\xEF\x83\xBC"},
    {"bell", "\xEF\x83\xB3"},
    {"birthday", "\xEF\x87\xBD"},
    {"blue_car", "\xEF\x86\xB9"},
    {"broken_heart", "\xEF\x9E\xA9"},
    {"bulb", "\xEF\x83\xAB"},
    {"cake", "\xEF\x87\xBD"},
    {"camera", "\xEF\x80\xB0"},
    {"car", "\xEF\x86\xB9"},
    {"clinking_glasses", "\xEF\x9E\x9F"},
    {"coffee", "\xEF\x83\xB4"},
    {"confetti_ball", "\xEF\x9E\x9F"},
    {"crescent_moon", "\xEF\x86\x86"},
    {"dizzy_face", "\xEF\x95\xA7"},
    {"exclamation", "\xEF\x84\xAA"},
    {"expressionless", "\xEF\x84\x9A"},
    {"eye", "\xEF\x81\xAE"},
    {"eyes", "\xEF\x81\xAE"},
    {"face_with_rolling_eyes", "\xEF\x96\xA5"},
    {"facepunch", "\xEF\x9B\x9E"},
    {"fire", "\xEF\x81\xAD"},
    {"fist", "\xEF\x9B\x9E"},
    {"fist_oncoming", "\xEF\x9B\x9E"},
    {"fist_raised", "\xEF\x9B\x9E"},
    {"flushed", "\xEF\x97\x82"},
    {"ghost", "\xEF\x9B\xA2"},
    {"gift", "\xEF\x81\xAB"},
    {"grey_exclamation", "\xEF\x84\xAA"},
    {"grey_question", "\xEF\x84\xA8"},
    {"grimacing", "\xEF\x95\xBF"},
    {"grin", "\xEF\x96\x80"},
    {"growing_heart", "\xEF\x80\x84"},
    {"hand", "\xEF\x89\x96"},
    {"handshake", "\xEF\x8A\xB5"},
    {"hankey", "\xEF\x8B\xBE"},
    {"heart", "\xEF\x80\x84"},
    {"heart_eyes", "\xEF\x96\x84"},
    {"heartbeat", "\xEF\x80\x84"},
    {"heartpulse", "\xEF\x80\x84"},
    {"hearts", "\xEF\x80\x84"},
    {"heavy_check_mark", "\xEF\x80\x8C"},
    {"heavy_exclamation_mark", "\xEF\x84\xAA"},
    {"heavy_multiplication_x", "\xEF\x80\x8D"},
    {"home", "\xEF\x80\x95"},
    {"house", "\xEF\x80\x95"},
    {"house_with_garden", "\xEF\x80\x95"},
    {"hushed", "\xEF\x97\x82"},
    {"kissing_heart", "\xEF\x96\x98"},
    {"laughing", "\xEF\x96\x99"},
    {"musical_note", "\xEF\x80\x81"},
    {"negative_squared_cross_mark", "\xEF\x80\x8D"},
    {"neutral_face", "\xEF\x84\x9A"},
    {"no_mouth", "\xEF\x84\x9A"},
    {"notes", "\xEF\x80\x81"},
    {"open_mouth", "\xEF\x97\x82"},
    {"partying_face", "\xEF\x9E\x9F"},
    {"pensive", "\xEF\x84\x99"},
    {"phone", "\xEF\x82\x95"},
    {"poop", "\xEF\x8B\xBE"},
    {"pray", "\xEF\x9A\x84"},
    {"punch", "\xEF\x9B\x9E"},
    {"question", "\xEF\x84\xA8"},
    {"rage", "\xEF\x95\x96"},
    {"raised_back_of_hand", "\xEF\x89\x96"},
    {"raised_hand", "\xEF\x89\x96"},
    {"red_car", "\xEF\x86\xB9"},
    {"red_heart", "\xEF\x80\x84"},
    {"relaxed", "\xEF\x84\x98"},
    {"revolving_hearts", "\xEF\x80\x84"},
    {"rocket", "\xEF\x84\xB5"},
    {"rofl", "\xEF\x96\x86"},
    {"roll_eyes", "\xEF\x96\xA5"},
    {"rolling_on_the_floor_laughing", "\xEF\x96\x86"},
    {"runner", "\xEF\x9C\x8C"},
    {"running", "\xEF\x9C\x8C"},
    {"satisfied", "\xEF\x96\x99"},
    {"scream", "\xEF\x97\x82"},
    {"shit", "\xEF\x8B\xBE"},
    {"skull", "\xEF\x95\x8C"},
    {"skull_and_crossbones", "\xEF\x95\x8C"},
    {"sleeping", "\xEF\x88\xB6"},
    {"sleeping_bed", "\xEF\x88\xB6"},
    {"slightly_frowning_face", "\xEF\x84\x99"},
    {"smile", "\xEF\x84\x98"},
    {"smiley", "\xEF\x84\x98"},
    {"smiling_face_with_hearts", "\xEF\x96\x84"},
    {"smiling_face_with_three_hearts", "\xEF\x96\x84"},
    {"sparkles", "\xEF\x80\x85"},
    {"sparkling_heart", "\xEF\x80\x84"},
    {"star", "\xEF\x80\x85"},
    {"star2", "\xEF\x80\x85"},
    {"star_struck", "\xEF\x96\x87"},
    {"sun_with_face", "\xEF\x86\x85"},
    {"sunny", "\xEF\x86\x85"},
    {"tada", "\xEF\x9E\x9F"},
    {"telephone", "\xEF\x82\x95"},
    {"telephone_receiver", "\xEF\x82\x95"},
    {"thumbsdown", "\xEF\x85\xA5"},
    {"thumbsup", "\xEF\x85\xA4"},
    {"tired_face", "\xEF\x97\x88"},
    {"trophy", "\xEF\x82\x91"},
    {"two_hearts", "\xEF\x80\x84"},
    {"unamused", "\xEF\x84\x9A"},
    {"warning", "\xEF\x81\xB1"},
    {"wave", "\xEF\x89\x96"},
    {"weary", "\xEF\x97\x88"},
    {"white_check_mark", "\xEF\x80\x8C"},
    {"wine_glass", "\xEF\x93\xA3"},
    {"worried", "\xEF\x84\x99"},
    {"x", "\xEF\x80\x8D"},
    {"zzz", "\xEF\x88\xB6"},
  };

  const char* FindEmojiGlyph(const char* alias, size_t length) {
    for (const auto& entry : emojiAliases) {
      if (strlen(entry.alias) == length && strncmp(entry.alias, alias, length) == 0) {
        return entry.glyph;
      }
    }
    return nullptr;
  }

  bool IsAliasChar(char c) {
    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '+' || c == '-';
  }

  // Copy text into buffer, replacing known ":alias:" emoji with their glyph. Unknown aliases are kept
  // as they are. Glyphs are 3 bytes and the shortest alias (":x:") is too, so the text never grows.
  void ReplaceEmojiAliases(const char* text, char* buffer, size_t size) {
    char* out = buffer;
    char* const end = buffer + size - 1;
    for (const char* c = text; *c != '\0' && out < end; c++) {
      if (*c == ':') {
        const char* alias = c + 1;
        const char* aliasEnd = alias;
        while (IsAliasChar(*aliasEnd)) {
          aliasEnd++;
        }
        const char* glyph = (*aliasEnd == ':') ? FindEmojiGlyph(alias, aliasEnd - alias) : nullptr;
        if (glyph != nullptr) {
          const size_t length = std::min(strlen(glyph), static_cast<size_t>(end - out));
          memcpy(out, glyph, length);
          out += length;
          c = aliasEnd;
          continue;
        }
      }
      *out++ = *c;
    }
    *out = '\0';
  }

  char displayBuffer[Pinetime::Controllers::NotificationManager::MaximumMessageSize() + 1];
}

Notifications::Notifications(DisplayApp* app,
                             Pinetime::Controllers::NotificationManager& notificationManager,
                             Pinetime::Controllers::AlertNotificationService& alertNotificationService,
                             Pinetime::Controllers::MotorController& motorController,
                             System::SystemTask& systemTask,
                             Modes mode)
  : app {app},
    notificationManager {notificationManager},
    alertNotificationService {alertNotificationService},
    motorController {motorController},
    wakeLock(systemTask),
    mode {mode} {

  notificationManager.ClearNewNotificationFlag();
  auto notification = notificationManager.GetLastNotification();
  if (notification.valid) {
    currentId = notification.id;
    currentItem = std::make_unique<NotificationItem>(notification.Title(),
                                                     notification.Message(),
                                                     1,
                                                     notification.category,
                                                     notificationManager.NbNotifications(),
                                                     alertNotificationService,
                                                     motorController);
    validDisplay = true;
  } else {
    currentItem = std::make_unique<NotificationItem>(alertNotificationService, motorController);
    validDisplay = false;
  }
  if (mode == Modes::Preview) {
    wakeLock.Lock();
    if (notification.category == Controllers::NotificationManager::Categories::IncomingCall) {
      motorController.StartRinging();
    } else {
      motorController.RunForDuration(35);
    }

    timeoutLine = lv_line_create(lv_scr_act(), nullptr);

    lv_obj_set_style_local_line_width(timeoutLine, LV_LINE_PART_MAIN, LV_STATE_DEFAULT, 3);
    lv_obj_set_style_local_line_color(timeoutLine, LV_LINE_PART_MAIN, LV_STATE_DEFAULT, Colors::fg);
    lv_obj_set_style_local_line_rounded(timeoutLine, LV_LINE_PART_MAIN, LV_STATE_DEFAULT, true);

    lv_line_set_points(timeoutLine, timeoutLinePoints, 2);
    timeoutTickCountStart = xTaskGetTickCount();
  }

  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
}

Notifications::~Notifications() {
  lv_task_del(taskRefresh);
  // make sure we stop any vibrations before exiting
  motorController.StopRinging();
  lv_obj_clean(lv_scr_act());
}

void Notifications::Refresh() {
  if (mode == Modes::Preview && timeoutLine != nullptr) {
    TickType_t tick = xTaskGetTickCount();
    int32_t pos = LV_HOR_RES - ((tick - timeoutTickCountStart) / (timeoutLength / LV_HOR_RES));
    if (pos <= 0) {
      running = false;
    } else {
      timeoutLinePoints[1].x = pos;
      lv_line_set_points(timeoutLine, timeoutLinePoints, 2);
    }

  } else if (mode == Modes::Preview && dismissingNotification) {
    running = false;

  } else if (dismissingNotification) {
    dismissingNotification = false;
    auto notification = notificationManager.Get(currentId);
    if (!notification.valid) {
      notification = notificationManager.GetLastNotification();
    }
    currentId = notification.id;

    if (!notification.valid) {
      validDisplay = false;
    }

    currentItem.reset(nullptr);
    if (afterDismissNextMessageFromAbove) {
      app->SetFullRefresh(DisplayApp::FullRefreshDirections::Down);
    } else {
      app->SetFullRefresh(DisplayApp::FullRefreshDirections::Up);
    }

    if (validDisplay) {
      Controllers::NotificationManager::Notification::Idx currentIdx = notificationManager.IndexOf(currentId);
      currentItem = std::make_unique<NotificationItem>(notification.Title(),
                                                       notification.Message(),
                                                       currentIdx + 1,
                                                       notification.category,
                                                       notificationManager.NbNotifications(),
                                                       alertNotificationService,
                                                       motorController);
    } else {
      running = false;
    }
  }

  running = running && currentItem->IsRunning();
}

void Notifications::OnPreviewInteraction() {
  wakeLock.Release();
  motorController.StopRinging();
  mode = Modes::Normal;
  if (timeoutLine != nullptr) {
    lv_obj_del(timeoutLine);
    timeoutLine = nullptr;
  }
}

void Notifications::DismissToBlack() {
  currentItem.reset(nullptr);
  app->SetFullRefresh(DisplayApp::FullRefreshDirections::RightAnim);
  // create black transition screen to let the notification dismiss to blackness
  lv_obj_t* blackBox = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_size(blackBox, LV_HOR_RES, LV_VER_RES);
  lv_obj_set_style_local_bg_color(blackBox, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, Colors::background);
  dismissingNotification = true;
}

void Notifications::OnPreviewDismiss() {
  notificationManager.Dismiss(currentId);
  if (timeoutLine != nullptr) {
    lv_obj_del(timeoutLine);
    timeoutLine = nullptr;
  }
  DismissToBlack();
}

bool Notifications::OnTouchEvent(Pinetime::Applications::TouchEvents event) {
  if (mode == Modes::Preview) {
    if (event == TouchEvents::Tap || event == TouchEvents::SwipeDown) {
      OnPreviewInteraction();
    } else if (event == Pinetime::Applications::TouchEvents::SwipeRight) {
      OnPreviewDismiss();
      return true;
    } else {
      return false;
    }
  }

  switch (event) {
    case Pinetime::Applications::TouchEvents::SwipeRight:
      if (validDisplay) {
        auto previousMessage = notificationManager.GetPrevious(currentId);
        auto nextMessage = notificationManager.GetNext(currentId);
        afterDismissNextMessageFromAbove = previousMessage.valid;
        notificationManager.Dismiss(currentId);
        if (previousMessage.valid) {
          currentId = previousMessage.id;
        } else if (nextMessage.valid) {
          currentId = nextMessage.id;
        } else {
          // don't update id, notification manager will try to fetch
          // but not find it. Refresh will try to load latest message
          // or dismiss to previous screen
        }
        DismissToBlack();
        return true;
      }
      return false;
    case Pinetime::Applications::TouchEvents::SwipeDown: {
      Controllers::NotificationManager::Notification previousNotification;
      if (validDisplay) {
        previousNotification = notificationManager.GetPrevious(currentId);
      } else {
        previousNotification = notificationManager.GetLastNotification();
      }

      if (!previousNotification.valid) {
        return true;
      }

      currentId = previousNotification.id;
      Controllers::NotificationManager::Notification::Idx currentIdx = notificationManager.IndexOf(currentId);
      validDisplay = true;
      currentItem.reset(nullptr);
      app->SetFullRefresh(DisplayApp::FullRefreshDirections::Down);
      currentItem = std::make_unique<NotificationItem>(previousNotification.Title(),
                                                       previousNotification.Message(),
                                                       currentIdx + 1,
                                                       previousNotification.category,
                                                       notificationManager.NbNotifications(),
                                                       alertNotificationService,
                                                       motorController);
    }
      return true;
    case Pinetime::Applications::TouchEvents::SwipeUp: {
      Controllers::NotificationManager::Notification nextNotification;
      if (validDisplay) {
        nextNotification = notificationManager.GetNext(currentId);
      } else {
        nextNotification = notificationManager.GetLastNotification();
      }

      if (!nextNotification.valid) {
        running = false;
        return false;
      }

      currentId = nextNotification.id;
      Controllers::NotificationManager::Notification::Idx currentIdx = notificationManager.IndexOf(currentId);
      validDisplay = true;
      currentItem.reset(nullptr);
      app->SetFullRefresh(DisplayApp::FullRefreshDirections::Up);
      currentItem = std::make_unique<NotificationItem>(nextNotification.Title(),
                                                       nextNotification.Message(),
                                                       currentIdx + 1,
                                                       nextNotification.category,
                                                       notificationManager.NbNotifications(),
                                                       alertNotificationService,
                                                       motorController);
    }
      return true;
    default:
      return false;
  }
}

namespace {
  void CallEventHandler(lv_obj_t* obj, lv_event_t event) {
    auto* item = static_cast<Notifications::NotificationItem*>(obj->user_data);
    item->OnCallButtonEvent(obj, event);
  }
}

Notifications::NotificationItem::NotificationItem(Pinetime::Controllers::AlertNotificationService& alertNotificationService,
                                                  Pinetime::Controllers::MotorController& motorController)
  : NotificationItem("Notifications",
                     "No notifications to display",
                     0,
                     Controllers::NotificationManager::Categories::Unknown,
                     0,
                     alertNotificationService,
                     motorController) {
}

Notifications::NotificationItem::NotificationItem(const char* title,
                                                  const char* msg,
                                                  uint8_t notifNr,
                                                  Controllers::NotificationManager::Categories category,
                                                  uint8_t notifNb,
                                                  Pinetime::Controllers::AlertNotificationService& alertNotificationService,
                                                  Pinetime::Controllers::MotorController& motorController)
  : alertNotificationService {alertNotificationService}, motorController {motorController} {
  container = lv_cont_create(lv_scr_act(), nullptr);
  lv_obj_set_size(container, LV_HOR_RES, LV_VER_RES);
  lv_obj_set_style_local_bg_color(container, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, Colors::background);
  lv_obj_set_style_local_pad_all(container, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
  lv_obj_set_style_local_pad_inner(container, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);
  lv_obj_set_style_local_border_width(container, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);

  subject_container = lv_cont_create(container, nullptr);
  lv_obj_set_style_local_bg_color(subject_container, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, Colors::bgAlt);
  lv_obj_set_style_local_pad_all(subject_container, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 10);
  lv_obj_set_style_local_pad_inner(subject_container, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 5);
  lv_obj_set_style_local_border_width(subject_container, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 0);

  lv_obj_set_pos(subject_container, 0, 50);
  lv_obj_set_size(subject_container, LV_HOR_RES, LV_VER_RES - 50);
  lv_cont_set_layout(subject_container, LV_LAYOUT_COLUMN_LEFT);
  lv_cont_set_fit(subject_container, LV_FIT_NONE);

  lv_obj_t* alert_count = lv_label_create(container, nullptr);
  lv_obj_set_style_local_text_color(alert_count, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::muted);
  lv_label_set_text_fmt(alert_count, "%i/%i", notifNr, notifNb);
  lv_obj_align(alert_count, nullptr, LV_ALIGN_IN_TOP_RIGHT, 0, 16);

  // Terminal-style header: "❯❯❯ <title>". The arrows are a separate label so the
  // title (sent by the phone) is never parsed for recolor commands
  lv_obj_t* prompt = lv_label_create(container, nullptr);
  lv_label_set_recolor(prompt, true);
  lv_label_set_text_static(prompt, promptArrows);
  lv_obj_align(prompt, nullptr, LV_ALIGN_IN_TOP_LEFT, 0, 16);

  lv_obj_t* alert_type = lv_label_create(container, nullptr);
  lv_obj_set_style_local_text_color(alert_type, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::blue);
  if (title == nullptr) {
    lv_label_set_text_static(alert_type, "Notification");
  } else {
    // copy title to label and replace newlines with spaces
    ReplaceEmojiAliases(title, displayBuffer, sizeof(displayBuffer));
    lv_label_set_text(alert_type, displayBuffer);
    char* pchar = strchr(lv_label_get_text(alert_type), '\n');
    while (pchar != nullptr) {
      *pchar = ' ';
      pchar = strchr(pchar + 1, '\n');
    }
    lv_label_refr_text(alert_type);
  }
  lv_label_set_long_mode(alert_type, LV_LABEL_LONG_SROLL_CIRC);
  // One character of space after the prompt and before the count
  static constexpr lv_coord_t charWidth = 12;
  lv_obj_set_width(alert_type, LV_HOR_RES - lv_obj_get_width(prompt) - lv_obj_get_width(alert_count) - 2 * charWidth);
  lv_obj_align(alert_type, prompt, LV_ALIGN_OUT_RIGHT_MID, charWidth, 0);

  lv_obj_t* alert_subject = lv_label_create(subject_container, nullptr);
  lv_label_set_long_mode(alert_subject, LV_LABEL_LONG_BREAK);
  lv_obj_set_width(alert_subject, LV_HOR_RES - 20);

  switch (category) {
    default:
      // Larger text: messages are short (Gadgetbridge sends at most 100 characters), and any that
      // don't fit end with "..."
      lv_obj_set_style_local_text_font(alert_subject, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_bold_24);
      lv_label_set_long_mode(alert_subject, LV_LABEL_LONG_DOT);
      lv_obj_set_height(alert_subject, lv_obj_get_height(subject_container) - 20);
      ReplaceEmojiAliases(msg, displayBuffer, sizeof(displayBuffer));
      lv_label_set_text(alert_subject, displayBuffer);
      break;
    case Controllers::NotificationManager::Categories::IncomingCall: {
      lv_obj_set_height(subject_container, 108);
      lv_label_set_text_static(alert_subject, "Incoming call from");

      lv_obj_t* alert_caller = lv_label_create(subject_container, nullptr);
      lv_obj_align(alert_caller, alert_subject, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
      lv_label_set_long_mode(alert_caller, LV_LABEL_LONG_BREAK);
      lv_obj_set_width(alert_caller, LV_HOR_RES - 20);
      lv_label_set_text(alert_caller, msg);

      bt_accept = lv_btn_create(container, nullptr);
      bt_accept->user_data = this;
      lv_obj_set_event_cb(bt_accept, CallEventHandler);
      lv_obj_set_size(bt_accept, 76, 76);
      lv_obj_align(bt_accept, nullptr, LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);
      label_accept = lv_label_create(bt_accept, nullptr);
      lv_label_set_text_static(label_accept, Symbols::phone);
      SetButtonColor(bt_accept, Colors::green);

      bt_reject = lv_btn_create(container, nullptr);
      bt_reject->user_data = this;
      lv_obj_set_event_cb(bt_reject, CallEventHandler);
      lv_obj_set_size(bt_reject, 76, 76);
      lv_obj_align(bt_reject, nullptr, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
      label_reject = lv_label_create(bt_reject, nullptr);
      lv_label_set_text_static(label_reject, Symbols::phoneSlash);
      SetButtonColor(bt_reject, Colors::red);

      bt_mute = lv_btn_create(container, nullptr);
      bt_mute->user_data = this;
      lv_obj_set_event_cb(bt_mute, CallEventHandler);
      lv_obj_set_size(bt_mute, 76, 76);
      lv_obj_align(bt_mute, nullptr, LV_ALIGN_IN_BOTTOM_RIGHT, 0, 0);
      label_mute = lv_label_create(bt_mute, nullptr);
      lv_label_set_text_static(label_mute, Symbols::volumMute);
      SetButtonColor(bt_mute, Colors::lightGray);
    } break;
  }
}

void Notifications::NotificationItem::OnCallButtonEvent(lv_obj_t* obj, lv_event_t event) {
  if (event != LV_EVENT_CLICKED) {
    return;
  }

  motorController.StopRinging();

  if (obj == bt_accept) {
    alertNotificationService.AcceptIncomingCall();
  } else if (obj == bt_reject) {
    alertNotificationService.RejectIncomingCall();
  } else if (obj == bt_mute) {
    alertNotificationService.MuteIncomingCall();
  }

  running = false;
}

Notifications::NotificationItem::~NotificationItem() {
  lv_obj_clean(lv_scr_act());
}
