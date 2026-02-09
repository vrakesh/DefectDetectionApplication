#pragma once

#include "models/models.hpp"
#include "pipeline/gst_pipeline.hpp"
#include "pipeline/pipeline_builder.hpp"
#include <string>
#include <optional>
#include <functional>

namespace dda {
namespace pipeline {

struct LatencyMetrics {
    std::unordered_map<std::string, double> timestamps;

    void add_timestamp(const std::string& name) {
        timestamps[name] = static_cast<double>(get_current_timestamp_ms()) / 1000.0;
    }

    double get_timestamp(const std::string& name) const {
        auto it = timestamps.find(name);
        return it != timestamps.end() ? it->second : 0.0;
    }
};

// Latency timestamp names
constexpr const char* TRIGGER_TIMESTAMP = "trigger_timestamp";
constexpr const char* FRAME_CAPTURE_TIMESTAMP = "frame_capture_timestamp";
constexpr const char* INFERENCE_RECEIVED_TIMESTAMP = "inference_received_timestamp";

class PipelineExecutor {
public:
    PipelineExecutor();

    // Execute image source pipeline (capture only, no inference)
    struct ImageCaptureResult {
        std::vector<uint8_t> image_data;
        std::string capture_location;
        bool success = false;
        std::string error;
    };
    ImageCaptureResult execute_image_source_pipeline(
        const models::ImageSource& image_source,
        bool is_preview = false,
        const models::FrameData* frame_data = nullptr,
        const std::string& file_prefix = "",
        const std::string& workflow_output_path = "");

    // Execute workflow pipeline (capture + inference)
    struct WorkflowResult {
        std::string capture_id;
        json parsed_tags;
        bool success = false;
        std::string error;
    };
    WorkflowResult execute_workflow_pipeline(
        const models::Workflow& workflow,
        const models::ImageSource& image_source,
        const models::FrameData* frame_data = nullptr,
        LatencyMetrics* latency_metrics = nullptr);

    // For folder-based workflows
    std::string get_oldest_image_file(const std::string& folder_path);
    void move_bad_folder_image(const std::string& workflow_id, const std::string& bad_image_path);

private:
    void update_file_permissions(const std::string& workflow_output_path, 
                                 const std::string& capture_id);
    void reset_digital_outputs(const models::Workflow& workflow);
    void cleanup_file_after_processing(const std::string& file_path);

    GstPipelineManager pipeline_manager_;
    PipelineBuilder pipeline_builder_;
};

} // namespace pipeline
} // namespace dda
