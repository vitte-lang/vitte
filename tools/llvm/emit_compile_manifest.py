#!/usr/bin/env python3

from __future__ import annotations

import argparse
import hashlib
import json
import platform
import shutil
import subprocess
import sys
from dataclasses import asdict, dataclass
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]

OUT_DIR = ROOT / "target" / "llvm"
OUT_DIR.mkdir(parents=True, exist_ok=True)

MANIFEST_FILE = OUT_DIR / "compile.manifest"
JSON_FILE = OUT_DIR / "compile.manifest.json"
MARKDOWN_FILE = OUT_DIR / "compile.manifest.md"


LLVM_TOOLS = [
    "clang",
    "clang++",
    "ld.lld",
    "llvm-as",
    "llvm-dis",
    "llvm-link",
    "llvm-ar",
    "llvm-mc",
    "llvm-profdata",
    "llvm-config",
    "llc",
    "opt",
]


SUPPORTED_TARGETS = [
    "x86_64-linux",
    "x86_64-windows",
    "x86_64-macos",
    "aarch64-linux",
    "aarch64-macos",
    "riscv64-linux",
]


@dataclass
class ToolInfo:
    name: str
    found: bool
    path: str
    version: str


def detect_version(binary: str) -> str:
    try:
        proc = subprocess.run(
            [binary, "--version"],
            capture_output=True,
            text=True,
            timeout=5,
        )

        output = proc.stdout or proc.stderr

        if output:
            return output.splitlines()[0].strip()

    except Exception:
        pass

    return ""


def detect_tool(name: str) -> ToolInfo:
    binary = shutil.which(name)

    if not binary:
        return ToolInfo(
            name=name,
            found=False,
            path="",
            version="",
        )

    return ToolInfo(
        name=name,
        found=True,
        path=binary,
        version=detect_version(binary),
    )


def bool_text(value: bool) -> str:
    return "true" if value else "false"


def manifest_checksum(lines: list[str]) -> str:
    payload = "\n".join(lines).encode("utf-8")

    return hashlib.sha256(payload).hexdigest()


def build_manifest(
    args: argparse.Namespace,
    tools: list[ToolInfo],
) -> list[str]:

    lines = [
        "# Vitte LLVM Compile Manifest",
        "",
        f"@generated={datetime.now(timezone.utc).isoformat()}",
        f"@backend={args.backend}",
        f"@target={args.target}",
        f"@profile={args.profile}",
        f"@packaging={args.packaging}",
        f"@deterministic={bool_text(args.deterministic)}",
        f"@debug={bool_text(args.debug)}",
        f"@native-llvm={bool_text(args.native_llvm)}",
        "",
        "# Host",
        f"@platform={platform.platform()}",
        f"@machine={platform.machine()}",
        f"@python={platform.python_version()}",
        "",
        "# LLVM Tools",
    ]

    for tool in tools:
        if tool.found:
            lines.append(
                f"@tool.{tool.name}={tool.path}"
            )

    lines.extend([
        "",
        "# Targets",
    ])

    for target in SUPPORTED_TARGETS:
        lines.append(
            f"@supported-target={target}"
        )

    if args.sample:
        lines.extend([
            "",
            "# Sample Sources",
            "",
            "demo_math.vit=space demo/math;",
            "export *;",
            "proc sum() -> int { give 0; }",
            "",
            "demo_app.vit=space demo/app;",
            "use demo/math { * };",
            "proc main() -> int { give sum(); }",
        ])

    checksum = manifest_checksum(lines)

    lines.extend([
        "",
        f"@checksum={checksum}",
    ])

    return lines


def write_manifest(lines: list[str]):
    MANIFEST_FILE.write_text(
        "\n".join(lines) + "\n",
        encoding="utf-8",
    )


def write_json(
    args: argparse.Namespace,
    tools: list[ToolInfo],
    lines: list[str],
):
    payload = {
        "version": "v1",
        "generated_at":
            datetime.now(timezone.utc).isoformat(),
        "backend": args.backend,
        "target": args.target,
        "profile": args.profile,
        "packaging": args.packaging,
        "deterministic": args.deterministic,
        "debug": args.debug,
        "native_llvm": args.native_llvm,
        "supported_targets":
            SUPPORTED_TARGETS,
        "toolchain": [
            asdict(tool)
            for tool in tools
        ],
        "manifest_sha256":
            manifest_checksum(lines),
    }

    JSON_FILE.write_text(
        json.dumps(
            payload,
            indent=2,
            ensure_ascii=False,
        )
        + "\n",
        encoding="utf-8",
    )


def write_markdown(
    args: argparse.Namespace,
    tools: list[ToolInfo],
):
    lines = [
        "# LLVM Compile Manifest",
        "",
        "## Build",
        "",
        f"- Backend: {args.backend}",
        f"- Target: {args.target}",
        f"- Profile: {args.profile}",
        f"- Packaging: {args.packaging}",
        "",
        "## Toolchain",
        "",
        "| Tool | Found | Version |",
        "|------|------|------|",
    ]

    for tool in tools:
        lines.append(
            f"| {tool.name} | "
            f"{tool.found} | "
            f"{tool.version} |"
        )

    lines.extend([
        "",
        "## Supported Targets",
        "",
    ])

    for target in SUPPORTED_TARGETS:
        lines.append(
            f"- {target}"
        )

    MARKDOWN_FILE.write_text(
        "\n".join(lines) + "\n",
        encoding="utf-8",
    )


def main() -> int:
    parser = argparse.ArgumentParser(
        description=
        "Emit Vitte LLVM compile manifest."
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

    args = parser.parse_args()

    tools = [
        detect_tool(tool)
        for tool in LLVM_TOOLS
    ]

    manifest = build_manifest(
        args,
        tools,
    )

    write_manifest(manifest)

    write_json(
        args,
        tools,
        manifest,
    )

    write_markdown(
        args,
        tools,
    )

    print(
        f"[llvm] manifest: {MANIFEST_FILE}"
    )

    print(
        f"[llvm] json: {JSON_FILE}"
    )

    print(
        f"[llvm] markdown: {MARKDOWN_FILE}"
    )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
