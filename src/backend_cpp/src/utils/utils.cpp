/**
 * @file utils.cpp
 * @brief Utility functions implementation
 */

#include "utils/utils.hpp"
#include "common/types.hpp"
#include <fstream>
#include <sstream>
#include <random>
#include <iomanip>
#include <algorithm>
#include <cstdlib>
#include <chrono>

namespace dda {
namespace utils {

std::string generate_uuid() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static std::uniform_int_distribution<> dis2(8, 11);
    
    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 8; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 4; i++) ss << dis(gen);
    ss << "-4";
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    ss << dis2(gen);
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 12; i++) ss << dis(gen);
    return ss.str();
}

std::string generate_capture_id(const std::string& workflow_id) {
    auto timestamp = get_current_timestamp_ms();
    return workflow_id + "-" + std::to_string(timestamp);
}

std::vector<uint8_t> read_file_bytes(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Cannot open file: " + path);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(file), {});
}

void write_file_bytes(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Cannot create file: " + path);
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
}

bool file_exists(const std::string& path) {
    return std::filesystem::exists(path);
}

int64_t file_size(const std::string& path) {
    return static_cast<int64_t>(std::filesystem::file_size(path));
}

void delete_file(const std::string& path) {
    std::filesystem::remove(path);
}

void create_directory(const std::string& path) {
    std::filesystem::create_directory(path);
}

void create_directories(const std::string& path) {
    std::filesystem::create_directories(path);
}

std::string get_oldest_file(const std::string& directory, const std::string& extension) {
    std::string oldest_file;
    std::filesystem::file_time_type oldest_time = std::filesystem::file_time_type::max();
    
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == extension) {
            auto time = entry.last_write_time();
            if (time < oldest_time) {
                oldest_time = time;
                oldest_file = entry.path().string();
            }
        }
    }
    return oldest_file;
}

std::string get_env(const std::string& name, const std::string& default_value) {
    const char* val = std::getenv(name.c_str());
    return val ? val : default_value;
}

bool get_env_bool(const std::string& name, bool default_value) {
    const char* val = std::getenv(name.c_str());
    if (!val) return default_value;
    std::string s(val);
    return s == "true" || s == "1" || s == "yes";
}

int get_env_int(const std::string& name, int default_value) {
    const char* val = std::getenv(name.c_str());
    return val ? std::stoi(val) : default_value;
}

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string join(const std::vector<std::string>& parts, const std::string& delimiter) {
    std::ostringstream oss;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) oss << delimiter;
        oss << parts[i];
    }
    return oss.str();
}

std::string trim(const std::string& str) {
    auto start = str.find_first_not_of(" \t\n\r");
    auto end = str.find_last_not_of(" \t\n\r");
    return (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
}

} // namespace utils
} // namespace dda
