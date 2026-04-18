#include "tray_support.h"
#include "logger.h"

#ifdef _WIN32
extern "C" {
#include "tray.h"
}

#include <atomic>

namespace TraySupport {
    namespace {
        SDL_Window *g_window = nullptr;
        std::atomic<bool> g_quitRequested{false};
        bool g_initialized = false;

        constexpr auto kTrayIcon = "assets/icon.ico";
        constexpr auto kShowText = "Show";
        constexpr auto kHideText = "Hide";

        void show_window() {
            if (!g_window) {
                return;
            }
            SDL_ShowWindow(g_window);
            SDL_RaiseWindow(g_window);
        }

        void hide_window() {
            if (!g_window) {
                return;
            }
            SDL_HideWindow(g_window);
        }

        void window_cb(struct tray *tray) {
            (void) tray;
             // Tray icon click: bring the app window to front.
             show_window();
        }

        void tray_toggle_cb(struct tray_menu_item *item) {
            if (!g_window || !item) {
                return;
            }

            if (SDL_GetWindowFlags(g_window) & SDL_WINDOW_HIDDEN) {
                show_window();
                item->text = kHideText;
            } else {
                hide_window();
                item->text = kShowText;
            }

            struct tray *tray = tray_get_instance();
            if (tray != nullptr) {
                tray_update(tray);
            }
        }

        void tray_quit_cb(struct tray_menu_item *item) {
            (void) item;
            g_quitRequested.store(true);
        }

        struct tray_menu_item g_menu[4]{};
        struct tray g_tray{};

        void prepare_tray_structs() {
            g_menu[0].text = kHideText;
            g_menu[0].cb = tray_toggle_cb;
            g_menu[1].text = "-";
            g_menu[2].text = "Quit";
            g_menu[2].cb = tray_quit_cb;
            g_menu[3].text = nullptr;

            g_tray.icon_filepath = kTrayIcon;
            g_tray.tooltip = "WT Plotter";
            g_tray.cb = window_cb;
            g_tray.menu = g_menu;
        }
    }

    bool Init(SDL_Window *window) {
        g_window = window;
        prepare_tray_structs();

        if (tray_init(&g_tray) < 0) {
            // Fallback without custom icon path if assets/icon.ico is missing.
            g_tray.icon_filepath = nullptr;
            if (tray_init(&g_tray) < 0) {
                app_log::warn("Failed to initialize tray icon");
                return false;
            }
        }

        g_initialized = true;
        app_log::info("Tray initialized");
        return true;
    }

    void Poll() {
        if (!g_initialized) {
            return;
        }

        if (tray_loop(0) != 0) {
            g_quitRequested.store(true);
        }
    }

    void HandleWindowClose() {
        hide_window();
        g_menu[0].text = kShowText;

        struct tray *tray = tray_get_instance();
        if (tray != nullptr) {
            tray_update(tray);
        }
    }

    bool ConsumeQuitRequest() {
        return g_quitRequested.exchange(false);
    }

    void Shutdown() {
        if (!g_initialized) {
            return;
        }

        tray_exit();
        g_initialized = false;
        app_log::info("Tray shutdown");
    }
}

#else

namespace tray_support {
    bool init(SDL_Window *window) {
        (void) window;
        return false;
    }

    void poll() {}

    void handle_window_close() {}

    bool consume_quit_request() {
        return false;
    }

    void shutdown() {}
}

#endif

