#ifndef GUI_WINDOWS_H
#define GUI_WINDOWS_H

#include "imgui.h"
#include "telemetry_thread.h"

// Forward declaration
extern ImFont *g_WtSymbolsFont;

// Panel drawing functions
void DrawReplayListPanel();
void DrawLoadingPanel();
void DrawReplayDetails();
void DrawPlaybackView();
void Gui_OnTelemetryUpdate(const TelemetryUpdate &update);

// Window functions
void Gui_ReplaysWindow();
void Gui_DetailsWindow();
void Gui_PlaybackWindow();
void Gui_DiscordRichPresence();
void Gui_AboutDialog();
void Gui_PreferencesDialog();
void Gui_TelemetryMapWindow();
void Gui_MenuBar();

#endif // GUI_WINDOWS_H
