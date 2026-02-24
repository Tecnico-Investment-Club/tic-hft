#pragma once
#include "core/Types.hpp"

namespace tic {

    // RISK INTERFACE
    // Interface that all risk layers must implement
    class IRisk {
        public:

        // Virtual destructor for proper cleanup of derived classes
        virtual ~IRisk() = default;

        // Enforce maximum position size for a given symbol
        virtual bool enforceMaxPositionSize(const std::string& symbol, double quantity);

        // Enforce maximum order size for a given symbol
        virtual bool enforceMaxOrderSize(const std::string& symbol, double quantity);

        // Enforce maximum daily loss limit
        virtual bool enforceDailyLossLimit(const std::string& symbol, double loss);

        // Enforce buying power for a given symbol
        virtual bool enforceBuyingPower(const std::string& symbol, double quantity, double price);
    }
}