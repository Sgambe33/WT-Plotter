#pragma once

#include <functional>
#include <chrono>
#include <string>

namespace status_thread {
    struct ServerStatus {
        enum class PlayState { Unknown = 0, InHangar = 1, InMatch = 2 };

        bool online = false;
        std::string map;
        std::string unit;
        int unitCrew = 0;
        int unitCrewTotal = 0;
        int unitSpeed = 0;
        PlayState playState = PlayState::Unknown;
    };

    using UpdateCallback = std::function<void(const ServerStatus&)>;

    // Starts the polling thread. If provided, callback will be invoked on worker thread
    // with each new status; prefer using try_pop_status() from the main thread instead
    // to marshal Discord SDK calls into the main thread.
    void start_status_thread(UpdateCallback cb = nullptr, std::chrono::milliseconds poll_interval = std::chrono::milliseconds(2000));

    // Signals the thread to stop and joins it.
    void stop_status_thread();

    // Try to pop the next available status update from the internal queue.
    // Returns true if an update was returned.
    bool try_pop_status(ServerStatus &out);
}
