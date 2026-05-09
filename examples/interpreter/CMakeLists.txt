cmake_minimum_required(VERSION 3.10.2)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
    ../../src
)

add_executable(tealscript
    main.cpp
)

target_link_libraries(tealscript
    pthread
    dl
)
