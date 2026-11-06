#include "displayapp/screens/WatchFaceLivingTokyo.h"
#include <algorithm>
#include <optional>
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "components/ble/SimpleWeatherService.h"
#include "components/heartrate/HeartRateController.h"
#include "components/motion/MotionController.h"
#include "components/settings/Settings.h"
#include "displayapp/InfiniTimeTheme.h"
#include "displayapp/screens/Symbols.h"

using namespace Pinetime::Applications::Screens;

namespace {
  struct Building {
    int16_t x;
    int16_t width;
    int16_t top;
  };

  struct Rect {
    int16_t x;
    int16_t y;
    int16_t width;
    int16_t height;
  };

  constexpr int16_t screenSize = 240;

  // Hazy buildings in the distance
  constexpr Building farBuildings[] = {
    {0, 30, 128},
    {26, 22, 112},
    {52, 26, 136},
    {84, 18, 104},
    {108, 34, 124},
    {150, 20, 98},
    {176, 28, 130},
    {208, 32, 116},
  };

  // The buildings in front, which have windows
  constexpr Building nearBuildings[] = {
    {0, 38, 168},
    {42, 30, 150},
    {76, 42, 178},
    {122, 28, 138},
    {154, 36, 160},
    {194, 46, 146},
  };

  constexpr int16_t windowWidth = 4;
  constexpr int16_t windowHeight = 6;
  constexpr int16_t windowColumnStep = 9;
  constexpr int16_t windowRowStep = 11;
  constexpr int16_t windowMargin = 5;

  // Kept out of the moon's corner, as its shadow is painted in the sky color
  constexpr lv_point_t stars[] = {
    {8, 78},   {30, 92},  {54, 70},  {72, 100}, {96, 84},  {118, 66}, {126, 104}, {146, 58},
    {160, 84}, {172, 70}, {190, 92}, {216, 62}, {232, 88}, {140, 8},  {164, 26},  {152, 44},
  };

  constexpr Rect clouds[] = {
    {18, 90, 62, 12},
    {40, 82, 42, 12},
    {112, 84, 76, 12},
    {136, 76, 40, 12},
    {176, 30, 58, 13},
    {192, 22, 34, 13},
  };

  constexpr Rect mistBands[] = {
    {0, 150, 150, 3},
    {60, 172, 180, 3},
    {0, 194, 170, 3},
    {90, 216, 150, 3},
  };

  constexpr lv_point_t moonCenter = {198, 30};
  constexpr int16_t moonRadius = 18;
  constexpr lv_area_t moonArea = {moonCenter.x - moonRadius - 1,
                                  moonCenter.y - moonRadius - 1,
                                  screenSize - 1,
                                  moonCenter.y + moonRadius + 1};
  constexpr lv_area_t cityArea = {0, 98, screenSize - 1, screenSize - 1};

  constexpr int16_t antennaX = 135;
  constexpr int16_t beaconSize = 6;
  constexpr int16_t beaconY = 114;
  constexpr int16_t billboardX = 192;
  constexpr int16_t billboardY = 122;

  constexpr lv_color_t litColors[] = {Colors::yellow, Colors::yellow, Colors::yellow, Colors::yellow, Colors::orange, Colors::orange, Colors::cyan, Colors::magenta};

  // Windows always lit, so that the city isn't dead at midnight, and the step between redraws
  constexpr uint16_t minLitLevel = 8;
  constexpr uint16_t maxLitLevel = 256;
  constexpr uint16_t litLevelStep = 8;

  constexpr uint32_t dropInterval = 100;

  void Fill(const lv_area_t* clipArea, int16_t x, int16_t y, int16_t width, int16_t height, lv_color_t color, int16_t radius = 0) {
    // The scene is drawn once for each strip of the display buffer, so most shapes are skipped here
    if (y > clipArea->y2 || y + height <= clipArea->y1 || x > clipArea->x2 || x + width <= clipArea->x1) {
      return;
    }
    const lv_area_t area = {x, y, static_cast<lv_coord_t>(x + width - 1), static_cast<lv_coord_t>(y + height - 1)};
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_color = color;
    dsc.radius = radius;
    lv_draw_rect(&area, clipArea, &dsc);
  }

  // Multiplying by an odd number permutes 0..255, so every level lights exactly its share of windows
  uint8_t WindowHash(uint16_t index) {
    return static_cast<uint8_t>(index * 167 + 13);
  }
}

WatchFaceLivingTokyo::WatchFaceLivingTokyo(Controllers::DateTime& dateTimeController,
                                           const Controllers::Battery& batteryController,
                                           const Controllers::Ble& bleController,
                                           Controllers::NotificationManager& notificationManager,
                                           Controllers::Settings& settingsController,
                                           Controllers::HeartRateController& heartRateController,
                                           Controllers::MotionController& motionController,
                                           Controllers::SimpleWeatherService& weatherService)
  : dateTimeController {dateTimeController},
    batteryController {batteryController},
    bleController {bleController},
    notificationManager {notificationManager},
    settingsController {settingsController},
    heartRateController {heartRateController},
    motionController {motionController},
    weatherService {weatherService} {

  // Read before the first draw
  batteryPercent = batteryController.PercentRemaining();
  charging = batteryController.IsCharging();
  litLevel = LitWindowLevel();
  sky = CurrentSky();

  scene = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_size(scene, screenSize, screenSize);
  lv_obj_set_pos(scene, 0, 0);
  lv_obj_set_click(scene, false);
  scene->user_data = this;
  lv_obj_set_design_cb(scene, DrawScene);

  beacon = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_size(beacon, beaconSize, beaconSize);
  lv_obj_set_pos(beacon, antennaX + 1 - beaconSize / 2, beaconY);
  lv_obj_set_click(beacon, false);
  lv_obj_set_style_local_radius(beacon, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_RADIUS_CIRCLE);
  lv_obj_set_style_local_bg_color(beacon, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, Colors::comment);

  billboard = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(billboard, "mail");
  lv_obj_set_pos(billboard, billboardX, billboardY);
  lv_obj_set_style_local_bg_color(billboard, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::bgDark);
  lv_obj_set_style_local_bg_opa(billboard, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_COVER);
  lv_obj_set_style_local_text_color(billboard, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::magenta);
  lv_obj_set_hidden(billboard, true);

  labelTime = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(labelTime, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_42);
  lv_obj_set_style_local_text_color(labelTime, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::fg);
  lv_obj_set_pos(labelTime, 8, 4);

  labelDate = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(labelDate, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::muted);
  lv_obj_set_pos(labelDate, 10, 50);

  labelBle = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(labelBle, Symbols::bluetooth);
  lv_obj_set_style_local_text_color(labelBle, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::yellow);
  lv_obj_set_pos(labelBle, 174, 50);
  lv_obj_set_hidden(labelBle, true);

  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
  Refresh();
}

WatchFaceLivingTokyo::~WatchFaceLivingTokyo() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
}

lv_design_res_t WatchFaceLivingTokyo::DrawScene(lv_obj_t* obj, const lv_area_t* clipArea, lv_design_mode_t mode) {
  if (mode == LV_DESIGN_COVER_CHK) {
    return LV_DESIGN_RES_NOT_COVER;
  }
  if (mode == LV_DESIGN_DRAW_MAIN) {
    auto* face = static_cast<WatchFaceLivingTokyo*>(obj->user_data);
    face->DrawSky(clipArea);
    face->DrawMoon(clipArea);
    face->DrawCity(clipArea);
  }
  return LV_DESIGN_RES_OK;
}

void WatchFaceLivingTokyo::DrawSky(const lv_area_t* clipArea) {
  const Sky currentSky = sky.Get();
  if (currentSky == Sky::Clear) {
    for (size_t i = 0; i < sizeof(stars) / sizeof(stars[0]); i++) {
      const int16_t size = i % 3 == 0 ? 2 : 1;
      Fill(clipArea, stars[i].x, stars[i].y, size, size, i % 2 == 0 ? Colors::fgDark : Colors::muted);
    }
  }
}

void WatchFaceLivingTokyo::DrawMoon(const lv_area_t* clipArea) {
  const int16_t diameter = moonRadius * 2;
  const int16_t x = moonCenter.x - moonRadius;
  const int16_t y = moonCenter.y - moonRadius;

  lv_color_t color = Colors::fg;
  if (charging.Get()) {
    color = Colors::yellow;
  } else if (batteryPercent.Get() < 20) {
    color = Colors::red;
  }
  Fill(clipArea, x, y, diameter, diameter, color, LV_RADIUS_CIRCLE);

  // The shadow slides off to the right as the battery fills up
  const int16_t shadowOffset = batteryPercent.Get() * diameter / 100;
  Fill(clipArea, x + shadowOffset, y, diameter, diameter, Colors::background, LV_RADIUS_CIRCLE);

  // An outline, so that the moon is still there on an empty battery
  if (y <= clipArea->y2 && y + diameter > clipArea->y1) {
    const lv_area_t area = {x, y, static_cast<lv_coord_t>(x + diameter - 1), static_cast<lv_coord_t>(y + diameter - 1)};
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_opa = LV_OPA_TRANSP;
    dsc.radius = LV_RADIUS_CIRCLE;
    dsc.border_width = 1;
    dsc.border_color = color;
    lv_draw_rect(&area, clipArea, &dsc);
  }

  if (sky.Get() == Sky::Cloudy || sky.Get() == Sky::Rain || sky.Get() == Sky::Snow) {
    for (const auto& cloud : clouds) {
      Fill(clipArea, cloud.x, cloud.y, cloud.width, cloud.height, Colors::bgAlt, cloud.height / 2);
    }
  }
}

void WatchFaceLivingTokyo::DrawCity(const lv_area_t* clipArea) {
  for (const auto& building : farBuildings) {
    Fill(clipArea, building.x, building.top, building.width, screenSize - building.top, Colors::bgAlt);
  }

  if (sky.Get() == Sky::Mist) {
    for (const auto& band : mistBands) {
      Fill(clipArea, band.x, band.y, band.width, band.height, Colors::comment);
    }
  }

  // The beacon's antenna
  Fill(clipArea, antennaX, beaconY + beaconSize, 2, nearBuildings[3].top - beaconY - beaconSize, Colors::comment);

  const uint16_t level = litLevel.Get();
  uint16_t windowIndex = 0;
  for (const auto& building : nearBuildings) {
    Fill(clipArea, building.x, building.top, building.width, screenSize - building.top, Colors::bgDark);

    for (int16_t y = building.top + windowMargin; y + windowHeight < screenSize; y += windowRowStep) {
      for (int16_t x = building.x + windowMargin; x + windowWidth <= building.x + building.width - windowMargin + 1; x += windowColumnStep) {
        const uint8_t hash = WindowHash(windowIndex);
        const lv_color_t color = hash < level ? litColors[(windowIndex * 37u) % 8] : Colors::bgAlt;
        Fill(clipArea, x, y, windowWidth, windowHeight, color);
        windowIndex++;
      }
    }
  }
}

uint16_t WatchFaceLivingTokyo::LitWindowLevel() const {
  const uint32_t goal = std::max<uint32_t>(settingsController.GetStepsGoal(), 1);
  const uint32_t steps = std::min(motionController.NbSteps(), goal);
  const uint32_t level = minLitLevel + steps * (maxLitLevel - minLitLevel) / goal;
  // A goal that was just reached still has to light up every window
  return steps == goal ? maxLitLevel : level - level % litLevelStep;
}

WatchFaceLivingTokyo::Sky WatchFaceLivingTokyo::CurrentSky() const {
  const auto current = weatherService.Current();
  if (!current.has_value()) {
    return Sky::Clear;
  }
  using Icons = Controllers::SimpleWeatherService::Icons;
  switch (current->iconId) {
    case Icons::CloudsSun:
    case Icons::Clouds:
    case Icons::BrokenClouds:
      return Sky::Cloudy;
    case Icons::CloudShowerHeavy:
    case Icons::CloudSunRain:
    case Icons::Thunderstorm:
      return Sky::Rain;
    case Icons::Snow:
      return Sky::Snow;
    case Icons::Smog:
      return Sky::Mist;
    default:
      return Sky::Clear;
  }
}

void WatchFaceLivingTokyo::RefreshTime() {
  currentDateTime = std::chrono::time_point_cast<std::chrono::minutes>(dateTimeController.CurrentDateTime());
  if (!currentDateTime.IsUpdated()) {
    return;
  }

  uint8_t hour = dateTimeController.Hours();
  const uint8_t minute = dateTimeController.Minutes();
  const char* suffix = "";
  if (settingsController.GetClockType() == Controllers::Settings::ClockType::H12) {
    suffix = hour < 12 ? " AM" : " PM";
    hour = hour % 12 == 0 ? 12 : hour % 12;
  }
  lv_label_set_text_fmt(labelTime, "%02d:%02d", hour, minute);
  lv_label_set_text_fmt(labelDate,
                        "%s %02d %s%s",
                        Controllers::DateTime::DayOfWeekShortToStringLow(dateTimeController.DayOfWeek()),
                        dateTimeController.Day(),
                        Controllers::DateTime::MonthShortToStringLow(dateTimeController.Month()),
                        suffix);
}

void WatchFaceLivingTokyo::RefreshBeacon(uint32_t now) {
  heartRate = heartRateController.HeartRate();
  heartRateRunning = heartRateController.State() != Controllers::HeartRateController::States::Stopped;

  bool lit = false;
  if (heartRateRunning.Get() && heartRate.Get() > 0) {
    const uint32_t beat = 60000 / heartRate.Get();
    lit = now % beat < beat / 3;
  }
  if (lit != beaconLit) {
    beaconLit = lit;
    lv_obj_set_style_local_bg_color(beacon, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lit ? Colors::red : Colors::comment);
  }
}

void WatchFaceLivingTokyo::RefreshBillboard(uint32_t now) {
  newNotifications = notificationManager.AreNewNotificationsAvailable();
  if (newNotifications.IsUpdated()) {
    lv_obj_set_hidden(billboard, !newNotifications.Get());
  }
  if (!newNotifications.Get()) {
    return;
  }
  // Mostly on, with a dropout whenever the hash of the time window comes up low, like a failing tube
  const bool lit = static_cast<uint8_t>((now >> 7) * 167) >= 48;
  if (lit != billboardLit) {
    billboardLit = lit;
    lv_obj_set_style_local_text_color(billboard, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lit ? Colors::magenta : Colors::comment);
  }
}

void WatchFaceLivingTokyo::CreatePrecipitation() {
  const bool isRain = sky.Get() == Sky::Rain;
  if (!isRain && sky.Get() != Sky::Snow) {
    return;
  }
  for (uint8_t i = 0; i < dropCount; i++) {
    drops[i] = lv_obj_create(lv_scr_act(), nullptr);
    lv_obj_set_click(drops[i], false);
    lv_obj_set_size(drops[i], isRain ? 2 : 3, isRain ? 9 : 3);
    lv_obj_set_style_local_radius(drops[i], LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, isRain ? 0 : LV_RADIUS_CIRCLE);
    lv_obj_set_style_local_bg_color(drops[i], LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, isRain ? Colors::blue : Colors::fg);
    dropPositions[i] = {static_cast<lv_coord_t>(i * screenSize / dropCount + (i * 7) % 11), static_cast<lv_coord_t>((i * 83) % screenSize)};
    lv_obj_set_pos(drops[i], dropPositions[i].x, dropPositions[i].y);
  }
}

void WatchFaceLivingTokyo::DeletePrecipitation() {
  for (auto& drop : drops) {
    if (drop != nullptr) {
      lv_obj_del(drop);
      drop = nullptr;
    }
  }
}

void WatchFaceLivingTokyo::RefreshPrecipitation(uint32_t now) {
  if (drops[0] == nullptr || now - lastDropMove < dropInterval) {
    return;
  }
  lastDropMove = now;

  const bool isRain = sky.Get() == Sky::Rain;
  const lv_coord_t fall = isRain ? 14 : 3;
  for (uint8_t i = 0; i < dropCount; i++) {
    auto& position = dropPositions[i];
    position.y += fall + (i % 3);
    if (isRain) {
      position.x -= 1;
    } else if ((now / dropInterval + i) % 8 == 0) {
      position.x += (i % 2 == 0) ? 1 : -1;
    }
    if (position.y >= screenSize) {
      position.y = -10;
    }
    if (position.x < 0) {
      position.x += screenSize;
    }
    lv_obj_set_pos(drops[i], position.x, position.y);
  }
}

void WatchFaceLivingTokyo::Refresh() {
  RefreshTime();

  batteryPercent = batteryController.PercentRemaining();
  charging = batteryController.IsCharging();
  if (batteryPercent.IsUpdated() || charging.IsUpdated()) {
    lv_obj_invalidate_area(scene, &moonArea);
  }

  litLevel = LitWindowLevel();
  if (litLevel.IsUpdated()) {
    lv_obj_invalidate_area(scene, &cityArea);
  }

  sky = CurrentSky();
  if (sky.IsUpdated()) {
    DeletePrecipitation();
    CreatePrecipitation();
    lv_obj_invalidate(scene);
  }

  bleConnected = bleController.IsConnected();
  if (bleConnected.IsUpdated()) {
    lv_obj_set_hidden(labelBle, bleConnected.Get());
  }

  const uint32_t now = lv_tick_get();
  RefreshBeacon(now);
  RefreshBillboard(now);
  RefreshPrecipitation(now);
}
