#ifndef VITTE_BOOTSTRAP_INCLUDE_VITTE_CLI_H_H
#define VITTE_BOOTSTRAP_INCLUDE_VITTE_CLI_H_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vitte_bootstrap_include_vitte_cli_h_module {
    const char *name;
    const char *category;
    const char *purpose;
    uint32_t version;
} vitte_bootstrap_include_vitte_cli_h_module_t;

const vitte_bootstrap_include_vitte_cli_h_module_t *vitte_bootstrap_include_vitte_cli_h_module(void);
const char *vitte_bootstrap_include_vitte_cli_h_name(void);
const char *vitte_bootstrap_include_vitte_cli_h_category(void);
const char *vitte_bootstrap_include_vitte_cli_h_purpose(void);
uint32_t vitte_bootstrap_include_vitte_cli_h_checksum(const char *text, size_t length);
bool vitte_bootstrap_include_vitte_cli_h_self_test(void);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_INCLUDE_VITTE_CLI_H_H */
