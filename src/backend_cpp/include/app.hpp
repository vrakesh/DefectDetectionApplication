#pragma once

#include "crow.h"
#include <memory>
#include <atomic>
#include <thread>

namespace dda {

namespace models {
    struct Workflow;  // Forward declaration
}

class Application {
public:
    static Application& instance();

    // Initialize and run the application
    bool initialize();
    void run();
    void shutdown();

    // Check if running
    bool is_running() const { return running_; }

    // Get the Crow app for endpoint registration
    crow::SimpleApp& get_crow_app() { return app_; }

private:
    Application() = default;
    ~Application();

    // Prevent copying
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    // Initialization steps
    void setup_triton();
    void setup_database();
    void setup_directories();
    void setup_digital_inputs();
    void setup_cameras();
    void setup_endpoints();

    // Cleanup
    void cleanup_digital_inputs();
    void cleanup_cameras();

    crow::SimpleApp app_;
    std::atomic<bool> running_{false};
    std::atomic<bool> initialized_{false};
};

} // namespace dda
