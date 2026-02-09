#pragma once

#include "models/models.hpp"
#include "gpio/gpio_controller.hpp"
#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <condition_variable>

namespace dda {
namespace gpio {

// Forward declaration
class PipelineExecutor;

class DigitalInputThread {
public:
    DigitalInputThread(const models::Workflow& workflow, 
                       std::function<void(const models::Workflow&)> trigger_callback);
    ~DigitalInputThread();

    // Non-copyable, non-movable
    DigitalInputThread(const DigitalInputThread&) = delete;
    DigitalInputThread& operator=(const DigitalInputThread&) = delete;

    // Start/stop monitoring
    void start();
    void stop();

    // Status
    bool is_running() const { return running_; }
    models::DIOHealthReport get_health_report() const;
    std::string get_workflow_id() const { return workflow_id_; }

private:
    void monitor_loop();
    void execute_trigger();
    void update_health_status(models::DIOHealthStatus status, const std::string& error = "");

    std::string workflow_id_;
    std::string image_source_id_;
    int pin_;
    GpioEdge trigger_edge_;
    int debounce_time_ms_;
    double polling_frequency_sec_;

    models::Workflow workflow_;
    std::function<void(const models::Workflow&)> trigger_callback_;

    std::thread monitor_thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stop_requested_{false};
    std::condition_variable stop_cv_;
    std::mutex cv_mutex_;

    models::DIOHealthReport health_report_;
    mutable std::mutex health_mutex_;
};

class DigitalInputManager {
public:
    static DigitalInputManager& instance();

    // Set the workflow trigger callback
    void set_trigger_callback(std::function<void(const models::Workflow&)> callback);

    // Create/terminate digital input threads for workflows
    bool create_digital_input_thread(const models::Workflow& workflow);
    void terminate_digital_input_thread(const std::string& workflow_id);
    void terminate_all_threads();

    // Status queries
    bool is_thread_running(const std::string& workflow_id) const;
    std::optional<models::DIOHealthReport> get_health_report(const std::string& workflow_id) const;
    std::vector<std::string> get_active_workflow_ids() const;

private:
    DigitalInputManager() = default;
    ~DigitalInputManager();

    // Prevent copying
    DigitalInputManager(const DigitalInputManager&) = delete;
    DigitalInputManager& operator=(const DigitalInputManager&) = delete;

    std::unordered_map<std::string, std::unique_ptr<DigitalInputThread>> threads_;
    std::function<void(const models::Workflow&)> trigger_callback_;
    mutable std::shared_mutex threads_mutex_;
};

} // namespace gpio
} // namespace dda
