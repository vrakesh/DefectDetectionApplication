/**
 * @file endpoints.cpp
 * @brief Endpoint helper functions and registration
 */

#include "endpoints/endpoints.hpp"
#include <spdlog/spdlog.h>

namespace dda {
namespace endpoints {

crow::response json_response(int status, const json& body) {
    crow::response res(status);
    res.set_header("Content-Type", "application/json");
    res.body = body.dump();
    return res;
}

crow::response error_response(int status, const std::string& message) {
    json body = {{"error", message}};
    return json_response(status, body);
}

crow::response success_response(const json& body) {
    return json_response(200, body);
}

void register_system_endpoints(crow::SimpleApp& app) {
    // GET /system/health - Health check
    CROW_ROUTE(app, "/system/health")
    .methods(crow::HTTPMethod::GET)
    ([](const crow::request& req) {
        json result = {
            {"status", "healthy"},
            {"service", "dda-backend-cpp"}
        };
        return success_response(result);
    });
    
    // GET /system/version
    CROW_ROUTE(app, "/system/version")
    .methods(crow::HTTPMethod::GET)
    ([](const crow::request& req) {
        json result = {
            {"version", "1.0.0"},
            {"build", "cpp"}
        };
        return success_response(result);
    });
}

void register_image_source_endpoints(crow::SimpleApp& app) {
    // GET /image-sources
    CROW_ROUTE(app, "/image-sources")
    .methods(crow::HTTPMethod::GET)
    ([](const crow::request& req) {
        return success_response(json::array());
    });
}

void register_inference_result_endpoints(crow::SimpleApp& app) {
    // GET /inference-results
    CROW_ROUTE(app, "/inference-results")
    .methods(crow::HTTPMethod::GET)
    ([](const crow::request& req) {
        return success_response(json::array());
    });
}

void register_all_endpoints(crow::SimpleApp& app) {
    spdlog::info("Registering all endpoints...");
    register_workflow_endpoints(app);
    register_camera_endpoints(app);
    register_model_endpoints(app);
    register_image_source_endpoints(app);
    register_inference_result_endpoints(app);
    register_system_endpoints(app);
    spdlog::info("All endpoints registered");
}

} // namespace endpoints
} // namespace dda
