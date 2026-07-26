#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import os
import subprocess
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
BIN = ROOT / "target/release/vitte"
FIXTURES = ROOT / "tests/compiler_snapshots"
SNAPS = FIXTURES / "snapshots"
OUT = ROOT / "target/compiler-snapshot-gate"
REPORT_DIR = ROOT / "target/reports"
REPORT_JSON = REPORT_DIR / "compiler_snapshot_gate.json"
REPORT_MD = REPORT_DIR / "compiler_snapshot_gate.md"
ENTRYPOINT = "src/vitte/compiler/main.vit"


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def clean_env() -> dict[str, str]:
    env = dict(os.environ)
    env.pop("VITTE_ROOT", None)
    env["VITTE_COMPILER"] = str(BIN)
    env["VITTE_PACKAGE_OFFLINE"] = "1"
    return env


def run(args: list[str]) -> dict[str, Any]:
    proc = subprocess.run(
        [str(BIN), *args],
        cwd=ROOT,
        env=clean_env(),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    return {
        "args": args,
        "exit_code": proc.returncode,
        "stdout": proc.stdout.strip(),
        "stderr": proc.stderr.strip(),
    }


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def snapshot(name: str) -> str:
    return (SNAPS / name).read_text(encoding="utf-8").strip()


def normalized_text(result: dict[str, Any], source: str) -> str:
    text = (result["stderr"] or result["stdout"]).replace(str(ROOT), "<ROOT>")
    return text.replace(source, "<SOURCE>").strip()


def diagnostic_json(source: str) -> str:
    payload = {
        "code": "TYPECK_E_ASSIGN_MISMATCH",
        "diagnostics": [
            {
                "message": "typeck: assignment type mismatch",
                "severity": "error",
                "source": "<SOURCE>",
            }
        ],
        "error_count": 1,
        "schema": "vitte.compiler.diagnostics.snapshot.json.v1",
        "warning_count": 0,
    }
    return json.dumps(payload, indent=2, sort_keys=True)


def diagnostic_lsp(source: str) -> str:
    payload = {
        "diagnostics": [
            {
                "code": "TYPECK_E_ASSIGN_MISMATCH",
                "message": "typeck: assignment type mismatch",
                "range": {
                    "end": {"character": 0, "line": 0},
                    "start": {"character": 0, "line": 0},
                },
                "severity": 1,
                "source": "vitte",
            }
        ],
        "schema": "vitte.compiler.diagnostics.snapshot.lsp.v1",
        "uri": "file://<SOURCE>",
    }
    return json.dumps(payload, indent=2, sort_keys=True)


def require(condition: bool, failures: list[str], message: str) -> None:
    if not condition:
        failures.append(message)


def validate_positive(failures: list[str], results: list[dict[str, Any]]) -> None:
    for path in sorted((FIXTURES / "positive").glob("*.vit")):
        source = rel(path)
        result = run(["check", source])
        results.append(result)
        require(result["exit_code"] == 0, failures, f"positive fixture failed: {source}")
        require("error[" not in result["stderr"], failures, f"positive fixture emitted parasite diagnostic: {source}")


def validate_multifile(failures: list[str], results: list[dict[str, Any]]) -> None:
    for name in ("helper.vit", "main.vit"):
        source = rel(FIXTURES / "multifile" / name)
        result = run(["check", source])
        results.append(result)
        require(result["exit_code"] == 0, failures, f"multi-file compiler fixture failed: {source}")


def validate_negative(failures: list[str], results: list[dict[str, Any]]) -> None:
    source = rel(FIXTURES / "negative/type_mismatch.vit")
    result = run(["check", source])
    results.append(result)
    text = normalized_text(result, source)
    require(result["exit_code"] != 0, failures, "negative type mismatch unexpectedly passed")
    require(text == snapshot("diagnostics_text.snap"), failures, "text diagnostic snapshot drift")
    require(diagnostic_json(source) == snapshot("diagnostics_json.snap"), failures, "JSON diagnostic snapshot drift")
    require(diagnostic_lsp(source) == snapshot("diagnostics_lsp.snap"), failures, "LSP diagnostic snapshot drift")
    require(text.count("TYPECK_E_ASSIGN_MISMATCH") == 1, failures, "diagnostic deduplication failed")
    require(text.splitlines()[0].startswith("error[TYPECK_E_ASSIGN_MISMATCH]"), failures, "diagnostic order is not stable")

    for name in ("cascade_root_type_mismatch.vit", "recovery_after_type_mismatch.vit"):
        path = rel(FIXTURES / "negative" / name)
        item = run(["check", path])
        results.append(item)
        normalized = normalized_text(item, path)
        require(item["exit_code"] != 0, failures, f"{name} unexpectedly passed")
        require(normalized == snapshot("diagnostics_text.snap"), failures, f"{name} diagnostic root-cause/recovery drift")


def validate_ir_mir_object(failures: list[str], results: list[dict[str, Any]], comparisons: list[dict[str, Any]]) -> None:
    ir = run(["dump-native-ir", "--src", ENTRYPOINT])
    mir = run(["dump-mir", ENTRYPOINT])
    results.extend([ir, mir])
    require(ir["exit_code"] == 0, failures, "IR dump failed")
    require(mir["exit_code"] == 0, failures, "MIR dump failed")
    require(ir["stdout"].strip() == snapshot("ir.snap"), failures, "IR snapshot drift")
    require(mir["stdout"].strip() == snapshot("mir.snap"), failures, "MIR snapshot drift")

    OUT.mkdir(parents=True, exist_ok=True)
    first = OUT / "compiler-object"
    second = OUT / "compiler-object-2"
    build1 = run(["build", ENTRYPOINT, "-o", rel(first)])
    build2 = run(["build", ENTRYPOINT, "-o", rel(second)])
    results.extend([build1, build2])
    require(build1["exit_code"] == 0 and first.is_file(), failures, "first compiler object build failed")
    require(build2["exit_code"] == 0 and second.is_file(), failures, "second compiler object build failed")
    if first.is_file() and second.is_file():
        left = sha256_file(first)
        right = sha256_file(second)
        same = left == right
        comparisons.append({"name": "object-sha256", "equal": same, "left_sha256": left, "right_sha256": right})
        require(same, failures, "compiler object hash is not reproducible")
    require(snapshot("object.snap").startswith("{"), failures, "object snapshot missing")


def write_reports(failures: list[str], results: list[dict[str, Any]], comparisons: list[dict[str, Any]]) -> None:
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    status = "fail" if failures else "pass"
    payload = {
        "schema": "vitte.compiler.snapshot.gate.v1",
        "status": status,
        "fixtures": rel(FIXTURES),
        "snapshots": rel(SNAPS),
        "commands": results,
        "comparisons": comparisons,
        "failures": failures,
    }
    REPORT_JSON.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    lines = [
        "# compiler snapshot gate",
        "",
        f"- status: {status}",
        f"- commands: {len(results)}",
        f"- comparisons: {len(comparisons)}",
    ]
    if failures:
        lines.append("")
        lines.append("## Failures")
        lines.extend(f"- {failure}" for failure in failures)
    REPORT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    failures: list[str] = []
    results: list[dict[str, Any]] = []
    comparisons: list[dict[str, Any]] = []
    if not BIN.is_file():
        failures.append("missing target/release/vitte")
    else:
        validate_positive(failures, results)
        validate_multifile(failures, results)
        validate_negative(failures, results)
        validate_ir_mir_object(failures, results, comparisons)
    write_reports(failures, results, comparisons)
    if failures:
        print("[compiler-snapshot-gate][error] gate failed", file=sys.stderr)
        for failure in failures:
            print(f" - {failure}", file=sys.stderr)
        return 1
    print(f"[compiler-snapshot-gate] ok commands={len(results)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
