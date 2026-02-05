#pragma once

#include "options.hpp"

namespace vitte::driver {

struct PassResult {
    bool ok = false;
};

PassResult run_passes(const Options& opts);

} // namespace vitte::driver
