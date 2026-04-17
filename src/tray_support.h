#pragma once

#include <SDL3/SDL.h>

namespace tray_support {
    bool init(SDL_Window *window);
    void poll();
    void handle_window_close();
    bool consume_quit_request();
    void shutdown();
}

