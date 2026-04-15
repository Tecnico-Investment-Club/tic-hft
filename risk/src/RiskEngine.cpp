#include "RiskEngine.hpp"

#include <cmath>

namespace hft::risk {

RiskEngine::RiskEngine(double initial_capital)
    : initial_capital_(initial_capital) {
}

void RiskEngine::set_limits(const RiskLimits& limits) {
    limits_ = limits;
}

void RiskEngine::set_max_notional(double value) {
    limits_.max_notional_per_order = value;
}

void RiskEngine::set_max_position(double value) {
    limits_.max_position_per_symbol = value;
}

void RiskEngine::set_max_total_exposure(double value) {
    limits_.max_total_exposure = value;
}

void RiskEngine::set_max_daily_drawdown(double value) {
    limits_.max_daily_drawdown = value;
}

void RiskEngine::update_daily_pnl(double pnl_delta) {
    daily_pnl_ += pnl_delta;
}

void RiskEngine::reset_daily_pnl() {
    daily_pnl_ = 0.0;
}

double RiskEngine::get_available_capital() const {
    return initial_capital_ + daily_pnl_;
}

double RiskEngine::get_daily_pnl() const {
    return daily_pnl_;
}

double RiskEngine::get_total_exposure() const {
    double total = 0.0;
    for (const auto& [_, qty] : position_map_) {
        total += std::abs(qty);
    }
    return total;
}

RiskDecision RiskEngine::validate(
    const OrderIntent& intent,
    const PositionSnapshot& current_pos,
    double unrealized_pnl
) {
    RiskDecision decision;

    if (intent.symbol.empty() || intent.quantity <= 0.0 || intent.price <= 0.0) {
        decision.approved = false;
        decision.reason = "Invalid order parameters (empty symbol, qty <= 0, or price <= 0)";
        return decision;
    }

    if (!validate_notional(intent)) {
        decision.approved = false;
        decision.reason = "Order notional exceeds max limit";
        return decision;
    }

    if (!validate_position(intent, current_pos)) {
        decision.approved = false;
        decision.reason = "New position would exceed max position limit per symbol";
        return decision;
    }

    if (!validate_exposure(intent, current_pos)) {
        decision.approved = false;
        decision.reason = "Total exposure would exceed max allowed";
        return decision;
    }

    if (!validate_drawdown()) {
        decision.approved = false;
        decision.reason = "Daily drawdown limit exceeded";
        return decision;
    }

    decision.approved = true;
    decision.reason = "Order approved";
    decision.adjusted_quantity = intent.quantity;
    return decision;
}

bool RiskEngine::validate_notional(const OrderIntent& intent) const {
    const double notional = intent.quantity * intent.price;
    return notional <= limits_.max_notional_per_order;
}

bool RiskEngine::validate_position(const OrderIntent& intent, const PositionSnapshot& current_pos) const {
    const double fill_qty_signed = (intent.side == Side::Buy) ? intent.quantity : -intent.quantity;
    const double new_qty = current_pos.quantity + fill_qty_signed;

    return std::abs(new_qty) <= limits_.max_position_per_symbol;
}

bool RiskEngine::validate_exposure(const OrderIntent& intent, const PositionSnapshot& current_pos) const {
    const double fill_qty_signed = (intent.side == Side::Buy) ? intent.quantity : -intent.quantity;
    const double new_qty = current_pos.quantity + fill_qty_signed;

    double total_exposure = get_total_exposure();
    total_exposure += std::abs(new_qty);

    return total_exposure <= limits_.max_total_exposure;
}

bool RiskEngine::validate_drawdown() const {
    return daily_pnl_ >= limits_.max_daily_drawdown;
}

} // namespace hft::risk
