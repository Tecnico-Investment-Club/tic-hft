cmake_minimum_required(VERSION 3.14)
project(HFT_ORDERS)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

include(FetchContent)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(nlohmann_json)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SOURCES
    src/main.cpp
    src/BinanceClient.cpp
    src/OrderStore.cpp
    src/OrdersManager.cpp
)

add_executable(hft_orders ${SOURCES})

target_link_libraries(hft_orders PRIVATE
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
    CURL::libcurl
    pthread
)

if(NOT MSVC)
    target_compile_options(hft_orders PRIVATE -O3 -Wall -Wextra)
endif()
