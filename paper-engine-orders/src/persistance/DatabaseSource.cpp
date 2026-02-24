#include "persistance/DatabaseSource.hpp"
#include <iostream>
#include <sstream>

namespace hft::paper_engine {

DatabaseSource::DatabaseSource(const std::string& connection_string)
    : connection_string_(connection_string) {}

DatabaseSource::~DatabaseSource() {
    disconnect();
}

void DatabaseSource::connect() {
    if (is_connected()) return;
    
    try {
        connection_ = std::make_unique<pqxx::connection>(connection_string_);
        if (!connection_->is_open()) {
            throw std::runtime_error("Failed to open database connection");
        }
        std::cout << "[DatabaseSource] Connected to PostgreSQL" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[DatabaseSource] Connection error: " << e.what() << std::endl;
        throw;
    }
}

void DatabaseSource::disconnect() {
    if (connection_ && connection_->is_open()) {
        connection_->disconnect();
        std::cout << "[DatabaseSource] Disconnected from PostgreSQL" << std::endl;
    }
}

bool DatabaseSource::is_connected() const {
    return connection_ && connection_->is_open();
}

std::vector<std::map<std::string, std::string>> DatabaseSource::query(const std::string& sql) {
    if (!is_connected()) {
        throw std::runtime_error("Database not connected");
    }
    
    std::vector<std::map<std::string, std::string>> results;
    
    try {
        pqxx::work txn(*connection_);
        pqxx::result res = txn.exec(sql);
        
        for (const auto& row : res) {
            std::map<std::string, std::string> row_map;
            for (int i = 0; i < static_cast<int>(row.size()); ++i) {
                auto value = row[i].as<std::string>();
                row_map[row[i].name()] = value;
            }
            results.push_back(row_map);
        }
        
        txn.commit();
    } catch (const std::exception& e) {
        std::cerr << "[DatabaseSource] Query error: " << e.what() << std::endl;
        throw;
    }
    
    return results;
}

void DatabaseSource::execute(const std::string& sql) {
    if (!is_connected()) {
        throw std::runtime_error("Database not connected");
    }
    
    try {
        pqxx::work txn(*connection_);
        txn.exec(sql);
        txn.commit();
        std::cout << "[DatabaseSource] Executed: " << sql.substr(0, 50) << "..." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[DatabaseSource] Execute error: " << e.what() << std::endl;
        throw;
    }
}

void DatabaseSource::init_tables(const std::string& sql_script) {
    if (!is_connected()) {
        throw std::runtime_error("Database not connected");
    }
    
    execute_sql_script(sql_script);
}

void DatabaseSource::execute_sql_script(const std::string& script) {
    if (!is_connected()) {
        throw std::runtime_error("Database not connected");
    }
    
    try {
        pqxx::work txn(*connection_);
        txn.exec(script);
        txn.commit();
        std::cout << "[DatabaseSource] Schema initialization completed" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[DatabaseSource] Schema error: " << e.what() << std::endl;
        throw;
    }
}

} // namespace hft::paper_engine
