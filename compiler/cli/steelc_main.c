

// SPDX-License-Identifier: MIT
// steelc_main.c
//
// Standalone `steel` executable entrypoint.
//
// This translation unit is the only one that should define the conventional
// `main()` symbol by default, to avoid duplicate symbol errors.

#include <stdio.h>

// Implemented by steelc_driver.c
int steelc_main(int argc, char** argv);

// Stable alias entrypoint for embedders.
int steel_main(int argc, char** argv)
{
    return steelc_main(argc, argv);
}

int main(int argc, char** argv)
{
    return steel_main(argc, argv);
}
