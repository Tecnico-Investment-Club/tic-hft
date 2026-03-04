#include <iostream>
#include <memory>
#include <thread>
#include <iomanip>
#include <atomic>
#include <fstream>
#include <string>
#include <cstdlib>
#include "../include/execution/AlpacaOrderExecutor.hpp"

using namespace hft::orders::execution;

/**
 * Simple Order Executor with Capital Tracking
 * 
 * Demonstrates:
 * 1. Creating and sending orders via sendOrder()
 * 2. Tracking capital/balance as orders are executed
 * 3. Periodic balance updates
 */
class SimpleOrderManager {
private:
    std::unique_ptr<IExecution> executor_;
    std::atomic<double> balance_{100000.0};  // Initial capital: $100k
    std::atomic<bool> running_{false};
    std::thread balance_update_thread_;

public:
    SimpleOrderManager(const std::string& api_key, const std::string& secret_key, const std::string& base_url)
        : executor_(std::make_unique<AlpacaOrderExecutor>(api_key, secret_key, base_url, 10))
    {
    }

    ~SimpleOrderManager() {
        shutdown();
    }

    void initialize() {
        executor_->initialize();
        running_ = true;
        balance_update_thread_ = std::thread(&SimpleOrderManager::update_balance_periodically, this);
        std::cout << "[OrderManager] Initialized with $" << std::fixed << std::setprecision(2) 
                  << balance_.load() << "\n";
    }

    void shutdown() {
        running_ = false;
        if (balance_update_thread_.joinable()) {
            balance_update_thread_.join();
        }
        executor_->shutdown();
        std::cout << "[OrderManager] Shutdown complete\n";
    }

    bool send_order(const Order& order) {
        // Calculate order cost
        double cost = order.quantity * order.price;
        
        // For BUY orders, check if we have enough capital
        if (order.side == "BUY") {
            double current_balance = balance_.load();
            if (cost > current_balance) {
                std::cout << "[OrderManager] ❌ REJECTED: insufficient capital. "
                         << "Need: $" << std::fixed << std::setprecision(2) << cost 
                         << ", Have: $" << current_balance << "\n";
                return false;
            }
            
            // Deduct cost from balance
            balance_ = balance_ - cost;
            std::cout << "[OrderManager] 📊 BUY order cost: -$" << std::fixed << std::setprecision(2) << cost 
                     << " | Balance: $" << balance_.load() << "\n";
        }
        else if (order.side == "SELL") {
            // For SELL orders, add proceeds to balance
            double proceeds = order.quantity * order.price;
            balance_ = balance_ + proceeds;
            std::cout << "[OrderManager] 📊 SELL order proceeds: +$" << std::fixed << std::setprecision(2) << proceeds 
                     << " | Balance: $" << balance_.load() << "\n";
        }
        
        // Send to executor (fire-and-forget)
        return executor_->sendOrder(order);
    }

    double get_balance() const {
        return balance_.load();
    }

private:
    void update_balance_periodically() {
        int count = 0;
        while (running_) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            
            count++;
            std::cout << "[OrderManager] 💰 Balance Update #" << count 
                     << " - Current: $" << std::fixed << std::setprecision(2) 
                     << balance_.load() << "\n";
        }
    }
};

int main() {
    std::cout << "=== HFT Orders System - Fire-and-Forget Example ===\n";
    std::cout << "Simple order execution with capital tracking\n\n";

    // Read API credentials from .env/env vars
    auto read_env = [](const char* key) {
        const char* value = std::getenv(key);
        return value ? std::string(value) : std::string();
    };

    std::string api_key = read_env("ALPACA_API_KEY");
    std::string secret_key = read_env("ALPACA_SECRET_KEY");
    std::string base_url = read_env("ALPACA_BASE_URL");
    
    if (base_url.empty()) {
        base_url = "https://paper-api.alpaca.markets";
    }
    
    std::cout << "API Key loaded: " << (api_key.empty() ? "❌ NOT FOUND" : "✓ OK") << "\n";
    std::cout << "Secret Key loaded: " << (secret_key.empty() ? "❌ NOT FOUND" : "✓ OK") << "\n";
    std::cout << "Base URL: " << base_url << "\n\n";

    // Create manager with API credentials
    SimpleOrderManager manager(api_key, secret_key, base_url);

    manager.initialize();

    std::cout << "\n--- Sending Orders ---\n";

    // Order 1: BUY 100 shares of AAPL at $150
    Order order1{
        .portfolio_id = 1,
        .event_id = 100,
        .delivery_id = 1,
        .asset_id = "AAPL",
        .quantity = 100.0,
        .price = 150.0,
        .side = "BUY"
    };

    std::cout << "Order 1: BUY 100 AAPL @ $150\n";
    auto start = std::chrono::high_resolution_clock::now();
    manager.send_order(order1);
    auto end = std::chrono::high_resolution_clock::now();
    auto latency_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << "Dispatch latency: " << latency_us << " μs\n\n";

    // Order 2: BUY 50 shares of MSFT at $350
    Order order2{
        .portfolio_id = 1,
        .event_id = 101,
        .delivery_id = 2,
        .asset_id = "MSFT",
        .quantity = 50.0,
        .price = 350.0,
        .side = "BUY"
    };

    std::cout << "Order 2: BUY 50 MSFT @ $350\n";
    manager.send_order(order2);
    std::cout << "\n";

    // Order 3: SELL 30 shares of GOOGL at $2800
    Order order3{
        .portfolio_id = 1,
        .event_id = 102,
        .delivery_id = 3,
        .asset_id = "GOOGL",
        .quantity = 30.0,
        .price = 2800.0,
        .side = "SELL"
    };

    std::cout << "Order 3: SELL 30 GOOGL @ $2800\n";
    manager.send_order(order3);
    std::cout << "\n";

    // Order 4: Try to buy more than we can afford (will be rejected)
    Order order4{
        .portfolio_id = 1,
        .event_id = 103,
        .delivery_id = 4,
        .asset_id = "BRK.A",
        .quantity = 100.0,
        .price = 500000.0,  // Too expensive!
        .side = "BUY"
    };

    std::cout << "Order 4: BUY 100 BRK.A @ $500,000 (testing rejection)\n";
    manager.send_order(order4);
    std::cout << "\n";

    std::cout << "--- Waiting for background processing ---\n";
    std::this_thread::sleep_for(std::chrono::seconds(6));

    std::cout << "\n--- Final State ---\n";
    std::cout << "Final Balance: $" << std::fixed << std::setprecision(2) 
              << manager.get_balance() << "\n";

    manager.shutdown();

    std::cout << "\n✅ Example completed!\n";
    std::cout << "Fire-and-forget orders sent successfully with capital tracking.\n";

    return 0;
}
