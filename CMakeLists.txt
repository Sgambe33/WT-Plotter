cmake_minimum_required(VERSION 3.24)

project(wtplotter
    VERSION 1.1.1    
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

set(DISCORD_GAME_SDK ${CMAKE_SOURCE_DIR}/discord-files)
if(NOT EXISTS ${DISCORD_GAME_SDK})
    message(STATUS "Downloading Discord Game SDK...")
    set(DISCORD_GAME_SDK_ZIP ${CMAKE_SOURCE_DIR}/discord-files/discord_game_sdk.zip)
    set(DISCORD_GAME_SDK_URL "https://dl-game-sdk.discordapp.net/2.5.6/discord_game_sdk.zip")
    file(DOWNLOAD ${DISCORD_GAME_SDK_URL} ${DISCORD_GAME_SDK_ZIP} SHOW_PROGRESS)
    file(ARCHIVE_EXTRACT INPUT ${DISCORD_GAME_SDK_ZIP} DESTINATION ${DISCORD_GAME_SDK})
    message(STATUS "Discord Game SDK downloaded to ${DISCORD_GAME_SDK}")
else()
    message(STATUS "Discord Game SDK already exists at ${DISCORD_GAME_SDK}")
endif()

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Svg Network Sql)
find_package(Qt${QT_VERSION_MAJOR} OPTIONAL_COMPONENTS LinguistTools)

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
    SceneImageViewer.cpp
    sceneimageviewer.h
    worker.cpp
    worker.h
    ${SOURCES}
    ${UI_FILES}
)

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

# Platform-specific linking
if(WIN32)
    set(DISCORD_LIB ${CMAKE_SOURCE_DIR}/discord-files/lib/x86_64/discord_game_sdk.dll.lib)
    target_link_libraries(wtplotter PRIVATE ${DISCORD_LIB})
    
    # Copy DLL to binary dir
    add_custom_command(TARGET wtplotter POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/discord-files/lib/x86_64/discord_game_sdk.dll
        $<TARGET_FILE_DIR:wtplotter>
    )
elseif(APPLE)
    set(DISCORD_LIB ${CMAKE_SOURCE_DIR}/discord-files/lib/x86_64/libdiscord_game_sdk.dylib)
    target_link_libraries(wtplotter PRIVATE ${DISCORD_LIB})
elseif(UNIX)
    set(DISCORD_LIB ${CMAKE_SOURCE_DIR}/discord-files/lib/x86_64/libdiscord_game_sdk.so)
    target_link_libraries(wtplotter PRIVATE ${DISCORD_LIB})
endif()

target_link_libraries(wtplotter PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Svg Qt${QT_VERSION_MAJOR}::Sql
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG_BUILD)
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h
    @ONLY
)

deploy(wtplotter deploy)