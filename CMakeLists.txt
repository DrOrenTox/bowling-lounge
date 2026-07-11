cmake_minimum_required(VERSION 3.21)

# FIXED: Project name matches mod.json ID to prevent missing binary name errors
project(mateo18023.cosmic_bowling_lounge VERSION 1.0.1)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK!")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

# Quotes fix space path conflicts on Windows and Linux runners
add_subdirectory("$ENV{GEODE_SDK}" "${CMAKE_CURRENT_BINARY_DIR}/geode")

# Dual-case glob ensures compilation succeeds on case-sensitive Linux platforms
file(GLOB_RECURSE SOURCES 
    "src/*.cpp" 
    "Src/*.cpp"
    "src/*.hpp"
    "Src/*.hpp"
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})
setup_geode_mod(${PROJECT_NAME} RESOURCES resources)
