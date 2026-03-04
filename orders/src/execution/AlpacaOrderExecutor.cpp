#include "include/execution/AlpacaOrderExecutor.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <curl/curl.h>
#include <cstdlib>
#include <algorithm>
#include <cctype>

namespace hft::orders::execution {

// Callback for curl to write response
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

AlpacaOrderExecutor::AlpacaOrderExecutor(
    const std::string& api_key,
    const std::string& secret_key,
    const std::string& base_url,
    int max_batch_size
)
    : api_key_(api_key), secret_key_(secret_key), base_url_(base_url), max_batch_size_(max_batch_size)
{
    std::cout << "[AlpacaOrderExecutor] Initialized with base_url: " << base_url_ << "\n";
}

AlpacaOrderExecutor::~AlpacaOrderExecutor() {
    shutdown();
}

void AlpacaOrderExecutor::initialize() {
    if (running_) {
        std::cout << "[AlpacaOrderExecutor] Already running\n";
        return;
    }
    
    running_ = true;
    
    // Show API key status (masked)
    if (api_key_.empty()) {
        std::cout << "[AlpacaOrderExecutor] ⚠️  No API key configured (ALPACA_API_KEY env var)\n";
    } else {
        std::cout << "[AlpacaOrderExecutor] API Key: " << api_key_.substr(0, 4) << "****" 
                  << api_key_.substr(api_key_.length() - 4) << " (length: " << api_key_.length() << ")\n";
    }

    if (secret_key_.empty()) {
        std::cout << "[AlpacaOrderExecutor] ⚠️  No Secret key configured (ALPACA_SECRET_KEY env var)\n";
    } else {
        std::cout << "[AlpacaOrderExecutor] Secret Key: " << secret_key_.substr(0, 4) << "****"
                  << secret_key_.substr(secret_key_.length() - 4) << " (length: " << secret_key_.length() << ")\n";
    }
    
    executor_thread_ = std::thread(&AlpacaOrderExecutor::executor_loop, this);
    std::cout << "[AlpacaOrderExecutor] Started (fire-and-forget mode with real HTTP)\n";
}

void AlpacaOrderExecutor::shutdown() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    if (executor_thread_.joinable()) {
        executor_thread_.join();
    }
    
    std::cout << "[AlpacaOrderExecutor] Shutdown complete\n";
}

bool AlpacaOrderExecutor::sendOrder(const Order& order) {
    // Fire-and-forget: simplesmente enfileira e retorna
    {
        std::lock_guard<std::mutex> lock(pending_orders_mutex_);
        pending_orders_.push(order);
    }
    
    std::cout << "[AlpacaOrderExecutor] Order enqueued: " 
              << order.asset_id << " " << order.side << " " << order.quantity << " @ $" << order.price << "\n";
    
    return true;
}

int AlpacaOrderExecutor::sendOrders(const std::vector<Order>& orders) {
    int count = 0;
    
    {
        std::lock_guard<std::mutex> lock(pending_orders_mutex_);
        for (const auto& order : orders) {
            pending_orders_.push(order);
            count++;
        }
    }
    
    std::cout << "[AlpacaOrderExecutor] Batch of " << count << " orders enqueued\n";
    return count;
}

bool AlpacaOrderExecutor::cancelOrder(uint64_t order_id) {
    std::lock_guard<std::mutex> lock(sent_orders_mutex_);
    
    if (sent_orders_.find(order_id) == sent_orders_.end()) {
        std::cout << "[AlpacaOrderExecutor] Order not found: " << order_id << "\n";
        return false;
    }
    
    std::cout << "[AlpacaOrderExecutor] Order cancelled: " << order_id << "\n";
    return true;
}

OrderStatus AlpacaOrderExecutor::getOrderStatus(uint64_t order_id) {
    std::lock_guard<std::mutex> lock(sent_orders_mutex_);
    
    if (sent_orders_.find(order_id) != sent_orders_.end()) {
        return OrderStatus{true, "Order found"};
    }
    
    return OrderStatus{false, "Order pending"};
}

void AlpacaOrderExecutor::executor_loop() {
    std::cout << "[AlpacaOrderExecutor] Executor loop started (background thread)\n";
    std::cout << "[AlpacaOrderExecutor] API Key: " << (api_key_.empty() ? "NOT SET" : "***hidden***") << "\n";
    
    while (running_) {
        process_batch();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "[AlpacaOrderExecutor] Executor loop finished\n";
}

void AlpacaOrderExecutor::process_batch() {
    std::vector<Order> batch;
    
    {
        std::lock_guard<std::mutex> lock(pending_orders_mutex_);
        
        while (!pending_orders_.empty() && (int)batch.size() < max_batch_size_) {
            batch.push_back(pending_orders_.front());
            pending_orders_.pop();
        }
    }
    
    if (batch.empty()) {
        return;
    }
    
    std::cout << "[AlpacaOrderExecutor] Processing batch of " << batch.size() << " orders\n";
    
    for (const auto& order : batch) {
        if (submit_to_alpaca(order)) {
            std::lock_guard<std::mutex> lock(sent_orders_mutex_);
            sent_orders_[order.event_id] = order;
            std::cout << "[AlpacaOrderExecutor] ✓ Order submitted to Alpaca\n";
        } else {
            std::cout << "[AlpacaOrderExecutor] ✗ Failed to submit order\n";
        }
    }
}

bool AlpacaOrderExecutor::submit_to_alpaca(const Order& order) {
    if (api_key_.empty()) {
        std::cout << "[AlpacaOrderExecutor] ERROR: ALPACA_API_KEY not set!\n";
        return false;
    }
    if (secret_key_.empty()) {
        std::cout << "[AlpacaOrderExecutor] ERROR: ALPACA_SECRET_KEY not set!\n";
        return false;
    }
    
    std::string account_response;
    if (!alpaca_get_account(account_response)) {
        return false;
    }

    std::string order_response;
    if (!alpaca_submit_order(order, order_response)) {
        return false;
    }

    if (order_response.find("\"id\"") != std::string::npos) {
        std::cout << "[AlpacaOrderExecutor] Order accepted: " << order.asset_id << "\n";
        return true;
    }

    std::cout << "[AlpacaOrderExecutor] Alpaca rejected: " << order_response << "\n";
    return false;
}

bool AlpacaOrderExecutor::alpaca_get_account(std::string& response) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cout << "[AlpacaOrderExecutor] ERROR: Failed to init CURL (account check)\n";
        return false;
    }

    const std::string api_key_header = "APCA-API-KEY-ID: " + api_key_;
    const std::string secret_key_header = "APCA-API-SECRET-KEY: " + secret_key_;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "accept: application/json");
    headers = curl_slist_append(headers, api_key_header.c_str());
    headers = curl_slist_append(headers, secret_key_header.c_str());

    const std::string url = base_url_ + "/v2/account";
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cout << "[AlpacaOrderExecutor] Account check failed: " << curl_easy_strerror(res) << "\n";
        return false;
    }

    return true;
}

bool AlpacaOrderExecutor::alpaca_submit_order(const Order& order, std::string& response) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cout << "[AlpacaOrderExecutor] ERROR: Failed to init CURL (order submit)\n";
        return false;
    }

    const std::string api_key_header = "APCA-API-KEY-ID: " + api_key_;
    const std::string secret_key_header = "APCA-API-SECRET-KEY: " + secret_key_;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "accept: application/json");
    headers = curl_slist_append(headers, "content-type: application/json");
    headers = curl_slist_append(headers, api_key_header.c_str());
    headers = curl_slist_append(headers, secret_key_header.c_str());

    std::string normalized_side = order.side;
    std::transform(normalized_side.begin(), normalized_side.end(), normalized_side.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    const std::string payload =
        "{\"symbol\":\"" + order.asset_id +
        "\",\"qty\":" + std::to_string(static_cast<int>(order.quantity)) +
        ",\"side\":\"" + normalized_side +
        "\",\"type\":\"market\",\"time_in_force\":\"day\"}";

    const std::string url = base_url_ + "/v2/orders";
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cout << "[AlpacaOrderExecutor] Order submit failed: " << curl_easy_strerror(res) << "\n";
        return false;
    }

    return true;
}

} // namespace hft::orders::execution
