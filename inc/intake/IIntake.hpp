#pragma once
#include "core/Types.hpp"

namespace tic {

    // INTAKE INTERFACE
    // Interface that all intake layers must implement
    class IIntake {
        public:

        // Virtual destructor for proper cleanup of derived classes
        virtual ~IIntake() = default;

        // Connect to the exchange
        virtual void connect();

        // Disconnect to the exchange
        virtual void disconnect();

        // Get data from the exchange
        virtual void getData();
    }
}