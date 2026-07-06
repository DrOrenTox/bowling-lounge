cmake_minimum_required(VERSION 3.21)

project(cosmic_bowling_lounge VERSION 1.0.0)

# Check if Geode SDK environment variable is properly set
if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

# Include the Geode SDK build directory configurations
add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# Safely find all C++ source and header files across both case-sensitive Linux and Windows
file(GLOB_RECURSE SOURCES 
    "src/*.cpp" 
    "Src/*.cpp"
    "src/*.hpp"
    "Src/*.hpp"
)

# Define the shared library module for your mod
add_library(${PROJECT_NAME} SHARED ${SOURCES})

# Configures the package metadata and bundles your graphics assets
# This embeds your custom bowling images into the final compiled .geode binary
setup_geode_mod(${PROJECT_NAME} RESOURCES resources)
