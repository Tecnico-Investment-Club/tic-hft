#include "RiskEngine.hpp"

#include <cassert>
#include <iostream>
#include <iomanip>

using namespace hft::risk;

void print_test(const std::string& name, bool passed) {
    std::cout << "[" << (passed ? "PASS" : "FAIL") << "] " << name << "\n";
}

int main() {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "Risk Engine Validation Tests\n";
    std::cout << std::string(70, '=') << "\n\n";

    RiskEngine engine(1000000.0);
    RiskLimits limits;
    limits.max_notional_per_order = 100000.0;
    limits.max_position_per_symbol = 10.0;
    engine.set_limits(limits);

    PositionSnapshot pos{.quantity = 0.0, .avg_price = 0.0};

    // ================================================================
    // APPROVED ORDERS
    // ================================================================
    std::cout << "--- APPROVED ORDERS ---\n\n";

    OrderIntent order1{
        .event_id = 1,
        .symbol = "BTCUSDT",
        .side = Side::Buy,
        .quantity = 1.0,
        .price = 50000.0
    };

    auto decision1 = engine.validate(order1, pos);
    assert(decision1.approved);
    assert(decision1.reason == "Order approved");
    print_test("Test 1: Valid order BUY 1 BTC @ $50k = $50k (approved)", decision1.approved);

    OrderIntent order2{
        .event_id = 2,
        .symbol = "ETHUSDT",
        .side = Side::Buy,
        .quantity = 5.0,
        .price = 3000.0
    };

    auto decision2 = engine.validate(order2, pos);
    assert(decision2.approved);
    print_test("Test 2: Valid order BUY 5 ETH @ $3k = $15k (approved)", decision2.approved);

    // ================================================================
    // REJECTED: NOTIONAL LIMIT
    // ================================================================
    std::cout << "\n--- REJECTED: NOTIONAL LIMIT ($100k max) ---\n\n";

    OrderIntent order3{
        .event_id = 3,
        .symbol = "BNBUSDT",
        .side = Side::Buy,
        .quantity = 1000.0,
        .price = 150.0
    };

    auto decision3 = engine.validate(order3, pos);
    assert(!decision3.approved);
    assert(decision3.reason.find("notional") != std::string::npos);
    print_test("Test 3: BUY 1000 BNB @ $150 = $150k (REJECTED - exceeds $100k)", !decision3.approved);
    std::cout << "        Reason: " << decision3.reason << "\n";

    OrderIntent order3b{
        .event_id = 3,
        .symbol = "BNBUSDT",
        .side = Side::Buy,
        .quantity = 500.0,
        .price = 150.0
    };

    auto decision3b = engine.validate(order3b, pos);
    assert(decision3b.approved);
    print_test("Test 3b: BUY 500 BNB @ $150 = $75k (approved - within limit)", decision3b.approved);

    // ================================================================
    // REJECTED: POSITION LIMIT
    // ================================================================
    std::cout << "\n--- REJECTED: POSITION LIMIT (10 BTC max per symbol) ---\n\n";

    PositionSnapshot pos_long{.quantity = 8.0, .avg_price = 50000.0};

    OrderIntent order4{
        .event_id = 4,
        .symbol = "BTCUSDT",
        .side = Side::Buy,
        .quantity = 5.0,
        .price = 52000.0
    };

    auto decision4 = engine.validate(order4, pos_long);
    assert(!decision4.approved);
    assert(decision4.reason.find("position") != std::string::npos);
    print_test("Test 4: Current pos 8 BTC + BUY 5 = 13 BTC (REJECTED - max 10)", !decision4.approved);
    std::cout << "        Reason: " << decision4.reason << "\n";

    OrderIntent order4b{
        .event_id = 4,
        .symbol = "BTCUSDT",
        .side = Side::Buy,
        .quantity = 2.0,
        .price = 52000.0
    };

    auto decision4b = engine.validate(order4b, pos_long);
    assert(decision4b.approved);
    print_test("Test 4b: Current pos 8 BTC + BUY 2 = 10 BTC (approved - at limit)", decision4b.approved);

    // ================================================================
    // REJECTED: DAILY DRAWDOWN LIMIT
    // ================================================================
    std::cout << "\n--- REJECTED: DAILY DRAWDOWN LIMIT ($50k max loss per day) ---\n\n";

    engine.update_daily_pnl(-60000.0);
    std::cout << "Current daily PnL: -$60,000\n";
    
    auto decision5 = engine.validate(order1, pos);
    assert(!decision5.approved);
    assert(decision5.reason.find("drawdown") != std::string::npos);
    print_test("Test 5: BUY request when -$60k loss already made (REJECTED - max -$50k)", !decision5.approved);
    std::cout << "        Reason: " << decision5.reason << "\n";

    engine.reset_daily_pnl();
    std::cout << "\nDaily PnL reset to $0\n";
    
    auto decision6 = engine.validate(order1, pos);
    assert(decision6.approved);
    print_test("Test 6: BUY request after drawdown reset (approved)", decision6.approved);

    // ================================================================
    // REJECTED: INVALID PARAMETERS
    // ================================================================
    std::cout << "\n--- REJECTED: INVALID PARAMETERS ---\n\n";

    OrderIntent order7{
        .event_id = 7,
        .symbol = "",  // Empty symbol!
        .side = Side::Buy,
        .quantity = 1.0,
        .price = 50000.0
    };

    auto decision7 = engine.validate(order7, pos);
    assert(!decision7.approved);
    print_test("Test 7: Order with empty symbol (REJECTED - invalid param)", !decision7.approved);
    std::cout << "        Reason: " << decision7.reason << "\n";

    OrderIntent order8{
        .event_id = 8,
        .symbol = "BTCUSDT",
        .side = Side::Buy,
        .quantity = -1.0,  // Negative!
        .price = 50000.0
    };

    auto decision8 = engine.validate(order8, pos);
    assert(!decision8.approved);
    print_test("Test 8: Order with negative quantity (REJECTED - invalid param)", !decision8.approved);
    std::cout << "        Reason: " << decision8.reason << "\n";

    OrderIntent order9{
        .event_id = 9,
        .symbol = "BTCUSDT",
        .side = Side::Buy,
        .quantity = 1.0,
        .price = -100.0  // Negative price!
    };

    auto decision9 = engine.validate(order9, pos);
    assert(!decision9.approved);
    print_test("Test 9: Order with negative price (REJECTED - invalid param)", !decision9.approved);
    std::cout << "        Reason: " << decision9.reason << "\n";

    // ================================================================
    // SUMMARY
    // ================================================================
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "[OK] All 9 tests passed!\n";
    std::cout << "[OK] 4 approved scenarios, 5 rejection scenarios tested\n";
    std::cout << std::string(70, '=') << "\n\n";

    return 0;
}
