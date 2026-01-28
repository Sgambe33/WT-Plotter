#include "preferences.h"
#include <fstream>
#include <iostream>

std::string GetPreferencesFilePath() {
    return "preferences.json";
}

void SavePreferences(const Preferences& prefs) {
    try {
        std::ofstream file(GetPreferencesFilePath());
        if (file.is_open()) {
            json j = prefs.to_json();
            file << j.dump(4); // Pretty print with 4 spaces
            file.close();
            std::cout << "✅ Preferences saved successfully\n";
        } else {
            std::cerr << "❌ Failed to open preferences file for writing\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "❌ Error saving preferences: " << e.what() << "\n";
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
            std::cout << "✅ Preferences loaded successfully\n";
        } else {
            std::cout << "ℹ️  No preferences file found, using defaults\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "⚠️  Error loading preferences: " << e.what() << ", using defaults\n";
    }
}
