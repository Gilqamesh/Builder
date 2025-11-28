cmake_minimum_required(VERSION 3.14)
project(builder NONE)

message(FATAL_ERROR "Configure one of the program-specific CMake files under programs/ by running the helper scripts in scripts/ or invoking CMake with -S programs -DPROGRAM_FILE=programs/<name>.cmake.")
