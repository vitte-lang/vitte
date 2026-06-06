# Stdlib

The Vitte standard library is a family-based surface, not a flat pile of helpers.

## Source Of Truth

- Tree root: `src/vitte/stdlib`
- Compiler/runtime contract: `docs/book/compiler-stdlib-contract.html`
- Book overview: `docs/book/chapters/17-stdlib.html`
- Catalog page: `docs/book/stdlib.html`

## Main Families

- `core`, `strings`, `memory`, `runtime`
  Owns scalar types, low-level helpers, string-oriented routines, and small reusable building blocks.
- `collections`, `data`
  Owns vectors, queues, maps, sets, graphs, matrices, schema work, transforms, and dataset manipulation.
- `path`, `io`, `os`, `sysinfo`
  Owns host interaction: files, streams, directories, walkers, special paths, and system inspection.
- `json`, `encoding`, `regex`
  Owns textual and structured exchange boundaries.
- `math`
  Owns arithmetic, algebra, comparison, geometry, trigonometry, probability, statistics, matrix, vector, and related transformations.
- `compression`, `crypto`
  Owns algorithmic families where encoding, integrity, secrecy, or compactness is the feature.
- `async`, `threading`
  Owns executors, futures, channels, mutexes, threads, and pools.
- `kernel`, `ffi`
  Owns platform-facing integration, process control, device/network/scheduler surfaces, and ABI bridges.

## Complete Example

```vit
space demo/report

form Report {
  name: string
  root: string
  targets: int
}

pick Outcome {
  case Ok(summary: string)
  case Err(code: int)
}

proc normalize_root(root: string) -> string {
  if root == "" { give "." }
  give root
}

proc validate_report(r: Report) -> Outcome {
  if r.name == "" { give Outcome.Err(11) }
  if r.targets <= 0 { give Outcome.Err(12) }
  give Outcome.Ok(r.name)
}

proc render_report(r: Report) -> string {
  let root: string = normalize_root(r.root)
  give "{name=" + r.name + ",root=" + root + ",targets=1}"
}

proc build_summary(r: Report) -> Outcome {
  match validate_report(r) {
    case Ok(_) { give Outcome.Ok(render_report(r)) }
    case Err(code) { give Outcome.Err(code) }
    otherwise { give Outcome.Err(70) }
  }
}
```

This example is intentionally complete enough to show the whole flow:

- domain data enters through `Report`
- normalization is kept separate from validation
- presentation is separated from acceptance
- transport/error projection is not mixed with business rules

In a fuller implementation of the same example:

- `path` would own path normalization and traversal
- `json` would own structured export
- `io` would own file emission
- `collections` would own grouped reports and indexes
- `async` would own concurrent report generation if needed

## Documentation Rule

Each stdlib family should explain:

- its responsibility
- its main submodules
- its public failure boundary
- where it plugs into a complete program

Minimal snippets are acceptable only as lower bounds. They are not sufficient to explain a library family on their own.
