#include <iostream>
#include <string>
#include <cstdlib>
#include <csignal>
#include <memory>
#include "PaperEngineOrders.hpp"

using namespace hft::paper_engine;

static std::unique_ptr<PaperEngineOrders> g_engine = nullptr;

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\n\nShutting down..." << std::endl;
        if (g_engine) {
            g_engine->shutdown();
        }
        exit(0);
    }
}

struct Config {
    std::string db_source_connection;
    std::string db_target_connection;
    std::string alpaca_api_key;
    std::string alpaca_base_url;
    uint64_t portfolio_id = 1;
    int strategy_id = 1;
    double cash_allocation = 1000.0;
    bool dry_run = false;
};

Config parse_arguments(int argc, char* argv[]) {
    Config config;
    
    // Get environment variables
    const char* db_source = std::getenv("DB_SOURCE_CONNECTION");
    if (db_source) config.db_source_connection = db_source;
    else config.db_source_connection = "postgresql://user:password@localhost:5432/hft_db";
    
    const char* db_target = std::getenv("DB_TARGET_CONNECTION");
    if (db_target) config.db_target_connection = db_target;
    else config.db_target_connection = "postgresql://user:password@localhost:5432/hft_db";
    
    const char* api_key = std::getenv("ALPACA_API_KEY");
    if (api_key) config.alpaca_api_key = api_key;
    
    const char* api_base_url = std::getenv("ALPACA_BASE_URL");
    if (api_base_url) config.alpaca_base_url = api_base_url;
    else config.alpaca_base_url = "https://paper-api.alpaca.markets";  // Paper trading
    
    const char* portfolio_id = std::getenv("PORTFOLIO_ID");
    if (portfolio_id) config.portfolio_id = std::stoull(portfolio_id);
    
    const char* strategy_id = std::getenv("STRATEGY_ID");
    if (strategy_id) config.strategy_id = std::atoi(strategy_id);
    
    const char* cash_alloc = std::getenv("CASH_ALLOCATION");
    if (cash_alloc) config.cash_allocation = std::stod(cash_alloc);
    
    const char* dry_run = std::getenv("DRY_RUN");
    if (dry_run && std::string(dry_run) == "true") config.dry_run = true;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--dry-run") {
            config.dry_run = true;
        } else if (arg == "--portfolio-id" && i + 1 < argc) {
            config.portfolio_id = std::stoull(argv[++i]);
        } else if (arg == "--strategy-id" && i + 1 < argc) {
            config.strategy_id = std::atoi(argv[++i]);
        } else if (arg == "--cash-allocation" && i + 1 < argc) {
            config.cash_allocation = std::stod(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "HFT Paper Engine Orders (Alpaca)\n\n"
                      << "Options:\n"
                      << "  --portfolio-id ID       Portfolio ID (default: 1)\n"
                      << "  --strategy-id ID        Strategy ID (default: 1)\n"
                      << "  --cash-allocation AMOUNT Cash allocation in USD\n"
                      << "  --dry-run               Dry-run mode (no real orders)\n"
                      << "  --help                  Show this help\n\n"
                      << "Environment Variables:\n"
                      << "  DB_SOURCE_CONNECTION    PostgreSQL connection string (source)\n"
                      << "  DB_TARGET_CONNECTION    PostgreSQL connection string (target)\n"
                      << "  ALPACA_API_KEY          Alpaca API key (token)\n"
                      << "  ALPACA_BASE_URL         Alpaca base URL (default: paper-api.alpaca.markets)\n"
                      << "  PORTFOLIO_ID            Portfolio ID\n"
                      << "  STRATEGY_ID             Strategy ID\n"
                      << "  CASH_ALLOCATION         Cash allocation\n"
                      << "  DRY_RUN                 Set to 'true' for dry-run mode\n";
            exit(0);
        }
    }
    
    return config;
}

int main(int argc, char* argv[]) {
    try {
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);
        
        std::cout << "╔════════════════════════════════════════╗" << std::endl;
        std::cout << "║  HFT Paper Engine Orders (Alpaca)      ║" << std::endl;
        std::cout << "║  Version 1.0 (C++ with PostgreSQL)     ║" << std::endl;
        std::cout << "╚════════════════════════════════════════╝" << std::endl << std::endl;
        
        Config config = parse_arguments(argc, argv);
        
        std::cout << "Configuration:" << std::endl;
        std::cout << "  Portfolio ID: " << config.portfolio_id << std::endl;
        std::cout << "  Strategy ID: " << config.strategy_id << std::endl;
        std::cout << "  Cash Allocation: $" << config.cash_allocation << std::endl;
        std::cout << "  Alpaca API Key: " << (config.alpaca_api_key.empty() ? "NOT SET" : "SET") << std::endl;
        std::cout << "  Alpaca Base URL: " << config.alpaca_base_url << std::endl;
        std::cout << "  Dry Run: " << (config.dry_run ? "YES" : "NO") << std::endl;
        std::cout << "  DB Source: " << config.db_source_connection << std::endl;
        std::cout << "  DB Target: " << config.db_target_connection << std::endl << std::endl;
        
        // Create and initialize the paper engine
        g_engine = std::make_unique<PaperEngineOrders>(
            config.db_source_connection,
            config.db_target_connection,
            config.alpaca_api_key,
            config.alpaca_base_url,
            config.portfolio_id,
            config.strategy_id,
            config.dry_run
        );
        
        g_engine->set_cash_allocation(config.cash_allocation);
        
        // Setup and run
        g_engine->setup();
        std::cout << "\n[Main] Setup complete. Starting engine...\n" << std::endl;
        g_engine->run();
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n[ERROR] " << e.what() << std::endl;
        return 1;
    }
}
