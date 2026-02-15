# HFT Orders Manager - C++

Ultra-low latency order execution and tracking system for Binance crypto trading. Built in C++ for microsecond-level response times.

## Overview

The HFT Orders service is a high-performance order management system that:
- Executes orders on Binance with minimal latency
- Tracks order status and fills in real-time
- Persists order data to PostgreSQL
- Coordinates with the HFT-ETL C++ trading engine

## Architecture

```
HFT Strategy Signals (C++ trading engine)
            ↓
    HFT Orders Manager (C++)
            ↓
     Binance REST API (libcurl)
            ↓
    PostgreSQL Database
```

## Technology Stack

- **Language**: C++20
- **HTTP Client**: libcurl (for Binance REST API)
- **JSON**: nlohmann/json + simdjson (high-performance parsing)
- **Database**: libpq (PostgreSQL async driver)
- **Cryptography**: OpenSSL (HMAC-SHA256 signatures)
- **Build System**: CMake 3.14+
- **Threading**: std::thread, std::mutex

## Building

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get install -y \
    build-essential \
    cmake \
    libcurl4-openssl-dev \
    libpq-dev \
    libssl-dev \
    postgresql-client

# macOS
brew install cmake curl libpq openssl
```

### Compilation

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)

# Run
./hft_orders --help
```

## Configuration

Configure via environment variables or command-line arguments:

```bash
# Environment variables
export DATABASE_URL="postgresql://user:pass@localhost:5432/tic"
export BINANCE_API_KEY="your_api_key"
export BINANCE_API_SECRET="your_api_secret"
export DRY_RUN="false"
export MIN_SLEEP=1
export MAX_SLEEP=5

# Or command-line
./hft_orders --db "postgresql://..." --dry-run --min-sleep 1
```

## Running

```bash
# Development - dry-run mode
./hft_orders --dry-run

# Production
./hft_orders

# In Docker
docker build -t tic-hft-orders .
docker run -e BINANCE_API_KEY=xxx -e DATABASE_URL=postgresql://... tic-hft-orders
```

## Database Schema

The service automatically creates the required schema on startup:

```sql
CREATE SCHEMA hft_orders;

CREATE TABLE hft_orders.orders (
    order_id BIGINT PRIMARY KEY,
    symbol VARCHAR(20) NOT NULL,
    side VARCHAR(4) NOT NULL,
    type VARCHAR(10) NOT NULL,
    quantity DECIMAL(18,8) NOT NULL,
    price DECIMAL(18,8),
    status VARCHAR(20) NOT NULL,
    binance_order_id BIGINT,
    filled_quantity DECIMAL(18,8),
    avg_price DECIMAL(18,8),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

## API Response Times

Expected latencies (from order submission to Binance):
- Limit Order: ~50-100ms
- Market Order: ~30-80ms
- Status Update: ~20-50ms

## Performance Notes

- **Zero-copy operations** where possible
- **Connection pooling** to PostgreSQL
- **Async-ready** architecture (ready for async I/O)
- **Memory-efficient** fixed-size buffers
- **CPU-optimized** with compiler flags (-O3, -march=native)

## Error Handling

- Graceful shutdown on SIGINT/SIGTERM
- Automatic reconnection on database failure
- Order state persistence for recovery
- Detailed logging for debugging

## Contributing

This is part of the Tecnico Investment Club's HFT system.

## License

Educational use only - Instituto Superior Técnico
