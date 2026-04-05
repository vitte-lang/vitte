#pragma once

#include "options.hpp"

#include "../frontend/diagnostics.hpp"

#include <string>

namespace vitte::driver {

bool apply_strict_core_guard(const Options& opts,
                             const std::string& source,
                             const std::string& path,
                             frontend::diag::DiagnosticEngine& diagnostics);

} // namespace vitte::driver

