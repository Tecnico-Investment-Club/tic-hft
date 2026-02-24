#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <pqxx/pqxx>

namespace hft::paper_engine {

/**
 * DatabaseSource: Manages PostgreSQL connection and data retrieval
 * Equivalent to Python's source.Source from tic-strategy-app/alpaca
 */
class DatabaseSource {
public:
    explicit DatabaseSource(const std::string& connection_string);
    ~DatabaseSource();
    
    void connect();
    void disconnect();
    bool is_connected() const;
    
    // Execute SQL query and return raw results
    std::vector<std::map<std::string, std::string>> query(const std::string& sql);
    
    // Execute SQL command (INSERT, UPDATE, DELETE)
    void execute(const std::string& sql);
    
    // Initialize tables from SQL script
    void init_tables(const std::string& sql_script);

private:
    std::string connection_string_;
    std::unique_ptr<pqxx::connection> connection_;
    
    void execute_sql_script(const std::string& script);
};

} // namespace hft::paper_engine
