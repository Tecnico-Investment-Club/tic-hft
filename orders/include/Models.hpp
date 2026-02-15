#pragma once

#include <ctime>
#include <string>
#include <cstdint>
#include <chrono>

namespace hft::orders {

/**
 * @enum OrderStatus
 * Represents the status of an order throughout its lifecycle
 */
enum class OrderStatus : std::uint8_t {
    PENDING = 0,            // Waiting to be sent to exchange
    NEW = 1,                // Sent to exchange, awaiting acknowledgment
    PARTIALLY_FILLED = 2,   // Partially executed
    FILLED = 3,             // Fully executed
    CANCELED = 4,           // Cancelled by user
    REJECTED = 5,           // Rejected by exchange
    EXPIRED = 6,            // Order expired
    ERROR = 7               // Error occurred
};

/**
 * @enum OrderSide
 * Represents the direction of the order
 */
enum class OrderSide : std::uint8_t {
    BUY = 0,
    SELL = 1
};

/**
 * @enum OrderType
 * Type of order execution
 */
enum class OrderType : std::uint8_t {
    LIMIT = 0,
    MARKET = 1
};

/**
 * @struct Order
 * Represents a single trading order
 */
struct Order {
    std::uint64_t order_id;              // Internal order ID
    std::uint64_t binance_order_id = 0;  // Binance order ID (set after placement)
    
    std::string symbol;                  // Trading pair (e.g., "BTCUSDT")
    OrderSide side;                      // BUY or SELL
    OrderType type;                      // LIMIT or MARKET
    
    double quantity;                     // Order quantity
    double price;                        // Limit price (0 for market orders)
    
    double filled_quantity = 0.0;        // Amount filled
    double avg_price = 0.0;              // Average fill price
    
    OrderStatus status = OrderStatus::PENDING;
    
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point updated_at;
    
    Order() : created_at(std::chrono::system_clock::now()), 
              updated_at(std::chrono::system_clock::now()) {}
};

/**
 * @struct OrderLatest
 * Latest state snapshot for fast lookups
 */
struct OrderLatest {
    std::uint64_t order_id;
    std::string symbol;
    OrderSide side;
    double filled_quantity;
    double avg_price;
    OrderStatus status;
    std::chrono::system_clock::time_point updated_at;
};

} // namespace hft::orders
