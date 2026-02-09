/**
 * @file model_endpoint.cpp
 * @brief Model management REST API endpoints
 */

#include "endpoints/endpoints.hpp"
#include "triton/triton_client.hpp"
#include <spdlog/spdlog.h>

namespace dda {
namespace endpoints {

void register_model_endpoints(crow::SimpleApp& app) {
    
    // GET /feature-configurations/models - List all models
    CROW_ROUTE(app, "/feature-configurations/models")
    .methods(crow::HTTPMethod::GET)
    ([](const crow::request& req) {
        try {
            auto& triton = triton::TritonClient::instance();
            auto models = triton.list_models();
            
            json result = json::array();
            for (const auto& model : models) {
                result.push_back({
                    {"model_component", model.model_component},
                    {"status", model.status}
                });
            }
            return success_response(result);
        } catch (const std::exception& e) {
            spdlog::error("Error listing models: {}", e.what());
            return error_response(500, e.what());
        }
    });

    // GET /feature-configurations/models/<id> - Get model status
    CROW_ROUTE(app, "/feature-configurations/models/<string>")
    .methods(crow::HTTPMethod::GET)
    ([](const crow::request& req, const std::string& model_id) {
        try {
            auto& triton = triton::TritonClient::instance();
            auto info = triton.get_model_info(model_id);
            
            json result = {
                {"model_component", info.model_component},
                {"status", info.status}
            };
            return success_response(result);
        } catch (const std::exception& e) {
            spdlog::error("Error getting model {}: {}", model_id, e.what());
            return error_response(500, e.what());
        }
    });

    // POST /feature-configurations/models/<id>/start - Load/start a model
    CROW_ROUTE(app, "/feature-configurations/models/<string>/start")
    .methods(crow::HTTPMethod::POST, crow::HTTPMethod::GET)
    ([](const crow::request& req, const std::string& model_id) {
        try {
            spdlog::info("Starting model: {}", model_id);
            auto& triton = triton::TritonClient::instance();
            
            bool success = triton.load_model(model_id);
            if (success) {
                auto info = triton.get_model_info(model_id);
                json result = {
                    {"model_component", info.model_component},
                    {"status", info.status},
                    {"message", "Model started successfully"}
                };
                return success_response(result);
            } else {
                return error_response(500, "Failed to start model");
            }
        } catch (const std::exception& e) {
            spdlog::error("Error starting model {}: {}", model_id, e.what());
            return error_response(500, e.what());
        }
    });

    // POST /feature-configurations/models/<id>/stop - Unload/stop a model
    CROW_ROUTE(app, "/feature-configurations/models/<string>/stop")
    .methods(crow::HTTPMethod::POST, crow::HTTPMethod::GET)
    ([](const crow::request& req, const std::string& model_id) {
        try {
            spdlog::info("Stopping model: {}", model_id);
            auto& triton = triton::TritonClient::instance();
            
            bool success = triton.unload_model(model_id);
            if (success) {
                json result = {
                    {"model_component", model_id},
                    {"status", "STOPPED"},
                    {"message", "Model stopped successfully"}
                };
                return success_response(result);
            } else {
                return error_response(500, "Failed to stop model");
            }
        } catch (const std::exception& e) {
            spdlog::error("Error stopping model {}: {}", model_id, e.what());
            return error_response(500, e.what());
        }
    });

    // POST /feature-configurations/models/convert - Convert and load a model
    CROW_ROUTE(app, "/feature-configurations/models/convert")
    .methods(crow::HTTPMethod::POST)
    ([](const crow::request& req) {
        try {
            auto body = json::parse(req.body);
            
            std::string model_zip_path = body.value("model_zip_path", "");
            std::string model_name = body.value("model_name", "");
            std::string model_version = body.value("model_version", "1");
            
            if (model_zip_path.empty() || model_name.empty()) {
                return error_response(400, "model_zip_path and model_name are required");
            }
            
            spdlog::info("Converting model {} from {}", model_name, model_zip_path);
            auto& triton = triton::TritonClient::instance();
            
            bool success = triton.convert_and_load_model(model_zip_path, model_name, model_version);
            if (success) {
                auto info = triton.get_model_info(model_name);
                json result = {
                    {"model_component", model_name},
                    {"status", info.status},
                    {"message", "Model converted and loaded successfully"}
                };
                return success_response(result);
            } else {
                return error_response(500, "Failed to convert model");
            }
        } catch (const json::exception& e) {
            spdlog::error("Invalid JSON in model conversion: {}", e.what());
            return error_response(400, std::string("Invalid JSON: ") + e.what());
        } catch (const std::exception& e) {
            spdlog::error("Error converting model: {}", e.what());
            return error_response(500, e.what());
        }
    });
}

} // namespace endpoints
} // namespace dda
