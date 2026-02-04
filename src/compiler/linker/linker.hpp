// ============================================================
// linker.hpp — Vitte Compiler
// High-level linker interface
// ============================================================

#pragma once

#include <string>
#include <vector>

namespace vitte::linker {

// ------------------------------------------------------------
// Linker
// ------------------------------------------------------------
//
// Rôle :
//  - façade haut niveau du linker Vitte
//  - API interne utilisée par vittec / CLI / tests
//  - délègue l'orchestration réelle à LinkerDriver
//
// Ce type ne connaît :
//  - ni les formats binaires
//  - ni les symboles
//  - ni les archives
//

class Linker {
public:
    Linker();

    // --------------------------------------------------------
    // Configuration
    // --------------------------------------------------------

    void set_verbose(bool verbose);

    void add_input(const std::string& path);
    void set_output(const std::string& path);

    // --------------------------------------------------------
    // Execution
    // --------------------------------------------------------

    bool run(std::string& error);

private:
    bool verbose_;
    std::string output_;
    std::vector<std::string> inputs_;
};

} // namespace vitte::linker