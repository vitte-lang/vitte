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
      ".file 1 \"probe.vit\"\n"
      ".cfi_sections .eh_frame\n"
      ".text\n"
      ".globl vitte_probe\n"
      ".type vitte_probe,@function\n"
      ".extern vitte_external\n"
      "vitte_probe:\n"
      "  .cfi_startproc\n"
      "  push %rbp\n"
      "  .cfi_def_cfa_offset 16\n"
      "  .cfi_offset %rbp, -16\n"
      "  mov %rsp, %rbp\n"
      "  .cfi_def_cfa_register %rbp\n"
      "  .loc 1 1 1\n"
      "  call vitte_external\n"
      "  mov $7, %eax\n"
      "  pop %rbp\n"
      "  .cfi_def_cfa %rsp, 8\n"
      "  ret\n"
      "  .cfi_endproc\n"
      ".size vitte_probe, .-vitte_probe\n"
      ".section .note.GNU-stack,\"\",@progbits\n"
      ".section .debug_abbrev,\"\",@progbits\n"
      ".Lvitte_debug_abbrev:\n"
      "  .uleb128 1\n"
      "  .uleb128 0x11\n"
      "  .byte 0\n"
      "  .uleb128 0x25\n"
      "  .uleb128 0x0e\n"
      "  .uleb128 0x13\n"
      "  .uleb128 0x05\n"
      "  .uleb128 0x03\n"
      "  .uleb128 0x0e\n"
      "  .uleb128 0x1b\n"
      "  .uleb128 0x0e\n"
      "  .uleb128 0x10\n"
      "  .uleb128 0x17\n"
      "  .byte 0\n"
      "  .byte 0\n"
      "  .byte 0\n"
      ".section .debug_str,\"MS\",@progbits,1\n"
      ".Lvitte_debug_producer:\n"
      "  .asciz \"vittec\"\n"
      ".Lvitte_debug_source:\n"
      "  .asciz \"probe.vit\"\n"
      ".Lvitte_debug_directory:\n"
      "  .asciz \".\"\n"
      ".section .debug_info,\"\",@progbits\n"
      ".Lvitte_debug_cu_begin:\n"
      "  .long .Lvitte_debug_cu_end-.Lvitte_debug_cu_version\n"
      ".Lvitte_debug_cu_version:\n"
      "  .short 4\n"
      "  .long .Lvitte_debug_abbrev\n"
      "  .byte 8\n"
      "  .uleb128 1\n"
      "  .long .Lvitte_debug_producer\n"
      "  .short 0x001d\n"
      "  .long .Lvitte_debug_source\n"
      "  .long .Lvitte_debug_directory\n"
      "  .long 0\n"
      ".Lvitte_debug_cu_end:\n";
  static const char release_assembly[] =
      ".cfi_sections .eh_frame\n"
      ".text\n"
      ".globl vitte_release\n"
      ".type vitte_release,@function\n"
      "vitte_release:\n"
      "  .cfi_startproc\n"
      "  mov $9, %eax\n"
      "  ret\n"
      "  .cfi_endproc\n"
      ".size vitte_release, .-vitte_release\n"
      ".section .note.GNU-stack,\"\",@progbits\n";
  if (argc != 4) {
    return 64;
  }
  if (vitte_host_emit_assembly_object(text(assembly), text("x86_64-unknown-linux-gnu"), text(argv[1]), 1) != 0) {
    return 65;
  }
  if (vitte_host_emit_assembly_object(text(assembly), text("x86_64-unknown-linux-gnu"), text(argv[2]), 1) != 0) {
    return 66;
  }
  {
    int verify = vitte_host_verify_native_object(text(argv[1]), text("x86_64-unknown-linux-gnu"), text("vitte_probe"), 1, 1);
    if (verify != 0) {
      return 80 + verify;
    }
  }
  if (vitte_host_verify_native_object(text(argv[2]), text("x86_64-unknown-linux-gnu"), text("vitte_probe"), 1, 1) != 0) {
    return 68;
  }
  if (vitte_host_verify_native_object(text(argv[1]), text("aarch64-unknown-linux-gnu"), text("vitte_probe"), 1, 1) != 6) {
    return 69;
  }
  if (vitte_host_verify_native_object(text(argv[1]), text("x86_64-unknown-linux-gnu"), text("missing_symbol"), 1, 1) != 11) {
    return 70;
  }
  if (vitte_host_emit_assembly_object(text(release_assembly), text("x86_64-unknown-linux-gnu"), text(argv[3]), 0) != 0) {
    return 71;
  }
  if (vitte_host_verify_native_object(text(argv[3]), text("x86_64-unknown-linux-gnu"), text("vitte_release"), 0, 0) != 0) {
    return 72;
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


def elf_section_names(data: bytes) -> set[str]:
    if len(data) < 64 or data[:4] != b"\x7fELF":
        return set()
    section_offset = struct.unpack_from("<Q", data, 40)[0]
    section_entry_size = struct.unpack_from("<H", data, 58)[0]
    section_count = struct.unpack_from("<H", data, 60)[0]
    section_name_index = struct.unpack_from("<H", data, 62)[0]
    if section_entry_size < 64 or section_name_index >= section_count:
        return set()
    name_header_offset = section_offset + section_name_index * section_entry_size
    if name_header_offset + 64 > len(data):
        return set()
    name_start, name_size = struct.unpack_from("<QQ", data, name_header_offset + 24)
    if name_start + name_size > len(data):
        return set()
    names_data = data[name_start:name_start + name_size]
    names: set[str] = set()
    for index in range(section_count):
        header_offset = section_offset + index * section_entry_size
        if header_offset + 64 > len(data):
            return set()
        name_offset = struct.unpack_from("<I", data, header_offset)[0]
        names.add(c_string(names_data, name_offset))
    return names


def elf_contract(data: bytes) -> tuple[bool, list[str], dict[str, bool]]:
    failures: list[str] = []
    details = {
        "required_sections_present": False,
        "defined_symbol_present": False,
        "undefined_symbol_present": False,
        "relocation_targets_external": False,
        "debug_sections_present": False,
        "unwind_sections_present": False,
        "release_debug_sections_absent": False,
        "release_unwind_present": False,
        "dwarf_verifier_available": False,
        "dwarf_verifier_passed": False,
    }
    if len(data) < 64:
        failures.append("object is smaller than an ELF64 header")
        return False, failures, details
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
        return False, failures, details

    section_entry_size = struct.unpack_from("<H", data, 58)[0]
    section_name_index = struct.unpack_from("<H", data, 62)[0]
    if section_entry_size < 64 or section_name_index >= section_count:
        failures.append("ELF section metadata is invalid")
        return False, failures, details
    headers: list[dict[str, int | str]] = []
    for index in range(section_count):
        offset = section_offset + index * section_entry_size
        if offset + 64 > len(data):
            failures.append("ELF section header is out of bounds")
            return False, failures, details
        name_offset, section_type = struct.unpack_from("<II", data, offset)
        body_offset, body_size = struct.unpack_from("<QQ", data, offset + 24)
        link = struct.unpack_from("<I", data, offset + 40)[0]
        entry_size = struct.unpack_from("<Q", data, offset + 56)[0]
        headers.append({
            "name_offset": name_offset,
            "type": section_type,
            "offset": body_offset,
            "size": body_size,
            "link": link,
            "entry_size": entry_size,
        })
    name_header = headers[section_name_index]
    name_start = int(name_header["offset"])
    name_end = name_start + int(name_header["size"])
    if name_start < 0 or name_end > len(data):
        failures.append("ELF section name table is out of bounds")
        return False, failures, details
    section_names = data[name_start:name_end]
    for header in headers:
        header["name"] = c_string(section_names, int(header["name_offset"]))
    names = {str(header["name"]) for header in headers}
    details["required_sections_present"] = {".text", ".symtab", ".strtab", ".rela.text"}.issubset(names)
    details["debug_sections_present"] = {".debug_info", ".debug_line"}.issubset(names)
    details["unwind_sections_present"] = {".eh_frame", ".rela.eh_frame"}.issubset(names)
    if not details["required_sections_present"]:
        failures.append("required ELF sections are missing")
    if not details["debug_sections_present"]:
        failures.append("required DWARF sections are missing")
    if not details["unwind_sections_present"]:
        failures.append("required stack unwind sections are missing")

    symbols: list[tuple[str, int]] = []
    for header in headers:
        if int(header["type"]) != 2:
            continue
        link = int(header["link"])
        if link >= len(headers):
            continue
        strings_header = headers[link]
        strings_start = int(strings_header["offset"])
        strings_end = strings_start + int(strings_header["size"])
        strings = data[strings_start:strings_end]
        entry_size = int(header["entry_size"])
        if entry_size < 24:
            continue
        count = int(header["size"]) // entry_size
        for index in range(count):
            offset = int(header["offset"]) + index * entry_size
            name_offset = struct.unpack_from("<I", data, offset)[0]
            section_index = struct.unpack_from("<H", data, offset + 6)[0]
            symbols.append((c_string(strings, name_offset), section_index))
    details["defined_symbol_present"] = any(name == "vitte_probe" and section_index != 0 for name, section_index in symbols)
    details["undefined_symbol_present"] = any(name == "vitte_external" and section_index == 0 for name, section_index in symbols)
    if not details["defined_symbol_present"]:
        failures.append("defined symbol vitte_probe is missing")
    if not details["undefined_symbol_present"]:
        failures.append("undefined symbol vitte_external is missing")

    for header in headers:
        if int(header["type"]) not in (4, 9):
            continue
        entry_size = int(header["entry_size"])
        if entry_size < 16:
            continue
        count = int(header["size"]) // entry_size
        for index in range(count):
            offset = int(header["offset"]) + index * entry_size
            info = struct.unpack_from("<Q", data, offset + 8)[0]
            symbol_index = info >> 32
            if symbol_index < len(symbols) and symbols[symbol_index][0] == "vitte_external":
                details["relocation_targets_external"] = True
    if not details["relocation_targets_external"]:
        failures.append("no relocation targets vitte_external")
    return not failures, failures, details


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
        "runtime_verifier_passed": False,
        "runtime_verifier_rejects_invalid": False,
        "required_sections_present": False,
        "defined_symbol_present": False,
        "undefined_symbol_present": False,
        "relocation_targets_external": False,
        "debug_sections_present": False,
        "unwind_sections_present": False,
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
        release = tmp / "release.o"
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
            emitted = run([str(probe_bin), str(first), str(second), str(release)])
            checks["objects_emitted"] = first.is_file() and second.is_file() and release.is_file()
            checks["runtime_verifier_passed"] = emitted.returncode == 0
            checks["runtime_verifier_rejects_invalid"] = emitted.returncode == 0
            if not checks["objects_emitted"]:
                failures.append(f"assembly object emission failed with code {emitted.returncode}: " + emitted.stderr.strip())
            else:
                first_data = first.read_bytes()
                second_data = second.read_bytes()
                checks["objects_identical"] = first_data == second_data
                checks["elf_contract_valid"], elf_failures, elf_details = elf_contract(first_data)
                checks.update(elf_details)
                failures.extend(elf_failures)
                if not checks["objects_identical"]:
                    failures.append("repeated object emission is not byte-identical")
                release_names = elf_section_names(release.read_bytes())
                checks["release_debug_sections_absent"] = ".debug_info" not in release_names and ".debug_line" not in release_names
                checks["release_unwind_present"] = ".eh_frame" in release_names
                if not checks["release_debug_sections_absent"]:
                    failures.append("release object unexpectedly contains DWARF sections")
                if not checks["release_unwind_present"]:
                    failures.append("release object has no unwind section")
                dwarf_tool = shutil.which("llvm-dwarfdump") or shutil.which("dwarfdump")
                checks["dwarf_verifier_available"] = dwarf_tool is not None
                if dwarf_tool is not None:
                    verified_dwarf = run([dwarf_tool, "--verify", str(first)])
                    checks["dwarf_verifier_passed"] = verified_dwarf.returncode == 0
                    if not checks["dwarf_verifier_passed"]:
                        failures.append("DWARF verification failed: " + verified_dwarf.stderr.strip())
            checks["assembly_sidecars_removed"] = not Path(str(first) + ".s").exists() and not Path(str(second) + ".s").exists() and not Path(str(release) + ".s").exists()
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
