/**
 * @file camera_endpoint.cpp
 * @brief Camera REST API endpoints
 */

#include "endpoints/endpoints.hpp"
#include "camera/camera_manager.hpp"
#include "camera/aravis_camera.hpp"
#include <spdlog/spdlog.h>

namespace dda {
namespace endpoints {

void register_camera_endpoints(crow::SimpleApp& app) {
    
    // GET /cameras - List all available cameras (discover)
    CROW_ROUTE(app, "/cameras")
    .methods(crow::HTTPMethod::GET)
    ([](const crow::request& req) {
        try {
            auto cameras = camera::discover_cameras();
            
            json result = json::array();
            for (const auto& info : cameras) {
                result.push_back(info.to_json());
            }
            return success_response(result);
        } catch (const std::exception& e) {
            spdlog::error("Error listing cameras: {}", e.what());
            return error_response(500, e.what());
        }
    });
    
    // GET /cameras/connected - List connected cameras
    CROW_ROUTE(app, "/cameras/connected")
    .methods(crow::HTTPMethod::GET)
    ([](const crow::request& req) {
        try {
            auto& manager = camera::CameraManager::instance();
            auto ids = manager.get_connected_camera_ids();
            
            json result = json::array();
            for (const auto& id : ids) {
                result.push_back(id);
            }
            return success_response(result);
        } catch (const std::exception& e) {
            spdlog::error("Error listing connected cameras: {}", e.what());
            return error_response(500, e.what());
        }
    });
    
    // GET /cameras/<id>/status - Get camera status
    CROW_ROUTE(app, "/cameras/<string>/status")
    .methods(crow::HTTPMethod::GET)
    ([](const crow::request& req, const std::string& camera_id) {
        try {
            auto& manager = camera::CameraManager::instance();
            auto status = manager.get_camera_status(camera_id);
            return success_response(status.to_json());
        } catch (const std::exception& e) {
            spdlog::error("Error getting camera {} status: {}", camera_id, e.what());
            return error_response(500, e.what());
        }
    });
    
    // GET /cameras/statuses - Get all camera statuses
    CROW_ROUTE(app, "/cameras/statuses")
    .methods(crow::HTTPMethod::GET)
    ([](const crow::request& req) {
        try {
            auto& manager = camera::CameraManager::instance();
            auto statuses = manager.get_all_camera_statuses();
            
            json result = json::object();
            for (const auto& [id, status] : statuses) {
                result[id] = status.to_json();
            }
            return success_response(result);
        } catch (const std::exception& e) {
            spdlog::error("Error getting camera statuses: {}", e.what());
            return error_response(500, e.what());
        }
    });
    
    // POST /cameras/<id>/connect - Connect to camera
    CROW_ROUTE(app, "/cameras/<string>/connect")
    .methods(crow::HTTPMethod::POST)
    ([](const crow::request& req, const std::string& camera_id) {
        try {
            auto& manager = camera::CameraManager::instance();
            
            if (manager.connect_camera(camera_id)) {
                auto status = manager.get_camera_status(camera_id);
                json result = {
                    {"status", "connected"},
                    {"camera_id", camera_id}
                };
                return success_response(result);
            } else {
                return error_response(500, "Failed to connect to camera");
            }
        } catch (const std::exception& e) {
            spdlog::error("Error connecting to camera {}: {}", camera_id, e.what());
            return error_response(500, e.what());
        }
    });
    
    // POST /cameras/<id>/disconnect - Disconnect from camera
    CROW_ROUTE(app, "/cameras/<string>/disconnect")
    .methods(crow::HTTPMethod::POST)
    ([](const crow::request& req, const std::string& camera_id) {
        try {
            auto& manager = camera::CameraManager::instance();
            manager.disconnect_camera(camera_id);
            json result = {
                {"status", "disconnected"},
                {"camera_id", camera_id}
            };
            return success_response(result);
        } catch (const std::exception& e) {
            spdlog::error("Error disconnecting camera {}: {}", camera_id, e.what());
            return error_response(500, e.what());
        }
    });
    
    // POST /cameras/<id>/capture - Capture a single frame
    CROW_ROUTE(app, "/cameras/<string>/capture")
    .methods(crow::HTTPMethod::POST)
    ([](const crow::request& req, const std::string& camera_id) {
        try {
            auto& manager = camera::CameraManager::instance();
            
            if (!manager.is_camera_connected(camera_id)) {
                return error_response(404, "Camera not connected");
            }
            
            auto frame = manager.get_camera_frame(camera_id);
            if (frame) {
                json result = {
                    {"width", frame->width},
                    {"height", frame->height},
                    {"pixel_format", frame->pixel_format},
                    {"size_bytes", frame->data.size()}
                };
                return success_response(result);
            } else {
                return error_response(500, "Failed to capture frame");
            }
        } catch (const std::exception& e) {
            spdlog::error("Error capturing from camera {}: {}", camera_id, e.what());
            return error_response(500, e.what());
        }
    });
}

} // namespace endpoints
} // namespace dda
