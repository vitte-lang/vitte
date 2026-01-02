# /Users/vincent/Documents/Github/vitte/std/regex/mod.muf
# -----------------------------------------------------------------------------
# Muffin manifest for std/regex
# -----------------------------------------------------------------------------
# MAX goals:
# - Provide std.regex as a standalone package (workspace-member compatible).
# - Expose regex APIs with two layers:
#     * lite: simple pattern matching (glob-like / subset) (optional)
#     * full: compiled regex engine (NFA/DFA) (default)
# - Allow optional native backend (PCRE2/RE2) via feature flags.
# - Keep deps minimal: std-core, std-runtime, std-string; std-collections optional.
# - Include unit tests and test vectors.
# - Feature-gate Unicode character classes and heavy tables.
#
# Notes:
# - Prefer a deterministic, sandboxable engine as default.
# -----------------------------------------------------------------------------

muf 1

package
  name        = "std-regex"
  version     = "0.1.0"
  license     = "MIT"
  description = "Vitte standard library: regex engine (compile/match/find/replace), optional native backend."
  homepage    = "https://github.com/roussov/vitte"
  keywords    = ["stdlib", "regex", "pattern", "matching", "nfa", "dfa"]
.end

workspace
  kind = "member"
.end

target
  name = "std-regex"
  kind = "lib"
  lang = "vitte"

  src_dir  = "src"
  test_dir = "tests"

  # Public entry modules
  entry
    module = "std.regex"
  .end
  entry
    module = "std.regex.ast"
  .end
  entry
    module = "std.regex.compile"
  .end
  entry
    module = "std.regex.match"
  .end
  entry
    module = "std.regex.replace"
  .end
  entry
    module = "std.regex.lite"
  .end
.end

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
    name = "std-string"
    path = "../string"
  .end

  dep
    name     = "std-collections"
    path     = "../collections"
    optional = true
  .end

  dep
    name     = "std-ffi"
    path     = "../ffi"
    optional = true
  .end

  dep
    name     = "std-unicode"
    path     = "../unicode"
    optional = true
  .end
.end

features
  feature
    name = "engine"
    description = "Enable built-in compiled regex engine (AST + NFA/DFA)."
    default = true
  .end

  feature
    name = "lite"
    description = "Enable lightweight pattern matching subset (glob-like)."
    default = true
  .end

  feature
    name = "unicode"
    description = "Enable Unicode character classes and case folding (requires std-unicode)."
    default = false
    requires = ["std-unicode"]
  .end

  feature
    name = "native_backend"
    description = "Enable native backend bindings (requires std-ffi)."
    default = false
    requires = ["std-ffi"]
  .end

  feature
    name = "pcre2"
    description = "Use PCRE2 native backend (requires native_backend)."
    default = false
    requires = ["native_backend"]
  .end

  feature
    name = "re2"
    description = "Use RE2 native backend (requires native_backend)."
    default = false
    requires = ["native_backend"]
  .end

  feature
    name = "tables"
    description = "Use precomputed tables for fast parsing/matching (bigger binary)."
    default = true
  .end

  feature
    name = "no_alloc"
    description = "Prefer caller-provided buffers; reduce allocations."
    default = false
  .end

  feature
    name = "test_vectors"
    description = "Include extended regex test vectors (slower tests)."
    default = true
  .end
.end

profile
  opt_level = 2
  debug = true
  warnings_as_errors = false
.end

scripts
  script
    name = "test"
    run  = "muffin test std-regex"
  .end

  script
    name = "check"
    run  = "muffin check std-regex"
  .end
.end
