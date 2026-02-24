#pragma once

#include "core/Types.hpp"
#include "pod/strategy/IStrategy.hpp"
#include "execution/IExecution.hpp"
#include "risk/IRisk.hpp"

namespace pod {
    class TradingPod {
    public:
        // Attributes
        const IStrategy& strategy;
        const IRisk& risk;
        const IExecution& execution;

        // Constructor
        TradingPod(const IStrategy& strategy, const IExecution& execution, const IRisk& risk)
            : strategy(strategy), execution(execution), risk(risk) {}
        
        // Destructor
        ~TradingPod() = default;

        // Methods
        void execStrategy();
        void execRisk();
        void execTrade();
    };
}