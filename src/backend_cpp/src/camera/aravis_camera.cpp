/**
 * @file aravis_camera.cpp
 * @brief Aravis GigE Vision camera implementation
 */

#include "camera/aravis_camera.hpp"
#include <spdlog/spdlog.h>
#include <chrono>

namespace dda {
namespace camera {

// Static variable for fake camera testing
static bool g_fake_camera_enabled = false;

AravisCamera::AravisCamera(const std::string& camera_id)
    : camera_id_(camera_id) {
    status_.status = CameraStatus::DISCONNECTED;
}

AravisCamera::~AravisCamera() {
    disconnect();
}

bool AravisCamera::connect() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (camera_) {
        spdlog::warn("Camera {} already connected", camera_id_);
        return true;
    }

    spdlog::info("Connecting to camera: {}", camera_id_);
    
    GError* error = nullptr;
    camera_ = arv_camera_new(camera_id_.c_str(), &error);
    
    if (error) {
        spdlog::error("Failed to connect to camera {}: {}", camera_id_, error->message);
        update_status(CameraStatus::ERROR, error->message);
        g_error_free(error);
        return false;
    }

    if (!camera_) {
        spdlog::error("Failed to connect to camera {}: unknown error", camera_id_);
        update_status(CameraStatus::ERROR, "Unknown error");
        return false;
    }

    setup_camera();
    update_status(CameraStatus::CONNECTED);
    spdlog::info("Connected to camera: {}", camera_id_);
    return true;
}

void AravisCamera::disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!camera_) {
        return;
    }

    spdlog::info("Disconnecting from camera: {}", camera_id_);
    
    if (acquiring_) {
        acquiring_ = false;
        GError* error = nullptr;
        arv_camera_stop_acquisition(camera_, &error);
        if (error) g_error_free(error);
    }

    cleanup_camera();
    
    g_object_unref(camera_);
    camera_ = nullptr;
    stream_ = nullptr;
    buffer_ = nullptr;
    
    update_status(CameraStatus::DISCONNECTED);
    spdlog::info("Disconnected from camera: {}", camera_id_);
}

bool AravisCamera::is_connected() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return camera_ != nullptr;
}

models::CameraStatusInfo AravisCamera::get_status() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return status_;
}

models::CameraInfo AravisCamera::get_camera_info() const {
    std::lock_guard<std::mutex> lock(mutex_);
    models::CameraInfo info;
    info.id = camera_id_;
    info.vendor = vendor_;
    info.model = model_;
    info.serial = serial_;
    return info;
}

void AravisCamera::set_gain(double gain) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!camera_) return;
    
    GError* error = nullptr;
    arv_camera_set_gain(camera_, gain, &error);
    if (error) {
        spdlog::error("Failed to set gain: {}", error->message);
        g_error_free(error);
    } else {
        spdlog::debug("Set gain to {}", gain);
    }
}

void AravisCamera::set_exposure(double exposure_us) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!camera_) return;
    
    GError* error = nullptr;
    arv_camera_set_exposure_time(camera_, exposure_us, &error);
    if (error) {
        spdlog::error("Failed to set exposure: {}", error->message);
        g_error_free(error);
    } else {
        spdlog::debug("Set exposure to {} us", exposure_us);
    }
}

void AravisCamera::set_trigger_mode(bool software_trigger) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!camera_) return;
    
    GError* error = nullptr;
    if (software_trigger) {
        arv_camera_set_trigger(camera_, "Software", &error);
    } else {
        arv_camera_set_trigger(camera_, "Off", &error);
    }
    
    if (error) {
        spdlog::error("Failed to set trigger mode: {}", error->message);
        g_error_free(error);
    }
}

void AravisCamera::start_acquisition() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!camera_ || acquiring_) return;

    spdlog::info("Starting acquisition on camera: {}", camera_id_);
    
    GError* error = nullptr;
    
    // Create stream
    stream_ = arv_camera_create_stream(camera_, nullptr, nullptr, &error);
    if (error) {
        spdlog::error("Failed to create stream: {}", error->message);
        g_error_free(error);
        return;
    }

    // Allocate buffers
    payload_size_ = arv_camera_get_payload(camera_, nullptr);
    for (int i = 0; i < 10; i++) {
        arv_stream_push_buffer(stream_, arv_buffer_new(payload_size_, nullptr));
    }

    // Start acquisition
    arv_camera_start_acquisition(camera_, &error);
    if (error) {
        spdlog::error("Failed to start acquisition: {}", error->message);
        g_error_free(error);
        g_object_unref(stream_);
        stream_ = nullptr;
        return;
    }

    acquiring_ = true;
    spdlog::info("Acquisition started on camera: {}", camera_id_);
}

void AravisCamera::stop_acquisition() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!camera_ || !acquiring_) return;

    spdlog::info("Stopping acquisition on camera: {}", camera_id_);
    
    GError* error = nullptr;
    arv_camera_stop_acquisition(camera_, &error);
    if (error) {
        spdlog::warn("Error stopping acquisition: {}", error->message);
        g_error_free(error);
    }

    if (stream_) {
        g_object_unref(stream_);
        stream_ = nullptr;
    }

    acquiring_ = false;
    spdlog::info("Acquisition stopped on camera: {}", camera_id_);
}

std::optional<models::FrameData> AravisCamera::capture_frame() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!camera_ || !stream_) {
        spdlog::error("Cannot capture: camera not streaming");
        return std::nullopt;
    }

    // Wait for buffer with timeout
    ArvBuffer* buffer = arv_stream_timeout_pop_buffer(stream_, 5000000); // 5 sec timeout
    
    if (!buffer) {
        spdlog::warn("Timeout waiting for frame");
        return std::nullopt;
    }
    
    ArvBufferStatus buf_status = arv_buffer_get_status(buffer);
    if (buf_status != ARV_BUFFER_STATUS_SUCCESS) {
        spdlog::warn("Buffer status error: {}", static_cast<int>(buf_status));
        arv_stream_push_buffer(stream_, buffer);
        return std::nullopt;
    }

    // Extract frame data
    size_t size;
    const void* data = arv_buffer_get_data(buffer, &size);
    
    models::FrameData frame;
    frame.width = arv_buffer_get_image_width(buffer);
    frame.height = arv_buffer_get_image_height(buffer);
    frame.data.assign(static_cast<const uint8_t*>(data), 
                     static_cast<const uint8_t*>(data) + size);

    // Return buffer to stream
    arv_stream_push_buffer(stream_, buffer);
    
    spdlog::debug("Captured frame: {}x{}, {} bytes", frame.width, frame.height, size);
    return frame;
}

void AravisCamera::software_trigger() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!camera_) return;
    
    GError* error = nullptr;
    arv_camera_software_trigger(camera_, &error);
    if (error) {
        spdlog::error("Software trigger failed: {}", error->message);
        g_error_free(error);
    }
}

void AravisCamera::setup_camera() {
    if (!camera_) return;
    
    GError* error = nullptr;
    
    // Get camera info
    const char* vendor = arv_camera_get_vendor_name(camera_, &error);
    if (vendor && !error) {
        vendor_ = vendor;
    }
    if (error) { g_error_free(error); error = nullptr; }
    
    const char* model = arv_camera_get_model_name(camera_, &error);
    if (model && !error) {
        model_ = model;
    }
    if (error) { g_error_free(error); error = nullptr; }
    
    const char* serial = arv_camera_get_device_serial_number(camera_, &error);
    if (serial && !error) {
        serial_ = serial;
    }
    if (error) { g_error_free(error); error = nullptr; }
}

void AravisCamera::cleanup_camera() {
    // Clean up any camera-specific resources
}

void AravisCamera::update_status(CameraStatus status, const std::string& error) {
    status_.status = status;
    status_.error = error;
    status_.last_updated_time = get_current_timestamp_ms();
}

// Free functions

std::vector<models::CameraInfo> discover_cameras() {
    std::vector<models::CameraInfo> cameras;
    
    arv_update_device_list();
    unsigned int count = arv_get_n_devices();
    
    spdlog::info("Discovered {} cameras", count);
    
    for (unsigned int i = 0; i < count; i++) {
        models::CameraInfo info;
        info.id = arv_get_device_id(i);
        info.vendor = arv_get_device_vendor(i) ?: "";
        info.model = arv_get_device_model(i) ?: "";
        info.serial = arv_get_device_serial_nbr(i) ?: "";
        info.address = arv_get_device_address(i) ?: "";
        cameras.push_back(info);
    }
    
    return cameras;
}

std::optional<models::CameraInfo> get_camera_info(const std::string& camera_id) {
    auto cameras = discover_cameras();
    for (const auto& cam : cameras) {
        if (cam.id == camera_id) {
            return cam;
        }
    }
    return std::nullopt;
}

void enable_fake_camera_interface(bool enable) {
    g_fake_camera_enabled = enable;
    spdlog::info("Fake camera interface: {}", enable ? "enabled" : "disabled");
}

} // namespace camera
} // namespace dda
