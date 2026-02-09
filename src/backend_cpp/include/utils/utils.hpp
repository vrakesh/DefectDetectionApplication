#pragma once

#include <string>
#include <vector>
#include <random>
#include <filesystem>

namespace dda {
namespace utils {

// UUID generation
std::string generate_uuid();

// Capture ID generation
std::string generate_capture_id(const std::string& workflow_id);

// File operations
std::vector<uint8_t> read_file_bytes(const std::string& path);
void write_file_bytes(const std::string& path, const std::vector<uint8_t>& data);
bool file_exists(const std::string& path);
int64_t file_size(const std::string& path);
void delete_file(const std::string& path);
void create_directory(const std::string& path);
void create_directories(const std::string& path);

// Get oldest file in directory (for folder-based image sources)
std::string get_oldest_file(const std::string& directory, const std::string& extension = ".jpg");

// Image utilities
std::string image_to_base64(const std::vector<uint8_t>& image_data);
std::vector<uint8_t> base64_to_image(const std::string& base64_str);

// Path utilities
std::string get_em_agent_config_path(const std::string& workflow_id);
std::string get_workflow_output_path(const std::string& workflow_id);

// Environment utilities
std::string get_env(const std::string& name, const std::string& default_value = "");
bool get_env_bool(const std::string& name, bool default_value = false);
int get_env_int(const std::string& name, int default_value = 0);

// String utilities
std::vector<std::string> split(const std::string& str, char delimiter);
std::string join(const std::vector<std::string>& parts, const std::string& delimiter);
std::string trim(const std::string& str);
std::string escape_json_string(const std::string& str);

} // namespace utils
} // namespace dda
