cmake_minimum_required(VERSION 3.14)
project(builder_program_selector NONE)

if(NOT DEFINED PROGRAM_FILE)
    message(FATAL_ERROR "Set PROGRAM_FILE to one of the program CMake files in ${CMAKE_CURRENT_LIST_DIR} (e.g. programs/library.cmake).")
endif()

get_filename_component(PROGRAM_PATH "${PROGRAM_FILE}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
if(NOT EXISTS "${PROGRAM_PATH}")
    message(FATAL_ERROR "PROGRAM_FILE '${PROGRAM_PATH}' does not exist. Choose a valid file in ${CMAKE_CURRENT_LIST_DIR}.")
endif()

include("${PROGRAM_PATH}")
