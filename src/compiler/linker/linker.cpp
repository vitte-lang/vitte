// ============================================================
// linker.cpp — Vitte Compiler
// High-level linker entry point
// ============================================================

#include "linker.hpp"

#include "driver.hpp"

#include <string>

namespace vitte::linker {

// ------------------------------------------------------------
// Linker
// ------------------------------------------------------------

Linker::Linker()
    : verbose_(false) {}

// ------------------------------------------------------------
// Configuration
// ------------------------------------------------------------

void Linker::set_verbose(bool verbose) {
    verbose_ = verbose;
}

void Linker::add_input(const std::string& path) {
    inputs_.push_back(path);
}

void Linker::set_output(const std::string& path) {
    output_ = path;
}

// ------------------------------------------------------------
// Run
// ------------------------------------------------------------

bool Linker::run(std::string& error) {
    LinkerDriver driver;
    driver.set_verbose(verbose_);
    driver.set_output(output_);

    for (const auto& in : inputs_) {
        driver.add_input(in);
    }

    return driver.run(error);
}

} // namespace vitte::linker