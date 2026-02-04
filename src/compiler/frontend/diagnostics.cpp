#include "diagnostics.hpp"

#include <iostream>

namespace vitte::frontend {

/* -------------------------------------------------
 * Error reporting
 * ------------------------------------------------- */
void error(const std::string& msg) {
    std::cerr << "[vitte:frontend:error] " << msg << "\n";
}

/* -------------------------------------------------
 * Warning reporting
 * ------------------------------------------------- */
void warning(const std::string& msg) {
    std::cerr << "[vitte:frontend:warning] " << msg << "\n";
}

/* -------------------------------------------------
 * Note / info
 * ------------------------------------------------- */
void note(const std::string& msg) {
    std::cerr << "[vitte:frontend:note] " << msg << "\n";
}

} // namespace vitte::frontend
