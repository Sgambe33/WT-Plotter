#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <SDL3/SDL.h>
#define DISCORDPP_IMPLEMENTATION
#include <iostream>
#include "discordpp.h"
#include "libs/sqlite3/sqlite3.h"
#include "src/logger.h"
#include "src/app_state.h"
#include "src/constants.h"
#include "src/preferences.h"
#include "src/replay_manager.h"
#include "src/rpc_manager.h"
#include "src/tray_support.h"
#include "src/gui_windows.h"
#include "src/telemetry_thread.h"

ImFont *g_WtSymbolsFont = nullptr;

constexpr uint64_t APPLICATION_ID = 1338259195455344650;
extern Uint32 EVENT_TELEMETRY_UPDATED;


void InitAppFolder() {
    const char *documents_path = SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS);
    if (documents_path == nullptr) {
        app_log::error("Failed to get user documents folder: " + std::string(SDL_GetError()));
        return;
    }

    const std::filesystem::path documents_dir = documents_path;
    const std::filesystem::path app_folder = documents_dir / "wtplotter/plots";
    if (!std::filesystem::exists(app_folder)) {
        create_directories(app_folder);
    }
}

void InitSQLiteDB() {
    const char *documents_path = SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS);
    if (documents_path == nullptr) {
        app_log::error("Failed to get user documents folder: " + std::string(SDL_GetError()));
        return;
    }
    const std::filesystem::path documents_dir = documents_path;
    const std::filesystem::path db_path = documents_dir / "wtplotter/replays.sqlite3";
    if (!std::filesystem::exists(db_path)) {
        sqlite3 *db;
        int rc = sqlite3_open(reinterpret_cast<const char *>(absolute(db_path).c_str()), &db);
        if( rc ) {
            app_log::error("Failed to open database: " + std::string(SDL_GetError()));
            return;
        }
        for (std::string statement : Constants::SQLITE_TABLES_DEFINITIONS) {
            rc = sqlite3_exec(db, statement.c_str(), nullptr, nullptr, nullptr);
            if (rc != SQLITE_OK) {
                app_log::error("Failed to create tables: " + std::string(sqlite3_errmsg(db)));
                sqlite3_close(db);
                return;
            }
            app_log::info("SQLITE tables created");
        }
        app_log::info("Opened database successfully");
        sqlite3_close(db);
    }
}


int main(int, char *[]) {
    if (!app_log::init()) {
        std::cerr << "Logger initialization failed\n";
    }

    InitAppFolder();
    InitSQLiteDB();

    //Setup discord only if the user wants
    if (g_AppState.prefs.enableDiscordRichPresence) {
        app_log::info("Initializing Discord SDK...");
        g_AppState.client = std::make_shared<discordpp::Client>();
        g_AppState.client->SetApplicationId(APPLICATION_ID);
        g_AppState.rpc_activity.SetApplicationId(APPLICATION_ID);
        g_AppState.rpc_activity.SetType(discordpp::ActivityTypes::Playing);
        g_AppState.rpc_activity.SetName(g_AppState.activityName);
        g_AppState.rpc_party.SetId("crew");
        g_AppState.rpc_assets.SetLargeImage("logowt_stripe_flat");
        g_AppState.rpc_activity.SetAssets(g_AppState.rpc_assets);
        g_AppState.rpc_timestamps.SetStart(time(nullptr));
        g_AppState.rpc_activity.SetTimestamps(g_AppState.rpc_timestamps);

        g_AppState.client->UpdateRichPresence(g_AppState.rpc_activity, [](const discordpp::ClientResult &result) {
            if (result.Successful()) {
                app_log::info("Rich Presence updated successfully");
            } else {
                app_log::error("Rich Presence update failed: " + result.Error());
            }
        });
    }

    // Game telemetry thread setup
    TelemetryManager telemetry_manager;
    telemetry_manager.Start(std::chrono::milliseconds(2000), [](const TelemetryUpdate &update) {
        SDL_Event event;
        event.type = EVENT_TELEMETRY_UPDATED;
        SDL_PushEvent(&event);
    });

    // Setup SDL
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        app_log::error(std::string("SDL_Init failed: ") + SDL_GetError());
        app_log::shutdown();
        return 1;
    }

    // Create window with SDL_Renderer graphics context
    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    constexpr auto window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    SDL_Window *window = SDL_CreateWindow("WT Plotter",static_cast<int>(600 * main_scale), static_cast<int>(480 * main_scale), window_flags);
    if (window == nullptr) {
        app_log::error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
        app_log::shutdown();
        return -1;
    }

    // Create SDL_Renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (renderer == nullptr) {
        app_log::error(std::string("SDL_CreateRenderer failed: ") + SDL_GetError());
        app_log::shutdown();
        return -1;
    }

    // Store renderer in AppState for texture loading
    g_AppState.renderer = renderer;

    LoadPreferences(g_AppState.prefs);
    LoadReplaysFromDisk();

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // Optimize font atlas for lower memory usage
    ImFontConfig font_config;
    font_config.OversampleH = 1; // Default is 3
    font_config.OversampleV = 1; // Default is 1
    io.Fonts->AddFontDefault(&font_config);
    io.Fonts->TexMaxWidth = 512; // Smaller texture atlas
    g_WtSymbolsFont = io.Fonts->AddFontFromFileTTF("assets/fonts/wt_symbols.ttf", 16.0f);
    ImGui::StyleColorsDark();

    // DISABLE WINDOW TRANSPARENCY
    ImGuiStyle &style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg].w = 1.0f; // Window background fully opaque
    style.Colors[ImGuiCol_ChildBg].w = 1.0f; // Child window background fully opaque
    style.Colors[ImGuiCol_PopupBg].w = 1.0f; // Popup background fully opaque
    style.Colors[ImGuiCol_MenuBarBg].w = 1.0f; // Menu bar background fully opaque

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    TraySupport::Init(window);

    // Main loop
    bool done = false;
    while (!done) {
        TraySupport::Poll();
        if (TraySupport::ConsumeQuitRequest()) {
            done = true;
        }

        discordpp::RunCallbacks();
        // Poll and handle events
        bool needsContinuousUpdate = g_AppState.isPlaying || (g_AppState.listMode == AppState::ListMode::Loading);
        Sint32 timeout_ms = needsContinuousUpdate ? 16 : 250;

        SDL_Event event;
        if (SDL_WaitEventTimeout(&event, timeout_ms)) {
            do {
                ImGui_ImplSDL3_ProcessEvent(&event);
                if (event.type == SDL_EVENT_QUIT)
                    done = true;
                if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window)) {
                    TraySupport::HandleWindowClose();
                }
                if (event.type == EVENT_TELEMETRY_UPDATED) {
                    TelemetryUpdate telemetry_update;
                    bool has_update = false;
                    while (telemetry_manager.TryPopUpdate(telemetry_update)) {
                        has_update = true;
                    }
                    if (has_update) {
                        UpdateRPC(&telemetry_update);
                        Gui_OnTelemetryUpdate(telemetry_update);
                        g_AppState.telemetryPositions.insert(g_AppState.telemetryPositions.end(), telemetry_update.positions.begin(), telemetry_update.positions.end());
                    }
                }
            } while (SDL_PollEvent(&event));
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport((ImGuiID) 1, ImGui::GetMainViewport());

        // Menu bar
        Gui_MenuBar();

        // Windows
        Gui_ReplaysWindow();
        Gui_DetailsWindow();
        Gui_PlaybackWindow();
        Gui_DiscordRichPresence();
        Gui_TelemetryMapWindow();
        Gui_AboutDialog();
        Gui_PreferencesDialog();
        //ImGui::ShowMetricsWindow();


        // Simulate loading progress
        if (g_AppState.listMode == AppState::ListMode::Loading) {
            g_AppState.loadingProgress += 0.01f;
        }

        // Rendering
        ImGui::Render();
        SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(renderer, static_cast<Uint8>(0.45f * 255), static_cast<Uint8>(0.55f * 255), static_cast<Uint8>(0.60f * 255), 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);

    }

    // Clean up RPC texture
    if (g_AppState.replay_details_map_preview) {
        SDL_DestroyTexture(g_AppState.replay_details_map_preview);
    }
    if (g_AppState.telemetryMapTexture) {
        SDL_DestroyTexture(g_AppState.telemetryMapTexture);
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    TraySupport::Shutdown();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    app_log::shutdown();

    return 0;
}
