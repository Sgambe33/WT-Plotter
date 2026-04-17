#include "rpc_manager.h"
#include "app_state.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <iostream>
#include "logger.h"

extern AppState g_AppState;

void LoadRpcImage(const std::string& imageKey) {
    if (!g_AppState.renderer) {
        app_log::error("Renderer not initialized");
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
        app_log::error("Failed to load RPC image: " + imagePath + " - " + SDL_GetError());
        // Try alternative path format
        imagePath = "assets/" + imageKey + "_map.png";
        surface = IMG_Load(imagePath.c_str());

        if (!surface) {
            app_log::error("Failed to load RPC image (alt): " + imagePath + " - " + SDL_GetError());
            return;
        }
    }

    g_AppState.rpcImageTexture = SDL_CreateTextureFromSurface(g_AppState.renderer, surface);
    g_AppState.rpcImageWidth = surface->w;
    g_AppState.rpcImageHeight = surface->h;
    SDL_DestroySurface(surface);

    if (g_AppState.rpcImageTexture) {
        app_log::info(
            "RPC image loaded: " + imagePath +
            " (" + std::to_string(g_AppState.rpcImageWidth) + "x" + std::to_string(g_AppState.rpcImageHeight) + ")"
        );
    } else {
        app_log::error("Failed to create texture from surface: " + std::string(SDL_GetError()));
    }
}
