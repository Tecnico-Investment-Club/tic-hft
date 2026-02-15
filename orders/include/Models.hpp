#pragma once

#include <string>
#include <cstdint>
#include <chrono>

namespace hft::orders {

enum class OrderStatus : uint8_t {
    PENDING = 0,
    NEW = 1,
    PARTIALLY_FILLED = 2,
    FILLED = 3,
    CANCELED = 4,
    REJECTED = 5,
    EXPIRED = 6,
    ERROR = 7
};

enum class OrderSide : uint8_t {
    BUY = 0,
    SELL = 1
};

enum class OrderType : uint8_t {
    LIMIT = 0,
    MARKET = 1
};

struct Order {
    uint64_t order_id;
    uint64_t binance_order_id = 0;
    
    std::string symbol;
    OrderSide side;
    OrderType type;
    
    double quantity;
    double price;
    
    double filled_quantity = 0.0;
    double avg_price = 0.0;
    
    OrderStatus status = OrderStatus::PENDING;
    
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point updated_at;
    
    Order() : created_at(std::chrono::system_clock::now()), 
              updated_at(std::chrono::system_clock::now()) {}
};

} // namespace hft::orders
