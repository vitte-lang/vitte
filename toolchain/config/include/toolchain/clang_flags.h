// toolchain/config/include/toolchain/c_lang_paths.h
// C17 - Canonical path conventions for the C toolchain layer (clang/lld/etc).

#ifndef VITTE_TOOLCHAIN_C_LANG_PATHS_H
#define VITTE_TOOLCHAIN_C_LANG_PATHS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#if defined(_WIN32) || defined(_WIN64)
  #define VITTE_TC_PATH_SEP_CHAR '\\'
  #define VITTE_TC_PATH_SEP_STR  "\\"
  #define VITTE_TC_PATH_LIST_SEP_CHAR ';'
  #define VITTE_TC_PATH_LIST_SEP_STR  ";"
#else
  #define VITTE_TC_PATH_SEP_CHAR '/'
  #define VITTE_TC_PATH_SEP_STR  "/"
  #define VITTE_TC_PATH_LIST_SEP_CHAR ':'
  #define VITTE_TC_PATH_LIST_SEP_STR  ":"
#endif

//------------------------------------------------------------------------------
// Toolchain asset locations (repo-relative)
//------------------------------------------------------------------------------
//
// These are *logical* paths used by the toolchain config layer.
// The build system may map them to absolute paths at runtime.
//
#define VITTE_TC_ASSETS_DIR              "toolchain/assets"
#define VITTE_TC_ASSETS_RSP_DIR          "toolchain/assets/reponse_files"
#define VITTE_TC_ASSETS_STUBS_DIR        "toolchain/assets/stubs"

#define VITTE_TC_RSP_CLANG_TMPL          VITTE_TC_ASSETS_RSP_DIR VITTE_TC_PATH_SEP_STR "clang.rsp.tmpl"
#define VITTE_TC_RSP_LLD_TMPL            VITTE_TC_ASSETS_RSP_DIR VITTE_TC_PATH_SEP_STR "lld.rsp.tmpl"

#define VITTE_TC_STUB_EMPTY_C            VITTE_TC_ASSETS_STUBS_DIR VITTE_TC_PATH_SEP_STR "empty.c"

//------------------------------------------------------------------------------
// Default output layout (project-relative)
//------------------------------------------------------------------------------

#define VITTE_TC_BUILD_DIR               "build"
#define VITTE_TC_BUILD_OBJ_DIR           VITTE_TC_BUILD_DIR VITTE_TC_PATH_SEP_STR "obj"
#define VITTE_TC_BUILD_BIN_DIR           VITTE_TC_BUILD_DIR VITTE_TC_PATH_SEP_STR "bin"
#define VITTE_TC_BUILD_LIB_DIR           VITTE_TC_BUILD_DIR VITTE_TC_PATH_SEP_STR "lib"
#define VITTE_TC_BUILD_TMP_DIR           VITTE_TC_BUILD_DIR VITTE_TC_PATH_SEP_STR "tmp"
#define VITTE_TC_BUILD_RSP_DIR           VITTE_TC_BUILD_DIR VITTE_TC_PATH_SEP_STR "rsp"

//------------------------------------------------------------------------------
// Filename conventions
//------------------------------------------------------------------------------

#define VITTE_TC_EXT_C                   ".c"
#define VITTE_TC_EXT_CPP                 ".cc"
#define VITTE_TC_EXT_OBJ_WINDOWS         ".obj"
#define VITTE_TC_EXT_OBJ_POSIX           ".o"
#define VITTE_TC_EXT_DEP                 ".d"
#define VITTE_TC_EXT_RSP                 ".rsp"

static inline const char* vitte_tc_obj_ext(void) {
#if defined(_WIN32) || defined(_WIN64)
    return VITTE_TC_EXT_OBJ_WINDOWS;
#else
    return VITTE_TC_EXT_OBJ_POSIX;
#endif
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // VITTE_TOOLCHAIN_C_LANG_PATHS_H
