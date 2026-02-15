#pragma once

#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>

#include "Models.hpp"
#include "BinanceClient.hpp"
#include "Database.hpp"

namespace hft::orders {

/**
 * @class OrdersManager
 * Main orchestrator for order execution and tracking
 * Manages the lifecycle of orders from submission to completion
 */
class OrdersManager {
public:
    /**
     * Constructor
     * @param db_connection_string PostgreSQL connection string
     * @param api_key Binance API key
     * @param api_secret Binance API secret
     * @param dry_run Run in dry-run mode without real orders
     * @param min_sleep Minimum sleep time between iterations (ms)
     * @param max_sleep Maximum sleep time between iterations (ms)
     */
    OrdersManager(const std::string& db_connection_string,
                  const std::string& api_key,
                  const std::string& api_secret,
                  bool dry_run = false,
                  int min_sleep = 1,
                  int max_sleep = 5);
    
    ~OrdersManager();
    
    /**
     * Start the main execution loop (blocks until shutdown)
     */
    void run();
    
    /**
     * Gracefully shutdown the manager
     */
    void shutdown();
    
    /**
     * Submit a new order for processing
     * @param order Order to submit
     */
    void submit_order(const Order& order);

private:
    std::unique_ptr<Database> db_;
    std::unique_ptr<BinanceClient> broker_;
    
    bool dry_run_;
    int min_sleep_;
    int max_sleep_;
    
    std::atomic<bool> running_{false};
    std::thread main_loop_thread_;
    
    // Order queue for pending orders
    std::queue<Order> pending_orders_;
    std::mutex pending_orders_mutex_;
    
    /**
     * Main event loop
     * Processes pending orders and updates active order statuses
     */
    void main_loop();
    
    /**
     * Process a pending order
     * Places it on Binance and updates database
     * @param order Order to process
     */
    void process_order(const Order& order);
    
    /**
     * Update status of an active order
     * Fetches status from Binance and updates database
     * @param order Order to update
     */
    void update_order_status(const Order& order);
    
    /**
     * Log order event
     * @param order_id Order ID
     * @param message Log message
     */
    void log_event(std::uint64_t order_id, const std::string& message);
};

} // namespace hft::orders
