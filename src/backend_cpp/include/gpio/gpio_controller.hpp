#pragma once

#include "common/types.hpp"
#include <string>
#include <memory>
#include <mutex>
#include <unordered_map>

// Forward declare c-periphery gpio structure
struct gpio_handle;
typedef struct gpio_handle gpio_t;

namespace dda {
namespace gpio {

/**
 * @brief GPIO pin wrapper using c-periphery
 * 
 * This matches the Python periphery library used by the Flask backend.
 * Uses sysfs interface for GPIO access.
 */
class GpioPin {
public:
    GpioPin(unsigned int pin, const std::string& direction);
    ~GpioPin();

    // Non-copyable
    GpioPin(const GpioPin&) = delete;
    GpioPin& operator=(const GpioPin&) = delete;

    // Read/write
    bool read();
    void write(bool value);

    // Poll for edge event
    bool poll(int timeout_ms);

    // Get pin info
    unsigned int pin_number() const { return pin_; }
    bool is_output() const { return is_output_; }
    bool is_valid() const { return gpio_ != nullptr; }

    // Close the pin
    void close();

private:
    gpio_t* gpio_ = nullptr;
    unsigned int pin_;
    bool is_output_;
};

/**
 * @brief GPIO Controller using c-periphery
 * 
 * Provides the same functionality as python-periphery used in Flask backend:
 * - GPIO(line=pin, direction="in"/"out")
 * - gpio.read() / gpio.write(value)
 * - Polling for edge detection
 */
class GpioController {
public:
    static GpioController& instance();

    // Initialize - just marks as ready (pins opened on demand)
    bool initialize();
    void shutdown();

    // Input operations - matching Flask backend
    bool read_input(unsigned int pin);
    bool poll_input(unsigned int pin, int timeout_ms);

    // Output operations - matching Flask backend dio.py
    void write_output(unsigned int pin, bool value);
    void pulse_output(unsigned int pin, GpioEdge edge, int pulse_width_ms);
    void reset_output(unsigned int pin, GpioEdge signal_type);

    // Check if initialized
    bool is_initialized() const { return initialized_; }

private:
    GpioController() = default;
    ~GpioController();

    // Prevent copying
    GpioController(const GpioController&) = delete;
    GpioController& operator=(const GpioController&) = delete;

    // Get or create pin
    GpioPin* get_input_pin(unsigned int pin);
    GpioPin* get_output_pin(unsigned int pin);

    std::unordered_map<unsigned int, std::unique_ptr<GpioPin>> input_pins_;
    std::unordered_map<unsigned int, std::unique_ptr<GpioPin>> output_pins_;
    std::mutex mutex_;
    bool initialized_ = false;
};

} // namespace gpio
} // namespace dda
