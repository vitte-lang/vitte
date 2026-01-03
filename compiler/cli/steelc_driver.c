// SPDX-License-Identifier: MIT
// steelc_driver.c
//
// Top-level CLI driver for the Steel toolchain.
//
// This file provides a single entrypoint that dispatches to subcommands.
// The subcommands are implemented in separate translation units (e.g.
// steel_cmd_fmt.c, steel_cmd_test.c, etc.).
//
// Contract:
//  - `steel <cmd> ...` dispatches with argv[0] == <cmd> for the callee.
//  - Unknown/invalid usage returns exit code 2.
//  - Subcommands return their own exit code.

#include <stdio.h>
#include <string.h>

//------------------------------------------------------------------------------
// Subcommand entry points (implemented elsewhere)
//------------------------------------------------------------------------------

int steel_cmd_clean_main(int argc, char** argv);
int steel_cmd_doc_main(int argc, char** argv);
int steel_cmd_fmt_main(int argc, char** argv);
int steel_cmd_pkg_main(int argc, char** argv);
int steel_cmd_run_main(int argc, char** argv);
int steel_cmd_test_main(int argc, char** argv);

//------------------------------------------------------------------------------
// Helpers
//------------------------------------------------------------------------------

static void steel_driver_print_help(FILE* out)
{
    fprintf(out,
        "Usage: steel <command> [options] [-- <args...>]\n"
        "\n"
        "Commands:\n"
        "  clean     Remove generated artifacts (build/cache/dist/tmp)\n"
        "  doc       Generate a lightweight documentation index\n"
        "  fmt       Normalize whitespace formatting (hygiene formatter)\n"
        "  pkg       Create a package directory with a manifest and copied files\n"
        "  run       Run an external command (process launcher)\n"
        "  test      Discover and run tests (wrapper runner)\n"
        "  help      Show help (optionally for a subcommand)\n"
        "  version   Print version\n"
        "\n"
        "Global options:\n"
        "  -h, --help        Show this help\n"
        "      --version     Print version\n"
        "\n"
        "Examples:\n"
        "  steel fmt --check\n"
        "  steel test --fail-fast\n"
        "  steel run --cmd vittec -- --help\n"
        "  steel pkg --out dist/pkg --prefix vitte\n");
}

static int steel_driver_print_version(FILE* out)
{
#if defined(STEEL_VERSION_STR)
    fprintf(out, "%s\n", STEEL_VERSION_STR);
#elif defined(VITTE_VERSION_STR)
    fprintf(out, "%s\n", VITTE_VERSION_STR);
#elif defined(STEEL_VERSION)
    fprintf(out, "%s\n", STEEL_VERSION);
#elif defined(VITTE_VERSION)
    fprintf(out, "%s\n", VITTE_VERSION);
#else
    fprintf(out, "steel (version unknown)\n");
#endif
    return 0;
}

static int steel_driver_dispatch(const char* cmd, int argc, char** argv)
{
    // Support a few common aliases.
    if (strcmp(cmd, "format") == 0) cmd = "fmt";
    if (strcmp(cmd, "package") == 0) cmd = "pkg";

    if (strcmp(cmd, "clean") == 0) return steel_cmd_clean_main(argc, argv);
    if (strcmp(cmd, "doc") == 0)   return steel_cmd_doc_main(argc, argv);
    if (strcmp(cmd, "fmt") == 0)   return steel_cmd_fmt_main(argc, argv);
    if (strcmp(cmd, "pkg") == 0)   return steel_cmd_pkg_main(argc, argv);
    if (strcmp(cmd, "run") == 0)   return steel_cmd_run_main(argc, argv);
    if (strcmp(cmd, "test") == 0)  return steel_cmd_test_main(argc, argv);

    return 2;
}

static int steel_driver_help_for(const char* subcmd)
{
    // Call `<subcmd> --help` through the dispatcher.
    char* av[2];
    av[0] = (char*)subcmd;
    av[1] = (char*)"--help";
    int rc = steel_driver_dispatch(subcmd, 2, av);
    if (rc == 2)
    {
        // Unknown command.
        fprintf(stderr, "steel: unknown command '%s'\n", subcmd);
        steel_driver_print_help(stderr);
        return 2;
    }
    return 0;
}

//------------------------------------------------------------------------------
// Entry point
//------------------------------------------------------------------------------

int steelc_main(int argc, char** argv)
{
    if (argc <= 1)
    {
        steel_driver_print_help(stdout);
        return 2;
    }

    const char* a1 = argv[1];

    if (strcmp(a1, "-h") == 0 || strcmp(a1, "--help") == 0)
    {
        steel_driver_print_help(stdout);
        return 0;
    }

    if (strcmp(a1, "--version") == 0)
    {
        return steel_driver_print_version(stdout);
    }

    if (strcmp(a1, "version") == 0)
    {
        return steel_driver_print_version(stdout);
    }

    if (strcmp(a1, "help") == 0)
    {
        if (argc >= 3) return steel_driver_help_for(argv[2]);
        steel_driver_print_help(stdout);
        return 0;
    }

    // Dispatch: pass argv[1] as argv[0] for the subcommand.
    int rc = steel_driver_dispatch(a1, argc - 1, argv + 1);
    if (rc == 2)
    {
        fprintf(stderr, "steel: unknown command '%s'\n", a1);
        steel_driver_print_help(stderr);
        return 2;
    }

    return rc;
}

// Some build setups want a conventional `main` symbol.
// If another TU already provides `main`, compile this with STEELC_NO_MAIN.
#ifndef STEELC_NO_MAIN
int main(int argc, char** argv)
{
    return steelc_main(argc, argv);
}
#endif
