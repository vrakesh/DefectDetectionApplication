#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace dda {
namespace config {

struct AppConfig {
    // Server configuration
    int server_port = 5000;
    int ssl_port = 5443;
    bool use_ssl = false;
    std::string ssl_cert_path;
    std::string ssl_key_path;
    int num_threads = 4;

    // Database paths
    std::string config_db_path;
    std::string metadata_db_path;

    // Triton configuration
    std::string triton_model_dir = "/aws_dda/triton/model_repository";
    std::string triton_installation_dir = "/aws_dda/triton";

    // Directories
    std::string component_work_path = "/aws_dda";
    std::string image_capture_dir = "/aws_dda/image-capture";
    std::string inference_results_dir = "/aws_dda/inference-results";
    std::string image_preview_dir = "/aws_dda/image-capture/preview";

    // GPIO configuration
    std::string gpio_chip = "/dev/gpiochip0";

    // Logging
    std::string log_level = "info";

    static AppConfig& instance() {
        static AppConfig config;
        return config;
    }

    void load_from_env();
    void load_from_file(const std::string& path);
    nlohmann::json to_json() const;

private:
    AppConfig() = default;
};

// Global access function
inline AppConfig& get_config() {
    return AppConfig::instance();
}

} // namespace config
} // namespace dda
