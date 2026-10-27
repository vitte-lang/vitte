#!/usr/bin/env python3
"""Exercise native parser, semantic, HIR, IR, and backend subset paths."""

from __future__ import annotations

import json
import subprocess
import sys
import tempfile
from pathlib import Path


VALID_CASES = {
    "scalar operators and compound assignment": """\
proc main() -> i64 {
    let mut value: i64 = 1;
    set value += 2 * 3;
    set value -= 1;
    if value == 6 and not false { return 0; }
    return 1;
}
""",
    "mutable field projection": """\
form Point { x: i64 }
proc main() -> i64 {
    let mut point: Point = Point { x: 40 };
    set (point.x) += 2;
    return point.x - 42;
}
""",
    "nested aggregate field mutation through native IR": """\
form Vec2 { x: i64, y: i64 }
form Rect { origin: Vec2, width: i64 }
proc main() -> i64 {
    let mut rect: Rect = Rect {
        origin: Vec2 { y: 20, x: 40 },
        width: 2,
    };
    set (rect.origin.x) += 2;
    return rect.origin.x + rect.origin.y - 62;
}
""",
    "fixed arrays through native IR": """\
proc main() -> i64 {
    let mut values: [3]i64 = [10, 20, 30];
    set values[1] = 42;
    if values[0] + values[1] + values[2] != 82 { return 1; }
    let mut bytes: [2]u8 = [7 as u8, 255 as u8];
    set bytes[0] = 9 as u8;
    if bytes[0] != 9 or bytes[1] != 255 { return 2; }
    let inferred = [2, 4, 6];
    if inferred[0] + inferred[1] + inferred[2] != 12 { return 3; }
    let mut matrix: [2][2]i64 = [[1, 2], [3, 4]];
    set matrix[1][0] = 7;
    if matrix[1][0] != 7 { return 4; }
    return 0;
}
""",
    "scalar picks and match through native IR": """\
pick State { Cold, Ready, Done }
pick Other { Started }

proc state_code(value: State) -> i64 {
    match value {
        State::Cold if value == State::Cold => { return 10; },
        State::Cold => { return 11; },
        State::Ready if value == State::Cold => { return 12; },
        State::Ready => { return 20; },
        else => { return 30; },
    }
    return 0;
}

proc main() -> i64 {
    let mut state: State = State::Cold;
    if state_code(state) != 10 { return 1; }
    set state = State::Ready;
    if state_code(state) != 20 { return 2; }
    set state = State::Done;
    if state_code(state) != 30 { return 3; }
    return 0;
}
""",
    "dotted pick variants through native sema": """\
pick State { Idle, Ready }
pick Maybe { None, Some(i64) }

proc selected_state() -> State {
    return State.Ready;
}

proc selected_value() -> Maybe {
    return Maybe.Some(42);
}

proc main() -> i64 {
    let state: State = selected_state();
    let value: Maybe = selected_value();
    if state == State.Ready { return 0; }
    return 1;
}
""",
    "multi-field payload picks through native IR": """\
pick Payload { Data(i64, bool, i64), Empty }

proc payload_code(value: Payload) -> i64 {
    match value {
        Payload::Data(left, enabled, right) => {
            if enabled { return left + right; }
            return 0;
        },
        Payload::Empty => { return -1; },
    }
    return -2;
}

proc main() -> i64 {
    let value: Payload = Payload::Data(20, true, 22);
    if payload_code(value) != 42 { return 1; }
    let disabled: Payload = Payload::Data(20, false, 22);
    if payload_code(disabled) != 0 { return 2; }
    let empty: Payload = Payload::Empty;
    if payload_code(empty) != -1 { return 3; }
    match value {
        Payload::Data(_, _, _) => { return 0; },
        Payload::Empty => { return 4; },
    }
    return 5;
}
""",
}

INVALID_CASES = {
    "logical operators require booleans": (
        "proc main() -> bool { return 1 and true; }\n",
        "VITTE_TYPE_E_INVALID_OPERAND",
    ),
    "ordering comparisons require numeric operands": (
        'proc main() -> bool { return "left" < "right"; }\n',
        "VITTE_TYPE_E_INVALID_OPERAND",
    ),
    "equality requires compatible operands": (
        'proc main() -> bool { return 1 == "wrong"; }\n',
        "VITTE_TYPE_E_INVALID_OPERAND",
    ),
    "logical negation requires a boolean": (
        "proc main() -> bool { return not 1; }\n",
        "VITTE_TYPE_E_INVALID_OPERAND",
    ),
    "immutable locals cannot use compound assignment": (
        "proc main() -> i64 { let value: i64 = 1; set value += 1; return value; }\n",
        "VITTE_TYPE_E_NOT_ASSIGNABLE",
    ),
    "assignment needs an assignable place": (
        "proc main() -> i64 { set (1 + 2) = 3; return 0; }\n",
        "VITTE_TYPE_E_NOT_ASSIGNABLE",
    ),
    "set requires an assignment expression": (
        "proc main() -> i64 { let mut value: i64 = 1; set value; return value; }\n",
        "VITTE_PARSER_E_UNEXPECTED_TOKEN",
    ),
    "fixed array initializer length must match": (
        "proc main() -> i64 { let values: [2]i64 = [1, 2, 3]; return values[0]; }\n",
        "VITTE_TYPE_E_MISMATCH",
    ),
    "unknown scalar pick variant is rejected": (
        "pick State { Ready, Done }\nproc main() -> i64 { let state: State = State::Missing; return state; }\n",
        "VITTE_TYPE_E_NO_MEMBER",
    ),
    "match pattern must belong to its pick type": (
        "pick State { Ready, Done }\npick Other { Started }\nproc main() -> i64 { let state: State = State::Ready; match state { Other::Started => { return 1; }, else => { return 0; } } return 2; }\n",
        "VITTE_TYPE_E_MISMATCH",
    ),
    "payload pick constructor requires its payload": (
        "pick Maybe { Some(i64) }\nproc main() -> i64 { let value: Maybe = Maybe::Some(); return 0; }\n",
        "VITTE_CALL_E_ARITY",
    ),
    "payload pick constructor checks payload type": (
        'pick Maybe { Some(i64) }\nproc main() -> i64 { let value: Maybe = Maybe::Some("wrong"); return 0; }\n',
        "VITTE_CALL_E_ARGUMENT_TYPE",
    ),
    "payload pick pattern requires its binding": (
        "pick Maybe { Some(i64) }\nproc main() -> i64 { let value: Maybe = Maybe::Some(1); match value { Maybe::Some() => { return 0; } } return 1; }\n",
        "VITTE_CALL_E_ARITY",
    ),
    "multi-field payload pick constructor checks arity": (
        "pick Payload { Data(i64, bool, i64) }\nproc main() -> i64 { let value: Payload = Payload::Data(1, true); return 0; }\n",
        "VITTE_CALL_E_ARITY",
    ),
    "multi-field payload pick checks every field type": (
        'pick Payload { Data(i64, bool, i64) }\nproc main() -> i64 { let value: Payload = Payload::Data(1, true, "wrong"); return 0; }\n',
        "VITTE_CALL_E_ARGUMENT_TYPE",
    ),
    "multi-field payload pick pattern checks arity": (
        "pick Payload { Data(i64, bool, i64) }\nproc main() -> i64 { let value: Payload = Payload::Data(1, true, 2); match value { Payload::Data(first, enabled) => { return first; } } return 0; }\n",
        "VITTE_CALL_E_ARITY",
    ),
    "unknown payload pick variant is rejected": (
        "pick Maybe { Some(i64) }\nproc main() -> i64 { let value: Maybe = Maybe::Missing(1); return 0; }\n",
        "VITTE_TYPE_E_NO_MEMBER",
    ),
}

AST_FALLBACK_CASES = {
    "unsupported form declaration": (
        "form Legacy { value: i32 }\n"
        "proc main() -> i64 { let old: Legacy = Legacy { value: 42 }; "
        "return old.value - 42; }\n",
    ),
    "non-range for statement": (
        "proc main() -> i64 {\n"
        "    let values: [2]i64 = [20, 22];\n"
        "    let mut total: i64 = 0;\n"
        "    for value in values { set total += value; }\n"
        "    return total - 42;\n"
        "}\n",
    ),
}


def invoke(binary: Path, command: str, source_path: Path):
    return subprocess.run(
        [
            str(binary), command, str(source_path), "--quiet", "--no-color",
            "--diagnostic=json",
        ],
        capture_output=True,
        text=True,
        timeout=60,
    )


def diagnostics_for(result: subprocess.CompletedProcess[str], label: str):
    try:
        document = json.loads(result.stderr or result.stdout)
    except json.JSONDecodeError as error:
        raise AssertionError(
            f"{label}: expected JSON diagnostics, got stdout={result.stdout!r}, "
            f"stderr={result.stderr!r}"
        ) from error
    return document.get("diagnostics", [])


def assert_ast_emitter_banner(binary: Path, directory: Path, name: str, source: str):
    source_path = directory / (name.replace(" ", "_") + ".vit")
    output_path = directory / (name.replace(" ", "_") + ".c")
    source_path.write_text(source, encoding="utf-8")
    result = subprocess.run(
        [
            str(binary), "compile", str(source_path), "--emit-c", "--quiet",
            "--no-color", "-o", str(output_path),
        ],
        capture_output=True,
        text=True,
        timeout=60,
    )
    if result.returncode != 0:
        raise AssertionError(
            f"{name}: AST fallback compile returned {result.returncode}; "
            f"stdout={result.stdout!r}; stderr={result.stderr!r}"
        )
    generated = output_path.read_text(encoding="utf-8")
    if "Generated by the native Vitte C17 AST emitter" not in generated:
        raise AssertionError(f"{name}: expected the AST emitter banner in generated C")


def assert_ast_fallback_rejected(binary: Path, directory: Path, name: str, source: str):
    source_path = directory / (name.replace(" ", "_") + ".vit")
    source_path.write_text(source, encoding="utf-8")
    result = subprocess.run(
        [
            str(binary), "compile", str(source_path), "--emit-c", "--quiet",
            "--no-color", "--diagnostic=json",
        ],
        capture_output=True,
        text=True,
        timeout=60,
    )
    if result.returncode != 1:
        raise AssertionError(
            f"{name}: expected a checked fallback rejection, got {result.returncode}; "
            f"stdout={result.stdout!r}; stderr={result.stderr!r}"
        )
    diagnostics = diagnostics_for(result, name)
    if "VITTE_BACKEND_E_CODEGEN" not in {
        item.get("internal_code") for item in diagnostics
    }:
        raise AssertionError(f"{name}: expected owned backend codegen diagnostic, got {diagnostics!r}")


def main() -> int:
    if len(sys.argv) != 2:
        print(f"usage: {sys.argv[0]} VITTE", file=sys.stderr)
        return 2
    binary = Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory(prefix="vitte-native-frontend-") as value:
        directory = Path(value)
        for name, source in VALID_CASES.items():
            source_path = directory / (name.replace(" ", "_") + ".vit")
            source_path.write_text(source, encoding="utf-8")
            checked = invoke(binary, "check", source_path)
            if checked.returncode != 0:
                raise AssertionError(
                    f"{name}: check returned {checked.returncode}; "
                    f"stdout={checked.stdout!r}; stderr={checked.stderr!r}"
                )
            if name == "scalar operators and compound assignment":
                executed = invoke(binary, "run", source_path)
                if executed.returncode != 0:
                    raise AssertionError(
                        f"{name}: run returned {executed.returncode}; "
                        f"stdout={executed.stdout!r}; stderr={executed.stderr!r}"
                    )
            if name == "fixed arrays through native IR":
                lowered = subprocess.run(
                    [
                        str(binary), "compile", str(source_path), "--emit-ir",
                        "--no-color",
                    ],
                    capture_output=True,
                    text=True,
                    timeout=60,
                )
                if lowered.returncode != 0 or "array_make" not in lowered.stdout or "gep" not in lowered.stdout:
                    raise AssertionError(
                        f"fixed arrays: expected array construction and indexed IR; "
                        f"return={lowered.returncode}; stdout={lowered.stdout!r}; "
                        f"stderr={lowered.stderr!r}"
                    )
                executed = invoke(binary, "run", source_path)
                if executed.returncode != 0:
                    raise AssertionError(
                        f"fixed arrays: run returned {executed.returncode}; "
                        f"stdout={executed.stdout!r}; stderr={executed.stderr!r}"
                    )
            if name in ("mutable field projection", "nested aggregate field mutation through native IR"):
                lowered = subprocess.run(
                    [
                        str(binary), "compile", str(source_path), "--emit-ir",
                        "--no-color",
                    ],
                    capture_output=True,
                    text=True,
                    timeout=60,
                )
                for opcode in ("struct_make", "struct_field", "load"):
                    if lowered.returncode != 0 or opcode not in lowered.stdout:
                        raise AssertionError(
                            f"{name}: expected {opcode} in native aggregate IR; "
                            f"return={lowered.returncode}; stdout={lowered.stdout!r}; "
                            f"stderr={lowered.stderr!r}"
                        )
                if name == "nested aggregate field mutation through native IR" and "store" not in lowered.stdout:
                    raise AssertionError("nested aggregate mutation: expected store in native IR")
                executed = invoke(binary, "run", source_path)
                if executed.returncode != 0:
                    raise AssertionError(
                        f"{name}: run returned {executed.returncode}; "
                        f"stdout={executed.stdout!r}; stderr={executed.stderr!r}"
                    )
            if name == "scalar picks and match through native IR":
                lowered = subprocess.run(
                    [
                        str(binary), "compile", str(source_path), "--emit-ir",
                        "--no-color",
                    ],
                    capture_output=True,
                    text=True,
                    timeout=60,
                )
                if lowered.returncode != 0 or "icmp_eq" not in lowered.stdout or "cond_br" not in lowered.stdout:
                    raise AssertionError(
                        f"scalar picks: expected equality and branch IR; "
                        f"return={lowered.returncode}; stdout={lowered.stdout!r}; "
                        f"stderr={lowered.stderr!r}"
                    )
                executed = invoke(binary, "run", source_path)
                if executed.returncode != 0:
                    raise AssertionError(
                        f"scalar picks: run returned {executed.returncode}; "
                        f"stdout={executed.stdout!r}; stderr={executed.stderr!r}"
                    )
            if name == "multi-field payload picks through native IR":
                lowered = subprocess.run(
                    [
                        str(binary), "compile", str(source_path), "--emit-ir",
                        "--no-color",
                    ],
                    capture_output=True,
                    text=True,
                    timeout=60,
                )
                for opcode in ("pick_make", "pick_tag", "pick_payload", "cond_br"):
                    if lowered.returncode != 0 or opcode not in lowered.stdout:
                        raise AssertionError(
                            f"multi-field payload picks: expected {opcode} in native IR; "
                            f"return={lowered.returncode}; stdout={lowered.stdout!r}; "
                            f"stderr={lowered.stderr!r}"
                        )
                executed = invoke(binary, "run", source_path)
                if executed.returncode != 0:
                    raise AssertionError(
                        f"multi-field payload picks: run returned {executed.returncode}; "
                        f"stdout={executed.stdout!r}; stderr={executed.stderr!r}"
                    )

        for name, (source, expected_code) in INVALID_CASES.items():
            source_path = directory / (name.replace(" ", "_") + ".vit")
            source_path.write_text(source, encoding="utf-8")
            checked = invoke(binary, "check", source_path)
            if checked.returncode != 1:
                raise AssertionError(
                    f"{name}: expected rejection, got {checked.returncode}; "
                    f"stdout={checked.stdout!r}; stderr={checked.stderr!r}"
                )
            diagnostics = diagnostics_for(checked, name)
            codes = {item.get("internal_code") for item in diagnostics}
            if expected_code not in codes:
                raise AssertionError(
                    f"{name}: expected {expected_code}, got {diagnostics!r}"
                )

        for name, (source,) in AST_FALLBACK_CASES.items():
            if name == "unsupported form declaration":
                assert_ast_emitter_banner(binary, directory, name, source)
            else:
                assert_ast_fallback_rejected(binary, directory, name, source)

    print(
        f"Native frontend subset passed: {len(VALID_CASES)} valid and "
        f"{len(INVALID_CASES)} invalid cases."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
