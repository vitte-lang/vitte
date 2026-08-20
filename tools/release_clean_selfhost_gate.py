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
RELEASE = ROOT / "target/release/vitte"
SELF = ROOT / "target/test/vitte-self"
SELF2 = ROOT / "target/test/vitte-self2"
ENTRYPOINT = "src/vitte/compiler/main.vit"
REPORT_DIR = ROOT / "target/reports"
REPORT_JSON = REPORT_DIR / "release_clean_selfhost_gate.json"
REPORT_MD = REPORT_DIR / "release_clean_selfhost_gate.md"
HASH_DIVERGENCE_REASON = "native executable format may include platform linker entropy; accepted only when diagnostics, IR, and MIR are identical"


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def clean_env(compiler: Path) -> dict[str, str]:
    env = {
        "HOME": os.environ.get("HOME", str(ROOT)),
        "PATH": "/usr/bin:/bin:/usr/sbin:/sbin:/opt/homebrew/bin:/usr/local/bin",
        "TMPDIR": os.environ.get("TMPDIR", "/tmp"),
        "VITTE_COMPILER": str(compiler),
        "VITTE_PACKAGE_OFFLINE": "1",
    }
    if "CC" in os.environ:
        env["CC"] = os.environ["CC"]
    return env


def run(command: list[str], compiler: Path) -> dict[str, Any]:
    proc = subprocess.run(
        command,
        cwd=ROOT,
        env=clean_env(compiler),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    return {
        "command": command,
        "exit_code": proc.returncode,
        "stdout": proc.stdout.strip(),
        "stderr": proc.stderr.strip(),
    }


def require_ok(command: list[str], compiler: Path, results: list[dict[str, Any]], failures: list[str]) -> dict[str, Any]:
    result = run(command, compiler)
    results.append(result)
    if result["exit_code"] != 0:
        failures.append(f"{' '.join(command)} failed: {result['stderr'] or result['stdout'] or result['exit_code']}")
    return result


def normalized(result: dict[str, Any]) -> dict[str, Any]:
    stdout = result["stdout"].replace(str(ROOT), "<ROOT>")
    stderr = result["stderr"].replace(str(ROOT), "<ROOT>")
    for name in ("vitte-self", "vitte-self2"):
        stdout = stdout.replace(name, "<self>")
        stderr = stderr.replace(name, "<self>")
    return {"exit_code": result["exit_code"], "stdout": stdout, "stderr": stderr}


def compare_pair(name: str, left: dict[str, Any], right: dict[str, Any], comparisons: list[dict[str, Any]], failures: list[str]) -> None:
    left_norm = normalized(left)
    right_norm = normalized(right)
    equal = left_norm == right_norm
    comparisons.append({"name": name, "equal": equal, "left": left_norm, "right": right_norm})
    if not equal:
        failures.append(f"{name} differs between vitte-self and vitte-self2")


def stdlib_sources() -> list[str]:
    checks = [Path("src/vitte/stdlib/index.vit")]
    for family in ("core", "alloc", "ffi"):
        root = ROOT / "src/vitte/stdlib" / family
        checks.extend(path.relative_to(ROOT) for path in sorted(root.glob("*.vit")))
        checks.extend(path.relative_to(ROOT) for path in sorted(root.glob("*.vitl")))
    return sorted({path.as_posix() for path in checks})


def behavior_comparisons_equal(comparisons: list[dict[str, Any]]) -> bool:
    required = {"diagnostics-entrypoint", "ir-entrypoint", "mir-entrypoint"}
    observed = {str(item.get("name")): item.get("equal") is True for item in comparisons}
    return all(observed.get(name) is True for name in required)


def compare_hashes(comparisons: list[dict[str, Any]], failures: list[str]) -> dict[str, Any]:
    if not SELF.is_file() or not SELF2.is_file():
        failures.append("missing vitte-self or vitte-self2 for hash comparison")
        return {"status": "missing", "enforced": True, "reason": "missing self binaries"}
    left = sha256_file(SELF)
    right = sha256_file(SELF2)
    equal = left == right
    behavior_equal = behavior_comparisons_equal(comparisons)
    enforce_hash = True
    reason = ""
    if not equal and sys.platform == "darwin" and behavior_equal:
        enforce_hash = False
        reason = HASH_DIVERGENCE_REASON
    comparisons.append(
        {
            "name": "self-binary-sha256",
            "equal": equal or not enforce_hash,
            "hash_equal": equal,
            "hash_enforced": enforce_hash,
            "behavior_equal": behavior_equal,
            "reason": reason,
            "left_sha256": left,
            "right_sha256": right,
        }
    )
    if not equal and enforce_hash:
        failures.append("vitte-self and vitte-self2 hash differs without declared reason")
    return {
        "status": "reproducible" if equal else "justified-divergence" if not enforce_hash else "failed",
        "enforced": enforce_hash,
        "behavior_equal": behavior_equal,
        "reason": reason,
        "left_sha256": left,
        "right_sha256": right,
    }


def write_reports(status: str, results: list[dict[str, Any]], comparisons: list[dict[str, Any]], hash_policy: dict[str, Any], failures: list[str]) -> None:
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    report = {
        "schema": "vitte.release.clean.selfhost.gate.v1",
        "status": status,
        "release": "target/release/vitte",
        "self": "target/test/vitte-self",
        "self2": "target/test/vitte-self2",
        "entrypoint": ENTRYPOINT,
        "build_chain": [
            "target/release/vitte build src/vitte/compiler/main.vit -o target/test/vitte-self",
            "target/test/vitte-self build src/vitte/compiler/main.vit -o target/test/vitte-self2",
        ],
        "required_equalities": ["diagnostics-entrypoint", "ir-entrypoint", "mir-entrypoint"],
        "hash_policy": hash_policy,
        "stdlib_checks": stdlib_sources(),
        "commands": results,
        "comparisons": comparisons,
        "failures": failures,
    }
    REPORT_JSON.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    lines = [
        "# release clean selfhost gate",
        "",
        f"- status: {status}",
        f"- commands: {len(results)}",
        f"- comparisons: {len(comparisons)}",
        f"- hash policy: {hash_policy.get('status', 'unknown')}",
        f"- stdlib checks: {len(stdlib_sources())}",
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

    if not RELEASE.is_file() or not os.access(RELEASE, os.X_OK):
        failures.append("missing executable target/release/vitte")
        write_reports("fail", results, comparisons, {"status": "not-run", "reason": "missing release binary"}, failures)
        return 1
    if b"vitte_stage0_clone_self" in RELEASE.read_bytes():
        failures.append("refusing to execute release because it contains the retired self-copy implementation")
        write_reports("fail", results, comparisons, {"status": "not-run", "reason": "self-copy compiler rejected"}, failures)
        return 1

    SELF.parent.mkdir(parents=True, exist_ok=True)
    require_ok([str(RELEASE), "--version"], RELEASE, results, failures)
    require_ok([str(RELEASE), "--help"], RELEASE, results, failures)
    require_ok([str(RELEASE), "check", ENTRYPOINT], RELEASE, results, failures)
    require_ok([str(RELEASE), "build", ENTRYPOINT, "-o", str(SELF.relative_to(ROOT))], RELEASE, results, failures)
    require_ok([str(SELF), "--version"], SELF, results, failures)
    require_ok([str(SELF), "check", ENTRYPOINT], SELF, results, failures)
    require_ok([str(SELF), "build", ENTRYPOINT, "-o", str(SELF2.relative_to(ROOT))], SELF, results, failures)
    require_ok([str(SELF2), "--version"], SELF2, results, failures)

    self_check = require_ok([str(SELF), "check", ENTRYPOINT], SELF, results, failures)
    self2_check = require_ok([str(SELF2), "check", ENTRYPOINT], SELF2, results, failures)
    compare_pair("diagnostics-entrypoint", self_check, self2_check, comparisons, failures)

    self_ir = require_ok([str(SELF), "dump-native-ir", "--src", ENTRYPOINT], SELF, results, failures)
    self2_ir = require_ok([str(SELF2), "dump-native-ir", "--src", ENTRYPOINT], SELF2, results, failures)
    compare_pair("ir-entrypoint", self_ir, self2_ir, comparisons, failures)

    self_mir = require_ok([str(SELF), "dump-mir", ENTRYPOINT], SELF, results, failures)
    self2_mir = require_ok([str(SELF2), "dump-mir", ENTRYPOINT], SELF2, results, failures)
    compare_pair("mir-entrypoint", self_mir, self2_mir, comparisons, failures)
    hash_policy = compare_hashes(comparisons, failures)

    require_ok([str(RELEASE), "check", "src/vitte/stdlib/index.vit"], RELEASE, results, failures)
    for source in stdlib_sources():
        require_ok([str(RELEASE), "check", source], RELEASE, results, failures)

    status = "fail" if failures else "pass"
    write_reports(status, results, comparisons, hash_policy, failures)
    if failures:
        print("[release-clean-selfhost-gate][error] gate failed", file=sys.stderr)
        for failure in failures:
            print(f" - {failure}", file=sys.stderr)
        return 1
    print(f"[release-clean-selfhost-gate] ok stdlib_checks={len(stdlib_sources())}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
