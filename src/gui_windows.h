#ifndef GUI_WINDOWS_H
#define GUI_WINDOWS_H

#include "imgui.h"

// Forward declaration
extern ImFont *g_WtSymbolsFont;

// Panel drawing functions
void DrawReplayListPanel();
void DrawLoadingPanel();
void DrawReplayDetails();
void DrawPlaybackView();

// Window functions
void Gui_ReplaysWindow();
void Gui_DetailsWindow();
void Gui_PlaybackWindow();
void Gui_DiscordRichPresence();
void Gui_AboutDialog();
void Gui_PreferencesDialog();
void Gui_MenuBar();

#endif // GUI_WINDOWS_H
