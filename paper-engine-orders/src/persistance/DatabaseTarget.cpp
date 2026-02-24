#include "persistance/DatabaseTarget.hpp"
#include "queries/OrdersQueries.hpp"
#include <iostream>

namespace hft::paper_engine {

DatabaseTarget::DatabaseTarget(const std::string& connection_string)
    : connection_string_(connection_string) {}

DatabaseTarget::~DatabaseTarget() {
    disconnect();
}

void DatabaseTarget::connect() {
    if (is_connected()) return;
    
    try {
        connection_ = std::make_unique<pqxx::connection>(connection_string_);
        if (!connection_->is_open()) {
            throw std::runtime_error("Failed to open database connection");
        }
        std::cout << "[DatabaseTarget] Connected to PostgreSQL" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[DatabaseTarget] Connection error: " << e.what() << std::endl;
        throw;
    }
}

void DatabaseTarget::disconnect() {
    if (connection_ && connection_->is_open()) {
        connection_->disconnect();
        std::cout << "[DatabaseTarget] Disconnected from PostgreSQL" << std::endl;
    }
}

bool DatabaseTarget::is_connected() const {
    return connection_ && connection_->is_open();
}

void DatabaseTarget::write_order(const Order& order) {
    if (!is_connected()) {
        throw std::runtime_error("Database not connected");
    }
    
    try {
        std::string sql = OrdersQueries::insert_order(order);
        execute(sql);
    } catch (const std::exception& e) {
        std::cerr << "[DatabaseTarget] Write order error: " << e.what() << std::endl;
        throw;
    }
}

void DatabaseTarget::write_orders(const std::vector<Order>& orders) {
    for (const auto& order : orders) {
        write_order(order);
    }
}

void DatabaseTarget::update_order(const Order& order) {
    if (!is_connected()) {
        throw std::runtime_error("Database not connected");
    }
    
    try {
        std::string sql = OrdersQueries::update_order_status(order.event_id, order.status);
        execute(sql);
    } catch (const std::exception& e) {
        std::cerr << "[DatabaseTarget] Update order error: " << e.what() << std::endl;
        throw;
    }
}

void DatabaseTarget::execute(const std::string& sql) {
    if (!is_connected()) {
        throw std::runtime_error("Database not connected");
    }
    
    try {
        pqxx::work txn(*connection_);
        txn.exec(sql);
        txn.commit();
    } catch (const std::exception& e) {
        std::cerr << "[DatabaseTarget] Execute error: " << e.what() << std::endl;
        throw;
    }
}

} // namespace hft::paper_engine
