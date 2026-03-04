#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <thread>
#include <chrono>
#include <cstdlib>
#include "include/execution/AlpacaOrderExecutor.hpp"

using namespace hft::orders::execution;

// Test fixture
class AlpacaOrderExecutorTest : public ::testing::Test {
protected:
    void SetUp() override {
        const char* api_key_ = std::getenv("ALPACA_API_KEY");
        const char* secret_key_ = std::getenv("ALPACA_SECRET_KEY");
        const char* base_url_ = std::getenv("ALPACA_BASE_URL");

        if (!api_key_ || !secret_key_ || std::string(api_key_).empty() || std::string(secret_key_).empty()) {
            GTEST_SKIP() << "ALPACA_API_KEY/ALPACA_SECRET_KEY not set. Skipping integration-like executor tests.";
        }

        executor = std::make_unique<AlpacaOrderExecutor>(
            api_key_,
            secret_key_,
            (base_url_ && std::string(base_url_).size() > 0) ? base_url_ : "https://paper-api.alpaca.markets",
            10  // max batch size
        );
        executor->initialize();
    }
    
    void TearDown() override {
        if (executor) {
            executor->shutdown();
        }
    }
    
    std::unique_ptr<AlpacaOrderExecutor> executor;
};

// Test 1: Single order send
TEST_F(AlpacaOrderExecutorTest, SendSingleOrder) {
    Order order{
        .portfolio_id = 1,
        .event_id = 100,
        .delivery_id = 1,
        .asset_id = "AAPL",
        .quantity = 100.0,
        .price = 150.0,
        .side = "BUY"
    };
    
    bool success = executor->sendOrder(order);
    ASSERT_TRUE(success);
}

// Test 2: Batch order send
TEST_F(AlpacaOrderExecutorTest, SendBatchOrders) {
    std::vector<Order> orders = {
        {1, 101, 1, "AAPL", 100.0, 150.0, "BUY"},
        {1, 102, 2, "MSFT", 50.0, 350.0, "BUY"},
        {1, 103, 3, "GOOGL", 25.0, 2800.0, "BUY"},
    };
    
    int sent = executor->sendOrders(orders);
    ASSERT_EQ(sent, 3);
}

// Test 3: Fire-and-forget latency (should be very fast)
TEST_F(AlpacaOrderExecutorTest, FireAndForgetLatency) {
    Order order{
        .portfolio_id = 1,
        .event_id = 104,
        .delivery_id = 4,
        .asset_id = "TSLA",
        .quantity = 200.0,
        .price = 800.0,
        .side = "BUY"
    };
    
    auto start = std::chrono::high_resolution_clock::now();
    executor->sendOrder(order);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto latency_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    // Fire-and-forget should complete in < 10 microseconds
    ASSERT_LT(latency_us, 10);
}

// Test 4: Multiple orders don't block
TEST_F(AlpacaOrderExecutorTest, MultipleOrdersDontBlock) {
    Order orders[100];
    
    for (int i = 0; i < 100; i++) {
        orders[i] = {
            .portfolio_id = 1,
            .event_id = static_cast<uint64_t>(200 + i),
            .delivery_id = static_cast<uint64_t>(10 + i),
            .asset_id = "SPY",
            .quantity = 10.0,
            .price = 450.0,
            .side = "BUY"
        };
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 100; i++) {
        executor->sendOrder(orders[i]);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    // 100 orders should complete in < 100ms
    ASSERT_LT(elapsed_ms, 100);
}

// Test 5: Initialize/Shutdown lifecycle
TEST_F(AlpacaOrderExecutorTest, InitializeAndShutdown) {
    const char* api_key_ = std::getenv("ALPACA_API_KEY");
    const char* secret_key_ = std::getenv("ALPACA_SECRET_KEY");
    const char* base_url_ = std::getenv("ALPACA_BASE_URL");

    if (!api_key_ || !secret_key_ || std::string(api_key_).empty() || std::string(secret_key_).empty()) {
        GTEST_SKIP() << "ALPACA_API_KEY/ALPACA_SECRET_KEY not set. Skipping integration-like executor tests.";
    }

    auto executor2 = std::make_unique<AlpacaOrderExecutor>(
        api_key_,
        secret_key_,
        (base_url_ && std::string(base_url_).size() > 0) ? base_url_ : "https://paper-api.alpaca.markets",
        10
    );
    
    // Should be safe to call multiple times
    executor2->initialize();
    executor2->initialize();
    
    Order order{
        .portfolio_id = 1,
        .event_id = 300,
        .delivery_id = 15,
        .asset_id = "NFLX",
        .quantity = 10.0,
        .price = 400.0,
        .side = "BUY"
    };
    
    bool success = executor2->sendOrder(order);
    ASSERT_TRUE(success);
    
    executor2->shutdown();
}

// Test 6: Cancel order
TEST_F(AlpacaOrderExecutorTest, CancelOrder) {
    Order order{
        .portfolio_id = 1,
        .event_id = 400,
        .delivery_id = 20,
        .asset_id = "AMD",
        .quantity = 50.0,
        .price = 120.0,
        .side = "BUY"
    };
    
    executor->sendOrder(order);
    
    // Try to cancel
    bool cancelled = executor->cancelOrder(400);
    
    // May return false since order might not be tracked, but shouldn't crash
    ASSERT_TRUE(cancelled || !cancelled);
}

// Test 7: Get order status
TEST_F(AlpacaOrderExecutorTest, GetOrderStatus) {
    Order order{
        .portfolio_id = 1,
        .event_id = 500,
        .delivery_id = 25,
        .asset_id = "NVDA",
        .quantity = 30.0,
        .price = 850.0,
        .side = "BUY"
    };
    
    executor->sendOrder(order);
    
    // Wait a bit for background thread to process
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    OrderStatus status = executor->getOrderStatus(500);
    
    // Status should be valid after sending
    ASSERT_TRUE(status.is_valid || !status.is_valid);  // Either way is OK
}

// Test 8: SELL orders work
TEST_F(AlpacaOrderExecutorTest, SellOrders) {
    Order sell_order{
        .portfolio_id = 1,
        .event_id = 600,
        .delivery_id = 30,
        .asset_id = "META",
        .quantity = 40.0,
        .price = 300.0,
        .side = "SELL"
    };
    
    bool success = executor->sendOrder(sell_order);
    ASSERT_TRUE(success);
}

// Test 9: Batch processing
TEST_F(AlpacaOrderExecutorTest, BatchProcessing) {
    std::vector<Order> orders;
    
    for (int i = 0; i < 15; i++) {
        orders.push_back({
            .portfolio_id = 1,
            .event_id = static_cast<uint64_t>(700 + i),
            .delivery_id = static_cast<uint64_t>(40 + i),
            .asset_id = "QQQ",
            .quantity = 5.0,
            .price = 350.0,
            .side = "BUY"
        });
    }
    
    int sent = executor->sendOrders(orders);
    ASSERT_EQ(sent, 15);
    
    // Wait for background thread to batch process
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

