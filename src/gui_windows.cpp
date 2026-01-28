#include "gui_windows.h"
#include "app_state.h"
#include "constants.h"
#include "replay_manager.h"
#include "rpc_manager.h"
#include "preferences.h"
#include <SDL3/SDL.h>
#include <cstring>

extern AppState g_AppState;
extern ImFont *g_WtSymbolsFont;

// ==========================================
// PANEL DRAWING FUNCTIONS
// ==========================================

void DrawReplayListPanel() {
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
        // Trigger an immediate reload of the replay list
        g_AppState.listMode = AppState::ListMode::Loading;
        g_AppState.loadingProgress = 0.0f;
        g_AppState.treeNodeOpen.clear(); // Clear tree state
        LoadReplaysFromDisk();
        g_AppState.listMode = AppState::ListMode::TreeView;
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
    if (ImGui::BeginChild("MapArea", ImVec2(0, contentHeight * Constants::MAP_AREA_RATIO), true,
                          ImGuiWindowFlags_NoScrollbar)) {
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
// WINDOW FUNCTIONS
// ==========================================

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

void Gui_PlaybackWindow() {
    if (!g_AppState.showPlaybackWindow) return;

    ImGui::SetNextWindowPos(ImVec2(900, 40), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(600, 600), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Playback", &g_AppState.showPlaybackWindow)) {
        DrawPlaybackView();
    }
    ImGui::End();
}

void Gui_DiscordRichPresence() {
    if (!g_AppState.showRichPresence) return;
    ImGui::SetNextWindowPos(ImVec2(300, 200), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 450), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Discord Rich Presence", &g_AppState.showRichPresence)) {
        ImGui::Text("Playing %s", g_AppState.activityName.c_str());
        ImGui::Text("Status: %s", g_AppState.activityStatus.c_str());
        ImGui::Text("Details: %s", g_AppState.activityDetails.c_str());

        ImGui::Separator();
        ImGui::Text("Current Image: %s", g_AppState.currentRpcImageKey.c_str());

        // Display the RPC image if loaded
        if (g_AppState.rpcImageTexture) {
            ImGui::Separator();
            ImGui::Text("Activity Image:");

            // Display at 60x60 pixels
            ImGui::Image((ImTextureID) (intptr_t) g_AppState.rpcImageTexture,
                         ImVec2(60.0f, 60.0f));
        } else {
            ImGui::TextDisabled("No image loaded");
            if (ImGui::Button("Load Image")) {
                LoadRpcImage(g_AppState.currentRpcImageKey);
            }
        }
    }
    ImGui::End();
}

void Gui_AboutDialog() {
    if (!g_AppState.showAboutDialog) return;

    ImGui::SetNextWindowPos(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_Always);

    if (ImGui::Begin("About WT Plotter", &g_AppState.showAboutDialog, ImGuiWindowFlags_NoResize)) {
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);

        // Title
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("WT Plotter").x) * 0.5f);
        ImGui::Text("WT Plotter");

        ImGui::PopFont();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Version info
        ImGui::Text("Version: 1.0.0");
        ImGui::Text("Build Date: %s", __DATE__);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Description
        ImGui::TextWrapped("WT Plotter is a replay viewer and analyzer for War Thunder. "
            "It allows you to view replays, analyze battles, and integrate with Discord Rich Presence.");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Technologies used
        ImGui::Text("Technologies:");
        ImGui::BulletText("Dear ImGui - UI Framework");
        ImGui::BulletText("SDL3 - Graphics and Input");
        ImGui::BulletText("nlohmann/json - JSON parsing");
        ImGui::BulletText("Discord SDK - Rich Presence");
        ImGui::Spacing();

        // Close button
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 100) * 0.5f);
        if (ImGui::Button("Close", ImVec2(100, 0))) {
            g_AppState.showAboutDialog = false;
        }
    }
    ImGui::End();
}

void Gui_PreferencesDialog() {
    if (!g_AppState.showPreferencesDialog) return;

    ImGui::SetNextWindowPos(ImVec2(350, 200), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(600, 350), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Preferences", &g_AppState.showPreferencesDialog)) {
        // Create a copy to work with (only save on Apply/OK)
        static Preferences tempPrefs = g_AppState.prefs;
        static char replayPathBuffer[512];

        // Initialize buffer on first open
        static bool initialized = false;
        if (!initialized) {
            strncpy(replayPathBuffer, tempPrefs.replayPath.c_str(), sizeof(replayPathBuffer) - 1);
            replayPathBuffer[sizeof(replayPathBuffer) - 1] = '\0';
            initialized = true;
        }

        ImGui::Text("General Settings");
        ImGui::Separator();
        ImGui::Spacing();

        // Replay Path
        ImGui::Text("Replay Folder Path:");
        ImGui::SetNextItemWidth(-100);
        if (ImGui::InputText("##replayPath", replayPathBuffer, sizeof(replayPathBuffer))) {
            tempPrefs.replayPath = replayPathBuffer;
        }
        ImGui::SameLine();
        if (ImGui::Button("Browse...")) {
            // TODO: Implement file browser dialog
            ImGui::OpenPopup("NotImplemented");
        }

        if (ImGui::BeginPopup("NotImplemented")) {
            ImGui::Text("File browser not yet implemented.");
            ImGui::Text("Please type the path manually.");
            if (ImGui::Button("OK")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::Spacing();

        // Language selection
        ImGui::Text("Language:");
        ImGui::SetNextItemWidth(200);
        const char *languages[] = {"English", "Deutsch", "Français", "Italiano", "Español", "日本語", "한국어", "简体中文"};
        const char *languageCodes[] = {"en", "de", "fr", "it", "es", "ja", "ko", "zh"};

        int currentLangIndex = 0;
        for (int i = 0; i < 8; i++) {
            if (tempPrefs.language == languageCodes[i]) {
                currentLangIndex = i;
                break;
            }
        }

        if (ImGui::Combo("##language", &currentLangIndex, languages, IM_ARRAYSIZE(languages))) {
            tempPrefs.language = languageCodes[currentLangIndex];
        }

        ImGui::Spacing();

        ImGui::Checkbox("Auto-download server replay when available", &tempPrefs.autoDownloadServerReplay);
        ImGui::TextDisabled("If enabled, server replays will be automatically downloaded when you select a replay.");

        ImGui::Spacing();

        ImGui::Checkbox("Enable Discord Rich Presence", &tempPrefs.enableDiscordRichPresence);
        ImGui::TextDisabled("If enabled, a better Discord Rich Presence will be provided.");

        // Buttons
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("OK", ImVec2(100, 0))) {
            g_AppState.prefs = tempPrefs;
            SavePreferences(g_AppState.prefs);
            g_AppState.showPreferencesDialog = false;
            // Reload replays immediately if replay path changed
            g_AppState.treeNodeOpen.clear();
            LoadReplaysFromDisk();
         }

         ImGui::SameLine();
         if (ImGui::Button("Cancel", ImVec2(100, 0))) {
             tempPrefs = g_AppState.prefs; // Reset to original
             strncpy(replayPathBuffer, tempPrefs.replayPath.c_str(), sizeof(replayPathBuffer) - 1);
             g_AppState.showPreferencesDialog = false;
         }

         ImGui::SameLine();
         if (ImGui::Button("Apply", ImVec2(100, 0))) {
            g_AppState.prefs = tempPrefs;
            SavePreferences(g_AppState.prefs);
            // Reload replays immediately
            g_AppState.treeNodeOpen.clear();
            LoadReplaysFromDisk();
         }
     }
     ImGui::End();
}

void Gui_MenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Preferences")) {
                g_AppState.showPreferencesDialog = true;
            }
            if (ImGui::MenuItem("Quit")) {
                SDL_Event quit_event;
                quit_event.type = SDL_EVENT_QUIT;
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
                g_AppState.showAboutDialog = true;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}
