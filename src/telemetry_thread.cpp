#include "telemetry_thread.h"
#include "md5.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <iostream>
#include "logger.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include "SDL3/SDL_events.h"
using json = nlohmann::json;

Uint32 EVENT_TELEMETRY_UPDATED = SDL_RegisterEvents(1);

size_t TelemetryManager::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    const size_t realSize = size * nmemb;
    auto* s = static_cast<std::string*>(userp);
    s->append(static_cast<char*>(contents), realSize);
    return realSize;
}

void TelemetryManager::Start(std::chrono::milliseconds poll_interval, UpdateCallback cb) {
    if (m_Running.load()) return;

    m_Callback = cb;
    m_Interval = poll_interval;
    m_Running.store(true);

    m_Worker = std::thread(&TelemetryManager::WorkerLoop, this);
}

void TelemetryManager::Stop() {
    if (!m_Running.load()) return;

    m_Running.store(false);
    m_Cv.notify_all();

    if (m_Worker.joinable()) {
        m_Worker.join();
    }
}

bool TelemetryManager::TryPopUpdate(TelemetryUpdate &out) {
    std::lock_guard<std::mutex> lk(m_Mutex);
    if (m_Queue.empty()) return false;

    out = m_Queue.front();
    m_Queue.pop_front();
    return true;
}

// Parse indicators JSON (from /indicators)
void TelemetryManager::ParseIndicators(const std::string &buf, TelemetryUpdate &telemetry_update, bool &indicators_valid) {
    indicators_valid = false;
    if (buf.empty()) return;
    try {
        const auto indicators_json = json::parse(buf);
        indicators_valid = indicators_json.value("valid", false);
        telemetry_update.unit_name = indicators_json.value("type", "");
        telemetry_update.current_crew = indicators_json.value("crew_current", 0); //TODO: use getparty.size?
        telemetry_update.total_crew = indicators_json.value("crew_total", 0);
        telemetry_update.unit_speed = indicators_json.value("speed", 0);
    } catch (const std::exception &e) {
        app_log::error(std::string("telemetry_thread: indicators JSON parse error: ") + e.what());
    }
}

// Parse map_info JSON (from /map_info.json)
void TelemetryManager::ParseMapInfo(const std::string &buf, TelemetryUpdate &telemetry_update, bool &map_valid) {
    map_valid = false;
    if (buf.empty()) return;
    try {
        const auto map_info_json = json::parse(buf);
        map_valid = map_info_json.value("valid", false);
        if (!map_valid) return;
        // Attempt to read map hash if provided
        if (map_info_json.contains("map_hash") && map_info_json["map_hash"].is_string()) {
            std::string map_md5_hash = map_info_json["map_hash"].get<std::string>();
            auto it = m_MapNameCache.find(map_md5_hash);
            if (it != m_MapNameCache.end()) {
                telemetry_update.map_name = it->second;
            }
        }
    } catch (const std::exception &e) {
        app_log::error(std::string("telemetry_thread: map_info JSON parse error: ") + e.what());
    }
}

void TelemetryManager::WorkerLoop() {
    CURL *curl = curl_easy_init();
    if (!curl) {
        app_log::error("status_thread: curl_easy_init failed");
        return;
    }

    // Configure curl defaults once
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 1500L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);

    const char *urls[] = {"http://127.0.0.1:8111/indicators", "http://127.0.0.1:8111/map_info.json"};
    constexpr size_t IDX_INDICATORS = 0;
    constexpr size_t IDX_MAPINFO = 1;
    constexpr size_t url_count = std::size(urls);

    constexpr auto map_img_url = "http://127.0.0.1:8111/map.img";

    int fail_count = 0; // Thread-safe fail counter (non-static)
    std::string buffers[2];
    std::string mapImgBuf;

    while (m_Running.load()) {
        // Pre-allocate string buffers to avoid repeated allocations
        buffers[IDX_INDICATORS].clear();
        buffers[IDX_MAPINFO].clear();
        mapImgBuf.clear();

        bool fetched_any = false;
        bool got_map_img = false;

        // Fetch JSON endpoints
        for (size_t i = 0; i < url_count; ++i) {
            curl_easy_setopt(curl, CURLOPT_URL, urls[i]);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffers[i]);

            CURLcode res = curl_easy_perform(curl);
            if (res == CURLE_OK) {
                long http_code = 0;
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
                if (http_code >= 200 && http_code < 300 && !buffers[i].empty()) {
                    fetched_any = true;
                }
            }
        }

        // Fetch map image (binary) - only if map_valid might be true (optimization)
        // This reduces unnecessary map.img downloads when not in match
        if (fetched_any) {
            curl_easy_setopt(curl, CURLOPT_URL, map_img_url);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &mapImgBuf);
            CURLcode r = curl_easy_perform(curl);
            if (r == CURLE_OK) {
                long http_code = 0;
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
                if (http_code >= 200 && http_code < 300 && !mapImgBuf.empty()) {
                    got_map_img = true;
                }
            }
        }

        if (fetched_any || got_map_img) {
            TelemetryUpdate telemetry_update;
            telemetry_update.Reset();

            bool indicators_valid = false;
            bool map_valid = false;

            ParseIndicators(buffers[IDX_INDICATORS], telemetry_update, indicators_valid);
            ParseMapInfo(buffers[IDX_MAPINFO], telemetry_update, map_valid);

            // If we fetched the raw image, compute MD5 and lookup in mapNameCache
            // Only compute MD5 if map name isn't already set from JSON
            if (got_map_img && telemetry_update.map_name.empty()) {
                std::string hash = md5_hex(reinterpret_cast<const uint8_t *>(mapImgBuf.data()), mapImgBuf.size());
                auto it = m_MapNameCache.find(hash);
                if (it != m_MapNameCache.end()) {
                    telemetry_update.map_name = it->second;
                } else {
                    // not found: insert placeholder (hex) to cache for later manual mapping
                    std::string unknown_name = "unknown_map_" + hash.substr(0, 8);
                    m_MapNameCache.emplace(hash, unknown_name);
                    telemetry_update.map_name = std::move(unknown_name);
                }
            }

            // map_info.json == valid:false && indicators == valid:true -> player in hangar or lobby
            // map_info.json == valid:true && indicators == valid:true -> player is in match
            if (indicators_valid && map_valid) {
                telemetry_update.player_state = TelemetryUpdate::PlayerState::InMatch;
            } else if (indicators_valid && !map_valid) {
                telemetry_update.player_state = TelemetryUpdate::PlayerState::InHangar;
            } else {
                telemetry_update.player_state = TelemetryUpdate::PlayerState::Unknown;
            }

            {
                // Queue update with bounded buffer (max 16 items)
                std::lock_guard lk(m_Mutex);
                m_Queue.push_back(telemetry_update);
                if (m_Queue.size() > 16) {
                    m_Queue.pop_front();
                }
            }
            m_Cv.notify_all();

            if (m_Callback) {
                try {
                    m_Callback(telemetry_update);
                } catch (const std::exception &e) {
                    app_log::error(std::string("status_thread: user callback threw: ") + e.what());
                }
            }
            fail_count = 0; // Reset on success
        } else {
            // Log intermittently on failure
            fail_count++;
            if (fail_count % 10 == 0) {
                app_log::warn("status_thread: failed to query status endpoint");
            }
        }

        // Sleep with early exit
        std::unique_lock<std::mutex> lk(m_Mutex);
        m_Cv.wait_for(lk, m_Interval, [this] { return !m_Running.load(); });
    }

    curl_easy_cleanup(curl);
}