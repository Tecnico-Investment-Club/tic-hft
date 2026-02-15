# Development Guide

## Project Structure

```
tic-hft/orders/
├── include/                    # Header files
│   ├── Models.hpp             # Order structures and enums
│   ├── BinanceClient.hpp      # Binance REST API client
│   ├── Database.hpp           # PostgreSQL interface
│   └── OrdersManager.hpp      # Main orchestrator
├── src/                       # Implementation files
│   ├── main.cpp               # Entry point
│   ├── BinanceClient.cpp      # Binance API implementation
│   ├── Database.cpp           # Database implementation
│   └── OrdersManager.cpp      # Orders manager implementation
├── build/                     # Build artifacts (git ignored)
├── CMakeLists.txt             # Build configuration
├── Dockerfile                 # Container image
├── docker-compose.yml         # Service orchestration
├── README.md                  # Main documentation
└── .env.example               # Configuration template
```

## Building for Development

### Local Build

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
make -j$(nproc)

# Run tests (optional - add if tests.cpp)
./hft_orders --help

# Run with dry-run
DRY_RUN=true DATABASE_URL=... ./hft_orders
```

### With Docker

```bash
# Build image
docker build -t tic-hft-orders:dev .

# Run container with env file
docker run --env-file .env --rm tic-hft-orders:dev
```

## Code Style

- C++20 modern features
- Snake case for functions and variables
- PascalCase for classes and structs
- Const correctness
- RAII principles
- No dynamic allocation where possible

## Adding New Features

### Adding a New Order Type

1. Add enum in `include/Models.hpp`:
   ```cpp
   enum class OrderType : std::uint8_t {
       LIMIT = 0,
       MARKET = 1,
       STOP_LOSS = 2,  // New
   };
   ```

2. Update `BinanceClient::place_order()` in `src/BinanceClient.cpp`

3. Update database schema in `src/Database.cpp`

4. Add tests

### Adding Database Queries

1. Add method to `Database` class header
2. Implement in `Database.cpp`
3. Use `execute_query()` helper
4. Handle errors appropriately

## Debugging

### Enable verbose logging

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
./hft_orders  # Will show more debug output
```

### GDB Debugging

```bash
gdb ./hft_orders

# Set breakpoint
(gdb) break src/OrdersManager.cpp:50

# Run
(gdb) run

# Continue
(gdb) continue

# Print variables
(gdb) print order_id
```

### Address Sanitizer

```bash
cmake -DCMAKE_CXX_FLAGS="-fsanitize=address" ..
make
./hft_orders
```

## Performance Profiling

### Using perf (Linux)

```bash
perf record ./hft_orders
perf report
```

### Using Valgrind

```bash
valgrind --leak-check=full ./hft_orders
```

## Testing

Add unit tests in a `tests/` directory:

```cpp
// tests/test_binance_client.cpp
#include <cassert>
#include "../include/BinanceClient.hpp"

void test_signature_generation() {
    BinanceClient client("key", "secret", true);
    // Test code
    assert(/* condition */);
}
```

Compile with:
```bash
g++ -std=c++20 tests/*.cpp src/*.cpp -o tests -I./include
./tests
```

## Dependencies

- **libcurl**: `apt-get install libcurl4-openssl-dev`
- **libpq**: `apt-get install libpq-dev`
- **OpenSSL**: `apt-get install libssl-dev`
- **CMake 3.14+**: `apt-get install cmake`

## Continuous Integration

For CI/CD pipelines, use:
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
make
ctest  # If tests are added
```

## Performance Tips

1. **Reduce allocations** - Use stack variables when possible
2. **Connection pooling** - Reuse DB connections
3. **Binary protocols** - Consider binary format for high-freq data
4. **Memory mapping** - For large order histories
5. **SIMD** - Use simdjson for JSON parsing speedup

## Common Issues

### PostgreSQL Connection Failed
- Check `DATABASE_URL` environment variable
- Ensure PostgreSQL is running
- Verify credentials in `.env`
- Check firewall/network access

### Binance API Errors
- Verify `BINANCE_API_KEY` and `BINANCE_API_SECRET`
- Check IP whitelist on Binance
- Ensure system time is synchronized (NTP)
- Use testnet first

### CMake Build Errors
- Run `cmake --version` to check version
- Delete `build/` and rebuild
- Ensure all dependencies are installed

## Next Steps

- Add comprehensive unit tests
- Implement async PostgreSQL (libpq async)
- Add metrics/monitoring (Prometheus)
- Implement circuit breakers
- Add rate limiting
