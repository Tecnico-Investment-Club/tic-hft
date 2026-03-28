cmake_minimum_required(VERSION 3.16)
project(hft_position LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_POSITION_TESTS "Build tests for position module" OFF)

add_library(hft-position
    src/PositionTracker.cpp
)

target_include_directories(hft-position
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(BUILD_POSITION_TESTS)
    enable_testing()
    add_executable(position_test tests/position_test.cpp)
    target_link_libraries(position_test PRIVATE hft-position)
    add_test(NAME position_test COMMAND position_test)
endif()
