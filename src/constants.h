#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "imgui.h"

namespace Constants {
    constexpr float TOOLBAR_HEIGHT = 35.0f;
    constexpr float FOOTER_HEIGHT = 60.0f;
    constexpr float PROGRESS_BAR_WIDTH = 200.0f;
    constexpr float MAP_PREVIEW_WIDTH = 120.0f;
    constexpr float MAP_PREVIEW_HEIGHT = 80.0f;
    constexpr float MAP_AREA_RATIO = 0.7f;
    constexpr float CHAT_AREA_RATIO = 0.28f;
    constexpr ImVec4 COLOR_VICTORY = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
    constexpr ImVec4 COLOR_ALLIES = ImVec4(0.5f, 0.5f, 1.0f, 1.0f);
    constexpr ImVec4 COLOR_AXIS = ImVec4(1.0f, 0.5f, 0.5f, 1.0f);
    inline auto LOADING_TEXT = "Loading replays...";
}

#endif // CONSTANTS_H
