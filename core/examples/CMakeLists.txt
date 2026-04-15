cmake_minimum_required(VERSION 3.16)

# Create example executable
add_executable(orchestrator_example orchestrator_example.cpp)

target_include_directories(orchestrator_example
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

target_link_libraries(orchestrator_example
    PRIVATE
        hft-position
        hft-risk
)
