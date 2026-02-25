#pragma once

#include <string>
#include <memory>
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include <map>
#include "IExecution.hpp"

namespace hft::orders::execution {

/**
 * AlpacaOrderExecutor: Implementação de IExecution para Alpaca
 * 
 * Fire-and-Forget:
 * - sendOrder() enfileira order
 * - Background thread processa assincramente
 * - Rate limiting automático
 */
class AlpacaOrderExecutor : public IExecution {
public:
    AlpacaOrderExecutor(
        const std::string& api_key,
        const std::string& base_url = "https://paper-api.alpaca.markets",
        int max_batch_size = 10
    );
    
    ~AlpacaOrderExecutor();
    
    bool sendOrder(const Order& order) override;
    int sendOrders(const std::vector<Order>& orders) override;
    bool cancelOrder(uint64_t order_id) override;
    OrderStatus getOrderStatus(uint64_t order_id) override;
    void initialize() override;
    void shutdown() override;

private:
    std::string api_key_;
    std::string base_url_;
    int max_batch_size_;
    
    // Fire-and-forget: Queue para processamento assincronío
    std::queue<Order> pending_orders_;
    std::mutex pending_orders_mutex_;
    
    // Background thread
    std::thread executor_thread_;
    std::atomic<bool> running_{false};
    
    // Cache de ordens já enviadas
    std::map<uint64_t, Order> sent_orders_;
    std::mutex sent_orders_mutex_;
    
    // Métodos privados
    void executor_loop();
    void process_batch();
    bool submit_to_alpaca(const Order& order);
};

} // namespace hft::orders::execution
