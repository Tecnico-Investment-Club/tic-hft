cmake_minimum_required(VERSION 3.15)
project(tic-hft-orders)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(CURL REQUIRED)

# Main library - only execution component
add_library(hft-orders STATIC
    src/execution/AlpacaOrderExecutor.cpp
)

target_include_directories(hft-orders PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(hft-orders PUBLIC CURL::libcurl)

# Example executable
add_executable(example_fire_and_forget examples/example_fire_and_forget.cpp)
target_link_libraries(example_fire_and_forget hft-orders)
target_include_directories(example_fire_and_forget PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Optional: Tests (if you want to keep test infrastructure)
option(BUILD_TESTS "Build test suite" ON)

if(BUILD_TESTS)
    find_package(GTest QUIET)
    
    if(GTest_FOUND)
        add_executable(orders_test tests/orders_test.cpp)
        target_link_libraries(orders_test hft-orders GTest::gtest GTest::gtest_main)
        target_include_directories(orders_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
        
        enable_testing()
        add_test(NAME OrdersTests COMMAND orders_test)
    else()
        message(STATUS "GoogleTest not found - install with: apt-get install libgtest-dev")
    endif()
endif()
