#pragma once

#include <string>
#include <vector>
#include <memory>
#include <libpq-fe.h>

#include "Models.hpp"

namespace hft::orders {

/**
 * @class Database
 * PostgreSQL database interface for orders persistence
 * Uses libpq with async connections for non-blocking operations
 */
class Database {
public:
    /**
     * Constructor
     * @param connection_string PostgreSQL connection string
     */
    explicit Database(const std::string& connection_string);
    
    ~Database();
    
    // Prevent copying
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    
    /**
     * Check if database connection is active
     * @return true if connected
     */
    bool is_connected() const;
    
    /**
     * Ensure database schema exists
     * Creates tables if they don't exist
     */
    void ensure_schema();
    
    /**
     * Insert a new order
     * @param order Order to insert
     */
    void insert_order(const Order& order);
    
    /**
     * Get all pending orders
     * @return Vector of pending orders
     */
    std::vector<Order> get_pending_orders();
    
    /**
     * Get all active orders (NEW, PARTIALLY_FILLED)
     * @return Vector of active orders
     */
    std::vector<Order> get_active_orders();
    
    /**
     * Update order status
     * @param order_id Internal order ID
     * @param status New status
     */
    void update_order_status(std::uint64_t order_id, OrderStatus status);
    
    /**
     * Update order with Binance order ID
     * @param order_id Internal order ID
     * @param binance_order_id Binance order ID
     */
    void update_order_binance_id(std::uint64_t order_id, 
                                 std::uint64_t binance_order_id);
    
    /**
     * Update order fill information
     * @param order_id Internal order ID
     * @param filled_qty Quantity filled
     * @param avg_price Average fill price
     */
    void update_order_fills(std::uint64_t order_id,
                            double filled_qty,
                            double avg_price);

private:
    std::string connection_string_;
    PGconn* conn_;
    
    /**
     * Execute SQL query synchronously
     * @param query SQL query string
     * @return PGresult pointer (must be freed with PQclear)
     */
    PGresult* execute_query(const std::string& query);
    
    /**
     * Convert PGresult row to Order object
     * @param res PGresult pointer
     * @param row Row number
     * @return Order object
     */
    Order row_to_order(PGresult* res, int row);
    
    /**
     * Convert OrderStatus enum to SQL string
     * @param status Order status
     * @return Status string
     */
    static std::string status_to_string(OrderStatus status);
    
    /**
     * Convert SQL string to OrderStatus enum
     * @param status_str Status string
     * @return OrderStatus enum
     */
    static OrderStatus string_to_status(const std::string& status_str);
};

} // namespace hft::orders
