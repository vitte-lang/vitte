#pragma once
#include <string>
#include <vector>

namespace vitte::backend::toolchain {

struct ArduinoOptions {
    std::string cli_path = "arduino-cli";
    std::string fqbn = "arduino:avr:uno";
    std::string work_dir = ".";
    std::string sketch_name = "vitte_sketch";
    bool verbose = false;
    bool upload = false;
    std::string port;
};

bool invoke_arduino_cli(
    const std::string& input_cpp,
    const std::vector<std::string>& extra_sources,
    const std::string& runtime_header,
    const std::string& output_hex,
    const ArduinoOptions& opts
);

} // namespace vitte::backend::toolchain
