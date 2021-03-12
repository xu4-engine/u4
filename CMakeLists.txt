cmake_minimum_required(VERSION 2.6)
project(xu4)

add_subdirectory (src) 

# REQUIRED does not work in CMake <=2.4.6 for SDL
find_package (SDL REQUIRED)
find_package (SDL_mixer REQUIRED)
find_package (LibXml2 REQUIRED)
find_package (PNG REQUIRED)
find_package (ZLIB REQUIRED)

# Workaround for the non-working REQUIRED flag
if (NOT SDL_FOUND)
   message (FATAL_ERROR "SDL not found!")
endif (NOT SDL_FOUND)

if (NOT SDLMIXER_FOUND)
   message (FATAL_ERROR "SDL_mixer not found!")
endif (NOT SDLMIXER_FOUND)

