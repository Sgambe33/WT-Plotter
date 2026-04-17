#include "logger.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>

namespace app_log {
    namespace {
        std::mutex g_logMutex;
        std::ofstream g_logFile;
        std::string g_logFilePath;
        bool g_initialized = false;

        std::string make_timestamp() {
            const auto now = std::chrono::system_clock::now();
            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
            const std::time_t t = std::chrono::system_clock::to_time_t(now);

            std::tm localTm{};
#ifdef _WIN32
            localtime_s(&localTm, &t);
#else
            localtime_r(&t, &localTm);
#endif

            std::ostringstream oss;
            oss << std::put_time(&localTm, "%Y-%m-%d %H:%M:%S")
                << '.' << std::setfill('0') << std::setw(3) << ms.count();
            return oss.str();
        }

        void write_line(const char *level, const std::string &message) {
            std::lock_guard<std::mutex> lock(g_logMutex);
            if (!g_initialized || !g_logFile.is_open()) {
                return;
            }

            g_logFile << '[' << make_timestamp() << "] [" << level << "] " << message << '\n';
            g_logFile.flush();
        }
    }

    bool init(const std::string &logFilePath) {
        std::lock_guard<std::mutex> lock(g_logMutex);

        if (g_logFile.is_open()) {
            g_logFile.close();
        }

        std::filesystem::path path = logFilePath.empty()
            ? std::filesystem::path("logs") / "wt_plotter.log"
            : std::filesystem::path(logFilePath);

        if (path.has_parent_path()) {
            std::error_code ec;
            std::filesystem::create_directories(path.parent_path(), ec);
        }

        g_logFile.open(path, std::ios::out | std::ios::app);
        g_initialized = g_logFile.is_open();
        g_logFilePath = path.string();

        if (g_initialized) {
            g_logFile << '[' << make_timestamp() << "] [INFO] Logger initialized" << '\n';
            g_logFile.flush();
        }

        return g_initialized;
    }

    void shutdown() {
        std::lock_guard<std::mutex> lock(g_logMutex);
        if (g_logFile.is_open()) {
            g_logFile << '[' << make_timestamp() << "] [INFO] Logger shutdown" << '\n';
            g_logFile.flush();
            g_logFile.close();
        }
        g_initialized = false;
    }

    void info(const std::string &message) {
        write_line("INFO", message);
    }

    void warn(const std::string &message) {
        write_line("WARN", message);
    }

    void error(const std::string &message) {
        write_line("ERROR", message);
    }

    std::string log_file_path() {
        std::lock_guard<std::mutex> lock(g_logMutex);
        return g_logFilePath;
    }
}

