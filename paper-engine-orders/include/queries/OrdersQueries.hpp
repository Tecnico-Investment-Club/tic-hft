#pragma once

#include <string>
#include <vector>
#include "../model/Order.hpp"

namespace hft::paper_engine {

/**
 * OrdersQueries: Manages SQL queries for Order entity
 * Mirrors structure of tic-strategy-app/alpaca queries
 */
class OrdersQueries {
public:
    // DDL (schema creation)
    static std::string create_table();
    
    // DML (data operations)
    static std::string insert_order(const Order& order);
    static std::string update_order_status(uint64_t event_id, OrderStatus status);
    static std::string select_pending_orders();
    static std::string select_order_by_id(uint64_t event_id);
    static std::string select_orders_by_portfolio(uint64_t portfolio_id);
    
    // Helper to convert Order to SQL insert values
    static std::string order_to_values(const Order& order);
    
private:
    static constexpr const char* TABLE_NAME = "hft_paper_engine.orders";
};

} // namespace hft::paper_engine
