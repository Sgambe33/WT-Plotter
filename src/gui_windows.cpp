#include "gui_windows.h"
#include "app_state.h"
#include "constants.h"
#include "replay_manager.h"
#include "rpc_manager.h"
#include "preferences.h"
#include <SDL3/SDL.h>
#include <cstring>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <curl/curl.h>
#include <SDL3_image/SDL_image.h>

extern AppState g_AppState;
extern ImFont *g_WtSymbolsFont;

namespace {
size_t CurlWriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    const size_t real_size = size * nmemb;
    auto *buffer = static_cast<std::string *>(userp);
    buffer->append(static_cast<const char *>(contents), real_size);
    return real_size;
}

bool ReloadTelemetryMapTexture() {
    if (g_AppState.renderer == nullptr) {
        return false;
    }

    CURL *curl = curl_easy_init();
    if (curl == nullptr) {
        SDL_Log("Failed to initialize curl for telemetry map");
        return false;
    }

    std::string map_bytes;
    curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:8111/map.img");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 1500L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &map_bytes);

    const CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || http_code < 200 || http_code >= 300 || map_bytes.empty()) {
        return false;
    }

    constexpr auto cache_path = "assets/map_images/.telemetry_map_cache.img";
    {
        std::ofstream out(cache_path, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            SDL_Log("Failed to open telemetry map cache file: %s", cache_path);
            return false;
        }
        out.write(map_bytes.data(), static_cast<std::streamsize>(map_bytes.size()));
    }

    SDL_Surface *surface = IMG_Load(cache_path);
    if (surface == nullptr) {
        SDL_Log("Failed to load telemetry map image from cache: %s", SDL_GetError());
        return false;
    }

    SDL_Texture *new_texture = SDL_CreateTextureFromSurface(g_AppState.renderer, surface);
    if (new_texture == nullptr) {
        SDL_Log("Failed to create telemetry map texture: %s", SDL_GetError());
        SDL_DestroySurface(surface);
        return false;
    }

    if (g_AppState.telemetryMapTexture != nullptr) {
        SDL_DestroyTexture(g_AppState.telemetryMapTexture);
    }

    g_AppState.telemetryMapTexture = new_texture;
    g_AppState.telemetryMapWidth = surface->w;
    g_AppState.telemetryMapHeight = surface->h;
    SDL_DestroySurface(surface);
    return true;
}

float NormalizePointCoord(float value) {
    // Telemetry coordinates are already normalized in [0,1].
    return std::clamp(value, 0.0f, 1.0f);
}
}

static bool EnsureReplayMapPreviewLoaded(const char *imagePath) {
    if (g_AppState.replay_details_map_preview != nullptr) {
        return true;
    }
    if (g_AppState.renderer == nullptr) {
        return false;
    }

    SDL_Surface *surface = IMG_Load(imagePath);
    if (surface == nullptr) {
        SDL_Log("Failed to load map preview surface '%s': %s", imagePath, SDL_GetError());
        return false;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(g_AppState.renderer, surface);
    SDL_DestroySurface(surface);

    if (texture == nullptr) {
        SDL_Log("Failed to create map preview texture '%s': %s", imagePath, SDL_GetError());
        return false;
    }

    g_AppState.replay_details_map_preview = texture;
    return true;
}

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
    const char *imagePath = "assets/map_images/avg_abandoned_town_tankmap.png";
    if (EnsureReplayMapPreviewLoaded(imagePath)) {
        ImGui::Image((ImTextureID) (intptr_t) g_AppState.replay_details_map_preview,
                     ImVec2(Constants::MAP_PREVIEW_WIDTH, Constants::MAP_PREVIEW_HEIGHT));
    } else {
        ImGui::Button("[ Map Image ]", ImVec2(Constants::MAP_PREVIEW_WIDTH, Constants::MAP_PREVIEW_HEIGHT));
    }
    ImGui::EndGroup();

    ImGui::Separator();

    // Action Buttons
    if (ImGui::Button("Download Server Replay")) {
        // TODO: Implement server replay
    }
    ImGui::SameLine();
    if (ImGui::Button("Watch Server Replay")) {
        // TODO: Implement server playback
    }
    ImGui::SameLine();
    if (ImGui::Button("Export Replay Plot")) {
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
                ImGui::TableSetupColumn("▮");
                ImGui::TableSetupColumn("┚");
                ImGui::TableSetupColumn("Assists");
                ImGui::TableSetupColumn("△");
                ImGui::TableSetupColumn("AI Kills");
                ImGui::TableSetupColumn("Awarded damage");
                ImGui::TableSetupColumn("▲");
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

void Gui_OnTelemetryUpdate(const TelemetryUpdate &update) {
    g_AppState.telemetryPositions = update.positions;
    g_AppState.telemetryMapName = update.map_name;
    if (g_AppState.telemetryMapName != g_AppState.telemetryMapLastName) {
        g_AppState.telemetryMapZoom = 1.0f;
        g_AppState.telemetryMapPanX = 0.0f;
        g_AppState.telemetryMapPanY = 0.0f;
        g_AppState.telemetryMapLastName = g_AppState.telemetryMapName;
    }
    ReloadTelemetryMapTexture();
}

void Gui_TelemetryMapWindow() {
    if (!g_AppState.showTelemetryMapWindow) return;

    ImGui::SetNextWindowPos(ImVec2(920, 40), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(560, 560), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Telemetry Map", &g_AppState.showTelemetryMapWindow)) {
        ImGui::Checkbox("Live updates", &g_AppState.prefs.telemetryLiveUpdates);
        if (ImGui::Button("Reset View")) {
            g_AppState.telemetryMapZoom = 1.0f;
            g_AppState.telemetryMapPanX = 0.0f;
            g_AppState.telemetryMapPanY = 0.0f;
        }
        ImGui::SameLine();
        ImGui::TextDisabled("Wheel: zoom, Middle mouse drag: pan");
        ImGui::Text("Map: %s", g_AppState.telemetryMapName.empty() ? "unknown" : g_AppState.telemetryMapName.c_str());
        ImGui::Text("Points: %d", static_cast<int>(g_AppState.telemetryPositions.size()));
        ImGui::Separator();

        if (g_AppState.telemetryMapTexture == nullptr || g_AppState.telemetryMapWidth <= 0 || g_AppState.telemetryMapHeight <= 0) {
            ImGui::TextDisabled("Waiting for telemetry map from localhost:8111/map.img");
        } else {
            const ImVec2 avail = ImGui::GetContentRegionAvail();
            const float aspect = static_cast<float>(g_AppState.telemetryMapWidth) / static_cast<float>(g_AppState.telemetryMapHeight);

            ImVec2 image_size = avail;
            if (image_size.y > 0.0f && image_size.x / image_size.y > aspect) {
                image_size.x = image_size.y * aspect;
            } else if (aspect > 0.0f) {
                image_size.y = image_size.x / aspect;
            }

            const auto clamp_map_view = [](float &zoom, float &pan_x, float &pan_y) {
                zoom = std::clamp(zoom, 1.0f, 12.0f);
                const float visible = 1.0f / zoom;
                pan_x = std::clamp(pan_x, 0.0f, 1.0f - visible);
                pan_y = std::clamp(pan_y, 0.0f, 1.0f - visible);
            };

            clamp_map_view(g_AppState.telemetryMapZoom, g_AppState.telemetryMapPanX, g_AppState.telemetryMapPanY);

            float visible_w = 1.0f / g_AppState.telemetryMapZoom;
            float visible_h = 1.0f / g_AppState.telemetryMapZoom;
            ImVec2 uv0(g_AppState.telemetryMapPanX, g_AppState.telemetryMapPanY);
            ImVec2 uv1(g_AppState.telemetryMapPanX + visible_w, g_AppState.telemetryMapPanY + visible_h);

            const ImVec2 image_pos = ImGui::GetCursorScreenPos();
            ImGui::Image((ImTextureID) (intptr_t) g_AppState.telemetryMapTexture, image_size, uv0, uv1);

            const bool hovered = ImGui::IsItemHovered();
            ImGuiIO &io = ImGui::GetIO();

            if (hovered && io.MouseWheel != 0.0f && image_size.x > 0.0f && image_size.y > 0.0f) {
                const float local_x = std::clamp((io.MousePos.x - image_pos.x) / image_size.x, 0.0f, 1.0f);
                const float local_y = std::clamp((io.MousePos.y - image_pos.y) / image_size.y, 0.0f, 1.0f);

                const float anchor_u = uv0.x + local_x * visible_w;
                const float anchor_v = uv0.y + local_y * visible_h;

                const float zoom_factor = io.MouseWheel > 0.0f ? 1.2f : (1.0f / 1.2f);
                g_AppState.telemetryMapZoom = std::clamp(g_AppState.telemetryMapZoom * zoom_factor, 1.0f, 12.0f);

                visible_w = 1.0f / g_AppState.telemetryMapZoom;
                visible_h = 1.0f / g_AppState.telemetryMapZoom;
                g_AppState.telemetryMapPanX = anchor_u - local_x * visible_w;
                g_AppState.telemetryMapPanY = anchor_v - local_y * visible_h;
                clamp_map_view(g_AppState.telemetryMapZoom, g_AppState.telemetryMapPanX, g_AppState.telemetryMapPanY);
            }

            if (hovered && ImGui::IsMouseDown(ImGuiMouseButton_Middle) && image_size.x > 0.0f && image_size.y > 0.0f) {
                g_AppState.telemetryMapPanX -= (io.MouseDelta.x / image_size.x) * visible_w;
                g_AppState.telemetryMapPanY -= (io.MouseDelta.y / image_size.y) * visible_h;
                clamp_map_view(g_AppState.telemetryMapZoom, g_AppState.telemetryMapPanX, g_AppState.telemetryMapPanY);
            }

            visible_w = 1.0f / g_AppState.telemetryMapZoom;
            visible_h = 1.0f / g_AppState.telemetryMapZoom;
            uv0 = ImVec2(g_AppState.telemetryMapPanX, g_AppState.telemetryMapPanY);
            uv1 = ImVec2(g_AppState.telemetryMapPanX + visible_w, g_AppState.telemetryMapPanY + visible_h);

            ImGui::Text("Zoom: %.2fx", g_AppState.telemetryMapZoom);

            auto HexToImU32 = [](const char* hexStr) -> ImU32 {
                int r, g, b;
                if (hexStr[0] == '#') hexStr++;

                // sscanf returns the number of items successfully filled
                if (sscanf(hexStr, "%02x%02x%02x", &r, &g, &b) == 3) {
                    return IM_COL32(r, g, b, 255);
                }

                return IM_COL32(255, 255, 255, 255); // Default to white if parsing fails
            };

            if (g_AppState.prefs.telemetryLiveUpdates) {
                ImDrawList *draw_list = ImGui::GetWindowDrawList();
                for (const auto &[x, y, color, unit_icon]: g_AppState.telemetryPositions) {
                    const float nx = NormalizePointCoord(x);
                    const float ny = NormalizePointCoord(y);
                    if (nx < uv0.x || nx > uv1.x || ny < uv0.y || ny > uv1.y) {
                        continue;
                    }

                    const float local_x = (nx - uv0.x) / (uv1.x - uv0.x);
                    const float local_y = (ny - uv0.y) / (uv1.y - uv0.y);
                    const ImVec2 point_pos(image_pos.x + local_x * image_size.x, image_pos.y + local_y * image_size.y);

                    if (g_WtSymbolsFont) {
                        constexpr float kSymbolFontSize = 16.0f;
                        draw_list->AddText(g_WtSymbolsFont, kSymbolFontSize+1, point_pos, IM_COL32(0, 0, 0, 255), Constants::UNICODE_SYMBOLS[unit_icon].c_str());
                        draw_list->AddText(g_WtSymbolsFont, kSymbolFontSize, point_pos, HexToImU32(color.c_str()), Constants::UNICODE_SYMBOLS[unit_icon].c_str());
                    } else {
                        draw_list->AddCircleFilled(point_pos, 4.0f, HexToImU32(color.c_str()));
                    }
                }
            }

        }
    }
    ImGui::End();
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
        ImGui::Text("Crew: %d / %d", g_AppState.rpc_party.CurrentSize(), g_AppState.rpc_party.MaxSize());

        ImGui::Separator();
        ImGui::Text("Current Image: %s", g_AppState.currentRpcImageKey.c_str());

        // Display the RPC image if loaded
        if (g_AppState.replay_details_map_preview) {
            ImGui::Separator();
            ImGui::Text("Activity Image:");

            // Display at 60x60 pixels
            ImGui::Image((ImTextureID) (intptr_t) g_AppState.replay_details_map_preview,
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

        ImGui::Spacing();
        ImGui::Checkbox("Enable Live Telemetry Updates", &tempPrefs.telemetryLiveUpdates);
        ImGui::TextDisabled("If enabled, the map will show live unit positions during playback (similar to in-game minimap).");

        // Buttons
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("OK", ImVec2(100, 0))) {
            g_AppState.prefs = tempPrefs;
            SavePreferences(g_AppState.prefs);
            g_AppState.showPreferencesDialog = false;
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
            ImGui::MenuItem("Telemetry Map", nullptr, &g_AppState.showTelemetryMapWindow);
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
