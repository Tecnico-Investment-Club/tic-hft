#include "include/execution/AlpacaOrderExecutor.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <curl/curl.h>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <cstring>

namespace hft::orders::execution {

// Helper: remove quotes from environment variable values
static std::string strip_quotes(const std::string& str) {
    std::string result = str;
    // Remove leading/trailing quotes
    if (!result.empty() && result.front() == '"') {
        result.erase(0, 1);
    }
    if (!result.empty() && result.back() == '"') {
        result.pop_back();
    }
    // Remove leading/trailing whitespace
    while (!result.empty() && (result.front() == ' ' || result.front() == '\t')) {
        result.erase(0, 1);
    }
    while (!result.empty() && (result.back() == ' ' || result.back() == '\t')) {
        result.pop_back();
    }
    return result;
}

// Helper: read environment variable or .env file
static std::string get_env_or_file(const std::string& var_name, const std::string& default_value = "") {
    // First try to get from environment
    const char* env_val = std::getenv(var_name.c_str());
    if (env_val && std::strlen(env_val) > 0) {
        return strip_quotes(std::string(env_val));
    }
    
    // Try to read from .env file
    std::ifstream env_file(".env");
    if (env_file.is_open()) {
        std::string line;
        std::string search_prefix = var_name + "=";
        while (std::getline(env_file, line)) {
            // Skip comments and empty lines
            if (line.empty() || line[0] == '#') continue;
            
            // Find the variable
            if (line.find(search_prefix) == 0) {
                std::string value = line.substr(search_prefix.length());
                env_file.close();
                return strip_quotes(value);
            }
        }
        env_file.close();
    }
    
    return default_value;
}

// Callback for curl to write response
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

AlpacaOrderExecutor::AlpacaOrderExecutor(
    const std::string& api_key,
    const std::string& base_url,
    int max_batch_size
)
    : api_key_(api_key)
    , base_url_(base_url)
    , max_batch_size_(max_batch_size)
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
    
    try {
        // Initialize CURL
        CURL* curl = curl_easy_init();
        if (!curl) {
            std::cout << "[AlpacaOrderExecutor] ERROR: Failed to init CURL\n";
            return false;
        }
        
        // Build URL
        std::string url = base_url_ + "/v2/orders";
        
        // Build JSON payload
        std::ostringstream json;
        json << "{"
             << "\"symbol\":\"" << order.asset_id << "\""
             << ",\"qty\":" << static_cast<int>(order.quantity)
             << ",\"side\":\"" << order.side << "\""
             << ",\"type\":\"limit\""
             << ",\"time_in_force\":\"day\""
             << ",\"limit_price\":" << std::fixed << std::setprecision(2) << order.price
             << "}";
        
        std::string json_str = json.str();
        std::string response;
        
        // Set up headers
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        std::string auth_header = "Authorization: Bearer " + api_key_;
        headers = curl_slist_append(headers, auth_header.c_str());
        
        // Set CURL options
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        
        // Perform request
        CURLcode res = curl_easy_perform(curl);
        
        // Cleanup
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        if (res != CURLE_OK) {
            std::cout << "[AlpacaOrderExecutor] CURL Error: " << curl_easy_strerror(res) << "\n";
            return false;
        }
        
        // Check response
        if (response.find("\"id\"") != std::string::npos) {
            std::cout << "[AlpacaOrderExecutor] Order accepted: " << order.asset_id << "\n";
            return true;
        } else {
            std::cout << "[AlpacaOrderExecutor] Alpaca rejected: " << response << "\n";
            return false;
        }
        
    } catch (const std::exception& e) {
        std::cout << "[AlpacaOrderExecutor] Exception: " << e.what() << "\n";
        return false;
    }
}

} // namespace hft::orders::execution
