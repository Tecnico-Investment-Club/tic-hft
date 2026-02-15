#include "BinanceClient.hpp"

#include <curl/curl.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <ctime>

namespace hft::orders {

// Callback for curl response
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    s->append((char*)contents, size * nmemb);
    return size * nmemb;
}

BinanceClient::BinanceClient(const std::string& api_key, 
                           const std::string& api_secret,
                           bool testnet)
    : api_key_(api_key), api_secret_(api_secret), testnet_(testnet) {
    
    if (testnet) {
        base_url_ = "https://testnet.binance.vision/api";
    } else {
        base_url_ = "https://api.binance.com/api";
    }
    
    curl_global_init(CURL_GLOBAL_DEFAULT);
    std::cout << "BinanceClient initialized (testnet=" << testnet << ")" << std::endl;
}

BinanceClient::~BinanceClient() {
    curl_global_cleanup();
}

std::string BinanceClient::generate_signature(const std::string& data) {
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashlen = 0;
    
    HMAC(EVP_sha256(),
         api_secret_.c_str(), api_secret_.length(),
         (unsigned char*)data.c_str(), data.length(),
         hash, &hashlen);
    
    std::stringstream ss;
    for (unsigned int i = 0; i < hashlen; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    
    return ss.str();
}

json BinanceClient::http_get(const std::string& endpoint, const std::string& params) {
    std::string url = base_url_ + endpoint + "?" + params;
    std::string signature = generate_signature(params);
    url += "&signature=" + signature;
    
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("Failed to initialize CURL");
    
    std::string response;
    std::string auth_header = "X-MBX-APIKEY: " + api_key_;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, auth_header.c_str());
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    
    CURLcode res = curl_easy_perform(curl);
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        throw std::runtime_error("CURL request failed: " + std::string(curl_easy_strerror(res)));
    }
    
    return json::parse(response);
}

json BinanceClient::http_post(const std::string& endpoint, const std::string& params) {
    std::string signature = generate_signature(params);
    std::string body = params + "&signature=" + signature;
    
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("Failed to initialize CURL");
    
    std::string response;
    std::string auth_header = "X-MBX-APIKEY: " + api_key_;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, auth_header.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    
    curl_easy_setopt(curl, CURLOPT_URL, (base_url_ + endpoint).c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    
    CURLcode res = curl_easy_perform(curl);
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        throw std::runtime_error("CURL request failed: " + std::string(curl_easy_strerror(res)));
    }
    
    return json::parse(response);
}

uint64_t BinanceClient::place_limit_order(const std::string& symbol,
                                         OrderSide side,
                                         double quantity,
                                         double price) {
    std::stringstream ss;
    ss << "symbol=" << symbol
       << "&side=" << (side == OrderSide::BUY ? "BUY" : "SELL")
       << "&type=LIMIT"
       << "&timeInForce=GTC"
       << "&quantity=" << quantity
       << "&price=" << price
       << "&timestamp=" << (long)std::time(nullptr) * 1000;
    
    auto response = http_post("/v3/order", ss.str());
    
    return response["orderId"].get<uint64_t>();
}

uint64_t BinanceClient::place_market_order(const std::string& symbol,
                                          OrderSide side,
                                          double quantity) {
    std::stringstream ss;
    ss << "symbol=" << symbol
       << "&side=" << (side == OrderSide::BUY ? "BUY" : "SELL")
       << "&type=MARKET"
       << "&quantity=" << quantity
       << "&timestamp=" << (long)std::time(nullptr) * 1000;
    
    auto response = http_post("/v3/order", ss.str());
    
    return response["orderId"].get<uint64_t>();
}

json BinanceClient::get_order_status(const std::string& symbol,
                                    uint64_t binance_order_id) {
    std::stringstream ss;
    ss << "symbol=" << symbol
       << "&orderId=" << binance_order_id
       << "&timestamp=" << (long)std::time(nullptr) * 1000;
    
    return http_get("/v3/order", ss.str());
}

json BinanceClient::cancel_order(const std::string& symbol,
                                uint64_t binance_order_id) {
    std::stringstream ss;
    ss << "symbol=" << symbol
       << "&orderId=" << binance_order_id
       << "&timestamp=" << (long)std::time(nullptr) * 1000;
    
    return http_post("/v3/order", ss.str());
}

} // namespace hft::orders
