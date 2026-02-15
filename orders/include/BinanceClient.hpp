#pragma once

#include <string>
#include <memory>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>

#include "Models.hpp"

namespace hft::orders {

using json = nlohmann::json;

/**
 * @class BinanceClient
 * REST API client for Binance order execution and status queries
 * Uses libcurl for HTTP and simdjson/nlohmann for JSON handling
 */
class BinanceClient {
public:
    /**
     * Constructor
     * @param api_key Binance API key
     * @param api_secret Binance API secret
     * @param testnet Use testnet if true
     */
    BinanceClient(const std::string& api_key, 
                  const std::string& api_secret,
                  bool testnet = false);
    
    ~BinanceClient();
    
    /**
     * Place a limit order on Binance
     * @param symbol Trading pair (e.g., "BTCUSDT")
     * @param side BUY or SELL
     * @param quantity Order quantity
     * @param price Limit price
     * @return Binance order ID
     */
    std::uint64_t place_limit_order(const std::string& symbol,
                                    OrderSide side,
                                    double quantity,
                                    double price);
    
    /**
     * Place a market order on Binance
     * @param symbol Trading pair
     * @param side BUY or SELL
     * @param quantity Order quantity
     * @return Binance order ID
     */
    std::uint64_t place_market_order(const std::string& symbol,
                                     OrderSide side,
                                     double quantity);
    
    /**
     * Get order status from Binance
     * @param symbol Trading pair
     * @param binance_order_id Binance order ID
     * @return Order status JSON response
     */
    json get_order_status(const std::string& symbol,
                          std::uint64_t binance_order_id);
    
    /**
     * Cancel an order on Binance
     * @param symbol Trading pair
     * @param binance_order_id Binance order ID
     * @return Cancellation response
     */
    json cancel_order(const std::string& symbol,
                      std::uint64_t binance_order_id);
    
    /**
     * Get account balance
     * @return JSON with account balances
     */
    json get_account();

private:
    std::string api_key_;
    std::string api_secret_;
    std::string base_url_;
    bool testnet_;
    
    /**
     * Make HTTP GET request
     * @param endpoint API endpoint
     * @param params Query parameters
     * @param signed_request Include signature if true
     * @return JSON response
     */
    json http_get(const std::string& endpoint,
                  const std::string& params = "",
                  bool signed_request = false);
    
    /**
     * Make HTTP POST request
     * @param endpoint API endpoint
     * @param params Request parameters
     * @param signed_request Include signature if true
     * @return JSON response
     */
    json http_post(const std::string& endpoint,
                   const std::string& params,
                   bool signed_request = true);
    
    /**
     * Generate HMAC-SHA256 signature
     * @param message Message to sign
     * @param secret Secret key
     * @return Hex-encoded signature
     */
    std::string generate_signature(const std::string& message,
                                   const std::string& secret);
    
    /**
     * Generate nonce timestamp
     * @return Millisecond timestamp
     */
    std::uint64_t get_server_time();
};

} // namespace hft::orders
