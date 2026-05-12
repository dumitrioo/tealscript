cmake_minimum_required(VERSION 3.10.2)

add_library(zeromq SHARED
    libmain.cpp
)

target_link_libraries(zeromq
    zmq
)
