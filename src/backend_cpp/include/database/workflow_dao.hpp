#pragma once

#include "models/models.hpp"
#include "database/db_manager.hpp"
#include <vector>
#include <optional>

namespace dda {
namespace database {

class WorkflowDao {
public:
    static WorkflowDao& instance();

    // CRUD operations
    models::Workflow create(const models::Workflow& workflow);
    std::optional<models::Workflow> get_by_id(const std::string& workflow_id);
    std::vector<models::Workflow> list_all();
    std::vector<models::Workflow> list_with_image_sources();
    models::Workflow update(const models::Workflow& workflow);
    bool remove(const std::string& workflow_id);

    // Helper methods
    bool exists(const std::string& workflow_id);
    std::vector<std::string> get_all_workflow_ids();

private:
    WorkflowDao() = default;
    
    models::Workflow row_to_workflow(PreparedStatement& stmt);
    void bind_workflow_params(PreparedStatement& stmt, const models::Workflow& workflow);
};

class WorkflowMetadataDao {
public:
    static WorkflowMetadataDao& instance();

    models::WorkflowMetadata create(const models::WorkflowMetadata& metadata);
    std::optional<models::WorkflowMetadata> get_by_id(const std::string& workflow_id);
    std::vector<models::WorkflowMetadata> list_all();
    models::WorkflowMetadata update(const models::WorkflowMetadata& metadata);
    bool remove(const std::string& workflow_id);

private:
    WorkflowMetadataDao() = default;
};

} // namespace database
} // namespace dda
