#pragma once

#include <string>
#include <cstdint>
#include <chrono>
#include <map>

namespace hft::paper_engine {

enum class OrderSide : uint8_t {
    BUY = 0,
    SELL = 1
};

enum class OrderStatus : uint8_t {
    PENDING = 0,
    NEW = 1,
    PARTIALLY_FILLED = 2,
    FILLED = 3,
    CANCELED = 4,
    REJECTED = 5,
    ERROR = 6
};

enum class AssetType {
    CRYPTO_TICKER,
    STOCK_TICKER
};

/**
 * Order: Represents an order in Paper Engine
 * Mirrors the structure of paper-engine-orders in tic-strategy-app/alpaca
 */
struct Order {
    // Unique identifiers
    uint64_t portfolio_id = 0;
    uint64_t event_id = 0;
    uint64_t delivery_id = 0;
    
    // Order details
    OrderSide side = OrderSide::BUY;
    std::string asset_id;  // ticker symbol
    AssetType asset_type = AssetType::CRYPTO_TICKER;
    
    // Quantities and prices
    double quantity = 0.0;
    double notional = 0.0;  // quantity * price
    double target_weight = 0.0;
    double real_weight = 0.0;
    
    // Status
    OrderStatus status = OrderStatus::PENDING;
    
    // Timestamps
    std::chrono::system_clock::time_point order_timestamp;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point updated_at;
    
    // Hash for integrity
    std::string hash;
    
    // Binance order ID if submitted
    std::string binance_order_id;
    
    Order() 
        : created_at(std::chrono::system_clock::now()),
          updated_at(std::chrono::system_clock::now()),
          order_timestamp(std::chrono::system_clock::now()) {}
};

using OrderMap = std::map<uint64_t, Order>;

} // namespace hft::paper_engine
