set(CMAKE_CXX_FLAGS "-Wall ${CMAKE_CXX_FLAGS}")
if (CMAKE_VERSION VERSION_LESS "3.1")
  set(CMAKE_CXX_FLAGS "--std=c++11 ${CMAKE_CXX_FLAGS}")
else()
  set(CMAKE_CXX_STANDARD 11)
endif()
