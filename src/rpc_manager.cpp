#include "rpc_manager.h"
#include "app_state.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <iostream>

extern AppState g_AppState;

void LoadRpcImage(const std::string& imageKey) {
    if (!g_AppState.renderer) {
        std::cerr << "❌ Renderer not initialized\n";
        return;
    }

    // Clean up old texture
    if (g_AppState.rpcImageTexture) {
        SDL_DestroyTexture(g_AppState.rpcImageTexture);
        g_AppState.rpcImageTexture = nullptr;
    }

    // Try to load the image from assets folder
    std::string imagePath = "assets/" + imageKey + ".png";
    SDL_Surface* surface = IMG_Load(imagePath.c_str());

    if (!surface) {
        std::cerr << "❌ Failed to load RPC image: " << imagePath << " - " << SDL_GetError() << "\n";
        // Try alternative path format
        imagePath = "assets/" + imageKey + "_map.png";
        surface = IMG_Load(imagePath.c_str());

        if (!surface) {
            std::cerr << "❌ Failed to load RPC image (alt): " << imagePath << " - " << SDL_GetError() << "\n";
            return;
        }
    }

    g_AppState.rpcImageTexture = SDL_CreateTextureFromSurface(g_AppState.renderer, surface);
    g_AppState.rpcImageWidth = surface->w;
    g_AppState.rpcImageHeight = surface->h;
    SDL_DestroySurface(surface);

    if (g_AppState.rpcImageTexture) {
        std::cout << "✅ RPC image loaded: " << imagePath << " (" << g_AppState.rpcImageWidth << "x" << g_AppState.rpcImageHeight << ")\n";
    } else {
        std::cerr << "❌ Failed to create texture from surface: " << SDL_GetError() << "\n";
    }
}
