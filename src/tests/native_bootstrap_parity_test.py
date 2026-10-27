#!/usr/bin/env python3
"""Compare native and bootstrap parsing, AST construction, and semantics."""

from __future__ import annotations

import json
import subprocess
import sys
import tempfile
from pathlib import Path


SEMANTIC_CASES = {
    "array form field": (
        "form Packet { values: [3]i64 }\n"
        "proc main() -> i64 {\n"
        "    let packet: Packet = Packet { values: [10, 20, 12] };\n"
        "    return packet.values[2] - 12;\n"
        "}\n",
        True,
    ),
    "array form field length mismatch": (
        "form Packet { values: [3]i64 }\n"
        "proc main() -> i64 {\n"
        "    let packet: Packet = Packet { values: [10, 20] };\n"
        "    return packet.values[0];\n"
        "}\n",
        False,
    ),
    "array form field element mismatch": (
        'form Packet { values: [3]i64 }\n'
        "proc main() -> i64 {\n"
        '    let packet: Packet = Packet { values: [10, "wrong", 12] };\n'
        "    return packet.values[0];\n"
        "}\n",
        False,
    ),
    "malformed form field type": (
        "form Packet { values: [3 i64 }\n"
        "proc main() -> i64 { return 0; }\n",
        False,
    ),
}


def run_check(binary: Path, source_path: Path, label: str):
    result = subprocess.run(
        [
            str(binary),
            "check",
            str(source_path),
            "--quiet",
            "--no-color",
            "--diagnostic=json",
        ],
        capture_output=True,
        text=True,
        timeout=60,
    )
    if result.returncode == 0:
        return True, ()

    payload = result.stderr or result.stdout
    try:
        document = json.loads(payload)
    except json.JSONDecodeError as error:
        raise AssertionError(
            f"{label}: expected JSON diagnostics from {binary}, got "
            f"stdout={result.stdout!r}, stderr={result.stderr!r}"
        ) from error

    codes = tuple(
        sorted(
            item.get("code", "")
            for item in document.get("diagnostics", [])
            if item.get("severity") in ("error", "fatal")
        )
    )
    return False, codes


def main() -> int:
    if len(sys.argv) != 4:
        print(
            f"usage: {sys.argv[0]} NATIVE_VITTE BOOTSTRAP_VITTE PARSER_PARITY_TEST",
            file=sys.stderr,
        )
        return 2

    native = Path(sys.argv[1]).resolve()
    bootstrap = Path(sys.argv[2]).resolve()
    parser_parity = Path(sys.argv[3]).resolve()

    parser_result = subprocess.run(
        [str(parser_parity)], capture_output=True, text=True, timeout=60
    )
    if parser_result.returncode != 0:
        raise AssertionError(
            "native/bootstrap parser and AST structural parity failed; "
            f"stdout={parser_result.stdout!r}, stderr={parser_result.stderr!r}"
        )
    print(parser_result.stdout.strip())

    with tempfile.TemporaryDirectory(prefix="vitte-native-bootstrap-parity-") as value:
        directory = Path(value)
        for name, (source, expected_success) in SEMANTIC_CASES.items():
            source_path = directory / (name.replace(" ", "_") + ".vit")
            source_path.write_text(source, encoding="utf-8")
            native_result = run_check(native, source_path, name)
            bootstrap_result = run_check(bootstrap, source_path, name)

            if native_result[0] != expected_success:
                raise AssertionError(
                    f"{name}: native compiler success={native_result[0]}, "
                    f"expected {expected_success}, diagnostics={native_result[1]}"
                )
            if bootstrap_result[0] != expected_success:
                raise AssertionError(
                    f"{name}: bootstrap compiler success={bootstrap_result[0]}, "
                    f"expected {expected_success}, diagnostics={bootstrap_result[1]}"
                )
            if native_result != bootstrap_result:
                raise AssertionError(
                    f"{name}: native/bootstrap semantic results differ: "
                    f"native={native_result}, bootstrap={bootstrap_result}"
                )

    print(
        "Native/bootstrap semantic parity passed: "
        f"{len(SEMANTIC_CASES)} cases."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
