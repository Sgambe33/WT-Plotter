#include "app_state.h"

AppState g_AppState;

json Preferences::to_json() const {
    return json{
            {"replayPath", replayPath},
            {"language", language},
            {"autoDownloadServerReplay", autoDownloadServerReplay},
        {"enableDiscordRichPresence", enableDiscordRichPresence}
    };
}

void Preferences::from_json(const json& j) {
    if (j.contains("replayPath")) replayPath = j["replayPath"];
    if (j.contains("language")) language = j["language"];
    if (j.contains("autoDownloadServerReplay")) autoDownloadServerReplay = j["autoDownloadServerReplay"];
    if (j.contains("enableDiscordRichPresence")) enableDiscordRichPresence = j["enableDiscordRichPresence"];
}