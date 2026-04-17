#pragma once

#include <string>

namespace app_log {
    // Initializes the file logger. If no path is provided, logs/wt_plotter.log is used.
    bool init(const std::string &logFilePath = "");

    // Flushes and closes the log file.
    void shutdown();

    // Logging helpers.
    void info(const std::string &message);
    void warn(const std::string &message);
    void error(const std::string &message);

    // Returns the currently configured log file path.
    std::string log_file_path();
}

