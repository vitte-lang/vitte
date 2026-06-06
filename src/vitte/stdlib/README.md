# stdlib

Path: `src/vitte/stdlib`

## Purpose

Top-level map of the Vitte standard library and the responsibilities owned by each family.

## Architecture Role

A realistic Vitte program usually starts in `core`, grows through `collections` or `data`, crosses textual boundaries with `json` or `encoding`, touches the host with `path` or `io`, and only then reaches system-facing families like `kernel`, `ffi`, `async`, or `threading`.

## Main Responsibilities

- Give maintainers a stable mental model of the whole library surface.
- Separate pure transformations from host-facing and runtime-facing boundaries.
- Make it easy to explain where a new helper belongs before adding code.

## Module Inventory

- `GETTING_STARTED.vitl`
- `collections.vitl`
- `compression.vitl`
- `core.vitl`
- `core_alias.vitl`
- `crypto.vitl`
- `datetime.vitl`
- `encoding.vitl`
- `graphics.vitl`
- `io.vitl`
- `json.vitl`
- `kernel.vitl`
- `math.vitl`
- `memory.vitl`
- `mod.vit`
- `os.vitl`
- `path.vitl`
- `regex.vitl`
- `runtime.vitl`
- `strings.vitl`
- `sysinfo.vitl`
- `async/async.vitl`
- `async/channel.vitl`
- `async/executor.vitl`
- `async/future.vitl`
- `collections/collections.vitl`
- `collections/deque.vitl`
- `collections/graph.vitl`
- `collections/hashmap.vitl`
- `collections/hashset.vitl`
- `collections/linkedlist.vitl`
- `collections/matrix.vitl`
- `collections/queue.vitl`
- `collections/stack.vitl`
- `collections/vector.vitl`
- `compiler/mod.vit`
- `compression/algorithms.vitl`
- `compression/brotli.vitl`
- `compression/deflate.vitl`
- `compression/huffman.vitl`
- `compression/interface.vitl`
- `compression/lz.vitl`
- `compression/stats.vitl`
- `core/algorithms.vitl`
- `core/concurrency.vitl`
- `core/io_helpers.vitl`
- `core/memory.vitl`
- `core/panic.vitl`
- `core/strings.vitl`
- `core/types.vitl`
- `core/utils.vitl`
- `crypto/asymmetric.vitl`
- `crypto/hash.vitl`
- `crypto/hashing.vitl`
- `crypto/hmac.vitl`
- `crypto/keyderivation.vitl`
- `crypto/random.vitl`
- `crypto/symmetric.vitl`
- `crypto/utils.vitl`
- `data/cleaning.vitl`
- `data/data.vitl`
- `data/dataset.vitl`
- `data/merge.vitl`
- `data/schema.vitl`
- `data/stats.vitl`
- `data/transform.vitl`
- `encoding/base64.vitl`
- `encoding/hex.vitl`
- `encoding/html.vitl`
- `encoding/legacy.vitl`
- `encoding/unicode.vitl`
- `encoding/url.vitl`
- `encoding/utf.vitl`
- `encoding/utf8.vitl`
- `ffi/abi.vitl`
- `ffi/ffi.vitl`
- `io/buffer.vitl`
- `io/file.vitl`
- `io/fileops.vitl`
- `io/host_runtime.vitl`
- `io/io.vitl`
- `io/stdio.vitl`
- `io/stream.vitl`
- `json/builder.vitl`
- `json/parse.vitl`
- `json/parser.vitl`
- `json/schema.vitl`
- `json/serialize.vitl`
- `json/stringify.vitl`
- `json/types.vitl`
- `kernel/device.vitl`
- `kernel/fileio.vitl`
- `kernel/interrupt.vitl`
- `kernel/memory.vitl`
- `kernel/network.vitl`
- `kernel/process.vitl`
- `kernel/scheduler.vitl`
- `kernel/signals.vitl`
- `kernel/sync.vitl`
- `kernel/threads.vitl`
- `kernel/users.vitl`
- `math/algebra.vitl`
- `math/arithmetic.vitl`
- `math/arrays.vitl`
- `math/calculus.vitl`
- `math/comparison.vitl`
- `math/complex.vitl`
- `math/geometry.vitl`
- `math/logic.vitl`
- `math/matrix.vitl`
- `math/modular.vitl`
- `math/number_theory.vitl`
- `math/powers.vitl`
- `math/probability.vitl`
- `math/roots.vitl`
- `math/sequences.vitl`
- `math/sort.vitl`
- `math/statistics.vitl`
- `math/topology.vitl`
- `math/trigonometry.vitl`
- `math/vector.vitl`
- `path/globbing.vitl`
- `path/manipulation.vitl`
- `path/special.vitl`
- `path/walker.vitl`
- `tests/smoke.vit`
- `threading/mutex.vitl`
- `threading/thread.vitl`
- `threading/threadpool.vitl`

## Complete Integration Story

- Domain values start in `core` and `strings`.
- Grouped data moves through `collections` or `data`.
- Structured export goes through `json` and `encoding`.
- Filesystem or process interaction goes through `path`, `io`, `os`, or `sysinfo`.
- Explicit runtime coordination goes through `async`, `threading`, `kernel`, or `ffi`.

## Documentation Rule

- Explain this family by responsibility first.
- Use complete scenarios, not only tiny snippets.
- Separate pure transformation, host interaction, and runtime boundaries.
- Keep failure paths explicit in examples and contracts.
