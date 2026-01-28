cmake_minimum_required(VERSION 3.28)
project(helloworld_full_static)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==========================================
# 0. STATIC LINKING CONFIGURATION
# ==========================================
# Force everything to be static. No DLLs allowed.
if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "-static")
endif()
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# ==========================================
# 1. FETCH & BUILD CURL (FROM SOURCE)
# ==========================================
message(STATUS "Fetching and Building Curl from source (this may take a minute)...")
include(FetchContent)

# Configure Curl to be lightweight and use Windows Native SSL (Schannel)
# This prevents needing libssl/libcrypto/zlib DLLs.
set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
set(CURL_ZLIB OFF CACHE BOOL "" FORCE)
set(CURL_DISABLE_LDAP ON CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

if(WIN32)
    message(STATUS "Configuring curl with Schannel (Windows)")
    set(CURL_USE_SCHANNEL ON CACHE BOOL "" FORCE)
    set(CURL_USE_OPENSSL OFF CACHE BOOL "" FORCE)
    set(CURL_USE_GNUTLS OFF CACHE BOOL "" FORCE)
else()
    message(STATUS "Configuring curl with OpenSSL (Linux/macOS)")
    set(CURL_USE_SCHANNEL OFF CACHE BOOL "" FORCE)
    set(CURL_USE_OPENSSL ON CACHE BOOL "" FORCE)
    set(CURL_USE_GNUTLS OFF CACHE BOOL "" FORCE)
endif()

FetchContent_Declare(
        curl
        GIT_REPOSITORY https://github.com/curl/curl.git
        GIT_TAG        curl-8_6_0 # Pin a stable version
)
FetchContent_MakeAvailable(curl)

# ==========================================
# 2. FETCH SDL3
# ==========================================
# Disable problematic features
set(SDL_PIPEWIRE OFF CACHE BOOL "" FORCE)
set(SDL_PIPEWIRE_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_PULSEAUDIO ON CACHE BOOL "" FORCE)
set(SDL_PULSEAUDIO_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_ALSA ON CACHE BOOL "" FORCE)

# X11 configuration - enable X11 but disable problematic extensions
if(UNIX AND NOT APPLE)
    set(SDL_VIDEO_X11 ON CACHE BOOL "" FORCE)
    set(SDL_X11_XINPUT2 OFF CACHE BOOL "" FORCE)
    set(SDL_X11_XFIXES OFF CACHE BOOL "" FORCE)
    set(SDL_X11_XRANDR OFF CACHE BOOL "" FORCE)
    set(SDL_X11_XSCRNSAVER OFF CACHE BOOL "" FORCE)
    set(SDL_X11_XSHAPE OFF CACHE BOOL "" FORCE)
endif()

set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)

message(STATUS "Fetching SDL3...")
FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.2.0  # Use a stable release
)
FetchContent_MakeAvailable(SDL3)

# ==========================================
# SDL3_image
# ==========================================
set(SDL3IMAGE_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL3IMAGE_DEPS_SHARED OFF CACHE BOOL "" FORCE)
set(SDL3IMAGE_VENDORED ON CACHE BOOL "" FORCE)
set(SDL3IMAGE_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

message(STATUS "Fetching SDL3_image...")
FetchContent_Declare(
        SDL3_image
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
        GIT_TAG release-3.2.0
)
FetchContent_MakeAvailable(SDL3_image)

# ==========================================
# 3. FETCH STOCK IMGUI
# ==========================================
message(STATUS "Fetching ImGui...")
FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        docking
)
FetchContent_MakeAvailable(imgui)

# Create the ImGui library target
add_library(imgui STATIC
        "${imgui_SOURCE_DIR}/imgui.cpp"
        "${imgui_SOURCE_DIR}/imgui_draw.cpp"
        "${imgui_SOURCE_DIR}/imgui_tables.cpp"
        "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
        "${imgui_SOURCE_DIR}/imgui_demo.cpp"
        "${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp"
        "${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp"
)
target_include_directories(imgui PUBLIC
        "${imgui_SOURCE_DIR}"
        "${imgui_SOURCE_DIR}/backends"
)
target_link_libraries(imgui PUBLIC SDL3::SDL3)

# ==========================================
# NLOHMANN JSON
# ==========================================
message(STATUS "Fetching nlohmann/json...")
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)

# ==========================================
# DISCORD SDK
# ==========================================
message(STATUS "Fetching Discord Social SDK...")
FetchContent_Declare(
        discord_social_sdk
        URL https://storage.googleapis.com/discord-slayer-sdk-artifacts/discord_partner_sdk/3b8f3adce7dd1d85463aa700d9185676633e98a1/release/DiscordSocialSdk-1.8.13395.zip?X-Goog-Algorithm=GOOG4-RSA-SHA256&X-Goog-Credential=slayer-sdk-token-creator%40discord-production.iam.gserviceaccount.com%2F20260127%2Fauto%2Fstorage%2Fgoog4_request&X-Goog-Date=20260127T204757Z&X-Goog-Expires=3600&X-Goog-SignedHeaders=host&X-Goog-Signature=0a82600b73964d4544d2b4a1217f8d07fed4d59b92bed96c7adced254db4f341bda048208151eed04d6fdc0c71fe408dc674ea958ec25d866097f501a2376464de45f74c397bcbc4101f19417728ac71fadaa7499d8803184a1d47adb7e2eb96a2a377e60cb73efa3a4c5d7b46010844fdf1f8c0c11b55e8ddb6267befb3941b940c3ed0a48d5f04b9d62f1f942771edb3e586220d9966997059845e82c5d99ab88b3f5d74c75986f25329dae9e8d9ad5b64a142519c7376e36500702473495073020d79c34af9b2e4a86177288434f032979d88cd3643ccb31d8b1f04e2db4b79464ba960b86ffdabfa941a97738bbb3c4b8954f6b1336da2ee568d9e29a7b9
)
FetchContent_MakeAvailable(discord_social_sdk)

set(DISCORD_SDK_ROOT "${discord_social_sdk_SOURCE_DIR}")
set(DISCORD_SDK_LIB_DIR "${DISCORD_SDK_ROOT}/lib/release")
set(DISCORD_SDK_BIN_DIR "${DISCORD_SDK_ROOT}/bin/release")
set(DISCORD_SDK_INCLUDE_DIR "${DISCORD_SDK_ROOT}/include")

# =========================================
# INCLUDE WRPL LIBRARY
# =========================================
add_subdirectory(lib/wrpl_parser)

# ==========================================
# 4. BUILD YOUR APP
# ==========================================
add_executable(hello_world_
    main.cpp
    lib/sqlite3/sqlite3.c
    lib/sqlite3/sqlite3.h
    src/status_thread.cpp
    src/status_thread.h
    src/app_state.cpp
    src/app_state.h
    src/preferences.cpp
    src/preferences.h
    src/replay_manager.cpp
    src/replay_manager.h
    src/rpc_manager.cpp
    src/rpc_manager.h
    src/gui_windows.cpp
    src/gui_windows.h
    src/constants.h
)

# Define CURL_STATICLIB so your code knows to link statically
target_compile_definitions(hello_world_ PRIVATE CURL_STATICLIB)

target_include_directories(hello_world_ PRIVATE ${DISCORD_SDK_INCLUDE_DIR})

# Link against the static libraries
target_link_libraries(hello_world_ PRIVATE
        imgui
        libcurl_static
        SDL3::SDL3
        SDL3_image::SDL3_image
        nlohmann_json::nlohmann_json
)

# Discord SDK linking
if(WIN32)
    set(DISCORD_LIB_PATH "${DISCORD_SDK_LIB_DIR}/discord_partner_sdk.lib")
    set(DISCORD_SHARED_LIB "${DISCORD_SDK_BIN_DIR}/discord_partner_sdk.dll")
elseif(APPLE)
    set(DISCORD_LIB_PATH "${DISCORD_SDK_LIB_DIR}/libdiscord_partner_sdk.dylib")
    set(DISCORD_SHARED_LIB "${DISCORD_SDK_LIB_DIR}/libdiscord_partner_sdk.dylib")
else() # Linux
    set(DISCORD_LIB_PATH "${DISCORD_SDK_LIB_DIR}/libdiscord_partner_sdk.so")
    set(DISCORD_SHARED_LIB "${DISCORD_SDK_LIB_DIR}/libdiscord_partner_sdk.so")
endif()

target_link_libraries(hello_world_ PRIVATE ${DISCORD_LIB_PATH})

# Set RPATH for Unix systems
if(UNIX)
    set_target_properties(hello_world_ PROPERTIES
            BUILD_WITH_INSTALL_RPATH TRUE
            INSTALL_RPATH "$ORIGIN"
    )
    if(APPLE)
        set_target_properties(hello_world_ PROPERTIES
                INSTALL_RPATH "@executable_path"
        )
    endif()
endif()

# Copy Discord DLL/SO to output directory
add_custom_command(TARGET hello_world_ POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${DISCORD_SHARED_LIB}"
        $<TARGET_FILE_DIR:hello_world_>
)

# Link Windows Native Libraries required by Static Curl/SDL2
if(WIN32)
    target_link_libraries(hello_world_ PRIVATE ws2_32 crypt32 wldap32 normaliz)
endif()

# Link required libraries for Unix systems
if(UNIX AND NOT APPLE)
    target_link_libraries(hello_world_ PRIVATE pthread dl m)
endif()