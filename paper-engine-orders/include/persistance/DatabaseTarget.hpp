#pragma once

#include <string>
#include <vector>
#include <memory>
#include <pqxx/pqxx>
#include "../model/Order.hpp"

namespace hft::paper_engine {

/**
 * DatabaseTarget: Manages data persistence to PostgreSQL
 * Equivalent to Python's target.Target from tic-strategy-app/alpaca
 */
class DatabaseTarget {
public:
    explicit DatabaseTarget(const std::string& connection_string);
    ~DatabaseTarget();
    
    void connect();
    void disconnect();
    bool is_connected() const;
    
    // Write order to database
    void write_order(const Order& order);
    
    // Write multiple orders
    void write_orders(const std::vector<Order>& orders);
    
    // Update order
    void update_order(const Order& order);
    
    // Execute SQL command
    void execute(const std::string& sql);

private:
    std::string connection_string_;
    std::unique_ptr<pqxx::connection> connection_;
};

} // namespace hft::paper_engine
