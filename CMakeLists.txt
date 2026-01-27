cmake_minimum_required(VERSION 3.28)
project(helloworld_full_static)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==========================================
# 0. STATIC LINKING CONFIGURATION
# ==========================================
# Force everything to be static. No DLLs allowed.
set(CMAKE_EXE_LINKER_FLAGS "-static")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# ==========================================
# 1. FETCH & BUILD CURL (FROM SOURCE)
# ==========================================
message(STATUS "Fetching and Building Curl from source (this may take a minute)...")
include(FetchContent)

# Configure Curl to be lightweight and use Windows Native SSL (Schannel)
# This prevents needing libssl/libcrypto/zlib DLLs.
set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
set(CURL_USE_SCHANNEL ON CACHE BOOL "" FORCE) # Use Windows SSL
set(CURL_USE_OPENSSL OFF CACHE BOOL "" FORCE) # Disable OpenSSL
set(CURL_ZLIB OFF CACHE BOOL "" FORCE)        # Disable Zlib (simpler build)
set(CURL_DISABLE_LDAP ON CACHE BOOL "" FORCE) # Disable LDAP

FetchContent_Declare(
        curl
        GIT_REPOSITORY https://github.com/curl/curl.git
        GIT_TAG        curl-8_6_0 # Pin a stable version
)
FetchContent_MakeAvailable(curl)

# ==========================================
# 2. FETCH SDL2
# ==========================================
message(STATUS "Fetching SDL2...")
FetchContent_Declare(
        SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-2.28.5
)
FetchContent_MakeAvailable(SDL2)

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
        "${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp"        # REMOVED TRAILING SPACE
        "${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer2.cpp"
)
target_include_directories(imgui PUBLIC
        "${imgui_SOURCE_DIR}"
        "${imgui_SOURCE_DIR}/backends"
)
target_link_libraries(imgui PUBLIC SDL2::SDL2)

# NLOHMANN JSON
message(STATUS "Fetching nlohmann/json...")
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)
# When using wrpl maybe this is not needed?

# ==========================================
# 4. BUILD YOUR APP
# ==========================================
add_executable(hello_world_ main.cpp lib/sqlite3/sqlite3.c lib/sqlite3/sqlite3.h src/status_thread.cpp)

# Define CURL_STATICLIB so your code knows to link statically
target_compile_definitions(hello_world_ PRIVATE CURL_STATICLIB)

# Link against the 'libcurl_static' target we just built
target_link_libraries(hello_world_ PRIVATE
        imgui
        libcurl_static
        SDL2::SDL2main  # Add SDL2main here for WinMain
        nlohmann_json::nlohmann_json
)

set(DISCORD_SDK_ROOT "${CMAKE_SOURCE_DIR}/lib/discord_social_sdk")
set(DISCORD_SDK_LIB_DIR "${DISCORD_SDK_ROOT}/lib/release")
set(DISCORD_SDK_BIN_DIR "${DISCORD_SDK_ROOT}/bin/release")
set(DISCORD_SDK_INCLUDE_DIR "${DISCORD_SDK_ROOT}/include")

target_include_directories(hello_world_ PRIVATE ${DISCORD_SDK_INCLUDE_DIR})

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

if(UNIX)
    # Use RPATH when building
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    # Set the RPATH to use the lib directory relative to the executable
    set(CMAKE_INSTALL_RPATH "$ORIGIN")
    if(APPLE)
        set(CMAKE_INSTALL_RPATH "@executable_path")
    endif()
endif()

add_custom_command(TARGET hello_world_ POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${DISCORD_SHARED_LIB}"
        $<TARGET_FILE_DIR:hello_world_>;
)

# Link Windows Native Libraries required by Static Curl/SDL2
if(WIN32)
    target_link_libraries(hello_world_ PRIVATE ws2_32 crypt32 wldap32 normaliz)
endif()