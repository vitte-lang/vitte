/* Compatibility bridge for the Vitte-native front-end entry points. */
#include "config/config.h"

#include <stddef.h>
#include <stdint.h>

/* The generated Vitte records retain their C ABI layout at this boundary. */
struct Config;
extern uint32_t vitte_native_frontend_run(
    const char *source,
    size_t source_length,
    uint64_t file_id);
extern uint32_t vitte_native_frontend_run_with_config(
    const char *source,
    size_t source_length,
    uint64_t file_id,
    struct Config *config);

uint32_t
compiler_driver_pipeline_vitte_native_frontend_run(
    const char *source,
    size_t source_length,
    uint64_t file_id)
{
    return vitte_native_frontend_run(source, source_length, file_id);
}

uint32_t
compiler_driver_pipeline_vitte_native_frontend_run_with_config(
    const char *source,
    size_t source_length,
    uint64_t file_id,
    const vitte_config_t *config)
{
    return vitte_native_frontend_run_with_config(
        source,
        source_length,
        file_id,
        (struct Config *)config);
}
