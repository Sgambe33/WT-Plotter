#include "preferences.h"
#include <fstream>
#include "logger.h"

std::string GetPreferencesFilePath() {
    return "preferences.json";
}

void SavePreferences(const Preferences& prefs) {
    try {
        std::ofstream file(GetPreferencesFilePath());
        if (file.is_open()) {
            json j = prefs.to_json();
            file << j.dump(4);
            file.close();
        }
    } catch (const std::exception& e) {
        app_log::error(std::string("Error saving preferences: ") + e.what());
    }
}

void LoadPreferences(Preferences& prefs) {
    try {
        std::ifstream file(GetPreferencesFilePath());
        if (file.is_open()) {
            json j;
            file >> j;
            prefs.from_json(j);
            file.close();
        }
    } catch (const std::exception& e) {
        app_log::error(std::string("Error loading preferences: ") + e.what());
    }
}
