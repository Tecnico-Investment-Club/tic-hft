# Configuração da App
cmake_minimum_required(VERSION 3.14)
project(HFT_ETL)

include(FetchContent)

# Buscar simdjson
FetchContent_Declare(
  simdjson
  GIT_REPOSITORY https://github.com/simdjson/simdjson.git
  GIT_TAG v3.0.0 
)
FetchContent_MakeAvailable(simdjson)

# Buscar IXWebSocket
FetchContent_Declare(
  ixwebsocket
  GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
  GIT_TAG v11.4.4
)
FetchContent_MakeAvailable(ixwebsocket)

set(USE_TLS 1 CACHE BOOL "Force TLS" FORCE)

find_package(OpenSSL REQUIRED)

# ... resto da tua configuração ...
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