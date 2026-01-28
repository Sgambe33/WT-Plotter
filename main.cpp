#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <SDL3/SDL.h>
#define DISCORDPP_IMPLEMENTATION
#include <iostream>
#include "discordpp.h"
#include "src/app_state.h"
#include "src/constants.h"
#include "src/preferences.h"
#include "src/replay_manager.h"
#include "src/rpc_manager.h"
#include "src/gui_windows.h"
#include "src/status_thread.h"

ImFont *g_WtSymbolsFont = nullptr;

// ==========================================
// 4. MAIN ENTRY POINT
// ==========================================
constexpr uint64_t APPLICATION_ID = 1338259195455344650;

int main(int, char *[]) {
    //Setup discord
    std::cout << "🚀 Initializing Discord SDK...\n";
    auto client = std::make_shared<discordpp::Client>();
    client->SetApplicationId(APPLICATION_ID);

    discordpp::Activity activity;
    activity.SetApplicationId(APPLICATION_ID);
    activity.SetType(discordpp::ActivityTypes::Playing);
    activity.SetName(g_AppState.activityName);
    activity.SetState(g_AppState.activityStatus);
    activity.SetDetails(g_AppState.activityDetails);
    discordpp::ActivityAssets assets;
    assets.SetLargeImage("logowt_stripe_flat");
    activity.SetAssets(assets);
    discordpp::ActivityTimestamps timestamps;
    timestamps.SetStart(time(nullptr));
    activity.SetTimestamps(timestamps);

    // Update rich presence
    client->UpdateRichPresence(activity, [](const discordpp::ClientResult &result) {
        if (result.Successful()) {
            std::cout << "🎮 Rich Presence updated successfully!\n";
        } else {
            std::cerr << "❌ Rich Presence update failed" << result.Error();
        }
    });

    // Start background status thread (no direct callback to avoid thread-safety issues with Discord SDK)
    status_thread::start_status_thread(nullptr, std::chrono::milliseconds(2000));

    // Setup SDL
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("Error: %s\n", SDL_GetError());
        return 1;
    }

    // Create window with SDL_Renderer graphics context
    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    constexpr auto window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    SDL_Window *window = SDL_CreateWindow("WT Plotter",static_cast<int>(1280 * main_scale), static_cast<int>(800 * main_scale), window_flags);
    if (window == nullptr) {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }

    // Create SDL_Renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (renderer == nullptr) {
        printf("Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
        return -1;
    }

    // Store renderer in AppState for texture loading
    g_AppState.renderer = renderer;

    // Load preferences
    LoadPreferences(g_AppState.prefs);

    SDL_GL_SetSwapInterval(1); //VSYNC

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Optimize font atlas for lower memory usage
    ImFontConfig font_config;
    font_config.OversampleH = 1; // Default is 3
    font_config.OversampleV = 1; // Default is 1
    io.Fonts->AddFontDefault(&font_config);
    io.Fonts->TexMaxWidth = 512; // Smaller texture atlas
    g_WtSymbolsFont = io.Fonts->AddFontFromFileTTF("wt_symbols.ttf", 16.0f); // scegli la size desiderata
    // Setup Dear ImGui style
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

    // Main loop
    bool done = false;
    while (!done) {
        discordpp::RunCallbacks();
        // Poll and handle events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Consume any pending server status updates and update Discord presence from main thread
        status_thread::ServerStatus st;
        while (status_thread::try_pop_status(st)) {
            // Map server status to Discord Activity
            discordpp::Activity newAct = activity; // copy base activity
            if (st.online) {
                newAct.SetState("In Match");
                newAct.SetDetails(st.unit + " / " + std::to_string(st.online));
                discordpp::ActivityAssets newAssets = assets;
                newAssets.SetLargeImage(st.map);
                newAct.SetAssets(newAssets);
                g_AppState.activityDetails = st.unit + " / " + std::to_string(st.online) + " / Map: " + st.map;
            } else {
                newAct.SetState("In Lobby");
                newAct.SetDetails("Idle");
            }

            client->UpdateRichPresence(newAct, [](const discordpp::ClientResult &result) {
                if (!result.Successful()) {
                    std::cerr << "❌ Rich Presence update failed: " << result.Error() << std::endl;
                }
            });
        }

        // Start the Dear ImGui frame
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // Menu bar
        Gui_MenuBar();

        // Windows
        Gui_ReplaysWindow();
        Gui_DetailsWindow();
        Gui_PlaybackWindow();
        Gui_DiscordRichPresence();
        Gui_AboutDialog();
        Gui_PreferencesDialog();
        ImGui::ShowMetricsWindow();


        // Simulate loading progress
        if (g_AppState.listMode == AppState::ListMode::Loading) {
            g_AppState.loadingProgress += 0.01f;
        }

        // Simulate playback progress
        if (g_AppState.isPlaying) {
            g_AppState.playbackProgress += 0.1f;
            if (g_AppState.playbackProgress > 100.0f) {
                g_AppState.playbackProgress = 0.0f;
            }
        }

        // Rendering
        ImGui::Render();
        SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(renderer, static_cast<Uint8>(0.45f * 255), static_cast<Uint8>(0.55f * 255), static_cast<Uint8>(0.60f * 255), 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    status_thread::stop_status_thread();

    // Clean up RPC texture
    if (g_AppState.rpcImageTexture) {
        SDL_DestroyTexture(g_AppState.rpcImageTexture);
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
