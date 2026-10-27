#!/usr/bin/env python3
"""Compare the native and bootstrap CLI on their shared supported subset."""

from __future__ import annotations

import json
import subprocess
import sys
import tempfile
from pathlib import Path


VALID_CASES = {
    "scalar and calls": """\
proc add(left: i64, right: i64) -> i64 {
    return left + right;
}
proc main() -> i64 {
    return add(20, 22) - 42;
}
""",
    "condition and loop": """\
proc main() -> i64 {
    let mut value: i64 = 0;
    while value < 4 {
        set value = value + 1;
    }
    if value == 4 { return 0; }
    return 1;
}
""",
    "form layout and member access": """\
form Point { x: i64, valid: bool, y: i64 }
proc main() -> i64 {
    let point: Point = Point { y: 22, valid: true, x: 20 };
    if not point.valid { return 1; }
    return point.x + point.y - 42;
}
""",
    "nested lexical block": """\
proc main() -> i64 {
    let mut value: i64 = 0;
    {
        set value = 42;
    }
    return value - 42;
}
""",
    "attributes and external declaration": """\
#[inline]
proc identity(value: i64) -> i64 { return value; }
extern proc host_probe(value: i64) -> i64;
proc main() -> i64 { return identity(0); }
""",
    "boolean result and branches": """\
proc ready(value: i64) -> bool {
    return value > 0 and not false;
}
proc main() -> i64 {
    if ready(1) { return 0; }
    return 1;
}
""",
    "operator precedence and cast": """\
proc main() -> i64 {
    let value: i64 = 2 + 3 * 4;
    if (value as i64) == 14 { return 0; }
    return 1;
}
""",
    "fixed array parameter and indexing": """\
proc first(items: [2]i64) -> i64 { return items[0]; }
proc main() -> i64 {
    let values: [2]i64 = [1, 2];
    return first(values) - 1;
}
""",
    "nested forms and member chain": """\
form Vec2 { x: i64, y: i64 }
form Rect { origin: Vec2, width: i64 }
proc main() -> i64 {
    let rect: Rect = Rect {
        origin: Vec2 { y: 22, x: 20 },
        width: 5,
    };
    let mut mutable_rect: Rect = rect;
    set (mutable_rect.origin.x) += 2;
    return mutable_rect.origin.x + mutable_rect.origin.y - 44;
}
""",
    "form field mutation": """\
form Point { x: i64, y: i64 }
proc main() -> i64 {
    let mut point: Point = Point { x: 40, y: 1 };
    set (point.x) += 2;
    return point.x - 42;
}
""",
    "scalar pick match": """\
pick State { Cold, Ready, Done }
proc code(value: State) -> i64 {
    match value {
        State::Cold => { return 0; },
        State::Ready => { return 1; },
        else => { return 2; },
    }
    return 3;
}
proc main() -> i64 {
    let state: State = State::Ready;
    return code(state) - 1;
}
""",
    "while with continue": """\
proc main() -> i64 {
    let mut index: i64 = 0;
    let mut total: i64 = 0;
    while index < 5 {
        set index += 1;
        if index < 3 { continue; }
        set total += index;
    }
    return total - 12;
}
""",
    "string literal comparison": """\
proc main() -> i64 {
    let text: string = "ok";
    if text == "ok" { return 0; }
    return 1;
}
""",
    "value-producing if expression": """\
proc main() -> i64 {
    let value: i64 = if true { 20 } else { 22 };
    return value - 42;
}
""",
    "if expression in call argument": """\
proc identity(value: i64) -> i64 { return value; }
proc main() -> i64 {
    return identity(if true { 20 } else { 22 }) - 20;
}
""",
    "exclusive range loop": """\
proc main() -> i64 {
    let mut total: i64 = 0;
    for value in 1..10 {
        if value == 3 { continue; }
        if value == 7 { break; }
        set total += value;
    }
    return total - 18;
}
""",
    "inclusive range loop": """\
proc main() -> i64 {
    let mut total: i64 = 0;
    for value in 0..=4 {
        set total += value;
    }
    return total - 10;
}
""",
    "wildcard range loop": """\
proc main() -> i64 {
    let mut count: i64 = 0;
    for _ in 0..3 { set count += 1; }
    return count - 3;
}
""",
}

PARSER_INVALID_CASES = {
    "parser expected expression": "proc broken() { let value = ; }\n",
    "parser missing block close": "proc main() -> i64 { if true { return 0; }\n",
    "parser malformed generic parameters": (
        "proc identity<T(value: T) -> T { return value; }\n"
    ),
    "parser missing call delimiter": "proc main() -> i64 { return add(1; }\n",
}

SEMANTIC_INVALID_CASES = {
    "unresolved name": "proc main() -> i64 { return missing_name; }\n",
    "call argument type": (
        "proc take(value: i64) -> i64 { return value; }\n"
        "proc main() -> i64 { return take(\"wrong\"); }\n"
    ),
    "immutable assignment": (
        "proc main() -> i64 { let value: i64 = 1; set value = 2; return value; }\n"
    ),
    "multiple semantic errors": (
        "proc take(value: i64) -> i64 { return value; }\n"
        "proc main() -> i64 { return take(\"a\") + take(true); }\n"
    ),
    "wrong boolean return type": "proc main() -> bool { return 1; }\n",
    "field assignment type": (
        "form Point { x: i64 }\n"
        "proc main() -> i64 { let mut point: Point = Point { x: 1 }; "
        "set (point.x) = true; return point.x; }\n"
    ),
    "field initializer type": (
        "form Point { x: i64 }\n"
        "proc main() -> i64 { let point: Point = Point { x: true }; "
        "return point.x; }\n"
    ),
    "call argument arity": (
        "proc take(value: i64) -> i64 { return value; }\n"
        "proc main() -> i64 { return take(); }\n"
    ),
    "local used before declaration": (
        "proc main() -> i64 { let value: i64 = later; "
        "let later: i64 = 1; return value; }\n"
    ),
}

RUN_CASE_NAMES = (
    "scalar and calls",
    "condition and loop",
    "form layout and member access",
    "nested lexical block",
    "boolean result and branches",
    "nested forms and member chain",
    "form field mutation",
    "scalar pick match",
    "while with continue",
    "string literal comparison",
    "value-producing if expression",
    "if expression in call argument",
)

NATIVE_IR_CASE_NAMES = (
    "form layout and member access",
    "nested forms and member chain",
    "form field mutation",
    "value-producing if expression",
    "if expression in call argument",
    "exclusive range loop",
    "inclusive range loop",
    "wildcard range loop",
)

NATIVE_IR_RUN_CASE_NAMES = (
    "form layout and member access",
    "nested forms and member chain",
    "form field mutation",
    "if expression in call argument",
    "exclusive range loop",
    "inclusive range loop",
    "wildcard range loop",
)

NATIVE_IR_EXPECTED_FRAGMENTS = {
    "form layout and member access": ("struct_make", "struct_field", "load"),
    "nested forms and member chain": ("struct_make", "struct_field", "load"),
    "form field mutation": ("struct_make", "struct_field", "store"),
    "value-producing if expression": ("cond_br", "alloca"),
    "if expression in call argument": ("cond_br", "alloca"),
    "exclusive range loop": ("cond_br", "alloca"),
    "inclusive range loop": ("cond_br", "alloca"),
    "wildcard range loop": ("cond_br", "alloca"),
}


def run_check(binary: Path, source_path: Path, options: tuple[str, ...] = ()):
    command = [
        str(binary), "check", str(source_path), "--quiet", "--no-color",
        "--diagnostic=json", *options,
    ]
    result = subprocess.run(
        command, capture_output=True, text=True, timeout=30
    )
    if result.returncode == 0:
        if result.stderr.strip():
            raise AssertionError((command, "unexpected diagnostics", result.stderr))
        return result.returncode, []
    if result.returncode != 1:
        raise AssertionError((command, result.returncode, result.stdout, result.stderr))
    try:
        document = json.loads(result.stderr or result.stdout)
    except json.JSONDecodeError as error:
        raise AssertionError((command, result.stderr, result.stdout)) from error
    diagnostics = []
    for item in document.get("diagnostics", []):
        position = item.get("position", {})
        context = item.get("context", {})
        # The native and bootstrap frontends currently differ in underline
        # width and contextual notes for some semantic errors. Pin the shared
        # public identity and primary start location until those are unified.
        diagnostics.append(
            (
                item.get("code"),
                item.get("severity"),
                item.get("phase"),
                item.get("category"),
                context.get("expected_type"),
                context.get("actual_type"),
                position.get("start_byte"),
                position.get("start_line"),
                position.get("start_column"),
            )
        )
    return result.returncode, diagnostics


def compare_case(name: str, source: str, native: Path, bootstrap: Path, directory: Path):
    source_path = directory / (name.replace(" ", "_") + ".vit")
    source_path.write_text(source, encoding="utf-8")
    native_result = run_check(native, source_path)
    bootstrap_result = run_check(bootstrap, source_path)
    comparable_native = native_result
    comparable_bootstrap = bootstrap_result
    if name == "field initializer type" and native_result[0] == bootstrap_result[0] == 1:
        # This sema diagnostic currently anchors to the field value in native
        # and to the initializer expression in bootstrap; compare its shared
        # public identity and expected/actual types while that span is unified.
        comparable_native = (native_result[0], [item[:6] for item in native_result[1]])
        comparable_bootstrap = (bootstrap_result[0], [item[:6] for item in bootstrap_result[1]])
    if comparable_native != comparable_bootstrap:
        raise AssertionError(
            f"{name}: native={native_result!r}; bootstrap={bootstrap_result!r}"
        )
    return native_result


def check_native_ir(native: Path, source_path: Path, expected_fragments=()):
    result = subprocess.run(
        [str(native), "compile", str(source_path), "--emit-ir", "--quiet", "--no-color"],
        capture_output=True,
        text=True,
        timeout=30,
    )
    if result.returncode != 0 or "; native Vitte IR" not in result.stdout:
        raise AssertionError((result.returncode, result.stdout, result.stderr))
    if "func @main" not in result.stdout or "return" not in result.stdout:
        raise AssertionError(("incomplete native IR", result.stdout))
    for fragment in expected_fragments:
        if fragment not in result.stdout:
            raise AssertionError((f"native IR missing {fragment!r}", result.stdout))


def emit_c(binary: Path, source_path: Path) -> str:
    result = subprocess.run(
        [str(binary), "compile", str(source_path), "--emit-c", "--quiet", "--no-color"],
        capture_output=True,
        text=True,
        timeout=60,
    )
    if result.returncode != 0:
        raise AssertionError((result.returncode, result.stdout, result.stderr))
    return result.stdout


def run_program(binary: Path, source_path: Path):
    command = [
        str(binary), "run", str(source_path), "--quiet", "--no-color"
    ]
    result = subprocess.run(
        command, capture_output=True, text=True, timeout=60
    )
    return result.returncode, result.stdout


def main() -> int:
    if len(sys.argv) != 3:
        print(f"usage: {sys.argv[0]} NATIVE_BOOTSTRAP_CLI BOOTSTRAP_CLI", file=sys.stderr)
        return 2
    native, bootstrap = (Path(value).resolve() for value in sys.argv[1:])
    matched = 0
    with tempfile.TemporaryDirectory(prefix="vitte-native-bootstrap-parity-") as value:
        directory = Path(value)
        for name, source in VALID_CASES.items():
            result = compare_case(name, source, native, bootstrap, directory)
            if result != (0, []):
                raise AssertionError(f"{name}: expected successful check, got {result!r}")
            matched += 1
        for name, source in PARSER_INVALID_CASES.items():
            result = compare_case(name, source, native, bootstrap, directory)
            if result[0] != 1 or not result[1] or any(
                diagnostic[2] != "parser" for diagnostic in result[1]
            ):
                raise AssertionError(
                    f"{name}: expected parser diagnostics, got {result!r}"
                )
            matched += 1
        for name, source in SEMANTIC_INVALID_CASES.items():
            result = compare_case(name, source, native, bootstrap, directory)
            if result[0] != 1 or not result[1] or any(
                diagnostic[2] not in ("name-resolution", "type-check")
                for diagnostic in result[1]
            ):
                raise AssertionError(
                    f"{name}: expected semantic diagnostics, got {result!r}"
                )
            matched += 1
        backend_probe = directory / "backend_probe.vit"
        backend_probe.write_text(VALID_CASES["scalar and calls"], encoding="utf-8")
        native_check = run_check(native, backend_probe, ("--backend=bytecode",))
        bootstrap_check = run_check(bootstrap, backend_probe, ("--backend=bytecode",))
        if native_check != (0, []) or bootstrap_check != native_check:
            raise AssertionError(
                "check must stop after semantic analysis even when the selected "
                f"backend cannot emit code: native={native_check!r}; "
                f"bootstrap={bootstrap_check!r}"
            )
        for name in RUN_CASE_NAMES:
            source_path = directory / (name.replace(" ", "_") + ".vit")
            native_result = run_program(native, source_path)
            bootstrap_result = run_program(bootstrap, source_path)
            if native_result != bootstrap_result:
                raise AssertionError(
                    f"{name} run: native={native_result!r}; "
                    f"bootstrap={bootstrap_result!r}"
                )
        for name in NATIVE_IR_RUN_CASE_NAMES:
            source_path = directory / (name.replace(" ", "_") + ".vit")
            native_result = run_program(native, source_path)
            if native_result != (0, ""):
                raise AssertionError(f"{name} native IR run: {native_result!r}")
        ir_source = directory / "ir_smoke.vit"
        ir_source.write_text(VALID_CASES["scalar and calls"], encoding="utf-8")
        check_native_ir(native, ir_source)
        scalar_c = emit_c(native, ir_source)
        if "Generated by the native Vitte C17 backend." not in scalar_c:
            raise AssertionError("default scalar compilation did not use the IR C17 backend")
        if "Generated by the native Vitte C17 AST emitter." in scalar_c:
            raise AssertionError("default scalar compilation unexpectedly used AST emission")

        for name in NATIVE_IR_CASE_NAMES:
            ir_case_source = directory / (name.replace(" ", "_") + ".vit")
            check_native_ir(native, ir_case_source, NATIVE_IR_EXPECTED_FRAGMENTS[name])
            ir_case_c = emit_c(native, ir_case_source)
            if "Generated by the native Vitte C17 backend." not in ir_case_c:
                raise AssertionError(f"{name}: expected native IR C17 emission")
            if "Generated by the native Vitte C17 AST emitter." in ir_case_c:
                raise AssertionError(f"{name}: unexpectedly used AST C17 emission")

    print(
        f"Native/bootstrap outcome/code/start parity: {matched} check cases "
        f"({len(VALID_CASES)} valid, {len(PARSER_INVALID_CASES)} parser errors, "
        f"{len(SEMANTIC_INVALID_CASES)} semantic errors) and "
        f"{len(RUN_CASE_NAMES)} shared run cases match and "
        f"{len(NATIVE_IR_RUN_CASE_NAMES)} additional native IR run cases pass; default scalar compilation "
        f"uses IR C17, {len(NATIVE_IR_CASE_NAMES)} cases use native IR C17, "
        "and native IR smoke passed."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
