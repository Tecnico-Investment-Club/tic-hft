#pragma once
#include <string>
#include <cstdint>

namespace tic {
    
    // SIDE DEFINITION
    // Enum to represent the side of an order (BUY or SELL)
    enum class Side { BUY, SELL };

    // TICK DEFINITION
    // Internal market tick structure to the engine
    struct Tick {
        std::string symbol; // "BTCUSD"
        double price;       // 150.25
        double volume;      // 1000
        uint64_t timestamp; // 1627847265000 (milliseconds since epoch)
    };

    // ORDER DEFNITION
    // Internal representation of a order to execute from a strategy to the exchange
    struct Order {
        std::string symbol;  // "BTCUSD"        
        double price;        // 150.25  
        double quantity;     // 1000  
        Side side;           // BUY or SELL

        // Constructor
        Order(const std::string& symbol, double qty, double price, Side s)
            : symbol(symbol), quantity(qty), price(price), side(s) {}
    };
} // namespace tic