#pragma once

#include <memory>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include "Models.hpp"
#include "BinanceClient.hpp"
#include "OrderStore.hpp"

namespace hft::orders {

class OrdersManager {
public:
    OrdersManager(const std::string& api_key,
                  const std::string& api_secret,
                  bool dry_run = false,
                  int min_sleep = 1,
                  int max_sleep = 5,
                  const std::string& log_file = "");
    ~OrdersManager();
    
    void run();
    void shutdown();
    void submit_order(const Order& order);

private:
    std::unique_ptr<OrderStore> order_store_;
    std::unique_ptr<BinanceClient> broker_;
    
    bool dry_run_;
    int min_sleep_;
    int max_sleep_;
    
    std::atomic<bool> running_{false};
    std::thread main_loop_thread_;
    std::queue<Order> pending_orders_;
    std::mutex pending_orders_mutex_;
    
    void main_loop();
    void process_order(const Order& order);
    void update_order_status(const Order& order);
    void log_event(uint64_t order_id, const std::string& message);
};

} // namespace hft::orders
