#!/usr/bin/env python3
"""Validate the top-level Vitte compiler contract manifest."""

from __future__ import annotations

import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "src" / "vitte" / "compiler" / "tests" / "compiler_contract_manifest.json"
MAKEFILE = ROOT / "Makefile"
REPORT_DIR = ROOT / "target" / "reports" / "compiler_contracts"
REPORT_JSON = REPORT_DIR / "coverage.json"
REPORT_MD = REPORT_DIR / "coverage.md"
REMAINING_MD = REPORT_DIR / "remaining.md"

REQUIRED_DOMAINS = {
    "bootstrap",
    "lexer",
    "parser",
    "ast",
    "hir",
    "sema",
    "typeck",
    "borrowck",
    "mir",
    "ir",
    "backend",
    "diagnostics",
    "suggestions",
    "counterfactual",
    "lsp",
    "cli",
    "ci",
    "reports",
}

MIN_CONTRACTS = 20
REQUIRED_CONTRACT_IDS = {
    "bootstrap.source_of_truth",
    "bootstrap.stage_chain",
    "bootstrap.selfhost_repro",
    "bootstrap.same_input_hash",
    "bootstrap.generated_declared",
    "bootstrap.no_orphan_modules",
    "bootstrap.no_legacy_imports",
    "bootstrap.no_path_typos",
    "bootstrap.entrypoints",
    "bootstrap.cli_options_documented",
    "lexer.token_fixtures",
    "lexer.errors_cataloged",
    "lexer.text_matrix",
    "lexer.long_lines",
    "lexer.empty_file",
    "lexer.token_order",
    "lexer.error_recovery",
    "lexer.span_policy",
    "lexer.no_panic_invalid",
    "lexer.snapshots_json_text",
    "parser.grammar_fixtures",
    "parser.recovery_fixtures",
    "parser.ast_nodes_tested",
    "parser.root_before_cascade",
    "parser.diagnostic_order",
    "parser.ast_snapshots",
    "parser.diagnostic_snapshots",
    "parser.verified_suggestions",
    "parser.no_silent_stray_tokens",
    "parser.no_catch_all_fallback",
    "ast.real_spans",
    "ast.no_synthetic_source_spans",
    "ast.child_order",
    "ast.attributes",
    "ast.imports_modules",
    "ast.expressions",
    "ast.statements",
    "ast.patterns",
    "ast.types",
    "ast.json_snapshots",
}
REQUIRED_GATE_NEEDLES = (
    "go make compiler-contracts",
    "go make hir-coverage",
    "go make sema-gate",
    "go make borrowck-gate",
    "go make mir-gate",
    "go make ir-gate",
)


def make_targets() -> set[str]:
    targets: set[str] = set()
    for line in MAKEFILE.read_text(encoding="utf-8").splitlines():
        if not line or line.startswith("\t") or line.startswith("."):
            continue
        if ":" not in line:
            continue
        lhs = line.split(":", 1)[0].strip()
        if lhs and " " not in lhs and "=" not in lhs:
            targets.add(lhs)
    return targets


def evidence_exists(evidence: str, targets: set[str]) -> tuple[bool, str]:
    if ":" not in evidence:
        return False, "evidence must use kind:path"
    kind, value = evidence.split(":", 1)
    if kind == "make":
        return value in targets, f"make target {value}"
    if kind in {"tool", "source", "test", "manifest", "report"}:
        return (ROOT / value).exists(), f"{kind} {value}"
    return False, f"unknown evidence kind {kind}"


def main() -> int:
    data = json.loads(MANIFEST.read_text(encoding="utf-8"))
    contracts = data.get("contracts", [])
    targets = make_targets()
    failures: list[str] = []
    rows: list[dict[str, object]] = []
    seen: set[str] = set()
    domains: set[str] = set()

    if not isinstance(contracts, list) or not contracts:
        failures.append("manifest has no contracts")
        contracts = []
    if len(contracts) < MIN_CONTRACTS:
        failures.append(f"manifest has {len(contracts)} contracts; expected at least {MIN_CONTRACTS}")

    gate_text = (ROOT / "tools" / "compiler_max_gate.sh").read_text(encoding="utf-8")
    for needle in REQUIRED_GATE_NEEDLES:
        if needle not in gate_text:
            failures.append(f"compiler max gate missing {needle}")

    for entry in contracts:
        cid = str(entry.get("id", ""))
        domain = str(entry.get("domain", ""))
        title = str(entry.get("title", ""))
        evidence = entry.get("evidence", [])
        if cid in seen:
            failures.append(f"duplicate contract id {cid}")
        seen.add(cid)
        if cid and (cid.lower() != cid or " " in cid or "." not in cid):
            failures.append(f"contract id {cid} must be stable lowercase dotted form")
        if not cid or not domain or not title:
            failures.append(f"contract {cid or '<missing>'} requires id, domain and title")
        domains.add(domain)
        if not isinstance(evidence, list) or not evidence:
            failures.append(f"{cid} has no evidence")
            evidence = []
        checked: list[dict[str, object]] = []
        passed = 0
        for item in evidence:
            ok, label = evidence_exists(str(item), targets)
            checked.append({"evidence": str(item), "ok": ok, "label": label})
            if ok:
                passed += 1
            else:
                failures.append(f"{cid} missing {label}")
        rows.append(
            {
                "id": cid,
                "domain": domain,
                "title": title,
                "evidence_total": len(evidence),
                "evidence_ok": passed,
                "status": "pass" if passed == len(evidence) and evidence else "fail",
                "evidence": checked,
            }
        )

    missing_domains = sorted(REQUIRED_DOMAINS - domains)
    for domain in missing_domains:
        failures.append(f"missing required contract domain {domain}")
    missing_contract_ids = sorted(REQUIRED_CONTRACT_IDS - seen)
    for cid in missing_contract_ids:
        failures.append(f"missing required contract id {cid}")

    covered = sum(1 for row in rows if row["status"] == "pass")
    total = len(rows)
    percent = int((covered * 100) / total) if total else 0
    status = "pass" if not failures else "fail"
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    REPORT_JSON.write_text(
        json.dumps(
            {
                "status": status,
                "covered": covered,
                "total": total,
                "percent": percent,
                "required_domains": sorted(REQUIRED_DOMAINS),
                "missing_domains": missing_domains,
                "failures": failures,
                "contracts": rows,
            },
            indent=2,
            sort_keys=True,
        )
        + "\n",
        encoding="utf-8",
    )
    REPORT_MD.write_text(
        "# Compiler Contract Coverage\n\n"
        + f"Status: {status}\n\n"
        + f"Covered: {covered}/{total} ({percent}%)\n\n"
        + "\n".join(f"- {row['status']}: {row['id']} ({row['domain']}) - {row['title']}" for row in rows)
        + "\n",
        encoding="utf-8",
    )
    REMAINING_MD.write_text(
        "# Remaining Compiler Contract Work\n\n"
        + ("No missing compiler contracts.\n" if not failures else "\n".join(f"- {failure}" for failure in failures) + "\n"),
        encoding="utf-8",
    )

    if failures:
        for failure in failures:
            print(f"[compiler-contracts][error] {failure}", file=sys.stderr)
        print(f"[compiler-contracts] coverage={covered}/{total} ({percent}%) report={REPORT_JSON.relative_to(ROOT)}", file=sys.stderr)
        return 1

    print(f"[compiler-contracts] OK coverage={covered}/{total} ({percent}%) report={REPORT_JSON.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
