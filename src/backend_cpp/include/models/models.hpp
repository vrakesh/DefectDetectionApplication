#pragma once

#include "common/types.hpp"
#include <string>
#include <vector>
#include <optional>
#include <memory>

namespace dda {
namespace models {

// Image crop configuration
struct ImageCrop {
    int top = 0;
    int bottom = 0;
    int left = 0;
    int right = 0;

    json to_json() const {
        return {{"top", top}, {"bottom", bottom}, {"left", left}, {"right", right}};
    }

    static ImageCrop from_json(const json& j) {
        ImageCrop crop;
        if (j.contains("top")) crop.top = j["top"].get<int>();
        if (j.contains("bottom")) crop.bottom = j["bottom"].get<int>();
        if (j.contains("left")) crop.left = j["left"].get<int>();
        if (j.contains("right")) crop.right = j["right"].get<int>();
        return crop;
    }
};

// Image source configuration
struct ImageSourceConfiguration {
    std::string config_id;
    int gain = 0;
    int exposure = 0;
    std::string processing_pipeline;
    int64_t creation_time = 0;
    std::optional<ImageCrop> image_crop;
    std::string device;       // for ICAM
    std::string device_name;  // for ICAM

    json to_json() const {
        json j = {
            {"imageSourceConfigId", config_id},
            {"gain", gain},
            {"exposure", exposure},
            {"processingPipeline", processing_pipeline},
            {"creationTime", creation_time}
        };
        if (image_crop) {
            j["imageCrop"] = image_crop->to_json();
        }
        if (!device.empty()) j["device"] = device;
        if (!device_name.empty()) j["deviceName"] = device_name;
        return j;
    }

    static ImageSourceConfiguration from_json(const json& j) {
        ImageSourceConfiguration config;
        if (j.contains("imageSourceConfigId")) config.config_id = j["imageSourceConfigId"].get<std::string>();
        if (j.contains("gain")) config.gain = j["gain"].get<int>();
        if (j.contains("exposure")) config.exposure = j["exposure"].get<int>();
        if (j.contains("processingPipeline")) config.processing_pipeline = j["processingPipeline"].get<std::string>();
        if (j.contains("creationTime")) config.creation_time = j["creationTime"].get<int64_t>();
        if (j.contains("imageCrop") && !j["imageCrop"].is_null()) {
            config.image_crop = ImageCrop::from_json(j["imageCrop"]);
        }
        if (j.contains("device")) config.device = j["device"].get<std::string>();
        if (j.contains("deviceName")) config.device_name = j["deviceName"].get<std::string>();
        return config;
    }
};

// Input configuration (GPIO trigger)
struct InputConfiguration {
    std::string config_id;
    int64_t creation_time = 0;
    int pin = 0;
    GpioEdge trigger_state = GpioEdge::RISING;
    int debounce_time_ms = 0;

    json to_json() const {
        return {
            {"inputConfigurationId", config_id},
            {"creationTime", creation_time},
            {"pin", std::to_string(pin)},
            {"triggerState", to_string(trigger_state)},
            {"debounceTime", debounce_time_ms}
        };
    }

    static InputConfiguration from_json(const json& j) {
        InputConfiguration config;
        if (j.contains("inputConfigurationId")) config.config_id = j["inputConfigurationId"].get<std::string>();
        if (j.contains("creationTime")) config.creation_time = j["creationTime"].get<int64_t>();
        if (j.contains("pin")) config.pin = std::stoi(j["pin"].get<std::string>());
        if (j.contains("triggerState")) config.trigger_state = gpio_edge_from_string(j["triggerState"].get<std::string>());
        if (j.contains("debounceTime")) config.debounce_time_ms = j["debounceTime"].get<int>();
        return config;
    }
};

// Output configuration (GPIO output)
struct OutputConfiguration {
    std::string config_id;
    int pin = 0;
    GpioEdge signal_type = GpioEdge::RISING;
    int pulse_width_ms = 0;
    int64_t creation_time = 0;
    OutputRule rule = OutputRule::ALL;

    json to_json() const {
        return {
            {"outputConfigurationId", config_id},
            {"pin", std::to_string(pin)},
            {"signalType", to_string(signal_type)},
            {"pulseWidth", pulse_width_ms},
            {"creationTime", creation_time},
            {"rule", to_string(rule)}
        };
    }

    static OutputConfiguration from_json(const json& j) {
        OutputConfiguration config;
        if (j.contains("outputConfigurationId")) config.config_id = j["outputConfigurationId"].get<std::string>();
        if (j.contains("pin")) config.pin = std::stoi(j["pin"].get<std::string>());
        if (j.contains("signalType")) config.signal_type = gpio_edge_from_string(j["signalType"].get<std::string>());
        if (j.contains("pulseWidth")) config.pulse_width_ms = j["pulseWidth"].get<int>();
        if (j.contains("creationTime")) config.creation_time = j["creationTime"].get<int64_t>();
        if (j.contains("rule")) config.rule = output_rule_from_string(j["rule"].get<std::string>());
        return config;
    }
};

// Feature configuration (model settings)
struct FeatureConfiguration {
    std::string model_name;
    std::string model_id;
    json additional_config;

    json to_json() const {
        json j = {{"modelName", model_name}};
        if (!model_id.empty()) j["modelId"] = model_id;
        if (!additional_config.is_null()) {
            for (auto& [key, value] : additional_config.items()) {
                j[key] = value;
            }
        }
        return j;
    }

    static FeatureConfiguration from_json(const json& j) {
        FeatureConfiguration config;
        if (j.contains("modelName")) config.model_name = j["modelName"].get<std::string>();
        if (j.contains("modelId")) config.model_id = j["modelId"].get<std::string>();
        config.additional_config = j;
        return config;
    }
};

// Image source
struct ImageSource {
    std::string image_source_id;
    std::string name;
    ImageSourceType type = ImageSourceType::CAMERA;
    std::string location;        // for folder type
    std::string camera_id;       // for camera type
    std::string description;
    int64_t creation_time = 0;
    int64_t last_update_time = 0;
    std::string image_capture_path;
    std::optional<ImageSourceConfiguration> configuration;

    json to_json() const {
        json j = {
            {"imageSourceId", image_source_id},
            {"name", name},
            {"type", to_string(type)},
            {"description", description},
            {"creationTime", creation_time},
            {"lastUpdateTime", last_update_time},
            {"imageCapturePath", image_capture_path}
        };
        if (!location.empty()) j["location"] = location;
        if (!camera_id.empty()) j["cameraId"] = camera_id;
        if (configuration) {
            j["imageSourceConfiguration"] = configuration->to_json();
        }
        return j;
    }

    static ImageSource from_json(const json& j) {
        ImageSource src;
        if (j.contains("imageSourceId")) src.image_source_id = j["imageSourceId"].get<std::string>();
        if (j.contains("name")) src.name = j["name"].get<std::string>();
        if (j.contains("type")) src.type = image_source_type_from_string(j["type"].get<std::string>());
        if (j.contains("location")) src.location = j["location"].get<std::string>();
        if (j.contains("cameraId")) src.camera_id = j["cameraId"].get<std::string>();
        if (j.contains("description")) src.description = j["description"].get<std::string>();
        if (j.contains("creationTime")) src.creation_time = j["creationTime"].get<int64_t>();
        if (j.contains("lastUpdateTime")) src.last_update_time = j["lastUpdateTime"].get<int64_t>();
        if (j.contains("imageCapturePath")) src.image_capture_path = j["imageCapturePath"].get<std::string>();
        if (j.contains("imageSourceConfiguration") && !j["imageSourceConfiguration"].is_null()) {
            src.configuration = ImageSourceConfiguration::from_json(j["imageSourceConfiguration"]);
        }
        return src;
    }
};

// Workflow
struct Workflow {
    std::string workflow_id;
    std::string name;
    std::string description;
    int64_t creation_time = 0;
    int64_t last_updated_time = 0;
    std::string workflow_output_path;
    std::string image_source_id;
    std::vector<FeatureConfiguration> feature_configurations;
    std::vector<InputConfiguration> input_configurations;
    std::vector<OutputConfiguration> output_configurations;

    json to_json() const {
        json j = {
            {"workflowId", workflow_id},
            {"name", name},
            {"description", description},
            {"creationTime", creation_time},
            {"lastUpdatedTime", last_updated_time},
            {"workflowOutputPath", workflow_output_path},
            {"imageSourceId", image_source_id}
        };
        
        json features = json::array();
        for (const auto& fc : feature_configurations) {
            features.push_back(fc.to_json());
        }
        j["featureConfigurations"] = features;

        json inputs = json::array();
        for (const auto& ic : input_configurations) {
            inputs.push_back(ic.to_json());
        }
        j["inputConfigurations"] = inputs;

        json outputs = json::array();
        for (const auto& oc : output_configurations) {
            outputs.push_back(oc.to_json());
        }
        j["outputConfigurations"] = outputs;

        return j;
    }

    static Workflow from_json(const json& j) {
        Workflow wf;
        if (j.contains("workflowId")) wf.workflow_id = j["workflowId"].get<std::string>();
        if (j.contains("name")) wf.name = j["name"].get<std::string>();
        if (j.contains("description")) wf.description = j["description"].get<std::string>();
        if (j.contains("creationTime")) wf.creation_time = j["creationTime"].get<int64_t>();
        if (j.contains("lastUpdatedTime")) wf.last_updated_time = j["lastUpdatedTime"].get<int64_t>();
        if (j.contains("workflowOutputPath")) wf.workflow_output_path = j["workflowOutputPath"].get<std::string>();
        if (j.contains("imageSourceId")) wf.image_source_id = j["imageSourceId"].get<std::string>();
        
        if (j.contains("featureConfigurations") && j["featureConfigurations"].is_array()) {
            for (const auto& fc : j["featureConfigurations"]) {
                wf.feature_configurations.push_back(FeatureConfiguration::from_json(fc));
            }
        }
        if (j.contains("inputConfigurations") && j["inputConfigurations"].is_array()) {
            for (const auto& ic : j["inputConfigurations"]) {
                wf.input_configurations.push_back(InputConfiguration::from_json(ic));
            }
        }
        if (j.contains("outputConfigurations") && j["outputConfigurations"].is_array()) {
            for (const auto& oc : j["outputConfigurations"]) {
                wf.output_configurations.push_back(OutputConfiguration::from_json(oc));
            }
        }
        return wf;
    }

    bool has_input_configurations() const {
        return !input_configurations.empty();
    }

    bool has_feature_configurations() const {
        return !feature_configurations.empty();
    }

    bool has_output_configurations() const {
        return !output_configurations.empty();
    }
};

// Inference result
struct InferenceResult {
    std::string capture_id;
    CaptureType capture_type = CaptureType::INFERENCE;
    std::string workflow_id;
    int64_t inference_creation_time = 0;
    Prediction prediction = Prediction::NORMAL;
    double confidence = 0.0;
    json anomaly_labels;
    double anomaly_score = 0.0;
    double anomaly_threshold = 0.0;
    std::string mask_image;
    json mask_background;
    std::string input_image_file_path;
    std::string output_image_file_path;
    std::string model_id;
    std::string model_name;
    bool flag_for_review = false;
    bool downloaded = false;
    std::optional<Prediction> human_classification;
    std::string text_note;
    bool human_review_required = false;
    json model_confidence_thresholds;

    json to_json() const {
        json j = {
            {"captureId", capture_id},
            {"captureType", capture_type == CaptureType::CAPTURE ? "capture" : "inference"},
            {"workflowId", workflow_id},
            {"inferenceCreationTime", inference_creation_time},
            {"prediction", to_string(prediction)},
            {"confidence", confidence},
            {"anomalyScore", anomaly_score},
            {"anomalyThreshold", anomaly_threshold},
            {"inputImageFilePath", input_image_file_path},
            {"outputImageFilePath", output_image_file_path},
            {"modelId", model_id},
            {"modelName", model_name},
            {"flagForReview", flag_for_review},
            {"downloaded", downloaded},
            {"textNote", text_note},
            {"humanReviewRequired", human_review_required}
        };
        if (!anomaly_labels.is_null()) j["anomalyLabels"] = anomaly_labels;
        if (!mask_image.empty()) j["maskImage"] = mask_image;
        if (!mask_background.is_null()) j["maskBackground"] = mask_background;
        if (human_classification) j["humanClassification"] = to_string(*human_classification);
        if (!model_confidence_thresholds.is_null()) j["modelConfidenceThresholds"] = model_confidence_thresholds;
        return j;
    }

    static InferenceResult from_json(const json& j) {
        InferenceResult result;
        if (j.contains("captureId")) result.capture_id = j["captureId"].get<std::string>();
        if (j.contains("captureType")) {
            result.capture_type = j["captureType"].get<std::string>() == "capture" ? CaptureType::CAPTURE : CaptureType::INFERENCE;
        }
        if (j.contains("workflowId")) result.workflow_id = j["workflowId"].get<std::string>();
        if (j.contains("inferenceCreationTime")) result.inference_creation_time = j["inferenceCreationTime"].get<int64_t>();
        if (j.contains("prediction")) result.prediction = prediction_from_string(j["prediction"].get<std::string>());
        if (j.contains("confidence")) result.confidence = j["confidence"].get<double>();
        if (j.contains("anomalyLabels")) result.anomaly_labels = j["anomalyLabels"];
        if (j.contains("anomalyScore")) result.anomaly_score = j["anomalyScore"].get<double>();
        if (j.contains("anomalyThreshold")) result.anomaly_threshold = j["anomalyThreshold"].get<double>();
        if (j.contains("maskImage")) result.mask_image = j["maskImage"].get<std::string>();
        if (j.contains("maskBackground")) result.mask_background = j["maskBackground"];
        if (j.contains("inputImageFilePath")) result.input_image_file_path = j["inputImageFilePath"].get<std::string>();
        if (j.contains("outputImageFilePath")) result.output_image_file_path = j["outputImageFilePath"].get<std::string>();
        if (j.contains("modelId")) result.model_id = j["modelId"].get<std::string>();
        if (j.contains("modelName")) result.model_name = j["modelName"].get<std::string>();
        if (j.contains("flagForReview")) result.flag_for_review = j["flagForReview"].get<bool>();
        if (j.contains("downloaded")) result.downloaded = j["downloaded"].get<bool>();
        if (j.contains("humanClassification") && !j["humanClassification"].is_null()) {
            result.human_classification = prediction_from_string(j["humanClassification"].get<std::string>());
        }
        if (j.contains("textNote")) result.text_note = j["textNote"].get<std::string>();
        if (j.contains("humanReviewRequired")) result.human_review_required = j["humanReviewRequired"].get<bool>();
        if (j.contains("modelConfidenceThresholds")) result.model_confidence_thresholds = j["modelConfidenceThresholds"];
        return result;
    }
};

// Workflow metadata
struct WorkflowMetadata {
    std::string workflow_id;
    int64_t summary_start_time = 0;

    json to_json() const {
        return {
            {"workflowId", workflow_id},
            {"summaryStartTime", summary_start_time}
        };
    }

    static WorkflowMetadata from_json(const json& j) {
        WorkflowMetadata meta;
        if (j.contains("workflowId")) meta.workflow_id = j["workflowId"].get<std::string>();
        if (j.contains("summaryStartTime")) meta.summary_start_time = j["summaryStartTime"].get<int64_t>();
        return meta;
    }
};

// Camera info from Aravis
struct CameraInfo {
    std::string id;
    std::string model;
    std::string address;
    std::string physical_id;
    std::string protocol;
    std::string serial;
    std::string vendor;

    json to_json() const {
        return {
            {"id", id},
            {"model", model},
            {"address", address},
            {"physicalId", physical_id},
            {"protocol", protocol},
            {"serial", serial},
            {"vendor", vendor}
        };
    }
};

// Camera status
struct CameraStatusInfo {
    CameraStatus status = CameraStatus::DISCONNECTED;
    int64_t last_updated_time = 0;
    std::string error;

    json to_json() const {
        json j = {
            {"status", status == CameraStatus::CONNECTED ? "connected" : 
                      (status == CameraStatus::ERROR ? "error" : "disconnected")},
            {"lastUpdatedTime", last_updated_time}
        };
        if (!error.empty()) j["error"] = error;
        return j;
    }
};

// Frame data from camera
struct FrameData {
    std::vector<uint8_t> data;
    int width = 0;
    int height = 0;
    std::string pixel_format;
};

// DIO health status
struct DIOHealthReport {
    DIOHealthStatus status = DIOHealthStatus::STARTING;
    std::string error_type;
    int64_t last_updated = 0;

    json to_json() const {
        std::string status_str;
        switch (status) {
            case DIOHealthStatus::STARTING: status_str = "starting"; break;
            case DIOHealthStatus::RUNNING: status_str = "running"; break;
            case DIOHealthStatus::ERROR: status_str = "error"; break;
        }
        json j = {
            {"status", status_str},
            {"lastUpdated", last_updated}
        };
        if (!error_type.empty()) j["errorType"] = error_type;
        return j;
    }
};

// Model info from Triton
struct ModelInfo {
    std::string model_component;
    std::string status;
    std::string status_message;

    json to_json() const {
        return {
            {"modelComponent", model_component},
            {"status", status},
            {"statusMessage", status_message}
        };
    }
};

} // namespace models
} // namespace dda
