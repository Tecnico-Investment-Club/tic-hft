#include "exchange/ConsoleExecutionClient.hpp"
#include "strategy/PingPongStrategy.hpp"
#include <memory>
#include <vector>
#include <iostream>

using namespace tic;

int main() {
    std::cout << "[MAIN] Starting the TIC Trading Engine \n\n";

    // Initialize all the components
    auto consoleClient = std::make_shared<ConsoleExecutionClient>();
    PingPongStrategy strategy(consoleClient);

    // Create fake market data
    std::vector<Tick> fakeMarketData = {
        {"BTCUSDT", 92000.0, 1.5, 1700000000},
        {"BTCUSDT", 91000.0, 2.0, 1700000001},
        {"BTCUSDT", 89500.0, 5.0, 1700000002}, // Should trigger BUY
        {"BTCUSDT", 93000.0, 1.0, 1700000003},
        {"BTCUSDT", 96000.0, 3.0, 1700000004}  // Should trigger SELL
    };

    // Feed data into the strategy
    for (const auto& tick : fakeMarketData) strategy.onTick(tick);

    // Final print
    std::cout << "[MAIN] Finish the TIC Trading Engine \n";
    return 0;
}