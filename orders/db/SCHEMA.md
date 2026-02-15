# Database Schema Documentation

## Overview

The HFT Orders service uses PostgreSQL to persist order data with the following schema.

## Schema: `hft_orders`

### Table: `orders`

Main table storing all order information.

```sql
CREATE TABLE hft_orders.orders (
    order_id BIGINT PRIMARY KEY,
    symbol VARCHAR(20) NOT NULL,
    side VARCHAR(4) NOT NULL,
    type VARCHAR(10) NOT NULL,
    quantity DECIMAL(18,8) NOT NULL,
    price DECIMAL(18,8),
    status VARCHAR(20) NOT NULL DEFAULT 'PENDING',
    binance_order_id BIGINT,
    filled_quantity DECIMAL(18,8) DEFAULT 0,
    avg_price DECIMAL(18,8) DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

#### Columns

| Column | Type | Nullable | Description |
|--------|------|----------|-------------|
| `order_id` | BIGINT | NO | Primary key - internal order ID |
| `symbol` | VARCHAR(20) | NO | Trading pair (e.g., BTCUSDT) |
| `side` | VARCHAR(4) | NO | BUY or SELL |
| `type` | VARCHAR(10) | NO | LIMIT or MARKET |
| `quantity` | DECIMAL(18,8) | NO | Order quantity in base asset |
| `price` | DECIMAL(18,8) | YES | Limit price (NULL for market orders) |
| `status` | VARCHAR(20) | NO | Current order status |
| `binance_order_id` | BIGINT | YES | Binance's order ID (set after placement) |
| `filled_quantity` | DECIMAL(18,8) | NO | Amount filled so far |
| `avg_price` | DECIMAL(18,8) | NO | Average fill price |
| `created_at` | TIMESTAMP | NO | When order was created |
| `updated_at` | TIMESTAMP | NO | Last update time |

#### Index

```sql
CREATE INDEX idx_orders_status ON hft_orders.orders(status);
CREATE INDEX idx_orders_symbol ON hft_orders.orders(symbol);
CREATE INDEX idx_orders_created_at ON hft_orders.orders(created_at);
```

### Order Statuses

| Status | Description |
|--------|-------------|
| `PENDING` | Order created, waiting to be sent to exchange |
| `NEW` | Order sent to exchange, acknowledged |
| `PARTIALLY_FILLED` | Partially executed |
| `FILLED` | Fully executed |
| `CANCELED` | Cancelled by user/system |
| `REJECTED` | Rejected by exchange |
| `EXPIRED` | Order expired |
| `ERROR` | An error occurred |

### Order Sides

- `BUY` - Buy order
- `SELL` - Sell order

### Order Types

- `LIMIT` - Limit order with specified price
- `MARKET` - Market order executed at best available price

## Queries

### Get Pending Orders
```sql
SELECT * FROM hft_orders.orders 
WHERE status = 'PENDING' 
ORDER BY created_at ASC;
```

### Get Active Orders
```sql
SELECT * FROM hft_orders.orders 
WHERE status IN ('NEW', 'PARTIALLY_FILLED') 
ORDER BY created_at ASC;
```

### Get Order Statistics
```sql
SELECT 
    symbol,
    COUNT(*) as total_orders,
    SUM(CASE WHEN status = 'FILLED' THEN 1 ELSE 0 END) as filled,
    AVG(CASE WHEN status = 'FILLED' THEN avg_price END) as avg_fill_price
FROM hft_orders.orders
GROUP BY symbol;
```

### Get Daily Volume
```sql
SELECT 
    DATE(created_at) as date,
    symbol,
    SUM(quantity) as total_quantity,
    SUM(filled_quantity) as total_filled
FROM hft_orders.orders
WHERE status IN ('FILLED', 'PARTIALLY_FILLED')
GROUP BY DATE(created_at), symbol
ORDER BY date DESC;
```

## Migration / Setup

The schema is automatically created on first run by `Database::ensure_schema()`:

```cpp
db.ensure_schema();  // Creates schema and tables
```

Or manually:
```bash
psql -d tic -f db/SCHEMA.md
```

## Performance Considerations

### Connection Pooling
Currently uses single connection. For high-load scenarios, implement connection pooling:
- PQconnectPoll (async connections)
- Connection pool library

### Batch Operations
For multiple orders, consider batch inserts:
```sql
INSERT INTO hft_orders.orders (order_id, symbol, ...) VALUES
(1, 'BTCUSDT', ...),
(2, 'ETHUSDT', ...),
...
```

### Data Retention
Consider archiving old orders:
```sql
-- Archive orders older than 90 days
INSERT INTO hft_orders.orders_archive 
SELECT * FROM hft_orders.orders 
WHERE created_at < CURRENT_DATE - INTERVAL '90 days';

DELETE FROM hft_orders.orders 
WHERE created_at < CURRENT_DATE - INTERVAL '90 days';
```

### Monitoring Queries

#### Pending Orders Age
```sql
SELECT order_id, symbol, AGE(NOW(), created_at) as pending_time
FROM hft_orders.orders
WHERE status = 'PENDING'
ORDER BY created_at ASC;
```

#### Fill Rate
```sql
SELECT 
    DATE_TRUNC('hour', created_at) as hour,
    COUNT(*) as total,
    SUM(CASE WHEN status = 'FILLED' THEN 1 ELSE 0 END) as filled,
    ROUND(100.0 * SUM(CASE WHEN status = 'FILLED' THEN 1 ELSE 0 END) / COUNT(*), 2) as fill_rate
FROM hft_orders.orders
GROUP BY DATE_TRUNC('hour', created_at)
ORDER BY hour DESC;
```

## Future Enhancements

- Add `order_sequence` for market order tracking
- Add `slippage` column to track execution slippage
- Add `commission` column for fee tracking
- Partition by date for large tables
- Add `order_notes` JSONB column for metadata
