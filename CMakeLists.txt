cmake_minimum_required(VERSION 3.25)

project(cosmic_bowling_lounge VERSION 1.0.0)

find_package(geode REQUIRED)

add_geode_mod(
    mateo18023.cosmic_bowling_lounge
    VERSION 1.0.0
    FILES Main.cpp
)
