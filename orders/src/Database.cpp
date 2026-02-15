#include "Database.hpp"

#include <iostream>
#include <stdexcept>
#include <sstream>
#include <chrono>

namespace hft::orders {

Database::Database(const std::string& connection_string)
    : connection_string_(connection_string) {
    
    conn_ = PQconnectdb(connection_string.c_str());
    
    if (PQstatus(conn_) != CONNECTION_OK) {
        std::string error = PQerrorMessage(conn_);
        PQfinish(conn_);
        throw std::runtime_error("Database connection failed: " + error);
    }
    
    std::cout << "Database connected successfully" << std::endl;
    ensure_schema();
}

Database::~Database() {
    if (conn_) {
        PQfinish(conn_);
    }
}

bool Database::is_connected() const {
    return conn_ && PQstatus(conn_) == CONNECTION_OK;
}

void Database::ensure_schema() {
    if (!is_connected()) {
        throw std::runtime_error("Database not connected");
    }
    
    // Create schema
    const char* create_schema = "CREATE SCHEMA IF NOT EXISTS hft_orders;";
    PGresult* res = PQexec(conn_, create_schema);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::string error = PQerrorMessage(conn_);
        PQclear(res);
        throw std::runtime_error("Failed to create schema: " + error);
    }
    PQclear(res);
    
    // Create orders table
    const char* create_orders_table = R"(
        CREATE TABLE IF NOT EXISTS hft_orders.orders (
            order_id BIGINT PRIMARY KEY,
            symbol VARCHAR(20) NOT NULL,
            side VARCHAR(4) NOT NULL,
            type VARCHAR(10) NOT NULL,
            quantity DECIMAL(18,8) NOT NULL,
            price DECIMAL(18,8),
            status VARCHAR(20) NOT NULL DEFAULT 'PENDING',
            binance_order_id BIGINT,
            filled_quantity DECIMAL(18,8) DEFAULT 0,
            avg_price DECIMAL(18,8) DEFAULT 0,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        );
    )";
    
    res = PQexec(conn_, create_orders_table);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::string error = PQerrorMessage(conn_);
        PQclear(res);
        throw std::runtime_error("Failed to create orders table: " + error);
    }
    PQclear(res);
    
    // Create indexes
    const char* create_indexes = R"(
        CREATE INDEX IF NOT EXISTS idx_orders_status ON hft_orders.orders(status);
        CREATE INDEX IF NOT EXISTS idx_orders_symbol ON hft_orders.orders(symbol);
        CREATE INDEX IF NOT EXISTS idx_orders_created_at ON hft_orders.orders(created_at);
    )";
    
    res = PQexec(conn_, create_indexes);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "Warning: Failed to create indexes" << std::endl;
    }
    PQclear(res);
    
    std::cout << "Database schema ensured" << std::endl;
}

void Database::insert_order(const Order& order) {
    std::ostringstream query;
    query << "INSERT INTO hft_orders.orders "
          << "(order_id, symbol, side, type, quantity, price, status, created_at, updated_at) "
          << "VALUES (" << order.order_id << ", '"
          << order.symbol << "', '"
          << (order.side == OrderSide::BUY ? "BUY" : "SELL") << "', '"
          << (order.type == OrderType::LIMIT ? "LIMIT" : "MARKET") << "', "
          << order.quantity << ", "
          << (order.price > 0 ? std::to_string(order.price) : "NULL") << ", '"
          << "PENDING', NOW(), NOW());";
    
    PGresult* res = execute_query(query.str());
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::string error = PQerrorMessage(conn_);
        PQclear(res);
        throw std::runtime_error("Failed to insert order: " + error);
    }
    PQclear(res);
}

std::vector<Order> Database::get_pending_orders() {
    std::vector<Order> orders;
    
    const char* query = "SELECT * FROM hft_orders.orders WHERE status = 'PENDING' ORDER BY created_at ASC;";
    PGresult* res = execute_query(query);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::string error = PQerrorMessage(conn_);
        PQclear(res);
        throw std::runtime_error("Failed to fetch pending orders: " + error);
    }
    
    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) {
        orders.push_back(row_to_order(res, i));
    }
    
    PQclear(res);
    return orders;
}

std::vector<Order> Database::get_active_orders() {
    std::vector<Order> orders;
    
    const char* query = "SELECT * FROM hft_orders.orders WHERE status IN ('NEW', 'PARTIALLY_FILLED') ORDER BY created_at ASC;";
    PGresult* res = execute_query(query);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::string error = PQerrorMessage(conn_);
        PQclear(res);
        throw std::runtime_error("Failed to fetch active orders: " + error);
    }
    
    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) {
        orders.push_back(row_to_order(res, i));
    }
    
    PQclear(res);
    return orders;
}

void Database::update_order_status(std::uint64_t order_id, OrderStatus status) {
    std::ostringstream query;
    query << "UPDATE hft_orders.orders SET status = '" << status_to_string(status)
          << "', updated_at = NOW() WHERE order_id = " << order_id << ";";
    
    PGresult* res = execute_query(query.str());
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::string error = PQerrorMessage(conn_);
        PQclear(res);
        throw std::runtime_error("Failed to update order status: " + error);
    }
    PQclear(res);
}

void Database::update_order_binance_id(std::uint64_t order_id,
                                       std::uint64_t binance_order_id) {
    std::ostringstream query;
    query << "UPDATE hft_orders.orders SET binance_order_id = " << binance_order_id
          << ", updated_at = NOW() WHERE order_id = " << order_id << ";";
    
    PGresult* res = execute_query(query.str());
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::string error = PQerrorMessage(conn_);
        PQclear(res);
        throw std::runtime_error("Failed to update binance order ID: " + error);
    }
    PQclear(res);
}

void Database::update_order_fills(std::uint64_t order_id,
                                  double filled_qty,
                                  double avg_price) {
    std::ostringstream query;
    query << "UPDATE hft_orders.orders SET filled_quantity = " << filled_qty
          << ", avg_price = " << avg_price
          << ", updated_at = NOW() WHERE order_id = " << order_id << ";";
    
    PGresult* res = execute_query(query.str());
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::string error = PQerrorMessage(conn_);
        PQclear(res);
        throw std::runtime_error("Failed to update order fills: " + error);
    }
    PQclear(res);
}

PGresult* Database::execute_query(const std::string& query) {
    if (!is_connected()) {
        throw std::runtime_error("Database not connected");
    }
    return PQexec(conn_, query.c_str());
}

Order Database::row_to_order(PGresult* res, int row) {
    Order order;
    
    order.order_id = std::stoull(PQgetvalue(res, row, 0));
    order.symbol = PQgetvalue(res, row, 1);
    order.side = (std::string(PQgetvalue(res, row, 2)) == "BUY") ? OrderSide::BUY : OrderSide::SELL;
    order.type = (std::string(PQgetvalue(res, row, 3)) == "LIMIT") ? OrderType::LIMIT : OrderType::MARKET;
    order.quantity = std::stod(PQgetvalue(res, row, 4));
    order.price = std::stod(PQgetvalue(res, row, 5));
    order.status = string_to_status(PQgetvalue(res, row, 6));
    
    if (PQgetisnull(res, row, 7) == 0) {
        order.binance_order_id = std::stoull(PQgetvalue(res, row, 7));
    }
    
    order.filled_quantity = std::stod(PQgetvalue(res, row, 8));
    order.avg_price = std::stod(PQgetvalue(res, row, 9));
    
    return order;
}

std::string Database::status_to_string(OrderStatus status) {
    switch (status) {
        case OrderStatus::PENDING: return "PENDING";
        case OrderStatus::NEW: return "NEW";
        case OrderStatus::PARTIALLY_FILLED: return "PARTIALLY_FILLED";
        case OrderStatus::FILLED: return "FILLED";
        case OrderStatus::CANCELED: return "CANCELED";
        case OrderStatus::REJECTED: return "REJECTED";
        case OrderStatus::EXPIRED: return "EXPIRED";
        case OrderStatus::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

OrderStatus Database::string_to_status(const std::string& status_str) {
    if (status_str == "PENDING") return OrderStatus::PENDING;
    if (status_str == "NEW") return OrderStatus::NEW;
    if (status_str == "PARTIALLY_FILLED") return OrderStatus::PARTIALLY_FILLED;
    if (status_str == "FILLED") return OrderStatus::FILLED;
    if (status_str == "CANCELED") return OrderStatus::CANCELED;
    if (status_str == "REJECTED") return OrderStatus::REJECTED;
    if (status_str == "EXPIRED") return OrderStatus::EXPIRED;
    if (status_str == "ERROR") return OrderStatus::ERROR;
    return OrderStatus::PENDING;
}

} // namespace hft::orders
