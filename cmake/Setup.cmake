include_guard(GLOBAL)

if(APPLE)
    enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS FALSE)
set(CMAKE_AUTOMOC TRUE)
set(CMAKE_AUTOUIC TRUE)
set(CMAKE_AUTORCC TRUE)
set(CMAKE_WIN32_EXECUTABLE TRUE)
set(CMAKE_MACOSX_BUNDLE TRUE)
set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
set(FETCHCONTENT_QUIET FALSE)
set(QT_ANDROID_BUILD_ALL_ABIS TRUE)

if(MSVC)
    set(CMAKE_DEBUG_POSTFIX d)
endif()

if(CMAKE_BUILD_TYPE STREQUAL Debug)
    set(CMAKE_INSTALL_DEBUG_LIBRARIES TRUE)
    set(CMAKE_INSTALL_DEBUG_LIBRARIES_ONLY TRUE)
endif()

if(BUILD_SHARED_LIBS)
    set(CMAKE_FRAMEWORK TRUE)
endif()

if(NOT ANDROID)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)
endif()

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_BINARY_DIR})

include(GenerateExportHeader)
include(InstallRequiredSystemLibraries)

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -pedantic-errors)
    if(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
        add_link_options(-fuse-ld=lld)
    endif()
endif()

find_package(QT NAMES Qt6 REQUIRED Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED Core)

string(TOLOWER ${CMAKE_SYSTEM_NAME} APP_SYSTEM_NAME)
if(CMAKE_SYSTEM_PROCESSOR) # Failsafe against iOS Simulator
    string(TOLOWER ${CMAKE_SYSTEM_PROCESSOR} APP_SYSTEM_PROCESSOR)
endif()

add_compile_definitions(APP_SYSTEM_NAME="${APP_SYSTEM_NAME}")
add_compile_definitions(APP_NAME="${CMAKE_PROJECT_NAME}")
add_compile_definitions(APP_VERSION="${CMAKE_PROJECT_VERSION}")
add_compile_definitions(APP_URL="${CMAKE_PROJECT_HOMEPAGE_URL}")

if(Qt6_DIR)
    set(QT_BINDIR ${Qt6_DIR}/../../../bin)
else()
    list(GET Qt6_LIB_DIRS 0 Qt6_DIR)
    set(QT_BINDIR ${Qt6_DIR}/../bin)
endif()
file(REAL_PATH ${QT_BINDIR} QT_BINDIR)
set(QMAKE_EXECUTABLE ${QT_BINDIR}/qmake)

function(app_add_library TARGET)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "STATIC;SHARED;MODULE;INTERFACE;OBJECT" IGNORED1 IGNORED2)

    set(OPT_COUNTER 0)
    if(ARG_STATIC)
        set(BUILD_TYPE STATIC)
        math(EXPR OPT_COUNTER "${OPT_COUNTER}+1")
    elseif(ARG_SHARED)
        set(BUILD_TYPE SHARED)
        math(EXPR OPT_COUNTER "${OPT_COUNTER}+1")
    elseif(ARG_MODULE)
        set(BUILD_TYPE MODULE)
        math(EXPR OPT_COUNTER "${OPT_COUNTER}+1")
    elseif(ARG_INTERFACE)
        set(BUILD_TYPE INTERFACE)
        math(EXPR OPT_COUNTER "${OPT_COUNTER}+1")
    elseif(ARG_OBJECT)
        set(BUILD_TYPE OBJECT)
        math(EXPR OPT_COUNTER "${OPT_COUNTER}+1")
    endif()

    if(OPT_COUNTER EQUAL 0)
        if(BUILD_SHARED_LIBS)
            set(BUILD_TYPE SHARED)
        else()
            set(BUILD_TYPE STATIC)
        endif()
    elseif(OPT_COUNTER GREATER 1)
        message(FATAL_ERROR "Multiple options provided for the build type.")
    endif()

    qt_add_library(
        ${TARGET}
        ${BUILD_TYPE}
        ${ARG_UNPARSED_ARGUMENTS}
    )
endfunction()

function(app_add_plugin TARGET)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "SHARED;STATIC" "OUTPUT_TARGETS;PLUGIN_TYPE" IGNORED)

    if(NOT ARG_PLUGIN_TYPE)
        message(FATAL_ERROR "PLUGIN_TYPE must be provided.")
    endif()

    if(ARG_SHARED AND ARG_STATIC)
        message(FATAL_ERROR "SHARED and STATIC options cannot both be provided.")
    elseif(ARG_SHARED)
        set(BUILD_TYPE SHARED)
    elseif(ARG_STATIC)
        set(BUILD_TYPE STATIC)
    else()
        if(BUILD_SHARED_LIBS)
            set(BUILD_TYPE SHARED)
        else()
            set(BUILD_TYPE STATIC)
        endif()
    endif()

    qt_add_plugin(
        ${TARGET}
        ${BUILD_TYPE}
        OUTPUT_TARGETS ${ARG_OUTPUT_TARGETS}
        PLUGIN_TYPE ${ARG_PLUGIN_TYPE}
        ${ARG_UNPARSED_ARGUMENTS}
    )

    if(ARG_OUTPUT_TARGETS)
        set(${ARG_OUTPUT_TARGETS} ${${ARG_OUTPUT_TARGETS}} PARENT_SCOPE)
    endif()

    set_target_properties(${TARGET} PROPERTIES PLUGIN_TYPE ${ARG_PLUGIN_TYPE})
endfunction()

function(app_add_translations TARGET)
    cmake_parse_arguments(PARSE_ARGV 1 ARG IGNORED1
        "RESOURCE_PREFIX;OUTPUT_TARGETS;QM_FILES_OUTPUT_VARIABLE" IGNORED2)

    if(ARG_RESOURCE_PREFIX)
        qt_add_translations(${ARGV})
    elseif(ARG_QM_FILES_OUTPUT_VARIABLE)
        set(_QM_FILES ${ARG_QM_FILES_OUTPUT_VARIABLE})
        qt_add_translations(${ARGV})
    else()
        if(BUILD_SHARED_LIBS AND NOT ANDROID)
            set(_QM_FILES QM_FILES)
            qt_add_translations(${ARGV} QM_FILES_OUTPUT_VARIABLE QM_FILES)
        else()
            qt_add_translations(${ARGV} RESOURCE_PREFIX /translations)
        endif()
    endif()

    if(ARG_OUTPUT_TARGETS)
        set(${ARG_OUTPUT_TARGETS} ${${ARG_OUTPUT_TARGETS}} PARENT_SCOPE)
    endif()

    if(ARG_QM_FILES_OUTPUT_VARIABLE)
        set(${ARG_QM_FILES_OUTPUT_VARIABLE} ${${ARG_QM_FILES_OUTPUT_VARIABLE}} PARENT_SCOPE)
    endif()

    if(_QM_FILES)
        set_target_properties(${TARGET} PROPERTIES QM_FILES "${${_QM_FILES}}")
    endif()
endfunction()
