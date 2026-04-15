#pragma once

#include <vector>
#include <memory>
#include <cstdint>

// Import common types from core (for integration with other modules)
// Note: local types below are kept for backward compatibility
#include "../../../core/include/CommonTypes.hpp"

namespace hft::orders::execution {

struct Order {
    uint64_t portfolio_id = 0;
    uint64_t event_id = 0;
    uint64_t delivery_id = 0;
    std::string asset_id;
    double quantity = 0.0;
    double price = 0.0;
    std::string side;  // "BUY" ou "SELL"
};

struct OrderStatus {
    bool is_valid = false;
    std::string reason;
};

/**
 * IExecution: Interface para execução de orders (fire-and-forget)
 * 
 * Strategy cria uma "bala" (Order)
 * Risk coloca a bala na arma (Orders)
 * A arma dispara sem olhar (fire-and-forget)
 */
class IExecution {
public:
    virtual ~IExecution() = default;
    
    /**
     * Submete uma order para execução
     * Fire-and-forget: retorna imediatamente
     */
    virtual bool sendOrder(const Order& order) = 0;
    
    /**
     * Submete múltiplas orders em batch
     */
    virtual int sendOrders(const std::vector<Order>& orders) = 0;
    
    /**
     * Cancela uma order
     */
    virtual bool cancelOrder(uint64_t order_id) = 0;
    
    /**
     * Obtém status de uma order
     */
    virtual OrderStatus getOrderStatus(uint64_t order_id) = 0;
    
    /**
     * Inicializa o executor
     */
    virtual void initialize() = 0;
    
    /**
     * Desliga o executor
     */
    virtual void shutdown() = 0;
};

using IExecutionPtr = std::unique_ptr<IExecution>;

} // namespace hft::orders::execution
