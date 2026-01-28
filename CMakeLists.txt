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

# Add cmake modules directory
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Include modular configs (these files are in cmake/)
include("${CMAKE_SOURCE_DIR}/cmake/CurlConfig.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/SDLConfig.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/ImGuiConfig.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/DiscordSdkConfig.cmake")

# ==========================================
# NLOHMANN JSON
# ==========================================
message(STATUS "Fetching nlohmann/json...")
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)

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

# Discord include comes from the Discord module
target_include_directories(hello_world_ PRIVATE ${DISCORD_SDK_INCLUDE_DIR})

# Link against the static libraries
target_link_libraries(hello_world_ PRIVATE
        imgui
        libcurl_static
        SDL3::SDL3
        SDL3_image::SDL3_image
        nlohmann_json::nlohmann_json
)

# Discord SDK linking (variables provided by Discord module)
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

# Use the path exported by the Discord module if available
if(DEFINED DISCORD_LIB_PATH)
    target_link_libraries(hello_world_ PRIVATE ${DISCORD_LIB_PATH})
endif()

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

# Copy Discord DLL/SO to output directory (if variable available)
if(DEFINED DISCORD_SHARED_LIB)
add_custom_command(TARGET hello_world_ POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${DISCORD_SHARED_LIB}"
        $<TARGET_FILE_DIR:hello_world_>
)
endif()

# Link Windows Native Libraries required by Static Curl/SDL2
if(WIN32)
    target_link_libraries(hello_world_ PRIVATE ws2_32 crypt32 wldap32 normaliz)
endif()

# Link required libraries for Unix systems
if(UNIX AND NOT APPLE)
    target_link_libraries(hello_world_ PRIVATE pthread dl m)
endif()