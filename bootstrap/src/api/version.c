#include "version.h"

static const vitte_version_t g_vitte_version = {
    VITTE_VERSION_MAJOR,
    VITTE_VERSION_MINOR,
    VITTE_VERSION_PATCH,
    VITTE_ABI_VERSION,
    "0.1.0",
    "bootstrap-c17"
};

const vitte_version_t *vitte_version(void) {
    return &g_vitte_version;
}

uint32_t vitte_version_major(void) {
    return g_vitte_version.major;
}

uint32_t vitte_version_minor(void) {
    return g_vitte_version.minor;
}

uint32_t vitte_version_patch(void) {
    return g_vitte_version.patch;
}

uint32_t vitte_abi_version(void) {
    return g_vitte_version.abi;
}

const char *vitte_version_string(void) {
    return g_vitte_version.string;
}

const char *vitte_build_kind(void) {
    return g_vitte_version.build_kind;
}
