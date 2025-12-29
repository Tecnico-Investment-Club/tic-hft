cmake_minimum_required(VERSION 3.14)
project(HFT_ETL)


set(USE_TLS 1 CACHE BOOL "Force TLS" FORCE)
set(USE_OPENSSL 1 CACHE BOOL "Force OpenSSL" FORCE)

find_package(OpenSSL REQUIRED)


include(FetchContent)

FetchContent_Declare(
  simdjson
  GIT_REPOSITORY https://github.com/simdjson/simdjson.git
  GIT_TAG v3.0.0 
)
FetchContent_MakeAvailable(simdjson)

FetchContent_Declare(
  ixwebsocket
  GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
  GIT_TAG v11.4.4
)
FetchContent_MakeAvailable(ixwebsocket)


add_executable(hft_etl 
    src/main.cpp
    src/network/WebSocketClient.cpp
    src/transform/MarketDataParser.cpp
    src/broker/BinanceBroker.cpp
)

target_link_libraries(hft_etl PRIVATE 
    simdjson 
    ixwebsocket::ixwebsocket
    OpenSSL::SSL 
    OpenSSL::Crypto
)