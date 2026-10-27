#include "config/config.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t
next_random(uint32_t *state)
{
    *state ^= *state << 13;
    *state ^= *state >> 17;
    *state ^= *state << 5;
    return *state;
}

static size_t
iteration_limit(size_t fallback)
{
    const char *text = getenv("VITTE_FUZZ_ITERATIONS");
    size_t value = 0u;

    if (text == NULL || text[0] == '\0') {
        return fallback;
    }
    while (*text >= '0' && *text <= '9') {
        size_t digit = (size_t)(*text - '0');
        if (value > (SIZE_MAX - digit) / 10u) {
            return fallback;
        }
        value = value * 10u + digit;
        ++text;
    }
    return *text == '\0' ? value : fallback;
}

int
main(void)
{
    static const char *const keys[] = {
        "jobs", "max-errors", "max-output-bytes", "profile", "backend",
        "optimization", "diagnostic-format", "color", "verbose", "quiet",
        "warnings-as-errors", "emit-debug-info", "emit-source-map",
        "runtime-checks", "deterministic", "incremental", "offline",
        "target", "output"
    };
    static const char *const values[] = {
        "0", "1", "2", "true", "false", "debug", "release", "c17",
        "none", "balanced", "json", "sarif", "lsp", "auto", "never",
        "x", "", "18446744073709551615", "-1", "garbage"
    };
    uint32_t state = UINT32_C(0xC0A57F1A);
    size_t iterations = iteration_limit(5000u);
    size_t iteration;

    for (iteration = 0u; iteration < iterations; ++iteration) {
        vitte_config_t config;
        const char *arguments[4];
        char storage[3][160];
        size_t argument_count = 1u +
            (size_t)(next_random(&state) % UINT32_C(3));
        size_t argument;

        assert(vitte_config_init(&config));
        assert(vitte_config_apply_defaults(&config));
        for (argument = 0u; argument < argument_count; ++argument) {
            size_t key_index = (size_t)(next_random(&state) %
                (uint32_t)(sizeof(keys) / sizeof(keys[0])));
            size_t value_index = (size_t)(next_random(&state) %
                (uint32_t)(sizeof(values) / sizeof(values[0])));
            int written = snprintf(
                storage[argument],
                sizeof(storage[argument]),
                "--%s=%s",
                keys[key_index],
                values[value_index]);
            assert(written > 0);
            assert((size_t)written < sizeof(storage[argument]));
            arguments[argument] = storage[argument];
        }
        arguments[argument_count] = NULL;

        /* Invalid assignments are expected input for this fuzz target. */
        if (vitte_config_apply_cli(
                &config,
                (int)argument_count,
                arguments)) {
            (void)vitte_config_validate(&config);
        }
        vitte_config_destroy(&config);
    }

    return 0;
}
