#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Minimal ABI for VITTE IDE GTK plugins.
// Build shared libraries exporting:
//   extern "C" const VitteIdePluginV1* vitte_ide_plugin_v1();
typedef struct VitteIdePluginV1 {
    int abi_version;  // must be 1
    const char* plugin_name;
    const char* (*commands_csv)();  // comma-separated command names
    const char* (*run_command)(const char* command, const char* payload);
    const char* (*provide_completion)(const char* file, int line, int col, const char* prefix);
} VitteIdePluginV1;

#ifdef __cplusplus
}
#endif

