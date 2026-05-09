cmake_minimum_required(VERSION 3.10.2)

add_definitions(-DTEALSCRIPT_NO_EIGEN)

add_library(ray SHARED
    libmain.cpp
)

target_link_libraries(ray
    raylib
)
