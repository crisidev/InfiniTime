#include <lvgl/lvgl.h>

namespace Pinetime {
  namespace Applications {
    namespace Screens {
      // Red, yellow and green "❯❯❯" shell prompt, as a recolor string (Colors::red, yellow, green)
      static constexpr const char* promptArrows = "#f7768e ❯##e0af68 ❯##9ece6a ❯#";

      void SetRadioButtonStyle(lv_obj_t* checkbox);

      // Terminal-style screen title, "❯❯❯ <title>", at the top left of the screen
      lv_obj_t* CreatePromptTitle(const char* title);
    }
  }
}
