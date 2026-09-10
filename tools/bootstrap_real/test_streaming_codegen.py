#!/usr/bin/env python3
"""Small executable regressions for the memory-bounded bootstrap path."""
from __future__ import annotations

import json
from pathlib import Path
import shutil
import signal

from resource_guard import run_guarded


ROOT = Path(__file__).resolve().parents[2]
BOOTSTRAP = ROOT / "target/bootstrap-c17/vitte-bootstrap"
OUT = ROOT / "target/bootstrap-c17"
REPORT = ROOT / "target/reports/bootstrap_streaming_regressions.json"
RUNTIME = ROOT / "src/vitte/compiler/backends/runtime_c"


def main() -> int:
    commands = []
    def check(command: list[str], cwd: Path = ROOT, timeout_seconds: int = 600,
              expected_exit: int = 0, expected_output: str = "") -> None:
        result = run_guarded(command, cwd=cwd, timeout_seconds=timeout_seconds)
        result["expected_exit_code"] = expected_exit
        commands.append(result)
        if (result["exit_code"] != expected_exit or result["resource_guard"]["reason"]
                or expected_output not in result["output"]
                or "runtime error:" in result["output"]
                or "ERROR: AddressSanitizer" in result["output"]):
            raise RuntimeError(f"{' '.join(command)} (exit {result['exit_code']})\n{result['output']}")

    status = "pass"
    error = ""
    try:
        OUT.mkdir(parents=True, exist_ok=True)
        for name in ("streaming_summary_runtime", "lazy_expressions_runtime", "llvm_pick_codegen_runtime", "mir_loop_targets_runtime", "llvm_integer_width_runtime", "llvm_array_runtime", "llvm_imports_runtime"):
            check([str(BOOTSTRAP), "build", f"../bootstrap/tests/{name}.vit", "-o", str(OUT / name)], ROOT / "src")
            check([str(OUT / name)], timeout_seconds=15)
        clang = shutil.which("clang")
        if not clang:
            raise RuntimeError("clang is required to verify the generated LLVM fixture")
        provider = OUT / "llvm_imports_provider.ll"
        check([clang, "-Wno-override-module", "-c", str(provider), "-o", str(OUT / "llvm_imports_provider.o")])
        for name in ("llvm_imports_caller", "llvm_imports_production", "llvm_imports_conditional"):
            check([clang, "-Wno-override-module", str(OUT / f"{name}.ll"), str(OUT / "llvm_imports_provider.o"), str(RUNTIME / "vitte_runtime.c"), "-o", str(OUT / name)])
            check([str(OUT / name)], timeout_seconds=5)
        if (OUT / "llvm_imports_caller.ll").read_bytes() != (OUT / "llvm_imports_production.ll").read_bytes():
            raise RuntimeError("bootstrap/production imported ABI mismatch")
        streamed = (OUT / "llvm_imports_streamed.ll").read_text()
        if "%Kind = type" in streamed or "%Value = type" in streamed or "declare %Value @create" in streamed:
            raise RuntimeError("streaming emits an imported definition or declaration")
        combined = OUT / "llvm_imports_combined.ll"
        combined.write_text(provider.read_text() + "\n" + streamed[streamed.index("define i64 @main"):])
        check([clang, "-Wno-override-module", str(combined), "-o", str(OUT / "llvm_imports_combined")])
        check([str(OUT / "llvm_imports_combined")], timeout_seconds=5)
        for name in ("llvm_pick_codegen", "llvm_integer_width", "llvm_integer_width_production"):
            check([clang, "-Wno-override-module", "-x", "ir", str(OUT / f"{name}.ll"), "-o", str(OUT / name)])
            check([str(OUT / name)], timeout_seconds=5)
        for name in ("llvm_integer_width", "llvm_array"):
            if (OUT / f"{name}.ll").read_bytes() != (OUT / f"{name}_production.ll").read_bytes():
                raise RuntimeError(f"bootstrap/production LLVM mismatch: {name}")
        for name in ("llvm_array", "llvm_array_production", "llvm_array_oob", "llvm_array_negative"):
            emitted = (OUT / f"{name}.ll").read_text(encoding="utf-8")
            if "; invalid " in emitted or "; unsupported " in emitted:
                raise RuntimeError(f"incomplete LLVM emission: {name}")
            check([clang, "-Wno-override-module", "-x", "ir", str(OUT / f"{name}.ll"),
                   "-x", "c", str(RUNTIME / "vitte_runtime.c"), "-o", str(OUT / name)])
            failing = name in ("llvm_array_oob", "llvm_array_negative")
            check([str(OUT / name)], timeout_seconds=5,
                  expected_exit=-signal.SIGABRT if failing else 0,
                  expected_output="array index out of bounds" if failing else "")
        check([clang, "-O2", "-Wno-override-module", "-x", "ir", str(OUT / "llvm_array.ll"),
               "-x", "c", str(RUNTIME / "vitte_runtime.c"), "-o", str(OUT / "llvm_array_optimized")])
        check([str(OUT / "llvm_array_optimized")], timeout_seconds=5)
        check([clang, "-O1", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
               "-Wno-override-module", "-x", "ir", str(OUT / "llvm_array.ll"),
               "-x", "c", str(RUNTIME / "vitte_runtime.c"), "-o", str(OUT / "llvm_array_sanitized")])
        check([str(OUT / "llvm_array_sanitized")], timeout_seconds=5)
        storage = OUT / "llvm_array_storage_runtime"
        check([clang, "-std=c17", "-I", str(RUNTIME),
               str(ROOT / "bootstrap/tests/llvm_array_storage_runtime.c"),
               str(RUNTIME / "vitte_runtime.c"), "-o", str(storage)])
        check([str(storage)], timeout_seconds=5)
        for mode in ("overflow-count", "overflow-header", "zero-size", "null-data", "empty"):
            check([str(storage), mode], timeout_seconds=5, expected_exit=-signal.SIGABRT,
                  expected_output="array index out of bounds" if mode in ("null-data", "empty") else "array allocation overflow")
        check([clang, "-std=c17", "-O1", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
               "-I", str(RUNTIME), str(ROOT / "bootstrap/tests/llvm_array_storage_runtime.c"),
               str(RUNTIME / "vitte_runtime.c"), "-o", str(OUT / "llvm_array_storage_sanitized")])
        check([str(OUT / "llvm_array_storage_sanitized")], timeout_seconds=5)
    except RuntimeError as exc:
        status, error = "fail", str(exc)
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps({"status": status, "error": error, "commands": commands}, indent=2) + "\n", encoding="utf-8")
    print(f"[bootstrap-streaming-regressions] {status}: {REPORT.relative_to(ROOT)}")
    if error:
        print(error)
    return 0 if status == "pass" else 1


if __name__ == "__main__":
    raise SystemExit(main())
