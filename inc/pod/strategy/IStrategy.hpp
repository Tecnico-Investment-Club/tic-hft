#pragma once
#include "core/Types.hpp"
#include <vector>

namespace tic {

    // STRATEGY INTERFACE
    // Interface that all trading strategies must implement
    class IStrategy {
    public:
        // Store historical ticks for the strategy to analyze
        std::vector<Tick> tickHistory;

        // Virtual destructor for proper cleanup of derived classes
        virtual ~IStrategy() = default;

        // Method to process incoming market ticks
        virtual void onTick(const Tick& tick) = 0;

        // Method to generate a array of orders based on the strategy's logic
        virtual std::vector<Order> generateOrders() = 0;
    };

}