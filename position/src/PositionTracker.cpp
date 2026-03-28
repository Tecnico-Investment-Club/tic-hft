#include "PositionTracker.hpp"

#include <cmath>

namespace hft::position {

namespace {

double signed_quantity(Side side, double quantity) {
    return side == Side::Buy ? quantity : -quantity;
}

} // namespace

void PositionTracker::on_fill(const Fill& fill) {
    if (fill.symbol.empty() || fill.quantity <= 0.0 || fill.price <= 0.0) {
        return;
    }

    auto& pos = positions_[fill.symbol];
    pos.symbol = fill.symbol;

    const double fill_qty_signed = signed_quantity(fill.side, fill.quantity);
    const double old_qty = pos.quantity;
    const double new_qty = old_qty + fill_qty_signed;

    if (std::abs(old_qty) < 1e-12 || (old_qty > 0.0 && fill_qty_signed > 0.0) || (old_qty < 0.0 && fill_qty_signed < 0.0)) {
        const double old_notional = pos.avg_price * std::abs(old_qty);
        const double fill_notional = fill.price * std::abs(fill_qty_signed);
        const double total_qty = std::abs(old_qty) + std::abs(fill_qty_signed);

        pos.avg_price = total_qty > 0.0 ? (old_notional + fill_notional) / total_qty : 0.0;
        pos.quantity = new_qty;
        return;
    }

    const double closing_qty = std::min(std::abs(fill_qty_signed), std::abs(old_qty));

    if (old_qty > 0.0) {
        pos.realized_pnl += closing_qty * (fill.price - pos.avg_price);
    } else {
        pos.realized_pnl += closing_qty * (pos.avg_price - fill.price);
    }

    pos.quantity = new_qty;

    if (std::abs(new_qty) < 1e-12) {
        pos.quantity = 0.0;
        pos.avg_price = 0.0;
        return;
    }

    if ((old_qty > 0.0 && new_qty < 0.0) || (old_qty < 0.0 && new_qty > 0.0)) {
        pos.avg_price = fill.price;
    }
}

std::optional<Position> PositionTracker::get_position(const std::string& symbol) const {
    auto it = positions_.find(symbol);
    if (it == positions_.end()) {
        return std::nullopt;
    }

    return it->second;
}

double PositionTracker::get_total_realized_pnl() const {
    double total = 0.0;
    for (const auto& [_, pos] : positions_) {
        total += pos.realized_pnl;
    }
    return total;
}

} // namespace hft::position
