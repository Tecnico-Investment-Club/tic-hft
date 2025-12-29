# tic-hft — ETL

This repository contains a C++ ETL that connects to Binance over WebSocket, parses closed klines (candles) with `simdjson`, pushes them into a lock-free `RingBuffer`, and prints them periodically. The built binary is `hft_etl`.

## Overview
- Main coordination: [etl/src/main.cpp](etl/src/main.cpp) manages the `RingBuffer` and the print loop.
- Broker integration: [etl/include/broker/BinanceBroker.hpp](etl/include/broker/BinanceBroker.hpp), [etl/src/broker/BinanceBroker.cpp](etl/src/broker/BinanceBroker.cpp) subscribe to `btcusdt@kline_1m` and forward messages.
- WebSocket client: [etl/include/network/WebSocketClient.hpp](etl/include/network/WebSocketClient.hpp), [etl/src/network/WebSocketClient.cpp](etl/src/network/WebSocketClient.cpp) (IXWebSocket).
- JSON parser: [etl/include/transform/MarketDataParser.hpp](etl/include/transform/MarketDataParser.hpp), [etl/src/transform/MarketDataParser.cpp](etl/src/transform/MarketDataParser.cpp) produce `Kline` objects.
- Types & buffer: [etl/include/core/Types.hpp](etl/include/core/Types.hpp), [etl/include/core/RingBuffer.hpp](etl/include/core/RingBuffer.hpp).
- Endpoint config: [etl/common/Constants.hpp](etl/common/Constants.hpp).

## Clean build & run (Linux)

```
sudo apt-get update && sudo apt-get install -y libssl-dev cmake
rm -rf build
cmake -S etl -B build
cmake --build build --config Release
./build/hft_etl
```

## Prerequisites (macOS)
1) Install Xcode Command Line Tools:

```
xcode-select --install
```

2) Install Homebrew (if needed):

```
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

3) Install dependencies:

```
brew install cmake openssl@3
```

## Clean build & run (macOS)
Create a fresh build directory to avoid stale CMake state:

```
rm -rf build
cmake -S etl -B build \
	-DOPENSSL_ROOT_DIR=$(brew --prefix openssl@3) \
	-DOPENSSL_INCLUDE_DIR=$(brew --prefix openssl@3)/include \
	-DOPENSSL_SSL_LIBRARY=$(brew --prefix openssl@3)/lib/libssl.dylib \
	-DOPENSSL_CRYPTO_LIBRARY=$(brew --prefix openssl@3)/lib/libcrypto.dylib
cmake --build build --config Release
./build/hft_etl
```

Notes:
- On Apple Silicon, Homebrew is typically under `/opt/homebrew`; on Intel, `/usr/local`. `brew --prefix openssl@3` resolves the correct path.
- CMake should print “TLS configured to use openssl”. If not, double-check the flags above.

## What to expect when running
- Connection logs: “Handshake efetuado! Conectado.” followed by “Conectado! A enviar subscrição...”.
- After each 1-minute candle closes, you’ll see messages like:
	- “⏰ KLINE PROCESSADA”
	- “Symbol: BTCUSDT”
	- “Close Price: <number>”

## Developer notes
- When you add new `.cpp` files, include them in [etl/CMakeLists.txt](etl/CMakeLists.txt).
- Incremental rebuild:

```
cmake --build build --config Release
```

- To change the endpoint, edit [etl/common/Constants.hpp](etl/common/Constants.hpp).

## Troubleshooting
- No klines after ~1–2 minutes: make sure the endpoint (spot/futures, testnet/mainnet) matches the subscription channel (`btcusdt@kline_1m`).
- macOS firewall: allow network access when prompted.
- macOS deprecation warnings: ensure the build uses OpenSSL (see macOS clean build flags above).
