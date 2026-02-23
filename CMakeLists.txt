cmake_minimum_required(VERSION 3.24)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

project(wtplotter
        VERSION 1.1.1
        HOMEPAGE_URL github.com/Sgambe33/WT-Plotter
        DESCRIPTION "GUI tool to read War Thunder replays and plot ground matches"
        LANGUAGES CXX
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}") #Ensures LIBWRPL dll is in the same directory as the executable

list(APPEND CMAKE_MODULE_PATH
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake
)

include(Options)
include(Setup)
include(Deploy)


############ ZSTD SETUP ############
add_library(zstd STATIC IMPORTED)
set_target_properties(zstd PROPERTIES
        IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/libs/zstd/lib/libzstd.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/libs/zstd/include"
)

# Locate Qt
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Svg Network Sql)
find_package(Qt${QT_VERSION_MAJOR} OPTIONAL_COMPONENTS LinguistTools)

# Collect sources and UI
file(GLOB_RECURSE SOURCES
        src/classes/*.cpp src/classes/*.h
        libs/zstd/include/*.h
)

set(UI_FILES
        src/ui/mainwindow.ui
        src/ui/preferencesdialog.ui
        src/ui/playerprofiledialog.ui
        src/ui/Structs.h
)

add_subdirectory(libs/WRPL_parser)

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
        src/GameTelemetryWorker.cpp
        src/GameTelemetryWorker.h
        ${SOURCES}
        ${UI_FILES}
)

# Resources
file(GLOB_RECURSE ICONS CONFIGURE_DEPENDS resources/icons/*)
file(GLOB_RECURSE MAP_IMAGES CONFIGURE_DEPENDS resources/map_images/*)
file(GLOB_RECURSE JSON_FILES CONFIGURE_DEPENDS resources/translations/*.json)
file(GLOB_RECURSE TRANSLATIONS CONFIGURE_DEPENDS resources/translations/*.ts)
file(GLOB_RECURSE FONTS CONFIGURE_DEPENDS resources/fonts/*.ttf)

qt_add_resources(wtplotter
        resources
        PREFIX /
        BASE resources
        FILES ${ICONS} ${MAP_IMAGES} ${JSON_FILES} ${FONTS}
)

app_add_translations(wtplotter TS_FILES ${TRANSLATIONS})

target_include_directories(wtplotter PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src/ui
)

# Platform-specific linking and deployment
if (WIN32)
    set(ZSTD_LIB ${CMAKE_SOURCE_DIR}/libs/zstd/lib/libzstd_static.lib) #TODO: Perhaps dynamic library instead?
    target_link_libraries(wtplotter PRIVATE ${ZSTD_LIB})
endif ()

# Qt core libraries
target_link_libraries(wtplotter PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Network
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Sql
        WRPL::wrpl
)

# Debug flag
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG_BUILD)
endif ()

# Version header
configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/version.h"
        @ONLY
)

# DISCORD SDK SETUP
set(DISCORD_SDK_ROOT "${CMAKE_SOURCE_DIR}/libs/discord_social_sdk")
set(DISCORD_SDK_LIB_DIR "${DISCORD_SDK_ROOT}/lib/release")
set(DISCORD_SDK_BIN_DIR "${DISCORD_SDK_ROOT}/bin/release")
set(DISCORD_SDK_INCLUDE_DIR "${DISCORD_SDK_ROOT}/include")

target_include_directories(wtplotter PRIVATE ${DISCORD_SDK_INCLUDE_DIR})

if (WIN32)
    set(DISCORD_LIB_PATH "${DISCORD_SDK_LIB_DIR}/discord_partner_sdk.lib")
    set(DISCORD_SHARED_LIB "${DISCORD_SDK_BIN_DIR}/discord_partner_sdk.dll")
elseif (APPLE)
    set(DISCORD_LIB_PATH "${DISCORD_SDK_LIB_DIR}/libdiscord_partner_sdk.dylib")
    set(DISCORD_SHARED_LIB "${DISCORD_SDK_LIB_DIR}/libdiscord_partner_sdk.dylib")
else () # Linux
    set(DISCORD_LIB_PATH "${DISCORD_SDK_LIB_DIR}/libdiscord_partner_sdk.so")
    set(DISCORD_SHARED_LIB "${DISCORD_SDK_LIB_DIR}/libdiscord_partner_sdk.so")
endif ()

target_link_libraries(wtplotter PRIVATE ${DISCORD_LIB_PATH})

if (UNIX)
    # Use RPATH when building
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    # Set the RPATH to use the lib directory relative to the executable
    set(CMAKE_INSTALL_RPATH "$ORIGIN")
    if (APPLE)
        set(CMAKE_INSTALL_RPATH "@executable_path")
    endif ()
endif ()

add_custom_command(TARGET wtplotter POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${DISCORD_SHARED_LIB}"
        $<TARGET_FILE_DIR:wtplotter>;
)

deploy(wtplotter deploy)
