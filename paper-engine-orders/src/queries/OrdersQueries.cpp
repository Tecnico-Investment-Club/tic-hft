#include "queries/OrdersQueries.hpp"
#include <sstream>
#include <iomanip>
#include <ctime>
#include <chrono>

namespace hft::paper_engine {

std::string OrdersQueries::create_table() {
    return R"(
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
            
            PRIMARY KEY(portfolio_id, asset_id, order_ts),
            INDEX idx_event_id (event_id),
            INDEX idx_delivery_id (delivery_id),
            INDEX idx_status (status)
        );
        
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
    )";
}

std::string OrdersQueries::insert_order(const Order& order) {
    std::ostringstream ss;
    
    auto time_t_cast = std::chrono::system_clock::to_time_t(order.order_timestamp);
    auto gmt_time = std::gmtime(&time_t_cast);
    
    char ts_buffer[20];
    std::strftime(ts_buffer, sizeof(ts_buffer), "%Y-%m-%d %H:%M:%S", gmt_time);
    
    ss << "INSERT INTO " << TABLE_NAME << " ("
       << "portfolio_id, side, asset_id_type, asset_id, order_ts, "
       << "target_wgt, real_wgt, quantity, notional, hash, "
       << "event_id, delivery_id, status, binance_order_id"
       << ") VALUES ("
       << order.portfolio_id << ", '"
       << (order.side == OrderSide::BUY ? "BUY" : "SELL") << "', '"
       << (order.asset_type == AssetType::CRYPTO_TICKER ? "CRYPTO_TICKER" : "STOCK_TICKER") << "', '"
       << order.asset_id << "', '"
       << ts_buffer << "', "
       << order.target_weight << ", "
       << order.real_weight << ", "
       << order.quantity << ", "
       << order.notional << ", '"
       << order.hash << "', "
       << order.event_id << ", "
       << order.delivery_id << ", '"
       << "PENDING" << "', '"
       << order.binance_order_id << "'"
       << ") ON CONFLICT (event_id) DO NOTHING;";
    
    return ss.str();
}

std::string OrdersQueries::update_order_status(uint64_t event_id, OrderStatus status) {
    std::string status_str;
    switch (status) {
        case OrderStatus::NEW: status_str = "NEW"; break;
        case OrderStatus::PARTIALLY_FILLED: status_str = "PARTIALLY_FILLED"; break;
        case OrderStatus::FILLED: status_str = "FILLED"; break;
        case OrderStatus::CANCELED: status_str = "CANCELED"; break;
        case OrderStatus::REJECTED: status_str = "REJECTED"; break;
        case OrderStatus::ERROR: status_str = "ERROR"; break;
        default: status_str = "PENDING"; break;
    }
    
    std::ostringstream ss;
    ss << "UPDATE " << TABLE_NAME 
       << " SET status = '" << status_str << "', updated_at = CURRENT_TIMESTAMP "
       << " WHERE event_id = " << event_id << ";";
    
    return ss.str();
}

std::string OrdersQueries::select_pending_orders() {
    std::ostringstream ss;
    ss << "SELECT * FROM " << TABLE_NAME 
       << " WHERE status = 'PENDING' OR status = 'NEW' "
       << " ORDER BY order_ts ASC;";
    return ss.str();
}

std::string OrdersQueries::select_order_by_id(uint64_t event_id) {
    std::ostringstream ss;
    ss << "SELECT * FROM " << TABLE_NAME 
       << " WHERE event_id = " << event_id << ";";
    return ss.str();
}

std::string OrdersQueries::select_orders_by_portfolio(uint64_t portfolio_id) {
    std::ostringstream ss;
    ss << "SELECT * FROM " << TABLE_NAME 
       << " WHERE portfolio_id = " << portfolio_id 
       << " ORDER BY order_ts DESC;";
    return ss.str();
}

std::string OrdersQueries::order_to_values(const Order& order) {
    std::ostringstream ss;
    auto time_t_cast = std::chrono::system_clock::to_time_t(order.order_timestamp);
    auto gmt_time = std::gmtime(&time_t_cast);
    
    char ts_buffer[20];
    std::strftime(ts_buffer, sizeof(ts_buffer), "%Y-%m-%d %H:%M:%S", gmt_time);
    
    ss << "("
       << order.portfolio_id << ", '"
       << (order.side == OrderSide::BUY ? "BUY" : "SELL") << "', '"
       << (order.asset_type == AssetType::CRYPTO_TICKER ? "CRYPTO_TICKER" : "STOCK_TICKER") << "', '"
       << order.asset_id << "', '"
       << ts_buffer << "', "
       << order.target_weight << ", "
       << order.real_weight << ", "
       << order.quantity << ", "
       << order.notional << ", '"
       << order.hash << "', "
       << order.event_id << ", "
       << order.delivery_id
       << ")";
    
    return ss.str();
}

} // namespace hft::paper_engine
