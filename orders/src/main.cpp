#include <iostream>
#include <string>
#include <cstdlib>
#include <csignal>
#include <memory>

#include "OrdersManager.hpp"

using namespace hft::orders;

static std::unique_ptr<OrdersManager> g_manager = nullptr;

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nShutting down..." << std::endl;
        if (g_manager) {
            g_manager->shutdown();
        }
        exit(0);
    }
}

struct Config {
    std::string api_key;
    std::string api_secret;
    bool dry_run = false;
    int min_sleep = 1;
    int max_sleep = 5;
    std::string log_file = "";
};

Config parse_arguments(int argc, char* argv[]) {
    Config config;
    
    const char* api_key = std::getenv("BINANCE_API_KEY");
    if (api_key) config.api_key = api_key;
    
    const char* api_secret = std::getenv("BINANCE_API_SECRET");
    if (api_secret) config.api_secret = api_secret;
    
    const char* dry_run = std::getenv("DRY_RUN");
    if (dry_run && std::string(dry_run) == "true") config.dry_run = true;
    
    const char* min_sleep = std::getenv("MIN_SLEEP");
    if (min_sleep) config.min_sleep = std::atoi(min_sleep);
    
    const char* max_sleep = std::getenv("MAX_SLEEP");
    if (max_sleep) config.max_sleep = std::atoi(max_sleep);

    const char* log_file = std::getenv("LOG_FILE");
    if (log_file) config.log_file = log_file;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--dry-run") {
            config.dry_run = true;
        } else if (arg == "--min-sleep" && i + 1 < argc) {
            config.min_sleep = std::atoi(argv[++i]);
        } else if (arg == "--max-sleep" && i + 1 < argc) {
            config.max_sleep = std::atoi(argv[++i]);
        } else if (arg == "--log-file" && i + 1 < argc) {
            config.log_file = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "HFT Orders Manager\n\n"
                      << "Options:\n"
                      << "  --dry-run           Dry-run mode\n"
                      << "  --min-sleep MS      Min sleep (ms)\n"
                      << "  --max-sleep MS      Max sleep (ms)\n"
                      << "  --log-file FILE     Order log file\n"
                      << "  --help              Show this help\n\n"
                      << "Environment:\n"
                      << "  BINANCE_API_KEY\n"
                      << "  BINANCE_API_SECRET\n"
                      << "  DRY_RUN\n"
                      << "  MIN_SLEEP\n"
                      << "  MAX_SLEEP\n"
                      << "  LOG_FILE\n";
            exit(0);
        }
    }
    
    return config;
}

int main(int argc, char* argv[]) {
    try {
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);
        
        std::cout << "=== HFT Orders Manager ===" << std::endl;
        std::cout << "Version 1.0 (Simplified)" << std::endl << std::endl;
        
        Config config = parse_arguments(argc, argv);
        
        std::cout << "API Key: " << (config.api_key.empty() ? "NOT SET" : "SET") << std::endl;
        std::cout << "Dry Run: " << (config.dry_run ? "YES" : "NO") << std::endl;
        std::cout << "Min Sleep: " << config.min_sleep << "ms" << std::endl;
        std::cout << "Max Sleep: " << config.max_sleep << "ms" << std::endl;
        if (!config.log_file.empty()) {
            std::cout << "Log File: " << config.log_file << std::endl;
        }
        std::cout << std::endl;
        
        g_manager = std::make_unique<OrdersManager>(
            config.api_key,
            config.api_secret,
            config.dry_run,
            config.min_sleep,
            config.max_sleep,
            config.log_file
        );
        
        std::cout << "Starting..." << std::endl;
        g_manager->run();
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
