#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <functional>
#include <vector>
#include <cstdint>
#include <sqlite3.h>

namespace dda {
namespace database {

class DbConnection {
public:
    explicit DbConnection(const std::string& db_path);
    ~DbConnection();

    // Non-copyable
    DbConnection(const DbConnection&) = delete;
    DbConnection& operator=(const DbConnection&) = delete;

    // Movable
    DbConnection(DbConnection&& other) noexcept;
    DbConnection& operator=(DbConnection&& other) noexcept;

    sqlite3* get() const { return db_; }
    bool is_open() const { return db_ != nullptr; }

    void execute(const std::string& sql);
    void execute_with_params(const std::string& sql, 
                             const std::function<void(sqlite3_stmt*)>& bind_params);

private:
    sqlite3* db_ = nullptr;
    std::string path_;
};

class DbManager {
public:
    static DbManager& instance();

    void initialize(const std::string& config_db_path, const std::string& metadata_db_path);
    void run_migrations();

    // Get a connection for configuration database
    std::unique_ptr<DbConnection> get_config_connection();
    
    // Get a connection for metadata database  
    std::unique_ptr<DbConnection> get_metadata_connection();

    const std::string& config_db_path() const { return config_db_path_; }
    const std::string& metadata_db_path() const { return metadata_db_path_; }

private:
    DbManager() = default;

    void create_config_tables(DbConnection& conn);
    void create_metadata_tables(DbConnection& conn);

    std::string config_db_path_;
    std::string metadata_db_path_;
    std::mutex config_mutex_;
    std::mutex metadata_mutex_;
    bool initialized_ = false;
};

// RAII wrapper for prepared statements
class PreparedStatement {
public:
    PreparedStatement(sqlite3* db, const std::string& sql);
    ~PreparedStatement();

    // Non-copyable
    PreparedStatement(const PreparedStatement&) = delete;
    PreparedStatement& operator=(const PreparedStatement&) = delete;

    void bind_int(int index, int value);
    void bind_int64(int index, int64_t value);
    void bind_double(int index, double value);
    void bind_text(int index, const std::string& value);
    void bind_blob(int index, const void* data, size_t size);
    void bind_null(int index);

    bool step();  // Returns true if there's a row
    void reset();

    int get_int(int column);
    int64_t get_int64(int column);
    double get_double(int column);
    std::string get_text(int column);
    std::vector<uint8_t> get_blob(int column);
    bool is_null(int column);

    sqlite3_stmt* get() const { return stmt_; }

private:
    sqlite3_stmt* stmt_ = nullptr;
};

// Transaction RAII helper
class Transaction {
public:
    explicit Transaction(DbConnection& conn);
    ~Transaction();

    void commit();
    void rollback();

private:
    DbConnection& conn_;
    bool committed_ = false;
};

} // namespace database
} // namespace dda
