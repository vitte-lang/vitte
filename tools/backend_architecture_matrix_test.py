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
REPORT = ROOT / "target" / "reports" / "backend_architecture_matrix.json"

PROBE_C = r'''
#include "vitte_runtime.h"

#include <string.h>

static VitteString text(const char *value) {
  VitteString out;
  out.data = value;
  out.len = strlen(value);
  return out;
}

static int emit_pair(const char *assembly, const char *assembler, const char *target,
                     const char *first, const char *second) {
  int verify;
  if (vitte_host_emit_assembly_object(text(assembly), text(assembler), text(target), text(first), 0) != 0) {
    return 1;
  }
  if (vitte_host_emit_assembly_object(text(assembly), text(assembler), text(target), text(second), 0) != 0) {
    return 2;
  }
  verify = vitte_host_verify_native_object(text(first), text(target), text("vitte_arch_probe"), 0, 0);
  if (verify != 0) {
    return 20 + verify;
  }
  verify = vitte_host_verify_native_object(text(second), text(target), text("vitte_arch_probe"), 0, 0);
  if (verify != 0) {
    return 40 + verify;
  }
  return 0;
}

int main(int argc, char **argv) {
  static const char x86_64_assembly[] =
      ".cfi_sections .eh_frame\n"
      ".text\n"
      ".p2align 4\n"
      ".globl vitte_arch_probe\n"
      ".type vitte_arch_probe,@function\n"
      "vitte_arch_probe:\n"
      "  .cfi_startproc\n"
      "  mov $7, %eax\n"
      "  ret\n"
      "  .cfi_endproc\n"
      ".size vitte_arch_probe, .-vitte_arch_probe\n"
      ".section .note.GNU-stack,\"\",@progbits\n";
  static const char aarch64_assembly[] =
      ".cfi_sections .eh_frame\n"
      ".text\n"
      ".p2align 4\n"
      ".globl vitte_arch_probe\n"
      ".type vitte_arch_probe,@function\n"
      "vitte_arch_probe:\n"
      "  .cfi_startproc\n"
      "  mov w0, #7\n"
      "  ret\n"
      "  .cfi_endproc\n"
      ".size vitte_arch_probe, .-vitte_arch_probe\n"
      ".section .note.GNU-stack,\"\",@progbits\n";
  int code;
  if (argc != 6) {
    return 64;
  }
  code = emit_pair(x86_64_assembly, argv[1], "x86_64-unknown-linux-gnu", argv[2], argv[3]);
  if (code != 0) {
    return 70 + code;
  }
  code = emit_pair(aarch64_assembly, argv[1], "aarch64-unknown-linux-gnu", argv[4], argv[5]);
  if (code != 0) {
    return 130 + code;
  }
  if (vitte_host_verify_native_object(text(argv[2]), text("aarch64-unknown-linux-gnu"), text("vitte_arch_probe"), 0, 0) != 6) {
    return 200;
  }
  if (vitte_host_verify_native_object(text(argv[4]), text("x86_64-unknown-linux-gnu"), text("vitte_arch_probe"), 0, 0) != 6) {
    return 201;
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


def c_string(data: bytes, offset: int) -> str:
    if offset < 0 or offset >= len(data):
        return ""
    end = data.find(b"\0", offset)
    if end < 0:
        return ""
    return data[offset:end].decode("utf-8", errors="replace")


def elf_sections(data: bytes) -> set[str]:
    if len(data) < 64 or data[:4] != b"\x7fELF":
        return set()
    section_offset = struct.unpack_from("<Q", data, 40)[0]
    section_size = struct.unpack_from("<H", data, 58)[0]
    section_count = struct.unpack_from("<H", data, 60)[0]
    names_index = struct.unpack_from("<H", data, 62)[0]
    if section_size < 64 or names_index >= section_count:
        return set()
    names_header = section_offset + names_index * section_size
    if names_header + 64 > len(data):
        return set()
    names_offset, names_size = struct.unpack_from("<QQ", data, names_header + 24)
    if names_offset + names_size > len(data):
        return set()
    names_data = data[names_offset:names_offset + names_size]
    names: set[str] = set()
    for index in range(section_count):
        header = section_offset + index * section_size
        if header + 64 > len(data):
            return set()
        names.add(c_string(names_data, struct.unpack_from("<I", data, header)[0]))
    return names


def elf_contract(data: bytes, expected_machine: int) -> tuple[bool, list[str]]:
    failures: list[str] = []
    if len(data) < 64 or data[:4] != b"\x7fELF":
        return False, ["object does not contain an ELF64 header"]
    if data[4] != 2:
        failures.append("object is not ELF64")
    if data[5] != 1:
        failures.append("object is not little-endian")
    object_type, machine = struct.unpack_from("<HH", data, 16)
    if object_type != 1:
        failures.append(f"ELF type is {object_type}, expected ET_REL")
    if machine != expected_machine:
        failures.append(f"ELF machine is {machine}, expected {expected_machine}")
    required = {".text", ".symtab", ".strtab", ".eh_frame", ".note.GNU-stack"}
    missing = sorted(required - elf_sections(data))
    if missing:
        failures.append("missing sections: " + ", ".join(missing))
    return not failures, failures


def write_report(status: str, checks: dict[str, bool], failures: list[str]) -> None:
    payload = {
        "schema": "vitte.compiler.backend_architecture_matrix",
        "schema_version": "1.0.0",
        "status": status,
        "targets": ["x86_64-unknown-linux-gnu", "aarch64-unknown-linux-gnu"],
        "format": "elf64-relocatable",
        "checks": checks,
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, ensure_ascii=True, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def main() -> int:
    compiler = shutil.which("cc")
    assembler = shutil.which("clang")
    checks = {
        "c_compiler_available": compiler is not None,
        "clang_available": assembler is not None,
        "runtime_probe_built": False,
        "x86_64_objects_emitted": False,
        "aarch64_objects_emitted": False,
        "x86_64_objects_deterministic": False,
        "aarch64_objects_deterministic": False,
        "x86_64_elf_contract_valid": False,
        "aarch64_elf_contract_valid": False,
        "runtime_verifier_passed_both_targets": False,
        "runtime_verifier_rejected_machine_mismatch": False,
        "assembly_sidecars_removed": False,
    }
    failures: list[str] = []
    if compiler is None:
        failures.append("C compiler is unavailable")
    if assembler is None:
        failures.append("clang is unavailable")
    if failures:
        write_report("fail", checks, failures)
        return 1

    with tempfile.TemporaryDirectory(prefix="vitte-backend-arch-matrix-") as raw_tmp:
        tmp = Path(raw_tmp)
        probe_c = tmp / "probe.c"
        probe = tmp / "probe"
        x86_first = tmp / "x86-first.o"
        x86_second = tmp / "x86-second.o"
        arm_first = tmp / "arm-first.o"
        arm_second = tmp / "arm-second.o"
        probe_c.write_text(PROBE_C, encoding="utf-8")
        built = run([
            compiler,
            "-std=c11",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-I",
            str(RUNTIME_DIR),
            str(probe_c),
            str(RUNTIME_C),
            "-o",
            str(probe),
        ])
        checks["runtime_probe_built"] = built.returncode == 0 and probe.is_file()
        if not checks["runtime_probe_built"]:
            failures.append("runtime probe build failed: " + built.stderr.strip())
        else:
            executed = run([
                str(probe),
                assembler,
                str(x86_first),
                str(x86_second),
                str(arm_first),
                str(arm_second),
            ])
            checks["runtime_verifier_passed_both_targets"] = executed.returncode == 0
            checks["runtime_verifier_rejected_machine_mismatch"] = executed.returncode == 0
            if executed.returncode != 0:
                failures.append(f"architecture probe failed with exit {executed.returncode}: {executed.stderr.strip()}")

        x86_objects = [path for path in (x86_first, x86_second) if path.is_file()]
        arm_objects = [path for path in (arm_first, arm_second) if path.is_file()]
        checks["x86_64_objects_emitted"] = len(x86_objects) == 2
        checks["aarch64_objects_emitted"] = len(arm_objects) == 2
        if len(x86_objects) == 2:
            x86_data = [path.read_bytes() for path in x86_objects]
            checks["x86_64_objects_deterministic"] = x86_data[0] == x86_data[1]
            valid, contract_failures = elf_contract(x86_data[0], 62)
            checks["x86_64_elf_contract_valid"] = valid
            failures.extend("x86_64: " + failure for failure in contract_failures)
        if len(arm_objects) == 2:
            arm_data = [path.read_bytes() for path in arm_objects]
            checks["aarch64_objects_deterministic"] = arm_data[0] == arm_data[1]
            valid, contract_failures = elf_contract(arm_data[0], 183)
            checks["aarch64_elf_contract_valid"] = valid
            failures.extend("aarch64: " + failure for failure in contract_failures)
        checks["assembly_sidecars_removed"] = not any(tmp.glob("*.o.s"))

    for name, passed in checks.items():
        if not passed and not any(name in failure for failure in failures):
            failures.append(f"check failed: {name}")
    status = "pass" if not failures else "fail"
    write_report(status, checks, failures)
    print(f"[backend-architecture-matrix] status={status} report={REPORT.relative_to(ROOT)}")
    for name, passed in checks.items():
        print(f"[backend-architecture-matrix][check] {'pass' if passed else 'fail'} {name}")
    for failure in failures:
        print(f"[backend-architecture-matrix][error] {failure}", file=sys.stderr)
    return 0 if status == "pass" else 1


if __name__ == "__main__":
    raise SystemExit(main())
