/**
 * @file gpio_controller.cpp
 * @brief GPIO Controller implementation using c-periphery
 * 
 * This implementation matches the Python periphery library used in Flask backend.
 * c-periphery is the C version of python-periphery by the same author (vsergeev).
 */

#include "gpio/gpio_controller.hpp"
#include <spdlog/spdlog.h>
#include <chrono>
#include <thread>

extern "C" {
#include <gpio.h>
}

namespace dda {
namespace gpio {

// ===== GpioPin Implementation =====

GpioPin::GpioPin(unsigned int pin, const std::string& direction)
    : pin_(pin), is_output_(direction == "out") {
    
    gpio_ = gpio_new();
    if (!gpio_) {
        spdlog::error("Failed to allocate GPIO for pin {}", pin);
        return;
    }

    // Open GPIO using sysfs interface (same as python-periphery)
    int ret = gpio_open_sysfs(gpio_, pin, 
        is_output_ ? GPIO_DIR_OUT : GPIO_DIR_IN);
    
    if (ret < 0) {
        spdlog::error("Failed to open GPIO pin {}: {}", pin, gpio_errmsg(gpio_));
        gpio_free(gpio_);
        gpio_ = nullptr;
        return;
    }

    spdlog::info("GPIO pin {} opened as {}", pin, direction);
}

GpioPin::~GpioPin() {
    close();
}

void GpioPin::close() {
    if (gpio_) {
        gpio_close(gpio_);
        gpio_free(gpio_);
        gpio_ = nullptr;
        spdlog::info("GPIO pin {} closed", pin_);
    }
}

bool GpioPin::read() {
    if (!gpio_) {
        spdlog::warn("Attempt to read from invalid GPIO pin {}", pin_);
        return false;
    }

    bool value = false;
    int ret = gpio_read(gpio_, &value);
    if (ret < 0) {
        spdlog::error("Failed to read GPIO pin {}: {}", pin_, gpio_errmsg(gpio_));
        return false;
    }

    return value;
}

void GpioPin::write(bool value) {
    if (!gpio_) {
        spdlog::warn("Attempt to write to invalid GPIO pin {}", pin_);
        return;
    }

    int ret = gpio_write(gpio_, value);
    if (ret < 0) {
        spdlog::error("Failed to write GPIO pin {}: {}", pin_, gpio_errmsg(gpio_));
    }
}

bool GpioPin::poll(int timeout_ms) {
    if (!gpio_) {
        return false;
    }

    // Set edge detection for polling
    int ret = gpio_set_edge(gpio_, GPIO_EDGE_BOTH);
    if (ret < 0) {
        spdlog::warn("Failed to set edge detection for pin {}: {}", pin_, gpio_errmsg(gpio_));
        return false;
    }

    ret = gpio_poll(gpio_, timeout_ms);
    if (ret < 0) {
        spdlog::error("GPIO poll error on pin {}: {}", pin_, gpio_errmsg(gpio_));
        return false;
    }

    return ret > 0;  // ret > 0 means edge detected
}

// ===== GpioController Implementation =====

GpioController& GpioController::instance() {
    static GpioController controller;
    return controller;
}

GpioController::~GpioController() {
    shutdown();
}

bool GpioController::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (initialized_) {
        return true;
    }

    spdlog::info("GPIO controller initialized (using c-periphery sysfs interface)");
    initialized_ = true;
    return true;
}

void GpioController::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        return;
    }

    // Close all pins
    for (auto& pair : input_pins_) {
        if (pair.second) {
            pair.second->close();
        }
    }
    input_pins_.clear();

    for (auto& pair : output_pins_) {
        if (pair.second) {
            pair.second->close();
        }
    }
    output_pins_.clear();

    initialized_ = false;
    spdlog::info("GPIO controller shutdown");
}

GpioPin* GpioController::get_input_pin(unsigned int pin) {
    auto it = input_pins_.find(pin);
    if (it != input_pins_.end()) {
        return it->second.get();
    }

    // Create new input pin
    auto gpio_pin = std::make_unique<GpioPin>(pin, "in");
    if (!gpio_pin->is_valid()) {
        return nullptr;
    }

    auto* ptr = gpio_pin.get();
    input_pins_[pin] = std::move(gpio_pin);
    return ptr;
}

GpioPin* GpioController::get_output_pin(unsigned int pin) {
    auto it = output_pins_.find(pin);
    if (it != output_pins_.end()) {
        return it->second.get();
    }

    // Create new output pin
    auto gpio_pin = std::make_unique<GpioPin>(pin, "out");
    if (!gpio_pin->is_valid()) {
        return nullptr;
    }

    auto* ptr = gpio_pin.get();
    output_pins_[pin] = std::move(gpio_pin);
    return ptr;
}

bool GpioController::read_input(unsigned int pin) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        spdlog::warn("GPIO controller not initialized");
        return false;
    }

    auto* gpio_pin = get_input_pin(pin);
    if (!gpio_pin) {
        return false;
    }

    return gpio_pin->read();
}

bool GpioController::poll_input(unsigned int pin, int timeout_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        return false;
    }

    auto* gpio_pin = get_input_pin(pin);
    if (!gpio_pin) {
        return false;
    }

    return gpio_pin->poll(timeout_ms);
}

void GpioController::write_output(unsigned int pin, bool value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        spdlog::warn("GPIO controller not initialized");
        return;
    }

    auto* gpio_pin = get_output_pin(pin);
    if (!gpio_pin) {
        return;
    }

    gpio_pin->write(value);
}

void GpioController::pulse_output(unsigned int pin, GpioEdge edge, int pulse_width_ms) {
    // Match Flask backend dio.py pulse_output_pin behavior
    // GPIO.RISING: set to True, wait, set to False
    // GPIO.FALLING: set to False, wait, set to True
    
    bool active_value = (edge == GpioEdge::RISING);
    bool inactive_value = !active_value;

    write_output(pin, active_value);
    std::this_thread::sleep_for(std::chrono::milliseconds(pulse_width_ms));
    write_output(pin, inactive_value);
}

void GpioController::reset_output(unsigned int pin, GpioEdge signal_type) {
    // Match Flask backend dio.py reset_output_pin behavior
    // GPIO.RISING: reset to False
    // GPIO.FALLING: reset to True
    
    bool reset_value = (signal_type == GpioEdge::FALLING);
    write_output(pin, reset_value);
}

} // namespace gpio
} // namespace dda
