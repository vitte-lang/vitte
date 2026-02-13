#include "vitte_runtime.hpp"

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#include <cstdlib>

namespace vitte::runtime {

void* alloc(std::size_t size) {
    void* mem = std::malloc(size);
    if (!mem) {
        panic("alloc failed");
    }
    return mem;
}

void dealloc(void* ptr) {
    std::free(ptr);
}

[[noreturn]]
void panic(const char* msg) {
    Serial.begin(9600);
    Serial.println(msg ? msg : "panic");
    for (;;) {
        delay(1000);
    }
}

void assert_true(bool cond, const char* msg) {
    if (!cond) {
        panic(msg ? msg : "assert_true failed");
    }
}

void print_i32(std::int32_t value) {
    Serial.begin(9600);
    Serial.println(value);
}

[[noreturn]]
void unreachable() {
    panic("unreachable");
}

void terminate() {
    for (;;) {
        delay(1000);
    }
}

} // namespace vitte::runtime

static VitteUnit unit_value() {
    VitteUnit u{};
    return u;
}

extern "C" {

VitteUnit arduino_gpio_pin_mode(std::uint8_t pin, VitteArduinoPinMode mode) {
    uint8_t m = INPUT;
    if (mode == VitteArduinoPinMode::Output) {
        m = OUTPUT;
    } else if (mode == VitteArduinoPinMode::InputPullup) {
        m = INPUT_PULLUP;
    }
    pinMode(pin, m);
    return unit_value();
}

VitteUnit arduino_gpio_digital_write(std::uint8_t pin, VitteArduinoPinState state) {
    digitalWrite(pin, state == VitteArduinoPinState::High ? HIGH : LOW);
    return unit_value();
}

VitteArduinoPinState arduino_gpio_digital_read(std::uint8_t pin) {
    return digitalRead(pin) == HIGH ? VitteArduinoPinState::High : VitteArduinoPinState::Low;
}

VitteUnit arduino_gpio_analog_write(std::uint8_t pin, std::uint16_t value) {
    analogWrite(pin, value);
    return unit_value();
}

std::uint16_t arduino_gpio_analog_read(std::uint8_t pin) {
    return static_cast<std::uint16_t>(analogRead(pin));
}

VitteUnit arduino_serial_begin(std::uint32_t baud) {
    Serial.begin(baud);
    return unit_value();
}

std::uint32_t arduino_serial_available() {
    return static_cast<std::uint32_t>(Serial.available());
}

std::int32_t arduino_serial_read() {
    return static_cast<std::int32_t>(Serial.read());
}

std::size_t arduino_serial_write(std::uint8_t byte) {
    return Serial.write(byte);
}

VitteUnit arduino_serial_flush() {
    Serial.flush();
    return unit_value();
}

VitteUnit arduino_i2c_begin() {
    Wire.begin();
    return unit_value();
}

VitteUnit arduino_i2c_begin_transmission(std::uint8_t addr) {
    Wire.beginTransmission(addr);
    return unit_value();
}

std::size_t arduino_i2c_write(std::uint8_t byte) {
    return Wire.write(byte);
}

std::uint8_t arduino_i2c_end_transmission() {
    return Wire.endTransmission();
}

std::uint8_t arduino_i2c_request_from(std::uint8_t addr, std::uint8_t len) {
    return Wire.requestFrom(addr, len);
}

std::int32_t arduino_i2c_read() {
    return static_cast<std::int32_t>(Wire.read());
}

VitteUnit arduino_spi_begin() {
    SPI.begin();
    return unit_value();
}

std::uint8_t arduino_spi_transfer(std::uint8_t byte) {
    return SPI.transfer(byte);
}

VitteUnit arduino_spi_end() {
    SPI.end();
    return unit_value();
}

VitteUnit arduino_delay_ms(std::uint32_t ms) {
    delay(ms);
    return unit_value();
}

VitteUnit arduino_delay_us(std::uint32_t us) {
    delayMicroseconds(us);
    return unit_value();
}

std::uint32_t arduino_millis() {
    return millis();
}

std::uint32_t arduino_micros() {
    return micros();
}

} // extern "C"
