set(DISCORD_GAMESDK_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(DISCORD_GAMESDK_SOURCE_DIR "${DISCORD_GAMESDK_ROOT}/src")
set(DISCORD_GAMESDK_LIB_DIR "${DISCORD_GAMESDK_ROOT}/lib")
set(DISCORD_GAMESDK_LIB_IMP "${DISCORD_GAMESDK_LIB_DIR}/discord_game_sdk.dll.lib")
set(DISCORD_GAMESDK_LIB_BIN "${DISCORD_GAMESDK_LIB_DIR}/discord_game_sdk.dll")
set(DISCORD_GAMESDK_INCLUDE_PATH "${DISCORD_GAMESDK_ROOT}/src")
set(DISCORD_GAMESDK_LINK_PATH "${DISCORD_GAMESDK_ROOT}/lib")

if(UNIX)
    set(DISCORD_GAMESDK_LIB_BIN "${DISCORD_GAMESDK_LIB_DIR}/discord_game_sdk.so")
endif()

set(DISCORD_GAMESDK_SOURCES
    "${DISCORD_GAMESDK_SOURCE_DIR}/achievement_manager.h"
    "${DISCORD_GAMESDK_SOURCE_DIR}/achievement_manager.cpp"
    "${DISCORD_GAMESDK_SOURCE_DIR}/activity_manager.h"
    "${DISCORD_GAMESDK_SOURCE_DIR}/activity_manager.cpp"
    "${DISCORD_GAMESDK_SOURCE_DIR}/application_manager.h"
    "${DISCORD_GAMESDK_SOURCE_DIR}/application_manager.cpp"
    "${DISCORD_GAMESDK_SOURCE_DIR}/core.h" 
    "${DISCORD_GAMESDK_SOURCE_DIR}/core.cpp"
    "${DISCORD_GAMESDK_SOURCE_DIR}/discord.h" 
    "${DISCORD_GAMESDK_SOURCE_DIR}/event.h"
    "${DISCORD_GAMESDK_SOURCE_DIR}/ffi.h"
    "${DISCORD_GAMESDK_SOURCE_DIR}/image_manager.h"
    "${DISCORD_GAMESDK_SOURCE_DIR}/image_manager.cpp"
    "${DISCORD_GAMESDK_SOURCE_DIR}/lobby_manager.h"
    "${DISCORD_GAMESDK_SOURCE_DIR}/lobby_manager.cpp"
    "${DISCORD_GAMESDK_SOURCE_DIR}/network_manager.h"
    "${DISCORD_GAMESDK_SOURCE_DIR}/network_manager.cpp"
    "${DISCORD_GAMESDK_SOURCE_DIR}/overlay_manager.h"
    "${DISCORD_GAMESDK_SOURCE_DIR}/overlay_manager.cpp"
    "${DISCORD_GAMESDK_SOURCE_DIR}/relationship_manager.h"
    "${DISCORD_GAMESDK_SOURCE_DIR}/relationship_manager.cpp"
    "${DISCORD_GAMESDK_SOURCE_DIR}/storage_manager.h"
    "${DISCORD_GAMESDK_SOURCE_DIR}/storage_manager.cpp"
    "${DISCORD_GAMESDK_SOURCE_DIR}/store_manager.h"
    "${DISCORD_GAMESDK_SOURCE_DIR}/store_manager.cpp"
    "${DISCORD_GAMESDK_SOURCE_DIR}/types.h" 
    "${DISCORD_GAMESDK_SOURCE_DIR}/types.cpp"
    "${DISCORD_GAMESDK_SOURCE_DIR}/user_manager.h"
    "${DISCORD_GAMESDK_SOURCE_DIR}/user_manager.cpp"
    "${DISCORD_GAMESDK_SOURCE_DIR}/voice_manager.h"
    "${DISCORD_GAMESDK_SOURCE_DIR}/voice_manager.cpp"
)

# Imported target configuration
add_library(discord-import SHARED IMPORTED)

set_target_properties(discord-import PROPERTIES
    IMPORTED_CONFIGURATIONS "Release;Debug;RelWithDebInfo;MinSizeRel"
    IMPORTED_LOCATION "${DISCORD_GAMESDK_LIB_BIN}"
)

if(WIN32)
    set_target_properties(discord-import PROPERTIES
        IMPORTED_IMPLIB "${DISCORD_GAMESDK_LIB_IMP}"
    )
endif()

# Main library target
add_library(discord_gamesdk SHARED ${DISCORD_GAMESDK_SOURCES})

# Target properties
set_target_properties(discord_gamesdk PROPERTIES
    DEBUG_POSTFIX ""
    OUTPUT_NAME "discord_gamesdk"
    BUILD_WITH_INSTALL_RPATH FALSE
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

if(UNIX)
    set_target_properties(discord_gamesdk PROPERTIES
        LINK_FLAGS "-Wl,-rpath=$ORIGIN"
    )
endif()

# Include directories
target_include_directories(discord_gamesdk PUBLIC
    ${DISCORD_GAMESDK_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(discord_gamesdk PRIVATE
    discord-import
)

# Auto-copy DLL to build directory
add_custom_command(TARGET discord_gamesdk POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${DISCORD_GAMESDK_LIB_BIN}"
    "$<TARGET_FILE_DIR:discord_gamesdk>"
    COMMENT "Copying Discord SDK DLL to output directory"
)

# Visual Studio specific configurations
if(MSVC)
    # Set debugger working directory to output path
    set_target_properties(discord_gamesdk PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:discord_gamesdk>"
    )

    # Handle all configuration types
    foreach(CFG_TYPE Debug Release RelWithDebInfo MinSizeRel)
        string(TOUPPER ${CFG_TYPE} CFG_TYPE_UPPER)
        set_target_properties(discord_gamesdk PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_${CFG_TYPE_UPPER} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
            LIBRARY_OUTPUT_DIRECTORY_${CFG_TYPE_UPPER} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        )
    endforeach()
endif()