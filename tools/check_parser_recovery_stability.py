#!/usr/bin/env python3
from __future__ import annotations

import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PARSER = ROOT / "src/vitte/compiler/frontend/parse/parser.vit"
RECOVERY = ROOT / "src/vitte/compiler/frontend/parse/recovery.vit"
LEGACY_PARSER_DIR = ROOT / "src/vitte/compiler/frontend/parser"
PARSER_TESTS = ROOT / "src/vitte/compiler/tests/parser_tests.vit"
STRICT_SMOKE = ROOT / "tools/strict_recovery_smoke.sh"
REPORT_DIR = ROOT / "target/reports"

EXPECTED_RECOVERY_KINDS = [
    "None",
    "Statement",
    "Block",
    "Declaration",
    "Expression",
    "Type",
    "Pattern",
    "Import",
    "Toplevel",
]

EXPECTED_SYNC_POINTS = [";", "}", ")", "]", ",", "space", "use", "proc", "const", "type", "form", "import"]

EXPECTED_RECOVER_INDEXES = [
    "recover_stmt_index",
    "recover_block_index",
    "recover_decl_index",
    "recover_expr_index",
    "recover_type_index",
    "recover_pattern_index",
    "recover_import_index",
    "recover_toplevel_index",
]

EXPECTED_PUBLIC_CODES = {
    "P0001": "PAR0021",
    "P000_UNBALANCED": "PAR0022",
    "PATTR003": "PAR0039",
    "PLOOP": "PAR0040",
    "PPRIMARY999": "PAR0041",
    "PSTMT007": "PAR0042",
}

EXPECTED_TESTS = [
    "test_recovery_syncs_statement_boundary",
    "test_recovery_syncs_toplevel_boundary",
    "test_frontend_marks_recovery_usage",
    "test_parser_reports_missing_parenthesis",
    "test_parser_reports_missing_bracket",
    "test_parser_reports_missing_brace",
    "test_parser_reports_missing_semicolon",
    "test_parser_reports_missing_comma",
]


def extract_pick_members(text: str, name: str) -> list[str]:
    match = re.search(rf"pick\s+{re.escape(name)}\s*\{{(?P<body>.*?)\}}", text, re.S)
    if not match:
        raise SystemExit(f"[parser-recovery-stability][error] missing pick {name}")
    members: list[str] = []
    for raw in match.group("body").splitlines():
        item = raw.strip().rstrip(",")
        if re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", item):
            members.append(item)
    return members


def function_body(text: str, signature: str) -> str:
    start = text.find(signature)
    if start < 0:
        raise SystemExit(f"[parser-recovery-stability][error] missing {signature}")
    brace = text.find("{", start)
    if brace < 0:
        raise SystemExit(f"[parser-recovery-stability][error] malformed {signature}")
    depth = 0
    in_string = False
    escaped = False
    for index in range(brace, len(text)):
        ch = text[index]
        if in_string:
            if escaped:
                escaped = False
            elif ch == "\\":
                escaped = True
            elif ch == '"':
                in_string = False
            continue
        if ch == '"':
            in_string = True
            continue
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                return text[brace + 1:index]
    raise SystemExit(f"[parser-recovery-stability][error] unterminated {signature}")


def quoted_array_values(body: str) -> list[str]:
    match = re.search(r"give\s*\[(?P<body>.*?)\]\s*;", body, re.S)
    if not match:
        return []
    return re.findall(r'"([^"]*)"', match.group("body"))


def main() -> int:
    parser_text = PARSER.read_text(encoding="utf-8")
    recovery_text = RECOVERY.read_text(encoding="utf-8")
    parser_tests_text = PARSER_TESTS.read_text(encoding="utf-8")
    strict_smoke_text = STRICT_SMOKE.read_text(encoding="utf-8")

    failures: list[str] = []

    if "space vitte/compiler/frontend/parse/parser" not in parser_text:
        failures.append(f"{PARSER.relative_to(ROOT)}: active parser module header is missing")
    if "space vitte/compiler/frontend/parse/recovery" not in recovery_text:
        failures.append(f"{RECOVERY.relative_to(ROOT)}: active recovery module header is missing")

    legacy_nonempty = [path.relative_to(ROOT) for path in LEGACY_PARSER_DIR.glob("*.vit") if path.stat().st_size > 0]
    if legacy_nonempty:
        failures.append(f"{LEGACY_PARSER_DIR.relative_to(ROOT)}: legacy parser files must stay empty or be removed: {legacy_nonempty}")

    recovery_kinds = extract_pick_members(recovery_text, "RecoveryKind")
    if recovery_kinds != EXPECTED_RECOVERY_KINDS:
        failures.append(
            f"{RECOVERY.relative_to(ROOT)}: RecoveryKind changed from {EXPECTED_RECOVERY_KINDS} to {recovery_kinds}"
        )

    sync_body = function_body(recovery_text, "proc parser_sync_points")
    sync_points = quoted_array_values(sync_body)
    if sync_points != EXPECTED_SYNC_POINTS:
        failures.append(f"{RECOVERY.relative_to(ROOT)}: parser sync points changed from {EXPECTED_SYNC_POINTS} to {sync_points}")

    if "const PARSE_DIAG_CAP: u64 = 3" not in recovery_text:
        failures.append(f"{RECOVERY.relative_to(ROOT)}: PARSE_DIAG_CAP must remain 3")
    if "const PARSE_RECOVERY_CAP: u64 = 64" not in recovery_text:
        failures.append(f"{RECOVERY.relative_to(ROOT)}: PARSE_RECOVERY_CAP must remain 64")
    if "preserve_primary_error: true" not in recovery_text:
        failures.append(f"{RECOVERY.relative_to(ROOT)}: default recovery must preserve primary errors")

    for proc in EXPECTED_RECOVER_INDEXES:
        if f"proc {proc}" not in recovery_text:
            failures.append(f"{RECOVERY.relative_to(ROOT)}: missing `{proc}`")

    for proc in ("recover_stmt_state", "recover_decl_state", "recover_expr_state", "recover_type_state", "recover_pattern_state"):
        if f"proc {proc}" not in parser_text:
            failures.append(f"{PARSER.relative_to(ROOT)}: missing parser state bridge `{proc}`")

    for internal, public in EXPECTED_PUBLIC_CODES.items():
        needle = f'if code == "{internal}"'
        if needle not in parser_text or f'give "{public}"' not in parser_text:
            failures.append(f"{PARSER.relative_to(ROOT)}: parser_public_code must map {internal} to {public}")

    for required in (
        "default_recovery_control()",
        "parser_recovery_allowed(walk, recovery_control)",
        "recover_stmt_state(walk)",
        "recover_decl_state(walk)",
        "walk.index == before",
        "PLOOP",
    ):
        if required not in parser_text:
            failures.append(f"{PARSER.relative_to(ROOT)}: missing recovery loop guard `{required}`")

    for test_name in EXPECTED_TESTS:
        if f"proc {test_name}" not in parser_tests_text:
            failures.append(f"{PARSER_TESTS.relative_to(ROOT)}: missing `{test_name}`")
        if test_name not in function_body(parser_tests_text, "proc run_all_tests"):
            failures.append(f"{PARSER_TESTS.relative_to(ROOT)}: `{test_name}` is not called by run_all_tests")

    if "target/bootstrap-c17/vitte-bootstrap" not in strict_smoke_text:
        failures.append(f"{STRICT_SMOKE.relative_to(ROOT)}: strict recovery smoke must default to the source bootstrap")

    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    manifest = {
        "failures": failures,
        "public_code_mappings": EXPECTED_PUBLIC_CODES,
        "recovery_kinds": recovery_kinds,
        "status": "ok" if not failures else "error",
        "sync_points": sync_points,
        "tests": EXPECTED_TESTS,
    }
    (REPORT_DIR / "parser_recovery_stability.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    (REPORT_DIR / "parser_recovery_stability.md").write_text(
        "\n".join(
            [
                "# Parser Recovery Stability",
                "",
                f"- recovery kinds: {len(recovery_kinds)}",
                f"- sync points: {len(sync_points)}",
                f"- public code mappings: {len(EXPECTED_PUBLIC_CODES)}",
                f"- tests: {len(EXPECTED_TESTS)}",
                f"- status: {manifest['status']}",
                "",
            ]
        ),
        encoding="utf-8",
    )

    if failures:
        print(f"[parser-recovery-stability][error] failures={len(failures)}")
        for failure in failures:
            print(f" - {failure}")
        return 1

    print(
        "[parser-recovery-stability] "
        f"kinds={len(recovery_kinds)} sync_points={len(sync_points)} public_codes={len(EXPECTED_PUBLIC_CODES)} tests={len(EXPECTED_TESTS)} status=ok"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
