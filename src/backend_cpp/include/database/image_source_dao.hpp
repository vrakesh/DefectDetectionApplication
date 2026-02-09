#pragma once

#include "models/models.hpp"
#include "database/db_manager.hpp"
#include <vector>
#include <optional>

namespace dda {
namespace database {

class ImageSourceConfigurationDao {
public:
    static ImageSourceConfigurationDao& instance();

    models::ImageSourceConfiguration create(const models::ImageSourceConfiguration& config);
    std::optional<models::ImageSourceConfiguration> get_by_id(const std::string& config_id);
    models::ImageSourceConfiguration update(const models::ImageSourceConfiguration& config);
    bool remove(const std::string& config_id);

private:
    ImageSourceConfigurationDao() = default;
};

class ImageSourceDao {
public:
    static ImageSourceDao& instance();

    // CRUD operations
    models::ImageSource create(const models::ImageSource& image_source);
    std::optional<models::ImageSource> get_by_id(const std::string& image_source_id);
    std::vector<models::ImageSource> list_all();
    models::ImageSource update(const models::ImageSource& image_source);
    bool remove(const std::string& image_source_id);

    // Helper methods
    bool exists(const std::string& image_source_id);
    std::vector<std::string> list_cameras_used_by_image_sources();

private:
    ImageSourceDao() = default;
    
    models::ImageSource row_to_image_source(PreparedStatement& stmt);
    void bind_image_source_params(PreparedStatement& stmt, const models::ImageSource& image_source);
};

class InputConfigurationDao {
public:
    static InputConfigurationDao& instance();

    models::InputConfiguration create(const models::InputConfiguration& config);
    std::optional<models::InputConfiguration> get_by_id(const std::string& config_id);
    models::InputConfiguration update(const models::InputConfiguration& config);
    bool remove(const std::string& config_id);

private:
    InputConfigurationDao() = default;
};

class OutputConfigurationDao {
public:
    static OutputConfigurationDao& instance();

    models::OutputConfiguration create(const models::OutputConfiguration& config);
    std::optional<models::OutputConfiguration> get_by_id(const std::string& config_id);
    models::OutputConfiguration update(const models::OutputConfiguration& config);
    bool remove(const std::string& config_id);

private:
    OutputConfigurationDao() = default;
};

} // namespace database
} // namespace dda
