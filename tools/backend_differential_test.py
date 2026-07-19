#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
CORPUS = ROOT / "tests" / "backend_differential" / "corpus.json"
REPORT = ROOT / "target" / "reports" / "backend_differential.json"
OPT_LEVELS = ("O0", "O2")
DRIVER_CANDIDATES = (ROOT / "bin" / "vitte", ROOT / "bin" / "vittec")
ARITHMETIC_OPS = {"add", "sub", "mul", "sdiv"}
COMPARISON_OPS = {"eq", "lt"}
ALL_OPS = {"const", "param", "call", "select"} | ARITHMETIC_OPS | COMPARISON_OPS


def run(command: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        command,
        cwd=ROOT,
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        timeout=20,
    )


def find_compiler_driver() -> str | None:
    for candidate in DRIVER_CANDIDATES:
        resolved = shutil.which(str(candidate))
        if resolved is not None:
            return resolved
    return None


def validate_expr(expr: Any, params: set[str], functions: dict[str, int]) -> list[str]:
    if not isinstance(expr, dict):
        return ["expression is not an object"]
    op = expr.get("op")
    if op not in ALL_OPS:
        return [f"unsupported expression op: {op!r}"]
    if op == "const":
        return [] if isinstance(expr.get("value"), int) else ["const value is not an integer"]
    if op == "param":
        name = expr.get("name")
        return [] if isinstance(name, str) and name in params else [f"unknown parameter: {name!r}"]
    if op in ARITHMETIC_OPS | COMPARISON_OPS:
        return validate_expr(expr.get("left"), params, functions) + validate_expr(expr.get("right"), params, functions)
    if op == "select":
        return (
            validate_expr(expr.get("condition"), params, functions)
            + validate_expr(expr.get("then"), params, functions)
            + validate_expr(expr.get("else"), params, functions)
        )
    name = expr.get("function")
    args = expr.get("args")
    failures: list[str] = []
    if not isinstance(name, str) or name not in functions:
        failures.append(f"unknown function: {name!r}")
    if not isinstance(args, list):
        return failures + ["call args are not an array"]
    if isinstance(name, str) and name in functions and len(args) != functions[name]:
        failures.append(f"call arity mismatch for {name}")
    for arg in args:
        failures.extend(validate_expr(arg, params, functions))
    return failures


def validate_case(case: Any) -> list[str]:
    if not isinstance(case, dict):
        return ["case is not an object"]
    failures: list[str] = []
    name = case.get("name")
    if not isinstance(name, str) or not name:
        failures.append("case name is missing")
    if not isinstance(case.get("vitte_source"), str) or not case["vitte_source"]:
        failures.append(f"{name}: Vitte source is missing")
    expected = case.get("expected")
    if not isinstance(expected, dict) or not isinstance(expected.get("exit_code"), int):
        failures.append(f"{name}: expected result is invalid")
    program = case.get("program")
    functions_raw = program.get("functions") if isinstance(program, dict) else None
    if not isinstance(functions_raw, list) or not functions_raw:
        return failures + [f"{name}: program functions are missing"]
    functions: dict[str, int] = {}
    for function in functions_raw:
        if not isinstance(function, dict) or not isinstance(function.get("name"), str) or not isinstance(function.get("params"), list):
            failures.append(f"{name}: invalid function declaration")
            continue
        fn_name = function["name"]
        if fn_name in functions:
            failures.append(f"{name}: duplicate function {fn_name}")
        functions[fn_name] = len(function["params"])
    if functions.get("main") != 0:
        failures.append(f"{name}: main() is required")
    for function in functions_raw:
        if not isinstance(function, dict) or not isinstance(function.get("params"), list):
            continue
        params = function["params"]
        if not all(isinstance(param, str) and param for param in params) or len(set(params)) != len(params):
            failures.append(f"{name}: invalid parameters for {function.get('name')}")
            continue
        failures.extend(f"{name}/{function.get('name')}: {failure}" for failure in validate_expr(function.get("body"), set(params), functions))
    return failures


def emit_c_expr(expr: dict[str, Any]) -> str:
    op = expr["op"]
    if op == "const":
        return str(expr["value"])
    if op == "param":
        return expr["name"]
    if op in ARITHMETIC_OPS | COMPARISON_OPS:
        operator = {"add": "+", "sub": "-", "mul": "*", "sdiv": "/", "eq": "==", "lt": "<"}[op]
        return f"({emit_c_expr(expr['left'])} {operator} {emit_c_expr(expr['right'])})"
    if op == "call":
        return f"{expr['function']}({', '.join(emit_c_expr(arg) for arg in expr['args'])})"
    return f"({emit_c_expr(expr['condition'])} ? {emit_c_expr(expr['then'])} : {emit_c_expr(expr['else'])})"


def emit_c(case: dict[str, Any]) -> str:
    functions = case["program"]["functions"]
    lines = ["#include <stdint.h>", ""]
    for function in functions:
        if function["name"] != "main":
            params = ", ".join(f"int64_t {param}" for param in function["params"])
            lines.append(f"static int64_t {function['name']}({params});")
    if len(lines) > 2:
        lines.append("")
    for function in functions:
        body = emit_c_expr(function["body"])
        if function["name"] == "main":
            lines.extend(["int main(void) {", f"  return (int)({body});", "}", ""])
        else:
            params = ", ".join(f"int64_t {param}" for param in function["params"])
            lines.extend([f"static int64_t {function['name']}({params}) {{", f"  return {body};", "}", ""])
    return "\n".join(lines)


@dataclass
class LlvmFunctionEmitter:
    lines: list[str] = field(default_factory=list)
    next_value: int = 0

    def value(self) -> str:
        self.next_value += 1
        return f"%v{self.next_value}"

    def to_i64(self, ty: str, value: str) -> str:
        if ty == "i64":
            return value
        result = self.value()
        self.lines.append(f"  {result} = zext i1 {value} to i64")
        return result

    def to_i1(self, ty: str, value: str) -> str:
        if ty == "i1":
            return value
        result = self.value()
        self.lines.append(f"  {result} = icmp ne i64 {value}, 0")
        return result

    def emit_expr(self, expr: dict[str, Any]) -> tuple[str, str]:
        op = expr["op"]
        if op == "const":
            return "i64", str(expr["value"])
        if op == "param":
            return "i64", f"%{expr['name']}"
        if op in ARITHMETIC_OPS:
            left_ty, left = self.emit_expr(expr["left"])
            right_ty, right = self.emit_expr(expr["right"])
            left = self.to_i64(left_ty, left)
            right = self.to_i64(right_ty, right)
            result = self.value()
            self.lines.append(f"  {result} = {op} i64 {left}, {right}")
            return "i64", result
        if op in COMPARISON_OPS:
            left_ty, left = self.emit_expr(expr["left"])
            right_ty, right = self.emit_expr(expr["right"])
            left = self.to_i64(left_ty, left)
            right = self.to_i64(right_ty, right)
            result = self.value()
            predicate = "eq" if op == "eq" else "slt"
            self.lines.append(f"  {result} = icmp {predicate} i64 {left}, {right}")
            return "i1", result
        if op == "call":
            args: list[str] = []
            for arg in expr["args"]:
                arg_ty, arg_value = self.emit_expr(arg)
                args.append("i64 " + self.to_i64(arg_ty, arg_value))
            result = self.value()
            self.lines.append(f"  {result} = call i64 @{expr['function']}({', '.join(args)})")
            return "i64", result
        condition_ty, condition = self.emit_expr(expr["condition"])
        then_ty, then_value = self.emit_expr(expr["then"])
        else_ty, else_value = self.emit_expr(expr["else"])
        condition = self.to_i1(condition_ty, condition)
        then_value = self.to_i64(then_ty, then_value)
        else_value = self.to_i64(else_ty, else_value)
        result = self.value()
        self.lines.append(f"  {result} = select i1 {condition}, i64 {then_value}, i64 {else_value}")
        return "i64", result


def emit_llvm(case: dict[str, Any]) -> str:
    lines = [f"; differential corpus: {case['name']}", 'source_filename = "vitte-backend-differential"', ""]
    for function in case["program"]["functions"]:
        emitter = LlvmFunctionEmitter()
        params = ", ".join(f"i64 %{param}" for param in function["params"])
        return_type = "i32" if function["name"] == "main" else "i64"
        lines.append(f"define {return_type} @{function['name']}({params}) {{")
        lines.append("entry:")
        value_type, value = emitter.emit_expr(function["body"])
        if function["name"] == "main":
            lines.extend(emitter.lines)
            if value_type == "i1":
                converted = emitter.value()
                lines.append(f"  {converted} = zext i1 {value} to i32")
            elif value.startswith("%"):
                converted = emitter.value()
                lines.append(f"  {converted} = trunc i64 {value} to i32")
            else:
                converted = value
            lines.append(f"  ret i32 {converted}")
        else:
            converted = emitter.to_i64(value_type, value)
            lines.extend(emitter.lines)
            lines.append(f"  ret i64 {converted}")
        lines.extend(["}", ""])
    return "\n".join(lines)


def write_report(
    status: str,
    checks: dict[str, Any],
    source_checks: list[dict[str, Any]],
    results: list[dict[str, Any]],
    failures: list[str],
    corpus_sha256: str,
) -> None:
    payload = {
        "schema": "vitte.compiler.backend_differential",
        "schema_version": "1.0.0",
        "status": status,
        "execution_model": "reference-artifacts-from-structured-shared-corpus",
        "production_adapter_contract_test": "test_c_and_llvm_backends_consume_shared_ir_corpus",
        "corpus": str(CORPUS.relative_to(ROOT)),
        "corpus_sha256": corpus_sha256,
        "optimization_levels": list(OPT_LEVELS),
        "checks": checks,
        "source_checks": source_checks,
        "results": results,
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, ensure_ascii=True, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def main() -> int:
    cc = shutil.which("cc")
    clang = shutil.which("clang")
    compiler_driver = find_compiler_driver()
    corpus_bytes = CORPUS.read_bytes() if CORPUS.is_file() else b""
    corpus_sha256 = hashlib.sha256(corpus_bytes).hexdigest()
    checks: dict[str, Any] = {
        "c_compiler_available": cc is not None,
        "clang_available": clang is not None,
        "compiler_driver_available": compiler_driver is not None,
        "corpus_valid": False,
        "case_count": 0,
        "all_vitte_sources_checked": False,
        "all_c_artifacts_compiled": False,
        "all_llvm_artifacts_compiled": False,
        "all_backend_outputs_match": False,
        "all_results_match_expected": False,
    }
    failures: list[str] = []
    source_checks: list[dict[str, Any]] = []
    results: list[dict[str, Any]] = []
    if cc is None:
        failures.append("C compiler is unavailable")
    if clang is None:
        failures.append("clang is unavailable")
    if compiler_driver is None:
        failures.append("Vitte compiler driver is unavailable")
    try:
        payload = json.loads(corpus_bytes)
    except (json.JSONDecodeError, UnicodeDecodeError) as error:
        payload = {}
        failures.append(f"invalid corpus JSON: {error}")
    cases = payload.get("cases") if isinstance(payload, dict) else None
    if not isinstance(cases, list):
        cases = []
        failures.append("corpus cases are missing")
    case_failures: list[str] = []
    names: set[str] = set()
    for case in cases:
        case_failures.extend(validate_case(case))
        if isinstance(case, dict) and isinstance(case.get("name"), str):
            if case["name"] in names:
                case_failures.append(f"duplicate case name: {case['name']}")
            names.add(case["name"])
    failures.extend(case_failures)
    checks["corpus_valid"] = bool(cases) and not case_failures
    checks["case_count"] = len(cases)

    if not failures and cc is not None and clang is not None and compiler_driver is not None:
        with tempfile.TemporaryDirectory(prefix="vitte-backend-differential-") as raw_tmp:
            tmp = Path(raw_tmp)
            for case in cases:
                case_root = tmp / case["name"]
                case_root.mkdir(parents=True)
                vitte_source = case_root / "program.vit"
                vitte_source.write_text(case["vitte_source"], encoding="utf-8")
                source_check = run([compiler_driver, "check", str(vitte_source)])
                source_checks.append({
                    "case": case["name"],
                    "checked": source_check.returncode == 0,
                })
                if source_check.returncode != 0:
                    failures.append(f"{case['name']}: Vitte source check failed: {source_check.stderr.strip()}")
                for opt_level in OPT_LEVELS:
                    case_dir = case_root / opt_level
                    case_dir.mkdir(parents=True)
                    c_source = case_dir / "program.c"
                    llvm_source = case_dir / "program.ll"
                    c_binary = case_dir / "program-c"
                    llvm_binary = case_dir / "program-llvm"
                    c_source.write_text(emit_c(case), encoding="utf-8")
                    llvm_source.write_text(emit_llvm(case), encoding="utf-8")
                    c_build = run([cc, "-std=c11", f"-{opt_level}", "-Wall", "-Wextra", "-Werror", str(c_source), "-o", str(c_binary)])
                    llvm_build = run([clang, "-Wno-override-module", f"-{opt_level}", str(llvm_source), "-o", str(llvm_binary)])
                    c_run = run([str(c_binary)]) if c_build.returncode == 0 else None
                    llvm_run = run([str(llvm_binary)]) if llvm_build.returncode == 0 else None
                    expected = case["expected"]
                    outputs_match = (
                        c_run is not None
                        and llvm_run is not None
                        and c_run.returncode == llvm_run.returncode
                        and c_run.stdout == llvm_run.stdout
                        and c_run.stderr == llvm_run.stderr
                    )
                    expected_match = (
                        c_run is not None
                        and llvm_run is not None
                        and c_run.returncode == expected["exit_code"]
                        and llvm_run.returncode == expected["exit_code"]
                        and c_run.stdout == expected.get("stdout", "")
                        and llvm_run.stdout == expected.get("stdout", "")
                        and c_run.stderr == expected.get("stderr", "")
                        and llvm_run.stderr == expected.get("stderr", "")
                    )
                    result = {
                        "case": case["name"],
                        "optimization": opt_level,
                        "c_compiled": c_build.returncode == 0,
                        "llvm_compiled": llvm_build.returncode == 0,
                        "c_exit_code": c_run.returncode if c_run is not None else None,
                        "llvm_exit_code": llvm_run.returncode if llvm_run is not None else None,
                        "outputs_match": outputs_match,
                        "expected_match": expected_match,
                    }
                    results.append(result)
                    if c_build.returncode != 0:
                        failures.append(f"{case['name']}/{opt_level}: C compiler rejected emitted source: {c_build.stderr.strip()}")
                    if llvm_build.returncode != 0:
                        failures.append(f"{case['name']}/{opt_level}: LLVM toolchain rejected emitted IR: {llvm_build.stderr.strip()}")
                    if not outputs_match:
                        failures.append(f"{case['name']}/{opt_level}: backend outputs differ")
                    if not expected_match:
                        failures.append(f"{case['name']}/{opt_level}: result differs from corpus expectation")

    checks["all_vitte_sources_checked"] = bool(source_checks) and all(result["checked"] for result in source_checks)
    checks["all_c_artifacts_compiled"] = bool(results) and all(result["c_compiled"] for result in results)
    checks["all_llvm_artifacts_compiled"] = bool(results) and all(result["llvm_compiled"] for result in results)
    checks["all_backend_outputs_match"] = bool(results) and all(result["outputs_match"] for result in results)
    checks["all_results_match_expected"] = bool(results) and all(result["expected_match"] for result in results)
    status = "pass" if not failures and all(value for key, value in checks.items() if key != "case_count") else "fail"
    write_report(status, checks, source_checks, results, failures, corpus_sha256)
    print(f"[backend-differential] status={status} cases={len(cases)} runs={len(results)} report={REPORT.relative_to(ROOT)}")
    for name, value in checks.items():
        print(f"[backend-differential][check] {'pass' if bool(value) else 'fail'} {name}={value}")
    for failure in failures:
        print(f"[backend-differential][error] {failure}", file=sys.stderr)
    return 0 if status == "pass" else 1


if __name__ == "__main__":
    raise SystemExit(main())
