#pragma once

#include "camera/aravis_camera.hpp"
#include "models/models.hpp"
#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>

namespace dda {
namespace camera {

class CameraManager {
public:
    static CameraManager& instance();

    // Camera lifecycle
    bool connect_camera(const std::string& camera_id);
    void disconnect_camera(const std::string& camera_id);
    void disconnect_all_cameras();

    // Check if camera is connected
    bool is_camera_connected(const std::string& camera_id) const;

    // Get camera status
    models::CameraStatusInfo get_camera_status(const std::string& camera_id) const;
    std::unordered_map<std::string, models::CameraStatusInfo> get_all_camera_statuses() const;

    // Frame capture - thread-safe
    std::optional<models::FrameData> get_camera_frame(
        const std::string& camera_id, 
        const models::ImageSourceConfiguration* config = nullptr
    );

    // Get list of connected camera IDs
    std::vector<std::string> get_connected_camera_ids() const;

private:
    CameraManager() = default;
    ~CameraManager();

    // Prevent copying
    CameraManager(const CameraManager&) = delete;
    CameraManager& operator=(const CameraManager&) = delete;

    // Internal camera access (must hold appropriate lock)
    AravisCamera* get_camera_internal(const std::string& camera_id);

    std::unordered_map<std::string, std::unique_ptr<AravisCamera>> cameras_;
    mutable std::shared_mutex cameras_mutex_;
    std::mutex frame_capture_mutex_;  // Serialize frame captures
};

} // namespace camera
} // namespace dda
