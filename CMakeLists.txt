cmake_minimum_required(VERSION 3.24)

list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake
)

project(wtplotter
    LANGUAGES CXX
    VERSION 0.2.4
    HOMEPAGE_URL github.com/Sgambe33/WT-Plotter
    DESCRIPTION "GUI tool to read War Thunder replays and plot ground matches"
)

include(Options)
include(Setup)
include(Deploy)

find_package(Qt${QT_VERSION_MAJOR}
    REQUIRED
        Widgets
        Svg
        Network
    OPTIONAL_COMPONENTS
        LinguistTools
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

qt_add_executable(wtplotter
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    preferencesdialog.cpp
    preferencesdialog.h
    preferencesdialog.ui
    SceneImageViewer.cpp
    sceneimageviewer.h
    worker.cpp
    worker.h
    classes/craftinfo.cpp
    classes/craftinfo.h
    classes/playerinfo.cpp
    classes/playerinfo.h
    classes/player.cpp
    classes/player.h
    classes/replay.cpp
    classes/replay.h
    classes/position.cpp
    classes/position.h
)

file(GLOB_RECURSE ICONS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    CONFIGURE_DEPENDS resources/icons/*
)

file(GLOB_RECURSE MAP_IMAGES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    CONFIGURE_DEPENDS resources/map_images/*
)

file(GLOB TRANSLATIONS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    CONFIGURE_DEPENDS resources/translations/*.ts
)

qt_add_resources(wtplotter
    resources
    PREFIX /
    BASE resources
    FILES ${ICONS} ${MAP_IMAGES}
)

app_add_translations(wtplotter TS_FILES ${TRANSLATIONS})

target_link_libraries(wtplotter PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h
    @ONLY
)

deploy(wtplotter deploy)