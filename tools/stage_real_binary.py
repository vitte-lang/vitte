#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import platform
import shutil
import subprocess
import sys
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_ROOT = ROOT / "target" / "real-binaries"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def binary_format(path: Path) -> str:
    data = path.read_bytes()[:4096]
    if data.startswith(b"\x7fELF"):
        return "ELF"
    if data.startswith(b"MZ"):
        pe_offset = int.from_bytes(data[0x3C:0x40], "little", signed=False) if len(data) >= 0x40 else 0
        if pe_offset > 0 and len(data) >= pe_offset + 4 and data[pe_offset:pe_offset + 4] == b"PE\0\0":
            return "PE"
        return "PE"
    if data[:4] in {
        b"\xfe\xed\xfa\xce",
        b"\xfe\xed\xfa\xcf",
        b"\xce\xfa\xed\xfe",
        b"\xcf\xfa\xed\xfe",
        b"\xca\xfe\xba\xbe",
        b"\xca\xfe\xba\xbf",
    }:
        return "Mach-O"
    if data.startswith(b"#!"):
        return "script"
    return "unknown"


def expected_format(os_name: str) -> str:
    if os_name.startswith("windows"):
        return "PE"
    if os_name == "macos":
        return "Mach-O"
    return "ELF"


def command_output(args: list[str]) -> tuple[int, str]:
    completed = subprocess.run(args, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    return completed.returncode, completed.stdout.strip()


def run_smoke(binary: Path, target_dir: Path, windows: bool) -> list[dict[str, object]]:
    commands: list[list[str]] = [
        [str(binary), "--version"],
        [str(binary), "--help"],
    ]
    work = target_dir / "smoke"
    work.mkdir(exist_ok=True)
    source = work / "smoke.vit"
    source.write_text("proc main() -> int {\n  give 0\n}\n", encoding="utf-8")
    output = work / ("smoke.exe" if windows else "smoke")
    commands.append([str(binary), "check", str(source)])
    commands.append([str(binary), "build", str(source), "-o", str(output)])

    results: list[dict[str, object]] = []
    for command in commands:
        code, output_text = command_output(command)
        results.append({"command": command, "exit_code": code, "output": output_text[:4000]})
        if code != 0:
            raise SystemExit(f"smoke command failed: {' '.join(command)}\n{output_text}")
    if output.exists():
        code, output_text = command_output([str(output)])
        results.append({"command": [str(output)], "exit_code": code, "output": output_text[:4000]})
        if code != 0:
            raise SystemExit(f"built smoke executable failed: {output}\n{output_text}")
    return results


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Stage a verified real Vitte binary for strict release gates")
    parser.add_argument("--os", required=True, help="target OS key, for example linux, macos, windows-11")
    parser.add_argument("--arch", required=True, help="target architecture, for example x86_64, i386, arm64")
    parser.add_argument("--binary", required=True, type=Path, help="path to the real vitte executable")
    parser.add_argument("--root", type=Path, default=DEFAULT_ROOT, help="real binary artifact root")
    parser.add_argument("--skip-smoke", action="store_true", help="skip executing smoke commands on this runner")
    args = parser.parse_args(argv)

    source = args.binary.resolve()
    if not source.is_file():
        raise SystemExit(f"missing binary: {source}")
    if not source.stat().st_mode & 0o111:
        raise SystemExit(f"binary is not executable: {source}")

    actual = binary_format(source)
    expected = expected_format(args.os)
    if actual != expected:
        raise SystemExit(f"wrong binary format for {args.os}-{args.arch}: expected {expected}, got {actual}")

    target_dir = args.root / f"{args.os}-{args.arch}"
    target_dir.mkdir(parents=True, exist_ok=True)
    target_name = "vitte.exe" if args.os.startswith("windows") else "vitte"
    target = target_dir / target_name
    shutil.copy2(source, target)
    target.chmod(target.stat().st_mode | 0o755)

    smoke_results: list[dict[str, object]] = []
    if not args.skip_smoke:
        smoke_results = run_smoke(target, target_dir, args.os.startswith("windows"))

    attestation = {
        "schema": "org.vitte.real-binary-attestation.v1",
        "target": {"os": args.os, "arch": args.arch},
        "artifact": str(target.relative_to(ROOT)),
        "format": actual,
        "sha256": sha256(target),
        "size": target.stat().st_size,
        "runner": {
            "system": platform.system(),
            "machine": platform.machine(),
            "platform": platform.platform(),
        },
        "created_at_unix": int(time.time()),
        "smoke_commands": smoke_results,
    }
    (target_dir / "ATTESTATION.json").write_text(json.dumps(attestation, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"[stage-real-binary] staged {target.relative_to(ROOT)} format={actual}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
