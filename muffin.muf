

# ============================================================================
# Muffin workspace manifest (vitte)
# ============================================================================
# Devise: accessible à tous • performant • utilisable pour tout
#
# Muffin est un manifest de build (comme Cargo.toml) + un "header" de workspace.
# Sans `muffin.muf` : rien ne build.
#
# Format Muffin 2025 (proposition stable)
# - Syntaxe déclarative orientée build graph
# - Blocs terminés par `.end` (pas d'accolades)
# - Chaînes: "..." ; commentaires: '#'
# - Paths: relatifs au workspace, séparateur '/'
#
# Remarque: ce fichier est volontairement exhaustif (MAX). Le driver `steel`
# peut ignorer des champs non supportés au début, mais le format reste stable.
# ============================================================================

workspace "vitte"
  version: "0.1.0"
  edition: 2025
  license: "MIT"
  homepage: "local"
  description: "Steel/Vitte toolchain + runtime + stdlib"

  # Layout
  root_dir: "."
  build_dir: "target"
  cache_dir: "target/cache"
  plan_dir:  "target/plan"
  dist_dir:  "target/dist"
  releases_dir: "target/releases"
  tmp_dir:   "target/tmp"

  # Lock
  lockfile: "muffin.lock"
  lock_policy: "required"                # required | optional | none

  # Defaults
  default_profile: "debug"               # debug | release | sanitize | bench
  default_target:  "host"                # host | darwin_arm64 | linux_x86_64 | ...
  default_package: "steel"               # package or meta-task entry

  # Workspace members (projects that have leur own muffin.muf)
  member "std"
  member "examples/hello"
  member "examples/ffi_c"

  # Ignored paths
  ignore_dir "target"
  ignore_dir ".git"
  ignore_dir ".vscode"

  # Feature flags (global)
  feature "asm" default off
  feature "tests" default on
  feature "bench" default on
  feature "fuzz" default off
  feature "docs" default on

  # Policies
  policy "reproducible" = on
  policy "offline" = on
  policy "werror" = off
  policy "color" = "auto"               # auto|always|never
  policy "diag" = "rich"                 # rich|plain
  policy "timings" = on
.end

# ============================================================================
# Toolchains
# ============================================================================

toolchain "system"
  kind: "native"                         # native | zig | msvc
  cc: "clang"
  cxx: "clang++"
  ar: "ar"
  ranlib: "ranlib"
  strip: "strip"
  pkg_config: "pkg-config"
  linker: "auto"                         # auto|lld|ld|link

  env_allow "CC"
  env_allow "CFLAGS"
  env_allow "LDFLAGS"
.end

toolchain "zig"
  kind: "zig"
  zig: "toolchain/zig/bin/zig"
  cc: "zig cc"
  cxx: "zig c++"
  ar: "zig ar"
  ranlib: "zig ranlib"
  linker: "auto"
.end

toolchain "msvc"
  kind: "msvc"
  cc: "cl"
  linker: "link"
.end

# ============================================================================
# Targets (triples + flags)
# ============================================================================

target "host"
  kind: "host"
  triple: "auto"
  toolchain: "system"
.end

target "darwin_arm64"
  kind: "native"
  os: "darwin"
  arch: "arm64"
  triple: "aarch64-apple-darwin"
  toolchain: "system"

  define "STEEL_DARWIN" = "1"
  define "STEEL_ARCH_AARCH64" = "1"

  cflag "-std=c11"
  cflag "-Wall"
  cflag "-Wextra"
  cflag "-Wshadow"
  cflag "-Wstrict-prototypes"
  cflag "-Wmissing-prototypes"
  cflag "-fno-omit-frame-pointer"
  cflag "-mmacosx-version-min=11.0"

  ldflag "-lm"
.end

target "darwin_x86_64"
  kind: "native"
  os: "darwin"
  arch: "x86_64"
  triple: "x86_64-apple-darwin"
  toolchain: "system"

  define "STEEL_DARWIN" = "1"
  define "STEEL_ARCH_X86_64" = "1"

  cflag "-std=c11"
  cflag "-Wall"
  cflag "-Wextra"
  cflag "-Wshadow"
  cflag "-Wstrict-prototypes"
  cflag "-Wmissing-prototypes"
  cflag "-fno-omit-frame-pointer"
  cflag "-mmacosx-version-min=11.0"

  ldflag "-lm"
.end

target "linux_x86_64"
  kind: "native"
  os: "linux"
  arch: "x86_64"
  triple: "x86_64-unknown-linux-gnu"
  toolchain: "system"

  define "STEEL_LINUX" = "1"
  define "STEEL_ARCH_X86_64" = "1"

  cflag "-std=c11"
  cflag "-Wall"
  cflag "-Wextra"
  cflag "-Wshadow"
  cflag "-Wstrict-prototypes"
  cflag "-Wmissing-prototypes"
  cflag "-fno-omit-frame-pointer"

  ldflag "-lm"
  ldflag "-ldl"
  ldflag "-pthread"
.end

target "freebsd_x86_64"
  kind: "native"
  os: "freebsd"
  arch: "x86_64"
  triple: "x86_64-unknown-freebsd"
  toolchain: "system"

  define "STEEL_FREEBSD" = "1"
  define "STEEL_ARCH_X86_64" = "1"

  cflag "-std=c11"
  cflag "-Wall"
  cflag "-Wextra"
  cflag "-fno-omit-frame-pointer"

  ldflag "-lm"
  ldflag "-pthread"
.end

target "solaris_x86_64"
  kind: "native"
  os: "solaris"
  arch: "x86_64"
  triple: "x86_64-pc-solaris2.11"
  toolchain: "system"

  define "STEEL_SOLARIS" = "1"
  define "STEEL_ARCH_X86_64" = "1"

  cflag "-std=c11"
  cflag "-Wall"
  cflag "-Wextra"
  cflag "-fno-omit-frame-pointer"

  ldflag "-lm"
  ldflag "-lsocket"
  ldflag "-lnsl"
.end

target "windows_x86_64"
  kind: "native"
  os: "windows"
  arch: "x86_64"
  triple: "x86_64-pc-windows-msvc"
  toolchain: "msvc"

  define "STEEL_WIN32" = "1"
  define "STEEL_ARCH_X86_64" = "1"

  syslib "ws2_32"
  syslib "advapi32"
  syslib "userenv"
.end

# ============================================================================
# Profiles
# ============================================================================

profile "debug"
  opt: "O0"
  debug: on
  sanitize: off
  lto: off
  werror: off

  define "DEBUG" = "1"
  cflag "-g"
.end

profile "release"
  opt: "O2"
  debug: off
  sanitize: off
  lto: on
  werror: off

  define "NDEBUG" = "1"
  cflag "-DNDEBUG=1"
.end

profile "sanitize"
  opt: "O1"
  debug: on
  sanitize: on
  lto: off
  werror: off

  define "DEBUG" = "1"
  cflag "-g"
  cflag "-fsanitize=address,undefined" if toolchain "system|zig" and not os "windows"
  ldflag "-fsanitize=address,undefined" if toolchain "system|zig" and not os "windows"
.end

profile "bench"
  opt: "O2"
  debug: off
  sanitize: off
  lto: off
  werror: off

  define "NDEBUG" = "1"
  define "STEEL_BENCH" = "1"
  cflag "-DNDEBUG=1"
.end

# ============================================================================
# Packages — Core libs
# ============================================================================

package "steel_common"
  kind: "c.static"
  public: off

  include_dir "include"

  source "src/common/arena.c"
  source "src/common/vec.c"
  source "src/common/hashmap.c"
  source "src/common/str.c"
  source "src/common/utf8.c"
  source "src/common/path.c"
  source "src/common/fs.c"
  source "src/common/os.c"
  source "src/common/atomics.c"
  source "src/common/log.c"

  define "STEEL_ENABLE_LOG" = "1"
.end

package "steel_diag"
  kind: "c.static"
  public: off

  include_dir "include"

  dep "steel_common"

  source "src/diag/span.c"
  source "src/diag/source_map.c"
  source "src/diag/diag.c"
  source "src/diag/codes.c"
.end

package "steel_pal"
  kind: "c.static"
  public: off

  include_dir "include"

  dep "steel_common"
  dep "steel_diag"

  when os "windows"
    source "src/pal/win32/pal_win32.c"
    source "src/pal/win32/pal_win32_fs.c"
    source "src/pal/win32/pal_win32_time.c"
    source "src/pal/win32/pal_win32_thread.c"
    source "src/pal/win32/pal_win32_dynload.c"
    source "src/pal/win32/pal_win32_net.c"
    source "src/pal/win32/pal_win32_proc.c"

    syslib "ws2_32"
    syslib "advapi32"
    syslib "userenv"
  .end

  when not os "windows"
    source "src/pal/posix/pal_posix.c"
    source "src/pal/posix/pal_posix_fs.c"
    source "src/pal/posix/pal_posix_time.c"
    source "src/pal/posix/pal_posix_thread.c"
    source "src/pal/posix/pal_posix_dynload.c"
    source "src/pal/posix/pal_posix_net.c"
    source "src/pal/posix/pal_posix_proc.c"

    syslib "m"
    syslib "pthread" if os "linux|freebsd|openbsd|netbsd|solaris"
    syslib "dl" if os "linux"
    syslib "socket" if os "solaris"
    syslib "nsl" if os "solaris"
  .end
.end

package "steel_runtime"
  kind: "c.static"
  public: on

  include_dir "include"

  dep "steel_common"
  dep "steel_diag"
  dep "steel_pal"

  source "src/runtime/rt_alloc.c"
  source "src/runtime/rt_slice.c"
  source "src/runtime/rt_string.c"
  source "src/runtime/rt_handles.c"
  source "src/runtime/rt_panic.c"

  # Optional asm acceleration (memwipe / ct_eq)
  when feature "asm"
    source "asm/wrappers/asm_memwipe.c"
    source "asm/wrappers/asm_ct_eq.c"

    source "asm/x86_64/memwipe.S" arch "x86_64"
    source "asm/x86_64/ct_eq.S" arch "x86_64"

    source "asm/aarch64/memwipe.S" arch "arm64"
    source "asm/aarch64/ct_eq.S" arch "arm64"

    define "STEEL_HAVE_ASM" = "1"
  .end
.end

# ============================================================================
# Packages — Language front-ends
# ============================================================================

package "steel_muf"
  kind: "c.static"
  public: off

  include_dir "include"

  dep "steel_common"
  dep "steel_diag"

  source "src/muf/muf_lex.c"
  source "src/muf/muf_parse.c"
  source "src/muf/muf_ast.c"
  source "src/muf/muf_validate.c"
  source "src/muf/muf_semantics.c"
  source "src/muf/muf_graph.c"
  source "src/muf/muf_lock.c"
  source "src/muf/muf_emit.c"
  source "src/muf/muf_fetch.c"
.end

package "steel_vitte"
  kind: "c.static"
  public: off

  include_dir "include"

  dep "steel_common"
  dep "steel_diag"

  source "src/vitte/tokens.c"
  source "src/vitte/lexer.c"
  source "src/vitte/parser_core.c"
  source "src/vitte/parser_phrase.c"
  source "src/vitte/desugar_phrase.c"
  source "src/vitte/ast.c"
  source "src/vitte/ast_ids.c"
  source "src/vitte/ast_printer.c"
.end

# ============================================================================
# Packages — Compiler (IR + backend C)
# ============================================================================

package "steel_compiler"
  kind: "c.static"
  public: off

  include_dir "include"

  dep "steel_common"
  dep "steel_diag"
  dep "steel_pal"
  dep "steel_runtime"
  dep "steel_muf"
  dep "steel_vitte"

  source "src/compiler/interner.c"
  source "src/compiler/symtab.c"
  source "src/compiler/resolver.c"
  source "src/compiler/types.c"
  source "src/compiler/typecheck.c"
  source "src/compiler/hir_build.c"
  source "src/compiler/ir_build.c"
  source "src/compiler/passes.c"

  source "src/compiler/backend_c/c_emit.c"
  source "src/compiler/backend_c/c_runtime_shim.c"
  source "src/compiler/backend_c/c_name_mangle.c"

  source "src/compiler/driver/compile_unit.c"
  source "src/compiler/driver/link_step.c"
  source "src/compiler/driver/pipeline.c"

  define "STEEL_BACKEND_C" = "1"
.end

# ============================================================================
# Packages — Executables
# ============================================================================

package "steel"
  kind: "c.exe"
  public: on

  include_dir "include"

  dep "steel_compiler"

  source "src/cli/steel_main.c"
  source "src/cli/steel_args.c"
  source "src/cli/steel_cmd_build.c"
  source "src/cli/steel_cmd_run.c"
  source "src/cli/steel_cmd_check.c"
  source "src/cli/steel_cmd_test.c"
  source "src/cli/steel_cmd_clean.c"
  source "src/cli/steel_cmd_fmt.c" if feature "docs"
  source "src/cli/steel_cmd_doc.c" if feature "docs"
  source "src/cli/steel_cmd_pkg.c" if feature "docs"

  output "target/bin/steel"
.end

package "steelc"
  kind: "c.exe"
  public: on

  include_dir "include"

  dep "steel_compiler"

  source "src/cli/steelc_main.c"
  source "src/cli/steelc_front.c"
  source "src/cli/steelc_driver.c"

  output "target/bin/steelc"
.end

# ============================================================================
# Packages — Tests (C)
# ============================================================================

package "tests_unit"
  kind: "meta"
  public: off

  when feature "tests"
    dep "t_muf_parse"
    dep "t_muf_validate"
    dep "t_lexer"
    dep "t_parser_core"
    dep "t_parser_phrase"
    dep "t_desugar_phrase"
    dep "t_resolver"
    dep "t_typecheck"
    dep "t_backend_c"
    dep "t_handles"
    dep "t_fs"
    dep "t_threads"
  .end
.end

package "t_muf_parse"
  kind: "c.test"
  include_dir "include"
  dep "steel_muf"
  dep "steel_diag"
  dep "steel_common"
  source "tests/unit/muf/t_muf_parse.c"
  output "target/tests/unit/t_muf_parse"
.end

package "t_muf_validate"
  kind: "c.test"
  include_dir "include"
  dep "steel_muf"
  dep "steel_diag"
  dep "steel_common"
  source "tests/unit/muf/t_muf_validate.c"
  output "target/tests/unit/t_muf_validate"
.end

package "t_lexer"
  kind: "c.test"
  include_dir "include"
  dep "steel_vitte"
  dep "steel_diag"
  dep "steel_common"
  source "tests/unit/vitte/t_lexer.c"
  output "target/tests/unit/t_lexer"
.end

package "t_parser_core"
  kind: "c.test"
  include_dir "include"
  dep "steel_vitte"
  dep "steel_diag"
  dep "steel_common"
  source "tests/unit/vitte/t_parser_core.c"
  output "target/tests/unit/t_parser_core"
.end

package "t_parser_phrase"
  kind: "c.test"
  include_dir "include"
  dep "steel_vitte"
  dep "steel_diag"
  dep "steel_common"
  source "tests/unit/vitte/t_parser_phrase.c"
  output "target/tests/unit/t_parser_phrase"
.end

package "t_desugar_phrase"
  kind: "c.test"
  include_dir "include"
  dep "steel_vitte"
  dep "steel_diag"
  dep "steel_common"
  source "tests/unit/vitte/t_desugar_phrase.c"
  output "target/tests/unit/t_desugar_phrase"
.end

package "t_resolver"
  kind: "c.test"
  include_dir "include"
  dep "steel_compiler"
  source "tests/unit/compiler/t_resolver.c"
  output "target/tests/unit/t_resolver"
.end

package "t_typecheck"
  kind: "c.test"
  include_dir "include"
  dep "steel_compiler"
  source "tests/unit/compiler/t_typecheck.c"
  output "target/tests/unit/t_typecheck"
.end

package "t_backend_c"
  kind: "c.test"
  include_dir "include"
  dep "steel_compiler"
  source "tests/unit/compiler/t_backend_c.c"
  output "target/tests/unit/t_backend_c"
.end

package "t_handles"
  kind: "c.test"
  include_dir "include"
  dep "steel_runtime"
  source "tests/unit/runtime/t_handles.c"
  output "target/tests/unit/t_handles"
.end

package "t_fs"
  kind: "c.test"
  include_dir "include"
  dep "steel_pal"
  source "tests/unit/pal/t_fs.c"
  output "target/tests/unit/t_fs"
.end

package "t_threads"
  kind: "c.test"
  include_dir "include"
  dep "steel_pal"
  source "tests/unit/pal/t_threads.c"
  output "target/tests/unit/t_threads"
.end

# ============================================================================
# Packages — Fuzz (optional)
# ============================================================================

package "fuzz_targets"
  kind: "meta"
  public: off

  when feature "fuzz"
    dep "fuzz_muf"
    dep "fuzz_vitte_parser"
    dep "fuzz_typecheck"
  .end
.end

package "fuzz_muf"
  kind: "c.fuzz"
  include_dir "include"
  dep "steel_muf"
  source "fuzz/targets/fuzz_muf.c"
  output "target/fuzz/fuzz_muf"
.end

package "fuzz_vitte_parser"
  kind: "c.fuzz"
  include_dir "include"
  dep "steel_vitte"
  source "fuzz/targets/fuzz_vitte_parser.c"
  output "target/fuzz/fuzz_vitte_parser"
.end

package "fuzz_typecheck"
  kind: "c.fuzz"
  include_dir "include"
  dep "steel_compiler"
  source "fuzz/targets/fuzz_typecheck.c"
  output "target/fuzz/fuzz_typecheck"
.end

# ============================================================================
# Packages — Bench micro (optional)
# ============================================================================

package "bench_micro"
  kind: "meta"
  public: off

  when feature "bench"
    dep "bench_lexer"
    dep "bench_parser"
    dep "bench_backend_c"
    dep "bench_pal_fs"
  .end
.end

package "bench_lexer"
  kind: "c.bench"
  include_dir "include"
  dep "steel_vitte"
  source "bench/micro/bench_lexer.c"
  output "target/bench/bench_lexer"
.end

package "bench_parser"
  kind: "c.bench"
  include_dir "include"
  dep "steel_vitte"
  source "bench/micro/bench_parser.c"
  output "target/bench/bench_parser"
.end

package "bench_backend_c"
  kind: "c.bench"
  include_dir "include"
  dep "steel_compiler"
  source "bench/micro/bench_backend_c.c"
  output "target/bench/bench_backend_c"
.end

package "bench_pal_fs"
  kind: "c.bench"
  include_dir "include"
  dep "steel_pal"
  source "bench/micro/bench_pal_fs.c"
  output "target/bench/bench_pal_fs"
.end

# ============================================================================
# Vitte stdlib + examples (sources)
# ============================================================================

package "stdlib"
  kind: "vitte.pkg"
  public: on

  # std/ has its own muffin.muf; this is a workspace-level alias
  path "std"
.end

package "example_hello"
  kind: "vitte.pkg"
  public: off
  path "examples/hello"
.end

package "example_ffi_c"
  kind: "vitte.pkg"
  public: off
  path "examples/ffi_c"
.end

# ============================================================================
# Meta tasks (build graph entrypoints)
# ============================================================================

task "toolchain"
  dep "steel"
  dep "steelc"
.end

task "all"
  dep "toolchain"
  dep "tests_unit" if feature "tests"
  dep "bench_micro" if feature "bench"
  dep "fuzz_targets" if feature "fuzz"
  dep "stdlib"
  dep "example_hello"
  dep "example_ffi_c"
.end

# ============================================================================
# Install/export rules (semantic)
# ============================================================================

export "sdk"
  bin "target/bin/steel" as "bin/steel"
  bin "target/bin/steelc" as "bin/steelc"

  dir "include" as "include/src"
  dir "sdk/include" as "include/sdk" if exists "sdk/include"
  dir "spec" as "spec"

  file "muffin.muf" as "muffin.muf"
  file "muffin.lock" as "muffin.lock" if exists "muffin.lock"
  file "VERSION" as "VERSION" if exists "VERSION"
  file "LICENSE" as "LICENSE" if exists "LICENSE"
.end