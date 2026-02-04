#pragma once

#include "options.hpp"

namespace vitte::driver {

/* -------------------------------------------------
 * Run the full compilation pipeline
 * -------------------------------------------------
 * Frontend → IR → Backend → Native
 */
bool run_pipeline(const Options& opts);

} // namespace vitte::driver
