# HFT Orders - Examples

## Building the Project

### Quick Start

```bash
# Clone and navigate
cd tic-hft/orders

# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
make -j4

# Verify
./hft_orders --help
```

### Output
```
=== HFT Orders Manager ===
Version 0.1.0

HFT Orders Manager

Usage: hft_orders [options]

Options:
  --dry-run           Run in dry-run mode (no real orders)
  --min-sleep MS      Minimum sleep between iterations
  --max-sleep MS      Maximum sleep between iterations
  --db CONNECTION     Database connection string
  --help              Show this help message

Environment Variables:
  DATABASE_URL        PostgreSQL connection string
  BINANCE_API_KEY     Binance API key
  BINANCE_API_SECRET  Binance API secret
  DRY_RUN             Set to 'true' for dry-run mode
  MIN_SLEEP           Min sleep in milliseconds
  MAX_SLEEP           Max sleep in milliseconds
```

## Configuration Examples

### Dry-Run Mode (Testing)

```bash
# Set environment
export DRY_RUN=true
export DATABASE_URL="postgresql://tic_strat_app:imaginemterumadb123@localhost:5432/tic"

# Run
./hft_orders

# Output:
# ==========================================================
# Configuration:
#   Database: postgresql://tic_strat_app:...@localhost:...
#   API Key: NOT SET
#   Dry Run: YES
#   Min Sleep: 1ms
#   Max Sleep: 5ms
# 
# Starting orders manager...
# OrdersManager initialized (dry_run=true)
# Database connected successfully
# Database schema ensured
# BinanceClient initialized (testnet=true)
# Main loop started
# =========================================================
```

### Testnet Mode

```bash
# Use Binance testnet
export BINANCE_API_KEY="testnet_key"
export BINANCE_API_SECRET="testnet_secret"
export DRY_RUN=false
export DATABASE_URL="postgresql://..."

./hft_orders
```

### Production Mode

```bash
# Real trading (be careful!)
export BINANCE_API_KEY="your_real_key"
export BINANCE_API_SECRET="your_real_secret"
export DRY_RUN=false
export DATABASE_URL="postgresql://tic_strat_app:pass@prod-server:5432/tic"
export MIN_SLEEP=0    # Check more frequently
export MAX_SLEEP=100  # But not too frequently

./hft_orders
```

## Docker Examples

### Build and Run Locally

```bash
# Build
docker build -t tic-hft-orders .

# Run in dry-run mode
docker run \
  --env DRY_RUN=true \
  --env DATABASE_URL="postgresql://tic_strat_app:pass@host.docker.internal:5432/tic" \
  tic-hft-orders
```

### Using Docker Compose

```bash
# Copy environment
cp .env.example .env

# Edit .env with your credentials
nano .env

# Start services
docker-compose up -d

# View logs
docker-compose logs -f hft-orders

# Stop services
docker-compose down
```

## Database Examples

### Pre-creating Orders (for testing)

```sql
-- Insert test orders
INSERT INTO hft_orders.orders (
    order_id, symbol, side, type, quantity, price, status
) VALUES
(1001, 'BTCUSDT', 'BUY', 'LIMIT', 0.1, 50000.00, 'PENDING'),
(1002, 'ETHUSDT', 'SELL', 'LIMIT', 1.0, 3000.00, 'PENDING'),
(1003, 'BNBUSDT', 'BUY', 'MARKET', 10.0, NULL, 'PENDING');

-- View pending orders
SELECT * FROM hft_orders.orders WHERE status = 'PENDING';
```

### Monitoring Orders (while running)

```bash
# Terminal 1: Run orders manager
./hft_orders

# Terminal 2: Monitor orders in real-time
watch 'psql -d tic -c "SELECT order_id, symbol, side, status, filled_quantity FROM hft_orders.orders ORDER BY created_at DESC LIMIT 10;"'
```

### Query Historical Data

```sql
-- Get all filled orders from last hour
SELECT * FROM hft_orders.orders 
WHERE status = 'FILLED' 
  AND updated_at > NOW() - INTERVAL '1 hour'
ORDER BY updated_at DESC;

-- Get fill statistics
SELECT 
    symbol,
    COUNT(*) as total,
    SUM(filled_quantity) as filled,
    AVG(avg_price) as avg_price
FROM hft_orders.orders
WHERE status = 'FILLED'
GROUP BY symbol;

-- Get pending orders older than 5 minutes
SELECT order_id, symbol, AGE(NOW(), created_at) 
FROM hft_orders.orders
WHERE status = 'PENDING' 
  AND created_at < NOW() - INTERVAL '5 minutes';
```

## Integration with HFT-ETL

The orders manager is designed to work with the HFT-ETL C++ engine:

```cpp
// In your strategy code (etl/src/transform/...)
#include "OrdersManager.hpp"

// When you generate a trading signal:
hft::orders::Order order;
order.order_id = signal.order_id;
order.symbol = "BTCUSDT";
order.side = signal.side;
order.type = hft::orders::OrderType::LIMIT;
order.quantity = signal.quantity;
order.price = signal.price;

orders_manager->submit_order(order);
```

## Performance Testing

### Load Test (Dry-Run)

```bash
# Build a test program to generate orders
cat > test_load.cpp << 'EOF'
#include "../include/OrdersManager.hpp"
#include <chrono>

using namespace hft::orders;

int main() {
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; ++i) {
        Order order;
        order.order_id = i;
        order.symbol = "BTCUSDT";
        order.side = OrderSide::BUY;
        order.quantity = 0.1;
        order.price = 50000.0;
        
        // Process order (measure time)
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Processed 1000 orders in " << duration.count() << "µs" << std::endl;
    std::cout << "Avg: " << (double)duration.count() / 1000 << "µs per order" << std::endl;
}
EOF

g++ -std=c++20 test_load.cpp -o test_load && ./test_load
```

## Troubleshooting

### Connection Failed

```bash
# Check PostgreSQL is running
psql -U tic_strat_app -c '\l'

# Verify credentials in DATABASE_URL
# Format: postgresql://user:pass@host:port/database
```

### API Errors

```bash
# Enable verbose output (add logging to code)
# Check Binance API status at status.binance.com

# Verify testnet credentials work
curl -H "X-MBX-APIKEY: $BINANCE_API_KEY" \
  "https://testnet.binance.vision/api/v3/account"
```

### High Latency

```bash
# Reduce sleep times
./hft_orders --min-sleep 0 --max-sleep 10

# Monitor network latency to Binance
ping api.binance.com

# Check PostgreSQL latency
time psql -c "SELECT 1;"
```

## Next Steps

1. **Deploy to staging** with small position sizes
2. **Monitor metrics** (fills, rejections, latency)
3. **Optimize for your use case** (adjust sleep times, batch sizes)
4. **Add monitoring/alerting** (Prometheus, Grafana)
5. **Scale horizontally** if needed (multiple managers, load balancing)

---

For more details, see [DEVELOPMENT.md](DEVELOPMENT.md) and [README.md](README.md).
