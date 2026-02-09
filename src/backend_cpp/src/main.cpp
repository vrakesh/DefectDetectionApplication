/**
 * @file main.cpp
 * @brief Entry point for the DDA C++ Backend Application
 * 
 * This application provides a REST API backend for defect detection using:
 * - Crow C++ web framework for HTTP server
 * - Triton Inference Server for ML model inference
 * - Aravis library for GigE Vision camera control
 * - libgpiod for GPIO digital I/O
 * - GStreamer for image processing pipelines
 * - SQLite for data persistence
 */

#include "app.hpp"
#include "config/app_config.hpp"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <csignal>
#include <iostream>

// Global flag for signal handling
static std::atomic<bool> g_shutdown_requested{false};

void signal_handler(int signal) {
    spdlog::info("Received signal {}, initiating shutdown...", signal);
    g_shutdown_requested = true;
    dda::Application::instance().shutdown();
}

void setup_signal_handlers() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
}

void setup_logging() {
    // Console sink
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::info);
    console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");

    // File sink with rotation
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        "/var/log/dda/backend.log", 
        10 * 1024 * 1024,  // 10MB max file size
        3                   // Keep 3 backup files
    );
    file_sink->set_level(spdlog::level::debug);
    file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] [%s:%#] %v");

    // Create logger with both sinks
    auto logger = std::make_shared<spdlog::logger>("dda", 
        spdlog::sinks_init_list{console_sink, file_sink});
    logger->set_level(spdlog::level::debug);
    
    spdlog::set_default_logger(logger);
    spdlog::info("DDA Backend C++ - Logging initialized");
}

int main(int argc, char* argv[]) {
    try {
        // Setup logging first
        setup_logging();
        
        spdlog::info("==============================================");
        spdlog::info("  DDA Backend C++ Server Starting");
        spdlog::info("==============================================");
        
        // Setup signal handlers
        setup_signal_handlers();
        
        // Load configuration
        auto& config = dda::config::get_config();
        config.load_from_env();
        
        spdlog::info("Configuration loaded:");
        spdlog::info("  - Server port: {}", config.server_port);
        spdlog::info("  - Triton model dir: {}", config.triton_model_dir);
        spdlog::info("  - Work path: {}", config.component_work_path);
        
        // Initialize and run application
        auto& app = dda::Application::instance();
        
        if (!app.initialize()) {
            spdlog::error("Failed to initialize application");
            return 1;
        }
        
        spdlog::info("Application initialized successfully");
        spdlog::info("Starting server...");
        
        // Run the application (blocking call)
        app.run();
        
        spdlog::info("Server stopped");
        return 0;
        
    } catch (const std::exception& e) {
        spdlog::critical("Unhandled exception: {}", e.what());
        return 1;
    } catch (...) {
        spdlog::critical("Unknown exception occurred");
        return 1;
    }
}
