/**
 * @file app_config.cpp
 * @brief Application configuration implementation
 */

#include "config/app_config.hpp"
#include <cstdlib>
#include <fstream>

namespace dda {
namespace config {

void AppConfig::load_from_env() {
    // Server configuration
    if (const char* val = std::getenv("DDA_SERVER_PORT")) {
        server_port = std::stoi(val);
    }
    if (const char* val = std::getenv("DDA_SSL_PORT")) {
        ssl_port = std::stoi(val);
    }
    if (const char* val = std::getenv("DDA_USE_SSL")) {
        use_ssl = std::string(val) == "true" || std::string(val) == "1";
    }
    if (const char* val = std::getenv("DDA_SSL_CERT")) {
        ssl_cert_path = val;
    }
    if (const char* val = std::getenv("DDA_SSL_KEY")) {
        ssl_key_path = val;
    }
    if (const char* val = std::getenv("DDA_NUM_THREADS")) {
        num_threads = std::stoi(val);
    }

    // Work path (primary configuration)
    if (const char* val = std::getenv("COMPONENT_WORK_PATH")) {
        component_work_path = val;
    } else if (const char* val = std::getenv("DDA_WORK_PATH")) {
        component_work_path = val;
    }

    // Derived paths
    config_db_path = component_work_path + "/dda_backend_app.db";
    metadata_db_path = component_work_path + "/dda_backend_metadata.db";
    image_capture_dir = component_work_path + "/image-capture";
    inference_results_dir = component_work_path + "/inference-results";
    image_preview_dir = image_capture_dir + "/preview";

    // Triton configuration
    if (const char* val = std::getenv("TRITON_MODEL_DIR")) {
        triton_model_dir = val;
    } else {
        triton_model_dir = component_work_path + "/triton/model_repository";
    }
    if (const char* val = std::getenv("TRITON_INSTALLATION_DIR")) {
        triton_installation_dir = val;
    } else {
        triton_installation_dir = component_work_path + "/triton";
    }

    // GPIO configuration
    if (const char* val = std::getenv("DDA_GPIO_CHIP")) {
        gpio_chip = val;
    }

    // Logging
    if (const char* val = std::getenv("DDA_LOG_LEVEL")) {
        log_level = val;
    }
}

void AppConfig::load_from_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open config file: " + path);
    }

    nlohmann::json config_json;
    file >> config_json;

    // Server configuration
    if (config_json.contains("server_port")) {
        server_port = config_json["server_port"].get<int>();
    }
    if (config_json.contains("ssl_port")) {
        ssl_port = config_json["ssl_port"].get<int>();
    }
    if (config_json.contains("use_ssl")) {
        use_ssl = config_json["use_ssl"].get<bool>();
    }
    if (config_json.contains("ssl_cert_path")) {
        ssl_cert_path = config_json["ssl_cert_path"].get<std::string>();
    }
    if (config_json.contains("ssl_key_path")) {
        ssl_key_path = config_json["ssl_key_path"].get<std::string>();
    }
    if (config_json.contains("num_threads")) {
        num_threads = config_json["num_threads"].get<int>();
    }

    // Paths
    if (config_json.contains("component_work_path")) {
        component_work_path = config_json["component_work_path"].get<std::string>();
        // Update derived paths
        config_db_path = component_work_path + "/dda_backend_app.db";
        metadata_db_path = component_work_path + "/dda_backend_metadata.db";
        image_capture_dir = component_work_path + "/image-capture";
        inference_results_dir = component_work_path + "/inference-results";
        image_preview_dir = image_capture_dir + "/preview";
    }

    // Triton
    if (config_json.contains("triton_model_dir")) {
        triton_model_dir = config_json["triton_model_dir"].get<std::string>();
    }
    if (config_json.contains("triton_installation_dir")) {
        triton_installation_dir = config_json["triton_installation_dir"].get<std::string>();
    }

    // GPIO
    if (config_json.contains("gpio_chip")) {
        gpio_chip = config_json["gpio_chip"].get<std::string>();
    }

    // Logging
    if (config_json.contains("log_level")) {
        log_level = config_json["log_level"].get<std::string>();
    }
}

nlohmann::json AppConfig::to_json() const {
    return {
        {"server_port", server_port},
        {"ssl_port", ssl_port},
        {"use_ssl", use_ssl},
        {"ssl_cert_path", ssl_cert_path},
        {"ssl_key_path", ssl_key_path},
        {"num_threads", num_threads},
        {"config_db_path", config_db_path},
        {"metadata_db_path", metadata_db_path},
        {"triton_model_dir", triton_model_dir},
        {"triton_installation_dir", triton_installation_dir},
        {"component_work_path", component_work_path},
        {"image_capture_dir", image_capture_dir},
        {"inference_results_dir", inference_results_dir},
        {"image_preview_dir", image_preview_dir},
        {"gpio_chip", gpio_chip},
        {"log_level", log_level}
    };
}

} // namespace config
} // namespace dda
