#pragma once

#include "models/models.hpp"
#include <string>
#include <vector>
#include <memory>
#include <mutex>

// EdgeML SDK Panorama headers (conditionally included when EdgeML SDK is available)
#ifdef HAVE_EDGEML_SDK
#include <Panorama/mlops.h>
#include <Panorama/comptr.h>
#include <Panorama/buffer.h>
#endif

namespace dda {
namespace triton {

class TritonClient {
public:
    static TritonClient& instance();

    // Initialize the Triton server
    bool initialize(const std::string& model_dir, const std::string& installation_dir);
    void shutdown();

    // Model management
    std::vector<models::ModelInfo> list_models();
    models::ModelInfo get_model_info(const std::string& model_id);
    std::string get_model_status(const std::string& model_id);
    
    // Model lifecycle
    bool load_model(const std::string& model_id);
    bool unload_model(const std::string& model_id);

    // Inference
    struct InferenceInput {
        std::string name;
        std::vector<int64_t> shape;
        std::vector<uint8_t> data;
    };

    struct InferenceOutput {
        std::string name;
        std::vector<int64_t> shape;
        std::vector<uint8_t> data;
    };

    bool run_inference(
        const std::string& model_name,
        const std::vector<InferenceInput>& inputs,
        std::vector<InferenceOutput>& outputs
    );

    // Check if Triton is available
    bool is_initialized() const { return initialized_; }
    std::string get_metrics();

    // Model conversion (calls Python converter script)
    bool convert_model(const std::string& model_zip_path,
                       const std::string& model_name,
                       const std::string& model_version = "1");
    
    // Convert and load model in one step
    bool convert_and_load_model(const std::string& model_zip_path,
                                 const std::string& model_name,
                                 const std::string& model_version = "1");

private:
    TritonClient() = default;
    ~TritonClient();

    // Prevent copying
    TritonClient(const TritonClient&) = delete;
    TritonClient& operator=(const TritonClient&) = delete;

#ifdef HAVE_EDGEML_SDK
    Panorama::ComPtr<Panorama::IInferenceServer> server_;
#else
    void* server_ = nullptr;  // Placeholder when EdgeML SDK not available
#endif

    std::string model_dir_;
    std::string installation_dir_;
    bool initialized_ = false;
    mutable std::mutex mutex_;
};

// Helper function to check if Triton mode is enabled
bool is_triton_enabled();

} // namespace triton
} // namespace dda
