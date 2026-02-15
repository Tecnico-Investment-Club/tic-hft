# HFT Orders Manager

Simple order execution system for Binance crypto trading.

## Build

```bash
mkdir build && cd build
cmake ..
make
```

## Run

```bash
# Dry-run mode
./hft_orders --dry-run

# With logging
./hft_orders --log-file orders.json

# Production
export BINANCE_API_KEY="your_key"
export BINANCE_API_SECRET="your_secret"
./hft_orders
```

## Configuration

Environment variables:
- `BINANCE_API_KEY` - API key
- `BINANCE_API_SECRET` - API secret
- `DRY_RUN` - true/false
- `MIN_SLEEP` - Min poll interval (ms)
- `MAX_SLEEP` - Max poll interval (ms)
- `LOG_FILE` - Optional JSON log file

## Docker

```bash
docker build -t tic-hft-orders .
docker-compose up
```

## Architecture

- Models.hpp - Order structures
- BinanceClient.hpp/cpp - Binance REST API
- OrderStore.hpp/cpp - In-memory order storage
- OrdersManager.hpp/cpp - Order execution engine
