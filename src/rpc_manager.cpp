#include "rpc_manager.h"
#include "app_state.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <algorithm>
#include <cctype>
#include <iostream>
#include "logger.h"

extern AppState g_AppState;

void LoadRpcImage(const std::string &imageKey) {
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
    SDL_Surface *surface = IMG_Load(imagePath.c_str());

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


void UpdateRPC(const TelemetryUpdate *telemetry_update) {
    if (telemetry_update == nullptr) {
        app_log::error("UpdateRPC called with null telemetry_update");
        return;
    }
    if (!g_AppState.client) {
        app_log::error("Discord client not initialized");
        return;
    }

    std::string vehicle_name = telemetry_update->unit_name;
    const std::string to_remove = "tankModels/";
    size_t pos = vehicle_name.find(to_remove);
    if (pos != std::string::npos) {
        vehicle_name.erase(pos, to_remove.length());
    }
    std::transform(vehicle_name.begin(), vehicle_name.end(), vehicle_name.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    std::string details = "Idle";
    std::string state = "In menu";

    if (telemetry_update->player_state == TelemetryUpdate::PlayerState::InHangar) {
        details = "In hangar";
        state = "Ready";
    } else if (telemetry_update->player_state == TelemetryUpdate::PlayerState::InMatch) {
        const std::string display_vehicle = vehicle_name.empty() ? "unknown vehicle" : vehicle_name;
        details = "In battle: " + display_vehicle;
        state = "Crew ";

        g_AppState.rpc_party.SetCurrentSize(telemetry_update->current_crew ? telemetry_update->current_crew : 1);
        g_AppState.rpc_party.SetMaxSize(telemetry_update->total_crew ? telemetry_update->total_crew : 1);
        g_AppState.rpc_activity.SetParty(g_AppState.rpc_party);
    }

    g_AppState.rpc_activity.SetDetails(details);
    g_AppState.rpc_activity.SetState(state);

    g_AppState.rpc_assets.SetLargeImage("firing_range_tankmap");
    g_AppState.rpc_assets.SetLargeText("Firing Range");
    g_AppState.rpc_assets.SetSmallImage(std::format("https://static.encyclopedia.warthunder.com/images/{}.png", vehicle_name));
    g_AppState.rpc_assets.SetSmallText(vehicle_name.empty() ? "unknown vehicle" : vehicle_name);
    g_AppState.rpc_activity.SetAssets(g_AppState.rpc_assets);

    g_AppState.client->UpdateRichPresence(g_AppState.rpc_activity, [](const discordpp::ClientResult &result) {
        if (!result.Successful()) {
            app_log::error("Rich Presence update failed: " + result.Error());
        }
    });
}
