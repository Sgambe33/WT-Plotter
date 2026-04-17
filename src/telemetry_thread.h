#pragma once

#include <atomic>
#include <functional>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <map>
#include <mutex>
#include <string>
#include <thread>

struct TelemetryUpdate {
    enum class PlayerState { Unknown = 0, InHangar = 1, InMatch = 2 };

    std::string unit_name;
    int total_crew;
    int actual_crew;
    int unit_speed;
    std::vector<std::tuple<float, float, std::string>> positions;
    std::string map_name;
    PlayerState player_state;

    void Reset() {
        unit_name.clear();
        total_crew = 0;
        actual_crew = 0;
        unit_speed = 0;
        positions.clear();
        map_name.clear();
        player_state = PlayerState::Unknown;
    }
};


class TelemetryManager {
    using UpdateCallback = std::function<void(const TelemetryUpdate&)>;
public:
    TelemetryManager() = default;
    ~TelemetryManager() { Stop(); }

    TelemetryManager(const TelemetryManager&) = delete;
    TelemetryManager& operator=(const TelemetryManager&) = delete;

    void Start(std::chrono::milliseconds poll_interval, UpdateCallback cb = nullptr);
    void Stop();
    bool TryPopUpdate(TelemetryUpdate& out);

private:
    void WorkerLoop();

    // Internal parsers
    static void ParseIndicators(const std::string& buf, TelemetryUpdate& update, bool& valid);
    void ParseMapInfo(const std::string& buf, TelemetryUpdate& update, bool& valid);
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

    // Thread management
    std::thread m_Worker;
    std::atomic<bool> m_Running{false};
    std::mutex m_Mutex;
    std::condition_variable m_Cv;
    std::chrono::milliseconds m_Interval{2000};

    // Data flow
    std::deque<TelemetryUpdate> m_Queue;
    UpdateCallback m_Callback{nullptr};

    // Caches
    std::map<std::string, std::string> m_MapNameCache = {
        {"3a6b992635cb471d0d435eec3f28ee815d832f0a6666412ac6dce2e80", "air_afghan_map"},
        {"93acf3bcb73c8b04eb1426b61ffe3a3c", "avg_training_ground_tankmap"}
    };
};