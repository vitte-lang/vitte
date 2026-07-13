#!/usr/bin/env python3
from __future__ import annotations

import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNTIME_DIR = ROOT / "src" / "vitte" / "compiler" / "backends" / "runtime_c"
RUNTIME_C = RUNTIME_DIR / "vitte_runtime.c"
REPORT = ROOT / "target" / "reports" / "backend_cross_sysroot.json"
TARGET = "aarch64-unknown-linux-gnu"

FAKE_TOOL_C = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
  const char *log_path = getenv("VITTE_CAPTURE_LOG");
  const char *output_path = NULL;
  FILE *log;
  int i;
  if (log_path == NULL || log_path[0] == '\0') {
    return 64;
  }
  log = fopen(log_path, "wb");
  if (log == NULL) {
    return 65;
  }
  for (i = 1; i < argc; ++i) {
    fprintf(log, "%s\n", argv[i]);
    if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
      output_path = argv[i + 1];
    }
  }
  if (fclose(log) != 0) {
    return 66;
  }
  if (output_path != NULL) {
    FILE *output = fopen(output_path, "wb");
    if (output == NULL) {
      return 67;
    }
    fputs("vitte-cross-tool-output\n", output);
    if (fclose(output) != 0) {
      return 68;
    }
  }
  return 0;
}
'''

PROBE_C = r'''
#define _POSIX_C_SOURCE 200809L
#include "vitte_runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static VitteString text(const char *value) {
  VitteString out;
  out.data = value;
  out.len = strlen(value);
  return out;
}

static int capture(const char *path) {
  return setenv("VITTE_CAPTURE_LOG", path, 1);
}

int main(int argc, char **argv) {
  char llvm_object[1024];
  char assembly_object[1024];
  char input_object[1024];
  char executable[1024];
  char compiler_log[1024];
  char assembler_log[1024];
  char linker_log[1024];
  FILE *input;
  if (argc != 6) {
    return 64;
  }
  snprintf(llvm_object, sizeof(llvm_object), "%s/llvm.o", argv[3]);
  snprintf(assembly_object, sizeof(assembly_object), "%s/assembly.o", argv[3]);
  snprintf(input_object, sizeof(input_object), "%s/input.o", argv[3]);
  snprintf(executable, sizeof(executable), "%s/program", argv[3]);
  snprintf(compiler_log, sizeof(compiler_log), "%s/compiler.args", argv[3]);
  snprintf(assembler_log, sizeof(assembler_log), "%s/assembler.args", argv[3]);
  snprintf(linker_log, sizeof(linker_log), "%s/linker.args", argv[3]);

  if (capture(compiler_log) != 0 ||
      vitte_host_emit_llvm_object(text("define i32 @main() { ret i32 0 }\n"), text(argv[1]), text("aarch64-unknown-linux-gnu"), text(argv[2]), text(llvm_object)) != 0) {
    return 70;
  }
  if (vitte_host_emit_llvm_object(text("define i32 @main() { ret i32 0 }\n"), text(argv[1]), text(""), text(argv[2]), text(llvm_object)) != -1) {
    return 71;
  }
  if (capture(assembler_log) != 0 ||
      vitte_host_emit_assembly_object(text(".text\n"), text(argv[1]), text("aarch64-unknown-linux-gnu"), text(argv[2]), text(assembly_object), 0) != 0) {
    return 72;
  }
  input = fopen(input_object, "wb");
  if (input == NULL) {
    return 73;
  }
  fputs("input\n", input);
  if (fclose(input) != 0) {
    return 74;
  }
  if (capture(linker_log) != 0 ||
      vitte_host_link_executable(text(argv[1]), text("aarch64-unknown-linux-gnu"), text(argv[2]), text(input_object), text(argv[4]), text(argv[5]), text(executable)) != 0) {
    return 75;
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


def read_args(path: Path) -> list[str]:
    if not path.is_file():
        return []
    return path.read_text(encoding="utf-8", errors="replace").splitlines()


def write_report(status: str, checks: dict[str, bool], failures: list[str]) -> None:
    payload = {
        "schema": "vitte.compiler.backend_cross_sysroot",
        "schema_version": "1.0.0",
        "status": status,
        "target": TARGET,
        "checks": checks,
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, ensure_ascii=True, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def main() -> int:
    compiler = shutil.which("cc")
    checks = {
        "c_compiler_available": compiler is not None,
        "fake_tool_built": False,
        "runtime_probe_built": False,
        "runtime_probe_passed": False,
        "compiler_received_target": False,
        "compiler_received_sysroot": False,
        "assembler_received_target": False,
        "assembler_received_sysroot": False,
        "linker_received_target": False,
        "linker_received_sysroot": False,
        "argv_order_is_stable": False,
        "cross_outputs_created": False,
        "assembly_sidecar_removed": False,
    }
    failures: list[str] = []
    if compiler is None:
        failures.append("C compiler is unavailable")
        write_report("fail", checks, failures)
        return 1

    with tempfile.TemporaryDirectory(prefix="vitte-cross-sysroot-") as raw_tmp:
        tmp = Path(raw_tmp)
        sysroot = tmp / "sysroot"
        sysroot.mkdir()
        fake_c = tmp / "fake_tool.c"
        fake_tool = tmp / "fake_tool"
        probe_c = tmp / "probe.c"
        probe = tmp / "probe"
        fake_c.write_text(FAKE_TOOL_C, encoding="utf-8")
        probe_c.write_text(PROBE_C, encoding="utf-8")

        fake_build = run([compiler, "-std=c11", "-Wall", "-Wextra", "-Werror", str(fake_c), "-o", str(fake_tool)])
        checks["fake_tool_built"] = fake_build.returncode == 0 and fake_tool.is_file()
        if not checks["fake_tool_built"]:
            failures.append("fake tool build failed: " + fake_build.stderr.strip())

        probe_build = run([
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
        checks["runtime_probe_built"] = probe_build.returncode == 0 and probe.is_file()
        if not checks["runtime_probe_built"]:
            failures.append("runtime probe build failed: " + probe_build.stderr.strip())

        if checks["fake_tool_built"] and checks["runtime_probe_built"]:
            executed = run([
                str(probe),
                str(fake_tool),
                str(sysroot),
                str(tmp),
                str(RUNTIME_C),
                str(RUNTIME_DIR),
            ])
            checks["runtime_probe_passed"] = executed.returncode == 0
            if executed.returncode != 0:
                failures.append(f"runtime probe failed with exit {executed.returncode}: {executed.stderr.strip()}")

        llvm_object = tmp / "llvm.o"
        assembly_object = tmp / "assembly.o"
        executable = tmp / "program"
        compiler_args = read_args(tmp / "compiler.args")
        assembler_args = read_args(tmp / "assembler.args")
        linker_args = read_args(tmp / "linker.args")
        target_prefix = ["-target", TARGET, "--sysroot", str(sysroot)]
        checks["compiler_received_target"] = compiler_args[:2] == target_prefix[:2]
        checks["compiler_received_sysroot"] = compiler_args[2:4] == target_prefix[2:4]
        checks["assembler_received_target"] = assembler_args[:2] == target_prefix[:2]
        checks["assembler_received_sysroot"] = assembler_args[2:4] == target_prefix[2:4]
        checks["linker_received_target"] = linker_args[:2] == target_prefix[:2]
        checks["linker_received_sysroot"] = linker_args[2:4] == target_prefix[2:4]
        checks["argv_order_is_stable"] = (
            compiler_args == target_prefix + ["-Wno-override-module", "-c", str(llvm_object) + ".ll", "-o", str(llvm_object)]
            and assembler_args == target_prefix + ["-x", "assembler", "-c", str(assembly_object) + ".s", "-o", str(assembly_object)]
            and linker_args == target_prefix + [str(tmp / "input.o"), str(RUNTIME_C), "-I", str(RUNTIME_DIR), "-o", str(executable)]
        )
        checks["cross_outputs_created"] = llvm_object.is_file() and assembly_object.is_file() and executable.is_file()
        checks["assembly_sidecar_removed"] = not Path(str(assembly_object) + ".s").exists()

    for name, passed in checks.items():
        if not passed:
            failures.append(f"check failed: {name}")
    status = "pass" if not failures else "fail"
    write_report(status, checks, failures)
    print(f"[backend-cross-sysroot] status={status} report={REPORT.relative_to(ROOT)}")
    for name, passed in checks.items():
        print(f"[backend-cross-sysroot][check] {'pass' if passed else 'fail'} {name}")
    for failure in failures:
        print(f"[backend-cross-sysroot][error] {failure}", file=sys.stderr)
    return 0 if status == "pass" else 1


if __name__ == "__main__":
    raise SystemExit(main())
