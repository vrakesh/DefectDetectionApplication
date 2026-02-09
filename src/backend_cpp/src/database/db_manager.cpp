/**
 * @file db_manager.cpp
 * @brief Database manager implementation
 */

#include "database/db_manager.hpp"
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace dda {
namespace database {

// DbConnection implementation
DbConnection::DbConnection(const std::string& db_path) : path_(db_path) {
    int rc = sqlite3_open(db_path.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::string error = sqlite3_errmsg(db_);
        sqlite3_close(db_);
        db_ = nullptr;
        throw std::runtime_error("Cannot open database: " + error);
    }
    
    // Enable WAL mode for better concurrency
    execute("PRAGMA journal_mode=WAL");
    execute("PRAGMA busy_timeout=5000");
}

DbConnection::~DbConnection() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

DbConnection::DbConnection(DbConnection&& other) noexcept 
    : db_(other.db_), path_(std::move(other.path_)) {
    other.db_ = nullptr;
}

DbConnection& DbConnection::operator=(DbConnection&& other) noexcept {
    if (this != &other) {
        if (db_) {
            sqlite3_close(db_);
        }
        db_ = other.db_;
        path_ = std::move(other.path_);
        other.db_ = nullptr;
    }
    return *this;
}

void DbConnection::execute(const std::string& sql) {
    char* error_msg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &error_msg);
    if (rc != SQLITE_OK) {
        std::string error = error_msg ? error_msg : "Unknown error";
        sqlite3_free(error_msg);
        throw std::runtime_error("SQL execution error: " + error);
    }
}

void DbConnection::execute_with_params(const std::string& sql,
                                        const std::function<void(sqlite3_stmt*)>& bind_params) {
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        throw std::runtime_error("SQL prepare error: " + std::string(sqlite3_errmsg(db_)));
    }
    
    try {
        bind_params(stmt);
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
            throw std::runtime_error("SQL step error: " + std::string(sqlite3_errmsg(db_)));
        }
    } catch (...) {
        sqlite3_finalize(stmt);
        throw;
    }
    
    sqlite3_finalize(stmt);
}

// PreparedStatement implementation
PreparedStatement::PreparedStatement(sqlite3* db, const std::string& sql) {
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt_, nullptr);
    if (rc != SQLITE_OK) {
        throw std::runtime_error("SQL prepare error: " + std::string(sqlite3_errmsg(db)));
    }
}

PreparedStatement::~PreparedStatement() {
    if (stmt_) {
        sqlite3_finalize(stmt_);
    }
}

void PreparedStatement::bind_int(int index, int value) {
    sqlite3_bind_int(stmt_, index, value);
}

void PreparedStatement::bind_int64(int index, int64_t value) {
    sqlite3_bind_int64(stmt_, index, value);
}

void PreparedStatement::bind_double(int index, double value) {
    sqlite3_bind_double(stmt_, index, value);
}

void PreparedStatement::bind_text(int index, const std::string& value) {
    sqlite3_bind_text(stmt_, index, value.c_str(), -1, SQLITE_TRANSIENT);
}

void PreparedStatement::bind_blob(int index, const void* data, size_t size) {
    sqlite3_bind_blob(stmt_, index, data, static_cast<int>(size), SQLITE_TRANSIENT);
}

void PreparedStatement::bind_null(int index) {
    sqlite3_bind_null(stmt_, index);
}

bool PreparedStatement::step() {
    int rc = sqlite3_step(stmt_);
    if (rc == SQLITE_ROW) {
        return true;
    } else if (rc == SQLITE_DONE) {
        return false;
    } else {
        throw std::runtime_error("SQL step error");
    }
}

void PreparedStatement::reset() {
    sqlite3_reset(stmt_);
    sqlite3_clear_bindings(stmt_);
}

int PreparedStatement::get_int(int column) {
    return sqlite3_column_int(stmt_, column);
}

int64_t PreparedStatement::get_int64(int column) {
    return sqlite3_column_int64(stmt_, column);
}

double PreparedStatement::get_double(int column) {
    return sqlite3_column_double(stmt_, column);
}

std::string PreparedStatement::get_text(int column) {
    const unsigned char* text = sqlite3_column_text(stmt_, column);
    return text ? reinterpret_cast<const char*>(text) : "";
}

std::vector<uint8_t> PreparedStatement::get_blob(int column) {
    const void* data = sqlite3_column_blob(stmt_, column);
    int size = sqlite3_column_bytes(stmt_, column);
    if (data && size > 0) {
        const uint8_t* bytes = static_cast<const uint8_t*>(data);
        return std::vector<uint8_t>(bytes, bytes + size);
    }
    return {};
}

bool PreparedStatement::is_null(int column) {
    return sqlite3_column_type(stmt_, column) == SQLITE_NULL;
}

// Transaction implementation
Transaction::Transaction(DbConnection& conn) : conn_(conn) {
    conn_.execute("BEGIN TRANSACTION");
}

Transaction::~Transaction() {
    if (!committed_) {
        try {
            rollback();
        } catch (...) {
            // Ignore errors in destructor
        }
    }
}

void Transaction::commit() {
    if (!committed_) {
        conn_.execute("COMMIT");
        committed_ = true;
    }
}

void Transaction::rollback() {
    if (!committed_) {
        conn_.execute("ROLLBACK");
        committed_ = true;
    }
}

// DbManager implementation
DbManager& DbManager::instance() {
    static DbManager manager;
    return manager;
}

void DbManager::initialize(const std::string& config_db_path, const std::string& metadata_db_path) {
    std::lock_guard<std::mutex> lock1(config_mutex_);
    std::lock_guard<std::mutex> lock2(metadata_mutex_);
    
    config_db_path_ = config_db_path;
    metadata_db_path_ = metadata_db_path;
    
    // Create databases and tables
    {
        auto conn = std::make_unique<DbConnection>(config_db_path_);
        create_config_tables(*conn);
    }
    {
        auto conn = std::make_unique<DbConnection>(metadata_db_path_);
        create_metadata_tables(*conn);
    }
    
    initialized_ = true;
    spdlog::info("Database manager initialized");
}

void DbManager::run_migrations() {
    spdlog::info("Running database migrations...");
    // Migrations would be implemented here
    // For now, tables are created in initialize()
}

std::unique_ptr<DbConnection> DbManager::get_config_connection() {
    return std::make_unique<DbConnection>(config_db_path_);
}

std::unique_ptr<DbConnection> DbManager::get_metadata_connection() {
    return std::make_unique<DbConnection>(metadata_db_path_);
}

void DbManager::create_config_tables(DbConnection& conn) {
    // Image source configuration table
    conn.execute(R"(
        CREATE TABLE IF NOT EXISTS image_source_configuration (
            imageSourceConfigId TEXT PRIMARY KEY,
            gain INTEGER,
            exposure INTEGER,
            processingPipeline TEXT,
            creationTime INTEGER,
            imageCrop TEXT,
            device TEXT,
            deviceName TEXT
        )
    )");

    // Input configuration table
    conn.execute(R"(
        CREATE TABLE IF NOT EXISTS input_configuration (
            inputConfigurationId TEXT PRIMARY KEY,
            creationTime INTEGER,
            pin TEXT NOT NULL,
            triggerState TEXT NOT NULL,
            debounceTime INTEGER NOT NULL
        )
    )");

    // Output configuration table
    conn.execute(R"(
        CREATE TABLE IF NOT EXISTS output_configuration (
            outputConfigurationId TEXT PRIMARY KEY,
            pin TEXT NOT NULL,
            signalType TEXT NOT NULL,
            pulseWidth INTEGER NOT NULL,
            creationTime INTEGER,
            rule TEXT NOT NULL
        )
    )");

    // Image source table
    conn.execute(R"(
        CREATE TABLE IF NOT EXISTS image_source (
            imageSourceId TEXT PRIMARY KEY,
            name TEXT,
            type TEXT,
            location TEXT,
            cameraId TEXT,
            description TEXT,
            creationTime INTEGER,
            lastUpdateTime INTEGER,
            imageCapturePath TEXT,
            imageSourceConfigId TEXT,
            FOREIGN KEY (imageSourceConfigId) REFERENCES image_source_configuration(imageSourceConfigId)
        )
    )");

    // Workflow table
    conn.execute(R"(
        CREATE TABLE IF NOT EXISTS workflow (
            workflowId TEXT PRIMARY KEY,
            name TEXT,
            description TEXT,
            creationTime INTEGER,
            lastUpdatedTime INTEGER,
            workflowOutputPath TEXT,
            featureConfigurations TEXT,
            inputConfigurations TEXT,
            outputConfigurations TEXT,
            imageSourceId TEXT,
            FOREIGN KEY (imageSourceId) REFERENCES image_source(imageSourceId)
        )
    )");
}

void DbManager::create_metadata_tables(DbConnection& conn) {
    // Inference result metadata table
    conn.execute(R"(
        CREATE TABLE IF NOT EXISTS inference_result_metadata (
            captureId TEXT PRIMARY KEY,
            captureType TEXT,
            workflowId TEXT,
            inferenceCreationTime INTEGER,
            prediction TEXT,
            confidence REAL,
            anomalyLabels TEXT,
            anomalyScore REAL,
            anomalyThreshod REAL,
            maskImage TEXT,
            maskBackground TEXT,
            inputImageFilePath TEXT,
            outputImageFilePath TEXT,
            modelId TEXT,
            modelName TEXT,
            flagForReview INTEGER,
            downloaded INTEGER,
            humanClassification TEXT,
            textNote TEXT,
            humanReviewRequired INTEGER,
            modelConfidenceThresholds TEXT
        )
    )");
    conn.execute("CREATE INDEX IF NOT EXISTS idx_workflow_id ON inference_result_metadata(workflowId)");

    // Workflow metadata table
    conn.execute(R"(
        CREATE TABLE IF NOT EXISTS workflow_metadata (
            workflowId TEXT PRIMARY KEY,
            summaryStartTime INTEGER NOT NULL
        )
    )");

    // Latency time table
    conn.execute(R"(
        CREATE TABLE IF NOT EXISTS latency_time (
            inferenceCaptureId TEXT,
            latencyType TEXT,
            timestamp REAL NOT NULL,
            PRIMARY KEY (inferenceCaptureId, latencyType)
        )
    )");
}

} // namespace database
} // namespace dda
