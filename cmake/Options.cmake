include_guard(GLOBAL)

if(IOS)
    set(BUILD_SHARED_LIBS FALSE CACHE BOOL
        "Build using shared libraries." FORCE)
else()
    set(BUILD_SHARED_LIBS TRUE CACHE BOOL
        "Build using shared libraries.")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Build type (Debug, Release, RelWithDebInfo, MinSizeRel)." FORCE)
endif()

set(APP_DEPLOY_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/deploy CACHE PATH
    "The directory to put deployment packages.")

if(CMAKE_BUILD_TYPE STREQUAL Debug AND NOT ANDROID AND NOT IOS)
    set(APP_DEPLOY_AS_PART_OF_ALL FALSE CACHE BOOL
        "Build deployment packages in the make step.")
else()
    set(APP_DEPLOY_AS_PART_OF_ALL TRUE CACHE BOOL
        "Build deployment packages in the make step.")
endif()
