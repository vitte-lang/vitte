#!/usr/bin/env python3

from __future__ import annotations

import hashlib
import json
import platform
import re
import sys
from dataclasses import asdict, dataclass
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]

REPORT_DIR = ROOT / "target" / "reports"
STDLIB_DIR = ROOT / "target" / "stdlib"
SOURCE_STDLIB_DIR = ROOT / "src/vitte/stdlib"
HISTORY_DIR = STDLIB_DIR / "history"
ARCHITECTURE_MANIFEST = SOURCE_STDLIB_DIR / "stdlib_architecture.json"
MODULE_MANIFEST = SOURCE_STDLIB_DIR / "stdlib_modules.json"
DEPENDENCY_GRAPH_MANIFEST = SOURCE_STDLIB_DIR / "stdlib_dependency_graph.json"
CORE_PRIMITIVE_SOURCE = SOURCE_STDLIB_DIR / "core" / "primitive.vitl"
CORE_PRIMITIVE_DOC = ROOT / "docs" / "compiler" / "stdlib_core_primitive.md"
CORE_OPTION_SOURCE = SOURCE_STDLIB_DIR / "core" / "option.vitl"
CORE_RESULT_SOURCE = SOURCE_STDLIB_DIR / "core" / "result.vitl"
CORE_OPTION_RESULT_DOC = ROOT / "docs" / "compiler" / "stdlib_core_option_result.md"

REPORT_DIR.mkdir(parents=True, exist_ok=True)
STDLIB_DIR.mkdir(parents=True, exist_ok=True)
HISTORY_DIR.mkdir(parents=True, exist_ok=True)

JSON_REPORT = REPORT_DIR / "stdlib_validation.json"
MARKDOWN_REPORT = REPORT_DIR / "stdlib_validation.md"
HTML_REPORT = REPORT_DIR / "stdlib_validation.html"


REQUIRED_FILES = [
    ROOT / "src/vitte/stdlib/mod.vit",
    ROOT / "src/vitte/stdlib/tests/smoke.vit",
    ARCHITECTURE_MANIFEST,
    MODULE_MANIFEST,
    DEPENDENCY_GRAPH_MANIFEST,
    CORE_PRIMITIVE_SOURCE,
    CORE_PRIMITIVE_DOC,
    CORE_OPTION_SOURCE,
    CORE_RESULT_SOURCE,
    CORE_OPTION_RESULT_DOC,
]


REQUIRED_SYMBOLS = [
    "vector_new",
    "vector_push",
    "vector_pop",
    "vector_len",
    "hashmap_new",
    "hashmap_insert",
    "hashmap_get",
    "hashmap_remove",
    "io_read_file",
    "io_write_file",
]


OPTIONAL_SYMBOLS = [
    "async_spawn",
    "async_sleep",
    "channel_send",
    "channel_recv",
    "ffi_call",
    "http_get",
    "http_post",
    "json_parse",
    "json_stringify",
    "regex_match",
    "regex_replace",
]


FEATURE_MODULES = {
    "collections": "collections.vitl",
    "vector": "collections/vector.vitl",
    "hashmap": "collections/hashmap.vitl",
    "hashset": "collections/hashset.vitl",
    "deque": "collections/deque.vitl",
    "io": "io.vitl",
    "filesystem": "io/file.vitl",
    "async": "async.vitl",
    "channels": "async/channel.vitl",
    "ffi": "ffi/ffi.vitl",
    "network": "network.vitl",
    "http": "network/http.vitl",
    "json": "json.vitl",
    "yaml": "yaml.vitl",
    "toml": "toml.vitl",
    "regex": "regex.vitl",
    "crypto": "crypto.vitl",
    "compression": "compression.vitl",
    "image": "image.vitl",
    "audio": "audio.vitl",
}


USE_RE = re.compile(r"^\s*use\s+([A-Za-z0-9_/-]+)")
WILDCARD_REEXPORT_RE = re.compile(r"^\s*(?:export|share)\s+(?:\*|all)\s*$")
CORE_FORBIDDEN_IMPORT_PREFIXES = (
    "vitte/stdlib/io",
    "vitte/stdlib/os",
    "vitte/stdlib/kernel",
    "vitte/stdlib/network",
    "vitte/stdlib/threading",
    "vitte/stdlib/ffi",
    "vitte/stdlib/platform",
)
CORE_DYNAMIC_ALLOCATION_PATTERNS = (
    r"\balloc\s*\(",
    r"\bmalloc\s*\(",
    r"\bnew\s*\(",
    r"\bVec\s*<",
    r"\bHashMap\s*<",
    r"\bdynamic_alloc",
)
REQUIRED_PRIMITIVE_FRAGMENTS = (
    "PrimitiveKind.Bool",
    "PrimitiveKind.Byte",
    "PrimitiveKind.Char",
    "PrimitiveKind.Rune",
    "PrimitiveKind.Int",
    "PrimitiveKind.Uint",
    "PrimitiveKind.Isize",
    "PrimitiveKind.Usize",
    "PrimitiveKind.I8",
    "PrimitiveKind.I16",
    "PrimitiveKind.I32",
    "PrimitiveKind.I64",
    "PrimitiveKind.I128",
    "PrimitiveKind.U8",
    "PrimitiveKind.U16",
    "PrimitiveKind.U32",
    "PrimitiveKind.U64",
    "PrimitiveKind.U128",
    "PrimitiveKind.F16",
    "PrimitiveKind.F32",
    "PrimitiveKind.F64",
    "PrimitiveKind.F128",
    "PrimitiveKind.Never",
    "PrimitiveKind.Unit",
    "PrimitiveKind.Tuple",
    "PrimitiveKind.FixedArray",
    "PrimitiveKind.Slice",
    "PrimitiveKind.Pointer",
    "PrimitiveKind.Reference",
    "PrimitiveKind.FunctionPointer",
    "PRIMITIVE_F16_BACKEND_FEATURE",
    "PRIMITIVE_F128_BACKEND_FEATURE",
)
REQUIRED_OPTION_FRAGMENTS = (
    "form Option<T>",
    "proc some<T>",
    "proc none<T>",
    "proc is_some<T>",
    "proc is_none<T>",
    "proc unwrap<T>",
    "proc expect<T>",
    "proc unwrap_or<T>",
    "proc unwrap_or_else<T>",
    "proc map<T, U>",
    "proc map_or<T, U>",
    "proc map_or_else<T, U>",
    "proc and<T, U>",
    "proc and_then<T, U>",
    "proc or<T>",
    "proc or_else<T>",
    "proc xor<T>",
    "proc filter<T>",
    "proc take<T>",
    "proc replace<T>",
    "proc insert<T>",
    "proc get_or_insert<T>",
    "proc get_or_insert_with<T>",
    "proc as_ref<T>",
    "proc as_mut<T>",
    "proc flatten<T>",
    "proc transpose<T, E>",
    "form OptionIterator<T>",
    "proc option_iter<T>",
    "proc option_iter_next<T>",
)
REQUIRED_RESULT_FRAGMENTS = (
    "form Result<T, E>",
    "proc ok<T, E>",
    "proc err<T, E>",
    "proc is_ok<T, E>",
    "proc is_err<T, E>",
    "proc unwrap<T, E>",
    "proc unwrap_err<T, E>",
    "proc expect<T, E>",
    "proc expect_err<T, E>",
    "proc map<T, U, E>",
    "proc map_err<T, E, F>",
    "proc map_or<T, U, E>",
    "proc map_or_else<T, U, E>",
    "proc and<T, U, E>",
    "proc and_then<T, U, E>",
    "proc or<T, E, F>",
    "proc or_else<T, E, F>",
    "proc unwrap_or<T, E>",
    "proc unwrap_or_else<T, E>",
    "proc as_ref<T, E>",
    "proc as_mut<T, E>",
    "proc flatten<T, E>",
    "proc transpose<T, E>",
    "form ResultIterator<T, E>",
    "proc result_iter<T, E>",
    "proc result_iter_next<T, E>",
    "proc propagate<T, E>",
    "proc try_result<T, E>",
)


@dataclass
class ValidationResult:
    name: str
    status: bool
    detail: str


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()

    with path.open("rb") as f:
        while chunk := f.read(1024 * 1024):
            h.update(chunk)

    return h.hexdigest()


def validate_files() -> list[ValidationResult]:
    results = []

    for file in REQUIRED_FILES:
        results.append(
            ValidationResult(
                name=str(file.relative_to(ROOT)),
                status=file.exists(),
                detail=(
                    sha256_file(file)
                    if file.exists()
                    else "missing"
                ),
            )
        )

    return results


def stdlib_symbol_index() -> dict[str, str]:
    symbols: dict[str, str] = {}
    declaration = re.compile(r"^\s*proc\s+([A-Za-z_][A-Za-z0-9_]*)\b")
    sources = sorted(SOURCE_STDLIB_DIR.glob("**/*.vit*"))

    for source_path in sources:
        for line in source_path.read_text(encoding="utf-8", errors="ignore").splitlines():
            match = declaration.match(line)
            if match and match.group(1) not in symbols:
                symbols[match.group(1)] = str(source_path.relative_to(ROOT))

    return symbols


def validate_required_symbols(
    symbols: dict[str, str],
) -> list[ValidationResult]:

    out = []

    for symbol in REQUIRED_SYMBOLS:
        out.append(
            ValidationResult(
                name=symbol,
                status=symbol in symbols,
                detail=symbols.get(symbol, "missing declaration"),
            )
        )

    return out


def validate_optional_symbols(
    symbols: dict[str, str],
) -> list[ValidationResult]:

    out = []

    for symbol in OPTIONAL_SYMBOLS:
        out.append(
            ValidationResult(
                name=symbol,
                status=symbol in symbols,
                detail=symbols.get(symbol, "missing declaration"),
            )
        )

    return out


def detected_features() -> dict[str, bool]:
    return {
        name: (SOURCE_STDLIB_DIR / relative_path).is_file()
        for name, relative_path in FEATURE_MODULES.items()
    }


def load_architecture_manifest() -> dict:
    return json.loads(
        ARCHITECTURE_MANIFEST.read_text(encoding="utf-8")
    )


def load_module_manifest() -> dict:
    return json.loads(
        MODULE_MANIFEST.read_text(encoding="utf-8")
    )


def load_dependency_graph_manifest() -> dict:
    return json.loads(
        DEPENDENCY_GRAPH_MANIFEST.read_text(encoding="utf-8")
    )


def stdlib_sources() -> list[Path]:
    return sorted(SOURCE_STDLIB_DIR.glob("**/*.vit*"))


def architecture_levels(manifest: dict) -> dict[str, dict]:
    return {
        level["name"]: level
        for level in manifest["levels"]
    }


def path_level(path: Path, manifest: dict) -> str:
    relative = path.relative_to(ROOT).as_posix()
    best_name = ""
    best_len = -1
    for level in manifest["levels"]:
        candidates = [level["root"]] + level.get("facades", [])
        for candidate in candidates:
            if relative == candidate or relative.startswith(candidate.rstrip("/") + "/"):
                if len(candidate) > best_len:
                    best_name = level["name"]
                    best_len = len(candidate)
    return best_name


def module_to_path(module: str) -> Path | None:
    if not module.startswith("vitte/stdlib"):
        return None
    suffix = module.removeprefix("vitte/stdlib").strip("/")
    if suffix == "":
        return SOURCE_STDLIB_DIR / "mod.vit"
    candidates = [
        SOURCE_STDLIB_DIR / f"{suffix}.vitl",
        SOURCE_STDLIB_DIR / f"{suffix}.vit",
        SOURCE_STDLIB_DIR / suffix / "mod.vit",
        SOURCE_STDLIB_DIR / suffix / f"{Path(suffix).name}.vitl",
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    return None


def imports_for_source(path: Path) -> list[str]:
    imports: list[str] = []
    for line in path.read_text(encoding="utf-8", errors="ignore").splitlines():
        match = USE_RE.match(line)
        if match:
            imports.append(match.group(1))
    return imports


def stdlib_dependency_edges(manifest: dict) -> list[tuple[str, str, str]]:
    edges: list[tuple[str, str, str]] = []
    for source in stdlib_sources():
        source_level = path_level(source, manifest)
        if not source_level:
            continue
        for imported in imports_for_source(source):
            imported_path = module_to_path(imported)
            if imported_path is None:
                continue
            imported_level = path_level(imported_path, manifest)
            if imported_level:
                edges.append((source_level, imported_level, source.relative_to(ROOT).as_posix()))
    return edges


def has_cycle(edges: list[tuple[str, str, str]]) -> bool:
    graph: dict[str, set[str]] = {}
    for left, right, _path in edges:
        if left == right:
            continue
        graph.setdefault(left, set()).add(right)
    visiting: set[str] = set()
    visited: set[str] = set()

    def visit(node: str) -> bool:
        if node in visiting:
            return True
        if node in visited:
            return False
        visiting.add(node)
        for child in graph.get(node, set()):
            if visit(child):
                return True
        visiting.remove(node)
        visited.add(node)
        return False

    return any(visit(node) for node in graph)


def validate_module_manifest(manifest: dict) -> list[ValidationResult]:
    results: list[ValidationResult] = []
    required_directories = manifest.get("required_directories", [])
    missing_required = [
        name
        for name in required_directories
        if not (SOURCE_STDLIB_DIR / name).is_dir()
    ]
    results.append(ValidationResult(
        name="required_stdlib_directories",
        status=not missing_required,
        detail="ok" if not missing_required else ", ".join(missing_required),
    ))

    module_directories = manifest.get("module_directories", [])
    missing_indexes = [
        name
        for name in module_directories
        if not (SOURCE_STDLIB_DIR / name / "index.vit").is_file()
    ]
    results.append(ValidationResult(
        name="index_required_for_each_module_directory",
        status=not missing_indexes,
        detail="ok" if not missing_indexes else ", ".join(missing_indexes),
    ))

    official = set(manifest.get("official_entrypoints", []))
    uncontrolled: list[str] = []
    for source in stdlib_sources():
        relative = source.relative_to(ROOT).as_posix()
        for line in source.read_text(encoding="utf-8", errors="ignore").splitlines():
            if WILDCARD_REEXPORT_RE.match(line) and relative not in official:
                uncontrolled.append(relative)
                break
    results.append(ValidationResult(
        name="wildcard_reexports_only_in_official_entrypoints",
        status=not uncontrolled,
        detail="ok" if not uncontrolled else "; ".join(uncontrolled[:8]),
    ))

    stale_entrypoints = [
        path
        for path in official
        if not (ROOT / path).is_file()
    ]
    results.append(ValidationResult(
        name="official_entrypoints_exist",
        status=not stale_entrypoints,
        detail="ok" if not stale_entrypoints else "; ".join(stale_entrypoints[:8]),
    ))
    return results


def validate_dependency_graph_manifest(manifest: dict, edges: list[tuple[str, str, str]]) -> list[ValidationResult]:
    results: list[ValidationResult] = []
    declared = {
        (edge["from"], edge["to"])
        for edge in manifest.get("edges", [])
    }
    undeclared = [
        f"{path}: {left} -> {right}"
        for left, right, path in edges
        if left != right and (left, right) not in declared
    ]
    results.append(ValidationResult(
        name="official_dependency_graph_edges",
        status=not undeclared,
        detail="ok" if not undeclared else "; ".join(undeclared[:8]),
    ))
    results.append(ValidationResult(
        name="official_dependency_graph_acyclic",
        status=not has_cycle([(left, right, path) for left, right, path in edges if left != right]),
        detail=f"declared_edges={len(declared)}",
    ))
    return results


def validate_core_primitives() -> list[ValidationResult]:
    source = CORE_PRIMITIVE_SOURCE.read_text(encoding="utf-8")
    doc = CORE_PRIMITIVE_DOC.read_text(encoding="utf-8")
    missing_fragments = [
        fragment
        for fragment in REQUIRED_PRIMITIVE_FRAGMENTS
        if fragment not in source
    ]
    required_doc_fragments = (
        "`bool`",
        "`byte`",
        "`char`",
        "`rune`",
        "`int`",
        "`uint`",
        "`isize`",
        "`usize`",
        "`i128`",
        "`u128`",
        "`f16`",
        "`f128`",
        "`never`",
        "`unit`",
        "Tuples",
        "Fixed arrays",
        "Slices",
        "pointer",
        "Architecture-Dependent Types",
    )
    missing_doc = [
        fragment
        for fragment in required_doc_fragments
        if fragment not in doc
    ]
    return [
        ValidationResult(
            name="core_primitive_defines_fundamental_types",
            status=not missing_fragments,
            detail="ok" if not missing_fragments else ", ".join(missing_fragments),
        ),
        ValidationResult(
            name="core_primitive_documents_layouts",
            status=not missing_doc,
            detail="ok" if not missing_doc else ", ".join(missing_doc),
        ),
    ]


def validate_core_option_result() -> list[ValidationResult]:
    option_source = CORE_OPTION_SOURCE.read_text(encoding="utf-8")
    result_source = CORE_RESULT_SOURCE.read_text(encoding="utf-8")
    doc = CORE_OPTION_RESULT_DOC.read_text(encoding="utf-8")
    missing_option = [
        fragment
        for fragment in REQUIRED_OPTION_FRAGMENTS
        if fragment not in option_source
    ]
    missing_result = [
        fragment
        for fragment in REQUIRED_RESULT_FRAGMENTS
        if fragment not in result_source
    ]
    required_doc_fragments = (
        "Option<T>",
        "Result<T, E>",
        "some",
        "none",
        "ok",
        "err",
        "propagate(result)",
        "try_result(result)",
        "Error Propagation",
    )
    missing_doc = [
        fragment
        for fragment in required_doc_fragments
        if fragment not in doc
    ]
    return [
        ValidationResult(
            name="core_option_defines_full_api",
            status=not missing_option,
            detail="ok" if not missing_option else ", ".join(missing_option),
        ),
        ValidationResult(
            name="core_result_defines_full_api",
            status=not missing_result,
            detail="ok" if not missing_result else ", ".join(missing_result),
        ),
        ValidationResult(
            name="core_option_result_documents_api",
            status=not missing_doc,
            detail="ok" if not missing_doc else ", ".join(missing_doc),
        ),
    ]


def validate_architecture(manifest: dict) -> list[ValidationResult]:
    results: list[ValidationResult] = []
    levels = architecture_levels(manifest)
    required_names = ["core", "alloc", "std", "platform", "experimental"]
    results.append(ValidationResult(
        name="stdlib_levels",
        status=list(levels.keys()) == required_names,
        detail=" -> ".join(levels.keys()),
    ))
    results.append(ValidationResult(
        name="maximum_additional_libraries",
        status=manifest.get("maximum_additional_libraries") == 0,
        detail=str(manifest.get("maximum_additional_libraries")),
    ))

    ranks = {name: int(level["rank"]) for name, level in levels.items()}
    edges = stdlib_dependency_edges(manifest)
    upward_edges = [
        f"{path}: {left} -> {right}"
        for left, right, path in edges
        if ranks[right] > ranks[left]
    ]
    results.append(ValidationResult(
        name="strict_dependency_hierarchy",
        status=not upward_edges,
        detail="ok" if not upward_edges else "; ".join(upward_edges[:5]),
    ))
    results.append(ValidationResult(
        name="no_circular_dependencies",
        status=not has_cycle(edges),
        detail=f"edges={len(edges)}",
    ))

    core_sources = [
        source
        for source in stdlib_sources()
        if path_level(source, manifest) == "core"
    ]
    core_os_violations: list[str] = []
    core_alloc_violations: list[str] = []
    for source in core_sources:
        text = source.read_text(encoding="utf-8", errors="ignore")
        for imported in imports_for_source(source):
            if imported.startswith(CORE_FORBIDDEN_IMPORT_PREFIXES):
                core_os_violations.append(f"{source.relative_to(ROOT)} imports {imported}")
        for pattern in CORE_DYNAMIC_ALLOCATION_PATTERNS:
            if re.search(pattern, text):
                core_alloc_violations.append(f"{source.relative_to(ROOT)} matches {pattern}")

    results.append(ValidationResult(
        name="core_has_no_operating_system_dependency",
        status=not core_os_violations,
        detail="ok" if not core_os_violations else "; ".join(core_os_violations[:5]),
    ))
    results.append(ValidationResult(
        name="core_has_no_dynamic_allocation",
        status=not core_alloc_violations,
        detail="ok" if not core_alloc_violations else "; ".join(core_alloc_violations[:5]),
    ))
    results.append(ValidationResult(
        name="stdlib_is_compiled_from_vitte_sources",
        status=all(path.suffix in {".vit", ".vitl"} for path in stdlib_sources()),
        detail=f"sources={len(stdlib_sources())}",
    ))
    return results


def feature_score(features: dict[str, bool]) -> float:
    enabled = sum(
        1
        for value in features.values()
        if value
    )

    return round(
        enabled * 100.0
        / len(features),
        2,
    )


def build_report() -> dict:
    symbols = stdlib_symbol_index()
    features = detected_features()
    architecture_manifest = load_architecture_manifest()
    module_manifest = load_module_manifest()
    dependency_graph_manifest = load_dependency_graph_manifest()

    files = validate_files()
    architecture = validate_architecture(architecture_manifest)
    module_results = validate_module_manifest(module_manifest)
    graph_results = validate_dependency_graph_manifest(
        dependency_graph_manifest,
        stdlib_dependency_edges(architecture_manifest),
    )
    primitive_results = validate_core_primitives()
    option_result_results = validate_core_option_result()

    required = validate_required_symbols(
        symbols
    )

    optional = validate_optional_symbols(
        symbols
    )

    missing_files = [
        item
        for item in files
        if not item.status
    ]

    missing_symbols = [
        item
        for item in required
        if not item.status
    ]
    architecture_failures = [
        item
        for item in architecture + module_results + graph_results + primitive_results + option_result_results
        if not item.status
    ]

    status = (
        "PASS"
        if not missing_files
        and not missing_symbols
        and not architecture_failures
        else "FAIL"
    )

    return {
        "version": "v3",
        "generated_at":
            datetime.now(
                timezone.utc
            ).isoformat(),
        "host": {
            "platform":
                platform.platform(),
            "machine":
                platform.machine(),
            "python":
                platform.python_version(),
        },
        "status": status,
        "coverage_score":
            feature_score(features),
        "features":
            features,
        "architecture":
            architecture_manifest,
        "module_manifest":
            module_manifest,
        "dependency_graph":
            dependency_graph_manifest,
        "required_files": [
            asdict(item)
            for item in files
        ],
        "architecture_results": [
            asdict(item)
            for item in architecture + module_results + graph_results + primitive_results + option_result_results
        ],
        "required_symbols": [
            asdict(item)
            for item in required
        ],
        "optional_symbols": [
            asdict(item)
            for item in optional
        ],
        "statistics": {
            "required_files":
                len(files),
            "required_symbols":
                len(required),
            "optional_symbols":
                len(optional),
            "missing_files":
                len(missing_files),
            "missing_symbols":
                len(missing_symbols),
            "architecture_failures":
                len(architecture_failures),
        },
    }


def write_json(report: dict):
    JSON_REPORT.write_text(
        json.dumps(
            report,
            indent=2,
            ensure_ascii=False,
        ) + "\n",
        encoding="utf-8",
    )


def write_markdown(report: dict):
    lines = [
        "# Standard Library Validation",
        "",
        f"Status: **{report['status']}**",
        "",
        f"Coverage: {report['coverage_score']}%",
        "",
        "## Required Files",
        "",
        "| File | Status |",
        "|------|------|",
    ]

    for item in report["required_files"]:
        lines.append(
            f"| {item['name']} | "
            f"{'PASS' if item['status'] else 'FAIL'} |"
        )

    lines.extend([
        "",
        "## Required Symbols",
        "",
        "| Symbol | Status |",
        "|------|------|",
    ])

    for item in report["required_symbols"]:
        lines.append(
            f"| {item['name']} | "
            f"{'PASS' if item['status'] else 'FAIL'} |"
        )

    lines.extend([
        "",
        "## Architecture",
        "",
        "| Contract | Status | Detail |",
        "|------|------|------|",
    ])

    for item in report["architecture_results"]:
        lines.append(
            f"| {item['name']} | "
            f"{'PASS' if item['status'] else 'FAIL'} | "
            f"{item['detail']} |"
        )

    MARKDOWN_REPORT.write_text(
        "\n".join(lines) + "\n",
        encoding="utf-8",
    )


def write_html(report: dict):
    rows = []

    for item in report["required_symbols"]:
        rows.append(
            f"<tr>"
            f"<td>{item['name']}</td>"
            f"<td>{'PASS' if item['status'] else 'FAIL'}</td>"
            f"</tr>"
        )

    architecture_rows = []

    for item in report["architecture_results"]:
        architecture_rows.append(
            f"<tr>"
            f"<td>{item['name']}</td>"
            f"<td>{'PASS' if item['status'] else 'FAIL'}</td>"
            f"<td>{item['detail']}</td>"
            f"</tr>"
        )

    html = f"""
<!doctype html>
<html>
<head>
<meta charset="utf-8">
<title>Vitte Stdlib Validation</title>
</head>
<body>

<h1>Vitte Standard Library Validation</h1>

<p>Status: {report["status"]}</p>

<p>Coverage: {report["coverage_score"]}%</p>

<table border="1">
<tr>
<th>Symbol</th>
<th>Status</th>
</tr>

{''.join(rows)}

</table>

<h2>Architecture</h2>

<table border="1">
<tr>
<th>Contract</th>
<th>Status</th>
<th>Detail</th>
</tr>

{''.join(architecture_rows)}

</table>

</body>
</html>
"""

    HTML_REPORT.write_text(
        html,
        encoding="utf-8",
    )


def save_history(report: dict):
    stamp = datetime.now(
        timezone.utc
    ).strftime(
        "%Y%m%dT%H%M%SZ"
    )

    history = (
        HISTORY_DIR
        / f"stdlib_validation_{stamp}.json"
    )

    history.write_text(
        json.dumps(
            report,
            indent=2,
        ) + "\n",
        encoding="utf-8",
    )


def main() -> int:
    report = build_report()

    write_json(report)
    write_markdown(report)
    write_html(report)

    save_history(report)

    print(
        f"[stdlib] status: "
        f"{report['status']}"
    )

    print(
        f"[stdlib] coverage: "
        f"{report['coverage_score']}%"
    )

    print(
        f"[stdlib] json: "
        f"{JSON_REPORT}"
    )

    if report["status"] != "PASS":
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
