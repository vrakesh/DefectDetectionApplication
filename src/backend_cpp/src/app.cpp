/**
 * @file app.cpp
 * @brief Application class implementation (simplified)
 */

#include "app.hpp"
#include "config/app_config.hpp"
#include "database/db_manager.hpp"
#include "endpoints/endpoints.hpp"
#include "triton/triton_client.hpp"
#include "camera/camera_manager.hpp"
#include "gpio/gpio_controller.hpp"

#include <spdlog/spdlog.h>
#include <filesystem>

namespace dda {

Application& Application::instance() {
    static Application app;
    return app;
}

Application::~Application() {
    if (running_) {
        shutdown();
    }
}

bool Application::initialize() {
    if (initialized_) {
        spdlog::warn("Application already initialized");
        return true;
    }

    try {
        spdlog::info("Initializing application...");

        // Setup in order
        setup_directories();
        setup_database();
        setup_triton();
        setup_cameras();
        setup_digital_inputs();
        setup_endpoints();

        initialized_ = true;
        spdlog::info("Application initialization complete");
        return true;

    } catch (const std::exception& e) {
        spdlog::error("Application initialization failed: {}", e.what());
        return false;
    }
}

void Application::run() {
    if (!initialized_) {
        spdlog::error("Cannot run uninitialized application");
        return;
    }

    running_ = true;
    
    auto& config = config::get_config();
    
    // Configure Crow
    app_.loglevel(crow::LogLevel::Warning);

    spdlog::info("Starting Crow server on port {}", config.server_port);
    
    app_.port(config.server_port)
        .multithreaded()
        .run();
}

void Application::shutdown() {
    if (!running_) {
        return;
    }

    spdlog::info("Shutting down application...");
    running_ = false;

    // Cleanup
    cleanup_digital_inputs();
    cleanup_cameras();

    // Stop Crow server
    app_.stop();

    spdlog::info("Application shutdown complete");
}

void Application::setup_directories() {
    auto& config = config::get_config();
    
    spdlog::info("Setting up directories...");
    
    std::vector<std::string> dirs = {
        config.component_work_path,
        config.image_capture_dir,
        config.inference_results_dir,
        config.image_preview_dir
    };

    for (const auto& dir : dirs) {
        if (!std::filesystem::exists(dir)) {
            spdlog::info("Creating directory: {}", dir);
            std::filesystem::create_directories(dir);
        }
    }
}

void Application::setup_database() {
    auto& config = config::get_config();
    
    spdlog::info("Setting up database...");
    
    std::string config_db = config.component_work_path + "/dda_backend_app.db";
    std::string metadata_db = config.component_work_path + "/dda_backend_metadata.db";
    
    auto& db_manager = database::DbManager::instance();
    db_manager.initialize(config_db, metadata_db);
    db_manager.run_migrations();
    
    spdlog::info("Database initialized");
}

void Application::setup_triton() {
    auto& config = config::get_config();
    
    spdlog::info("Setting up Triton inference server...");
    
    auto& triton = triton::TritonClient::instance();
    bool success = triton.initialize(config.triton_model_dir, config.triton_installation_dir);
    
    if (success) {
        spdlog::info("Triton client initialized successfully");
        
        // List available models
        auto models = triton.list_models();
        spdlog::info("Found {} models in repository", models.size());
        for (const auto& model : models) {
            spdlog::info("  - Model: {} (status: {})", model.model_component, model.status);
        }
    } else {
        spdlog::warn("Triton client initialization failed - running without inference support");
    }
}

void Application::setup_cameras() {
    spdlog::info("Camera manager ready - cameras will be connected on demand");
    
    // CameraManager uses lazy connection - cameras connect when needed
    // We just verify the manager is accessible
    auto& camera_manager = camera::CameraManager::instance();
    auto connected = camera_manager.get_connected_camera_ids();
    spdlog::info("Currently connected cameras: {}", connected.size());
}

void Application::setup_digital_inputs() {
    spdlog::info("Setting up digital inputs (using c-periphery)...");
    
    auto& gpio = gpio::GpioController::instance();
    // Initialize GPIO controller - pins are opened on demand
    bool success = gpio.initialize();
    
    if (success) {
        spdlog::info("GPIO controller initialized (pins opened on demand)");
    } else {
        spdlog::warn("GPIO controller initialization failed - running without trigger support");
    }
}

void Application::setup_endpoints() {
    spdlog::info("Registering endpoints...");
    endpoints::register_all_endpoints(app_);
    spdlog::info("Endpoints registered");
}

void Application::cleanup_digital_inputs() {
    spdlog::info("Cleaning up digital input threads...");
    auto& gpio = gpio::GpioController::instance();
    gpio.shutdown();
}

void Application::cleanup_cameras() {
    spdlog::info("Disconnecting all cameras...");
    auto& camera_manager = camera::CameraManager::instance();
    camera_manager.disconnect_all_cameras();
}

} // namespace dda
