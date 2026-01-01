cmake_minimum_required(VERSION 3.24)

project(wtplotter
    VERSION 1.2.0
    HOMEPAGE_URL github.com/Sgambe33/WT-Plotter
    DESCRIPTION "GUI tool to read War Thunder replays and plot ground matches"
    LANGUAGES CXX
)

list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake
)

include(Options)
include(Setup)
include(Deploy)

# Path to Discord Game SDK source directory
set(DISCORD_GAME_SDK ${CMAKE_SOURCE_DIR}/discord-files)

# Locate Qt
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Svg Network Sql)
find_package(Qt${QT_VERSION_MAJOR} OPTIONAL_COMPONENTS LinguistTools)

# Collect sources and UI
file(GLOB_RECURSE SOURCES
    classes/*.cpp classes/*.h
    discord-files/cpp/*.cpp discord-files/cpp/*.h
)

set(UI_FILES
    mainwindow.ui
    preferencesdialog.ui
    playerprofiledialog.ui
)

qt_add_executable(wtplotter
    main.cpp
    mainwindow.cpp
    mainwindow.h
    preferencesdialog.cpp
    preferencesdialog.h
    playerprofiledialog.cpp
    playerprofiledialog.h
    sceneimageviewer.cpp
    sceneimageviewer.h
    worker.cpp
    worker.h
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
    ${CMAKE_CURRENT_SOURCE_DIR}
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
    set(DISCORD_GAME_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/discord-files/lib/x86_64")
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
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h
    @ONLY
)

deploy(wtplotter deploy)
