/*
 * CloneVitte
 * Public Version API
 *
 * Copyright (C) 2026 VitteFoundation
 * SPDX-License-Identifier: LicenseRef-VFPL-1.0
 */

#ifndef CLONEVITTE_VERSION_H
#define CLONEVITTE_VERSION_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * CloneVitte version
 * ============================================================ */

#define CLONEVITTE_VERSION_MAJOR 0
#define CLONEVITTE_VERSION_MINOR 1
#define CLONEVITTE_VERSION_PATCH 0

#define CLONEVITTE_VERSION_STRING "0.1.0"

/* ============================================================
 * Version encoding
 * ============================================================ */

#define CLONEVITTE_VERSION_ENCODE(major, minor, patch) \
    ((((uint32_t)(major) & 0x3FFu) << 22u) |           \
     (((uint32_t)(minor) & 0x3FFu) << 12u) |           \
     ((uint32_t)(patch) & 0xFFFu))

#define CLONEVITTE_VERSION_DECODE_MAJOR(version) \
    (((uint32_t)(version) >> 22u) & 0x3FFu)

#define CLONEVITTE_VERSION_DECODE_MINOR(version) \
    (((uint32_t)(version) >> 12u) & 0x3FFu)

#define CLONEVITTE_VERSION_DECODE_PATCH(version) \
    ((uint32_t)(version) & 0xFFFu)

#define CLONEVITTE_VERSION \
    CLONEVITTE_VERSION_ENCODE( \
        CLONEVITTE_VERSION_MAJOR, \
        CLONEVITTE_VERSION_MINOR, \
        CLONEVITTE_VERSION_PATCH)

/* ============================================================
 * Language version
 * ============================================================ */

#define CLONEVITTE_LANGUAGE_VERSION_MAJOR 1
#define CLONEVITTE_LANGUAGE_VERSION_MINOR 0
#define CLONEVITTE_LANGUAGE_VERSION_PATCH 0

#define CLONEVITTE_LANGUAGE_VERSION_STRING "1.0.0"

#define CLONEVITTE_LANGUAGE_VERSION \
    CLONEVITTE_VERSION_ENCODE( \
        CLONEVITTE_LANGUAGE_VERSION_MAJOR, \
        CLONEVITTE_LANGUAGE_VERSION_MINOR, \
        CLONEVITTE_LANGUAGE_VERSION_PATCH)

/* ============================================================
 * ABI version
 * ============================================================ */

#define CLONEVITTE_ABI_VERSION_MAJOR 1
#define CLONEVITTE_ABI_VERSION_MINOR 0
#define CLONEVITTE_ABI_VERSION_PATCH 0

#define CLONEVITTE_ABI_VERSION_STRING "1.0.0"

#define CLONEVITTE_ABI_VERSION \
    CLONEVITTE_VERSION_ENCODE( \
        CLONEVITTE_ABI_VERSION_MAJOR, \
        CLONEVITTE_ABI_VERSION_MINOR, \
        CLONEVITTE_ABI_VERSION_PATCH)

/* ============================================================
 * Public API version
 * ============================================================ */

#define CLONEVITTE_API_VERSION_MAJOR 1
#define CLONEVITTE_API_VERSION_MINOR 0
#define CLONEVITTE_API_VERSION_PATCH 0

#define CLONEVITTE_API_VERSION_STRING "1.0.0"

#define CLONEVITTE_API_VERSION \
    CLONEVITTE_VERSION_ENCODE( \
        CLONEVITTE_API_VERSION_MAJOR, \
        CLONEVITTE_API_VERSION_MINOR, \
        CLONEVITTE_API_VERSION_PATCH)

/* ============================================================
 * Version structure
 * ============================================================ */

typedef struct CloneVitteVersion {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
} CloneVitteVersion;

/* ============================================================
 * Build type
 * ============================================================ */

typedef enum CloneVitteBuildType {
    CLONEVITTE_BUILD_UNKNOWN = 0,
    CLONEVITTE_BUILD_DEBUG,
    CLONEVITTE_BUILD_RELEASE
} CloneVitteBuildType;

/* ============================================================
 * Compiler information
 * ============================================================ */

typedef struct CloneVitteBuildInfo {
    CloneVitteVersion compiler_version;
    CloneVitteVersion language_version;
    CloneVitteVersion abi_version;
    CloneVitteVersion api_version;

    CloneVitteBuildType build_type;

    const char *compiler;
    const char *compiler_version_string;

    const char *host;
    const char *target;

    const char *build_date;
    const char *build_time;

    const char *git_commit;
    const char *git_branch;

    bool git_dirty;
} CloneVitteBuildInfo;

/* ============================================================
 * Compiler version
 * ============================================================ */

uint32_t
clonevitte_version(void);

CloneVitteVersion
clonevitte_version_components(void);

const char *
clonevitte_version_string(void);

/* ============================================================
 * Language version
 * ============================================================ */

uint32_t
clonevitte_language_version(void);

CloneVitteVersion
clonevitte_language_version_components(void);

const char *
clonevitte_language_version_string(void);

/* ============================================================
 * ABI version
 * ============================================================ */

uint32_t
clonevitte_abi_version(void);

CloneVitteVersion
clonevitte_abi_version_components(void);

const char *
clonevitte_abi_version_string(void);

/* ============================================================
 * API version
 * ============================================================ */

uint32_t
clonevitte_api_version(void);

CloneVitteVersion
clonevitte_api_version_components(void);

const char *
clonevitte_api_version_string(void);

/* ============================================================
 * Build information
 * ============================================================ */

const CloneVitteBuildInfo *
clonevitte_build_info(void);

CloneVitteBuildType
clonevitte_build_type(void);

const char *
clonevitte_build_type_name(void);

const char *
clonevitte_build_compiler(void);

const char *
clonevitte_build_host(void);

const char *
clonevitte_build_target(void);

const char *
clonevitte_build_date(void);

const char *
clonevitte_build_time(void);

const char *
clonevitte_git_commit(void);

const char *
clonevitte_git_branch(void);

bool
clonevitte_git_dirty(void);

/* ============================================================
 * Version comparison
 * ============================================================ */

int
clonevitte_version_compare(
    CloneVitteVersion left,
    CloneVitteVersion right
);

bool
clonevitte_version_equal(
    CloneVitteVersion left,
    CloneVitteVersion right
);

bool
clonevitte_version_at_least(
    CloneVitteVersion version,
    uint32_t major,
    uint32_t minor,
    uint32_t patch
);

/* ============================================================
 * Compatibility
 * ============================================================ */

bool
clonevitte_language_version_supported(
    uint32_t major,
    uint32_t minor,
    uint32_t patch
);

bool
clonevitte_abi_version_compatible(
    uint32_t major,
    uint32_t minor,
    uint32_t patch
);

bool
clonevitte_api_version_compatible(
    uint32_t major,
    uint32_t minor,
    uint32_t patch
);

/* ============================================================
 * Convenience macros
 * ============================================================ */

#define CLONEVITTE_VERSION_AT_LEAST(major, minor, patch) \
    (CLONEVITTE_VERSION >=                              \
     CLONEVITTE_VERSION_ENCODE((major), (minor), (patch)))

#define CLONEVITTE_LANGUAGE_VERSION_AT_LEAST(major, minor, patch) \
    (CLONEVITTE_LANGUAGE_VERSION >=                              \
     CLONEVITTE_VERSION_ENCODE((major), (minor), (patch)))

#define CLONEVITTE_API_VERSION_AT_LEAST(major, minor, patch) \
    (CLONEVITTE_API_VERSION >=                              \
     CLONEVITTE_VERSION_ENCODE((major), (minor), (patch)))

#ifdef __cplusplus
}
#endif

#endif /* CLONEVITTE_VERSION_H */