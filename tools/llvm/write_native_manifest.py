#!/usr/bin/env python3

from __future__ import annotations

import argparse
import shutil
import sys


def detect(tool: str) -> str:
    path = shutil.which(tool)
    return path or ""


def bool_text(value: bool) -> str:
    return "true" if value else "false"


def render_manifest(args: argparse.Namespace) -> str:
    clang = detect("clang")
    lld = detect("ld.lld")
    llvm_as = detect("llvm-as")
    llc = detect("llc")
    llvm_mc = detect("llvm-mc")
    ar = detect("ar")
    lines = [
        f"@backend={args.backend}",
        f"@target={args.target}",
        f"@profile={args.profile}",
        f"@packaging={args.packaging}",
        f"@deterministic={bool_text(args.deterministic)}",
        f"@debug={bool_text(args.debug)}",
        f"@native-llvm={bool_text(args.native_llvm)}",
    ]
    if clang:
        lines.append(f"@clang={clang}")
    if lld:
        lines.append(f"@lld={lld}")
    if llvm_as:
        lines.append(f"@llvm-as={llvm_as}")
    if llc:
        lines.append(f"@llc={llc}")
    if llvm_mc:
        lines.append(f"@llvm-mc={llvm_mc}")
    if ar:
        lines.append(f"@ar={ar}")
    if args.sample:
        lines.extend(
            [
                r"demo_math.vit=space demo/math;\nexport *;\nproc sum() -> int { give 0; }\n",
                r"demo_app.vit=space demo/app;\nuse demo/math { * };\nproc main() -> int { give sum(); }\n",
            ]
        )
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description="Emit a compile-set manifest with detected LLVM toolchain paths.")
    parser.add_argument("--backend", default="llvm")
    parser.add_argument("--target", default="x86_64-linux")
    parser.add_argument("--profile", default="enterprise-default")
    parser.add_argument("--packaging", default="executable")
    parser.add_argument("--deterministic", action=argparse.BooleanOptionalAction, default=True)
    parser.add_argument("--debug", action=argparse.BooleanOptionalAction, default=True)
    parser.add_argument("--native-llvm", action=argparse.BooleanOptionalAction, default=True)
    parser.add_argument("--sample", action="store_true", help="append a minimal multi-module sample to the manifest")
    sys.stdout.write(render_manifest(parser.parse_args()))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
