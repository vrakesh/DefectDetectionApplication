#pragma once

#include "models/models.hpp"
#include <string>
#include <memory>
#include <vector>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

namespace dda {
namespace pipeline {

// Plugin argument for pipeline building
struct PluginArg {
    std::string name;
    std::string value;

    PluginArg(const std::string& n, const std::string& v) : name(n), value(v) {}
    PluginArg(const std::string& n, int v) : name(n), value(std::to_string(v)) {}
};

// Plugin definition for pipeline building
struct PluginDefinition {
    std::string plugin_name;
    std::vector<PluginArg> args;

    explicit PluginDefinition(const std::string& name) : plugin_name(name) {}
    PluginDefinition(const std::string& name, std::vector<PluginArg> arguments)
        : plugin_name(name), args(std::move(arguments)) {}

    std::string to_string() const;
};

// GStreamer pipeline manager
class GstPipelineManager {
public:
    GstPipelineManager();
    ~GstPipelineManager();

    // Initialize GStreamer (call once at startup)
    static bool initialize();
    static void deinitialize();

    // Run a pipeline with optional frame data for appsrc
    struct PipelineResult {
        bool success = false;
        std::string error;
        json parsed_tags;  // Metadata extracted from pipeline
    };

    PipelineResult run_pipeline(const std::string& pipeline_str, 
                                const models::FrameData* frame_data = nullptr);

private:
    void push_frame_to_appsrc(GstElement* appsrc, const models::FrameData& frame);
    static void on_need_data(GstAppSrc* src, guint length, gpointer user_data);
    static GstFlowReturn on_new_sample(GstAppSink* sink, gpointer user_data);

    bool gst_initialized_ = false;
};

// Pipeline configuration builder
class PipelineConfiguration {
public:
    void add_plugin(const PluginDefinition& plugin);
    void add_plugin(const std::string& raw_plugin_string);

    std::string build_pipeline_string() const;
    void clear();

private:
    std::vector<std::variant<PluginDefinition, std::string>> plugins_;
};

} // namespace pipeline
} // namespace dda
