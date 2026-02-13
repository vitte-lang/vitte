#pragma once
#include <cstddef>
#include <cstdint>

namespace vitte::runtime {

void* alloc(std::size_t size);
void dealloc(void* ptr);

[[noreturn]]
void panic(const char* msg);

void assert_true(bool cond, const char* msg);
void print_i32(std::int32_t value);

[[noreturn]]
void unreachable();

void terminate();

} // namespace vitte::runtime

struct VitteUnit {
    std::uint8_t _dummy;
};

enum class VitteArduinoPinMode : std::uint8_t {
    Input = 0,
    Output = 1,
    InputPullup = 2
};

enum class VitteArduinoPinState : std::uint8_t {
    Low = 0,
    High = 1
};

extern "C" {

VitteUnit arduino_gpio_pin_mode(std::uint8_t pin, VitteArduinoPinMode mode);
VitteUnit arduino_gpio_digital_write(std::uint8_t pin, VitteArduinoPinState state);
VitteArduinoPinState arduino_gpio_digital_read(std::uint8_t pin);
VitteUnit arduino_gpio_analog_write(std::uint8_t pin, std::uint16_t value);
std::uint16_t arduino_gpio_analog_read(std::uint8_t pin);

VitteUnit arduino_serial_begin(std::uint32_t baud);
std::uint32_t arduino_serial_available();
std::int32_t arduino_serial_read();
std::size_t arduino_serial_write(std::uint8_t byte);
VitteUnit arduino_serial_flush();

VitteUnit arduino_i2c_begin();
VitteUnit arduino_i2c_begin_transmission(std::uint8_t addr);
std::size_t arduino_i2c_write(std::uint8_t byte);
std::uint8_t arduino_i2c_end_transmission();
std::uint8_t arduino_i2c_request_from(std::uint8_t addr, std::uint8_t len);
std::int32_t arduino_i2c_read();

VitteUnit arduino_spi_begin();
std::uint8_t arduino_spi_transfer(std::uint8_t byte);
VitteUnit arduino_spi_end();

VitteUnit arduino_delay_ms(std::uint32_t ms);
VitteUnit arduino_delay_us(std::uint32_t us);
std::uint32_t arduino_millis();
std::uint32_t arduino_micros();

} // extern "C"
