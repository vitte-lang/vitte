#!/usr/bin/env python3
from __future__ import annotations

import json
import shutil
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNTIME_DIR = ROOT / "src" / "vitte" / "compiler" / "backends" / "runtime_c"
RUNTIME_C = RUNTIME_DIR / "vitte_runtime.c"
REPORT = ROOT / "target" / "reports" / "native_object_determinism.json"

PROBE_C = r'''
#include "vitte_runtime.h"

#include <stdio.h>
#include <string.h>

static VitteString text(const char *value) {
  VitteString out;
  out.data = value;
  out.len = strlen(value);
  return out;
}

int main(int argc, char **argv) {
  static const char assembly[] =
      ".text\n"
      ".globl vitte_probe\n"
      ".type vitte_probe,@function\n"
      "vitte_probe:\n"
      "  mov $7, %eax\n"
      "  ret\n"
      ".size vitte_probe, .-vitte_probe\n"
      ".section .note.GNU-stack,\"\",@progbits\n";
  if (argc != 3) {
    return 64;
  }
  if (vitte_host_emit_assembly_object(text(assembly), text("x86_64-unknown-linux-gnu"), text(argv[1])) != 0) {
    return 65;
  }
  if (vitte_host_emit_assembly_object(text(assembly), text("x86_64-unknown-linux-gnu"), text(argv[2])) != 0) {
    return 66;
  }
  return 0;
}
'''


def run(command: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        command,
        cwd=ROOT,
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )


def elf_contract(data: bytes) -> tuple[bool, list[str]]:
    failures: list[str] = []
    if len(data) < 64:
        failures.append("object is smaller than an ELF64 header")
        return False, failures
    if data[:4] != b"\x7fELF":
        failures.append("object does not have ELF magic")
    if data[4] != 2:
        failures.append("object is not ELF64")
    if data[5] != 1:
        failures.append("object is not little-endian")
    object_type, machine = struct.unpack_from("<HH", data, 16)
    if object_type != 1:
        failures.append(f"ELF type is {object_type}, expected ET_REL")
    if machine != 62:
        failures.append(f"ELF machine is {machine}, expected EM_X86_64")
    section_offset = struct.unpack_from("<Q", data, 40)[0]
    section_count = struct.unpack_from("<H", data, 60)[0]
    if section_offset == 0 or section_count < 4:
        failures.append("ELF section table is missing or incomplete")
    return not failures, failures


def write_report(status: str, checks: dict[str, bool], failures: list[str]) -> None:
    payload = {
        "schema": "vitte.compiler.native_object_determinism",
        "schema_version": "1.0.0",
        "status": status,
        "target": "x86_64-unknown-linux-gnu",
        "format": "elf64-relocatable",
        "checks": checks,
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, ensure_ascii=True, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def main() -> int:
    failures: list[str] = []
    checks = {
        "clang_available": shutil.which("clang") is not None,
        "c_compiler_available": shutil.which("cc") is not None,
        "runtime_probe_built": False,
        "objects_emitted": False,
        "objects_identical": False,
        "elf_contract_valid": False,
        "assembly_sidecars_removed": False,
    }
    if not checks["clang_available"]:
        failures.append("clang is unavailable")
    if not checks["c_compiler_available"]:
        failures.append("cc is unavailable")
    if failures:
        write_report("fail", checks, failures)
        return 1

    with tempfile.TemporaryDirectory(prefix="vitte-native-object-") as raw_tmp:
        tmp = Path(raw_tmp)
        probe_c = tmp / "probe.c"
        probe_bin = tmp / "probe"
        first = tmp / "first.o"
        second = tmp / "second.o"
        probe_c.write_text(PROBE_C, encoding="utf-8")
        built = run([
            "cc",
            "-std=c11",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-I",
            str(RUNTIME_DIR),
            str(probe_c),
            str(RUNTIME_C),
            "-o",
            str(probe_bin),
        ])
        checks["runtime_probe_built"] = built.returncode == 0 and probe_bin.is_file()
        if not checks["runtime_probe_built"]:
            failures.append("runtime probe build failed: " + built.stderr.strip())
        else:
            emitted = run([str(probe_bin), str(first), str(second)])
            checks["objects_emitted"] = emitted.returncode == 0 and first.is_file() and second.is_file()
            if not checks["objects_emitted"]:
                failures.append("assembly object emission failed: " + emitted.stderr.strip())
            else:
                first_data = first.read_bytes()
                second_data = second.read_bytes()
                checks["objects_identical"] = first_data == second_data
                checks["elf_contract_valid"], elf_failures = elf_contract(first_data)
                failures.extend(elf_failures)
                if not checks["objects_identical"]:
                    failures.append("repeated object emission is not byte-identical")
            checks["assembly_sidecars_removed"] = not Path(str(first) + ".s").exists() and not Path(str(second) + ".s").exists()
            if not checks["assembly_sidecars_removed"]:
                failures.append("assembly sidecar was not removed")

    status = "pass" if not failures and all(checks.values()) else "fail"
    write_report(status, checks, failures)
    print(f"[native-object-determinism] status={status} report={REPORT.relative_to(ROOT)}")
    for name, passed in checks.items():
        print(f"[native-object-determinism][check] {'pass' if passed else 'fail'} {name}")
    for failure in failures:
        print(f"[native-object-determinism][error] {failure}", file=sys.stderr)
    return 0 if status == "pass" else 1


if __name__ == "__main__":
    raise SystemExit(main())
