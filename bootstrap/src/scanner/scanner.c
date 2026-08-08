#include "scanner.h"

static const vitte_bootstrap_src_scanner_scanner_h_module_t g_vitte_bootstrap_src_scanner_scanner_h_module = {
    "src/scanner/scanner.c",
    "Scanner",
    "scanner bootstrap module",
    1u
};

const vitte_bootstrap_src_scanner_scanner_h_module_t *vitte_bootstrap_src_scanner_scanner_h_module(void) {
    return &g_vitte_bootstrap_src_scanner_scanner_h_module;
}

const char *vitte_bootstrap_src_scanner_scanner_h_name(void) {
    return g_vitte_bootstrap_src_scanner_scanner_h_module.name;
}

const char *vitte_bootstrap_src_scanner_scanner_h_category(void) {
    return g_vitte_bootstrap_src_scanner_scanner_h_module.category;
}

const char *vitte_bootstrap_src_scanner_scanner_h_purpose(void) {
    return g_vitte_bootstrap_src_scanner_scanner_h_module.purpose;
}

uint32_t vitte_bootstrap_src_scanner_scanner_h_checksum(const char *text, size_t length) {
    uint32_t hash = 2166136261u;
    size_t index;

    if (text == (const char *)0) {
        return 0u;
    }

    for (index = 0u; index < length; index++) {
        hash ^= (unsigned char)text[index];
        hash *= 16777619u;
    }

    return hash;
}

bool vitte_bootstrap_src_scanner_scanner_h_self_test(void) {
    return vitte_bootstrap_src_scanner_scanner_h_name() != (const char *)0 &&
        vitte_bootstrap_src_scanner_scanner_h_category() != (const char *)0 &&
        vitte_bootstrap_src_scanner_scanner_h_purpose() != (const char *)0 &&
        vitte_bootstrap_src_scanner_scanner_h_checksum(vitte_bootstrap_src_scanner_scanner_h_name(), 0u) == 2166136261u;
}
