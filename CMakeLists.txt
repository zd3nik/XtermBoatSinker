project(XtermBoatSinker)
cmake_minimum_required(VERSION 2.8)

if (CMAKE_VERSION VERSION_LESS "3.1")
  set(CMAKE_CXX_FLAGS "--std=c++11 ${CMAKE_CXX_FLAGS}")
else()
  set(CMAKE_CXX_STANDARD 11)
endif()

set(CMAKE_CXX_FLAGS "-Wall ${CMAKE_CXX_FLAGS}")

include_directories(src src/db src/bots)
aux_source_directory(src SRC_LIST)
aux_source_directory(src/db SRC_LIST)
aux_source_directory(src/bots SRC_LIST)

add_executable(xbs-server ${SRC_LIST} "src/main/ServerMain.cpp")
add_executable(xbs-client ${SRC_LIST} "src/main/UserClient.cpp")
