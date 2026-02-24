#pragma once
#include "core/Types.hpp"

namespace tic {

    // EXECUTION INTERFACE
    // Interface that all execution layers must implement
    class IExecution {
        public:

        // Virtual destructor for proper cleanup of derived classes
        virtual ~IExecution() = default;

        // Connect to the exchange
        virtual void connect();

        // Disconnect to the exchange
        virtual void disconnect();

        // Execute an order on the exchange
        virtual void executeOrder(const Order& order);
    }
}