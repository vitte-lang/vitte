#!/usr/bin/env python3

from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform
import shutil
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

TOOLS = [
    "clang",
    "clang++",
    "ld.lld",
    "llvm-as",
    "llvm-link",
    "llvm-dis",
    "llvm-ar",
    "llvm-profdata",
    "llc",
    "llvm-mc",
    "opt",
    "ar",
]

DEFAULT_TARGETS = [
    "x86_64-linux",
    "x86_64-windows",
    "x86_64-macos",
    "aarch64-linux",
    "aarch64-macos",
    "riscv64-linux",
]


def bool_text(value: bool) -> str:
    return "true" if value else "false"


def sha256_file(path: str) -> str:
    try:
        with open(path, "rb") as f:
            return hashlib.sha256(f.read()).hexdigest()
    except Exception:
        return ""


def detect_tool(tool: str) -> dict:
    binary = shutil.which(tool)

    if not binary:
        return {
            "found": False,
            "path": "",
            "version": "",
            "sha256": "",
        }

    version = ""

    try:
        proc = subprocess.run(
            [binary, "--version"],
            capture_output=True,
            text=True,
            timeout=3,
        )

        output = proc.stdout or proc.stderr

        if output:
            version = output.splitlines()[0]

    except Exception:
        pass

    return {
        "found": True,
        "path": binary,
        "version": version,
        "sha256": sha256_file(binary),
    }


def detect_toolchain() -> dict:
    return {
        tool: detect_tool(tool)
        for tool in TOOLS
    }


def toolchain_score(toolchain: dict) -> int:
    found = sum(
        1
        for item in toolchain.values()
        if item["found"]
    )

    return round(found * 100 / len(TOOLS))


def render_manifest(
    args: argparse.Namespace,
    toolchain: dict,
) -> str:

    lines = [
        f"@backend={args.backend}",
        f"@target={args.target}",
        f"@profile={args.profile}",
        f"@packaging={args.packaging}",
        f"@deterministic={bool_text(args.deterministic)}",
        f"@debug={bool_text(args.debug)}",
        f"@native-llvm={bool_text(args.native_llvm)}",
        f"@toolchain-score={toolchain_score(toolchain)}",
    ]

    for tool, info in toolchain.items():
        if info["found"]:
            lines.append(f"@{tool}={info['path']}")

    if args.sample:
        lines.extend(
            [
                "",
                "demo_math.vit=space demo/math;",
                "export *;",
                "proc sum() -> int { give 0; }",
                "",
                "demo_app.vit=space demo/app;",
                "use demo/math { * };",
                "proc main() -> int { give sum(); }",
            ]
        )

    return "\n".join(lines) + "\n"


def build_report(
    args: argparse.Namespace,
    toolchain: dict,
) -> dict:

    return {
        "version": "v1",
        "generated_at": datetime.now(
            timezone.utc
        ).isoformat(),
        "host": {
            "platform": platform.platform(),
            "machine": platform.machine(),
            "python": platform.python_version(),
        },
        "build": {
            "backend": args.backend,
            "target": args.target,
            "profile": args.profile,
            "packaging": args.packaging,
            "deterministic": args.deterministic,
            "debug": args.debug,
            "native_llvm": args.native_llvm,
        },
        "toolchain": toolchain,
        "supported_targets": DEFAULT_TARGETS,
        "score": toolchain_score(toolchain),
        "ci": {
            "github_actions":
                "GITHUB_ACTIONS" in os.environ,
            "gitlab_ci":
                "GITLAB_CI" in os.environ,
        },
    }


def main() -> int:

    parser = argparse.ArgumentParser(
        description=(
            "Emit a Vitte LLVM compile manifest "
            "and toolchain report."
        )
    )

    parser.add_argument(
        "--backend",
        default="llvm",
    )

    parser.add_argument(
        "--target",
        default="x86_64-linux",
    )

    parser.add_argument(
        "--profile",
        default="release",
    )

    parser.add_argument(
        "--packaging",
        default="executable",
    )

    parser.add_argument(
        "--deterministic",
        action=argparse.BooleanOptionalAction,
        default=True,
    )

    parser.add_argument(
        "--debug",
        action=argparse.BooleanOptionalAction,
        default=True,
    )

    parser.add_argument(
        "--native-llvm",
        action=argparse.BooleanOptionalAction,
        default=True,
    )

    parser.add_argument(
        "--sample",
        action="store_true",
    )

    parser.add_argument(
        "--json",
        action="store_true",
    )

    args = parser.parse_args()

    toolchain = detect_toolchain()

    if args.json:
        json.dump(
            build_report(args, toolchain),
            sys.stdout,
            indent=2,
        )
        sys.stdout.write("\n")
    else:
        sys.stdout.write(
            render_manifest(
                args,
                toolchain,
            )
        )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())