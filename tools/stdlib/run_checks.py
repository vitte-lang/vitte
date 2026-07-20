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
    SOURCE_STDLIB_DIR / "alloc" / "vec.vitl",
    SOURCE_STDLIB_DIR / "alloc" / "string.vitl",
    SOURCE_STDLIB_DIR / "alloc" / "rc.vitl",
    SOURCE_STDLIB_DIR / "alloc" / "arc.vitl",
    SOURCE_STDLIB_DIR / "std" / "io.vitl",
    SOURCE_STDLIB_DIR / "std" / "fs.vitl",
    SOURCE_STDLIB_DIR / "std" / "env.vitl",
    SOURCE_STDLIB_DIR / "std" / "time.vitl",
    SOURCE_STDLIB_DIR / "std" / "process.vitl",
    SOURCE_STDLIB_DIR / "platform" / "abi.vitl",
    SOURCE_STDLIB_DIR / "generated" / "unicode_tables.vitl",
    SOURCE_STDLIB_DIR / "tools" / "unicode_tables.vitl",
    SOURCE_STDLIB_DIR / "tests" / "api_contracts.vit",
    SOURCE_STDLIB_DIR / "tests" / "core_alloc_contracts.vit",
    ROOT / "tools" / "stdlib" / "generate_api_docs.py",
    ROOT / "docs" / "compiler" / "stdlib_next_steps.md",
    ROOT / "docs" / "compiler" / "stdlib_boundaries.md",
    ROOT / "docs" / "compiler" / "stdlib_api.md",
    ROOT / "docs" / "compiler" / "stdlib_api.generated.md",
)

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
    "proc utf8_view",
    "proc byte_length",
    "proc char_length",
    "proc bytes",
    "proc chars",
    "proc char_indices",
    "proc lines",
    "proc split",
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
)
REQUIRED_NEXT_STEP_FRAGMENTS = {
    "core/slice.vitl": ("form Slice<T>", "proc get<T>", "proc chunks<T>", "proc windows<T>", "proc binary_search<T>", "proc sort_unstable<T>", "proc sort_stable<T>"),
    "core/array.vitl": ("form Array<T>", "proc array_len<T", "proc array_get<T", "proc array_sort<T"),
    "core/cmp.vitl": ("pick Ordering", "form Eq<T>", "form Ord<T>", "proc compare<T>"),
    "core/hash.vitl": ("form Hasher", "form Hash<T>", "proc hash<T>", "proc combine_hash"),
    "alloc/box.vitl": ("form Box<T>", "proc box_new<T>", "proc box_drop<T>"),
    "alloc/vec.vitl": ("form Vec<T>", "proc vec_new<T>", "proc vec_push<T>", "proc vec_iter<T>", "proc vec_drop<T>", "compiler_vec_realloc"),
    "alloc/string.vitl": ("form String", "proc string_new", "proc string_push", "proc string_concat", "proc string_slice", "proc string_as_utf8_view"),
    "alloc/rc.vitl": ("form Rc<T>", "form Weak<T>", "proc rc_new<T>", "proc weak_upgrade<T>"),
    "alloc/arc.vitl": ("form Arc<T>", "form ArcWeak<T>", "proc arc_new<T>", "proc arc_weak_upgrade<T>"),
    "std/io.vitl": ("form IoError", "form Reader", "form Writer", "proc read_to_string"),
    "std/fs.vitl": ("form Path", "form Metadata", "form FsError", "proc read_to_string"),
    "std/env.vitl": ("form EnvError", "proc args", "proc var", "proc current_dir"),
    "std/time.vitl": ("form Duration", "form Instant", "proc instant_now", "proc elapsed"),
    "std/process.vitl": ("form ExitStatus", "form Command", "proc command", "proc exit"),
    "platform/abi.vitl": ("form PlatformInfo", "form PlatformFeature", "proc platform_info", "proc supports_filesystem"),
    "generated/unicode_tables.vitl": ("GENERATED_UNICODE_VERSION", "proc generated_unicode_category", "proc generated_unicode_properties"),
    "tools/unicode_tables.vitl": ("form UnicodeTableGeneration", "proc generate_unicode_tables", "proc verify_unicode_tables"),
    "tests/api_contracts.vit": ("stdlib_api_contracts_smoke", "std_time.duration_from_secs", "platform_abi.supports_filesystem"),
    "tests/core_alloc_contracts.vit": ("stdlib_core_alloc_contracts_smoke", "alloc_vec.vec_push", "alloc_string.string_push"),
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
        for item in architecture + module_results + graph_results + primitive_results + option_result_results + convert_default_clone_results + drop_scope_memory_results + iterator_results + range_number_results + float_math_results + string_ascii_unicode_results + next_step_results
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
            for item in architecture + module_results + graph_results + primitive_results + option_result_results + convert_default_clone_results + drop_scope_memory_results + iterator_results + range_number_results + float_math_results + string_ascii_unicode_results + next_step_results
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
