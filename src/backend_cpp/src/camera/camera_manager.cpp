/**
 * @file camera_manager.cpp
 * @brief Camera manager implementation
 */

#include "camera/camera_manager.hpp"
#include <spdlog/spdlog.h>

namespace dda {
namespace camera {

CameraManager& CameraManager::instance() {
    static CameraManager manager;
    return manager;
}

CameraManager::~CameraManager() {
    disconnect_all_cameras();
}

bool CameraManager::connect_camera(const std::string& camera_id) {
    std::unique_lock<std::shared_mutex> lock(cameras_mutex_);
    
    if (cameras_.find(camera_id) != cameras_.end()) {
        spdlog::warn("Camera {} already connected", camera_id);
        return true;
    }
    
    auto camera = std::make_unique<AravisCamera>(camera_id);
    if (!camera->connect()) {
        return false;
    }
    
    cameras_[camera_id] = std::move(camera);
    spdlog::info("Camera {} added to managed cameras", camera_id);
    return true;
}

void CameraManager::disconnect_camera(const std::string& camera_id) {
    std::unique_lock<std::shared_mutex> lock(cameras_mutex_);
    
    auto it = cameras_.find(camera_id);
    if (it == cameras_.end()) {
        spdlog::warn("Camera {} not found in managed cameras", camera_id);
        return;
    }
    
    it->second->disconnect();
    cameras_.erase(it);
    spdlog::info("Camera {} disconnected and removed", camera_id);
}

void CameraManager::disconnect_all_cameras() {
    std::unique_lock<std::shared_mutex> lock(cameras_mutex_);
    
    for (auto& [id, camera] : cameras_) {
        camera->disconnect();
    }
    cameras_.clear();
    spdlog::info("All cameras disconnected");
}

bool CameraManager::is_camera_connected(const std::string& camera_id) const {
    std::shared_lock<std::shared_mutex> lock(cameras_mutex_);
    auto it = cameras_.find(camera_id);
    return it != cameras_.end() && it->second->is_connected();
}

models::CameraStatusInfo CameraManager::get_camera_status(const std::string& camera_id) const {
    std::shared_lock<std::shared_mutex> lock(cameras_mutex_);
    
    auto it = cameras_.find(camera_id);
    if (it == cameras_.end()) {
        models::CameraStatusInfo status;
        status.status = CameraStatus::DISCONNECTED;
        status.error = "Camera not found";
        return status;
    }
    
    return it->second->get_status();
}

std::unordered_map<std::string, models::CameraStatusInfo> CameraManager::get_all_camera_statuses() const {
    std::shared_lock<std::shared_mutex> lock(cameras_mutex_);
    
    std::unordered_map<std::string, models::CameraStatusInfo> statuses;
    for (const auto& [id, camera] : cameras_) {
        statuses[id] = camera->get_status();
    }
    return statuses;
}

std::optional<models::FrameData> CameraManager::get_camera_frame(
    const std::string& camera_id, 
    const models::ImageSourceConfiguration* config) {
    
    std::lock_guard<std::mutex> capture_lock(frame_capture_mutex_);
    
    AravisCamera* camera = get_camera_internal(camera_id);
    if (!camera) {
        spdlog::error("Cannot capture: camera {} not connected", camera_id);
        return std::nullopt;
    }
    
    // Apply config if provided
    if (config) {
        if (config->gain > 0) {
            camera->set_gain(config->gain);
        }
        if (config->exposure > 0) {
            camera->set_exposure(config->exposure);
        }
    }
    
    // Start acquisition if not already streaming
    bool was_streaming = camera->is_acquiring();
    if (!was_streaming) {
        camera->start_acquisition();
    }
    
    auto frame = camera->capture_frame();
    
    // Stop acquisition if we started it
    if (!was_streaming) {
        camera->stop_acquisition();
    }
    
    return frame;
}

std::vector<std::string> CameraManager::get_connected_camera_ids() const {
    std::shared_lock<std::shared_mutex> lock(cameras_mutex_);
    
    std::vector<std::string> ids;
    ids.reserve(cameras_.size());
    for (const auto& [id, camera] : cameras_) {
        ids.push_back(id);
    }
    return ids;
}

AravisCamera* CameraManager::get_camera_internal(const std::string& camera_id) {
    std::shared_lock<std::shared_mutex> lock(cameras_mutex_);
    
    auto it = cameras_.find(camera_id);
    if (it == cameras_.end()) {
        return nullptr;
    }
    return it->second.get();
}

} // namespace camera
} // namespace dda
