#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace hft::core {

// ============================================================================
// Basic enums
// ============================================================================

enum class Side {
    Buy,
    Sell
};

// ============================================================================
// Data flow: ETL -> Position Tracker
// ============================================================================

struct Fill {
    uint64_t event_id = 0;
    std::string symbol;
    Side side = Side::Buy;
    double quantity = 0.0;
    double price = 0.0;
};

struct Position {
    std::string symbol;
    double quantity = 0.0;
    double avg_price = 0.0;
    double realized_pnl = 0.0;
};

struct PositionSnapshot {
    double quantity = 0.0;
    double avg_price = 0.0;
};

// ============================================================================
// Data flow: Strategy -> Risk Engine -> Execution
// ============================================================================

struct OrderIntent {
    uint64_t event_id = 0;
    std::string symbol;
    Side side = Side::Buy;
    double quantity = 0.0;
    double price = 0.0;
};

struct RiskDecision {
    bool approved = false;
    std::string reason;
    double adjusted_quantity = 0.0;
};

// ============================================================================
// Data flow: Execution -> Exchange & back
// ============================================================================

struct Order {
    uint64_t portfolio_id = 0;
    uint64_t event_id = 0;
    uint64_t delivery_id = 0;
    std::string asset_id;
    double quantity = 0.0;
    double price = 0.0;
    std::string side;  // "BUY" ou "SELL"
};

struct OrderStatus {
    bool is_valid = false;
    std::string reason;
};

// ============================================================================
// Consolidated state & metrics (for monitoring)
// ============================================================================

struct PortfolioSnapshot {
    double total_capital = 0.0;
    double available_capital = 0.0;
    double daily_pnl = 0.0;
    double total_realized_pnl = 0.0;
    double total_unrealized_pnl = 0.0;
    double total_exposure = 0.0;
    std::vector<Position> positions;
};

} // namespace hft::core
