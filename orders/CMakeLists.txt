cmake_minimum_required(VERSION 3.14)
project(HFT_ORDERS)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenSSL REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(CURL REQUIRED)

# FetchContent for dependencies
include(FetchContent)

# simdjson for fast JSON parsing
FetchContent_Declare(
  simdjson
  GIT_REPOSITORY https://github.com/simdjson/simdjson.git
  GIT_TAG v3.0.0 
)
FetchContent_MakeAvailable(simdjson)

# nlohmann/json for convenience (optional, can use simdjson alone)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(nlohmann_json)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${POSTGRESQL_INCLUDE_DIRS})
include_directories(/usr/include/postgresql)

# Source files
set(SOURCES
    src/main.cpp
    src/BinanceClient.cpp
    src/Database.cpp
    src/OrdersManager.cpp
)

# Create executable
add_executable(hft_orders ${SOURCES})

# Link libraries
target_link_libraries(hft_orders PRIVATE
    simdjson
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
    CURL::libcurl
    pq
    ${POSTGRESQL_LIBRARIES}
    pthread
)

# Compiler flags for optimization
if(MSVC)
    target_compile_options(hft_orders PRIVATE /O2 /W4)
else()
    target_compile_options(hft_orders PRIVATE -O3 -Wall -Wextra -march=native)
endif()
