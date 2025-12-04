cmake_minimum_required(VERSION 3.14)
project(DynamicalSystemSimulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- DEPENDENCY MANAGEMENT ---

# 1. Try to find Raylib on the system first (Standard /usr/local or Brew)
find_package(raylib QUIET)

# 2. If not found, download and build it from source (The "Robotics" fallback)
if (NOT raylib_FOUND)
    message(STATUS "Raylib not found on system. Downloading from GitHub...")
    
    include(FetchContent)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG        5.0  # Lock to a specific version for stability
    )
    
    # This downloads Raylib, builds it, and makes it available to link
    FetchContent_MakeAvailable(raylib)
endif()

# --- BUILD DEFINITIONS ---

add_executable(dynamic main.cpp)

# Link Raylib (works whether it was found systematically or built locally)
target_link_libraries(dynamic raylib)

# Linux/Mac Math compatibility (Standard hygiene)
if(UNIX AND NOT APPLE)
    target_link_libraries(dynamic m pthread dl)
endif()
