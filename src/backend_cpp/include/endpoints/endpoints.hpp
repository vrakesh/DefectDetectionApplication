#pragma once

#include "crow.h"
#include "models/models.hpp"

namespace dda {
namespace endpoints {

// Base response helpers
crow::response json_response(int status, const json& body);
crow::response error_response(int status, const std::string& message);
crow::response success_response(const json& body);

// Register all endpoints with the Crow app
void register_workflow_endpoints(crow::SimpleApp& app);
void register_camera_endpoints(crow::SimpleApp& app);
void register_model_endpoints(crow::SimpleApp& app);
void register_image_source_endpoints(crow::SimpleApp& app);
void register_inference_result_endpoints(crow::SimpleApp& app);
void register_system_endpoints(crow::SimpleApp& app);

// Combined registration
void register_all_endpoints(crow::SimpleApp& app);

} // namespace endpoints
} // namespace dda
