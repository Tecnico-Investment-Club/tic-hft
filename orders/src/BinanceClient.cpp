#include "BinanceClient.hpp"

#include <curl/curl.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <iostream>

namespace hft::orders {

namespace {
    // CURL callback for writing response body
    static size_t write_callback(void* contents, size_t size, size_t nmemb, std::string* userp) {
        userp->append((char*)contents, size * nmemb);
        return size * nmemb;
    }
}

BinanceClient::BinanceClient(const std::string& api_key,
                             const std::string& api_secret,
                             bool testnet)
    : api_key_(api_key), api_secret_(api_secret), testnet_(testnet) {
    
    if (testnet) {
        base_url_ = "https://testnet.binance.vision";
    } else {
        base_url_ = "https://api.binance.com";
    }
    
    curl_global_init(CURL_GLOBAL_DEFAULT);
    std::cout << "BinanceClient initialized (testnet=" << testnet << ")" << std::endl;
}

BinanceClient::~BinanceClient() {
    curl_global_cleanup();
}

std::uint64_t BinanceClient::place_limit_order(const std::string& symbol,
                                               OrderSide side,
                                               double quantity,
                                               double price) {
    std::ostringstream oss;
    oss << "symbol=" << symbol 
        << "&side=" << (side == OrderSide::BUY ? "BUY" : "SELL")
        << "&type=LIMIT"
        << "&timeInForce=GTC"
        << "&quantity=" << std::fixed << std::setprecision(8) << quantity
        << "&price=" << std::fixed << std::setprecision(8) << price;
    
    json response = http_post("/api/v3/order", oss.str(), true);
    
    return response["orderId"].get<std::uint64_t>();
}

std::uint64_t BinanceClient::place_market_order(const std::string& symbol,
                                                OrderSide side,
                                                double quantity) {
    std::ostringstream oss;
    oss << "symbol=" << symbol 
        << "&side=" << (side == OrderSide::BUY ? "BUY" : "SELL")
        << "&type=MARKET"
        << "&quantity=" << std::fixed << std::setprecision(8) << quantity;
    
    json response = http_post("/api/v3/order", oss.str(), true);
    
    return response["orderId"].get<std::uint64_t>();
}

json BinanceClient::get_order_status(const std::string& symbol,
                                     std::uint64_t binance_order_id) {
    std::ostringstream oss;
    oss << "symbol=" << symbol << "&orderId=" << binance_order_id;
    
    return http_get("/api/v3/order", oss.str(), true);
}

json BinanceClient::cancel_order(const std::string& symbol,
                                 std::uint64_t binance_order_id) {
    std::ostringstream oss;
    oss << "symbol=" << symbol << "&orderId=" << binance_order_id;
    
    return http_post("/api/v3/order", oss.str(), true);
}

json BinanceClient::get_account() {
    return http_get("/api/v3/account", "", true);
}

json BinanceClient::http_get(const std::string& endpoint,
                             const std::string& params,
                             bool signed_request) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }
    
    std::string url = base_url_ + endpoint;
    
    std::string full_params = params;
    if (signed_request) {
        full_params += "&timestamp=" + std::to_string(get_server_time());
        std::string signature = generate_signature(full_params, api_secret_);
        full_params += "&signature=" + signature;
    }
    
    url += "?" + full_params;
    
    std::string response_body;
    std::string headers_str = "X-MBX-APIKEY: " + api_key_;
    
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, headers_str.c_str());
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    
    CURLcode res = curl_easy_perform(curl);
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        throw std::runtime_error("CURL request failed: " + std::string(curl_easy_strerror(res)));
    }
    
    return json::parse(response_body);
}

json BinanceClient::http_post(const std::string& endpoint,
                              const std::string& params,
                              bool signed_request) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }
    
    std::string url = base_url_ + endpoint;
    
    std::string post_data = params;
    if (signed_request) {
        post_data += "&timestamp=" + std::to_string(get_server_time());
        std::string signature = generate_signature(post_data, api_secret_);
        post_data += "&signature=" + signature;
    }
    
    std::string response_body;
    std::string headers_str = "X-MBX-APIKEY: " + api_key_;
    
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, headers_str.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    
    CURLcode res = curl_easy_perform(curl);
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        throw std::runtime_error("CURL POST failed: " + std::string(curl_easy_strerror(res)));
    }
    
    return json::parse(response_body);
}

std::string BinanceClient::generate_signature(const std::string& message,
                                              const std::string& secret) {
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len = 0;
    
    HMAC(EVP_sha256(),
         (unsigned char*)secret.c_str(), secret.length(),
         (unsigned char*)message.c_str(), message.length(),
         hash, &hash_len);
    
    std::ostringstream oss;
    for (unsigned int i = 0; i < hash_len; i++) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    
    return oss.str();
}

std::uint64_t BinanceClient::get_server_time() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    return millis.count();
}

} // namespace hft::orders
