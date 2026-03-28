#include "PositionTracker.hpp"

#include <cassert>
#include <cmath>

using namespace hft::position;

int main() {
    PositionTracker tracker;

    tracker.on_fill(Fill{.event_id = 1, .symbol = "BTCUSDT", .side = Side::Buy, .quantity = 1.0, .price = 100.0});
    tracker.on_fill(Fill{.event_id = 2, .symbol = "BTCUSDT", .side = Side::Sell, .quantity = 0.4, .price = 120.0});

    auto pos = tracker.get_position("BTCUSDT");
    assert(pos.has_value());
    assert(std::abs(pos->quantity - 0.6) < 1e-9);
    assert(std::abs(pos->realized_pnl - 8.0) < 1e-9);

    return 0;
}
