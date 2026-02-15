#pragma once

#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include "Models.hpp"

namespace hft::orders {

using json = nlohmann::json;

class BinanceClient {
public:
    BinanceClient(const std::string& api_key, 
                  const std::string& api_secret,
                  bool testnet = false);
    ~BinanceClient();
    
    uint64_t place_limit_order(const std::string& symbol,
                               OrderSide side,
                               double quantity,
                               double price);
    
    uint64_t place_market_order(const std::string& symbol,
                                OrderSide side,
                                double quantity);
    
    json get_order_status(const std::string& symbol,
                         uint64_t binance_order_id);
    
    json cancel_order(const std::string& symbol,
                     uint64_t binance_order_id);

private:
    std::string api_key_;
    std::string api_secret_;
    std::string base_url_;
    bool testnet_;
    
    json http_get(const std::string& endpoint, const std::string& params);
    json http_post(const std::string& endpoint, const std::string& params);
    std::string generate_signature(const std::string& data);
};

} // namespace hft::orders
