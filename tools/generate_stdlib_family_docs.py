#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
STDLIB = ROOT / "src" / "vitte" / "stdlib"


FAMILY_INFO = {
    "root": {
        "title": "stdlib",
        "purpose": "Top-level map of the Vitte standard library and the responsibilities owned by each family.",
        "story": (
            "A realistic Vitte program usually starts in `core`, grows through `collections` "
            "or `data`, crosses textual boundaries with `json` or `encoding`, touches the "
            "host with `path` or `io`, and only then reaches system-facing families like "
            "`kernel`, `ffi`, `async`, or `threading`."
        ),
        "responsibilities": [
            "Give maintainers a stable mental model of the whole library surface.",
            "Separate pure transformations from host-facing and runtime-facing boundaries.",
            "Make it easy to explain where a new helper belongs before adding code.",
        ],
        "example": [
            "Domain values start in `core` and `strings`.",
            "Grouped data moves through `collections` or `data`.",
            "Structured export goes through `json` and `encoding`.",
            "Filesystem or process interaction goes through `path`, `io`, `os`, or `sysinfo`.",
            "Explicit runtime coordination goes through `async`, `threading`, `kernel`, or `ffi`.",
        ],
    },
    "core": {
        "title": "core",
        "purpose": "Portable low-level building blocks: types, strings, memory helpers, panic/runtime-adjacent basics, and reusable utility routines.",
        "story": (
            "Use `core` when the code should remain portable and unsurprising. It is the family "
            "you reach for before involving the filesystem, network, process table, or threading runtime."
        ),
        "responsibilities": [
            "Own scalar and foundational helper surfaces.",
            "Expose utilities that higher-level families can safely compose.",
            "Stay small, explicit, and stable because most of the tree depends on it.",
        ],
        "example": [
            "A manifest validator stores names and counters with `core` types.",
            "A pure helper normalizes a string or integer without touching host state.",
            "The same helper can be reused in compiler code, stdlib code, and user code.",
        ],
    },
    "collections": {
        "title": "collections",
        "purpose": "Container and traversal surfaces such as vector, deque, queue, stack, linked list, hashmap, hashset, graph, and matrix.",
        "story": (
            "Use `collections` when the shape of data matters more than the host system. "
            "This family owns grouping, ordering, indexing, and traversal concerns."
        ),
        "responsibilities": [
            "Offer explicit data-structure choices for program state.",
            "Keep container behavior separated from business rules.",
            "Support larger flows where data shape and access cost both matter.",
        ],
        "example": [
            "A build report groups diagnostics in a vector and indexes them in a hashmap.",
            "A scheduler stores pending work in a queue or deque.",
            "A graph or matrix page should explain why those shapes exist, not just list filenames.",
        ],
    },
    "compiler": {
        "title": "compiler",
        "purpose": "Compiler-owned stdlib surfaces that support self-hosted compiler and driver integration.",
        "story": (
            "This family is not general-purpose business code. It exists to support compiler-owned "
            "flows where the compiler and stdlib need a shared contract."
        ),
        "responsibilities": [
            "Keep compiler-facing library helpers clearly scoped.",
            "Avoid leaking compiler-specific surfaces into the general-purpose families.",
            "Document any ownership boundary between compiler internals and shared helpers.",
        ],
        "example": [
            "A self-hosted compiler flow can reuse structured helpers without pretending they are general stdlib entry points.",
            "A driver surface can depend on this family while still keeping its public contract documented elsewhere.",
        ],
    },
    "compression": {
        "title": "compression",
        "purpose": "Algorithms and interfaces for compacting data: huffman, lz, deflate, brotli, stats, and shared compression interfaces.",
        "story": (
            "Use `compression` when compactness is a first-class requirement and the program must "
            "explain which algorithmic boundary owns that transformation."
        ),
        "responsibilities": [
            "Keep compression algorithms explicit and isolated from business code.",
            "Document algorithm families and shared interfaces in one place.",
            "Clarify tradeoffs between compactness, speed, and complexity.",
        ],
        "example": [
            "A report archive can be built in memory, then compressed before emission.",
            "A transport layer can separate serialization from compression instead of mixing both in one procedure.",
        ],
    },
    "crypto": {
        "title": "crypto",
        "purpose": "Hashing, HMAC, randomness, key derivation, symmetric primitives, and asymmetric primitives.",
        "story": (
            "Use `crypto` when integrity, secrecy, or key management is the feature. "
            "This family should never be presented as generic formatting or utility code."
        ),
        "responsibilities": [
            "Own cryptographic transformations and their vocabulary.",
            "Keep randomness and hashing distinct from casual helper logic.",
            "Document where security-sensitive flows begin and end.",
        ],
        "example": [
            "A package manifest can be serialized first, then hashed, then optionally signed.",
            "A token flow can derive a key in one boundary and use it in another without mixing concerns.",
        ],
    },
    "data": {
        "title": "data",
        "purpose": "Dataset-oriented helpers such as schema, transform, merge, cleaning, and statistics.",
        "story": (
            "Use `data` when the program manipulates rows, records, schemas, or staged transformations "
            "instead of one-off scalar logic."
        ),
        "responsibilities": [
            "Own data-shaping operations that are richer than basic containers.",
            "Keep schema and transform steps explicit in the docs.",
            "Clarify how datasets evolve through a pipeline.",
        ],
        "example": [
            "A telemetry report can be cleaned, merged, transformed, and summarized before export.",
            "A schema page should explain what a valid record looks like before code samples start.",
        ],
    },
    "encoding": {
        "title": "encoding",
        "purpose": "Text and byte encoding surfaces such as utf, base64, url, hex, html, legacy encodings, and unicode helpers.",
        "story": (
            "Use `encoding` when values cross a textual or byte-oriented boundary and representation matters."
        ),
        "responsibilities": [
            "Own representation conversions instead of business meaning.",
            "Keep wire/text transformations separate from structured domain logic.",
            "Explain which submodule is responsible for which external format.",
        ],
        "example": [
            "A JSON payload can be rendered first, then base64-encoded for transport.",
            "A path or URL can be normalized before joining it with host-facing code.",
        ],
    },
    "ffi": {
        "title": "ffi",
        "purpose": "ABI and foreign-function boundaries used when Vitte code must cross language or runtime edges.",
        "story": (
            "Use `ffi` only when the program really needs a foreign boundary. "
            "This family should make coupling visible instead of hiding it."
        ),
        "responsibilities": [
            "Document ABI-facing assumptions clearly.",
            "Keep unsafe or foreign boundaries distinct from pure library logic.",
            "Support interop without blurring ownership lines.",
        ],
        "example": [
            "A system integration module can expose a narrow ABI-facing wrapper while the rest of the program stays pure.",
            "A runtime boundary should say exactly when the library stops and the foreign surface begins.",
        ],
    },
    "io": {
        "title": "io",
        "purpose": "File, buffer, stream, stdio, and host-runtime access helpers.",
        "story": (
            "Use `io` when the program must read or write bytes, files, or streams. "
            "Keep it separate from validation, parsing, or business decisions."
        ),
        "responsibilities": [
            "Own file and stream movement.",
            "Clarify buffered vs direct access paths.",
            "Keep host interaction visible in the architecture.",
        ],
        "example": [
            "A manifest is loaded through `io`, parsed elsewhere, validated elsewhere, and only then emitted back through `io`.",
            "A stdio helper should explain where user-facing text enters the flow.",
        ],
    },
    "json": {
        "title": "json",
        "purpose": "Structured JSON surfaces including parse, parser, types, builder, schema, stringify, and serialize.",
        "story": (
            "Use `json` when data must cross a structured textual boundary. "
            "This family owns JSON shape and conversion, not business validation itself."
        ),
        "responsibilities": [
            "Own JSON parsing and rendering concerns.",
            "Keep structured exchange separate from file access and domain acceptance.",
            "Document the distinction between raw JSON shape and validated program state.",
        ],
        "example": [
            "A build plan can be validated as domain data first and then exported to JSON in a separate step.",
            "A parser page should say how syntax becomes JSON values before serialization details appear.",
        ],
    },
    "kernel": {
        "title": "kernel",
        "purpose": "System-facing runtime helpers such as process, scheduler, threads, sync, users, signals, network, device, and memory.",
        "story": (
            "Use `kernel` when the program explicitly models system services, scheduling, process behavior, or device-facing coordination."
        ),
        "responsibilities": [
            "Own explicit system-service and runtime control surfaces.",
            "Keep platform-facing concerns separate from pure collections or data flows.",
            "Document the cost and coupling of these interfaces.",
        ],
        "example": [
            "A service manager may use scheduler, process, and signals while keeping policy in separate code.",
            "A network-facing runtime should explain why it depends on kernel surfaces instead of lighter families.",
        ],
    },
    "math": {
        "title": "math",
        "purpose": "Arithmetic, algebra, comparison, calculus, geometry, modular arithmetic, number theory, probability, statistics, matrix, and vector helpers.",
        "story": (
            "Use `math` when the transformation itself is the feature. "
            "This family exists so algorithmic intent stays visible and testable."
        ),
        "responsibilities": [
            "Own numeric and symbolic transformations beyond one-off helpers.",
            "Group mathematical domains into explicit submodules.",
            "Let readers see what kind of computation a program is performing.",
        ],
        "example": [
            "A scoring engine can compute aggregates in `math` while keeping I/O and transport elsewhere.",
            "A statistics or matrix chapter should explain the workflow around the computation, not just a single formula.",
        ],
    },
    "path": {
        "title": "path",
        "purpose": "Path manipulation, special locations, walkers, and globbing.",
        "story": (
            "Use `path` when the program needs path semantics, traversal, or normalization. "
            "Keep it distinct from file contents and from business validation."
        ),
        "responsibilities": [
            "Own path normalization and traversal logic.",
            "Clarify special directories and matching rules.",
            "Keep host path semantics explicit in the docs.",
        ],
        "example": [
            "A project root can be normalized before any file is read.",
            "A walker can enumerate source files while parsing and diagnostics remain elsewhere.",
        ],
    },
    "tests": {
        "title": "tests",
        "purpose": "Stdlib-focused smoke and integration validation surfaces.",
        "story": (
            "This family exists to prove that the documented stdlib surfaces keep behaving as promised."
        ),
        "responsibilities": [
            "Provide stable smoke checks for the stdlib tree.",
            "Keep proof artifacts close to the library surface they exercise.",
            "Document what the tests are supposed to guarantee.",
        ],
        "example": [
            "A smoke file should prove the library can still be loaded and exercised through its intended public paths.",
        ],
    },
    "threading": {
        "title": "threading",
        "purpose": "Thread, mutex, and pool-based concurrency helpers.",
        "story": (
            "Use `threading` when the program needs explicit concurrency coordination rather than single-threaded transformation."
        ),
        "responsibilities": [
            "Own thread-based coordination surfaces.",
            "Keep shared-state synchronization visible.",
            "Document how threading differs from lighter async orchestration.",
        ],
        "example": [
            "A worker pool can process tasks in parallel while leaving task definition and result aggregation elsewhere.",
        ],
    },
    "async": {
        "title": "async",
        "purpose": "Future, channel, executor, and task orchestration helpers.",
        "story": (
            "Use `async` when work should be coordinated as tasks rather than as direct thread ownership."
        ),
        "responsibilities": [
            "Own task orchestration and message-passing surfaces.",
            "Clarify the difference between asynchronous coordination and thread control.",
            "Keep scheduling concepts visible in the docs.",
        ],
        "example": [
            "A pipeline can spawn tasks, exchange messages through channels, and join through the executor boundary.",
        ],
    },
}


def list_modules(base: Path) -> list[str]:
    modules: list[str] = []
    for path in sorted(base.glob("*.vit*")):
        modules.append(path.name)
    for path in sorted(base.iterdir()):
        if path.is_dir() and path.name != "__pycache__":
            for sub in sorted(path.glob("*.vit*")):
                modules.append(f"{path.name}/{sub.name}")
    return modules


def render_readme(target: Path, key: str) -> str:
    info = FAMILY_INFO[key]
    modules = list_modules(target.parent if target.name == "README.md" and target.parent == STDLIB else target.parent)
    if key == "root":
        modules = list_modules(STDLIB)

    lines = [
        f"# {info['title']}",
        "",
        f"Path: `{target.parent.relative_to(ROOT)}`",
        "",
        "## Purpose",
        "",
        info["purpose"],
        "",
        "## Architecture Role",
        "",
        info["story"],
        "",
        "## Main Responsibilities",
        "",
    ]
    for item in info["responsibilities"]:
        lines.append(f"- {item}")

    lines.extend([
        "",
        "## Module Inventory",
        "",
    ])
    for module in modules:
        lines.append(f"- `{module}`")

    lines.extend([
        "",
        "## Complete Integration Story",
        "",
    ])
    for item in info["example"]:
        lines.append(f"- {item}")

    lines.extend([
        "",
        "## Documentation Rule",
        "",
        "- Explain this family by responsibility first.",
        "- Use complete scenarios, not only tiny snippets.",
        "- Separate pure transformation, host interaction, and runtime boundaries.",
        "- Keep failure paths explicit in examples and contracts.",
        "",
    ])
    return "\n".join(lines)


def main() -> int:
    targets: list[tuple[str, Path]] = [("root", STDLIB / "README.md")]
    for readme in sorted(STDLIB.glob("*/README.md")):
        targets.append((readme.parent.name, readme))

    for key, readme in targets:
        if key not in FAMILY_INFO:
            continue
        readme.write_text(render_readme(readme, key), encoding="utf-8")

    print(f"generated_readmes={len(targets)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
