cmake_minimum_required(VERSION 3.24)

project(wtplotter
    VERSION 1.2.1
    HOMEPAGE_URL github.com/Sgambe33/WT-Plotter
    DESCRIPTION "GUI tool to read War Thunder replays and plot ground matches"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake
)

include(Options)
include(Setup)
include(Deploy)

# Path to Discord Game SDK source directory
set(DISCORD_GAME_SDK ${CMAKE_SOURCE_DIR}/libs/discord-files)
if(NOT EXISTS "${DISCORD_GAME_SDK}/cpp/discord.h")
    message(STATUS "Downloading Discord Game SDK...")

    # Create directory if missing
    file(MAKE_DIRECTORY ${DISCORD_GAME_SDK})

    set(DISCORD_GAME_SDK_ZIP ${DISCORD_GAME_SDK}/discord_game_sdk.zip)
    set(DISCORD_GAME_SDK_URL "https://dl-game-sdk.discordapp.net/2.5.6/discord_game_sdk.zip")

    file(DOWNLOAD ${DISCORD_GAME_SDK_URL} ${DISCORD_GAME_SDK_ZIP} SHOW_PROGRESS)
    file(ARCHIVE_EXTRACT INPUT ${DISCORD_GAME_SDK_ZIP} DESTINATION ${DISCORD_GAME_SDK})

    message(STATUS "Patching Discord SDK headers for Linux compatibility...")
    file(GLOB DISCORD_CPP_HEADERS "${DISCORD_GAME_SDK}/cpp/*.h")
    foreach(header_file ${DISCORD_CPP_HEADERS})
        file(READ "${header_file}" FILE_CONTENT)
        if(NOT "${FILE_CONTENT}" MATCHES "#include <cstdint>")
            # Prepend the missing include
            file(WRITE "${header_file}" "#include <cstdint>\n#include <memory>\n${FILE_CONTENT}")
        endif()
    endforeach()

    if(UNIX AND NOT APPLE)
        find_program(PATCHELF_EXE patchelf)
        set(DISCORD_SO "${DISCORD_GAME_SDK}/lib/x86_64/discord_game_sdk.so")

        if(PATCHELF_EXE AND EXISTS "${DISCORD_SO}")
            message(STATUS "Patching SONAME for ${DISCORD_SO}...")
            execute_process(COMMAND ${PATCHELF_EXE} --set-soname "discord_game_sdk.so" "${DISCORD_SO}")
        else()
            message(WARNING " 'patchelf' not found! The resulting AppImage might crash because Discord SDK lacks a SONAME.")
        endif()
    endif()

    message(STATUS "Discord Game SDK downloaded and patched.")
else()
    message(STATUS "Discord Game SDK found at ${DISCORD_GAME_SDK}")
endif()

# Locate Qt
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Svg Network Sql)
find_package(Qt${QT_VERSION_MAJOR} OPTIONAL_COMPONENTS LinguistTools)

# Collect sources and UI
file(GLOB_RECURSE SOURCES
        src/classes/*.cpp src/classes/*.h
        libs/discord-files/cpp/*.cpp libs/discord-files/cpp/*.h
)

set(UI_FILES
        src/ui/mainwindow.ui
        src/ui/preferencesdialog.ui
        src/ui/playerprofiledialog.ui
)

qt_add_executable(wtplotter
        src/main.cpp
        src/ui/mainwindow.cpp
        src/ui/mainwindow.h
        src/ui/preferencesdialog.cpp
        src/ui/preferencesdialog.h
        src/ui/playerprofiledialog.cpp
        src/ui/playerprofiledialog.h
        src/ui/sceneimageviewer.cpp
        src/ui/sceneimageviewer.h
        src/worker.cpp
        src/worker.h
    ${SOURCES}
    ${UI_FILES}
)

# Resources
file(GLOB_RECURSE ICONS         CONFIGURE_DEPENDS resources/icons/*)
file(GLOB_RECURSE MAP_IMAGES    CONFIGURE_DEPENDS resources/map_images/*)
file(GLOB_RECURSE JSON_FILES    CONFIGURE_DEPENDS resources/translations/*.json)
file(GLOB_RECURSE TRANSLATIONS  CONFIGURE_DEPENDS resources/translations/*.ts)
file(GLOB_RECURSE FONTS         CONFIGURE_DEPENDS resources/fonts/*.ttf)

qt_add_resources(wtplotter
    resources
    PREFIX /
    BASE resources
    FILES ${ICONS} ${MAP_IMAGES} ${JSON_FILES} ${FONTS}
)

app_add_translations(wtplotter TS_FILES ${TRANSLATIONS})

target_include_directories(wtplotter PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src/ui ${DISCORD_GAME_SDK}/cpp
)

# Platform-specific linking and deployment
if(WIN32)
    set(DISCORD_LIB ${DISCORD_GAME_SDK}/lib/x86_64/discord_game_sdk.dll.lib)
    target_link_libraries(wtplotter PRIVATE ${DISCORD_LIB})

    # Copy DLL to binary dir
    add_custom_command(TARGET wtplotter POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${DISCORD_GAME_SDK}/lib/x86_64/discord_game_sdk.dll
        $<TARGET_FILE_DIR:wtplotter>
    )
elseif(APPLE)
    set(DISCORD_LIB ${DISCORD_GAME_SDK}/lib/x86_64/discord_game_sdk.dylib)
    target_link_libraries(wtplotter PRIVATE ${DISCORD_LIB})

    # Ensure the .dylib is bundled with app
    add_custom_command(TARGET wtplotter POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${DISCORD_LIB}
        $<TARGET_FILE_DIR:wtplotter>
    )
elseif(UNIX)
    # Set the base dir for the SDK (relative to project root, adjust if necessary)
    set(DISCORD_GAME_SDK_DIR "${DISCORD_GAME_SDK}/lib/x86_64")
    add_library(discord_game_sdk SHARED IMPORTED)
    set_target_properties(discord_game_sdk PROPERTIES
        IMPORTED_LOCATION "${DISCORD_GAME_SDK_DIR}/discord_game_sdk.so"
        IMPORTED_SONAME "discord_game_sdk.so"
    )
    # Add the directory to the linker search path
    link_directories("${DISCORD_GAME_SDK_DIR}")

    target_link_libraries(wtplotter PRIVATE discord_game_sdk)

    set_target_properties(wtplotter PROPERTIES
        INSTALL_RPATH "$ORIGIN/../lib"
        BUILD_WITH_INSTALL_RPATH TRUE
        SKIP_BUILD_RPATH FALSE
        BUILD_RPATH_USE_ORIGIN TRUE
    )
endif()

# Qt core libraries
target_link_libraries(wtplotter PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::Sql
)

# Debug flag
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG_BUILD)
endif()

# Version header
configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in
        ${CMAKE_CURRENT_SOURCE_DIR}/src/version.h
    @ONLY
)

deploy(wtplotter deploy)
