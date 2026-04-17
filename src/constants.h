#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "imgui.h"

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
    inline auto LOADING_TEXT = "Loading replays...";

    inline std::string SQLITE_TABLES_DEFINITIONS[] = {
        R"(
            CREATE TABLE IF NOT EXISTS ReplayMetadata (
                session_id TEXT PRIMARY KEY,
                author_id INTEGER,
                wrpl_version INTEGER,                 -- ReplayHeader.wrplVersion
                raw_level TEXT,                       -- ReplayHeader.rawLevel
                raw_level_settings TEXT,              -- ReplayHeader.rawLevelSettings
                raw_battle_type TEXT,                 -- ReplayHeader.rawBattleType
                raw_environment TEXT,                 -- ReplayHeader.rawEnvironment
                raw_visibility TEXT,                  -- ReplayHeader.rawVisibility
                difficulty INTEGER,                   -- ReplayHeader.difficulty
                session_type INTEGER,                 -- ReplayHeader.sessionType
                is_server INTEGER,                    -- ReplayHeader.isServer (SQLite uses INTEGER 0/1 for booleans)
                raw_location_name TEXT,               -- ReplayHeader.rawLocationName
                start_time_epoch_ms INTEGER,          -- ReplayHeader.startTimeEpochS
                time_limit_in_minutes INTEGER,        -- ReplayHeader.timeLimitInMinutes
                score_limit INTEGER,                  -- ReplayHeader.scoreLimit
                raw_battle_class TEXT,                -- ReplayHeader.rawBattleClass
                raw_battle_kill_streak TEXT           -- ReplayHeader.rawBattleKillStreak
            )
        )",
        R"(
            CREATE TABLE IF NOT EXISTS ReplayData (
                session_id TEXT PRIMARY KEY,        -- Foreign key to replays.id
                settings_blk JSON,
                results_blk JSON,
                chat_packets BLOB,                    -- Compressed serialized vector<ChatPacket>
                kill_packets BLOB,                    -- Compressed serialized vector<KillPacket>
                award_packets BLOB,                   -- Compressed serialized vector<AwardPacket>
                movement_packets BLOB,                -- Compressed serialized vector<MovementPacket>
                slot_packets BLOB,                    -- Compressed serialized vector<SlotPacket>
                FOREIGN KEY (session_id) REFERENCES ReplayMetadata(session_id) ON DELETE CASCADE
            )
        )"
    };

}

#endif // CONSTANTS_H
