cmake_minimum_required(VERSION 3.16)
project(hft_risk LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_RISK_TESTS "Build tests for risk module" OFF)

add_library(hft-risk
    src/RiskEngine.cpp
)

target_include_directories(hft-risk
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(BUILD_RISK_TESTS)
    enable_testing()
    add_executable(risk_test tests/risk_test.cpp)
    target_link_libraries(risk_test PRIVATE hft-risk)
    add_test(NAME risk_test COMMAND risk_test)
endif()
