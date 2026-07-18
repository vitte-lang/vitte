#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / "target" / "bootstrap" / "clean-checkout" / "report.json"
REMOVED_ARTIFACT_DIRS = ("bin", "target", "build")
BRIDGE_MARKER = b"vitte-bootstrap-payload-bridge"


def tracked_paths() -> list[Path]:
    result = subprocess.run(
        ["git", "ls-files", "-z"],
        cwd=ROOT,
        check=True,
        stdout=subprocess.PIPE,
    )
    return [Path(os.fsdecode(raw)) for raw in result.stdout.split(b"\0") if raw]


def copy_tracked_checkout(destination: Path) -> int:
    paths = tracked_paths()
    for relative_path in paths:
        if relative_path.is_absolute() or ".." in relative_path.parts:
            raise ValueError(f"tracked path escapes checkout: {relative_path}")
        source = ROOT / relative_path
        target = destination / relative_path
        target.parent.mkdir(parents=True, exist_ok=True)
        if source.is_symlink():
            target.symlink_to(os.readlink(source))
        elif source.is_file():
            shutil.copy2(source, target)
        else:
            raise FileNotFoundError(f"tracked path is missing from the worktree: {relative_path}")
    return len(paths)


def normalize_output(output: str, checkout: Path) -> str:
    normalized = output.replace(str(checkout), "<checkout>")
    lines = normalized.splitlines()
    return "\n".join(lines[-80:])


def run_step(checkout: Path, name: str, command: list[str]) -> dict[str, object]:
    try:
        completed = subprocess.run(
            command,
            cwd=checkout,
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=240,
        )
        return {
            "name": name,
            "command": command,
            "returncode": completed.returncode,
            "ok": completed.returncode == 0,
            "stdout_tail": normalize_output(completed.stdout, checkout),
            "stderr_tail": normalize_output(completed.stderr, checkout),
        }
    except subprocess.TimeoutExpired as error:
        stdout = error.stdout if isinstance(error.stdout, str) else ""
        stderr = error.stderr if isinstance(error.stderr, str) else ""
        return {
            "name": name,
            "command": command,
            "returncode": 124,
            "ok": False,
            "stdout_tail": normalize_output(stdout, checkout),
            "stderr_tail": normalize_output(stderr or "command timed out", checkout),
        }


def artifact_state(checkout: Path, relative_path: str) -> dict[str, object]:
    path = checkout / relative_path
    if not path.is_file():
        return {
            "path": relative_path,
            "available": False,
            "executable": False,
            "embedded_bridge": False,
            "sha256": "",
        }
    data = path.read_bytes()
    return {
        "path": relative_path,
        "available": True,
        "executable": os.access(path, os.X_OK),
        "embedded_bridge": BRIDGE_MARKER in data,
        "sha256": hashlib.sha256(data).hexdigest(),
    }


def write_report(payload: dict[str, object]) -> None:
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    temporary = REPORT.with_suffix(".json.tmp")
    temporary.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    temporary.replace(REPORT)


def main() -> int:
    parser = argparse.ArgumentParser(description="verify bootstrap from a tracked-file-only checkout")
    parser.add_argument("--keep-on-failure", action="store_true", help="preserve the temporary checkout on failure")
    args = parser.parse_args()

    temporary_root = Path(tempfile.mkdtemp(prefix="vitte-clean-checkout-"))
    checkout = temporary_root / "checkout"
    checkout.mkdir()
    payload: dict[str, object] = {
        "schema": "vitte.bootstrap.clean_checkout",
        "schema_version": "1.0.0",
        "status": "fail",
        "tracked_file_count": 0,
        "removed_artifact_directories": list(REMOVED_ARTIFACT_DIRS),
        "steps": [],
        "artifacts": [],
    }

    try:
        payload["tracked_file_count"] = copy_tracked_checkout(checkout)
        for directory_name in REMOVED_ARTIFACT_DIRS:
            shutil.rmtree(checkout / directory_name, ignore_errors=True)

        commands = [
            (
                "bootstrap_seed",
                ["make", "--no-print-directory", "bootstrap-seed"],
            ),
            (
                "verify_seed_artifact",
                ["python3", "tools/check_bootstrap_stage_chain.py", "--artifacts"],
            ),
            (
                "verify_bootstrap_native_snapshots",
                ["make", "--no-print-directory", "bootstrap-native-snapshots"],
            ),
        ]
        steps: list[dict[str, object]] = []
        for name, command in commands:
            step = run_step(checkout, name, command)
            steps.append(step)
            print(f"[bootstrap-clean-checkout] {name}: {'ok' if step['ok'] else 'fail'}")
            if not step["ok"]:
                break
        payload["steps"] = steps

        artifacts = [artifact_state(checkout, "bin/vittec0")]
        payload["artifacts"] = artifacts
        artifacts_ok = all(bool(item["available"] and item["executable"]) for item in artifacts)
        sidecars = list(checkout.glob("bin/*.bootstrap-bridge"))
        payload["bridge_sidecar_count"] = len(sidecars)
        steps_ok = len(steps) == len(commands) and all(bool(step["ok"]) for step in steps)
        payload["status"] = "ok" if steps_ok and artifacts_ok and not sidecars else "fail"
        write_report(payload)

        if payload["status"] == "ok":
            print(f"[bootstrap-clean-checkout] ok: report={REPORT.relative_to(ROOT)}")
            return 0
        print(f"[bootstrap-clean-checkout][error] failed: report={REPORT.relative_to(ROOT)}")
        return 1
    finally:
        if args.keep_on_failure and payload["status"] != "ok":
            print(f"[bootstrap-clean-checkout] preserved checkout={checkout}")
        else:
            shutil.rmtree(temporary_root, ignore_errors=True)


if __name__ == "__main__":
    raise SystemExit(main())
