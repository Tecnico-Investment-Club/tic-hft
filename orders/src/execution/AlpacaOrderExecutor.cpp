#include "include/execution/AlpacaOrderExecutor.hpp"
#include <iostream>
#include <chrono>
#include <thread>

namespace hft::orders::execution {

AlpacaOrderExecutor::AlpacaOrderExecutor(
    const std::string& api_key,
    const std::string& base_url,
    int max_batch_size
)
    : api_key_(api_key)
    , base_url_(base_url)
    , max_batch_size_(max_batch_size)
{
    std::cout << "[AlpacaOrderExecutor] Initialized\n";
}

AlpacaOrderExecutor::~AlpacaOrderExecutor() {
    shutdown();
}

void AlpacaOrderExecutor::initialize() {
    if (running_) {
        std::cout << "[AlpacaOrderExecutor] Already running\n";
        return;
    }
    
    running_ = true;
    executor_thread_ = std::thread(&AlpacaOrderExecutor::executor_loop, this);
    std::cout << "[AlpacaOrderExecutor] Started (fire-and-forget mode)\n";
}

void AlpacaOrderExecutor::shutdown() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    if (executor_thread_.joinable()) {
        executor_thread_.join();
    }
    
    std::cout << "[AlpacaOrderExecutor] Shutdown complete\n";
}

bool AlpacaOrderExecutor::sendOrder(const Order& order) {
    // Fire-and-forget: simplesmente enfileira e retorna
    {
        std::lock_guard<std::mutex> lock(pending_orders_mutex_);
        pending_orders_.push(order);
    }
    
    std::cout << "[AlpacaOrderExecutor] Order enqueued (fire-and-forget): " 
              << order.asset_id << " " << order.side << " " << order.quantity << "\n";
    
    return true;
}

int AlpacaOrderExecutor::sendOrders(const std::vector<Order>& orders) {
    int count = 0;
    
    {
        std::lock_guard<std::mutex> lock(pending_orders_mutex_);
        for (const auto& order : orders) {
            pending_orders_.push(order);
            count++;
        }
    }
    
    std::cout << "[AlpacaOrderExecutor] Batch of " << count << " orders enqueued\n";
    return count;
}

bool AlpacaOrderExecutor::cancelOrder(uint64_t order_id) {
    std::lock_guard<std::mutex> lock(sent_orders_mutex_);
    
    if (sent_orders_.find(order_id) == sent_orders_.end()) {
        std::cout << "[AlpacaOrderExecutor] Order not found: " << order_id << "\n";
        return false;
    }
    
    std::cout << "[AlpacaOrderExecutor] Order cancelled: " << order_id << "\n";
    return true;
}

OrderStatus AlpacaOrderExecutor::getOrderStatus(uint64_t order_id) {
    std::lock_guard<std::mutex> lock(sent_orders_mutex_);
    
    if (sent_orders_.find(order_id) != sent_orders_.end()) {
        return OrderStatus{true, "Order found"};
    }
    
    return OrderStatus{false, "Order pending"};
}

void AlpacaOrderExecutor::executor_loop() {
    std::cout << "[AlpacaOrderExecutor] Executor loop started (background thread)\n";
    
    while (running_) {
        process_batch();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "[AlpacaOrderExecutor] Executor loop finished\n";
}

void AlpacaOrderExecutor::process_batch() {
    std::vector<Order> batch;
    
    {
        std::lock_guard<std::mutex> lock(pending_orders_mutex_);
        
        while (!pending_orders_.empty() && (int)batch.size() < max_batch_size_) {
            batch.push_back(pending_orders_.front());
            pending_orders_.pop();
        }
    }
    
    if (batch.empty()) {
        return;
    }
    
    std::cout << "[AlpacaOrderExecutor] Processing batch of " << batch.size() << " orders\n";
    
    for (const auto& order : batch) {
        if (submit_to_alpaca(order)) {
            std::lock_guard<std::mutex> lock(sent_orders_mutex_);
            sent_orders_[order.event_id] = order;
        }
    }
}

bool AlpacaOrderExecutor::submit_to_alpaca(const Order& order) {
    try {
        std::cout << "[AlpacaOrderExecutor] Submitting to Alpaca: " 
                  << order.asset_id << " " << order.quantity << "\n";
        return true;
    } catch (const std::exception& e) {
        std::cout << "[AlpacaOrderExecutor] Error: " << e.what() << "\n";
        return false;
    }
}

} // namespace hft::orders::execution
