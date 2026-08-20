#!/usr/bin/env python3
"""Exercise native object assembly, linking, and execution through the host ABI."""

from __future__ import annotations

import json
import os
import platform
import subprocess
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
RUNTIME_DIR = ROOT / "src/vitte/compiler/backends/runtime_c"
RUNTIME_C = RUNTIME_DIR / "vitte_runtime.c"
RUNTIME_H = RUNTIME_DIR / "vitte_runtime.h"
BRIDGE = ROOT / "src/vitte/compiler/backend/native_bridge.vit"
TOOLCHAIN = ROOT / "src/vitte/compiler/backend/native_toolchain.vit"
REPORT = ROOT / "target/reports/native_object_link_stability.json"


HARNESS = r'''
#include "vitte_runtime.h"

#include <string.h>

static VitteString text(const char *value) {
  VitteString out;
  out.data = value;
  out.len = strlen(value);
  return out;
}

int main(int argc, char **argv) {
  static const char assembly[] = __ASSEMBLY_LITERAL__;
  if (argc != 3) {
    return 64;
  }
  if (vitte_host_emit_assembly_object(text(assembly), text("clang"), text("__TARGET__"), text(""), text(argv[1]), 0) != 0) {
    return 65;
  }
__VERIFY_BLOCK__
  if (vitte_host_link_executable(text("cc"), text("__TARGET__"), text(""), text(argv[1]), text("src/vitte/compiler/backends/runtime_c/vitte_runtime.c"), text("src/vitte/compiler/backends/runtime_c"), text(argv[2])) != 0) {
    return 67;
  }
  return vitte_host_run_executable(text(argv[2])) == 0 ? 0 : 68;
}
'''


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8") if path.is_file() else ""


def host_profile() -> tuple[str, list[str], bool]:
    system = platform.system()
    machine = platform.machine()
    if system == "Darwin" and machine in ("arm64", "aarch64"):
        return "arm64-apple-darwin", [
            ".text", ".globl _main", "_main:", "  mov w0, #0", "  ret",
        ], False
    if system == "Darwin":
        return "x86_64-apple-darwin", [
            ".text", ".globl _main", "_main:", "  mov $0, %eax", "  ret",
        ], False
    if machine in ("arm64", "aarch64"):
        return "aarch64-unknown-linux-gnu", [
            ".cfi_sections .eh_frame", ".text", ".globl main", ".type main,@function", "main:",
            "  .cfi_startproc", "  mov w0, #0", "  ret", "  .cfi_endproc",
            ".size main, .-main", ".section .note.GNU-stack,\"\",@progbits",
        ], True
    return "x86_64-unknown-linux-gnu", [
        ".cfi_sections .eh_frame", ".text", ".globl main", ".type main,@function", "main:",
        "  .cfi_startproc", "  mov $0, %eax", "  ret", "  .cfi_endproc",
        ".size main, .-main", ".section .note.GNU-stack,\"\",@progbits",
    ], True


def c_string_literal(lines: list[str]) -> str:
    escaped = []
    for line in lines:
        escaped.append('"' + line.replace('\\', '\\\\').replace('"', '\\"') + '\\n"')
    return "\n".join(escaped)


def main() -> int:
    failures: list[str] = []
    source_checks = {
        "bridge_plans_link": "proc link_native_executable_with_context_and_toolchain(" in read(BRIDGE),
        "bridge_calls_host_linker": "host_link_executable(command.tool" in read(BRIDGE),
        "toolchain_plans_link": "proc plan_link_executable_command(" in read(TOOLCHAIN),
        "runtime_links_object_and_runtime": all(
            needle in read(RUNTIME_C)
            for needle in ("argv[arg_index++] = object_c;", "argv[arg_index++] = runtime_c;", 'argv[arg_index++] = "-o";')
        ),
        "runtime_exposes_run": "int32_t vitte_host_run_executable(" in read(RUNTIME_H),
    }
    failures.extend(name for name, passed in source_checks.items() if not passed)
    evidence: dict[str, object] = {"source_checks": source_checks}

    if not RUNTIME_C.is_file() or not RUNTIME_H.is_file():
        failures.append("runtime C bridge sources are missing")
    elif not shutil_which("cc") or not shutil_which("clang"):
        failures.append("cc and clang are required for native link probe")
    else:
        target, assembly_lines, verify_object = host_profile()
        verify_block = ""
        if verify_object:
            verify_block = f'''  if (vitte_host_verify_native_object(text(argv[1]), text("{target}"), text("main"), 0, 0) != 0) {{
    return 66;
  }}
'''
        harness_text = HARNESS.replace("__TARGET__", target)
        harness_text = harness_text.replace("__ASSEMBLY_LITERAL__", c_string_literal(assembly_lines))
        harness_text = harness_text.replace("__VERIFY_BLOCK__", verify_block)
        evidence["target"] = target
        evidence["verify_object"] = verify_object
        with tempfile.TemporaryDirectory(prefix="vitte-native-link-") as directory:
            root = Path(directory)
            harness = root / "link_probe.c"
            probe = root / "link_probe"
            object_path = root / "main.o"
            executable = root / "linked-probe"
            harness.write_text(harness_text, encoding="utf-8")
            build = subprocess.run(
                ["cc", "-std=c17", "-Wall", "-Wextra", "-Werror", "-I", str(RUNTIME_DIR), str(harness), str(RUNTIME_C), "-o", str(probe)],
                cwd=ROOT,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                check=False,
            )
            evidence["harness_build"] = {"exit_code": build.returncode, "stderr": build.stderr[-4000:]}
            if build.returncode != 0:
                failures.append("native link probe harness failed to compile")
            else:
                run = subprocess.run(
                    [str(probe), str(object_path), str(executable)],
                    cwd=ROOT,
                    env={**os.environ, "SOURCE_DATE_EPOCH": "0"},
                    text=True,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    check=False,
                )
                evidence["probe"] = {
                    "exit_code": run.returncode,
                    "stdout": run.stdout[-2000:],
                    "stderr": run.stderr[-4000:],
                    "object_emitted": object_path.is_file(),
                    "executable_linked": executable.is_file(),
                }
                if run.returncode != 0:
                    failures.append(f"native object link/run probe failed with exit code {run.returncode}")
                if not object_path.is_file():
                    failures.append("native link probe did not emit an object")
                if not executable.is_file():
                    failures.append("native link probe did not emit an executable")

    payload = {
        "schema": "vitte.compiler.native_object_link_stability",
        "schema_version": "1.0.0",
        "status": "fail" if failures else "pass",
        "evidence": evidence,
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"[native-object-link] status={payload['status']} source_checks={len(source_checks)}")
    for failure in failures:
        print(f"[native-object-link][error] {failure}", file=sys.stderr)
    return 1 if failures else 0


def shutil_which(name: str) -> str | None:
    for directory in os.environ.get("PATH", "").split(os.pathsep):
        candidate = Path(directory) / name
        if candidate.is_file() and os.access(candidate, os.X_OK):
            return str(candidate)
    return None


if __name__ == "__main__":
    raise SystemExit(main())
