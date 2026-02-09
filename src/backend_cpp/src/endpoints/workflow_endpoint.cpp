/**
 * @file workflow_endpoint.cpp
 * @brief Workflow REST API endpoints (simplified)
 */

#include "endpoints/endpoints.hpp"
#include "database/db_manager.hpp"
#include <spdlog/spdlog.h>

namespace dda {
namespace endpoints {

void register_workflow_endpoints(crow::SimpleApp& app) {
    
    // GET /workflows - List all workflows
    CROW_ROUTE(app, "/workflows")
    .methods(crow::HTTPMethod::GET)
    ([](const crow::request& req) {
        try {
            json result = json::array();
            return success_response(result);
        } catch (const std::exception& e) {
            spdlog::error("Error listing workflows: {}", e.what());
            return error_response(500, e.what());
        }
    });
    
    // GET /workflows/<id> - Get workflow by ID
    CROW_ROUTE(app, "/workflows/<string>")
    .methods(crow::HTTPMethod::GET)
    ([](const crow::request& req, const std::string& workflow_id) {
        try {
            return error_response(404, "Workflow not found");
        } catch (const std::exception& e) {
            spdlog::error("Error getting workflow {}: {}", workflow_id, e.what());
            return error_response(500, e.what());
        }
    });
    
    // POST /workflows - Create new workflow  
    CROW_ROUTE(app, "/workflows")
    .methods(crow::HTTPMethod::POST)
    ([](const crow::request& req) {
        try {
            auto body = json::parse(req.body);
            return json_response(201, body);
        } catch (const json::exception& e) {
            spdlog::error("Invalid JSON in workflow creation: {}", e.what());
            return error_response(400, std::string("Invalid JSON: ") + e.what());
        } catch (const std::exception& e) {
            spdlog::error("Error creating workflow: {}", e.what());
            return error_response(500, e.what());
        }
    });
    
    // DELETE /workflows/<id> - Delete workflow
    CROW_ROUTE(app, "/workflows/<string>")
    .methods(crow::HTTPMethod::DELETE)
    ([](const crow::request& req, const std::string& workflow_id) {
        try {
            return json_response(204, json::object());
        } catch (const std::exception& e) {
            spdlog::error("Error deleting workflow {}: {}", workflow_id, e.what());
            return error_response(500, e.what());
        }
    });
}

} // namespace endpoints
} // namespace dda
