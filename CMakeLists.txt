project(XtermBoatSinker)
cmake_minimum_required(VERSION 2.8)
include_directories(src)
aux_source_directory(src SRC_LIST)

list(REMOVE_ITEM SRC_LIST "src/Client.cpp")
add_executable(xbs-server ${SRC_LIST})

list(REMOVE_ITEM SRC_LIST "src/Server.cpp")
list(APPEND SRC_LIST "src/Client.cpp")
add_executable(xbs-client ${SRC_LIST})
