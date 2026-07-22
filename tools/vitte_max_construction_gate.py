#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BIN = ROOT / "bin" / "vitte"
REPORT_JSON = ROOT / "target" / "reports" / "vitte_max_construction_gate.json"
REPORT_MD = ROOT / "target" / "reports" / "vitte_max_construction_gate.md"
WORK = ROOT / "target" / "vitte_max_construction_gate"
SMOKE = ROOT / "tests" / "strict_ok.vit"
SMOKE_BIN = WORK / "strict_ok"


@dataclass(frozen=True)
class CommandSpec:
    name: str
    args: tuple[str, ...]
    expect: tuple[str, ...] = ()
    json_schema: str = ""


def write_smoke() -> None:
    WORK.mkdir(parents=True, exist_ok=True)


def run(args: list[str], *, clean_env: bool = False, timeout: int = 30) -> dict[str, object]:
    env = None
    if clean_env:
        env = {
            "HOME": str(WORK / "home"),
            "PATH": "/usr/bin:/bin",
            "TMPDIR": str(WORK / "tmp"),
        }
        (WORK / "home").mkdir(parents=True, exist_ok=True)
        (WORK / "tmp").mkdir(parents=True, exist_ok=True)
    completed = subprocess.run(
        args,
        cwd=ROOT,
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=timeout,
    )
    return {"command": args, "exit_code": completed.returncode, "output": completed.stdout[-8000:]}


def check_command(spec: CommandSpec, failures: list[str]) -> dict[str, object]:
    clean = spec.name.startswith(("package.", "workspace.")) or spec.name == "publish.dry_run"
    row = run([str(BIN), *spec.args], clean_env=clean)
    output = str(row["output"])
    ok = row["exit_code"] == 0
    missing = [term for term in spec.expect if term not in output]
    parsed_schema = ""
    if spec.json_schema and row["exit_code"] == 0:
        try:
            parsed = json.loads(output)
            parsed_schema = str(parsed.get("schema", ""))
        except json.JSONDecodeError as exc:
            ok = False
            failures.append(f"{spec.name} did not emit JSON: {exc}")
        else:
            if parsed_schema != spec.json_schema:
                ok = False
                failures.append(f"{spec.name} schema mismatch: expected {spec.json_schema}, got {parsed_schema}")
    if row["exit_code"] != 0:
        ok = False
        failures.append(f"{spec.name} failed with exit {row['exit_code']}")
    for term in missing:
        ok = False
        failures.append(f"{spec.name} output missing `{term}`")
    return {
        "name": spec.name,
        "ok": ok,
        "args": list(spec.args),
        "missing_terms": missing,
        "schema": parsed_schema,
        "exit_code": row["exit_code"],
    }


def check_help(failures: list[str]) -> dict[str, object]:
    row = run([str(BIN), "--help"])
    required = [
        "check FILE",
        "build FILE -o OUT",
        "run FILE",
        "parse FILE",
        "explain CODE",
        "fix FILE",
        "package graph explain",
        "workspace build --package P",
        "workspace test --all",
        "publish --dry-run",
        "lsp --stdio",
        "dump-tokens FILE",
        "dump-ast FILE",
        "dump-hir FILE",
        "dump-mir FILE",
        "dump-native-ir --src FILE",
        "selfhost-source",
        "--error-format text|json",
        "--diagnostic-width N",
        "--color auto|always|never",
    ]
    output = str(row["output"])
    missing = [term for term in required if term not in output]
    for term in missing:
        failures.append(f"help missing `{term}`")
    return {"ok": row["exit_code"] == 0 and not missing, "missing_terms": missing}


def check_build_run(failures: list[str]) -> dict[str, object]:
    rows = [
        check_command(CommandSpec("core.check", ("check", str(SMOKE)), ("check succeeded",)), failures),
        check_command(CommandSpec("core.build", ("build", str(SMOKE), "-o", str(SMOKE_BIN)), ("build succeeded",)), failures),
        check_command(CommandSpec("core.run", ("run", str(SMOKE))), failures),
    ]
    executable = SMOKE_BIN.exists() and os.access(SMOKE_BIN, os.X_OK)
    if not executable:
        failures.append(f"build did not produce executable {SMOKE_BIN.relative_to(ROOT)}")
    return {"ok": all(row["ok"] for row in rows) and executable, "commands": rows, "executable": executable}


def check_external_gate(name: str, args: list[str], failures: list[str]) -> dict[str, object]:
    row = run(args, timeout=60)
    ok = row["exit_code"] == 0
    if not ok:
        failures.append(f"{name} failed with exit {row['exit_code']}")
    return {"name": name, "ok": ok, "exit_code": row["exit_code"], "output_tail": row["output"]}


def check_required_targets(failures: list[str]) -> dict[str, object]:
    makefile = (ROOT / "Makefile").read_text(encoding="utf-8")
    targets = [
        "core-language-gate",
        "compiler-max-gate",
        "compiler-max-gate-strict",
        "type-system-advanced-gate",
        "ownership-borrow-lifetimes-max",
        "backend-gate",
        "package-manager-gate",
        "post-install-package-cli-test",
        "lsp-gate",
        "formatter-gate",
        "release-gate-90-119",
        "real-release-gate",
        "release-check",
    ]
    rows = []
    for target in targets:
        present = f".PHONY: {target}" in makefile or f"\n{target}:" in makefile
        rows.append({"target": target, "present": present})
        if not present:
            failures.append(f"Makefile missing target {target}")
    return {"ok": all(row["present"] for row in rows), "targets": rows}


def check_reports(failures: list[str]) -> dict[str, object]:
    reports = [
        "target/reports/compiler_contracts/coverage.json",
        "target/reports/lexer_parser_coverage_100.json",
        "target/reports/syntax_parser_diagnostics_max.json",
        "target/reports/type_system_advanced_all.json",
        "target/reports/ownership_borrow_lifetimes_max.json",
        "target/reports/package_lsp_format_96_119.json",
        "target/reports/real_release_gate.json",
    ]
    rows = []
    for rel in reports:
        path = ROOT / rel
        present = path.exists() and path.stat().st_size > 0
        rows.append({"report": rel, "present": present})
        if not present:
            failures.append(f"missing report {rel}")
    return {"ok": all(row["present"] for row in rows), "reports": rows}


def write_reports(report: dict[str, object]) -> None:
    REPORT_JSON.parent.mkdir(parents=True, exist_ok=True)
    REPORT_JSON.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    lines = [
        "# Vitte Max Construction Gate",
        "",
        f"Status: {report['status']}",
        f"Requirements: {report['requirements_passed']}/{report['requirements_total']}",
        "",
        "This gate keeps Vitte syntax as the source surface and verifies maximum CLI/build construction coverage through real commands.",
        "",
        "## Failures",
        "",
    ]
    failures = report["failures"]
    if isinstance(failures, list) and failures:
        lines.extend(f"- {failure}" for failure in failures)
    else:
        lines.append("- none")
    REPORT_MD.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")


def main() -> int:
    write_smoke()
    failures: list[str] = []
    command_specs = [
        CommandSpec("core.parse", ("parse", str(SMOKE)), ("parse ok",)),
        CommandSpec("core.explain", ("explain", "E_CLI_MISSING_ARG"), ("error code: E_CLI_MISSING_ARG", "fix:")),
        CommandSpec("core.fix", ("fix", "tests/diagnostics/frontend/parser/missing-brace.vit"), ("fix dry-run", "refused_if_behavior_changed: true")),
        CommandSpec("debug.dump_tokens", ("dump-tokens", str(SMOKE)), ("proc", "main")),
        CommandSpec("debug.dump_ast", ("dump-ast", str(SMOKE)), ("AST", "node_count")),
        CommandSpec("debug.dump_hir", ("dump-hir", str(SMOKE)), ("HIR", "node_count")),
        CommandSpec("debug.dump_mir", ("dump-mir", str(SMOKE)), ("MIR", "block_count")),
        CommandSpec("debug.dump_native_ir", ("dump-native-ir", "--src", str(SMOKE)), ("native_ir_v1", "pipeline.check=ok")),
        CommandSpec("debug.selfhost_source", ("selfhost-source",), ("compiler_source_root=src/vitte/compiler", "compiler_entry_point=src/vitte/compiler/main.vit")),
        CommandSpec("package.graph_explain", ("package", "graph", "explain"), json_schema="vitte.package.graph.explain"),
        CommandSpec("workspace.build_selective", ("workspace", "build", "--package", "packages/app"), json_schema="vitte.workspace.build"),
        CommandSpec("workspace.test_all", ("workspace", "test", "--all"), json_schema="vitte.workspace.test"),
        CommandSpec("publish.dry_run", ("publish", "--dry-run"), json_schema="vitte.publish.dry_run"),
    ]

    sections: list[dict[str, object]] = []
    sections.append({"name": "help", **check_help(failures)})
    sections.append({"name": "build_run", **check_build_run(failures)})
    sections.append({"name": "cli_commands", "commands": [check_command(spec, failures) for spec in command_specs]})
    sections.append({"name": "lsp_jsonrpc", **check_external_gate("lsp_jsonrpc", ["python3", "tools/lsp/jsonrpc_client_test.py"], failures)})
    sections.append({"name": "formatter", **check_external_gate("formatter", ["python3", "tools/formatter/run_checks.py"], failures)})
    sections.append({"name": "package_cli_post_install", **check_external_gate("package_cli_post_install", ["sh", "tools/post_install_package_cli_test.sh"], failures)})
    sections.append({"name": "real_release_pending_report", **check_external_gate("real_release_pending_report", ["python3", "tools/real_release_gate.py", "--allow-pending"], failures)})
    sections.append({"name": "make_targets", **check_required_targets(failures)})
    sections.append({"name": "reports", **check_reports(failures)})

    command_rows = [row for section in sections for row in section.get("commands", [])]  # type: ignore[union-attr]
    requirements_total = 1 + 1 + len(command_rows) + 4 + 13 + 7
    requirements_passed = (
        int(bool(sections[0].get("ok")))
        + int(bool(sections[1].get("ok")))
        + sum(1 for row in command_rows if row.get("ok"))
        + sum(1 for section in sections[3:7] if section.get("ok"))
        + sum(1 for row in sections[7]["targets"] if row.get("present"))  # type: ignore[index]
        + sum(1 for row in sections[8]["reports"] if row.get("present"))  # type: ignore[index]
    )
    report = {
        "schema": "org.vitte.max-construction-gate.v1",
        "version": "0.1.0",
        "status": "PASS" if not failures else "FAIL",
        "requirements_total": requirements_total,
        "requirements_passed": requirements_passed,
        "syntax_policy": "Vitte source syntax remains canonical",
        "sections": sections,
        "failures": failures,
    }
    write_reports(report)
    if failures:
        for failure in failures:
            print(f"[vitte-max-construction-gate][error] {failure}", file=sys.stderr)
        print(f"[vitte-max-construction-gate] report={REPORT_JSON.relative_to(ROOT)}", file=sys.stderr)
        return 1
    print(f"[vitte-max-construction-gate] OK requirements={requirements_passed}/{requirements_total} report={REPORT_JSON.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
