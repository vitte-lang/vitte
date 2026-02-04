// ============================================================
// driver.hpp — Vitte Compiler
// Linker driver interface
// ============================================================

#pragma once

#include <string>
#include <vector>

namespace vitte::linker {

// ------------------------------------------------------------
// LinkerDriver
// ------------------------------------------------------------
//
// Rôle :
//  - orchestrer les étapes du linker
//  - gérer les entrées (objets / archives)
//  - préparer la résolution de symboles
//  - déléguer le parsing binaire à d'autres modules
//

class LinkerDriver {
public:
    LinkerDriver();

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
    // --------------------------------------------------------
    // Helpers
    // --------------------------------------------------------

    bool is_archive(const std::string& path) const;

    bool load_archive(const std::string& path,
                      std::string& error);

    bool load_object(const std::string& path,
                     std::string& error);

private:
    bool verbose_;
    std::string output_;
    std::vector<std::string> inputs_;

    // chargés durant run()
    std::vector<class Archive> archives_;
    std::vector<std::string> objects_;
};

} // namespace vitte::linker