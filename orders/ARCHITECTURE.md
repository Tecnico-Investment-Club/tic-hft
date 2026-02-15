# HFT Orders Manager - Structure Overview

```
tic-hft/orders/
│
├── 📄 CMakeLists.txt                # Build configuration
│   ├─ simdjson library
│   ├─ nlohmann/json library  
│   ├─ libcurl
│   ├─ libpq
│   └─ OpenSSL
│
├── 📁 include/                      # Header files (declarations)
│   ├─ Models.hpp                   # Order, OrderStatus, OrderSide, OrderType
│   ├─ BinanceClient.hpp            # REST API wrapper for Binance
│   ├─ Database.hpp                 # PostgreSQL interface
│   └─ OrdersManager.hpp            # Main orchestrator
│
├── 📁 src/                         # Implementation files  
│   ├─ main.cpp                     # Entry point, argument parsing, signal handling
│   ├─ BinanceClient.cpp            # HTTP client, HMAC signatures, API calls
│   ├─ Database.cpp                 # SQL queries, schema creation
│   └─ OrdersManager.cpp            # Main event loop, order processing
│
├── 📁 build/                       # Build artifacts (git ignored)
│   └─ hft_orders (executable)
│
├── 📁 db/                          # Database documentation
│   └─ SCHEMA.md                    # DB schema, queries, monitoring
│
├── 📄 Dockerfile                   # Multi-stage Docker build
│   ├─ Builder stage: Compile
│   └─ Runtime stage: Minimal image
│
├── 📄 docker-compose.yml           # Service orchestration
│   ├─ hft-orders service
│   ├─ hft-etl dependency
│   └─ tic-network
│
├── 📄 README.md                    # Project overview, quick start
├── 📄 DEVELOPMENT.md               # Dev guide, debugging, testing
├── 📄 EXAMPLES.md                  # Configuration & usage examples
│
├── 📄 .gitignore                   # Git ignore patterns
├── 📄 .env.example                 # Environment template
│
└── 📖 This file                    # Structure overview
```

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│           HFT Trading Strategy (C++ in hft-etl)              │
│              Generates Order Signals                          │
└────────────────────┬────────────────────────────────────────┘
                     │ submit_order(Order)
                     │
                     ▼
        ┌────────────────────────────┐
        │   OrdersManager (C++)      │
        │  - Event loop              │
        │  - Order queue             │
        │  - Status updates          │
        └────────────────────────────┘
                     │
        ┌────────────┴──────────────┐
        │                           │
        ▼                           ▼
  ┌──────────────┐           ┌──────────────┐
  │BinanceClient │           │  Database    │
  │ - REST API   │           │- PostgreSQL  │
  │ - HMAC-SHA256│           │- Persistence │
  │ - JSON parse │           │- Queries     │
  └──────────────┘           └──────────────┘
        │                           │
        ▼                           ▼
  Binance Exchange         PostgreSQL Server
  REST API (libcurl)       (hft_orders schema)
```

## Key Components

### 1. **Models.hpp** (35 lines)
Data structures for orders:
- `enum OrderStatus` - PENDING, NEW, PARTIALLY_FILLED, FILLED, CANCELED, etc.
- `enum OrderSide` - BUY, SELL
- `enum OrderType` - LIMIT, MARKET
- `struct Order` - Full order object
- `struct OrderLatest` - Latest state snapshot

### 2. **BinanceClient.hpp/cpp** (150 lines)
REST API wrapper:
- `place_limit_order()` - Place limit orders
- `place_market_order()` - Place market orders
- `get_order_status()` - Query order status
- `cancel_order()` - Cancel orders
- `http_get/post()` - HTTP methods with HMAC signatures
- `generate_signature()` - HMAC-SHA256 signing
- Dependencies: libcurl, OpenSSL, nlohmann/json

### 3. **Database.hpp/cpp** (200 lines)
PostgreSQL interface:
- `ensure_schema()` - Create tables/indexes on startup
- `insert_order()` - Add new orders
- `get_pending_orders()` - Fetch pending orders
- `get_active_orders()` - Fetch NEW/PARTIALLY_FILLED
- `update_order_status()` - Update status
- `update_order_binance_id()` - Link to Binance ID
- `update_order_fills()` - Update fill info
- Dependencies: libpq (PostgreSQL client library)

### 4. **OrdersManager.hpp/cpp** (200 lines)
Main orchestrator:
- `run()` - Start main event loop
- `submit_order()` - Add order to queue
- `main_loop()` - Process pending, update active
- `process_order()` - Place order on Binance
- `update_order_status()` - Check Binance for updates
- `shutdown()` - Graceful shutdown

### 5. **main.cpp** (200 lines)
CLI entry point:
- `parse_arguments()` - Parse CLI args & env vars
- Signal handlers for SIGINT/SIGTERM
- Configuration validation
- Service initialization

## Build & Deployment

```
Local Development          Docker Development        Production
─────────────────────────────────────────────────────────────
mkdir build                docker build .             Push to registry
cmake ..                   docker run                 Deploy via K8s
make -j4                   --env-file .env            Load balance
./hft_orders               tic-hft-orders             Scale horizontally
  ↓
  Quick iteration         Container tested           Zero downtime
  Easy debugging          IDE agnostic               Reproducible
  Fast feedback           CI/CD ready                Observable
```

## Database Schema

```
┌─────────────────────────────────────────────────────────┐
│  PostgreSQL: tic (TIC_Strategy_App database)            │
├─────────────────────────────────────────────────────────┤
│  hft_orders.orders [MAIN TABLE]                        │
│  ┌──────────────────────────────────────────────────┐  │
│  │ order_id (PK)  | symbol  | side | type | ...    │  │
│  │ 1001           | BTCUSDT | BUY  | LIMIT| ...    │  │
│  │ 1002           | ETHUSDT | SELL | MARKET|...    │  │
│  └──────────────────────────────────────────────────┘  │
│                                                         │
│  Indexes:                                              │
│  - idx_orders_status (for filtering by status)        │
│  - idx_orders_symbol (for symbol lookups)             │
│  - idx_orders_created_at (for time-based queries)     │
└─────────────────────────────────────────────────────────┘
```

## Dependencies

```
External Libraries (CMake FetchContent downloads):
├── simdjson (3.0.0)          → ultrafast JSON parsing
├── nlohmann/json (3.11.2)    → convenient JSON API
│
System Libraries (must install):
├── libcurl-dev               → HTTP client (REST API)
├── libpq-dev                 → PostgreSQL client
├── libssl-dev                → OpenSSL (HMAC-SHA256)
├── postgresql-client         → psql CLI tool
│
Compiler Requirements:
└── C++20 standard            → Modern C++ features
    GCC 10+, Clang 12+, MSVC 2019+
```

## Performance Characteristics

```
Operation                    Typical Latency
──────────────────────────────────────────────
Place Limit Order           50-100ms   (network)
Place Market Order          30-80ms    (network)
Query Order Status          20-50ms    (network)
DB Insert                   2-5ms      (local)
DB Update                   2-5ms      (local)
DB Select                   5-10ms     (local)
─────────────────────────────────────────────
Total E2E (submit→placed)   ~100-120ms (typical)
```

## Thread Architecture

```
┌────────────────────────────────────┐
│ main() - Parse args, init services │
└────────────────┬───────────────────┘
                 │
                 ▼
    ┌────────────────────────────┐
    │ Main Loop Thread           │
    │ - Fetch pending orders     │
    │ - Process each order       │
    │ - Update active orders     │
    │ - Sleep (min/max)          │
    └────────────────────────────┘
                 │
    ┌────────────┴───────────┬─────────────────┐
    │                        │                 │
    ▼                        ▼                 ▼
 Binance                 PostgreSQL        Queue
 (libcurl)               (libpq)          (thread-safe)
```

## Configuration Methods (Priority Order)

1. Command-line arguments: `--dry-run --min-sleep 1`
2. Environment variables: `DRY_RUN=true MIN_SLEEP=1`
3. Defaults: Hardcoded in parse_arguments()

---

**Created:** February 15, 2026 | **Language:** C++20 | **Purpose:** Ultra-low latency crypto trading order management
