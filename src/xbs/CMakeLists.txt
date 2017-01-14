cmake_minimum_required(VERSION 2.8)
project(xbs)
include(../../init.cmake)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

include_directories(.)
aux_source_directory(. SRC_LIST)
aux_source_directory(db SRC_LIST)

add_library(xbs STATIC ${SRC_LIST})
