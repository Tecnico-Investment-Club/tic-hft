#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

namespace hft::position {

enum class Side {
    Buy,
    Sell
};

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

class PositionTracker {
public:
    void on_fill(const Fill& fill);

    std::optional<Position> get_position(const std::string& symbol) const;
    double get_total_realized_pnl() const;

private:
    std::unordered_map<std::string, Position> positions_;
};

} // namespace hft::position
