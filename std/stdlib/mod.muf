

# /Users/vincent/Documents/Github/vitte/std/stdlib/mod.muf
# -----------------------------------------------------------------------------
# Muffin manifest for std/stdlib (meta-package)
# -----------------------------------------------------------------------------
# MAX goals:
# - Provide a single dependency "std" / "std-stdlib" that pulls the full
#   standard library set as a coherent bundle.
# - Keep it as a meta-package: no code, only dependencies/features.
# - Allow selecting subsets via features (core-only, io, fs, cli, net, crypto,
#   serialize, regex, etc.).
# - Provide sensible defaults for typical application builds.
# - Stage0-friendly: allow disabling heavy deps and sys backends.
#
# Notes:
# - This package is intentionally thin. The actual modules are in each package.
# - Consumers can depend on std-stdlib and enable/disable features.
# -----------------------------------------------------------------------------

muf 1

package
  name        = "std-stdlib"
  version     = "0.1.0"
  license     = "MIT"
  description = "Vitte standard library bundle (meta-package)."
  homepage    = "https://github.com/roussov/vitte"
  keywords    = ["stdlib", "std", "bundle", "meta"]
.end

workspace
  kind = "member"
.end

target
  name = "std-stdlib"
  kind = "meta"
  lang = "none"
.end

# -----------------------------------------------------------------------------
# Dependencies (all optional; driven by features)
# -----------------------------------------------------------------------------

deps
  dep
    name = "std-core"
    path = "../core"
  .end

  dep
    name = "std-runtime"
    path = "../runtime"
  .end

  dep
    name     = "std-mem"
    path     = "../mem"
    optional = true
  .end

  dep
    name     = "std-collections"
    path     = "../collections"
    optional = true
  .end

  dep
    name     = "std-string"
    path     = "../string"
    optional = true
  .end

  dep
    name     = "std-io"
    path     = "../io"
    optional = true
  .end

  dep
    name     = "std-fs"
    path     = "../fs"
    optional = true
  .end

  dep
    name     = "std-cli"
    path     = "../cli"
    optional = true
  .end

  dep
    name     = "std-path"
    path     = "../path"
    optional = true
  .end

  dep
    name     = "std-net"
    path     = "../net"
    optional = true
  .end

  dep
    name     = "std-crypto"
    path     = "../crypto"
    optional = true
  .end

  dep
    name     = "std-regex"
    path     = "../regex"
    optional = true
  .end

  dep
    name     = "std-serialize"
    path     = "../serialize"
    optional = true
  .end

  dep
    name     = "std-time"
    path     = "../time"
    optional = true
  .end

  dep
    name     = "std-log"
    path     = "../log"
    optional = true
  .end

  dep
    name     = "std-encoding"
    path     = "../encoding"
    optional = true
  .end

  dep
    name     = "std-unicode"
    path     = "../unicode"
    optional = true
  .end

  dep
    name     = "std-sys"
    path     = "../sys"
    optional = true
  .end
.end

# -----------------------------------------------------------------------------
# Feature matrix
# -----------------------------------------------------------------------------

features
  # Minimal always-on core surface.
  feature
    name = "core"
    description = "Core stdlib (std-core + std-runtime)."
    default = true
  .end

  feature
    name = "mem"
    description = "Memory utilities (std-mem)."
    default = true
    requires = ["std-mem"]
  .end

  feature
    name = "collections"
    description = "Collections (vec/hashmap/etc.)."
    default = true
    requires = ["std-collections"]
  .end

  feature
    name = "string"
    description = "String utilities."
    default = true
    requires = ["std-string"]
  .end

  feature
    name = "io"
    description = "I/O (reader/writer/buffered)."
    default = true
    requires = ["std-io"]
  .end

  feature
    name = "fs"
    description = "Filesystem (path/walk + sys backend)."
    default = true
    requires = ["std-fs", "std-path"]
  .end

  feature
    name = "cli"
    description = "CLI utilities (args, ansi, progress, prompt, term)."
    default = true
    requires = ["std-cli"]
  .end

  feature
    name = "path"
    description = "Pure path manipulation."
    default = true
    requires = ["std-path"]
  .end

  feature
    name = "net"
    description = "Networking (tcp/udp/ip + optional dns)."
    default = false
    requires = ["std-net"]
  .end

  feature
    name = "crypto"
    description = "Cryptography primitives."
    default = false
    requires = ["std-crypto"]
  .end

  feature
    name = "regex"
    description = "Regex engine."
    default = false
    requires = ["std-regex"]
  .end

  feature
    name = "serialize"
    description = "Serialization codecs (varint/endian/hex/base64)."
    default = false
    requires = ["std-serialize"]
  .end

  feature
    name = "time"
    description = "Time utilities (if available in std-time)."
    default = true
    requires = ["std-time"]
  .end

  feature
    name = "log"
    description = "Logging facade."
    default = true
    requires = ["std-log"]
  .end

  feature
    name = "encoding"
    description = "Encoding helpers (percent/base64/etc)."
    default = false
    requires = ["std-encoding"]
  .end

  feature
    name = "unicode"
    description = "Unicode tables/utilities."
    default = false
    requires = ["std-unicode"]
  .end

  feature
    name = "sys"
    description = "OS/syscall bridge via std-sys (enables sys-backed implementations)."
    default = true
    requires = ["std-sys"]
  .end

  # Convenience presets
  feature
    name = "default"
    description = "Default full-ish bundle for apps (core+mem+collections+string+io+fs+cli+path+time+log+sys)."
    default = true
    requires = ["core", "mem", "collections", "string", "io", "fs", "cli", "path", "time", "log", "sys"]
  .end

  feature
    name = "minimal"
    description = "Minimal stage0-friendly bundle (core only)."
    default = false
    requires = ["core"]
  .end

  feature
    name = "server"
    description = "Server preset (default + net + serialize + regex)."
    default = false
    requires = ["default", "net", "serialize", "regex"]
  .end

  feature
    name = "embedded"
    description = "Embedded preset (core + mem + collections + string; no sys, no fs, no net)."
    default = false
    requires = ["core", "mem", "collections", "string"]
  .end

  feature
    name = "no_sys"
    description = "Disable sys backend (for sandbox/stage0)."
    default = false
  .end
.end

profile
  opt_level = 2
  debug = true
  warnings_as_errors = false
.end

scripts
  script
    name = "check"
    run  = "muffin check std-stdlib"
  .end
  script
    name = "test"
    run  = "muffin test std-stdlib"
  .end
.end