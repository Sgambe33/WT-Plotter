#ifndef PREFERENCES_H
#define PREFERENCES_H

#include "app_state.h"
#include <string>

std::string GetPreferencesFilePath();
void SavePreferences(const Preferences& prefs);
void LoadPreferences(Preferences& prefs);

#endif // PREFERENCES_H
