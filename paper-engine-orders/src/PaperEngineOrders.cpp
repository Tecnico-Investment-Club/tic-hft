#include "PaperEngineOrders.hpp"
#include "queries/OrdersQueries.hpp"
#include <iostream>
#include <chrono>
#include <random>
#include <iomanip>
#include <sstream>
#include <fstream>

namespace hft::paper_engine {

PaperEngineOrders::PaperEngineOrders(
    const std::string& db_source_connection,
    const std::string& db_target_connection,
    const std::string& alpaca_api_key,
    const std::string& alpaca_base_url,
    uint64_t portfolio_id,
    int strategy_id,
    bool dry_run)
    : source_(std::make_unique<DatabaseSource>(db_source_connection)),
      target_(std::make_unique<DatabaseTarget>(db_target_connection)),
      alpaca_api_key_(alpaca_api_key),
      alpaca_base_url_(alpaca_base_url),
      portfolio_id_(portfolio_id),
      strategy_id_(strategy_id),
      dry_run_(dry_run) {
    
    std::cout << "[PaperEngineOrders] Initialized" << std::endl;
    std::cout << "  Portfolio ID: " << portfolio_id << std::endl;
    std::cout << "  Strategy ID: " << strategy_id << std::endl;
    std::cout << "  Dry Run: " << (dry_run ? "YES" : "NO") << std::endl;
}

PaperEngineOrders::~PaperEngineOrders() {
    shutdown();
}

void PaperEngineOrders::setup() {
    std::cout << "\n[PaperEngineOrders] Setting up..." << std::endl;
    
    // Connect to databases
    source_->connect();
    target_->connect();
    
    // Initialize database schema
    try {
        std::string schema_sql = OrdersQueries::create_table();
        source_->init_tables(schema_sql);
        std::cout << "[PaperEngineOrders] Database schema initialized" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[PaperEngineOrders] Schema initialization error: " << e.what() << std::endl;
        throw;
    }
}

void PaperEngineOrders::run() {
    std::cout << "\n[PaperEngineOrders] Starting main loop..." << std::endl;
    
    running_ = true;
    main_loop_thread_ = std::thread(&PaperEngineOrders::main_loop, this);
    
    // Keep the application running
    main_loop_thread_.join();
}

void PaperEngineOrders::shutdown() {
    std::cout << "\n[PaperEngineOrders] Shutting down..." << std::endl;
    
    running_ = false;
    if (main_loop_thread_.joinable()) {
        main_loop_thread_.join();
    }
    
    // Close positions if not dry run
    if (!dry_run_) {
        try {
            close_positions();
        } catch (const std::exception& e) {
            std::cerr << "[PaperEngineOrders] Error closing positions: " << e.what() << std::endl;
        }
    }
    
    // Disconnect from databases
    if (source_) source_->disconnect();
    if (target_) target_->disconnect();
    
    std::cout << "[PaperEngineOrders] Shutdown complete" << std::endl;
}

void PaperEngineOrders::main_loop() {
    std::cout << "[PaperEngineOrders] Main loop started" << std::endl;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 5000);  // 1-5 seconds
    
    while (running_) {
        try {
            // Fetch latest market data
            fetch_market_data();
            
            // Process any pending orders
            process_pending_orders();
            
            // Sync order states with broker
            sync_order_states();
            
            // Check if rebalancing is needed
            rebalance_portfolio();
            
            // Sleep for a random interval
            int sleep_ms = dis(gen);
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
            
        } catch (const std::exception& e) {
            std::cerr << "[PaperEngineOrders] Error in main loop: " << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
}

void PaperEngineOrders::set_cash_allocation(double amount) {
    cash_allocation_ = amount;
    std::cout << "[PaperEngineOrders] Cash allocation set to: " << amount << std::endl;
}

void PaperEngineOrders::set_rebalance_frequency(const std::string& frequency) {
    rebalance_frequency_ = frequency;
    std::cout << "[PaperEngineOrders] Rebalance frequency set to: " << frequency << std::endl;
}

void PaperEngineOrders::submit_order(const Order& order) {
    std::cout << "[PaperEngineOrders] Submitting order: " << order.asset_id 
              << " " << (order.side == OrderSide::BUY ? "BUY" : "SELL") 
              << " " << order.quantity << std::endl;
    
    // Add to pending queue
    {
        std::lock_guard<std::mutex> lock(pending_orders_mutex_);
        pending_orders_.push(order);
    }
}

void PaperEngineOrders::update_order_status(uint64_t event_id, OrderStatus status) {
    try {
        target_->update_order(Order());  // Will be improved in future
        std::cout << "[PaperEngineOrders] Updated order status: " << event_id << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[PaperEngineOrders] Error updating order: " << e.what() << std::endl;
    }
}

void PaperEngineOrders::close_positions() {
    std::cout << "[PaperEngineOrders] Closing all positions..." << std::endl;
    // Implementation would close all active positions
}

std::vector<Order> PaperEngineOrders::get_pending_orders() {
    try {
        auto results = source_->query(OrdersQueries::select_pending_orders());
        std::vector<Order> orders;
        // Convert results to Order objects
        return orders;
    } catch (const std::exception& e) {
        std::cerr << "[PaperEngineOrders] Error fetching pending orders: " << e.what() << std::endl;
        return {};
    }
}

std::vector<Order> PaperEngineOrders::get_portfolio_orders() {
    try {
        auto results = source_->query(OrdersQueries::select_orders_by_portfolio(portfolio_id_));
        std::vector<Order> orders;
        // Convert results to Order objects
        return orders;
    } catch (const std::exception& e) {
        std::cerr << "[PaperEngineOrders] Error fetching portfolio orders: " << e.what() << std::endl;
        return {};
    }
}

void PaperEngineOrders::process_pending_orders() {
    std::lock_guard<std::mutex> lock(pending_orders_mutex_);
    
    while (!pending_orders_.empty()) {
        Order order = pending_orders_.front();
        pending_orders_.pop();
        
        try {
            // Submit to Alpaca broker
            if (order.side == OrderSide::BUY) {
                // Alpaca API call to place order
                std::cout << "[PaperEngineOrders] Placing BUY order on Alpaca: "
                          << order.asset_id << " x " << order.quantity << std::endl;
                // TODO: Implement Alpaca API call
            } else {
                std::cout << "[PaperEngineOrders] Placing SELL order on Alpaca: "
                          << order.asset_id << " x " << order.quantity << std::endl;
                // TODO: Implement Alpaca API call
            }
            
            // Persist to database
            target_->write_order(order);
            log_order_event(order, "SUBMITTED");
            
        } catch (const std::exception& e) {
            std::cerr << "[PaperEngineOrders] Error processing order: " << e.what() << std::endl;
            order.status = OrderStatus::ERROR;
            target_->write_order(order);
        }
    }
}

void PaperEngineOrders::fetch_market_data() {
    // This would fetch data from the ETL
    // Implementation depends on ETL data structure
}

void PaperEngineOrders::rebalance_portfolio() {
    // This would trigger portfolio rebalancing based on frequency
    // Implementation depends on strategy logic
}

void PaperEngineOrders::sync_order_states() {
    // This would sync order states with the broker
    // Implementation would check order status on Binance
}

std::string PaperEngineOrders::calculate_order_hash(const Order& order) {
    // Simple hash calculation - can be improved with SHA256
    std::ostringstream ss;
    ss << order.portfolio_id << order.asset_id << order.quantity 
       << order.order_timestamp.time_since_epoch().count();
    return ss.str();
}

void PaperEngineOrders::log_order_event(const Order& order, const std::string& event) {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    
    std::cout << "[" << std::put_time(std::localtime(&time_t_now), "%H:%M:%S") << "] "
              << "Order Event - " << event << ": "
              << order.asset_id << " " 
              << (order.side == OrderSide::BUY ? "BUY" : "SELL") << " "
              << order.quantity << std::endl;
}

} // namespace hft::paper_engine
