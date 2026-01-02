

# /Users/vincent/Documents/Github/vitte/std/collections/mod.muf
# -----------------------------------------------------------------------------
# Muffin manifest for std/collections
# -----------------------------------------------------------------------------
# MAX goals:
# - Provide std.collections as a standalone package (workspace-member compatible).
# - Offer core data structures with consistent APIs and good test coverage:
#     * vec
#     * deque
#     * list
#     * bitset
#     * hashmap / hashset
#     * btree (map/set)
# - Keep deps minimal: std-core, std-runtime, std-string (optional), std-hash.
# - Include unit tests and micro-bench hooks.
# - Feature-gate heavier structures (btree) and debug checks.
#
# Notes:
# - Align keys with your canonical Muffin EBNF if names differ.
# - Avoid platform dependencies.
# -----------------------------------------------------------------------------

muf 1

package
  name        = "std-collections"
  version     = "0.1.0"
  license     = "MIT"
  description = "Vitte standard library: collections (vec/deque/list/hashmap/btree/bitset)."
  homepage    = "https://github.com/roussov/vitte"
  keywords    = ["stdlib", "collections", "data-structures", "hashmap", "btree", "vec"]
.end

workspace
  kind = "member"
.end

target
  name = "std-collections"
  kind = "lib"
  lang = "vitte"

  src_dir  = "src"
  test_dir = "tests"

  # Public entry modules
  entry
    module = "std.collections"
  .end
  entry
    module = "std.collections.vec"
  .end
  entry
    module = "std.collections.deque"
  .end
  entry
    module = "std.collections.list"
  .end
  entry
    module = "std.collections.bitset"
  .end
  entry
    module = "std.collections.hashmap"
  .end
  entry
    module = "std.collections.hashset"
  .end
  entry
    module = "std.collections.btree"
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
    name     = "std-string"
    path     = "../string"
    optional = true
  .end

  dep
    name     = "std-hash"
    path     = "../hash"
    optional = true
  .end
.end

features
  feature
    name = "vec"
    description = "Enable std.collections.vec."
    default = true
  .end

  feature
    name = "deque"
    description = "Enable std.collections.deque."
    default = true
  .end

  feature
    name = "list"
    description = "Enable std.collections.list."
    default = true
  .end

  feature
    name = "bitset"
    description = "Enable std.collections.bitset."
    default = true
  .end

  feature
    name = "hashmap"
    description = "Enable std.collections.hashmap/hashset."
    default = true
  .end

  feature
    name = "btree"
    description = "Enable std.collections.btree (heavier)."
    default = true
  .end

  feature
    name = "debug_checks"
    description = "Enable extra invariants and bounds checks."
    default = false
  .end

  feature
    name = "bench"
    description = "Expose micro-bench hooks for std/bench."
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
    name = "test"
    run  = "muffin test std-collections"
  .end

  script
    name = "check"
    run  = "muffin check std-collections"
  .end
.end