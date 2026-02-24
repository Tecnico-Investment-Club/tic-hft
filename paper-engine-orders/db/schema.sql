-- Schema and tables for HFT Paper Engine Orders
-- Mirrors the structure of tic-strategy-app/alpaca/paper-engine-orders

CREATE SCHEMA IF NOT EXISTS hft_paper_engine;

CREATE TABLE IF NOT EXISTS hft_paper_engine.orders (
    portfolio_id BIGINT NOT NULL,
    side VARCHAR(20) NOT NULL,
    asset_id_type VARCHAR(20) NOT NULL,
    asset_id VARCHAR(20) NOT NULL,
    order_ts TIMESTAMP NOT NULL,
    target_wgt DECIMAL(10,4),
    real_wgt DECIMAL(10,4),
    quantity DECIMAL(24,4) NOT NULL,
    notional DECIMAL(24,4),
    
    hash VARCHAR(256),
    binance_order_id VARCHAR(50),
    status VARCHAR(20) DEFAULT 'PENDING',
    
    event_id BIGINT NOT NULL UNIQUE,
    delivery_id BIGINT NOT NULL,
    
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    PRIMARY KEY(portfolio_id, asset_id, order_ts)
);

CREATE INDEX IF NOT EXISTS idx_event_id ON hft_paper_engine.orders(event_id);
CREATE INDEX IF NOT EXISTS idx_delivery_id ON hft_paper_engine.orders(delivery_id);
CREATE INDEX IF NOT EXISTS idx_status ON hft_paper_engine.orders(status);
CREATE INDEX IF NOT EXISTS idx_order_ts ON hft_paper_engine.orders(order_ts);

CREATE TABLE IF NOT EXISTS hft_paper_engine.orders_config (
    portfolio_id BIGINT NOT NULL PRIMARY KEY,
    strategy_id INT NOT NULL,
    cash_allocation DECIMAL(24,4) NOT NULL,
    rebalance_frequency VARCHAR(50),
    
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS hft_paper_engine.orders_control (
    portfolio_id BIGINT NOT NULL PRIMARY KEY,
    last_rebalance TIMESTAMP,
    total_orders INT DEFAULT 0,
    filled_orders INT DEFAULT 0,
    
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS hft_paper_engine.orders_latest (
    portfolio_id BIGINT NOT NULL PRIMARY KEY,
    asset_id VARCHAR(20),
    last_order_ts TIMESTAMP,
    last_status VARCHAR(20),
    
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
