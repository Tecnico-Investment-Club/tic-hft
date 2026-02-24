#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <queue>
#include <mutex>
#include "persistance/DatabaseSource.hpp"
#include "persistance/DatabaseTarget.hpp"
#include "model/Order.hpp"

namespace hft::paper_engine {

/**
 * PaperEngineOrders: Main class that orchestrates paper trading with database persistence
 * Equivalent to Python's Loader class from tic-strategy-app/alpaca/paper-engine-orders
 * 
 * Architecture:
 * 1. DatabaseSource: Reads market data from ETL (PostgreSQL)
 * 2. Alpaca API: Executes orders on Alpaca broker
 * 3. DatabaseTarget: Persists orders to PostgreSQL
 */
class PaperEngineOrders {
public:
    PaperEngineOrders(
        const std::string& db_source_connection,
        const std::string& db_target_connection,
        const std::string& alpaca_api_key,
        const std::string& alpaca_base_url,
        uint64_t portfolio_id,
        int strategy_id,
        bool dry_run = false
    );
    
    ~PaperEngineOrders();
    
    // Main lifecycle methods
    void setup();
    void run();
    void shutdown();
    
    // Configuration
    void set_cash_allocation(double amount);
    void set_rebalance_frequency(const std::string& frequency);
    
    // Order management
    void submit_order(const Order& order);
    void update_order_status(uint64_t event_id, OrderStatus status);
    void close_positions();
    
    // Data retrieval
    std::vector<Order> get_pending_orders();
    std::vector<Order> get_portfolio_orders();

private:
    // Database connections
    std::unique_ptr<DatabaseSource> source_;
    std::unique_ptr<DatabaseTarget> target_;
    
    // Broker connection (Alpaca API)
    std::string alpaca_api_key_;
    std::string alpaca_base_url_;
    
    // Configuration
    uint64_t portfolio_id_;
    int strategy_id_;
    double cash_allocation_ = 1000.0;
    std::string rebalance_frequency_ = "daily";
    bool dry_run_;
    
    // Execution state
    std::atomic<bool> running_{false};
    std::thread main_loop_thread_;
    std::queue<Order> pending_orders_;
    std::mutex pending_orders_mutex_;
    
    // Private methods
    void main_loop();
    void process_pending_orders();
    void fetch_market_data();
    void rebalance_portfolio();
    void sync_order_states();
    
    // Helper methods
    std::string calculate_order_hash(const Order& order);
    void log_order_event(const Order& order, const std::string& event);
};

} // namespace hft::paper_engine
