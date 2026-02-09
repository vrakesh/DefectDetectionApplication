#pragma once

#include "models/models.hpp"
#include "pipeline/gst_pipeline.hpp"
#include <string>
#include <optional>

namespace dda {
namespace pipeline {

class PipelineBuilder {
public:
    PipelineBuilder();

    // Reset builder state
    void reset();

    // Add image source to pipeline
    PipelineBuilder& add_image_source(const models::ImageSource& image_source,
                                      const std::string& override_processing_pipeline = "",
                                      const std::string& override_folder_source_file = "");

    // Add inference to pipeline
    PipelineBuilder& add_inference(const models::Workflow& workflow, 
                                   const std::string& capture_id);

    // Build the final pipeline string
    struct BuildResult {
        std::string pipeline_string;
        std::string capture_location;
    };
    BuildResult build(bool is_preview = false, 
                      const std::string& file_prefix = "",
                      const std::string& override_output_location = "");

private:
    // Image source type handlers
    void add_camera_image_source(const models::ImageSourceConfiguration& config,
                                 const std::string& override_processing_pipeline);
    void add_icam_image_source(const models::ImageSourceConfiguration& config,
                               const std::string& override_processing_pipeline);
    void add_file_image_source(const std::string& file_path);

    // Pipeline stage handlers
    void add_pre_processing_plugins();
    void add_inference_plugins();
    void add_post_processing_plugins();
    void add_output_plugins();

    PipelineConfiguration pipeline_config_;
    std::optional<models::Workflow> workflow_;
    std::optional<models::ImageSource> image_source_;
    std::string capture_id_;
    std::string em_agent_config_path_;
};

} // namespace pipeline
} // namespace dda
