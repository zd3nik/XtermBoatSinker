project(XtermBoatSinker)
cmake_minimum_required(VERSION 2.8)
include_directories(src)
aux_source_directory(src SRC_LIST)

add_executable(xbs-server ${SRC_LIST} "main/ServerMain.cpp")
add_executable(xbs-client ${SRC_LIST} "main/UserClient.cpp")
add_executable(xbs-rufus ${SRC_LIST} "main/RandomRufus.cpp")
add_executable(xbs-hal ${SRC_LIST} "main/Hal9000.cpp")
