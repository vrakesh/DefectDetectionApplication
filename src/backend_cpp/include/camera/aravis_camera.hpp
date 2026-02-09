#pragma once

#include "models/models.hpp"
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <arv.h>  // Aravis C API

namespace dda {
namespace camera {

class AravisCamera {
public:
    explicit AravisCamera(const std::string& camera_id);
    ~AravisCamera();

    // Non-copyable
    AravisCamera(const AravisCamera&) = delete;
    AravisCamera& operator=(const AravisCamera&) = delete;

    // Connect/disconnect
    bool connect();
    void disconnect();
    bool is_connected() const;

    // Camera info
    std::string get_camera_id() const { return camera_id_; }
    models::CameraStatusInfo get_status() const;
    models::CameraInfo get_camera_info() const;

    // Configuration
    void set_gain(double gain);
    void set_exposure(double exposure_us);
    void set_trigger_mode(bool software_trigger);

    // Acquisition
    void start_acquisition();
    void stop_acquisition();
    bool is_acquiring() const { return acquiring_; }

    // Frame capture
    std::optional<models::FrameData> capture_frame();
    void software_trigger();

private:
    void setup_camera();
    void cleanup_camera();
    void update_status(CameraStatus status, const std::string& error = "");

    std::string camera_id_;
    ArvCamera* camera_ = nullptr;
    ArvStream* stream_ = nullptr;
    ArvBuffer* buffer_ = nullptr;
    
    // Camera info
    std::string vendor_;
    std::string model_;
    std::string serial_;
    
    int payload_size_ = 0;
    bool acquiring_ = false;
    
    models::CameraStatusInfo status_;
    mutable std::mutex mutex_;
};

// Camera discovery functions
std::vector<models::CameraInfo> discover_cameras();
std::optional<models::CameraInfo> get_camera_info(const std::string& camera_id);

// Enable fake camera interface for testing
void enable_fake_camera_interface(bool enable = true);

} // namespace camera
} // namespace dda
