#include "OrderStore.hpp"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <nlohmann/json.hpp>

namespace hft::orders {

using json = nlohmann::json;

OrderStore::OrderStore(const std::string& log_file)
    : log_file_(log_file) {
    if (!log_file_.empty()) {
        std::cout << "OrderStore initialized with log file: " << log_file_ << std::endl;
    } else {
        std::cout << "OrderStore initialized (in-memory)" << std::endl;
    }
}

void OrderStore::insert_order(const Order& order) {
    std::lock_guard<std::mutex> lock(orders_mutex_);
    orders_[order.order_id] = order;
    log_order_event(order);
}

std::vector<Order> OrderStore::get_pending_orders() {
    std::lock_guard<std::mutex> lock(orders_mutex_);
    std::vector<Order> pending;
    
    for (const auto& [id, order] : orders_) {
        if (order.status == OrderStatus::PENDING) {
            pending.push_back(order);
        }
    }
    
    return pending;
}

std::vector<Order> OrderStore::get_active_orders() {
    std::lock_guard<std::mutex> lock(orders_mutex_);
    std::vector<Order> active;
    
    for (const auto& [id, order] : orders_) {
        if (order.status == OrderStatus::NEW || 
            order.status == OrderStatus::PARTIALLY_FILLED) {
            active.push_back(order);
        }
    }
    
    return active;
}

std::shared_ptr<Order> OrderStore::get_order(uint64_t order_id) {
    std::lock_guard<std::mutex> lock(orders_mutex_);
    
    auto it = orders_.find(order_id);
    if (it != orders_.end()) {
        return std::make_shared<Order>(it->second);
    }
    
    return nullptr;
}

void OrderStore::update_order_status(uint64_t order_id, OrderStatus status) {
    std::lock_guard<std::mutex> lock(orders_mutex_);
    
    auto it = orders_.find(order_id);
    if (it != orders_.end()) {
        it->second.status = status;
        it->second.updated_at = std::chrono::system_clock::now();
        log_order_event(it->second);
    }
}

void OrderStore::update_order_binance_id(uint64_t order_id, uint64_t binance_order_id) {
    std::lock_guard<std::mutex> lock(orders_mutex_);
    
    auto it = orders_.find(order_id);
    if (it != orders_.end()) {
        it->second.binance_order_id = binance_order_id;
        it->second.updated_at = std::chrono::system_clock::now();
    }
}

void OrderStore::update_order_fills(uint64_t order_id, double filled_qty, double avg_price) {
    std::lock_guard<std::mutex> lock(orders_mutex_);
    
    auto it = orders_.find(order_id);
    if (it != orders_.end()) {
        it->second.filled_quantity = filled_qty;
        it->second.avg_price = avg_price;
        it->second.updated_at = std::chrono::system_clock::now();
    }
}

std::vector<Order> OrderStore::get_all_orders() {
    std::lock_guard<std::mutex> lock(orders_mutex_);
    std::vector<Order> all;
    
    for (const auto& [id, order] : orders_) {
        all.push_back(order);
    }
    
    return all;
}

void OrderStore::log_order_event(const Order& order) {
    if (log_file_.empty()) {
        return;
    }
    
    try {
        std::ifstream infile(log_file_);
        json log_data = json::array();
        
        if (infile.good()) {
            infile >> log_data;
            infile.close();
        }
        
        json entry;
        entry["order_id"] = order.order_id;
        entry["binance_order_id"] = order.binance_order_id;
        entry["symbol"] = order.symbol;
        entry["side"] = (order.side == OrderSide::BUY) ? "BUY" : "SELL";
        entry["type"] = (order.type == OrderType::LIMIT) ? "LIMIT" : "MARKET";
        entry["quantity"] = order.quantity;
        entry["price"] = order.price;
        entry["filled_quantity"] = order.filled_quantity;
        entry["avg_price"] = order.avg_price;
        
        switch (order.status) {
            case OrderStatus::PENDING: entry["status"] = "PENDING"; break;
            case OrderStatus::NEW: entry["status"] = "NEW"; break;
            case OrderStatus::PARTIALLY_FILLED: entry["status"] = "PARTIALLY_FILLED"; break;
            case OrderStatus::FILLED: entry["status"] = "FILLED"; break;
            case OrderStatus::CANCELED: entry["status"] = "CANCELED"; break;
            case OrderStatus::REJECTED: entry["status"] = "REJECTED"; break;
            case OrderStatus::EXPIRED: entry["status"] = "EXPIRED"; break;
            case OrderStatus::ERROR: entry["status"] = "ERROR"; break;
        }
        
        auto time = std::chrono::system_clock::to_time_t(order.updated_at);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        entry["timestamp"] = ss.str();
        
        log_data.push_back(entry);
        
        std::ofstream outfile(log_file_);
        outfile << log_data.dump(2) << std::endl;
        outfile.close();
        
    } catch (const std::exception& e) {
        std::cerr << "Error logging order: " << e.what() << std::endl;
    }
}

} // namespace hft::orders
