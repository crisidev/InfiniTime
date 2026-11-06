#pragma once

#include <lvgl/lvgl.h>
#include <array>
#include <chrono>
#include <cstdint>
#include "displayapp/Controllers.h"
#include "displayapp/screens/Screen.h"
#include "components/datetime/DateTimeController.h"
#include "utility/DirtyValue.h"

namespace Pinetime {
  namespace Controllers {
    class Settings;
    class Battery;
    class Ble;
    class NotificationManager;
    class HeartRateController;
    class MotionController;
    class SimpleWeatherService;
  }

  namespace Applications {
    namespace Screens {

      // A night skyline driven by the watch's data: windows light up with steps, the moon follows the
      // battery, the sky follows the weather, a rooftop beacon blinks at the heart rate and a billboard
      // flickers on for notifications.
      class WatchFaceLivingTokyo : public Screen {
      public:
        WatchFaceLivingTokyo(Controllers::DateTime& dateTimeController,
                             const Controllers::Battery& batteryController,
                             const Controllers::Ble& bleController,
                             Controllers::NotificationManager& notificationManager,
                             Controllers::Settings& settingsController,
                             Controllers::HeartRateController& heartRateController,
                             Controllers::MotionController& motionController,
                             Controllers::SimpleWeatherService& weatherService);
        ~WatchFaceLivingTokyo() override;

        void Refresh() override;

      private:
        enum class Sky : uint8_t { Clear, Cloudy, Rain, Snow, Mist };

        static lv_design_res_t DrawScene(lv_obj_t* obj, const lv_area_t* clipArea, lv_design_mode_t mode);
        void DrawSky(const lv_area_t* clipArea);
        void DrawMoon(const lv_area_t* clipArea);
        void DrawCity(const lv_area_t* clipArea);

        void RefreshTime();
        void RefreshBeacon(uint32_t now);
        void RefreshBillboard(uint32_t now);
        void RefreshPrecipitation(uint32_t now);
        void CreatePrecipitation();
        void DeletePrecipitation();
        uint16_t LitWindowLevel() const;
        Sky CurrentSky() const;

        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::minutes>> currentDateTime {};
        Utility::DirtyValue<uint8_t> batteryPercent {};
        Utility::DirtyValue<bool> charging {};
        Utility::DirtyValue<bool> bleConnected {};
        Utility::DirtyValue<uint16_t> litLevel {};
        Utility::DirtyValue<Sky> sky {};
        Utility::DirtyValue<bool> newNotifications {};
        Utility::DirtyValue<uint8_t> heartRate {};
        Utility::DirtyValue<bool> heartRateRunning {};

        lv_obj_t* scene;
        lv_obj_t* labelTime;
        lv_obj_t* labelDate;
        lv_obj_t* labelBle;
        lv_obj_t* beacon;
        lv_obj_t* billboard;

        static constexpr uint8_t dropCount = 12;
        std::array<lv_obj_t*, dropCount> drops {};
        std::array<lv_point_t, dropCount> dropPositions {};
        uint32_t lastDropMove = 0;

        bool beaconLit = false;
        bool billboardLit = false;

        Controllers::DateTime& dateTimeController;
        const Controllers::Battery& batteryController;
        const Controllers::Ble& bleController;
        Controllers::NotificationManager& notificationManager;
        Controllers::Settings& settingsController;
        Controllers::HeartRateController& heartRateController;
        Controllers::MotionController& motionController;
        Controllers::SimpleWeatherService& weatherService;

        lv_task_t* taskRefresh;
      };
    }

    template <>
    struct WatchFaceTraits<WatchFace::LivingTokyo> {
      static constexpr WatchFace watchFace = WatchFace::LivingTokyo;
      static constexpr const char* name = "Living Tokyo";

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::WatchFaceLivingTokyo(controllers.dateTimeController,
                                                 controllers.batteryController,
                                                 controllers.bleController,
                                                 controllers.notificationManager,
                                                 controllers.settingsController,
                                                 controllers.heartRateController,
                                                 controllers.motionController,
                                                 *controllers.weatherController);
      };

      static bool IsAvailable(Pinetime::Controllers::FS& /*filesystem*/) {
        return true;
      }
    };
  }
}
