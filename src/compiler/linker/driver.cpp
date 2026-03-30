// ============================================================
// driver.cpp — Vitte Compiler
// Linker driver (orchestration layer)
// ============================================================

#include "driver.hpp"

#include "archive.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace vitte::linker {

// ------------------------------------------------------------
// Driver
// ------------------------------------------------------------

LinkerDriver::LinkerDriver()
    : verbose_(false) {}

// ------------------------------------------------------------
// Configuration
// ------------------------------------------------------------

void LinkerDriver::set_verbose(bool v) {
    verbose_ = v;
}

void LinkerDriver::add_input(const std::string& path) {
    inputs_.push_back(path);
}

void LinkerDriver::set_output(const std::string& path) {
    output_ = path;
}

// ------------------------------------------------------------
// Run
// ------------------------------------------------------------

bool LinkerDriver::run(std::string& error) {
    if (output_.empty()) {
        error = "no output file specified";
        return false;
    }

    if (inputs_.empty()) {
        error = "no input files specified";
        return false;
    }

    if (verbose_) {
        std::cerr << "[linker] output: " << output_ << "\n";
        for (const auto& in : inputs_) {
            std::cerr << "[linker] input: " << in << "\n";
        }
    }

    // --------------------------------------------------------
    // Phase 1: load archives / object files
    // --------------------------------------------------------

    for (const auto& path : inputs_) {
        if (is_archive(path)) {
            if (!load_archive(path, error)) {
                return false;
            }
        } else {
            if (!load_object(path, error)) {
                return false;
            }
        }
    }

    // --------------------------------------------------------
    // Phase 2: symbol resolution (future)
    // --------------------------------------------------------
    //
    //  - build global symbol table
    //  - resolve undefined symbols
    //  - select archive members
    //

    if (verbose_) {
        std::cerr
            << "[linker][LNK-W-UNIMPL-SYMBOL-RESOLUTION] "
            << "symbol resolution phase is not implemented yet; "
            << "the current linker run validates/archive-loads inputs only\n";
        std::cerr
            << "[linker][action] if your build depends on symbol resolution, "
            << "use the default compiler driver/linker path for now\n";
    }

    // --------------------------------------------------------
    // Phase 3: output generation (future)
    // --------------------------------------------------------
    //
    //  - write executable / shared object
    //

    if (verbose_) {
        std::cerr
            << "[linker][LNK-W-UNIMPL-OUTPUT-GENERATION] "
            << "output generation phase is not implemented yet; "
            << "no executable/shared object is emitted by this path\n";
        std::cerr
            << "[linker][action] use this linker path for orchestration and "
            << "input validation only until emit support lands\n";
    }

    return true;
}

// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------

bool LinkerDriver::is_archive(const std::string& path) const {
    return path.size() > 2 &&
           path.substr(path.size() - 2) == ".a";
}

bool LinkerDriver::load_archive(
    const std::string& path,
    std::string& error)
{
    if (verbose_) {
        std::cerr << "[linker] loading archive: " << path << "\n";
    }

    Archive archive;
    if (!archive.load_from_file(path, error)) {
        return false;
    }

    archives_.push_back(std::move(archive));
    return true;
}

bool LinkerDriver::load_object(
    const std::string& path,
    std::string& error)
{
    (void)error;
    if (verbose_) {
        std::cerr << "[linker] loading object: " << path << "\n";
    }

    // Placeholder : parsing objet (ELF/Mach-O/COFF)
    // sera implémenté dans object_file.cpp

    objects_.push_back(path);
    return true;
}

} // namespace vitte::linker
