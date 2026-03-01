#pragma once

namespace vitte::driver {

enum class CrashStage {
    Unknown = 0,
    Parse,
    Resolve,
    Ir,
    Backend,
};

void set_crash_stage(CrashStage stage);
CrashStage get_crash_stage();
const char* crash_stage_name(CrashStage stage);

/* -------------------------------------------------
 * Compiler driver entry point
 * -------------------------------------------------
 * This is called by the program entry (main.cpp).
 */
int run(int argc, char** argv);

} // namespace vitte::driver
