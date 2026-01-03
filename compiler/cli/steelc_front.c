// SPDX-License-Identifier: MIT
// steelc_front.c
//
// Frontend wiring for the Steel CLI.
//
// This translation unit exists to keep the public/host entrypoints stable across
// different build layouts. Some builds embed the Steel driver into other tools,
// or rename the primary binary.
//
// Responsibilities:
//  - Provide `steel_main` as a stable entrypoint name.
//  - Optionally provide `main` if the build does not already define it.
//  - Forward to `steelc_main` from steelc_driver.c.
//
// Build knobs:
//  - Define STEELC_NO_MAIN to prevent defining `main` here.
//  - Define STEEL_FRONT_MAIN_NAME to rename the public entrypoint.
//

#include <stdio.h>

// Implemented by steelc_driver.c
int steelc_main(int argc, char** argv);

#ifndef STEEL_FRONT_MAIN_NAME
  #define STEEL_FRONT_MAIN_NAME steel_main
#endif

// Stable public entrypoint.
int STEEL_FRONT_MAIN_NAME(int argc, char** argv)
{
    return steelc_main(argc, argv);
}

#ifndef STEELC_NO_MAIN
// If a conventional main() is needed (standalone binary), provide it.
int main(int argc, char** argv)
{
    return STEEL_FRONT_MAIN_NAME(argc, argv);
}
#endif

