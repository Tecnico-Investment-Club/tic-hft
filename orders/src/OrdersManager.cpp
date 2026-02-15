#include "OrdersManager.hpp"

#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <ctime>

namespace hft::orders {

OrdersManager::OrdersManager(const std::string& db_connection_string,
                             const std::string& api_key,
                             const std::string& api_secret,
                             bool dry_run,
                             int min_sleep,
                             int max_sleep)
    : dry_run_(dry_run), min_sleep_(min_sleep), max_sleep_(max_sleep) {
    
    db_ = std::make_unique<Database>(db_connection_string);
    broker_ = std::make_unique<BinanceClient>(api_key, api_secret, dry_run);
    
    std::cout << "OrdersManager initialized (dry_run=" << dry_run << ")" << std::endl;
}

OrdersManager::~OrdersManager() {
    shutdown();
}

void OrdersManager::run() {
    running_ = true;
    main_loop_thread_ = std::thread(&OrdersManager::main_loop, this);
    
    // Wait for thread to finish
    if (main_loop_thread_.joinable()) {
        main_loop_thread_.join();
    }
}

void OrdersManager::shutdown() {
    running_ = false;
    if (main_loop_thread_.joinable()) {
        main_loop_thread_.join();
    }
    std::cout << "OrdersManager shutdown" << std::endl;
}

void OrdersManager::submit_order(const Order& order) {
    std::lock_guard<std::mutex> lock(pending_orders_mutex_);
    pending_orders_.push(order);
    db_->insert_order(order);
}

void OrdersManager::main_loop() {
    std::cout << "Main loop started" << std::endl;
    
    while (running_) {
        try {
            // Check for pending orders in database
            auto pending = db_->get_pending_orders();
            if (!pending.empty()) {
                std::cout << "Processing " << pending.size() << " pending orders" << std::endl;
                for (const auto& order : pending) {
                    process_order(order);
                }
            }
            
            // Update status of active orders
            auto active = db_->get_active_orders();
            for (const auto& order : active) {
                update_order_status(order);
            }
            
            // Sleep for a bit
            int sleep_time = pending.empty() ? max_sleep_ : min_sleep_;
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
            
        } catch (const std::exception& e) {
            std::cerr << "Error in main loop: " << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(max_sleep_));
        }
    }
    
    std::cout << "Main loop exited" << std::endl;
}

void OrdersManager::process_order(const Order& order) {
    try {
        if (dry_run_) {
            std::cout << "[DRY RUN] Would place order: " << order.order_id 
                      << " " << order.symbol << " " << order.quantity << std::endl;
            db_->update_order_status(order.order_id, OrderStatus::FILLED);
            return;
        }
        
        std::uint64_t binance_order_id = 0;
        
        if (order.type == OrderType::LIMIT) {
            binance_order_id = broker_->place_limit_order(
                order.symbol,
                order.side,
                order.quantity,
                order.price
            );
        } else {
            binance_order_id = broker_->place_market_order(
                order.symbol,
                order.side,
                order.quantity
            );
        }
        
        // Update database with Binance order ID
        db_->update_order_binance_id(order.order_id, binance_order_id);
        db_->update_order_status(order.order_id, OrderStatus::NEW);
        
        log_event(order.order_id, "Order placed on Binance: " + std::to_string(binance_order_id));
        
    } catch (const std::exception& e) {
        std::cerr << "Error processing order " << order.order_id << ": " << e.what() << std::endl;
        db_->update_order_status(order.order_id, OrderStatus::REJECTED);
    }
}

void OrdersManager::update_order_status(const Order& order) {
    try {
        if (!order.binance_order_id) {
            std::cerr << "Order " << order.order_id << " has no binance_order_id" << std::endl;
            return;
        }
        
        auto response = broker_->get_order_status(order.symbol, order.binance_order_id);
        
        std::string status_str = response["status"].get<std::string>();
        OrderStatus new_status = OrderStatus::ERROR;
        
        if (status_str == "NEW") new_status = OrderStatus::NEW;
        else if (status_str == "PARTIALLY_FILLED") new_status = OrderStatus::PARTIALLY_FILLED;
        else if (status_str == "FILLED") new_status = OrderStatus::FILLED;
        else if (status_str == "CANCELED") new_status = OrderStatus::CANCELED;
        else if (status_str == "REJECTED") new_status = OrderStatus::REJECTED;
        else if (status_str == "EXPIRED") new_status = OrderStatus::EXPIRED;
        
        db_->update_order_status(order.order_id, new_status);
        
        // Update fill information
        if (response.contains("executedQty") && response.contains("cummulativeQuoteAssetTransactedQty")) {
            double filled_qty = std::stod(response["executedQty"].get<std::string>());
            double cummulative = std::stod(response["cummulativeQuoteAssetTransactedQty"].get<std::string>());
            double avg_price = (filled_qty > 0) ? (cummulative / filled_qty) : 0.0;
            
            db_->update_order_fills(order.order_id, filled_qty, avg_price);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error updating order status " << order.order_id << ": " << e.what() << std::endl;
    }
}

void OrdersManager::log_event(std::uint64_t order_id, const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    std::cout << "[" << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") 
              << "] Order " << order_id << ": " << message << std::endl;
}

} // namespace hft::orders
