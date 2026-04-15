#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

// Import common types from core (for integration with other modules)
// Note: local types below are kept for backward compatibility
#include "../../core/include/CommonTypes.hpp"

namespace hft::risk {

enum class Side {
    Buy,
    Sell
};

struct OrderIntent {
    uint64_t event_id = 0;
    std::string symbol;
    Side side = Side::Buy;
    double quantity = 0.0;
    double price = 0.0;
};

struct PositionSnapshot {
    double quantity = 0.0;
    double avg_price = 0.0;
};

struct RiskDecision {
    bool approved = false;
    std::string reason;
    double adjusted_quantity = 0.0;
};

struct RiskLimits {
    double max_notional_per_order = 100000.0;
    double max_position_per_symbol = 10.0;
    double max_total_exposure = 800000.0;
    double max_daily_drawdown = -50000.0;
    int max_orders_per_second = 100;
};

class RiskEngine {
public:
    RiskEngine(double initial_capital = 1000000.0);

    RiskDecision validate(
        const OrderIntent& intent,
        const PositionSnapshot& current_pos,
        double unrealized_pnl = 0.0
    );

    void set_limits(const RiskLimits& limits);
    void set_max_notional(double value);
    void set_max_position(double value);
    void set_max_total_exposure(double value);
    void set_max_daily_drawdown(double value);

    void update_daily_pnl(double pnl_delta);
    void reset_daily_pnl();

    double get_available_capital() const;
    double get_daily_pnl() const;
    double get_total_exposure() const;

private:
    RiskLimits limits_;
    double initial_capital_;
    double daily_pnl_ = 0.0;
    std::unordered_map<std::string, double> position_map_;

    bool validate_notional(const OrderIntent& intent) const;
    bool validate_position(const OrderIntent& intent, const PositionSnapshot& current_pos) const;
    bool validate_exposure(const OrderIntent& intent, const PositionSnapshot& current_pos) const;
    bool validate_drawdown() const;
};

} // namespace hft::risk
