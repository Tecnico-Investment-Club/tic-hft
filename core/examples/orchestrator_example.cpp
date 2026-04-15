// ============================================================================
// HFT System Orchestrator Example
// ============================================================================
// This example demonstrates the complete integrated pipeline:
//   1. Position Tracker receives fills and maintains state
//   2. Risk Engine validates order intents
//   3. (Execution would send orders and receive fills back)
//
// Demonstrates type conversions between core:: types (for strategy logic) 
// and module-specific types (for backward compatibility).
// ============================================================================

#include "../include/CommonTypes.hpp"

// Include modules
#include "../../position/include/PositionTracker.hpp"
#include "../../risk/include/RiskEngine.hpp"

#include <iostream>
#include <iomanip>

namespace trading {

// ============================================================================
// Type Conversions: Bridge between core:: and module-specific types
// ============================================================================

// Convert core::Side to position::Side
inline hft::position::Side to_position_side(hft::core::Side s) {
    return (s == hft::core::Side::Buy) ? hft::position::Side::Buy 
                                       : hft::position::Side::Sell;
}

// Convert core::Side to risk::Side
inline hft::risk::Side to_risk_side(hft::core::Side s) {
    return (s == hft::core::Side::Buy) ? hft::risk::Side::Buy 
                                       : hft::risk::Side::Sell;
}

// Convert core::OrderIntent to risk::OrderIntent
inline hft::risk::OrderIntent to_risk_order_intent(const hft::core::OrderIntent& intent) {
    return {
        .event_id = intent.event_id,
        .symbol = intent.symbol,
        .side = to_risk_side(intent.side),
        .quantity = intent.quantity,
        .price = intent.price
    };
}

// Convert position::Position to risk::PositionSnapshot
inline hft::risk::PositionSnapshot to_risk_position_snapshot(const hft::position::Position& pos) {
    return {
        .quantity = pos.quantity,
        .avg_price = pos.avg_price
    };
}

// ============================================================================
// Mock Strategy: generates order intents using core types
// ============================================================================
class SimpleStrategy {
public:
    hft::core::OrderIntent generate_signal(
        const std::string& symbol,
        const hft::core::PositionSnapshot& current_pos
    ) {
        // Simple logic: if no position, buy; if long, sell 50%
        if (current_pos.quantity < 0.1) {
            return {
                .event_id = next_id_++,
                .symbol = symbol,
                .side = hft::core::Side::Buy,
                .quantity = 1.0,
                .price = 50000.0
            };
        } else {
            return {
                .event_id = next_id_++,
                .symbol = symbol,
                .side = hft::core::Side::Sell,
                .quantity = 0.5,
                .price = 51000.0
            };
        }
    }

private:
    uint64_t next_id_ = 1000;
};

// ============================================================================
// Mock Execution: accepts orders and simulates fills using position types
// ============================================================================
class MockExecution {
public:
    void send_order(const hft::core::OrderIntent& intent) {
        std::cout << "[Execution] Sending order: " << intent.symbol 
                  << " " << (intent.side == hft::core::Side::Buy ? "BUY" : "SELL")
                  << " " << intent.quantity << " @ $" << intent.price << "\n";
        
        // Create fill using position module types (what PositionTracker expects)
        last_fill_ = hft::position::Fill{
            .event_id = intent.event_id,
            .symbol = intent.symbol,
            .side = to_position_side(intent.side),
            .quantity = intent.quantity,
            .price = intent.price
        };
    }

    hft::position::Fill get_last_fill() const {
        return last_fill_;
    }

private:
    hft::position::Fill last_fill_;
};

// ============================================================================
// Main Orchestrator: coordinates all modules
// ============================================================================
class Orchestrator {
public:
    Orchestrator()
        : position_tracker_(),
          risk_engine_(1000000.0),
          strategy_(),
          execution_(),
          cycle_count_(0) {
        
        // Configure risk limits
        hft::risk::RiskLimits limits;
        limits.max_notional_per_order = 100000.0;
        limits.max_position_per_symbol = 10.0;
        risk_engine_.set_limits(limits);
    }

    void run_trading_cycle(const std::string& symbol) {
        std::cout << "\n" << std::string(70, '=') << "\n";
        std::cout << "Trading Cycle #" << (++cycle_count_) << " for: " << symbol << "\n";
        std::cout << std::string(70, '=') << "\n";

        // Step 1: Get current position from PositionTracker
        auto pos_opt = position_tracker_.get_position(symbol);
        double current_qty = pos_opt ? pos_opt->quantity : 0.0;
        double current_avg = pos_opt ? pos_opt->avg_price : 0.0;
        
        std::cout << "[Position] Current: qty=" << std::fixed << std::setprecision(2) 
                  << current_qty << " avg_price=$" << current_avg << "\n";

        // Step 2: Strategy generates intent (creates core:: types)
        // Create a core::PositionSnapshot for strategy input
        hft::core::PositionSnapshot strategy_pos{
            .quantity = current_qty,
            .avg_price = current_avg
        };
        auto intent = strategy_.generate_signal(symbol, strategy_pos);
        std::cout << "[Strategy] Generated intent: " << symbol
                  << " " << (intent.side == hft::core::Side::Buy ? "BUY" : "SELL")
                  << " " << intent.quantity << " @ $" << intent.price << "\n";

        // Step 3: Convert intent to risk:: types and validate
        auto risk_intent = to_risk_order_intent(intent);
        
        hft::risk::PositionSnapshot risk_pos{
            .quantity = current_qty,
            .avg_price = current_avg
        };
        
        auto risk_decision = risk_engine_.validate(risk_intent, risk_pos);
        std::cout << "[Risk] Decision: " << (risk_decision.approved ? "APPROVED" : "REJECTED");
        std::cout << " - " << risk_decision.reason << "\n";

        if (!risk_decision.approved) {
            std::cout << "[Orchestrator] Order blocked by risk gates.\n";
            return;
        }

        // Step 4: Execution sends order (creates position:: fills)
        execution_.send_order(intent);

        // Step 5: Position updates with fill (uses position:: types)
        auto fill = execution_.get_last_fill();
        position_tracker_.on_fill(fill);
        
        auto updated_pos_opt = position_tracker_.get_position(symbol);
        if (updated_pos_opt) {
            std::cout << "[Position] Updated with fill. New qty: " 
                      << updated_pos_opt->quantity << "\n";
        }

        // Step 6: Risk updates PnL (simulated)
        double simulated_pnl = fill.quantity * (fill.price * 0.001);
        risk_engine_.update_daily_pnl(simulated_pnl);
        
        print_status();
    }

    void print_status() {
        std::cout << "\n[Status Snapshot]\n";
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "  Available Capital: $" << risk_engine_.get_available_capital() << "\n";
        std::cout << "  Daily PnL: $" << risk_engine_.get_daily_pnl() << "\n";
        std::cout << "  Total Realized PnL: $" << position_tracker_.get_total_realized_pnl() << "\n";
    }

private:
    hft::position::PositionTracker position_tracker_;
    hft::risk::RiskEngine risk_engine_;
    SimpleStrategy strategy_;
    MockExecution execution_;
    int cycle_count_;
};

} // namespace trading

int main() {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "HFT System Orchestrator Example - E2E Pipeline\n";
    std::cout << "Demonstrates: Position -> Risk -> Execution\n";
    std::cout << "All modules working together through type conversions\n";
    std::cout << std::string(70, '=') << "\n";

    trading::Orchestrator orchestrator;

    // Simulate 3 trading cycles on two symbols
    std::cout << "\n--- Symbol: BTCUSDT ---\n";
    orchestrator.run_trading_cycle("BTCUSDT");
    orchestrator.run_trading_cycle("BTCUSDT");
    
    std::cout << "\n--- Symbol: ETHUSDT ---\n";
    orchestrator.run_trading_cycle("ETHUSDT");

    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "[OK] Example completed successfully!\n";
    std::cout << "[OK] All modules integrated and working together\n";
    std::cout << "[OK] Core types coordinating data flow through conversions\n";
    std::cout << "[OK] Backward compatibility preserved (modules use their own types)\n";
    std::cout << std::string(70, '=') << "\n\n";

    return 0;
}
