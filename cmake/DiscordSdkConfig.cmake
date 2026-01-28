# DiscordSdkConfig.cmake
# Fetch the Discord Social SDK and expose variables for the main CMakeLists
message(STATUS "Fetching Discord Social SDK...")
include(FetchContent)

FetchContent_Declare(
    discord_social_sdk
    URL https://storage.googleapis.com/discord-slayer-sdk-artifacts/discord_partner_sdk/3b8f3adce7dd1d85463aa700d9185676633e98a1/release/DiscordSocialSdk-1.8.13395.zip?X-Goog-Algorithm=GOOG4-RSA-SHA256&X-Goog-Credential=slayer-sdk-token-creator%40discord-production.iam.gserviceaccount.com%2F20260128%2Fauto%2Fstorage%2Fgoog4_request&X-Goog-Date=20260128T221022Z&X-Goog-Expires=3600&X-Goog-SignedHeaders=host&X-Goog-Signature=6d26415f5df1c9af7717f22c1e854fc82bfe88db7f41150a56ce4676b75e2967fd9254ad0843f5e687e3eb5a29019076b6699972fbb73742a385bcc08817081493b49e9c0d437ddfc6a86587749315b376226a8735d875c36a257bd87f101bf636ed4595a45a381bf12abfb3d86f10e78a7f52572006bdbc87c659d2817d5dfb31035b9a421865904d98dcb7efda1fbe18a324a2935603af4cc2701fe7910f6d9169b82004e8aff6a333ff17014e66531e0adf65bfd49b8381958f747a273bd9a071c51d1dd05a5656210dfae3f1ee0c1c24a8f83266455a0b96b746820c3b5ca81fe3963dd5f64579eb6d8787590f75502169719b359063e1e5f99a19cd48ac
)
FetchContent_MakeAvailable(discord_social_sdk)

set(DISCORD_SDK_ROOT "${discord_social_sdk_SOURCE_DIR}")
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
