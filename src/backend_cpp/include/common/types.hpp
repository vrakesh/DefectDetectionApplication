#pragma once

#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <nlohmann/json.hpp>

namespace dda {

// Type aliases
using json = nlohmann::json;
using Timestamp = std::chrono::system_clock::time_point;

// Enums
enum class ImageSourceType {
    CAMERA,
    FOLDER,
    ICAM
};

enum class GpioEdge {
    RISING,
    FALLING
};

enum class Prediction {
    NORMAL,
    ANOMALY
};

enum class CaptureType {
    CAPTURE,
    INFERENCE
};

enum class CameraStatus {
    CONNECTED,
    DISCONNECTED,
    ERROR
};

enum class DIOHealthStatus {
    STARTING,
    RUNNING,
    ERROR
};

enum class OutputRule {
    ALL,
    NORMAL,
    ANOMALY
};

// Helper functions for enum conversion
inline std::string to_string(ImageSourceType type) {
    switch (type) {
        case ImageSourceType::CAMERA: return "camera";
        case ImageSourceType::FOLDER: return "folder";
        case ImageSourceType::ICAM: return "icam";
    }
    return "unknown";
}

inline ImageSourceType image_source_type_from_string(const std::string& str) {
    if (str == "camera") return ImageSourceType::CAMERA;
    if (str == "folder") return ImageSourceType::FOLDER;
    if (str == "icam") return ImageSourceType::ICAM;
    throw std::invalid_argument("Unknown image source type: " + str);
}

inline std::string to_string(GpioEdge edge) {
    switch (edge) {
        case GpioEdge::RISING: return "rising";
        case GpioEdge::FALLING: return "falling";
    }
    return "unknown";
}

inline GpioEdge gpio_edge_from_string(const std::string& str) {
    if (str == "rising") return GpioEdge::RISING;
    if (str == "falling") return GpioEdge::FALLING;
    throw std::invalid_argument("Unknown GPIO edge: " + str);
}

inline std::string to_string(Prediction pred) {
    switch (pred) {
        case Prediction::NORMAL: return "normal";
        case Prediction::ANOMALY: return "anomaly";
    }
    return "unknown";
}

inline Prediction prediction_from_string(const std::string& str) {
    if (str == "normal") return Prediction::NORMAL;
    if (str == "anomaly") return Prediction::ANOMALY;
    throw std::invalid_argument("Unknown prediction: " + str);
}

inline std::string to_string(OutputRule rule) {
    switch (rule) {
        case OutputRule::ALL: return "All";
        case OutputRule::NORMAL: return "normal";
        case OutputRule::ANOMALY: return "anomaly";
    }
    return "unknown";
}

inline OutputRule output_rule_from_string(const std::string& str) {
    if (str == "All") return OutputRule::ALL;
    if (str == "normal") return OutputRule::NORMAL;
    if (str == "anomaly") return OutputRule::ANOMALY;
    throw std::invalid_argument("Unknown output rule: " + str);
}

// Utility for timestamp
inline int64_t get_current_timestamp_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

inline int64_t get_current_timestamp_sec() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

} // namespace dda
