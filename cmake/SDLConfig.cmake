# SDLConfig.cmake
# Fetch and configure SDL3 and SDL3_image
message(STATUS "Configuring and fetching SDL3 + SDL3_image...")
include(FetchContent)

# SDL3 options (disable problematic features)
set(SDL_PIPEWIRE OFF CACHE BOOL "" FORCE)
set(SDL_PIPEWIRE_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_PULSEAUDIO ON CACHE BOOL "" FORCE)
set(SDL_PULSEAUDIO_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_ALSA ON CACHE BOOL "" FORCE)

if(UNIX AND NOT APPLE)
    set(SDL_VIDEO_X11 ON CACHE BOOL "" FORCE)
    set(SDL_X11_XINPUT2 OFF CACHE BOOL "" FORCE)
    set(SDL_X11_XFIXES OFF CACHE BOOL "" FORCE)
    set(SDL_X11_XRANDR OFF CACHE BOOL "" FORCE)
    set(SDL_X11_XSCRNSAVER OFF CACHE BOOL "" FORCE)
    set(SDL_X11_XSHAPE OFF CACHE BOOL "" FORCE)
endif()

set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.0
)
FetchContent_MakeAvailable(SDL3)

# SDL_image options
set(SDL3IMAGE_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL3IMAGE_DEPS_SHARED OFF CACHE BOOL "" FORCE)
set(SDL3IMAGE_VENDORED ON CACHE BOOL "" FORCE)
set(SDL3IMAGE_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    SDL3_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG release-3.2.0
)
FetchContent_MakeAvailable(SDL3_image)
