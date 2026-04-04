-- Table for ReplayHeader metadata
-- This allows for fast querying, filtering, and sorting of replays.
CREATE TABLE IF NOT EXISTS replays (
                                       session_id TEXT PRIMARY KEY,      -- ReplayHeader.sessionId
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
);

-- Table for the heavily packed vectors and BLK maps
-- Kept separate to keep the 'replays' table scans fast.
CREATE TABLE IF NOT EXISTS replay_data (
                                           replay_id INTEGER PRIMARY KEY,        -- Foreign key to replays.id

    -- BLK Maps (Can be serialized as JSON text or custom binary BLOBs)
                                           settings_blk TEXT,
                                           results_blk TEXT,

    -- Packet Vectors (Stored as compressed binary BLOBs)
                                           chat_packets BLOB,                    -- Compressed serialized vector<ChatPacket>
                                           kill_packets BLOB,                    -- Compressed serialized vector<KillPacket>
                                           award_packets BLOB,                   -- Compressed serialized vector<AwardPacket>
                                           movement_packets BLOB,                -- Compressed serialized vector<MovementPacket>
                                           slot_packets BLOB,                    -- Compressed serialized vector<SlotPacket>

                                           FOREIGN KEY (replay_id) REFERENCES replays (id) ON DELETE CASCADE
    );

-- Indexes to speed up common lookups
CREATE INDEX IF NOT EXISTS idx_replays_session_id ON replays(session_id);
CREATE INDEX IF NOT EXISTS idx_replays_start_time ON replays(start_time_epoch_ms);
CREATE INDEX IF NOT EXISTS idx_replays_location ON replays(raw_location_name);