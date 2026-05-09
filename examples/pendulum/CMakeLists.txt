cmake_minimum_required(VERSION 3.10.2)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-maybe-uninitialized -Wno-unused-parameter -Wno-reorder -Wno-deprecated-copy -Wno-strict-aliasing")

add_definitions(-DTEALSCRIPT_NO_EIGEN)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
    bullet
    ../../src
)

add_executable(pendulum
    main.cpp
    bullet/btBulletCollisionAll.cpp
    bullet/btBulletDynamicsAll.cpp
    bullet/btLinearMathAll.cpp
)

target_link_libraries(pendulum
    raylib
)
