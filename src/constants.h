#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "imgui.h"
#include <map>
#include <string>

namespace Constants {
    constexpr float TOOLBAR_HEIGHT = 35.0f;
    constexpr float FOOTER_HEIGHT = 60.0f;
    constexpr float PROGRESS_BAR_WIDTH = 200.0f;
    constexpr float MAP_PREVIEW_WIDTH = 120.0f;
    constexpr float MAP_PREVIEW_HEIGHT = 120.0f;
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

    inline std::map<std::string, std::string> UNICODE_SYMBOLS = {
        {"Fighter", "┤"},
        {"Attacker", "┞"},
        {"Bomber", "┠"},
        {"Player", "╧"},
        {"LightTank", "┪"},
        {"MediumTank", "┬"},
        {"HeavyTank", "┨"},
        {"SPAA", "┰"},
        {"TankDestroyer", "┴"},
        {"capture_zone", "╶"},
        {"respawn_base_tank", "␝"},


        {"Allies", "\uE001"},
        {"Axis", "\uE002"},
        {"Victory", "\uE003"},
        {"Defeat", "\uE004"},
        {"Unknown", "\uE000"}
    };

    inline std::map<std::string, std::string> MAP_DHASHES = {
        {"cc79230edcd98d34", "air_afghan_map"},
        {"c5c04b3be47d816d", "air_africa_desert_map"},
        {"b34ee39180e7e899", "air_denmark_map"},
        {"8cdf63689b328c69", "air_equatorial_island_map"},
        {"942669d986acd177", "air_falklands_map"},
        {"f83dd84c2bc46c96", "air_grand_canyon_map"},
        {"a5325255a5a95bb5", "air_israel_map"},
        {"8cbdf157678424ac", "air_kamchatka_map"},
        {"a561719acb2c69d3", "air_ladoga_map"},
        {"a26b692f7c951e06", "air_mysterious_valley_map"},
        {"a1671b8a53b6274d", "air_normandy_map"},
        {"b4b12a129ffdc3a0", "air_pyrenees_map"},
        {"856a7a9e3fb0c831", "air_race_phiphi_islands_map"},
        {"bc0fc5aa71559694", "air_skyscraper_city_map"},
        {"981ab4a727f19267", "air_smolensk_map"},
        {"dbd81406c77b5a4a", "air_southeastern_cliffs_map"},
        {"832fbd7cda134701", "air_south_eastern_city_map"},
        {"98c762162669bafc", "air_vietnam_map"},
        {"d7d08680d2fc9c6e", "arcade_africa_canyon_map"},
        {"858e5b3778c5e14c", "arcade_africa_seashore_map"},
        {"a18cdaaf09bbc253", "arcade_alps_map"},
        {"a976f96498e4486d", "arcade_asia_4roads_map"},
        {"fdbcd01b83a03a17", "arcade_canyon_snow_map"},
        {"d0c7d388f13f302e", "arcade_ireland_map"},
        {"d4624f9e474e2b61", "arcade_mediterranean_map"},
        {"dbc158f901dbfd00", "arcade_norway_fjords_map"},
        {"aa45d2a5de528575", "arcade_norway_green_map"},
        {"aa45d62d8af11573", "arcade_norway_plain_map"},
        {"c0597aa63c1b267e", "arcade_phiphi_crater_map"},
        {"d1e1690f463d26f2", "arcade_phiphi_crater_rocks_map"},
        {"e92d30d5c64a5c3d", "arcade_rice_terraces_map"},
        {"eb2ca4949b5956a3", "arcade_snow_rocks_map"},
        {"96ca6d317aa4d349", "arcade_tabletop_mountain_map"},
        {"a8a1a8b8a32d47df", "arcade_zhang_park_map"},
        {"8bb5c5669b16e941", "avg_abandoned_factory_map"},
        {"94c08b7174b3e55b", "avg_abandoned_factory_tankmap"},
        {"bf9c86a49e5a4266", "avg_abandoned_town_map"},
        {"d0da292295dedb51", "avg_abandoned_town_tankmap"},
        {"c5c04b3be47d816d", "avg_africa_desert_map"},
        {"ff1d8dc462391847", "avg_africa_desert_tankmap"},
        {"e00b84e2766cfc9b", "avg_alaska_town_map"},
        {"d4a535494abde368", "avg_alaska_town_tankmap"},
        {"cc9b9de7b838c320", "avg_american_valley_map"},
        {"dfc8e0843fdce083", "avg_american_valley_tankmap"},
        {"e698996c789f2c43", "avg_aral_sea_map"},
        {"f18fce7171c68431", "avg_aral_sea_tankmap"},
        {"9995666699996666", "avg_arctic_map"},
        {"da52433ee0073ef4", "avg_arctic_tankmap"},
        {"db952978a54be381", "avg_ardennes_map"},
        {"da952d68a14ee399", "avg_ardennes_snow_map"},
        {"fd51b183581cf4f0", "avg_ardennes_snow_tankmap"},
        {"fd51b1835a1cdc70", "avg_ardennes_tankmap"},
        {"f72bed9e501070a3", "avg_berlin_tankmap"},
        {"b383c96432b5ccf2", "avg_breslau_map"},
        {"9fcc7a3fc094849a", "avg_breslau_tankmap"},
        {"d34bb14be9834a6c", "avg_container_port_map"},
        {"c660391e3ed2b3c9", "avg_container_port_tankmap"},
        {"964b4da67353b8c4", "avg_eastern_europe_map"},
        {"ac888b95eaf63558", "avg_eastern_europe_tankmap"},
        {"be43790ceb84f01d", "avg_egypt_sinai_map"},
        {"d16d8a4da5b69592", "avg_egypt_sinai_tankmap"},
        {"b6ede9efb6c00090", "avg_european_fortress_map"},
        {"f630b8aea95b1c23", "avg_european_fortress_tankmap"},
        {"84101bfce2874ebf", "avg_finland_map"},
        {"87f1efbac3188942", "avg_finland_tankmap"},
        {"d3892c5a3d2d3467", "avg_football_field_map"},
        {"9c6968a9f1155769", "avg_fulda_map"},
        {"c81b774dd8b23730", "avg_fulda_tankmap"},
        {"8b4d331b8d3272f8", "avg_future_city_map"},
        {"95de692311ad1ef0", "avg_greece_map"},
        {"911033f6accfc965", "avg_greece_tankmap"},
        {"9f91581d4c6c8e4f", "avg_guadalcanal_map"},
        {"c078766e3fd84a85", "avg_guadalcanal_tankmap"},
        {"e436dac51ee3230e", "avg_hurtgen_map"},
        {"f8000ffa1357e4ab", "avg_hurtgen_tankmap"},
        {"9696f97f16802939", "avg_iberian_castle_map"},
        {"96f2788e6378ac43", "avg_iberian_castle_tankmap"},
        {"f770df184c0cec0e", "avg_ireland_tankmap"},
        {"c97a9b05666d8999", "avg_israel_map"},
        {"c9d6863132cdc7cc", "avg_israel_tankmap"},
        {"923518da6fd194b6", "avg_japan_map"},
        {"f163c7be48c1346c", "avg_japan_tankmap"},
        {"bbe4668a8f261cd8", "avg_karantan_tankmap"},
        {"d99906fc9127e09e", "avg_karelia_forest_a_map"},
        {"dfab2880ccb981dd", "avg_karelia_forest_a_tankmap"},
        {"ee6f5c3c4542e892", "avg_karpaty_passage_map"},
        {"c0cc1f17777c3033", "avg_karpaty_passage_tankmap"},
        {"a6c190ae19878dfe", "avg_korea_lake_map"},
        {"ab01a8ff14ab9ba1", "avg_korea_lake_tankmap"},
        {"e39ed62c29a2a399", "avg_krymsk_tankmap"},
        {"ce34623badc4748e", "avg_kursk_villages_tankmap"},
        {"a49c5d47cb1b78e0", "avg_lazzaro_italy_map"},
        {"dc366399269836d3", "avg_lazzaro_italy_new_city_tankmap"},
        {"a3e19a5367b09d4a", "avg_maginot_map"},
        {"e49ba5d0680d83ef", "avg_maginot_tankmap"},
        {"aad323a2e74754c6", "avg_mozdok_tankmap"},
        {"c63a4bf112e4ec8d", "avg_netherlands_map"},
        {"e16e8c0c30ef4b8f", "avg_netherlands_tankmap"},
        {"a1671b8a53b6274d", "avg_normandy_map"},
        {"845b572d7669b24a", "avg_normandy_tankmap"},
        {"a827d54c4a479ceb", "avg_northern_india_map"},
        {"eda4caa54835cb8d", "avg_northern_india_tankmap"},
        {"ebe649184f1567b0", "avg_northern_valley_map"},
        {"b8b0b41f161e4fa9", "avg_northern_valley_tankmap"},
        {"d5a534614abce578", "avg_nuclear_incident_tankmap"},
        {"907a2ca61bcd6735", "avg_poland_map"},
        {"927a2c271bcd6f30", "avg_poland_snow_map"},
        {"a380cc3767decc23", "avg_poland_snow_tankmap"},
        {"a744c8b57fde4920", "avg_poland_tankmap"},
        {"a6df4b26d7b110c8", "avg_port_novorossiysk_map"},
        {"ed8e8930d1cc9d99", "avg_port_novorossiysk_tankmap"},
        {"e45635ac58ef0a59", "avg_red_desert_map"},
        {"eb97bd4744169c05", "avg_red_desert_tankmap"},
        {"bb23bb588609a2bb", "avg_rheinland_map"},
        {"fd0fa60df0938f80", "avg_rheinland_tankmap"},
        {"a9b28be0fb42cd4c", "avg_sector_montmedy_map"},
        {"8fb283e0fb42cd4c", "avg_sector_montmedy_snow_map"},
        {"84f0d3927a9f1bc1", "avg_sector_montmedy_snow_tankmap"},
        {"9692d392f2cb4bc1", "avg_sector_montmedy_tankmap"},
        {"992ba9093bf2c5ca", "avg_snow_alps_map"},
        {"c4638b38e36e34c7", "avg_snow_alps_tankmap"},
        {"f58984a0766cbe93", "avg_soviet_range_map"},
        {"fcc142de8c3e3499", "avg_soviet_range_tankmap"},
        {"bbc9e484b666e489", "avg_soviet_suburban_map"},
        {"f98984e03666fe8a", "avg_soviet_suburban_snow_map"},
        {"acda850f6a62de64", "avg_soviet_suburban_snow_tankmap"},
        {"dc992aa7d8f68886", "avg_soviet_suburban_tankmap"},
        {"c9152ed2a5ca96ad", "avg_stalingrad_factory_tankmap"},
        {"81ef799a32666626", "avg_sweden_map"},
        {"c1323f4d6db42437", "avg_sweden_tankmap"},
        {"e65ae57c4cc2c748", "avg_syria_map"},
        {"c2d73e54b26a4b46", "avg_syria_tankmap"},
        {"de50435e74b005fd", "avg_training_ground_tankmap"},
        {"ccbd921cb61816fc", "avg_tunisia_desert_map"},
        {"d554f1854291f5f4", "avg_tunisia_desert_tankmap"},
        {"d0d637701c8f331e", "avg_vietnam_hills_map"},
        {"d72d2ae629da6681", "avg_vietnam_hills_tankmap"},
        {"bb11e65c4596be03", "avg_vlaanderen_map"},
        {"98d4e74fce486835", "avg_vlaanderen_tankmap"},
        {"be950fc1baa1ba21", "avg_volokolamsk_map"},
        {"f0b187ce1cb88cc7", "avg_volokolamsk_tankmap"},
        {"d3d81c72c70e9347", "avg_western_europe_map"},
        {"e09e1f2768cd1e54", "avg_western_europe_tankmap"},
        {"8ec6303999cfe68c", "avn_africa_gulf_map"},
        {"cb34cb34c978c936", "avn_africa_gulf_tankmap"},
        {"b5b4b2dad89c6449", "avn_aleutian_islands_map"},
        {"dfcfc78589858486", "avn_aleutian_islands_tankmap"},
        {"a8c4ca85b7dbca92", "avn_alps_fjord_map"},
        {"9bc1c3758ef48e24", "avn_alps_fjord_tankmap"},
        {"c926d23b05e72d1b", "avn_arabian_north_coast_map"},
        {"e5368007e4f4f1e6", "avn_arabian_north_coast_tankmap"},
        {"b3269933c66c6c63", "avn_bering_sea_map"},
        {"a8556aaad5aa55aa", "avn_bering_sea_tankmap"},
        {"e3635d9d880aceca", "avn_blacksea_port_map"},
        {"df689708816a877e", "avn_blacksea_port_tankmap"},
        {"99cd3332ce6499c9", "avn_coral_islands_map"},
        {"91cb1b9b339562b4", "avn_coral_islands_tankmap"},
        {"cd99992608dadaf2", "avn_england_shore_map"},
        {"df7e8859c0cad286", "avn_england_shore_tankmap"},
        {"d63021ef79929639", "avn_fiji_map"},
        {"8ad52275c2d4b8f9", "avn_fiji_tankmap"},
        {"cbcfa62450576768", "avn_finland_islands_map"},
        {"dccde9c1c601273b", "avn_finland_islands_tankmap"},
        {"fa9e050deb9604e9", "avn_franz_josef_land_map"},
        {"99d1d1d1d187870f", "avn_franz_josef_land_tankmap"},
        {"cc273a39738eb186", "avn_fuego_islands_map"},
        {"e1e7822eb056dea1", "avn_fuego_islands_tankmap"},
        {"93c3c566b86cd2c3", "avn_ice_port_map"},
        {"aaa865a5a3a9ad5a", "avn_ice_port_tankmap"},
        {"c4e4390a46f7bf88", "avn_ireland_bay_map"},
        {"e3abf193d1cc20ac", "avn_ireland_bay_tankmap"},
        {"db92163d75839166", "avn_japan_map"},
        {"d5f5950a68da2b0b", "avn_japan_tankmap"},
        {"dbd3a624e0de5a11", "avn_mediterranean_port_map"},
        {"dfcfca9217172290", "avn_mediterranean_port_tankmap"},
        {"a2a1d55e2e2755d2", "avn_new_zealand_map"},
        {"babab2d09094a53f", "avn_new_zealand_tankmap"},
        {"adece9cd83091c33", "avn_northwestern_islands_map"},
        {"99692dbe2698c933", "avn_northwestern_islands_tankmap"},
        {"903f89e968ac6774", "avn_north_sea_map"},
        {"f14e930546b1797a", "avn_north_sea_tankmap"},
        {"8bcdc839929366d6", "avn_norway_islands_map"},
        {"adb4a94ac25cf4aa", "avn_norway_islands_tankmap"},
        {"9495925a4bad6d63", "avn_peleliu_map"},
        {"9e86653133656c9e", "avn_peleliu_tankmap"},
        {"8384d16e65a37ce3", "avn_phang_nga_bay_islands_map"},
        {"dbd9d882e71c9682", "avn_phang_nga_bay_islands_tankmap"},
        {"98ecf45a284ed5d2", "avn_san_francisco_map"},
        {"99ccf9ad6d8d8620", "avn_san_francisco_tankmap"},
        {"d5d5b5340dcb7230", "avn_south_africa_map"},
        {"ceceeae7a5904261", "avn_south_africa_tankmap"},
        {"dec0016ff6b07946", "avn_sunken_city_map"},
        {"f8e981d2dd9a2a4c", "avn_sunken_city_tankmap"},
        {"cf9861673cb0d398", "avn_volcanic_island_map"},
        {"d98df197c94b6848", "avn_volcanic_island_tankmap"},
        {"b8a4b0bd886ef4b1", "berlin_map"},
        {"cad632358ad1387d", "britain_map"},
        {"c0b02117376f9f95", "bulge_map"},
        {"c91934e6cd19698f", "caribbean_islands_map"},
        {"b36c9c08e1c2ed3e", "dover_strait_map"},
        {"f1a5b4fbb080a1e6", "firing_range_tankmap"},
        {"c03f8f78184da7b1", "guadalcanal_map"},
        {"c3993a676cc93236", "guam_map"},
        {"ce4d303293e9713e", "honolulu_map"},
        {"c5329ae71ef0019f", "hurtgen_map"},
        {"a433a7b78cb93232", "iwo_jima_map"},
        {"eb1e9ea39940b60d", "khalkhin_gol_map"},
        {"db60a056af4817bb", "korea_map"},
        {"ef73e8a2a625d112", "korsun_map"},
        {"88cc6d3fd7e1340c", "krymsk_map"},
        {"9a253a3781f1ec6c", "kursk_map"},
        {"cf8a612d3492cb6b", "logowt_stripe_flat_map"},
        {"99c766989866c7c3", "malta_map"},
        {"8dde6211d9ce6239", "midway_map"},
        {"c351c2f9c9739343", "moscow_map"},
        {"d065ba4cf01b4f36", "mozdok_map"},
        {"f7cc4221de6fe018", "mozdok_winter_map"},
        {"8fd46e6ca199a1c3", "norway_map"},
        {"989a252ce961def4", "peleliu_map"},
        {"a1937fa4c633c91a", "port_moresby_map"},
        {"9c4ef389612dc8d9", "ruhr_map"},
        {"889837aeec615a76", "saipan_map"},
        {"d75a9c95b45e9026", "sicily_map"},
        {"e7b4e04276a629da", "spain_map"},
        {"e1cf9b33ce03c3c0", "stalingrad_map"},
        {"e1cf4a1a5e06cbd2", "stalingrad_w_map"},
        {"cd8c32728ccc33f3", "wake_island_map"},
        {"96b5e074a740fcc6", "water_map"},
        {"fe19bff06823f800", "zhengzhou_map"}
    };
}

#endif // CONSTANTS_H
