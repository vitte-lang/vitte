/*
 * Vitte Compiler
 * src/main.c
 *
 * Minimal process entry point. CLI parsing, configuration, diagnostics,
 * pipeline construction and execution live in the cli modules; this file
 * only forwards argc/argv across the process boundary.
 */

#include "cli/entry.h"

int
main(
    int argc,
    char **argv)
{
    return vitte_main_entry(argc, argv);
}
