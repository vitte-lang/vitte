// fuzz_dict.c
// C TU backing for fuzz_dict.h (C17).
//
// The current fuzz_dict.h is header-only (static inline). This file exists to:
//  - satisfy build systems that expect a .c per module,
//  - provide a future place to move non-inline implementations.
//
// If you later refactor fuzz_dict.h to declare non-static functions,
// implement them here and remove the “static inline” versions from the header.

#include "fuzz/fuzz_dict.h"

/* intentionally empty for now */
