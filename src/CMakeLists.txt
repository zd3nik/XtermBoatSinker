cmake_minimum_required(VERSION 2.8)
project(main)
include(../init.cmake)
add_subdirectory(xbs)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

include_directories(xbs)
add_executable(xbs-server "ServerMain.cpp")
add_executable(xbs-client "UserClient.cpp")

target_link_libraries(xbs-server xbs)
target_link_libraries(xbs-client xbs)

