/**
 * @file triton_client.cpp
 * @brief Triton inference client implementation using EdgeML SDK
 */

#include "triton/triton_client.hpp"
#include <spdlog/spdlog.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>

namespace dda {
namespace triton {

TritonClient& TritonClient::instance() {
    static TritonClient client;
    return client;
}

TritonClient::~TritonClient() {
    shutdown();
}

bool TritonClient::initialize(const std::string& model_dir, const std::string& installation_dir) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (initialized_) {
        spdlog::warn("Triton client already initialized");
        return true;
    }

    model_dir_ = model_dir;
    installation_dir_ = installation_dir;

    spdlog::info("Initializing Triton client with model_dir={}, installation_dir={}", 
                 model_dir, installation_dir);

#ifdef HAVE_EDGEML_SDK
    // Use EdgeML SDK MLOps to create Triton inference server
    HRESULT hr = Panorama::MLOps::TritonInferenceServer(
        server_.AddressOf(),
        model_dir.c_str(),
        installation_dir.c_str(),
        false  // Use shared instance
    );

    if (FAILED(hr)) {
        spdlog::error("Failed to create Triton inference server: hr={:#x}", hr);
        return false;
    }

    spdlog::info("Triton inference server created successfully");
    initialized_ = true;
    return true;
#else
    // Stub implementation when EdgeML SDK is not available
    spdlog::warn("EdgeML SDK not available, Triton client running in stub mode");
    initialized_ = true;
    return true;
#endif
}

void TritonClient::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        return;
    }

    spdlog::info("Shutting down Triton client");

#ifdef HAVE_EDGEML_SDK
    server_.Release();
    Panorama::MLOps::ReleaseTritonServers();
#endif

    initialized_ = false;
}

std::vector<models::ModelInfo> TritonClient::list_models() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<models::ModelInfo> models;

    if (!initialized_) {
        return models;
    }

#ifdef HAVE_EDGEML_SDK
    const char* model_list_json = server_->ListModels();
    spdlog::info("ListModels raw response: {}", model_list_json ? model_list_json : "NULL");
    if (model_list_json) {
        try {
            json model_list = json::parse(model_list_json);
            spdlog::info("Parsed model_list type: {}, content: {}", 
                        model_list.is_object() ? "object" : (model_list.is_array() ? "array" : "other"),
                        model_list.dump());
            // Response is a dict: {"model_name": {"state": "READY"}, ...}
            for (auto& [model_name, model_state] : model_list.items()) {
                models::ModelInfo info;
                info.model_component = model_name;
                if (model_state.is_object()) {
                    info.status = model_state.value("state", "UNKNOWN");
                    spdlog::debug("Model {} state object: {}", model_name, model_state.dump());
                } else if (model_state.is_string()) {
                    info.status = model_state.get<std::string>();
                    spdlog::debug("Model {} state string: {}", model_name, info.status);
                } else {
                    info.status = "UNKNOWN";
                    spdlog::debug("Model {} state unknown type", model_name);
                }
                models.push_back(info);
            }
        } catch (const std::exception& e) {
            spdlog::error("Failed to parse model list: {}", e.what());
        }
    }
#else
    // Stub: return empty list
    spdlog::debug("list_models: stub mode, returning empty list");
#endif

    return models;
}

models::ModelInfo TritonClient::get_model_info(const std::string& model_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    models::ModelInfo info;
    info.model_component = model_id;

    if (!initialized_) {
        info.status = "NOT_INITIALIZED";
        return info;
    }

#ifdef HAVE_EDGEML_SDK
    const char* metadata_json = server_->ModelMetadata(model_id.c_str());
    if (metadata_json) {
        try {
            json metadata = json::parse(metadata_json);
            info.status = "READY";
            info.status_message = metadata.dump();
        } catch (const std::exception& e) {
            info.status = "ERROR";
            info.status_message = e.what();
        }
    } else {
        info.status = "NOT_FOUND";
    }
#else
    info.status = "STUB_MODE";
#endif

    return info;
}

std::string TritonClient::get_model_status(const std::string& model_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        return "NOT_INITIALIZED";
    }

#ifdef HAVE_EDGEML_SDK
    const char* status = server_->GetModelStatus(model_id.c_str());
    return status ? std::string(status) : "UNKNOWN";
#else
    return "STUB_MODE";
#endif
}

bool TritonClient::load_model(const std::string& model_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        spdlog::error("Cannot load model: Triton not initialized");
        return false;
    }

    spdlog::info("Loading model: {}", model_id);

#ifdef HAVE_EDGEML_SDK
    HRESULT hr = server_->LoadModel(model_id.c_str());
    if (FAILED(hr)) {
        spdlog::error("Failed to load model {}: hr={:#x}", model_id, hr);
        return false;
    }
    spdlog::info("Model {} loaded successfully", model_id);
    return true;
#else
    spdlog::info("Stub mode: pretending to load model {}", model_id);
    return true;
#endif
}

bool TritonClient::unload_model(const std::string& model_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        return false;
    }

    spdlog::info("Unloading model: {}", model_id);

#ifdef HAVE_EDGEML_SDK
    HRESULT hr = server_->UnloadModel(model_id.c_str());
    if (FAILED(hr)) {
        spdlog::error("Failed to unload model {}: hr={:#x}", model_id, hr);
        return false;
    }
    return true;
#else
    return true;
#endif
}

bool TritonClient::run_inference(
    const std::string& model_name,
    const std::vector<InferenceInput>& inputs,
    std::vector<InferenceOutput>& outputs) {
    
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        spdlog::error("Cannot run inference: Triton not initialized");
        return false;
    }

#ifdef HAVE_EDGEML_SDK
    // Create inference request
    Panorama::ComPtr<Panorama::IInferenceRequest> request;
    HRESULT hr = Panorama::MLOps::TritonRequest(request.AddressOf(), server_, model_name.c_str());
    if (FAILED(hr)) {
        spdlog::error("Failed to create inference request: hr={:#x}", hr);
        return false;
    }

    // Set input tensors
    for (size_t i = 0; i < inputs.size(); i++) {
        const auto& input = inputs[i];
        
        // Create buffer for input data
        Panorama::ComPtr<Panorama::IBuffer> buffer;
        hr = Panorama::CreateBuffer(buffer.AddressOf(), static_cast<int32_t>(input.data.size()));
        if (FAILED(hr)) continue;
        
        // Copy data into buffer
        std::memcpy(buffer->Data(), input.data.data(), input.data.size());

        // Create tensor
        Panorama::ComPtr<Panorama::ITensor> tensor;
        hr = Panorama::MLOps::Tensor(
            tensor.AddressOf(),
            input.name.c_str(),
            input.shape,
            Panorama::TensorDataType::UINT8,  // TODO: determine from input
            buffer
        );
        if (FAILED(hr)) continue;

        request->SetInput(tensor, static_cast<int32_t>(i));
    }

    // Process the request
    hr = server_->ProcessRequest(request);
    if (FAILED(hr)) {
        spdlog::error("Inference request failed: hr={:#x}", hr);
        return false;
    }

    // Wait for completion
    hr = request->WaitForRequestToComplete(30000);  // 30 second timeout
    if (FAILED(hr)) {
        spdlog::error("Inference request timed out: hr={:#x}", hr);
        return false;
    }

    // Extract outputs
    outputs.clear();
    uint32_t num_outputs = request->GetNumOfOutputTensors();
    for (uint32_t i = 0; i < num_outputs; i++) {
        Panorama::ComPtr<Panorama::ITensor> output_tensor;
        hr = request->Output(output_tensor.AddressOf(), i);
        if (FAILED(hr)) continue;

        InferenceOutput output;
        output.name = output_tensor->Name();

        // Get shape
        Panorama::ComPtr<Panorama::IInt64Vector> shape;
        hr = output_tensor->Shape(shape.AddressOf());
        if (SUCCEEDED(hr)) {
            // Convert shape to vector
            // (shape API depends on IInt64Vector interface)
        }

        // Get data
        Panorama::ComPtr<Panorama::IBuffer> buffer;
        hr = output_tensor->Buffer(buffer.AddressOf());
        if (SUCCEEDED(hr)) {
            const void* data = buffer->Data();
            size_t size = buffer->Size();
            output.data.assign(
                static_cast<const uint8_t*>(data),
                static_cast<const uint8_t*>(data) + size
            );
        }

        outputs.push_back(output);
    }

    return true;
#else
    // Stub mode
    spdlog::warn("Stub mode: inference not available without EdgeML SDK");
    return false;
#endif
}

std::string TritonClient::get_metrics() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        return "{}";
    }

#ifdef HAVE_EDGEML_SDK
    const char* metrics = server_->GetMetrics();
    return metrics ? std::string(metrics) : "{}";
#else
    return R"({"mode": "stub"})";
#endif
}

bool TritonClient::convert_model(const std::string& model_zip_path, 
                                  const std::string& model_name,
                                  const std::string& model_version) {
    spdlog::info("Converting model {} from {}", model_name, model_zip_path);
    
    // Create temp directory for extracted model
    std::string extract_dir = "/tmp/model_extract_" + model_name;
    
    // Remove existing directory if present
    std::filesystem::remove_all(extract_dir);
    std::filesystem::create_directories(extract_dir);
    
    // Extract the model zip
    std::string unzip_cmd = "unzip -o " + model_zip_path + " -d " + extract_dir;
    int ret = std::system(unzip_cmd.c_str());
    if (ret != 0) {
        spdlog::error("Failed to extract model zip: {}", model_zip_path);
        return false;
    }
    spdlog::info("Model extracted to {}", extract_dir);
    
    // Call Python model converter script
    std::string python_cmd = "python3.9 -c \""
        "import sys; sys.path.insert(0, '/app'); "
        "from dda_triton.model_convertor import convert_to_triton_structure; "
        "result = convert_to_triton_structure("
        "model_repo_dir='" + model_dir_ + "', "
        "deployed_model_path='" + extract_dir + "', "
        "model_name='" + model_name + "', "
        "model_version='" + model_version + "'); "
        "exit(0 if result else 1)\"";
    
    spdlog::info("Running model conversion: {}", python_cmd);
    ret = std::system(python_cmd.c_str());
    
    if (ret != 0) {
        spdlog::error("Model conversion failed for {}", model_name);
        return false;
    }
    
    spdlog::info("Model {} converted successfully", model_name);
    return true;
}

bool TritonClient::convert_and_load_model(const std::string& model_zip_path,
                                           const std::string& model_name,
                                           const std::string& model_version) {
    // First convert the model
    if (!convert_model(model_zip_path, model_name, model_version)) {
        return false;
    }
    
    // Then load it into Triton
    return load_model(model_name);
}

bool is_triton_enabled() {
    const char* mode = std::getenv("TRITON_MODE");
    return mode && std::string(mode) == "true";
}

} // namespace triton
} // namespace dda
