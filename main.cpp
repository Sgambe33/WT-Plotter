#include <filesystem>
#include <iostream>
#include <map>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include <SDL2/SDL.h>
#include <string>
#include <vector>
#define DISCORDPP_IMPLEMENTATION
#include "discordpp.h"

#include "imgui_internal.h"
#include "src/status_thread.h"

// ==========================================
// CONSTANTS
// ==========================================
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
    const char *LOADING_TEXT = "Loading replays...";
}

// ==========================================
// 1. APP STATE (The Data Model)
// ==========================================
struct ReplayFile {
    std::filesystem::path fullPath;
    std::string filename;
    std::string date; // "2026.01.21"
    std::string time; // "22.57.15"
    int id{};
};

struct AppState {
    enum class ListMode { TreeView, Loading };

    enum class DetailMode { Info, Playback };

    ListMode listMode = ListMode::TreeView;
    float loadingProgress = 0.24f;
    DetailMode detailMode = DetailMode::Info;
    bool isPlaying = false;
    float playbackProgress = 0.0f;
    int selectedReplayId = -1;

    // Window state
    bool showReplaysWindow = true;
    bool showDetailsWindow = true;

    // CACHED REPLAY DATA
    std::map<std::string, std::vector<ReplayFile> > replaysByDate;
    bool replaysLoaded = false;

    // TREE STATE
    std::map<std::string, bool> treeNodeOpen; // Track open/closed state per date
    bool showPlaybackWindow = false;
    bool showRichPresence = false;

    //Discord Rich Presence
    std::string activityName = "War Thunder";
    std::string activityStatus = "In hangar";
    std::string activityDetails = "Idle";
};

static AppState g_AppState;
static ImFont *g_WtSymbolsFont = nullptr;

void LoadReplaysFromDisk() {
    const std::filesystem::path sandbox{R"(E:\SteamLibrary\steamapps\common\War Thunder\Replays)"};

    g_AppState.replaysByDate.clear();

    try {
        for (auto const &dir_entry: std::filesystem::directory_iterator{sandbox}) {
            if (dir_entry.is_regular_file() && dir_entry.path().extension() == ".wrpl") {
                std::string filename = dir_entry.path().filename().string();

                // Extract date from filename like "#2026.01.21 22.57.15.wrpl"
                if (filename.size() > 11 && filename[0] == '#') {
                    ReplayFile replay;
                    replay.fullPath = dir_entry.path();
                    replay.filename = filename;
                    replay.date = filename.substr(1, 10); // "2026.01.21"
                    replay.time = filename.substr(12, 8); // "22.57.15"
                    replay.id = static_cast<int>(std::hash<std::string>{}(dir_entry.path().string()));

                    g_AppState.replaysByDate[replay.date].push_back(replay);
                }
            }
        }
        g_AppState.replaysLoaded = true;
    } catch (const std::filesystem::filesystem_error &e) {
        std::cerr << "Error reading directory: " << e.what() << std::endl;
        g_AppState.replaysLoaded = false;
    }
}

// ==========================================
// OPTIMIZED DRAW FUNCTION
// ==========================================
void DrawReplayListPanel() {
    // Load replays on first display
    if (!g_AppState.replaysLoaded) {
        LoadReplaysFromDisk();
    }

    // The Tree View Area (leave space for bottom toolbar)
    if (ImGui::BeginChild("ReplayTree", ImVec2(0, -Constants::TOOLBAR_HEIGHT), true)) {
        if (!g_AppState.replaysLoaded) {
            ImGui::Text("Failed to load replays directory");
        } else if (g_AppState.replaysByDate.empty()) {
            ImGui::Text("No replays found");
        } else {
            // Display cached replays (no disk access!)
            for (auto &[date, replays]: g_AppState.replaysByDate) {
                // Check if we have state for this node, if not initialize as closed
                if (g_AppState.treeNodeOpen.find(date) == g_AppState.treeNodeOpen.end()) {
                    g_AppState.treeNodeOpen[date] = false;
                }

                // Set the tree node open/closed state
                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
                if (g_AppState.treeNodeOpen[date]) {
                    flags |= ImGuiTreeNodeFlags_DefaultOpen;
                }

                // Create collapsible tree node for each date
                const bool nodeOpen = ImGui::TreeNodeEx(date.c_str(), flags);

                // Update state based on current ImGui state
                g_AppState.treeNodeOpen[date] = nodeOpen;

                if (nodeOpen) {
                    // Display all replays for this date
                    for (const auto &replay: replays) {
                        if (ImGui::Selectable(replay.time.c_str(), g_AppState.selectedReplayId == replay.id)) {
                            g_AppState.selectedReplayId = replay.id;
                            g_AppState.detailMode = AppState::DetailMode::Info;
                            g_AppState.showDetailsWindow = true;
                        }
                    }
                    ImGui::TreePop();
                }
            }
        }
    }
    ImGui::EndChild();

    // Bottom Toolbar
    ImGui::Separator();
    if (ImGui::Button("Collapse All")) {
        // Close all tree nodes
        for (auto &[date, isOpen]: g_AppState.treeNodeOpen) {
            isOpen = false;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Expand All")) {
        // Open all tree nodes
        for (auto &[date, isOpen]: g_AppState.treeNodeOpen) {
            isOpen = true;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Refresh")) {
        g_AppState.listMode = AppState::ListMode::Loading;
        g_AppState.loadingProgress = 0.0f;
        g_AppState.replaysLoaded = false; // Mark for reload
        g_AppState.treeNodeOpen.clear(); // Clear tree state
    }
}

void DrawLoadingPanel() {
    const ImVec2 availRegion = ImGui::GetContentRegionAvail();

    // Center vertically
    ImGui::Dummy(ImVec2(0, availRegion.y * 0.4f));

    // Centered Text
    const float textWidth = ImGui::CalcTextSize(Constants::LOADING_TEXT).x;
    ImGui::SetCursorPosX((availRegion.x - textWidth) * 0.5f);
    ImGui::Text("%s", Constants::LOADING_TEXT);

    // Progress Bar
    ImGui::SetCursorPosX((availRegion.x - Constants::PROGRESS_BAR_WIDTH) * 0.5f);
    ImGui::ProgressBar(g_AppState.loadingProgress, ImVec2(Constants::PROGRESS_BAR_WIDTH, 0));

    // Simulate loading finishing
    if (ImGui::Button("Cancel (Simulate Finish)")) {
        g_AppState.listMode = AppState::ListMode::TreeView;
    }

    // Auto-complete simulation
    if (g_AppState.loadingProgress >= 1.0f) {
        g_AppState.listMode = AppState::ListMode::TreeView;
    }
}

void DrawReplayDetails() {
    // Top Info Section
    ImGui::BeginGroup();
    ImGui::TextDisabled("Session ID:");
    ImGui::SameLine();
    ImGui::Text("60805f90003bf2a");

    ImGui::TextDisabled("Map:");
    ImGui::SameLine();
    ImGui::Text("Abandoned Factory");

    ImGui::TextDisabled("Difficulty:");
    ImGui::SameLine();
    ImGui::Text("Realistic");

    ImGui::TextDisabled("Start time:");
    ImGui::SameLine();
    ImGui::Text("17:36:04");

    ImGui::TextDisabled("Time played:");
    ImGui::SameLine();
    ImGui::Text("00:09:22");

    ImGui::TextDisabled("Result:");
    ImGui::SameLine();
    ImGui::TextColored(Constants::COLOR_VICTORY, "Victory");
    ImGui::EndGroup();

    ImGui::SameLine(0, 50);

    // Map Image Placeholder
    ImGui::BeginGroup();
    ImGui::Button("[ Map Image ]", ImVec2(Constants::MAP_PREVIEW_WIDTH, Constants::MAP_PREVIEW_HEIGHT));
    ImGui::EndGroup();

    ImGui::Separator();

    // Action Buttons
    if (ImGui::Button("Server Replay")) {
        // TODO: Implement server replay
    }
    ImGui::SameLine();
    if (ImGui::Button("Play Local")) {
        g_AppState.detailMode = AppState::DetailMode::Playback;
        g_AppState.playbackProgress = 0.0f;
        g_AppState.isPlaying = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Play Server")) {
        // TODO: Implement server playback
    }

    ImGui::Separator();

    // Tab Widget (Allies/Axis Teams)
    if (ImGui::BeginTabBar("TeamsTabs")) {
        if (ImGui::BeginTabItem("Allies Team")) {
            if (ImGui::BeginTable("AlliesTable", 11, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Player");
                ImGui::TableSetupColumn("Score");
                ImGui::TableSetupColumn("Air kills");
                ImGui::TableSetupColumn("Ground Kills");
                ImGui::TableSetupColumn("Naval Kills");
                ImGui::TableSetupColumn("Assists");
                ImGui::TableSetupColumn("Captured Zones");
                ImGui::TableSetupColumn("AI Kills");
                ImGui::TableSetupColumn("Awarded damage");
                ImGui::TableSetupColumn("Bombing damage");
                ImGui::TableSetupColumn("▴"); // Deaths

                if (g_WtSymbolsFont) {
                    ImGui::PushFont(g_WtSymbolsFont);
                    ImGui::TableHeadersRow();
                    ImGui::PopFont();
                } else {
                    ImGui::TableHeadersRow();
                }
                for (int i = 0; i < 16; i++) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("PlayerOne");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("1500");
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("5");
                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("5");
                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text("5");
                    ImGui::TableSetColumnIndex(5);
                    ImGui::Text("5");
                    ImGui::TableSetColumnIndex(6);
                    ImGui::Text("6");
                    ImGui::TableSetColumnIndex(7);
                    ImGui::Text("6");
                    ImGui::TableSetColumnIndex(8);
                    ImGui::Text("6");
                    ImGui::TableSetColumnIndex(9);
                    ImGui::Text("6");
                    ImGui::TableSetColumnIndex(10);
                    ImGui::Text("6");
                }


                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Axis Team")) {
            ImGui::Text("Axis Team Data...");
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void DrawPlaybackView() {
    const ImVec2 availRegion = ImGui::GetContentRegionAvail();
    const float contentHeight = availRegion.y - Constants::FOOTER_HEIGHT;

    // Map Area (Top 70%)
    if (ImGui::BeginChild("MapArea", ImVec2(0, contentHeight * Constants::MAP_AREA_RATIO), true, ImGuiWindowFlags_NoScrollbar)) {
        ImGui::Text("Scene Image Viewer (Map)");
        // Zoom/Pan logic would go here
        ImGui::Button("##MapCanvas", ImVec2(-1, -1));
    }
    ImGui::EndChild();

    // Chat/Log Area (Bottom 28%)
    if (ImGui::BeginChild("ChatArea", ImVec2(0, contentHeight * Constants::CHAT_AREA_RATIO), true)) {
        if (ImGui::BeginTabBar("LogTabs")) {
            if (ImGui::BeginTabItem("Chat")) {
                ImGui::TextColored(Constants::COLOR_ALLIES, "[Allies] P1: Attack left!");
                ImGui::TextColored(Constants::COLOR_AXIS, "[Axis] P2: Defending...");
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Battle Log")) {
                ImGui::Text("10:00 - Match Started");
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::EndChild();

    // Playback Controls (Bottom)
    ImGui::Separator();
    if (ImGui::Button("Back")) {
        g_AppState.detailMode = AppState::DetailMode::Info;
    }
    ImGui::SameLine();

    // Play/Pause button
    const char *playPauseLabel = g_AppState.isPlaying ? "Pause" : "Play";
    if (ImGui::Button(playPauseLabel)) {
        g_AppState.isPlaying = !g_AppState.isPlaying;
    }
    ImGui::SameLine();

    ImGui::SetNextItemWidth(availRegion.x - 150);
    ImGui::SliderFloat("##progress", &g_AppState.playbackProgress, 0.0f, 100.0f, "");

    ImGui::SameLine();
    ImGui::Text("0:45/2:00");
}

// ==========================================
// 3. FLOATING WINDOWS
// ==========================================
void Gui_DiscordRichPresence() {
    if (!g_AppState.showRichPresence) return;
    ImGui::SetNextWindowPos(ImVec2(300, 200), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Discord Rich Presence", &g_AppState.showRichPresence)) {
        ImGui::Text("Playing %s", g_AppState.activityName.c_str());
        ImGui::Text("Status: %s", g_AppState.activityStatus.c_str());
        ImGui::Text("Details: %s", g_AppState.activityDetails.c_str());
    }
    ImGui::End();
}

void Gui_PlaybackWindow() {
    if (!g_AppState.showPlaybackWindow) return;

    ImGui::SetNextWindowPos(ImVec2(900, 40), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(600, 600), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Playback", &g_AppState.showPlaybackWindow)) {
        DrawPlaybackView();
    }
    ImGui::End();
}

void Gui_ReplaysWindow() {
    if (!g_AppState.showReplaysWindow) return;

    ImGui::SetNextWindowPos(ImVec2(20, 40), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Replays", &g_AppState.showReplaysWindow)) {
        switch (g_AppState.listMode) {
            case AppState::ListMode::TreeView:
                DrawReplayListPanel();
                break;
            case AppState::ListMode::Loading:
                DrawLoadingPanel();
                break;
        }
    }
    ImGui::End();
}

void Gui_DetailsWindow() {
    if (!g_AppState.showDetailsWindow) return;

    ImGui::SetNextWindowPos(ImVec2(340, 40), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(540, 500), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Details", &g_AppState.showDetailsWindow)) {
        DrawReplayDetails();
    }
    ImGui::End();
}

void Gui_MenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Preferences")) {
                // TODO: Open preferences
            }
            if (ImGui::MenuItem("Quit")) {
                SDL_Event quit_event;
                quit_event.type = SDL_QUIT;
                SDL_PushEvent(&quit_event);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Replays", nullptr, &g_AppState.showReplaysWindow);
            ImGui::MenuItem("Details", nullptr, &g_AppState.showDetailsWindow);
            ImGui::MenuItem("Playback", nullptr, &g_AppState.showPlaybackWindow);
            ImGui::MenuItem("Discord Rich Presence", nullptr, &g_AppState.showRichPresence);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
                // TODO: Show about dialog
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

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
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Create window with SDL_Renderer graphics context
    constexpr auto window_flags = static_cast<SDL_WindowFlags>(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window *window = SDL_CreateWindow("WT Plotter",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          900, 600, window_flags);
    if (window == nullptr) {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }

    // Create SDL_Renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr) {
        printf("Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
        return -1;
    }

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
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    // Main loop
    bool done = false;
    while (!done) {
        discordpp::RunCallbacks();
        // Poll and handle events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
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
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Menu bar
        Gui_MenuBar();

        // Windows
        Gui_ReplaysWindow();
        Gui_DetailsWindow();
        Gui_PlaybackWindow();
        Gui_DiscordRichPresence();
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
        SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(renderer, static_cast<Uint8>(0.45f * 255), static_cast<Uint8>(0.55f * 255), static_cast<Uint8>(0.60f * 255), 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    status_thread::stop_status_thread();

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
