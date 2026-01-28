#include "replay_manager.h"
#include <filesystem>
#include <iostream>

extern AppState g_AppState;

void LoadReplaysFromDisk() {
    const std::filesystem::path sandbox{g_AppState.prefs.replayPath};

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
