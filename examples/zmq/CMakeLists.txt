cmake_minimum_required(VERSION 3.10.2)

add_definitions(-DTEALSCRIPT_NO_EIGEN)

add_library(zeromq SHARED
    libmain.cpp
)

target_link_libraries(zeromq
    zmq
)
