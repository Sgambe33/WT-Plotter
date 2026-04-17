# DiscordSdkConfig.cmake
# Load the Discord Social SDK from the local libs folder
message(STATUS "Using local Discord Social SDK...")

# Expected local SDK path (can be overridden from CMake cache)
set(DISCORD_SDK_ROOT "${CMAKE_SOURCE_DIR}/libs/discord_social_sdk" CACHE PATH "Path to local Discord Social SDK root")

if(NOT EXISTS "${DISCORD_SDK_ROOT}")
    message(FATAL_ERROR
        "Discord Social SDK not found at: ${DISCORD_SDK_ROOT}\n"
        "Place the SDK in libs/DiscordSocialSdk or set DISCORD_SDK_ROOT manually."
    )
endif()

set(DISCORD_SDK_LIB_DIR "${DISCORD_SDK_ROOT}/lib/release")
set(DISCORD_SDK_BIN_DIR "${DISCORD_SDK_ROOT}/bin/release")
set(DISCORD_SDK_INCLUDE_DIR "${DISCORD_SDK_ROOT}/include")

if(WIN32)
    set(DISCORD_LIB_PATH "${DISCORD_SDK_LIB_DIR}/discord_partner_sdk.lib")
    set(DISCORD_SHARED_LIB "${DISCORD_SDK_BIN_DIR}/discord_partner_sdk.dll")
elseif(APPLE)
    set(DISCORD_LIB_PATH "${DISCORD_SDK_LIB_DIR}/libdiscord_partner_sdk.dylib")
    set(DISCORD_SHARED_LIB "${DISCORD_SDK_LIB_DIR}/libdiscord_partner_sdk.dylib")
else()
    set(DISCORD_LIB_PATH "${DISCORD_SDK_LIB_DIR}/libdiscord_partner_sdk.so")
    set(DISCORD_SHARED_LIB "${DISCORD_SDK_LIB_DIR}/libdiscord_partner_sdk.so")
endif()

# Export variables for the main CMakeLists
set(DISCORD_SDK_INCLUDE_DIR ${DISCORD_SDK_INCLUDE_DIR} PARENT_SCOPE)
set(DISCORD_LIB_PATH ${DISCORD_LIB_PATH} PARENT_SCOPE)
set(DISCORD_SHARED_LIB ${DISCORD_SHARED_LIB} PARENT_SCOPE)
set(DISCORD_SDK_ROOT ${DISCORD_SDK_ROOT} PARENT_SCOPE)
