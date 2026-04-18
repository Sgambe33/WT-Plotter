#pragma once

#include <SDL3/SDL.h>

namespace TraySupport {
    bool Init(SDL_Window *window);
    void Poll();
    void HandleWindowClose();
    bool ConsumeQuitRequest();
    void Shutdown();
}

