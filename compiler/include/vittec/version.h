#ifndef VITTEC_INCLUDE_VITTEC_VERSION_H
#define VITTEC_INCLUDE_VITTEC_VERSION_H

/*
  version.h — vittec versioning (max)

  Goals:
  - Single place for semantic version macros.
  - Lightweight header (no heavy includes).
  - Override-friendly (#ifndef around build-time injected macros).
  - Small helpers for comparing versions and printing.
*/

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VITTEC_VERSION_API_VERSION 1u

/* -------------------------------------------------------------------------
 * Semantic version (override-friendly)
 * ------------------------------------------------------------------------- */

#ifndef VITTEC_VERSION_MAJOR
  #define VITTEC_VERSION_MAJOR 0
#endif
#ifndef VITTEC_VERSION_MINOR
  #define VITTEC_VERSION_MINOR 1
#endif
#ifndef VITTEC_VERSION_PATCH
  #define VITTEC_VERSION_PATCH 0
#endif

/* Optional pre-release / metadata (empty by default). */
#ifndef VITTEC_VERSION_PRERELEASE
  #define VITTEC_VERSION_PRERELEASE ""
#endif
#ifndef VITTEC_VERSION_METADATA
  #define VITTEC_VERSION_METADATA ""
#endif

/* Encoded version for fast comparisons: 0xMMmmpp (8 bits each). */
#define VITTEC_VERSION_HEX ((uint32_t)(((VITTEC_VERSION_MAJOR & 0xFFu) << 16) | \
                                       ((VITTEC_VERSION_MINOR & 0xFFu) <<  8) | \
                                       ((VITTEC_VERSION_PATCH & 0xFFu) <<  0)))

/* Integer version (major*10000 + minor*100 + patch). */
#define VITTEC_VERSION_INT ((int)((VITTEC_VERSION_MAJOR * 10000) + (VITTEC_VERSION_MINOR * 100) + VITTEC_VERSION_PATCH))

/* -------------------------------------------------------------------------
 * Build information (optional; can be injected by build system)
 * ------------------------------------------------------------------------- */

#ifndef VITTEC_BUILD_GIT_SHA
  #define VITTEC_BUILD_GIT_SHA ""
#endif
#ifndef VITTEC_BUILD_GIT_DIRTY
  #define VITTEC_BUILD_GIT_DIRTY 0
#endif

#ifndef VITTEC_BUILD_DATE
  /* Use compiler predefined macros if not overridden. */
  #define VITTEC_BUILD_DATE __DATE__
#endif
#ifndef VITTEC_BUILD_TIME
  #define VITTEC_BUILD_TIME __TIME__
#endif

#ifndef VITTEC_BUILD_COMPILER
  #if defined(__clang__)
    #define VITTEC_BUILD_COMPILER "clang"
  #elif defined(__GNUC__)
    #define VITTEC_BUILD_COMPILER "gcc"
  #elif defined(_MSC_VER)
    #define VITTEC_BUILD_COMPILER "msvc"
  #else
    #define VITTEC_BUILD_COMPILER "unknown"
  #endif
#endif

/* -------------------------------------------------------------------------
 * API
 * ------------------------------------------------------------------------- */

/*
  Returns a stable semantic version string.

  Examples:
    - "0.1.0"
    - "0.1.0-alpha"
    - "0.1.0+meta"
    - "0.1.0-alpha+meta"

  The exact formatting is owned by the implementation; callers should treat the
  returned pointer as read-only and valid for the lifetime of the program.
*/
const char* vittec_version_string(void);

/* Build info helpers (may return empty strings if not available). */
const char* vittec_build_git_sha(void);
int         vittec_build_git_dirty(void);
const char* vittec_build_date(void);
const char* vittec_build_time(void);
const char* vittec_build_compiler(void);

/* Numeric helpers. */
static inline uint32_t vittec_version_hex(void) { return (uint32_t)VITTEC_VERSION_HEX; }
static inline int      vittec_version_int(void) { return (int)VITTEC_VERSION_INT; }

/* Compare against a minimum required version. */
static inline int vittec_version_at_least(int maj, int min, int pat) {
  const int a = (int)VITTEC_VERSION_INT;
  const int b = (maj * 10000) + (min * 100) + pat;
  return a >= b;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTEC_INCLUDE_VITTEC_VERSION_H */
