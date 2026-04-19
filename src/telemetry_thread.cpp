#include "telemetry_thread.h"
#include "md5.h"
#include <algorithm>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <iostream>
#include "logger.h"
#include "image_utils.h"
#include "constants.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "SDL3/SDL_events.h"
using json = nlohmann::json;

namespace {
void CollectPointsFromNode(const json &node, std::vector<std::tuple<float, float, std::string, std::string>> &out) {
    if (node.is_object()) {
        const auto x_it = node.find("x");
        const auto y_it = node.find("y");
        if (x_it != node.end() && y_it != node.end() && x_it->is_number() && y_it->is_number()) {
            std::string color = "point";
            std::string unit_icon = "MediumTank";
            const auto color_it = node.find("color");
            const auto icon_it = node.find("icon");

            if (color_it != node.end() && color_it->is_string()) {
                color = color_it->get<std::string>();
            }

            if (icon_it != node.end() && icon_it->is_string()) {
                unit_icon = icon_it->get<std::string>();
            }
            out.emplace_back(x_it->get<float>(), y_it->get<float>(), color, unit_icon);
        }

        for (const auto &item: node.items()) {
            CollectPointsFromNode(item.value(), out);
        }
        return;
    }

    if (node.is_array()) {
        for (const auto &entry: node) {
            CollectPointsFromNode(entry, out);
        }
    }
}
}

Uint32 EVENT_TELEMETRY_UPDATED = SDL_RegisterEvents(1);

// Callback specifically for std::string (JSON endpoints)
size_t StringWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    auto* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), realsize);
    return realsize;
}

// Callback specifically for std::vector<uint8_t> (Binary Image endpoints)
size_t VectorWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    auto* vec = static_cast<std::vector<uint8_t>*>(userp);
    auto* data = static_cast<uint8_t*>(contents);
    vec->insert(vec->end(), data, data + realsize);
    return realsize;
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
    } catch (const std::exception &e) {
        app_log::error(std::string("telemetry_thread: map_info JSON parse error: ") + e.what());
    }
}

// Parse map_info JSON (from /map_obj.json)
void TelemetryManager::ParseMapObj(const std::string &buf, TelemetryUpdate &telemetry_update) {
    telemetry_update.positions.clear();
    if (buf.empty()) return;
    try {
        const auto map_objects_json = json::parse(buf);
        CollectPointsFromNode(map_objects_json, telemetry_update.positions);
    } catch (const std::exception &e) {
        app_log::error(std::string("telemetry_thread: map_obj JSON parse error: ") + e.what());
    }
}

inline int HexCharToInt(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}

// Calculates Hamming distance between two arbitrarily long hex strings
int StringHammingDistance(const std::string& hex1, const std::string& hex2) {
    int bit_diff_total = 0;
    const size_t shared_length = hex1.length() < hex2.length() ? hex1.length() : hex2.length();

    for (size_t i = 0; i < shared_length; ++i) {
        const int nibble1 = HexCharToInt(hex1[i]);
        const int nibble2 = HexCharToInt(hex2[i]);

        // XOR the two nibbles and count the differing bits.
        bit_diff_total += __builtin_popcount(static_cast<unsigned int>(nibble1 ^ nibble2));
    }

    // Punish differing lengths (each missing hex char represents 4 missing bits).
    bit_diff_total += std::abs(static_cast<int>(hex1.length()) - static_cast<int>(hex2.length())) * 4;

    return bit_diff_total;
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

    const char *urls[] = {
        "http://127.0.0.1:8111/indicators",
        "http://127.0.0.1:8111/map_info.json",
        "http://127.0.0.1:8111/map_obj.json"
    };
    constexpr size_t IDX_INDICATORS = 0;
    constexpr size_t IDX_MAPINFO = 1;
    constexpr size_t IDX_MAPOBJ = 2;
    constexpr size_t url_count = std::size(urls);

    constexpr auto map_img_url = "http://127.0.0.1:8111/map.img";

    int fail_count = 0; // Thread-safe fail counter (non-static)
    std::string buffers[3];
    std::vector<uint8_t> mapImgBuf;

    while (m_Running.load()) {
        // Pre-allocate string buffers to avoid repeated allocations
        buffers[IDX_INDICATORS].clear();
        buffers[IDX_MAPINFO].clear();
        buffers[IDX_MAPOBJ].clear();
        mapImgBuf.clear();

        bool fetched_any = false;

        // Fetch JSON endpoints
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, StringWriteCallback);
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

        if (fetched_any) {
            TelemetryUpdate telemetry_update;
            telemetry_update.Reset();

            bool indicators_valid = false;
            bool map_valid = false;

            ParseIndicators(buffers[IDX_INDICATORS], telemetry_update, indicators_valid);
            ParseMapInfo(buffers[IDX_MAPINFO], telemetry_update, map_valid);
            ParseMapObj(buffers[IDX_MAPOBJ], telemetry_update);

            if (telemetry_update.map_name.empty() && !m_Current_map.empty()) {
                telemetry_update.map_name = m_Current_map;
            }

            bool got_map_img = false;
            const bool should_fetch_map_img = indicators_valid && map_valid &&
                                              telemetry_update.map_name.empty() && m_Current_map.empty();

            // Fetch map image only while in match and map identity is still unknown.
            if (should_fetch_map_img) {
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, VectorWriteCallback);
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

            if (got_map_img && telemetry_update.map_name.empty() && m_Current_map.empty()) {
                int width, height, channels;
                unsigned char* pixels = stbi_load_from_memory(
                    mapImgBuf.data(),
                    mapImgBuf.size(),
                    &width,
                    &height,
                    &channels,
                    0 // Tell stb to keep the original channel count
                    );

                if (pixels) {
                    // Calculate the 64-bit dHash (using a 9x8 grid)
                    std::string hash = ImageUtils::PHashFromRawPixels(pixels, width, height, channels);
                    std::cout << "dHash: " << hash << std::endl;

                    // Free the pixel memory allocated by stb_image
                    stbi_image_free(pixels);

                    // --- NEW HAMMING DISTANCE SEARCH ---
                    int min_distance=64;
                    std::string best_match = "unknownmap";
                    for (const auto& [map_hash_str, map_name] : Constants::MAP_DHASHES) {
                        int distance = StringHammingDistance(hash, map_hash_str);
                        if (distance < min_distance) {
                            min_distance = distance;
                            best_match = map_name;
                        }
                    }
                    std::cout<<"best match: " << best_match << " " << min_distance << std::endl;

                    telemetry_update.map_name = best_match;
                    m_Current_map = best_match;



                    // ------------------------------------

                } else {
                    std::cerr << "stb_image failed to decode the data. Reason: " << stbi_failure_reason() << std::endl;
                }
            }

            // map_info.json == valid:false && indicators == valid:true -> player in hangar or lobby
            // map_info.json == valid:true && indicators == valid:true -> player is in match
            if (indicators_valid && map_valid) {
                telemetry_update.player_state = TelemetryUpdate::PlayerState::InMatch;
            } else if (indicators_valid && !map_valid) {
                telemetry_update.player_state = TelemetryUpdate::PlayerState::InHangar;
                m_Current_map.clear();
            } else {
                telemetry_update.player_state = TelemetryUpdate::PlayerState::Unknown;
                m_Current_map.clear();
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
