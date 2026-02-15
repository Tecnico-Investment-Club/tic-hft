#include <iostream>
#include <string>
#include <cstdlib>
#include <csignal>
#include <memory>

#include "OrdersManager.hpp"

using namespace hft::orders;

// Global pointer for signal handler
static std::unique_ptr<OrdersManager> g_manager = nullptr;

/**
 * Signal handler for graceful shutdown
 */
void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nReceived shutdown signal, shutting down gracefully..." << std::endl;
        if (g_manager) {
            g_manager->shutdown();
        }
        exit(0);
    }
}

/**
 * Parse command line arguments
 */
struct Config {
    std::string db_connection_string;
    std::string api_key;
    std::string api_secret;
    bool dry_run = false;
    int min_sleep = 1;
    int max_sleep = 5;
};

Config parse_arguments(int argc, char* argv[]) {
    Config config;
    
    // Get from environment variables
    const char* db_url = std::getenv("DATABASE_URL");
    if (db_url) {
        config.db_connection_string = db_url;
    } else {
        config.db_connection_string = "postgresql://tic_strat_app:imaginemterumadb123@localhost:5432/tic";
    }
    
    const char* api_key = std::getenv("BINANCE_API_KEY");
    if (api_key) {
        config.api_key = api_key;
    }
    
    const char* api_secret = std::getenv("BINANCE_API_SECRET");
    if (api_secret) {
        config.api_secret = api_secret;
    }
    
    const char* dry_run = std::getenv("DRY_RUN");
    if (dry_run && std::string(dry_run) == "true") {
        config.dry_run = true;
    }
    
    const char* min_sleep = std::getenv("MIN_SLEEP");
    if (min_sleep) {
        config.min_sleep = std::atoi(min_sleep);
    }
    
    const char* max_sleep = std::getenv("MAX_SLEEP");
    if (max_sleep) {
        config.max_sleep = std::atoi(max_sleep);
    }
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--dry-run") {
            config.dry_run = true;
        } else if (arg == "--min-sleep" && i + 1 < argc) {
            config.min_sleep = std::atoi(argv[++i]);
        } else if (arg == "--max-sleep" && i + 1 < argc) {
            config.max_sleep = std::atoi(argv[++i]);
        } else if (arg == "--db" && i + 1 < argc) {
            config.db_connection_string = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "HFT Orders Manager\n\n"
                      << "Usage: hft_orders [options]\n\n"
                      << "Options:\n"
                      << "  --dry-run           Run in dry-run mode (no real orders)\n"
                      << "  --min-sleep MS      Minimum sleep between iterations\n"
                      << "  --max-sleep MS      Maximum sleep between iterations\n"
                      << "  --db CONNECTION     Database connection string\n"
                      << "  --help              Show this help message\n\n"
                      << "Environment Variables:\n"
                      << "  DATABASE_URL        PostgreSQL connection string\n"
                      << "  BINANCE_API_KEY     Binance API key\n"
                      << "  BINANCE_API_SECRET  Binance API secret\n"
                      << "  DRY_RUN             Set to 'true' for dry-run mode\n"
                      << "  MIN_SLEEP           Min sleep in milliseconds\n"
                      << "  MAX_SLEEP           Max sleep in milliseconds\n";
            exit(0);
        }
    }
    
    return config;
}

int main(int argc, char* argv[]) {
    try {
        // Setup signal handlers
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);
        
        std::cout << "=== HFT Orders Manager ===" << std::endl;
        std::cout << "Version 0.1.0" << std::endl;
        std::cout << std::endl;
        
        // Parse configuration
        Config config = parse_arguments(argc, argv);
        
        std::cout << "Configuration:" << std::endl;
        std::cout << "  Database: " << config.db_connection_string.substr(0, 30) << "..." << std::endl;
        std::cout << "  API Key: " << (config.api_key.empty() ? "NOT SET" : "SET") << std::endl;
        std::cout << "  Dry Run: " << (config.dry_run ? "YES" : "NO") << std::endl;
        std::cout << "  Min Sleep: " << config.min_sleep << "ms" << std::endl;
        std::cout << "  Max Sleep: " << config.max_sleep << "ms" << std::endl;
        std::cout << std::endl;
        
        // Create and run manager
        g_manager = std::make_unique<OrdersManager>(
            config.db_connection_string,
            config.api_key,
            config.api_secret,
            config.dry_run,
            config.min_sleep,
            config.max_sleep
        );
        
        std::cout << "Starting orders manager..." << std::endl;
        g_manager->run();
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
