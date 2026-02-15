#pragma once

#include <map>
#include <vector>
#include <mutex>
#include <memory>
#include "Models.hpp"

namespace hft::orders {

class OrderStore {
public:
    explicit OrderStore(const std::string& log_file = "");
    
    void insert_order(const Order& order);
    std::vector<Order> get_pending_orders();
    std::vector<Order> get_active_orders();
    std::shared_ptr<Order> get_order(uint64_t order_id);
    
    void update_order_status(uint64_t order_id, OrderStatus status);
    void update_order_binance_id(uint64_t order_id, uint64_t binance_order_id);
    void update_order_fills(uint64_t order_id, double filled_qty, double avg_price);
    
    std::vector<Order> get_all_orders();

private:
    std::map<uint64_t, Order> orders_;
    std::mutex orders_mutex_;
    std::string log_file_;
    
    void log_order_event(const Order& order);
};

} // namespace hft::orders
