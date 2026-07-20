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
CI_MATRIX_MANIFEST = SOURCE_STDLIB_DIR / "stdlib_ci_matrix.json"
CORE_PRIMITIVE_SOURCE = SOURCE_STDLIB_DIR / "core" / "primitive.vitl"
CORE_PRIMITIVE_DOC = ROOT / "docs" / "compiler" / "stdlib_core_primitive.md"
CORE_OPTION_SOURCE = SOURCE_STDLIB_DIR / "core" / "option.vitl"
CORE_RESULT_SOURCE = SOURCE_STDLIB_DIR / "core" / "result.vitl"
CORE_OPTION_RESULT_DOC = ROOT / "docs" / "compiler" / "stdlib_core_option_result.md"
CORE_CONVERT_SOURCE = SOURCE_STDLIB_DIR / "core" / "convert.vitl"
CORE_DEFAULT_SOURCE = SOURCE_STDLIB_DIR / "core" / "default.vitl"
CORE_CLONE_SOURCE = SOURCE_STDLIB_DIR / "core" / "clone.vitl"
CORE_CONVERT_DEFAULT_CLONE_DOC = ROOT / "docs" / "compiler" / "stdlib_core_convert_default_clone.md"
CORE_DROP_SOURCE = SOURCE_STDLIB_DIR / "core" / "drop.vitl"
CORE_SCOPE_SOURCE = SOURCE_STDLIB_DIR / "core" / "scope.vitl"
CORE_DROP_SCOPE_MEMORY_DOC = ROOT / "docs" / "compiler" / "stdlib_core_drop_scope_memory.md"
CORE_ITERATOR_SOURCE = SOURCE_STDLIB_DIR / "core" / "iterator.vitl"
CORE_RANGE_SOURCE = SOURCE_STDLIB_DIR / "core" / "range.vitl"
CORE_NUMBER_SOURCE = SOURCE_STDLIB_DIR / "core" / "number.vitl"
CORE_FLOAT_SOURCE = SOURCE_STDLIB_DIR / "core" / "float.vitl"
CORE_MATH_SOURCE = SOURCE_STDLIB_DIR / "core" / "math.vitl"
CORE_STRING_SOURCE = SOURCE_STDLIB_DIR / "core" / "string.vitl"
CORE_ASCII_SOURCE = SOURCE_STDLIB_DIR / "core" / "ascii.vitl"
CORE_UNICODE_SOURCE = SOURCE_STDLIB_DIR / "core" / "unicode.vitl"
STDLIB_NEXT_STEP_SOURCES = (
    SOURCE_STDLIB_DIR / "core" / "slice.vitl",
    SOURCE_STDLIB_DIR / "core" / "array.vitl",
    SOURCE_STDLIB_DIR / "core" / "cmp.vitl",
    SOURCE_STDLIB_DIR / "core" / "hash.vitl",
    SOURCE_STDLIB_DIR / "alloc" / "box.vitl",
    SOURCE_STDLIB_DIR / "alloc" / "btree.vitl",
    SOURCE_STDLIB_DIR / "alloc" / "collections.vitl",
    SOURCE_STDLIB_DIR / "alloc" / "deque.vitl",
    SOURCE_STDLIB_DIR / "alloc" / "vec.vitl",
    SOURCE_STDLIB_DIR / "alloc" / "string.vitl",
    SOURCE_STDLIB_DIR / "alloc" / "smallvec.vitl",
    SOURCE_STDLIB_DIR / "alloc" / "rc.vitl",
    SOURCE_STDLIB_DIR / "alloc" / "arc.vitl",
    SOURCE_STDLIB_DIR / "std" / "io.vitl",
    SOURCE_STDLIB_DIR / "std" / "error.vitl",
    SOURCE_STDLIB_DIR / "std" / "fs.vitl",
    SOURCE_STDLIB_DIR / "std" / "env.vitl",
    SOURCE_STDLIB_DIR / "std" / "time.vitl",
    SOURCE_STDLIB_DIR / "std" / "process.vitl",
    SOURCE_STDLIB_DIR / "std" / "serialization.vitl",
    SOURCE_STDLIB_DIR / "std" / "atomic.vitl",
    SOURCE_STDLIB_DIR / "std" / "archive.vitl",
    SOURCE_STDLIB_DIR / "std" / "bitset.vitl",
    SOURCE_STDLIB_DIR / "std" / "cache.vitl",
    SOURCE_STDLIB_DIR / "std" / "checksum.vitl",
    SOURCE_STDLIB_DIR / "std" / "cron.vitl",
    SOURCE_STDLIB_DIR / "std" / "graph.vitl",
    SOURCE_STDLIB_DIR / "std" / "pool.vitl",
    SOURCE_STDLIB_DIR / "std" / "rate_limit.vitl",
    SOURCE_STDLIB_DIR / "std" / "retry.vitl",
    SOURCE_STDLIB_DIR / "std" / "tracing.vitl",
    SOURCE_STDLIB_DIR / "std" / "path.vitl",
    SOURCE_STDLIB_DIR / "std" / "thread.vitl",
    SOURCE_STDLIB_DIR / "std" / "sync.vitl",
    SOURCE_STDLIB_DIR / "std" / "net.vitl",
    SOURCE_STDLIB_DIR / "std" / "hash.vitl",
    SOURCE_STDLIB_DIR / "std" / "random.vitl",
    SOURCE_STDLIB_DIR / "std" / "format.vitl",
    SOURCE_STDLIB_DIR / "std" / "parse.vitl",
    SOURCE_STDLIB_DIR / "std" / "testing.vitl",
    SOURCE_STDLIB_DIR / "std" / "bench.vitl",
    SOURCE_STDLIB_DIR / "std" / "log.vitl",
    SOURCE_STDLIB_DIR / "std" / "cli.vitl",
    SOURCE_STDLIB_DIR / "std" / "uuid.vitl",
    SOURCE_STDLIB_DIR / "std" / "url.vitl",
    SOURCE_STDLIB_DIR / "std" / "csv.vitl",
    SOURCE_STDLIB_DIR / "std" / "base64.vitl",
    SOURCE_STDLIB_DIR / "std" / "semver.vitl",
    SOURCE_STDLIB_DIR / "std" / "mime.vitl",
    SOURCE_STDLIB_DIR / "std" / "http.vitl",
    SOURCE_STDLIB_DIR / "std" / "uri.vitl",
    SOURCE_STDLIB_DIR / "std" / "percent_encoding.vitl",
    SOURCE_STDLIB_DIR / "std" / "kernel.vitl",
    SOURCE_STDLIB_DIR / "std" / "terminal.vitl",
    SOURCE_STDLIB_DIR / "std" / "signal.vitl",
    SOURCE_STDLIB_DIR / "std" / "tempfile.vitl",
    SOURCE_STDLIB_DIR / "std" / "glob.vitl",
    SOURCE_STDLIB_DIR / "std" / "diff.vitl",
    SOURCE_STDLIB_DIR / "std" / "locale.vitl",
    SOURCE_STDLIB_DIR / "std" / "calendar.vitl",
    SOURCE_STDLIB_DIR / "std" / "units.vitl",
    SOURCE_STDLIB_DIR / "std" / "metrics.vitl",
    SOURCE_STDLIB_DIR / "std" / "event.vitl",
    SOURCE_STDLIB_DIR / "platform" / "abi.vitl",
    SOURCE_STDLIB_DIR / "platform" / "posix.vitl",
    SOURCE_STDLIB_DIR / "platform" / "windows.vitl",
    SOURCE_STDLIB_DIR / "platform" / "wasm.vitl",
    SOURCE_STDLIB_DIR / "platform" / "embedded.vitl",
    SOURCE_STDLIB_DIR / "generated" / "unicode_tables.vitl",
    SOURCE_STDLIB_DIR / "tools" / "unicode_tables.vitl",
    SOURCE_STDLIB_DIR / "tests" / "api_contracts.vit",
    SOURCE_STDLIB_DIR / "tests" / "alloc_memory_invariants.vit",
    SOURCE_STDLIB_DIR / "tests" / "core_alloc_contracts.vit",
    SOURCE_STDLIB_DIR / "tests" / "range_unicode_std_contracts.vit",
    SOURCE_STDLIB_DIR / "tests" / "std_runtime_contracts.vit",
    SOURCE_STDLIB_DIR / "tests" / "serialization_platform_contracts.vit",
    SOURCE_STDLIB_DIR / "tests" / "std_extra_libraries_contracts.vit",
    SOURCE_STDLIB_DIR / "tests" / "std_more_libraries_contracts.vit",
    SOURCE_STDLIB_DIR / "tests" / "std_max_libraries_contracts.vit",
    SOURCE_STDLIB_DIR / "tests" / "fuzz" / "utf8_url_csv.vit",
    SOURCE_STDLIB_DIR / "tests" / "fuzz" / "path_json_parse.vit",
    SOURCE_STDLIB_DIR / "tests" / "fuzz" / "utf8_fuzz.vit",
    SOURCE_STDLIB_DIR / "tests" / "fuzz" / "url_fuzz.vit",
    SOURCE_STDLIB_DIR / "tests" / "fuzz" / "csv_fuzz.vit",
    SOURCE_STDLIB_DIR / "tests" / "fuzz" / "json_fuzz.vit",
    SOURCE_STDLIB_DIR / "tests" / "fuzz" / "path_fuzz.vit",
    SOURCE_STDLIB_DIR / "tests" / "fuzz" / "minimize.vit",
    SOURCE_STDLIB_DIR / "tests" / "module_runner.vit",
    SOURCE_STDLIB_DIR / "tests" / "public_module_coverage.vit",
    SOURCE_STDLIB_DIR / "tests" / "modules" / "alloc_api_test.vit",
    SOURCE_STDLIB_DIR / "tests" / "modules" / "std_uuid_test.vit",
    SOURCE_STDLIB_DIR / "tests" / "modules" / "std_calendar_test.vit",
    SOURCE_STDLIB_DIR / "tests" / "modules" / "std_io_process_test.vit",
    SOURCE_STDLIB_DIR / "tests" / "modules" / "std_units_test.vit",
    SOURCE_STDLIB_DIR / "tests" / "modules" / "std_metrics_test.vit",
    SOURCE_STDLIB_DIR / "tests" / "modules" / "std_mime_test.vit",
    SOURCE_STDLIB_DIR / "tests" / "modules" / "std_percent_encoding_test.vit",
    SOURCE_STDLIB_DIR / "tests" / "modules" / "std_thread_sync_time_env_test.vit",
    SOURCE_STDLIB_DIR / "tests" / "platform" / "posix_test.vit",
    SOURCE_STDLIB_DIR / "tests" / "platform" / "windows_test.vit",
    SOURCE_STDLIB_DIR / "tests" / "platform" / "wasm_test.vit",
    SOURCE_STDLIB_DIR / "tests" / "platform" / "embedded_test.vit",
    SOURCE_STDLIB_DIR / "tests" / "negative" / "stdlib_negative_cases.vit",
    SOURCE_STDLIB_DIR / "benchmarks" / "index.vit",
    SOURCE_STDLIB_DIR / "benchmarks" / "modules" / "vec_bench.vit",
    SOURCE_STDLIB_DIR / "benchmarks" / "modules" / "string_bench.vit",
    SOURCE_STDLIB_DIR / "benchmarks" / "modules" / "hashmap_bench.vit",
    SOURCE_STDLIB_DIR / "benchmarks" / "modules" / "utf8_bench.vit",
    SOURCE_STDLIB_DIR / "benchmarks" / "modules" / "path_bench.vit",
    SOURCE_STDLIB_DIR / "benchmarks" / "modules" / "format_bench.vit",
    SOURCE_STDLIB_DIR / "benchmarks" / "modules" / "parse_bench.vit",
    SOURCE_STDLIB_DIR / "benchmarks" / "thresholds.json",
    SOURCE_STDLIB_DIR / "benchmarks" / "history.json",
    SOURCE_STDLIB_DIR / "tests" / "snapshots" / "stdlib_api.snap",
    SOURCE_STDLIB_DIR / "tests" / "snapshots" / "modules" / "alloc.snap",
    SOURCE_STDLIB_DIR / "tests" / "snapshots" / "modules" / "io.snap",
    SOURCE_STDLIB_DIR / "tests" / "snapshots" / "modules" / "fs.snap",
    SOURCE_STDLIB_DIR / "tests" / "snapshots" / "modules" / "path.snap",
    SOURCE_STDLIB_DIR / "tests" / "snapshots" / "modules" / "platform.snap",
    SOURCE_STDLIB_DIR / "examples" / "stdlib_max.vit",
    SOURCE_STDLIB_DIR / "examples" / "public_module_examples.vit",
    SOURCE_STDLIB_DIR / "examples" / "stdlib_usage_examples.vit",
    ROOT / "tools" / "stdlib" / "generate_api_docs.py",
    ROOT / "tools" / "stdlib" / "generate_changelog.py",
    ROOT / "tools" / "stdlib" / "generate_unicode_tables.py",
    ROOT / "docs" / "compiler" / "stdlib_next_steps.md",
    ROOT / "docs" / "compiler" / "stdlib_boundaries.md",
    ROOT / "docs" / "compiler" / "stdlib_api.md",
    ROOT / "docs" / "compiler" / "stdlib_api.generated.md",
    ROOT / "docs" / "compiler" / "stdlib_api.generated.json",
    ROOT / "docs" / "compiler" / "stdlib_lsp_index.md",
    ROOT / "docs" / "compiler" / "stdlib_lsp_index.generated.json",
    ROOT / "docs" / "compiler" / "stdlib_api_stability.md",
    ROOT / "docs" / "compiler" / "stdlib_contributing.md",
    ROOT / "docs" / "compiler" / "stdlib_changelog.md",
    CI_MATRIX_MANIFEST,
)

REPORT_DIR.mkdir(parents=True, exist_ok=True)
STDLIB_DIR.mkdir(parents=True, exist_ok=True)
HISTORY_DIR.mkdir(parents=True, exist_ok=True)

JSON_REPORT = REPORT_DIR / "stdlib_validation.json"
MARKDOWN_REPORT = REPORT_DIR / "stdlib_validation.md"
HTML_REPORT = REPORT_DIR / "stdlib_validation.html"
COMPILER_HELPERS_REPORT = REPORT_DIR / "stdlib_compiler_helpers.json"


REQUIRED_FILES = [
    ROOT / "src/vitte/stdlib/mod.vit",
    ROOT / "src/vitte/stdlib/tests/smoke.vit",
    ARCHITECTURE_MANIFEST,
    MODULE_MANIFEST,
    DEPENDENCY_GRAPH_MANIFEST,
    CI_MATRIX_MANIFEST,
    CORE_PRIMITIVE_SOURCE,
    CORE_PRIMITIVE_DOC,
    CORE_OPTION_SOURCE,
    CORE_RESULT_SOURCE,
    CORE_OPTION_RESULT_DOC,
    CORE_CONVERT_SOURCE,
    CORE_DEFAULT_SOURCE,
    CORE_CLONE_SOURCE,
    CORE_CONVERT_DEFAULT_CLONE_DOC,
    CORE_DROP_SOURCE,
    CORE_SCOPE_SOURCE,
    CORE_DROP_SCOPE_MEMORY_DOC,
    CORE_ITERATOR_SOURCE,
    CORE_RANGE_SOURCE,
    CORE_NUMBER_SOURCE,
    CORE_FLOAT_SOURCE,
    CORE_MATH_SOURCE,
    CORE_STRING_SOURCE,
    CORE_ASCII_SOURCE,
    CORE_UNICODE_SOURCE,
    *STDLIB_NEXT_STEP_SOURCES,
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
    "ffi": "ffi/ffi.vitl",
    "http": "network/http.vitl",
    "json": "json.vitl",
    "regex": "regex.vitl",
    "crypto": "crypto.vitl",
    "compression": "compression.vitl",
}

ROADMAP_FEATURE_MODULES = {
    "async": "async.vitl",
    "channels": "async/channel.vitl",
    "network_facade": "network.vitl",
    "image": "image.vitl",
    "audio": "audio.vitl",
}

NON_SCOPE_FEATURE_MODULES = {
    "yaml": "yaml.vitl",
    "toml": "toml.vitl",
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
STD_RAW_PLATFORM_PATTERNS = (
    r"\bsyscall_",
    r"\blibc_",
    r"\bwin32_",
    r"\bposix_",
    r"\bwasm_",
    r"\bembedded_",
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
REQUIRED_CONVERT_FRAGMENTS = (
    "form From<T, U>",
    "form Into<T, U>",
    "form TryFrom<T, U, E>",
    "form TryInto<T, U, E>",
    "form AsRef<T, U>",
    "form AsMut<T, U>",
    "form Borrow<T, U>",
    "form BorrowMut<T, U>",
    "numeric_safe_i32_to_i64",
    "numeric_checked_i64_to_i32",
    "numeric_saturating_i64_to_i32",
    "numeric_wrapping_i64_to_i32",
    "numeric_truncating_f64_to_i64",
    "dangerous_implicit_conversion_allowed",
    "conversion_impossible_error",
    "require_explicit_conversion",
)
REQUIRED_DEFAULT_FRAGMENTS = (
    "form Default<T>",
    "default_bool",
    "default_byte",
    "default_char",
    "default_rune",
    "default_int",
    "default_uint",
    "default_isize",
    "default_usize",
    "default_i8",
    "default_i16",
    "default_i32",
    "default_i64",
    "default_i128",
    "default_u8",
    "default_u16",
    "default_u32",
    "default_u64",
    "default_u128",
    "default_f32",
    "default_f64",
    "default_for_compatible_struct",
    "reject_meaningless_default",
)
REQUIRED_CLONE_FRAGMENTS = (
    "form Clone<T>",
    "form Copy<T>",
    "pick CloneCost",
    "copy_trivial",
    "clone_from_ref",
    "copy_is_trivial",
    "clone_is_costly",
    "implicit_clone_allowed",
    "copy_forbidden_diagnostic",
    "clone_required_diagnostic",
    "reject_implicit_clone",
)
REQUIRED_DROP_FRAGMENTS = (
    "pick DropState",
    "form DropProtocol<T>",
    "form DropResult",
    "form DropSlot",
    "proc drop<T>",
    "proc forget<T>",
    "proc needs_drop<T>",
    "deterministic_drop_order",
    "partial_drop_allowed",
    "early_exit_drop_required",
    "drop_error_policy",
    "double_drop_forbidden",
    "double_drop_diagnostic",
)
REQUIRED_SCOPE_FRAGMENTS = (
    "pick ScopeExitReason",
    "form ScopeGuard<T>",
    "form DeferAction",
    "proc scope_guard<T>",
    "proc dismiss<T>",
    "proc run_scope_guard<T>",
    "proc defer",
    "proc run_defer",
    "defer_runs_for_exit",
    "defer_panic_behavior",
    "scope_exit_guarantee",
)
REQUIRED_MEMORY_FRAGMENTS = (
    "form MemoryLayout",
    "form MemoryCheck",
    "proc size_of<T>",
    "proc size_of_value<T>",
    "proc align_of<T>",
    "proc align_of_value<T>",
    "proc offset_of<T>",
    "proc swap<T>",
    "proc replace<T>",
    "proc take<T>",
    "proc forget<T>",
    "proc drop<T>",
    "proc unsafe_transmute<T, U>",
    "proc unsafe_zeroed<T>",
    "proc unsafe_uninitialized<T>",
    "detect_size_compatible",
    "detect_alignment_compatible",
    "unsafe_operation_invariants",
)
REQUIRED_ITERATOR_FRAGMENTS = (
    "form Iterator<T>",
    "form SizeHint",
    "proc next<T>",
    "proc iterator_size_hint<T>",
    "proc count<T>",
    "proc last<T>",
    "proc nth<T>",
    "proc step_by<T>",
    "proc chain<T>",
    "proc zip<T, U>",
    "proc map<T, U>",
    "proc filter<T>",
    "proc filter_map<T, U>",
    "proc flat_map<T, U>",
    "proc flatten<T>",
    "proc enumerate<T>",
    "proc peekable<T>",
    "proc skip<T>",
    "proc take<T>",
    "proc skip_while<T>",
    "proc take_while<T>",
    "proc scan<T, S, U>",
    "proc inspect<T>",
    "proc fold<T, U>",
    "proc reduce<T>",
    "proc try_fold<T, U, E>",
    "proc all<T>",
    "proc any<T>",
    "proc find<T>",
    "proc find_map<T, U>",
    "proc position<T>",
    "proc rposition<T>",
    "proc min<T>",
    "proc max<T>",
    "proc min_by<T>",
    "proc max_by<T>",
    "proc sum<T>",
    "proc product<T>",
    "proc collect<T, C>",
    "proc partition<T, C>",
    "proc unzip<T, U, C, D>",
    "form DoubleEndedIterator<T>",
    "form ExactSizeIterator<T>",
    "form FusedIterator<T>",
    "form CloneableIterator<T>",
    "form RangeIterator<T>",
    "proc double_ended<T>",
    "proc exact_size<T>",
    "proc fused<T>",
    "proc cloneable<T>",
    "proc range<T>",
)
REQUIRED_RANGE_FRAGMENTS = (
    "pick BoundKind",
    "BoundKind.Included",
    "BoundKind.Excluded",
    "BoundKind.Unbounded",
    "pick RangeKind",
    "RangeKind.Exclusive",
    "RangeKind.Inclusive",
    "RangeKind.Open",
    "RangeKind.Full",
    "RangeKind.From",
    "RangeKind.To",
    "form Bound<T>",
    "form Range<T>",
    "form RangeCheck",
    "proc included<T>",
    "proc excluded<T>",
    "proc unbounded<T>",
    "proc exclusive<T>",
    "proc inclusive<T>",
    "proc open<T>",
    "proc full<T>",
    "proc from<T>",
    "proc to<T>",
    "proc positive_step<T>",
    "proc negative_step<T>",
    "detect_zero_step",
    "detect_range_overflow",
    "form RangeIterator<T>",
    "proc contains<T>",
    "proc iter<T>",
    "proc next<T>",
    "compiler_range_next_overflows",
)
REQUIRED_NUMBER_FRAGMENTS = (
    "form NumberLimits<T>",
    "form FloatConstants<T>",
    "form Checked<T>",
    "form Overflowing<T>",
    "pick Sign",
    "pick Endian",
    "proc number_limits<T>",
    "proc float_constants<T>",
    "proc checked_add<T>",
    "proc checked_sub<T>",
    "proc checked_mul<T>",
    "proc checked_div<T>",
    "proc checked_rem<T>",
    "proc checked_neg<T>",
    "proc checked_shl<T>",
    "proc checked_shr<T>",
    "proc saturating_add<T>",
    "proc saturating_sub<T>",
    "proc saturating_mul<T>",
    "proc saturating_div<T>",
    "proc wrapping_add<T>",
    "proc wrapping_sub<T>",
    "proc wrapping_mul<T>",
    "proc wrapping_div<T>",
    "proc overflowing_add<T>",
    "proc overflowing_sub<T>",
    "proc overflowing_mul<T>",
    "proc overflowing_div<T>",
    "proc abs<T>",
    "proc sign<T>",
    "proc signum<T>",
    "proc pow<T>",
    "proc pow_mod<T>",
    "proc gcd<T>",
    "proc lcm<T>",
    "proc is_even<T>",
    "proc is_odd<T>",
    "proc count_ones<T>",
    "proc count_zeros<T>",
    "proc leading_zeros<T>",
    "proc trailing_zeros<T>",
    "proc rotate_left<T>",
    "proc rotate_right<T>",
    "proc reverse_bits<T>",
    "proc reverse_bytes<T>",
    "proc to_little_endian<T>",
    "proc to_big_endian<T>",
    "proc from_little_endian<T>",
    "proc from_big_endian<T>",
    "proc endian_convert<T>",
)
REQUIRED_FLOAT_FRAGMENTS = (
    "pick FloatClass",
    "pick FloatOrdering",
    "proc is_nan<T>",
    "proc is_infinite<T>",
    "proc is_finite<T>",
    "proc is_normal<T>",
    "proc is_subnormal<T>",
    "proc is_sign_positive<T>",
    "proc is_sign_negative<T>",
    "proc next_up<T>",
    "proc next_down<T>",
    "proc total_compare<T>",
    "proc float_class<T>",
)
REQUIRED_MATH_FRAGMENTS = (
    "const PI: f64",
    "const TAU: f64",
    "const E: f64",
    "const FRAC_PI_2: f64",
    "const FRAC_PI_3: f64",
    "const FRAC_PI_4: f64",
    "const FRAC_PI_6: f64",
    "const FRAC_PI_8: f64",
    "const SQRT_2: f64",
    "const FRAC_1_SQRT_2: f64",
    "const LN_2: f64",
    "const LN_10: f64",
    "const LOG2_E: f64",
    "const LOG10_E: f64",
    "proc backend_math_available",
    "proc sqrt",
    "proc cbrt",
    "proc hypot",
    "proc exp",
    "proc exp2",
    "proc expm1",
    "proc ln",
    "proc ln1p",
    "proc log2",
    "proc log10",
    "proc sin",
    "proc cos",
    "proc tan",
    "proc asin",
    "proc acos",
    "proc atan",
    "proc atan2",
    "proc sinh",
    "proc cosh",
    "proc tanh",
    "proc floor",
    "proc ceil",
    "proc round",
    "proc trunc",
    "proc fract",
    "proc copysign",
    "proc fma",
    "portable_math_",
    "compiler_math_intrinsic_available",
)
REQUIRED_STRING_FRAGMENTS = (
    "form Utf8View",
    "form CharIndex",
    "form SplitOnce",
    "pick StringOrdering",
    "form Utf8Decode",
    "form SplitIterator",
    "proc utf8_view",
    "proc byte_length",
    "proc char_length",
    "proc bytes",
    "proc chars",
    "proc char_indices",
    "proc lines",
    "proc split",
    "proc split_iter",
    "proc split_once",
    "proc split_whitespace",
    "proc trim",
    "proc trim_start",
    "proc trim_end",
    "proc starts_with",
    "proc ends_with",
    "proc contains",
    "proc find",
    "proc rfind",
    "proc compare",
    "proc equals",
    "proc validate_utf8",
    "proc decode_char",
    "proc encode_utf8",
    "proc is_char_boundary",
    "proc checked_byte_index",
    "proc reject_mid_char_index",
)
REQUIRED_ASCII_FRAGMENTS = (
    "proc is_ascii",
    "proc is_alphabetic",
    "proc is_numeric",
    "proc is_alphanumeric",
    "proc is_whitespace",
    "proc to_uppercase",
    "proc to_lowercase",
    "proc escape_ascii",
)
REQUIRED_UNICODE_FRAGMENTS = (
    "const UNICODE_VERSION",
    "const UNICODE_TABLES_GENERATED",
    "const UNICODE_TABLE_GENERATOR",
    "pick UnicodeCategory",
    "form UnicodeProperties",
    "proc unicode_version",
    "proc unicode_tables_generated",
    "proc unicode_table_generator",
    "proc general_category",
    "proc properties",
    "proc is_alphabetic",
    "proc is_numeric",
    "proc is_alphanumeric",
    "proc is_whitespace",
    "proc is_uppercase",
    "proc is_lowercase",
    "proc to_uppercase",
    "proc to_lowercase",
    "proc to_titlecase",
    "proc case_fold",
    "const UNICODE_UCD_CHECKSUM",
    "pick NormalizationForm",
    "proc normalize",
    "proc normalize_nfc",
    "proc normalize_nfd",
    "proc normalize_nfkc",
    "proc normalize_nfkd",
    "proc case_fold_string",
    "proc unicode_ucd_checksum",
    "proc unicode_tables_verified",
    "proc normalize_with_checksum",
)
REQUIRED_NEXT_STEP_FRAGMENTS = {
    "core/iterator.vitl": ("form Iterator<T>", "form MapIterator<T, U>", "form FilterIterator<T>", "form SkipIterator<T>", "form TakeIterator<T>", "form EnumerateIterator<T>", "proc map<T, U>", "proc filter<T>", "proc skip_iter<T>", "proc take_iter<T>", "proc next_skip<T>", "proc next_take<T>", "proc next_enumerate<T>"),
    "core/range.vitl": ("pick BoundKind", "pick RangeKind", "form Range<T>", "form RangeBounds<T>", "proc inclusive<T>", "proc exclusive<T>", "proc descending_exclusive<T>", "proc descending_inclusive<T>", "proc detect_range_overflow<T>", "proc positive_step<T>", "proc negative_step<T>", "proc checked<T>"),
    "core/slice.vitl": ("form Slice<T>", "proc get<T>", "proc chunks<T>", "proc windows<T>", "proc split<T>", "proc binary_search<T>", "proc binary_search_by<T>", "proc sort_unstable<T>", "proc sort_stable<T>", "proc reverse<T>"),
    "core/array.vitl": ("form Array<T>", "proc array_len<T", "proc array_get<T", "proc array_sort<T"),
    "core/cmp.vitl": ("pick Ordering", "form Eq<T>", "form Ord<T>", "proc compare<T>"),
    "core/hash.vitl": ("form Hasher", "form Hash<T>", "proc hash<T>", "proc combine_hash"),
    "alloc/box.vitl": ("form Box<T>", "proc box_new<T>", "proc box_try_new<T>", "proc box_replace<T>", "proc box_leak<T>", "proc box_drop<T>"),
    "alloc/btree.vitl": ("type BTreeMap<K, V>", "proc btree_map_insert<K, V>", "proc btree_map_get<K, V>", "proc btree_map_range<K, V>", "proc btree_set_range<T>"),
    "alloc/collections.vitl": ("form HashMap<K, V>", "form HashSet<T>", "form BTreeMap<K, V>", "form BTreeSet<T>", "form Deque<T>", "form SmallVec<T, N>", "proc hashmap_insert<K, V>", "proc hashmap_get_or_insert<K, V>", "proc hashmap_keys<K, V>", "proc hashset_union<T>", "proc btreemap_range<K, V>", "proc btreeset_range<T>", "proc deque_rotate_left<T>", "proc deque_make_contiguous<T>", "proc smallvec_inline_capacity<T, N>", "proc smallvec_shrink_to_fit<T, N>"),
    "alloc/deque.vitl": ("type Deque<T>", "proc deque_push_back<T>", "proc deque_pop_front<T>", "proc deque_capacity<T>", "proc deque_rotate_left<T>", "proc deque_make_contiguous<T>"),
    "alloc/vec.vitl": ("form Vec<T>", "proc vec_new<T>", "proc vec_push<T>", "proc vec_reserve_exact<T>", "proc vec_split_off<T>", "proc vec_drain<T>", "proc vec_retain<T>", "proc vec_as_slice<T>", "compiler_vec_realloc"),
    "alloc/string.vitl": ("form String", "form StringSlice", "proc string_new", "proc string_with_capacity", "proc string_push", "proc string_pop", "proc string_split", "proc string_trim", "proc string_insert_str", "proc string_drain", "proc string_as_bytes"),
    "alloc/smallvec.vitl": ("type SmallVec<T, N>", "proc smallvec_push<T, N>", "proc smallvec_pop<T, N>", "proc smallvec_inline_capacity<T, N>", "proc smallvec_spilled<T, N>", "proc smallvec_shrink_to_fit<T, N>"),
    "alloc/rc.vitl": ("form Rc<T>", "form Weak<T>", "proc rc_new<T>", "proc rc_try_new<T>", "proc rc_get_mut<T>", "proc rc_try_unwrap<T>", "proc weak_upgrade<T>"),
    "alloc/arc.vitl": ("form Arc<T>", "form ArcWeak<T>", "proc arc_new<T>", "proc arc_try_new<T>", "proc arc_get_mut<T>", "proc arc_try_unwrap<T>", "proc arc_weak_upgrade<T>"),
    "std/io.vitl": ("form IoError", "form Reader", "form Writer", "form Read", "form Write", "form Seek", "form BufReader", "form BufWriter", "form Cursor", "proc read_to_string", "proc seek", "proc buffered_read", "proc buffered_write"),
    "std/error.vitl": ("form Error", "form Backtrace", "form ErrorTrait<T>", "proc error_with_source", "proc capture_backtrace", "proc error_chain"),
    "std/fs.vitl": ("form Path", "form Metadata", "form FsError", "form FileType", "form FileTimes", "form CopyOptions", "proc read_to_string", "proc read_dir", "proc copy_with_options", "proc canonicalize", "proc atomic_write", "proc same_file"),
    "std/env.vitl": ("form EnvError", "form RuntimeInfo", "proc args", "proc args_vec", "proc var_or", "proc current_exe", "proc runtime_info", "proc split_paths", "proc join_paths"),
    "std/path.vitl": ("form PathBuf", "pick PathStyle", "pick ComponentKind", "proc components", "proc normalize", "proc normalize_strict", "proc strip_prefix", "proc relative_from", "proc with_file_name", "proc to_fs_path"),
    "std/time.vitl": ("form Duration", "form Instant", "proc instant_now", "proc elapsed", "proc duration_checked_add", "proc format_duration", "proc checked_add_system_time"),
    "std/process.vitl": ("form ExitStatus", "form Command", "form Child", "form Output", "form Pipe", "form CommandStatus", "proc args", "proc env_clear", "proc wait_timeout", "proc kill", "proc status_report", "proc output_checked"),
    "std/serialization.vitl": ("form Encode<T>", "form Decode<T>", "form Encoder", "form Decoder", "form JsonField", "proc encode_json<T>", "proc decode_json<T>", "proc encode_derived<T>", "proc decode_derived<T>", "proc json_string"),
    "std/atomic.vitl": ("pick Ordering", "form AtomicBool", "form AtomicUsize", "proc load_bool", "proc compare_exchange_bool", "proc fetch_add_usize", "proc swap_usize", "proc fetch_or_usize", "proc fetch_xor_usize"),
    "std/archive.vitl": ("form ArchiveEntry", "form Archive", "form ArchiveError", "proc archive_new", "proc archive_add", "proc archive_pack"),
    "std/bitset.vitl": ("form BitSet", "proc bitset_new", "proc bitset_set", "proc bitset_contains", "proc bitset_count_ones"),
    "std/cache.vitl": ("form CacheEntry<T>", "form Cache<T>", "proc cache_new<T>", "proc cache_insert<T>", "proc cache_prune_expired<T>"),
    "std/checksum.vitl": ("form Checksum", "proc checksum_crc32", "proc checksum_adler32", "proc checksum_sha256", "proc checksum_verify"),
    "std/cron.vitl": ("form CronSchedule", "form CronError", "proc cron_parse", "proc cron_every_minute", "proc cron_next"),
    "std/graph.vitl": ("form GraphNode<T>", "form GraphEdge<W>", "form Graph<T, W>", "proc graph_new<T, W>", "proc graph_toposort<T, W>"),
    "std/pool.vitl": ("form Pool<T>", "proc pool_new<T>", "proc pool_put<T>", "proc pool_take<T>", "proc pool_clear<T>"),
    "std/rate_limit.vitl": ("form RateLimit", "proc rate_limit_new", "proc rate_limit_allow", "proc rate_limit_remaining", "proc rate_limit_reset"),
    "std/retry.vitl": ("form RetryPolicy", "form RetryState", "proc retry_policy", "proc retry_should_continue", "proc retry_next_delay"),
    "std/tracing.vitl": ("form TraceId", "form SpanId", "form TraceSpan", "form TraceEvent", "proc trace_span_start", "proc trace_emit"),
    "std/thread.vitl": ("form ThreadId", "form JoinHandle<T>", "form ThreadBuilder", "form ThreadScope", "proc available_parallelism", "proc spawn<T>", "proc spawn_with<T>", "proc is_finished<T>", "proc detach<T>", "proc scoped_spawn<T>"),
    "std/sync.vitl": ("form Mutex<T>", "form RwLock<T>", "form Once", "form Condvar", "form Barrier", "form Semaphore", "proc mutex_try_lock<T>", "proc try_read<T>", "proc barrier_wait", "proc semaphore_try_acquire", "proc atomic_compare_exchange<T>"),
    "std/net.vitl": ("form TcpStream", "form TcpListener", "form UdpSocket", "form Timeout", "form DnsOptions", "form TcpOptions", "proc dns_lookup_with_options", "proc tcp_set_options", "proc tcp_peer_addr", "proc udp_join_multicast"),
    "std/hash.vitl": ("form StableHasher", "form RandomHasher", "form SipHasher", "pick HashStability", "form HashSeed", "proc sip_hasher", "proc stable_finish", "proc stable_hash_bytes", "proc nonstable_hash_bytes", "proc siphash24"),
    "std/random.vitl": ("form Prng", "pick EntropyFailure", "form Seed", "form DistributionU64", "proc prng", "proc prng_from_seed", "proc next_u64", "proc uniform_u64", "proc sample_u64", "proc os_entropy", "proc random_f64"),
    "std/format.vitl": ("pick FormatBase", "pick Alignment", "form FormatSpec", "form Display<T>", "form Debug<T>", "proc format_int", "proc pad_left", "proc pad_right", "proc pad_center", "proc format_debug<T>"),
    "std/parse.vitl": ("form ParseError", "pick ParseErrorKind", "proc parse_bool", "proc parse_i64", "proc parse_i32", "proc parse_f64", "proc parse_utf8", "proc parse_error_kind"),
    "std/testing.vitl": ("form TestError", "form Fixture<T>", "form Snapshot", "form TestSuite", "form TestRunner", "proc assert_ne<T>", "proc assert_err<T, E>", "proc test_suite", "proc suite_add", "proc run_runner"),
    "std/bench.vitl": ("form Benchmark", "form BenchOptions", "form BenchStats", "form BenchReport", "proc run", "proc run_with_options", "proc threshold", "proc report_text", "proc threshold_passed"),
    "std/log.vitl": ("pick LogLevel", "pick LogSinkKind", "form LogField", "form LogRecord", "form LogSink", "form Logger", "proc structured_sink", "proc log_record", "proc log_with_field", "proc compact_record"),
    "std/cli.vitl": ("form Flag", "form Subcommand", "form CliApp", "form CliMatches", "pick CliErrorKind", "proc version", "proc switch", "proc option", "proc parse_env", "proc usage", "proc present"),
    "std/uuid.vitl": ("form Uuid", "form UuidError", "proc uuid_v4", "proc uuid_parse", "proc uuid_to_string"),
    "std/url.vitl": ("form Url", "form UrlError", "proc url_parse", "proc url_join", "proc url_to_string"),
    "std/csv.vitl": ("form CsvOptions", "form CsvRecord", "form CsvError", "proc csv_parse", "proc csv_write"),
    "std/base64.vitl": ("pick Base64Alphabet", "form Base64Config", "form Base64Error", "proc base64_encode", "proc base64_decode"),
    "std/semver.vitl": ("form Version", "form VersionReq", "form SemverError", "proc semver_parse", "proc semver_matches"),
    "std/mime.vitl": ("form Mime", "form MimeError", "proc mime_parse", "proc mime_text_plain"),
    "std/http.vitl": ("form HeaderMap", "form Request", "form Response", "form HttpError", "form HttpClient", "proc header_get", "proc client_with_timeout", "proc with_json_body", "proc http_send_with_client", "proc status_is_success", "proc body_text"),
    "std/uri.vitl": ("form Uri", "form UriError", "proc uri_parse", "proc uri_to_string"),
    "std/percent_encoding.vitl": ("form PercentEncodeSet", "form PercentDecodeError", "proc percent_encode", "proc percent_decode"),
    "std/kernel.vitl": ("form KernelInfo", "form KernelError", "proc kernel_info", "proc page_size", "proc cpu_count"),
    "std/terminal.vitl": ("pick TerminalColor", "form TerminalStyle", "form TerminalSize", "proc terminal_size", "proc terminal_apply"),
    "std/signal.vitl": ("form Signal", "form SignalHandler", "form SignalError", "proc signal_install", "proc signal_raise"),
    "std/tempfile.vitl": ("form TempFile", "form TempDir", "form TempError", "proc temp_file", "proc persist_file"),
    "std/glob.vitl": ("form GlobPattern", "form GlobOptions", "form GlobError", "proc glob_match", "proc glob_walk"),
    "std/diff.vitl": ("pick DiffTag", "form DiffHunk", "form DiffOptions", "proc diff_lines", "proc diff_unified"),
    "std/locale.vitl": ("form Locale", "form LocaleError", "proc locale_parse", "proc locale_current"),
    "std/calendar.vitl": ("pick Weekday", "form Date", "form CalendarError", "proc is_leap_year", "proc weekday"),
    "std/units.vitl": ("pick UnitKind", "form Unit", "form Quantity", "proc convert", "proc meter"),
    "std/metrics.vitl": ("form Counter", "form Gauge", "form Histogram", "proc counter_inc", "proc metrics_render_counter"),
    "std/event.vitl": ("form Event", "form EventBus", "form EventError", "proc subscribe", "proc publish"),
    "platform/abi.vitl": ("form PlatformInfo", "form PlatformFeature", "proc platform_info", "proc supports_filesystem"),
    "platform/posix.vitl": ("form PosixFd", "form PosixError", "proc posix_open", "proc posix_read"),
    "platform/windows.vitl": ("form WindowsHandle", "form WindowsError", "proc windows_open", "proc windows_read"),
    "platform/wasm.vitl": ("form WasmMemory", "form WasmImport", "proc wasm_memory_pages", "proc wasm_call"),
    "platform/embedded.vitl": ("form EmbeddedPin", "form EmbeddedClock", "proc embedded_available", "proc write_pin"),
    "generated/unicode_tables.vitl": ("GENERATED_UNICODE_VERSION", "GENERATED_UNICODE_CHECKSUM", "sha256:", "proc generated_unicode_verify_checksum", "proc generated_unicode_category", "proc generated_unicode_properties", "proc generated_unicode_normalization", "proc generated_unicode_case_fold"),
    "tools/unicode_tables.vitl": ("form UnicodeTableGeneration", "proc generate_unicode_tables", "proc verify_unicode_tables"),
    "tests/api_contracts.vit": ("stdlib_api_contracts_smoke", "std_time.duration_from_secs", "platform_abi.supports_filesystem"),
    "tests/core_alloc_contracts.vit": ("stdlib_core_alloc_contracts_smoke", "alloc_vec.vec_push", "alloc_string.string_push"),
    "tests/range_unicode_std_contracts.vit": ("stdlib_range_unicode_std_contracts_smoke", "core_range.contains", "alloc_collections.hashmap_new", "std_error.error_new"),
    "tests/std_runtime_contracts.vit": ("stdlib_runtime_contracts_smoke", "std_path.normalize", "std_random.prng", "std_cli.help"),
    "tests/serialization_platform_contracts.vit": ("stdlib_serialization_platform_contracts_smoke", "std_serialization.json_encoder", "platform_posix.posix_available"),
    "tests/std_extra_libraries_contracts.vit": ("stdlib_extra_libraries_contracts_smoke", "std_uuid.uuid_nil", "std_base64.base64_standard", "std_kernel.page_size"),
    "tests/std_more_libraries_contracts.vit": ("stdlib_more_libraries_contracts_smoke", "std_terminal.terminal_style", "std_calendar.is_leap_year", "std_event.event_bus"),
    "tests/std_max_libraries_contracts.vit": ("stdlib_max_libraries_contracts_smoke", "std_archive.archive_new", "std_checksum.checksum_crc32", "std_tracing.trace_id_new"),
    "tests/fuzz/utf8_url_csv.vit": ("fuzz_utf8_url_csv", "core_string.validate_utf8", "std_url.url_parse", "std_csv.csv_parse"),
    "tests/fuzz/path_json_parse.vit": ("fuzz_path_json_parse", "std_path.normalize", "std_serialization.json_value", "std_parse.parse_i64"),
    "tests/module_runner.vit": ("stdlib_module_tests_run", "std_uuid_test", "std_calendar_test", "std_percent_encoding_test", "std_io_process_test", "std_thread_sync_time_env_test", "std_text_inputs_test", "alloc_api_test", "stdlib_negative_cases_run"),
    "tests/modules/std_text_inputs_test.vit": ("std_text_inputs_test", "café déjà vu", "alpha\\r\\nbeta", "\\tcol1\\tcol2", "unicode_tables_verified", "parse_bool"),
    "tests/public_module_coverage.vit": ("PUBLIC_MODULE_TEST_COUNT", "public_module_tests_present", "src/vitte/stdlib/std/uuid.vitl"),
    "tests/modules/alloc_api_test.vit": ("alloc_api_test", "vec_capacity", "string_capacity", "hashmap_len"),
    "tests/modules/std_uuid_test.vit": ("std_uuid_test", "std_uuid.uuid_nil", "std_uuid.uuid_is_nil"),
    "tests/modules/std_calendar_test.vit": ("std_calendar_test", "std_calendar.is_leap_year", "std_calendar.date"),
    "tests/modules/std_io_process_test.vit": ("std_io_process_test", "std_io.flush", "std_process.command", "std_process.arg"),
    "tests/modules/std_units_test.vit": ("std_units_test", "std_units.convert", "std_units.meter"),
    "tests/modules/std_metrics_test.vit": ("std_metrics_test", "std_metrics.counter_inc", "counter.value == 3"),
    "tests/modules/std_mime_test.vit": ("std_mime_test", "std_mime.mime_text_plain", "std_mime.mime_application_json"),
    "tests/modules/std_percent_encoding_test.vit": ("std_percent_encoding_test", "percent_encode_set_component", "component"),
    "tests/modules/std_thread_sync_time_env_test.vit": ("std_thread_sync_time_env_test", "std_sync.atomic_compare_exchange", "std_time.checked_add_duration", "std_env.temp_dir"),
    "tests/alloc_memory_invariants.vit": ("alloc_memory_invariants_run", "alloc_vec_memory_invariants", "alloc_collections_memory_invariants"),
    "tests/negative/stdlib_negative_cases.vit": ("stdlib_negative_invalid_inputs", "stdlib_negative_boundaries", "stdlib_negative_overflow", "is_err", "is_none", "9223372036854775808", "range_step(0, 10, 0)"),
    "tests/fuzz/utf8_fuzz.vit": ("fuzz_utf8", "validate_utf8", "is_char_boundary"),
    "tests/fuzz/url_fuzz.vit": ("fuzz_url", "std_url.url_parse"),
    "tests/fuzz/csv_fuzz.vit": ("fuzz_csv", "std_csv.csv_parse"),
    "tests/fuzz/json_fuzz.vit": ("fuzz_json", "std_serialization.json_value"),
    "tests/fuzz/path_fuzz.vit": ("fuzz_path", "std_path.normalize"),
    "tests/fuzz/minimize.vit": ("form FuzzFailure", "minimize_failure", "fuzz_minimizer_smoke"),
    "tests/platform/posix_test.vit": ("platform_posix_test", "posix_available"),
    "tests/platform/windows_test.vit": ("platform_windows_test", "windows_available"),
    "tests/platform/wasm_test.vit": ("platform_wasm_test", "wasm_available"),
    "tests/platform/embedded_test.vit": ("platform_embedded_test", "embedded_available"),
    "benchmarks/index.vit": ("stdlib_benchmarks_smoke", "bench_vec_push_pop", "bench_string_push_concat", "bench_hashmap_insert_get", "bench_utf8_validate_decode", "bench_path_normalize_join", "bench_format_ints", "bench_parse_numbers"),
    "benchmarks/modules/vec_bench.vit": ("bench_vec_push_pop", "vec_with_capacity", "vec_push", "vec_pop"),
    "benchmarks/modules/string_bench.vit": ("bench_string_push_concat", "string_reserve", "string_push", "string_concat"),
    "benchmarks/modules/hashmap_bench.vit": ("bench_hashmap_insert_get", "hashmap_insert", "hashmap_get"),
    "benchmarks/modules/utf8_bench.vit": ("bench_utf8_validate_decode", "is_valid_utf8", "decode_utf8", "validate_utf8"),
    "benchmarks/modules/path_bench.vit": ("bench_path_normalize_join", "path_buf", "join", "normalize"),
    "benchmarks/modules/format_bench.vit": ("bench_format_ints", "format_int", "format_uint", "FormatBase.HexLower"),
    "benchmarks/modules/parse_bench.vit": ("bench_parse_numbers", "parse_i64", "is_ok"),
    "examples/stdlib_max.vit": ("stdlib_max_example", "std_uuid.uuid_nil", "std_path.path_buf", "std_random.prng"),
    "examples/public_module_examples.vit": ("PUBLIC_MODULE_EXAMPLE_COUNT", "public_module_examples_present", "src/vitte/stdlib/std/uuid.vitl"),
    "examples/stdlib_usage_examples.vit": ("stdlib_usage_examples", "std_uuid.uuid_v4", "std_base64.base64_standard", "std_csv.csv_record"),
}


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


def compiler_helper_inventory() -> dict[str, dict[str, list[str]]]:
    backend_markers = (
        "compiler_backend_",
        "compiler_platform_",
        "compiler_os_",
        "compiler_alloc_",
        "compiler_vec_",
        "compiler_box_",
        "compiler_rc_",
        "compiler_arc_",
        "compiler_mutex_",
        "compiler_rwlock_",
        "compiler_once_",
        "compiler_condvar_",
        "compiler_atomic_",
        "compiler_pair_",
    )
    helper_re = re.compile(r"\bcompiler_[A-Za-z0-9_]+")
    inventory: dict[str, dict[str, list[str]]] = {}
    for source in stdlib_sources():
        helpers = sorted(set(helper_re.findall(source.read_text(encoding="utf-8", errors="ignore"))))
        if not helpers:
            continue
        backend = [name for name in helpers if name.startswith(backend_markers)]
        temporary = [name for name in helpers if name not in backend]
        inventory[source.relative_to(ROOT).as_posix()] = {
            "backend_primitives": backend,
            "temporary_wrappers": temporary,
        }
    return inventory


def validate_compiler_helper_inventory(inventory: dict[str, dict[str, list[str]]]) -> list[ValidationResult]:
    prioritized = ("std/io.vitl", "std/process.vitl", "std/thread.vitl", "std/sync.vitl", "std/time.vitl", "std/env.vitl")
    temporary_in_prioritized: list[str] = []
    for module, entry in inventory.items():
        if any(module.endswith(path) for path in prioritized):
            temporary_in_prioritized.extend(
                f"{module}:{helper}"
                for helper in entry.get("temporary_wrappers", [])
                if not helper.startswith(("compiler_test_", "compiler_values_equal", "compiler_string_bytes"))
            )
    return [
        ValidationResult(
            name="compiler_helpers_inventory_generated",
            status=COMPILER_HELPERS_REPORT.is_file(),
            detail=str(COMPILER_HELPERS_REPORT.relative_to(ROOT)) if COMPILER_HELPERS_REPORT.is_file() else "missing",
        ),
        ValidationResult(
            name="prioritized_contractual_modules_split_backend_helpers",
            status=not temporary_in_prioritized,
            detail="ok" if not temporary_in_prioritized else "; ".join(temporary_in_prioritized[:8]),
        ),
    ]


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


def load_ci_matrix_manifest() -> dict:
    return json.loads(
        CI_MATRIX_MANIFEST.read_text(encoding="utf-8")
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
    if module.startswith("src/vitte/stdlib/"):
        candidate = ROOT / f"{module}.vitl"
        if candidate.exists():
            return candidate
        candidate = ROOT / f"{module}.vit"
        if candidate.exists():
            return candidate
        candidate = ROOT / module
        if candidate.exists():
            return candidate
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
    rules = manifest.get("rules", {})
    public_coverage_rules_enabled = (
        rules.get("public_modules_require_examples") is True
        and rules.get("public_modules_require_dedicated_tests") is True
    )
    results.append(ValidationResult(
        name="public_module_coverage_rules_enabled",
        status=public_coverage_rules_enabled,
        detail="ok" if public_coverage_rules_enabled else json.dumps(rules, sort_keys=True),
    ))

    def is_public_module(entry: str) -> bool:
        parts = Path(entry).parts
        return (
            entry.startswith("src/vitte/stdlib/")
            and entry.endswith((".vit", ".vitl"))
            and not any(part in parts for part in ("tests", "benchmarks", "examples", "generated", "tools"))
        )

    public_modules = sorted(path for path in official if is_public_module(path))
    coverage = manifest.get("public_module_coverage", {})
    modules_without_coverage = [
        module
        for module in public_modules
        if module not in coverage
    ]
    modules_without_examples: list[str] = []
    modules_without_tests: list[str] = []

    for module in public_modules:
        entry = coverage.get(module, {})
        example_path = ROOT / entry.get("example", "")
        test_path = ROOT / entry.get("test", "")

        if not example_path.is_file() or module not in example_path.read_text(encoding="utf-8", errors="ignore"):
            modules_without_examples.append(module)
        if not test_path.is_file() or module not in test_path.read_text(encoding="utf-8", errors="ignore"):
            modules_without_tests.append(module)

    results.append(ValidationResult(
        name="public_modules_have_examples",
        status=not modules_without_coverage and not modules_without_examples,
        detail="ok" if not modules_without_coverage and not modules_without_examples else "; ".join((modules_without_coverage + modules_without_examples)[:8]),
    ))
    results.append(ValidationResult(
        name="public_modules_have_dedicated_tests",
        status=not modules_without_coverage and not modules_without_tests,
        detail="ok" if not modules_without_coverage and not modules_without_tests else "; ".join((modules_without_coverage + modules_without_tests)[:8]),
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


def validate_core_convert_default_clone() -> list[ValidationResult]:
    convert_source = CORE_CONVERT_SOURCE.read_text(encoding="utf-8")
    default_source = CORE_DEFAULT_SOURCE.read_text(encoding="utf-8")
    clone_source = CORE_CLONE_SOURCE.read_text(encoding="utf-8")
    doc = CORE_CONVERT_DEFAULT_CLONE_DOC.read_text(encoding="utf-8")
    missing_convert = [
        fragment
        for fragment in REQUIRED_CONVERT_FRAGMENTS
        if fragment not in convert_source
    ]
    missing_default = [
        fragment
        for fragment in REQUIRED_DEFAULT_FRAGMENTS
        if fragment not in default_source
    ]
    missing_clone = [
        fragment
        for fragment in REQUIRED_CLONE_FRAGMENTS
        if fragment not in clone_source
    ]
    required_doc_fragments = (
        "core.convert",
        "From",
        "TryFrom",
        "Dangerous implicit conversions are forbidden",
        "conversion_impossible_error",
        "core.default",
        "Default<T>",
        "no clear neutral value",
        "core.clone",
        "Clone<T>",
        "Copy<T>",
        "avoid implicit clones",
    )
    missing_doc = [
        fragment
        for fragment in required_doc_fragments
        if fragment not in doc
    ]
    return [
        ValidationResult(
            name="core_convert_defines_conversion_traits",
            status=not missing_convert,
            detail="ok" if not missing_convert else ", ".join(missing_convert),
        ),
        ValidationResult(
            name="core_default_defines_meaningful_defaults",
            status=not missing_default,
            detail="ok" if not missing_default else ", ".join(missing_default),
        ),
        ValidationResult(
            name="core_clone_defines_copy_clone_contracts",
            status=not missing_clone,
            detail="ok" if not missing_clone else ", ".join(missing_clone),
        ),
        ValidationResult(
            name="core_convert_default_clone_documents_policy",
            status=not missing_doc,
            detail="ok" if not missing_doc else ", ".join(missing_doc),
        ),
    ]


def validate_core_drop_scope_memory() -> list[ValidationResult]:
    drop_source = CORE_DROP_SOURCE.read_text(encoding="utf-8")
    scope_source = CORE_SCOPE_SOURCE.read_text(encoding="utf-8")
    memory_source = (SOURCE_STDLIB_DIR / "core" / "memory.vitl").read_text(encoding="utf-8")
    doc = CORE_DROP_SCOPE_MEMORY_DOC.read_text(encoding="utf-8")
    missing_drop = [
        fragment
        for fragment in REQUIRED_DROP_FRAGMENTS
        if fragment not in drop_source
    ]
    missing_scope = [
        fragment
        for fragment in REQUIRED_SCOPE_FRAGMENTS
        if fragment not in scope_source
    ]
    missing_memory = [
        fragment
        for fragment in REQUIRED_MEMORY_FRAGMENTS
        if fragment not in memory_source
    ]
    required_doc_fragments = (
        "deterministic order",
        "Partial destruction",
        "Early exits",
        "Double destruction is forbidden",
        "defer",
        "panic unwinding",
        "unsafe_transmute",
        "unsafe_zeroed",
        "unsafe_uninitialized",
        "detect_size_compatible",
        "detect_alignment_compatible",
        "Unsafe Invariants",
    )
    missing_doc = [
        fragment
        for fragment in required_doc_fragments
        if fragment not in doc
    ]
    return [
        ValidationResult(
            name="core_drop_defines_destruction_protocol",
            status=not missing_drop,
            detail="ok" if not missing_drop else ", ".join(missing_drop),
        ),
        ValidationResult(
            name="core_scope_defines_defer_guarantees",
            status=not missing_scope,
            detail="ok" if not missing_scope else ", ".join(missing_scope),
        ),
        ValidationResult(
            name="core_memory_defines_layout_and_unsafe_api",
            status=not missing_memory,
            detail="ok" if not missing_memory else ", ".join(missing_memory),
        ),
        ValidationResult(
            name="core_drop_scope_memory_documents_invariants",
            status=not missing_doc,
            detail="ok" if not missing_doc else ", ".join(missing_doc),
        ),
    ]


def validate_core_iterator() -> list[ValidationResult]:
    iterator_source = CORE_ITERATOR_SOURCE.read_text(encoding="utf-8")
    missing_iterator = [
        fragment
        for fragment in REQUIRED_ITERATOR_FRAGMENTS
        if fragment not in iterator_source
    ]
    return [
        ValidationResult(
            name="core_iterator_defines_standard_iterator_contract",
            status=not missing_iterator,
            detail="ok" if not missing_iterator else ", ".join(missing_iterator),
        ),
    ]


def validate_core_range_number() -> list[ValidationResult]:
    range_source = CORE_RANGE_SOURCE.read_text(encoding="utf-8")
    number_source = CORE_NUMBER_SOURCE.read_text(encoding="utf-8")
    missing_range = [
        fragment
        for fragment in REQUIRED_RANGE_FRAGMENTS
        if fragment not in range_source
    ]
    missing_number = [
        fragment
        for fragment in REQUIRED_NUMBER_FRAGMENTS
        if fragment not in number_source
    ]
    return [
        ValidationResult(
            name="core_range_defines_bounds_and_range_kinds",
            status=not missing_range,
            detail="ok" if not missing_range else ", ".join(missing_range),
        ),
        ValidationResult(
            name="core_number_centralizes_numeric_operations",
            status=not missing_number,
            detail="ok" if not missing_number else ", ".join(missing_number),
        ),
    ]


def validate_core_float_math() -> list[ValidationResult]:
    float_source = CORE_FLOAT_SOURCE.read_text(encoding="utf-8")
    math_source = CORE_MATH_SOURCE.read_text(encoding="utf-8")
    missing_float = [
        fragment
        for fragment in REQUIRED_FLOAT_FRAGMENTS
        if fragment not in float_source
    ]
    missing_math = [
        fragment
        for fragment in REQUIRED_MATH_FRAGMENTS
        if fragment not in math_source
    ]
    return [
        ValidationResult(
            name="core_float_defines_float_classification",
            status=not missing_float,
            detail="ok" if not missing_float else ", ".join(missing_float),
        ),
        ValidationResult(
            name="core_math_defines_portable_math_contract",
            status=not missing_math,
            detail="ok" if not missing_math else ", ".join(missing_math),
        ),
    ]


def validate_core_string_ascii_unicode() -> list[ValidationResult]:
    string_source = CORE_STRING_SOURCE.read_text(encoding="utf-8")
    ascii_source = CORE_ASCII_SOURCE.read_text(encoding="utf-8")
    unicode_source = CORE_UNICODE_SOURCE.read_text(encoding="utf-8")
    missing_string = [
        fragment
        for fragment in REQUIRED_STRING_FRAGMENTS
        if fragment not in string_source
    ]
    missing_ascii = [
        fragment
        for fragment in REQUIRED_ASCII_FRAGMENTS
        if fragment not in ascii_source
    ]
    missing_unicode = [
        fragment
        for fragment in REQUIRED_UNICODE_FRAGMENTS
        if fragment not in unicode_source
    ]
    return [
        ValidationResult(
            name="core_string_defines_utf8_view_contract",
            status=not missing_string,
            detail="ok" if not missing_string else ", ".join(missing_string),
        ),
        ValidationResult(
            name="core_ascii_defines_ascii_character_contract",
            status=not missing_ascii,
            detail="ok" if not missing_ascii else ", ".join(missing_ascii),
        ),
        ValidationResult(
            name="core_unicode_defines_versioned_unicode_tables",
            status=not missing_unicode,
            detail="ok" if not missing_unicode else ", ".join(missing_unicode),
        ),
    ]


def validate_stdlib_next_steps() -> list[ValidationResult]:
    results: list[ValidationResult] = []
    for relative, fragments in REQUIRED_NEXT_STEP_FRAGMENTS.items():
        path = SOURCE_STDLIB_DIR / relative
        source = path.read_text(encoding="utf-8")
        missing = [
            fragment
            for fragment in fragments
            if fragment not in source
        ]
        results.append(ValidationResult(
            name="stdlib_next_step_" + relative.replace("/", "_").replace(".", "_"),
            status=not missing,
            detail="ok" if not missing else ", ".join(missing),
        ))

    doc = ROOT / "docs" / "compiler" / "stdlib_next_steps.md"
    doc_text = doc.read_text(encoding="utf-8")
    boundaries_text = (ROOT / "docs" / "compiler" / "stdlib_boundaries.md").read_text(encoding="utf-8")
    api_text = (ROOT / "docs" / "compiler" / "stdlib_api.md").read_text(encoding="utf-8")
    generated_api_text = (ROOT / "docs" / "compiler" / "stdlib_api.generated.md").read_text(encoding="utf-8")
    generator_text = (ROOT / "tools" / "stdlib" / "generate_api_docs.py").read_text(encoding="utf-8")
    doc_fragments = (
        "core slices",
        "allocation containers",
        "standard I/O",
        "generated Unicode tables",
        "`core` is the allocation-free layer",
        "`alloc` owns heap-backed containers",
        "`std` is the OS-facing layer",
        "Current Stabilized Surface",
        "Generated from `src/vitte/stdlib/stdlib_modules.json`",
        "collect_symbols",
    )
    combined_docs = "\n".join((doc_text, boundaries_text, api_text, generated_api_text, generator_text))
    missing_doc = [
        fragment
        for fragment in doc_fragments
        if fragment not in combined_docs
    ]
    results.append(ValidationResult(
        name="stdlib_next_steps_documented",
        status=not missing_doc,
        detail="ok" if not missing_doc else ", ".join(missing_doc),
    ))
    return results


def validate_ci_matrix(matrix: dict) -> list[ValidationResult]:
    targets = matrix.get("targets", [])
    platforms = {target.get("platform") for target in targets}
    required = {"posix", "windows", "wasm", "embedded"}
    commands = matrix.get("commands", [])
    rules = matrix.get("rules", {})
    platform_families = matrix.get("platform_families", {})
    required_family_commands = {
        f"python3 tools/stdlib/run_checks.py --platform-family {family}"
        for family in required
    }
    return [
        ValidationResult(
            name="stdlib_ci_matrix_platform_targets",
            status=required.issubset(platforms),
            detail="ok" if required.issubset(platforms) else ", ".join(sorted(required - platforms)),
        ),
        ValidationResult(
            name="stdlib_ci_matrix_runs_gate",
            status="make stdlib-gate" in commands and "python3 tools/stdlib/run_checks.py" in commands,
            detail=", ".join(commands),
        ),
        ValidationResult(
            name="stdlib_ci_matrix_dependency_rules",
            status=rules.get("core_has_no_alloc_std_platform_imports") is True
            and rules.get("std_uses_platform_for_os_access") is True
            and rules.get("api_index_generated") is True
            and rules.get("platform_family_matrix_required") is True,
            detail=json.dumps(rules, sort_keys=True),
        ),
        ValidationResult(
            name="stdlib_ci_matrix_runs_each_platform_family",
            status=required_family_commands.issubset(set(commands))
            and required.issubset(set(platform_families.keys())),
            detail="ok" if required_family_commands.issubset(set(commands)) and required.issubset(set(platform_families.keys())) else ", ".join(sorted(required_family_commands - set(commands))),
        ),
    ]


def validate_std_platform_access(manifest: dict) -> list[ValidationResult]:
    std_sources = [
        source
        for source in stdlib_sources()
        if path_level(source, manifest) == "std"
    ]
    violations: list[str] = []
    direct_kernel_imports: list[str] = []
    for source in std_sources:
        text = source.read_text(encoding="utf-8", errors="ignore")
        imports = imports_for_source(source)
        direct_kernel_imports.extend(
            f"{source.relative_to(ROOT)} imports {imported}"
            for imported in imports
            if imported.startswith(("src/vitte/stdlib/kernel", "vitte/stdlib/kernel"))
        )
        for pattern in STD_RAW_PLATFORM_PATTERNS:
            if re.search(pattern, text):
                violations.append(f"{source.relative_to(ROOT)} matches {pattern}")
    return [
        ValidationResult(
            name="std_does_not_bypass_platform",
            status=not violations,
            detail="ok" if not violations else "; ".join(violations[:5]),
        ),
        ValidationResult(
            name="std_never_depends_on_kernel_outside_platform",
            status=not direct_kernel_imports,
            detail="ok" if not direct_kernel_imports else "; ".join(direct_kernel_imports[:5]),
        ),
    ]


def validate_explicit_external_imports() -> list[ValidationResult]:
    required_imports = {
        "Vec": "src/vitte/stdlib/alloc/vec",
        "String": "src/vitte/stdlib/alloc/string",
        "HashMap": "src/vitte/stdlib/alloc/collections",
        "HashSet": "src/vitte/stdlib/alloc/collections",
        "Box": "src/vitte/stdlib/alloc/box",
        "Option": "src/vitte/stdlib/core/option",
        "Result": "src/vitte/stdlib/core/result",
        "Iterator": "src/vitte/stdlib/core/iterator",
        "Utf8View": "src/vitte/stdlib/core/string",
        "PathBuf": "src/vitte/stdlib/std/path",
        "Path": "src/vitte/stdlib/std/fs",
        "Reader": "src/vitte/stdlib/std/io",
        "Writer": "src/vitte/stdlib/std/io",
        "IoError": "src/vitte/stdlib/std/io",
        "Url": "src/vitte/stdlib/std/url",
        "SystemTime": "src/vitte/stdlib/std/time",
    }
    scoped_roots = (
        SOURCE_STDLIB_DIR / "alloc",
        SOURCE_STDLIB_DIR / "std",
        SOURCE_STDLIB_DIR / "platform",
    )
    violations: list[str] = []
    for source in stdlib_sources():
        if not any(source.is_relative_to(root) for root in scoped_roots):
            continue
        text = source.read_text(encoding="utf-8", errors="ignore")
        imports = set(imports_for_source(source))
        for token, required in required_imports.items():
            required_path = ROOT / required
            same_module = source.as_posix().startswith(required_path.as_posix())
            if same_module:
                continue
            if re.search(rf"\b{re.escape(token)}\b", text) and required not in imports:
                violations.append(f"{source.relative_to(ROOT)} uses {token} without {required}")
    return [
        ValidationResult(
            name="std_alloc_external_types_have_explicit_imports",
            status=not violations,
            detail="ok" if not violations else "; ".join(violations[:8]),
        )
    ]


def validate_lsp_api_index() -> list[ValidationResult]:
    api_text = (ROOT / "docs" / "compiler" / "stdlib_api.generated.md").read_text(encoding="utf-8")
    api_json = json.loads((ROOT / "docs" / "compiler" / "stdlib_api.generated.json").read_text(encoding="utf-8"))
    lsp_text = (ROOT / "docs" / "compiler" / "stdlib_lsp_index.md").read_text(encoding="utf-8")
    lsp_json = json.loads((ROOT / "docs" / "compiler" / "stdlib_lsp_index.generated.json").read_text(encoding="utf-8"))
    required_api = ("signature `", "example `", "executable_example `", "stability `", "std/serialization.vitl", "std/atomic.vitl")
    required_lsp = ("symbol name", "public signature", "minimal usage example", "executable example", "stability", "source line")
    missing_api = [fragment for fragment in required_api if fragment not in api_text]
    missing_lsp = [fragment for fragment in required_lsp if fragment not in lsp_text]
    json_ok = api_json.get("schema") == "vitte.stdlib.api" and bool(api_json.get("modules"))
    lsp_ok = lsp_json.get("schema") == "vitte.stdlib.lsp-index" and bool(lsp_json.get("symbols"))
    required_symbol_fields = {"id", "name", "kind", "signature", "module", "line", "example", "executable_example", "stability", "visibility"}
    json_symbols = [
        symbol
        for module in api_json.get("modules", [])
        for symbol in module.get("symbols", [])
    ]
    api_structured_ok = all(required_symbol_fields.issubset(symbol) for symbol in json_symbols)
    lsp_structured_ok = all(required_symbol_fields.issubset(symbol) for symbol in lsp_json.get("symbols", []))
    return [
        ValidationResult(
            name="stdlib_lsp_api_index_generated",
            status=not missing_api and not missing_lsp and json_ok and lsp_ok and api_structured_ok and lsp_structured_ok,
            detail="ok" if not missing_api and not missing_lsp and json_ok and lsp_ok and api_structured_ok and lsp_structured_ok else ", ".join(missing_api + missing_lsp + ["missing structured fields"]),
        )
    ]


def validate_stdlib_max_artifacts() -> list[ValidationResult]:
    paths = {
        "examples": SOURCE_STDLIB_DIR / "examples" / "stdlib_max.vit",
        "snapshot": SOURCE_STDLIB_DIR / "tests" / "snapshots" / "stdlib_api.snap",
        "fuzz_utf8": SOURCE_STDLIB_DIR / "tests" / "fuzz" / "utf8_url_csv.vit",
        "fuzz_path": SOURCE_STDLIB_DIR / "tests" / "fuzz" / "path_json_parse.vit",
        "stability": ROOT / "docs" / "compiler" / "stdlib_api_stability.md",
        "contributing": ROOT / "docs" / "compiler" / "stdlib_contributing.md",
        "changelog": ROOT / "docs" / "compiler" / "stdlib_changelog.md",
    }
    missing = [name for name, path in paths.items() if not path.exists()]
    stability_text = paths["stability"].read_text(encoding="utf-8")
    contributing_text = paths["contributing"].read_text(encoding="utf-8")
    required_fragments = (
        "public names are not removed",
        "each public module must have a test reference",
        "each public module must have an example reference",
        "Do not add direct OS access in `std`",
        "Module Stability Classes",
        "Panic Documentation Contract",
        "Review Checklist",
        "Fuzzing",
    )
    missing_fragments = [
        fragment
        for fragment in required_fragments
        if fragment not in stability_text + contributing_text
    ]
    fuzz_files = {
        "utf8": SOURCE_STDLIB_DIR / "tests" / "fuzz" / "utf8_fuzz.vit",
        "url": SOURCE_STDLIB_DIR / "tests" / "fuzz" / "url_fuzz.vit",
        "csv": SOURCE_STDLIB_DIR / "tests" / "fuzz" / "csv_fuzz.vit",
        "json": SOURCE_STDLIB_DIR / "tests" / "fuzz" / "json_fuzz.vit",
        "path": SOURCE_STDLIB_DIR / "tests" / "fuzz" / "path_fuzz.vit",
    }
    missing_fuzz = [name for name, path in fuzz_files.items() if not path.is_file()]
    corpus_files = {
        "utf8": SOURCE_STDLIB_DIR / "tests" / "fuzz" / "corpus" / "utf8.seed",
        "url": SOURCE_STDLIB_DIR / "tests" / "fuzz" / "corpus" / "url.seed",
        "csv": SOURCE_STDLIB_DIR / "tests" / "fuzz" / "corpus" / "csv.seed",
        "json": SOURCE_STDLIB_DIR / "tests" / "fuzz" / "corpus" / "json.seed",
        "path": SOURCE_STDLIB_DIR / "tests" / "fuzz" / "corpus" / "path.seed",
    }
    missing_corpus = [name for name, path in corpus_files.items() if not path.is_file() or not path.read_text(encoding="utf-8", errors="ignore").strip()]
    minimizer = SOURCE_STDLIB_DIR / "tests" / "fuzz" / "minimize.vit"
    minimizer_ok = minimizer.is_file() and "minimize_failure" in minimizer.read_text(encoding="utf-8", errors="ignore")
    snapshot_dir = SOURCE_STDLIB_DIR / "tests" / "snapshots" / "modules"
    required_snapshots = {"alloc.snap", "io.snap", "fs.snap", "path.snap", "platform.snap"}
    present_snapshots = {path.name for path in snapshot_dir.glob("*.snap")} if snapshot_dir.is_dir() else set()
    missing_snapshots = sorted(required_snapshots - present_snapshots)
    threshold_path = SOURCE_STDLIB_DIR / "benchmarks" / "thresholds.json"
    history_path = SOURCE_STDLIB_DIR / "benchmarks" / "history.json"
    threshold_data = json.loads(threshold_path.read_text(encoding="utf-8")) if threshold_path.is_file() else {}
    history_data = json.loads(history_path.read_text(encoding="utf-8")) if history_path.is_file() else {}
    bench_names = {"vec", "string", "hashmap", "utf8", "path", "format", "parse"}
    threshold_ok = bench_names.issubset(set(threshold_data.get("thresholds", {}).keys()))
    history_ok = bench_names.issubset(set(history_data.get("baseline", {}).keys())) and "max_regression_percent" in history_data
    invariant_text = (SOURCE_STDLIB_DIR / "tests" / "alloc_memory_invariants.vit").read_text(encoding="utf-8", errors="ignore")
    invariant_ok = all(fragment in invariant_text for fragment in ("vec_len", "string_capacity", "hashmap_len", "deque_len"))
    changelog_text = paths["changelog"].read_text(encoding="utf-8")
    changelog_ok = "Generated from commits touching" in changelog_text and "Recent Changes" in changelog_text
    examples_ok = (SOURCE_STDLIB_DIR / "examples" / "stdlib_usage_examples.vit").is_file()

    panic_doc = stability_text + contributing_text
    public_panic_violations: list[str] = []
    for source in stdlib_sources():
        rel = source.relative_to(ROOT).as_posix()
        if "/tests/" in rel or "/examples/" in rel or "/benchmarks/" in rel:
            continue
        text = source.read_text(encoding="utf-8", errors="ignore")
        if re.search(r"\bproc\s+[A-Za-z0-9_]*panic[A-Za-z0-9_]*\b", text) or re.search(r"\bpanic\s*\(", text):
            if rel not in panic_doc and "public panic" not in panic_doc:
                public_panic_violations.append(rel)

    return [
        ValidationResult(
            name="stdlib_max_artifacts_present",
            status=not missing and not missing_fragments,
            detail="ok" if not missing and not missing_fragments else ", ".join(missing + missing_fragments),
        ),
        ValidationResult(
            name="stdlib_fuzzing_covers_utf8_url_csv_json_path",
            status=not missing_fuzz,
            detail="ok" if not missing_fuzz else ", ".join(missing_fuzz),
        ),
        ValidationResult(
            name="stdlib_fuzzing_has_seed_corpus_and_minimizer",
            status=not missing_corpus and minimizer_ok,
            detail="ok" if not missing_corpus and minimizer_ok else ", ".join(missing_corpus + ([] if minimizer_ok else ["minimizer"])),
        ),
        ValidationResult(
            name="stdlib_module_snapshots_present",
            status=not missing_snapshots,
            detail="ok" if not missing_snapshots else ", ".join(missing_snapshots),
        ),
        ValidationResult(
            name="stdlib_benchmarks_have_thresholds",
            status=threshold_ok,
            detail="ok" if threshold_ok else "missing benchmark thresholds",
        ),
        ValidationResult(
            name="stdlib_benchmark_history_compared",
            status=history_ok,
            detail="ok" if history_ok else "missing benchmark history baseline",
        ),
        ValidationResult(
            name="stdlib_alloc_memory_invariants_checked",
            status=invariant_ok,
            detail="ok" if invariant_ok else "missing alloc invariant fragments",
        ),
        ValidationResult(
            name="stdlib_changelog_generated_from_commits",
            status=changelog_ok,
            detail="ok" if changelog_ok else "missing generated changelog marker",
        ),
        ValidationResult(
            name="stdlib_examples_present",
            status=examples_ok,
            detail="ok" if examples_ok else "missing stdlib_usage_examples.vit",
        ),
        ValidationResult(
            name="public_panic_requires_documented_diagnostic",
            status=not public_panic_violations,
            detail="ok" if not public_panic_violations else "; ".join(public_panic_violations[:8]),
        ),
    ]


def validate_real_vitte_implementations() -> list[ValidationResult]:
    checks = {
        SOURCE_STDLIB_DIR / "std" / "uuid.vitl": (
            "proc uuid_nil() -> Uuid { give compiler_uuid_nil(); }",
            "proc uuid_is_nil(value: Uuid) -> bool { give compiler_uuid_is_nil(value); }",
        ),
        SOURCE_STDLIB_DIR / "std" / "calendar.vitl": (
            "compiler_calendar_is_leap_year",
            "compiler_calendar_days_in_month",
        ),
        SOURCE_STDLIB_DIR / "std" / "units.vitl": (
            "compiler_units_convert",
        ),
        SOURCE_STDLIB_DIR / "std" / "metrics.vitl": (
            "compiler_metrics_render_counter",
        ),
        SOURCE_STDLIB_DIR / "alloc" / "collections.vitl": (
            "compiler_hashmap_new",
            "compiler_hashmap_insert",
            "compiler_hashmap_get",
            "compiler_hashmap_remove",
            "compiler_btreemap_new",
            "compiler_btreemap_insert",
            "compiler_btreemap_get",
            "compiler_btreemap_remove",
            "compiler_deque_push_front",
            "compiler_deque_pop_back",
            "compiler_deque_pop_front",
        ),
        SOURCE_STDLIB_DIR / "std" / "path.vitl": (
            "compiler_path_normalize(path_value)",
            "compiler_path_join(base, child)",
            "compiler_path_parent(path_value)",
            "compiler_path_file_name(path_value)",
            "compiler_path_extension(path_value)",
            "compiler_path_with_extension(path_value, ext)",
        ),
        SOURCE_STDLIB_DIR / "std" / "fs.vitl": (
            "compiler_fs_read",
            "compiler_fs_write",
            "compiler_fs_metadata",
        ),
        SOURCE_STDLIB_DIR / "std" / "net.vitl": (
            "compiler_net_ip_addr",
            "compiler_net_dns_lookup",
            "compiler_net_tcp_connect",
            "compiler_net_udp_bind",
        ),
        SOURCE_STDLIB_DIR / "std" / "http.vitl": (
            "compiler_http_request",
            "compiler_http_response",
            "compiler_http_get",
            "compiler_http_send",
        ),
        SOURCE_STDLIB_DIR / "std" / "serialization.vitl": (
            "compiler_json_encode<T>",
            "compiler_json_decode<T>",
            "compiler_json_value(input)",
            "compiler_json_write_string",
        ),
        SOURCE_STDLIB_DIR / "std" / "base64.vitl": (
            "compiler_base64_encode(bytes, config)",
            "compiler_base64_decode(text, config)",
        ),
        SOURCE_STDLIB_DIR / "std" / "csv.vitl": (
            "compiler_csv_parse(text, options)",
            "compiler_csv_write(records, options)",
        ),
        SOURCE_STDLIB_DIR / "std" / "uuid.vitl": (
            "compiler_uuid_v4",
            "compiler_uuid_parse(text)",
            "compiler_uuid_to_string(value)",
        ),
        SOURCE_STDLIB_DIR / "std" / "semver.vitl": (
            "compiler_semver_parse(text)",
            "compiler_semver_req_parse(text)",
            "compiler_semver_matches(req, value)",
            "compiler_semver_compare(left, right)",
        ),
        SOURCE_STDLIB_DIR / "std" / "format.vitl": (
            "compiler_format<T>",
            "compiler_debug<T>",
            "compiler_format_pad_left",
            "compiler_format_int",
            "compiler_format_uint",
        ),
        SOURCE_STDLIB_DIR / "std" / "parse.vitl": (
            "compiler_parse_bool",
            "compiler_parse_i64(text, base)",
            "compiler_parse_u64(text, base)",
            "compiler_parse_f64(text)",
            "compiler_parse_utf8(bytes)",
        ),
        SOURCE_STDLIB_DIR / "std" / "random.vitl": (
            "compiler_random_os_entropy",
            "compiler_random_u64_to_unit_f64",
        ),
        SOURCE_STDLIB_DIR / "std" / "testing.vitl": (
            "compiler_testing_assert_true",
            "compiler_testing_assert_eq",
            "compiler_testing_assert_snapshot",
            "compiler_testing_parameterized",
        ),
        SOURCE_STDLIB_DIR / "std" / "bench.vitl": (
            "compiler_bench_run",
            "compiler_bench_report_text",
        ),
        SOURCE_STDLIB_DIR / "std" / "mime.vitl": (
            "compiler_mime_text_plain",
            "compiler_mime_application_json",
        ),
    }
    violations: list[str] = []
    for path, forbidden in checks.items():
        text = path.read_text(encoding="utf-8")
        for fragment in forbidden:
            if fragment in text:
                violations.append(f"{path.relative_to(ROOT)} keeps {fragment}")

    module_tests = sorted((SOURCE_STDLIB_DIR / "tests" / "modules").glob("*.vit"))
    runner_text = (SOURCE_STDLIB_DIR / "tests" / "module_runner.vit").read_text(encoding="utf-8")
    missing_runner_refs = [
        path.stem
        for path in module_tests
        if path.stem not in runner_text
    ]
    return [
        ValidationResult(
            name="stdlib_replaces_simple_compiler_surfaces",
            status=not violations,
            detail="ok" if not violations else "; ".join(violations[:5]),
        ),
        ValidationResult(
            name="stdlib_has_executable_module_tests",
            status=len(module_tests) >= 6 and not missing_runner_refs,
            detail=f"tests={len(module_tests)}" if not missing_runner_refs else ", ".join(missing_runner_refs),
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
    core_no_std_violations: list[str] = []
    for source in core_sources:
        text = source.read_text(encoding="utf-8", errors="ignore")
        for imported in imports_for_source(source):
            if imported.startswith(CORE_FORBIDDEN_IMPORT_PREFIXES):
                core_os_violations.append(f"{source.relative_to(ROOT)} imports {imported}")
            if imported.startswith(("src/vitte/stdlib/std", "vitte/stdlib/std", "src/vitte/stdlib/platform", "vitte/stdlib/platform")):
                core_no_std_violations.append(f"{source.relative_to(ROOT)} imports {imported}")
        for pattern in CORE_DYNAMIC_ALLOCATION_PATTERNS:
            if re.search(pattern, text):
                core_alloc_violations.append(f"{source.relative_to(ROOT)} matches {pattern}")
        for forbidden in ("compiler_platform_family", "compiler_posix_", "compiler_windows_", "compiler_wasm_", "compiler_embedded_", "compiler_kernel_"):
            if forbidden in text:
                core_no_std_violations.append(f"{source.relative_to(ROOT)} uses {forbidden}")

    results.append(ValidationResult(
        name="core_has_no_operating_system_dependency",
        status=not core_os_violations,
        detail="ok" if not core_os_violations else "; ".join(core_os_violations[:5]),
    ))
    core_forbidden_level_imports = [
        f"{source.relative_to(ROOT)} imports {imported}"
        for source in core_sources
        for imported in imports_for_source(source)
        if imported.startswith(("vitte/stdlib/alloc", "vitte/stdlib/std", "vitte/stdlib/platform"))
    ]
    results.append(ValidationResult(
        name="core_has_no_alloc_std_platform_imports",
        status=not core_forbidden_level_imports,
        detail="ok" if not core_forbidden_level_imports else "; ".join(core_forbidden_level_imports[:5]),
    ))
    results.append(ValidationResult(
        name="core_has_no_dynamic_allocation",
        status=not core_alloc_violations,
        detail="ok" if not core_alloc_violations else "; ".join(core_alloc_violations[:5]),
    ))
    results.append(ValidationResult(
        name="core_no_std_strict",
        status=not core_no_std_violations,
        detail="ok" if not core_no_std_violations else "; ".join(core_no_std_violations[:5]),
    ))
    platform_families = set(manifest.get("rules", {}).get("platform_families_real", []))
    platform_sources = {
        "posix": SOURCE_STDLIB_DIR / "platform" / "posix.vitl",
        "windows": SOURCE_STDLIB_DIR / "platform" / "windows.vitl",
        "wasm": SOURCE_STDLIB_DIR / "platform" / "wasm.vitl",
        "embedded": SOURCE_STDLIB_DIR / "platform" / "embedded.vitl",
    }
    missing_platforms = [
        family
        for family, path in platform_sources.items()
        if family not in platform_families or not path.is_file() or f"{family}_available" not in path.read_text(encoding="utf-8", errors="ignore")
    ]
    results.append(ValidationResult(
        name="platform_families_real",
        status=not missing_platforms,
        detail="ok" if not missing_platforms else ", ".join(missing_platforms),
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
    ci_matrix_manifest = load_ci_matrix_manifest()
    compiler_inventory = compiler_helper_inventory()
    COMPILER_HELPERS_REPORT.write_text(json.dumps({
        "schema": "vitte.stdlib.compiler-helpers",
        "schema_version": "1.0.0",
        "modules": compiler_inventory,
    }, indent=2), encoding="utf-8")

    files = validate_files()
    architecture = validate_architecture(architecture_manifest)
    std_platform_results = validate_std_platform_access(architecture_manifest)
    explicit_import_results = validate_explicit_external_imports()
    ci_matrix_results = validate_ci_matrix(ci_matrix_manifest)
    lsp_api_results = validate_lsp_api_index()
    max_artifact_results = validate_stdlib_max_artifacts()
    real_impl_results = validate_real_vitte_implementations()
    compiler_helper_results = validate_compiler_helper_inventory(compiler_inventory)
    module_results = validate_module_manifest(module_manifest)
    graph_results = validate_dependency_graph_manifest(
        dependency_graph_manifest,
        stdlib_dependency_edges(architecture_manifest),
    )
    primitive_results = validate_core_primitives()
    option_result_results = validate_core_option_result()
    convert_default_clone_results = validate_core_convert_default_clone()
    drop_scope_memory_results = validate_core_drop_scope_memory()
    iterator_results = validate_core_iterator()
    range_number_results = validate_core_range_number()
    float_math_results = validate_core_float_math()
    string_ascii_unicode_results = validate_core_string_ascii_unicode()
    next_step_results = validate_stdlib_next_steps()

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
        for item in architecture + std_platform_results + explicit_import_results + ci_matrix_results + lsp_api_results + max_artifact_results + real_impl_results + compiler_helper_results + module_results + graph_results + primitive_results + option_result_results + convert_default_clone_results + drop_scope_memory_results + iterator_results + range_number_results + float_math_results + string_ascii_unicode_results + next_step_results
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
        "roadmap_features":
            ROADMAP_FEATURE_MODULES,
        "non_scope_features":
            NON_SCOPE_FEATURE_MODULES,
        "architecture":
            architecture_manifest,
        "module_manifest":
            module_manifest,
        "dependency_graph":
            dependency_graph_manifest,
        "ci_matrix":
            ci_matrix_manifest,
        "compiler_helpers":
            compiler_inventory,
        "required_files": [
            asdict(item)
            for item in files
        ],
        "architecture_results": [
            asdict(item)
            for item in architecture + std_platform_results + explicit_import_results + ci_matrix_results + lsp_api_results + max_artifact_results + real_impl_results + compiler_helper_results + module_results + graph_results + primitive_results + option_result_results + convert_default_clone_results + drop_scope_memory_results + iterator_results + range_number_results + float_math_results + string_ascii_unicode_results + next_step_results
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
