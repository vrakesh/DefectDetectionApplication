#pragma once

#include "models/models.hpp"
#include "database/db_manager.hpp"
#include <vector>
#include <optional>

namespace dda {
namespace database {

struct InferenceResultQuery {
    std::string workflow_id;
    std::optional<int64_t> start_time;
    std::optional<int64_t> end_time;
    std::optional<models::Prediction> prediction;
    std::optional<bool> flag_for_review;
    std::optional<bool> human_review_required;
    int offset = 0;
    int limit = 100;
};

class InferenceResultDao {
public:
    static InferenceResultDao& instance();

    // CRUD operations
    models::InferenceResult create(const models::InferenceResult& result);
    std::optional<models::InferenceResult> get_by_id(const std::string& capture_id);
    std::vector<models::InferenceResult> query(const InferenceResultQuery& query);
    models::InferenceResult update(const models::InferenceResult& result);
    bool remove(const std::string& capture_id);

    // Bulk operations
    int remove_by_workflow_id(const std::string& workflow_id);
    int count_by_workflow_id(const std::string& workflow_id);
    
    // Summary statistics
    struct WorkflowSummary {
        int total_count = 0;
        int normal_count = 0;
        int anomaly_count = 0;
        int flagged_count = 0;
        double avg_confidence = 0.0;
    };
    WorkflowSummary get_summary(const std::string& workflow_id, int64_t start_time);

private:
    InferenceResultDao() = default;
    
    models::InferenceResult row_to_result(PreparedStatement& stmt);
    void bind_result_params(PreparedStatement& stmt, const models::InferenceResult& result);
};

class LatencyTimeDao {
public:
    static LatencyTimeDao& instance();

    void store(const std::string& capture_id, const std::string& latency_type, double timestamp);
    std::vector<std::pair<std::string, double>> get_by_capture_id(const std::string& capture_id);
    bool remove_by_capture_id(const std::string& capture_id);

private:
    LatencyTimeDao() = default;
};

} // namespace database
} // namespace dda
