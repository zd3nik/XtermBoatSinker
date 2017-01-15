cmake_minimum_required(VERSION 2.8)
include(../init.cmake)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
add_subdirectory(xbs)
include_directories(xbs)

project(server)
add_executable(xbs-server "ServerMain.cpp")
target_link_libraries(xbs-server xbs)

project(client)
add_executable(xbs-client "UserClient.cpp")
target_link_libraries(xbs-client xbs)

project(rufus)
include_directories(bots)
add_executable(xbs-rufus "bots/RandomRufus.cpp")
target_link_libraries(xbs-rufus xbs)
