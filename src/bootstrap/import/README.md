# src/bootstrap/import

The import resolver maps Vitte module names to source files for the compiler
compiler. It is deterministic, bounded, and uses the compiler filesystem layer.

## Contract

- No dependency on `runtime/*`.
- Errors use `src/bootstrap/api/error.h`.
- Filesystem access goes through `src/bootstrap/filesystem`.
- Search paths are fixed-size and checked for duplicates.
- Module names and paths are bounded.
- Source reads are bounded by `max_source_bytes`.
- Cache storage is fixed-size.
- The resolver never removes or mutates files.

## Module Names

Accepted characters are letters, digits, `_`, `.`, and `/`.

Rejected:

- empty names
- names containing `..`
- repeated separators
- trailing separators
- names that exceed `VITTE_IMPORT_MAX_MODULE_NAME`

`math.add` and `math/add` both resolve to `math/add.vit`.

## Search Paths

Search paths are tried in insertion order. For module `foo.bar`, each search
path tries:

1. `foo/bar.vit`
2. `foo/bar/mod.vit`

Relative requests use the importer file directory before global search paths.

## Cache

The resolver keeps a fixed cache keyed by module name and importer base path.
Cache entries own their source buffers. Results returned from cache borrow those
buffers and must not free them.

`vitte_import_resolver_clear_cache` releases all cache-owned sources.
`vitte_import_resolver_destroy` clears cache and resolver state.

The CLI registers every discovered source in `vitte_import_context_t`, using
the resolved physical path as its identity and retaining the logical path for
source locations.  A module reached through two relative spellings or a
symlink is therefore loaded once.  Import edges, aliases, and their source
spans are registered in the same context before `vitte_import_resolve()` is
called.

Import aliases are bindings in the importing module.  Reusing one alias for
two different targets is rejected as an alias conflict; an explicit alias
must be changed before resolution can continue.

Each graph edge keeps the complete `use` declaration span: source file ID,
byte range, and one-based line/column coordinates.  The edge also retains the
logical module name and optional alias, allowing CLI diagnostics to identify
the exact declaration that introduced an import.

## Result Ownership

`vitte_import_result_t` stores the resolved path in a fixed buffer. Source
ownership is explicit:

- `source_owned == false`: source is borrowed from resolver cache.
- `source_owned == true`: caller must release through
  `vitte_import_result_destroy`.

The current resolver caches resolved modules, so normal resolve results borrow
cache-owned source buffers.

## Depth And Cycles

Requests carry a `depth` field. The resolver rejects requests deeper than
`max_depth`, and direct self-import is rejected when a resolved path equals the
importer path. The bootstrap driver performs source discovery and registers
each edge in the graph context; graph resolution then detects cycles and
exposes the complete `A -> B -> ... -> A` chain for the CLI diagnostic. The
legacy source assembly bookkeeping remains only as a transitional layer while
the graph becomes the single authority for identity, edges, aliases, and
cycle diagnostics.

## Minimal Example

```c
vitte_import_resolver_t resolver;
vitte_import_request_t request;
vitte_import_result_t result;

vitte_import_resolver_init(&resolver, NULL);
vitte_import_resolver_add_search_path(&resolver, "src");

vitte_import_request_init(&request);
request.module_name = "math.add";

vitte_import_result_init(&result);
vitte_import_resolve(&resolver, &request, &result);

vitte_import_result_destroy(&result);
vitte_import_resolver_destroy(&resolver);
```
