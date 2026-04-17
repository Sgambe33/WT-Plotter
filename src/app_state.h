#ifndef APP_STATE_H
#define APP_STATE_H

#include <map>
#include <string>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <SDL3/SDL.h>

using json = nlohmann::json;

// ==========================================
// PREFERENCES
// ==========================================
struct Preferences {
    std::string replayPath = R"(E:/SteamLibrary/steamapps/common/War Thunder/Replays)";
    std::string language = "en";
    bool autoDownloadServerReplay = false;
    bool enableDiscordRichPresence = true;

    [[nodiscard]] json to_json() const;
    void from_json(const json& j);
};

// ==========================================
// REPLAY FILE
// ==========================================
struct ReplayFile {
    std::filesystem::path fullPath;
    std::string filename;
    std::string date; // "2026.01.21"
    std::string time; // "22.57.15"
    int id{};
};

// ==========================================
// APP STATE
// ==========================================
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
    bool showAboutDialog = false;
    bool showPreferencesDialog = false;

    // CACHED REPLAY DATA
    std::map<std::string, std::vector<ReplayFile>> replaysByDate;
    bool replaysLoaded = false;

    // TREE STATE
    std::map<std::string, bool> treeNodeOpen;
    bool showPlaybackWindow = false;
    bool showRichPresence = false;

    // Discord Rich Presence
    std::string activityName = "War Thunder";
    std::string activityStatus = "In hangar";
    std::string activityDetails = "Idle";
    std::string currentRpcImageKey = "logowt_stripe_flat";

    // Preferences
    Preferences prefs;

    // SDL Renderer pointer for texture loading
    SDL_Renderer* renderer = nullptr;

    // RPC Image texture
    SDL_Texture* rpcImageTexture = nullptr;
    int rpcImageWidth = 0;
    int rpcImageHeight = 0;
};

extern AppState g_AppState;

#endif // APP_STATE_H
