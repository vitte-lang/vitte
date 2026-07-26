#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
REPORT_DIR = ROOT / "target/reports"
REPORT_JSON = REPORT_DIR / "selfhost_ci_regression_gate.json"
REPORT_MD = REPORT_DIR / "selfhost_ci_regression_gate.md"
HASH_JSON = REPORT_DIR / "selfhost_stage_hashes.json"
HASH_SHA256 = REPORT_DIR / "selfhost_stage_hashes.sha256"
ENTRYPOINT = "src/vitte/compiler/main.vit"
STAGES = {
    "stage1": ROOT / "target/stage1/vitte",
    "stage2": ROOT / "target/stage2/vitte",
    "release": ROOT / "target/release/vitte",
}
REQUIRED_REPORTS = [
    "selfhost_full_gate.json",
    "release_binary_gate.json",
    "selfhost_stage_compare_gate.json",
]
FORBIDDEN_NORMAL_MARKERS = (
    "bin/vittec0",
    "vittec0.seed",
    "toolchain/seed",
    "scripts/seed/install_seed.sh",
    "scripts/seed/verify_seed.sh",
)


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_report(name: str, failures: list[str]) -> dict[str, Any]:
    path = REPORT_DIR / name
    if not path.is_file():
        failures.append(f"missing selfhost CI report: target/reports/{name}")
        return {}
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        failures.append(f"invalid JSON report target/reports/{name}: {exc}")
        return {}
    if not isinstance(payload, dict):
        failures.append(f"report target/reports/{name} is not a JSON object")
        return {}
    if payload.get("status") != "pass":
        failures.append(f"report target/reports/{name} is not pass")
    return payload


def command_texts(value: Any) -> list[str]:
    texts: list[str] = []
    if isinstance(value, dict):
        if isinstance(value.get("command"), list):
            texts.append(" ".join(str(part) for part in value["command"]))
        if isinstance(value.get("args"), list):
            stage = str(value.get("stage", ""))
            texts.append(" ".join([stage, *[str(part) for part in value["args"]]]).strip())
        for item in value.values():
            texts.extend(command_texts(item))
    elif isinstance(value, list):
        for item in value:
            texts.extend(command_texts(item))
    return texts


def validate_reports(failures: list[str]) -> list[dict[str, Any]]:
    reports: list[dict[str, Any]] = []
    for name in REQUIRED_REPORTS:
        payload = load_report(name, failures)
        if payload:
            reports.append({"name": name, "schema": payload.get("schema"), "status": payload.get("status")})
            for text in command_texts(payload):
                for marker in FORBIDDEN_NORMAL_MARKERS:
                    if marker in text:
                        failures.append(f"normal selfhost report command uses forbidden marker {marker}: {text}")
    return reports


def stage_hashes(failures: list[str]) -> dict[str, Any]:
    hashes: dict[str, Any] = {"schema": "vitte.selfhost.stage.hashes.v1", "stages": {}}
    lines: list[str] = []
    for name, path in STAGES.items():
        if not path.is_file():
            failures.append(f"missing {name} binary for hash retention: {rel(path)}")
            continue
        if not os.access(path, os.X_OK):
            failures.append(f"{name} binary is not executable: {rel(path)}")
        digest = sha256_file(path)
        item = {"path": rel(path), "sha256": digest, "size": path.stat().st_size}
        hashes["stages"][name] = item
        lines.append(f"{digest}  {rel(path)}")
    HASH_JSON.write_text(json.dumps(hashes, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    HASH_SHA256.write_text("\n".join(lines) + ("\n" if lines else ""), encoding="utf-8")
    return hashes


def clean_env(*, polluted_path: bool) -> dict[str, str]:
    env = {
        "HOME": str(ROOT / "target/empty-home"),
        "TMPDIR": os.environ.get("TMPDIR", "/tmp"),
        "VITTE_COMPILER": str(STAGES["release"]),
        "VITTE_PACKAGE_OFFLINE": "1",
    }
    base_path = "/usr/bin:/bin:/usr/sbin:/sbin:/opt/homebrew/bin:/usr/local/bin"
    if polluted_path:
        fake = ROOT / "target/polluted-path/bin"
        fake.mkdir(parents=True, exist_ok=True)
        fake_vitte = fake / "vitte"
        fake_vitte.write_text("#!/bin/sh\necho polluted vitte >&2\nexit 127\n", encoding="utf-8")
        fake_vitte.chmod(0o755)
        env["PATH"] = f"{fake}:{base_path}"
    else:
        env["PATH"] = base_path
    if "CC" in os.environ:
        env["CC"] = os.environ["CC"]
    return env


def run(command: list[str], *, polluted_path: bool) -> dict[str, Any]:
    proc = subprocess.run(
        command,
        cwd=ROOT,
        env=clean_env(polluted_path=polluted_path),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    return {
        "command": command,
        "polluted_path": polluted_path,
        "exit_code": proc.returncode,
        "stdout": proc.stdout.strip(),
        "stderr": proc.stderr.strip(),
    }


def validate_shell_modes(failures: list[str]) -> list[dict[str, Any]]:
    (ROOT / "target/empty-home").mkdir(parents=True, exist_ok=True)
    release = STAGES["release"]
    commands = [
        [str(release), "--version"],
        [str(release), "--help"],
        [str(release), "check", ENTRYPOINT],
    ]
    results: list[dict[str, Any]] = []
    for polluted in (False, True):
        for command in commands:
            result = run(command, polluted_path=polluted)
            results.append(result)
            if result["exit_code"] != 0:
                mode = "PATH pollue" if polluted else "shell propre"
                failures.append(f"{mode} failed for {' '.join(command)}: {result['stderr'] or result['stdout']}")
    if shutil.which("vitte", path=clean_env(polluted_path=True)["PATH"]):
        # The fake vitte intentionally shadows PATH. Absolute target/release/vitte must still pass above.
        pass
    return results


def write_reports(status: str, reports: list[dict[str, Any]], hashes: dict[str, Any], shell_results: list[dict[str, Any]], failures: list[str]) -> None:
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    payload = {
        "schema": "vitte.selfhost.ci.regression.gate.v1",
        "status": status,
        "required_reports": REQUIRED_REPORTS,
        "forbidden_normal_markers": FORBIDDEN_NORMAL_MARKERS,
        "reports": reports,
        "hashes": hashes,
        "shell_results": shell_results,
        "chain_compiler_vitte_100_declaration": status == "pass",
        "failures": failures,
    }
    REPORT_JSON.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    lines = [
        "# selfhost CI regression gate",
        "",
        f"- status: {status}",
        f"- required reports: {len(REQUIRED_REPORTS)}",
        f"- hashed stages: {len(hashes.get('stages', {})) if isinstance(hashes, dict) else 0}",
        f"- shell commands: {len(shell_results)}",
        f"- chaine compiler Vitte 100%: {'yes' if status == 'pass' else 'no'}",
    ]
    if failures:
        lines.append("")
        lines.append("## Failures")
        lines.extend(f"- {failure}" for failure in failures)
    REPORT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    failures: list[str] = []
    reports = validate_reports(failures)
    hashes = stage_hashes(failures)
    shell_results = validate_shell_modes(failures)
    status = "fail" if failures else "pass"
    write_reports(status, reports, hashes, shell_results, failures)
    if failures:
        print("[selfhost-ci-regression-gate][error] gate failed", file=sys.stderr)
        for failure in failures:
            print(f" - {failure}", file=sys.stderr)
        return 1
    print("[selfhost-ci-regression-gate] ok reports=3 shell=clean+polluted hashes=stage1/stage2/release")
    return 0


if __name__ == "__main__":
    sys.exit(main())
