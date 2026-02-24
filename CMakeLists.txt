cmake_minimum_required(VERSION 3.14)
project(tic-engine VERSION 0.1.0 LANGUAGES CXX)

# Set C++20 as the standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# --- 1. DEPENDENCIES ---
# Automatically fetch GoogleTest for our testing pillar
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# Prevent Windows test runner issues
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE) 
FetchContent_MakeAvailable(googletest)

# --- 2. CORE LIBRARY ---
# We build the engine as a static library. 
add_library(tic_core STATIC)

# Target sources
target_sources(tic_core PRIVATE 
    src/strategy/PingPongStrategy.cpp
)

# Tell the library where our header files are (your 'inc' folder)
target_include_directories(tic_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

# --- 3. MAIN EXECUTABLE ---
add_executable(tic-engine src/main.cpp)
target_link_libraries(tic-engine PRIVATE tic_core)

# --- 4. TESTING SETUP ---
enable_testing()

# NOTE: We will uncomment this block as soon as we write our first test!
# add_executable(strategy_tests tests/strategy_tests/test_ping_pong.cpp)
# target_link_libraries(strategy_tests PRIVATE tic_core GTest::gtest_main)
# target_include_directories(strategy_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/inc)
# include(GoogleTest)
# gtest_discover_tests(strategy_tests)