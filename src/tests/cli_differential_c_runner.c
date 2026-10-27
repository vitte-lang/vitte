#include "cli/cli.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

typedef struct cli_case {
    const char *name;
    int argc;
    const char *const *argv;
} cli_case_t;

static const char *const valid_options[] = {
    "vitte", "build", "--backend=c17", "--opt=debug", "--output",
    "out.c", "src/main.vit"
};
static const char *const valid_switches[] = {
    "vitte", "check", "--deterministic", "--no-runtime-checks",
    "src/main.vit"
};
static const char *const duplicate_option[] = {
    "vitte", "build", "--output=a.c", "--output", "b.c", "src/main.vit"
};
static const char *const conflicting_options[] = {
    "vitte", "build", "--verbose", "--quiet", "src/main.vit"
};
static const char *const forwarded_args[] = {
    "vitte", "run", "src/main.vit", "--", "--verbose", "arg with spaces"
};
static const char *const input_limit[] = {
    "vitte", "build", "a.vit", "b.vit", "c.vit"
};
static const char *const forward_limit[] = {
    "vitte", "run", "main.vit", "--", "one", "two", "three"
};
static const char *const missing_value[] = {
    "vitte", "build", "--backend"
};
static const char *const invalid_value[] = {
    "vitte", "build", "--backend=invalid", "src/main.vit"
};
static const char *const unknown_option[] = {
    "vitte", "build", "--not-a-real-option", "src/main.vit"
};

#define CLI_CASE(name_, argv_) \
    { name_, (int)(sizeof(argv_) / sizeof((argv_)[0])), argv_ }

static const cli_case_t cases[] = {
    CLI_CASE("valid-options", valid_options),
    CLI_CASE("valid-switches", valid_switches),
    CLI_CASE("duplicate-option", duplicate_option),
    CLI_CASE("conflicting-options", conflicting_options),
    CLI_CASE("forwarded-args", forwarded_args),
    CLI_CASE("input-limit", input_limit),
    CLI_CASE("forward-limit", forward_limit),
    CLI_CASE("missing-value", missing_value),
    CLI_CASE("invalid-value", invalid_value),
    CLI_CASE("unknown-option", unknown_option)
};

static void
write_hex_u64(uint64_t value)
{
    static const char digits[] = "0123456789abcdef";
    unsigned int shift;

    for (shift = 60u;; shift -= 4u) {
        (void)putchar(digits[(value >> shift) & UINT64_C(0xf)]);
        if (shift == 0u) {
            break;
        }
    }
}

static int
run_case(const cli_case_t *test_case)
{
    vitte_cli_t cli;
    const vitte_cli_config_t *config;
    const vitte_cli_message_t *message;
    vitte_cli_stats_t stats;
    uint64_t fields[18];
    size_t index;
    bool parsed;

    if (!vitte_cli_init(&cli)) {
        return 0;
    }
    if (!vitte_cli_set_limits(&cli, 2u, 2u)) {
        vitte_cli_destroy(&cli);
        return 0;
    }
    parsed = vitte_cli_parse(&cli, test_case->argc, test_case->argv);
    config = vitte_cli_config(&cli);
    message = vitte_cli_message(&cli);
    stats = vitte_cli_stats(&cli);
    if (config == NULL || message == NULL) {
        vitte_cli_destroy(&cli);
        return 0;
    }

    fields[0] = parsed ? UINT64_C(1) : UINT64_C(0);
    fields[1] = (uint64_t)vitte_cli_state(&cli);
    fields[2] = (uint64_t)vitte_cli_last_error(&cli);
    fields[3] = (uint64_t)config->command;
    fields[4] = (uint64_t)config->backend;
    fields[5] = (uint64_t)config->optimization;
    fields[6] = (uint64_t)config->diagnostic_format;
    fields[7] = (uint64_t)config->color;
    fields[8] = vitte_cli_seen_options(&cli);
    fields[9] = (uint64_t)stats.argument_count;
    fields[10] = (uint64_t)stats.parsed_argument_count;
    fields[11] = (uint64_t)stats.option_count;
    fields[12] = (uint64_t)stats.positional_count;
    fields[13] = (uint64_t)stats.consumed_value_count;
    fields[14] = (uint64_t)stats.input_count;
    fields[15] = (uint64_t)stats.forward_argument_count;
    fields[16] = vitte_cli_fingerprint(&cli);
    fields[17] = (uint64_t)(uint32_t)message->argument_index;

    (void)printf("%s|", test_case->name);
    for (index = 0u; index < sizeof(fields) / sizeof(fields[0]); ++index) {
        if (index != 0u) {
            (void)putchar(';');
        }
        write_hex_u64(fields[index]);
    }
    (void)putchar('\n');
    vitte_cli_destroy(&cli);
    return 1;
}

int
main(void)
{
    size_t index;

    for (index = 0u; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        if (!run_case(&cases[index])) {
            (void)fprintf(stderr, "C CLI probe failed: %s\n", cases[index].name);
            return 2;
        }
    }
    return 0;
}
