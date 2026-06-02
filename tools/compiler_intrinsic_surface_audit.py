#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
HOST_RUNTIME = ROOT / "src" / "vitte" / "stdlib" / "io" / "host_runtime.vitl"
PARSER = ROOT / "src" / "vitte" / "compiler" / "frontend" / "parse" / "parser.vit"
AST_ITEM = ROOT / "src" / "vitte" / "compiler" / "frontend" / "ast" / "item.vit"
AST_VALIDATE = ROOT / "src" / "vitte" / "compiler" / "frontend" / "ast" / "validate.vit"
REPORT = ROOT / "target" / "reports" / "compiler_intrinsic_surface" / "audit.json"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def run_command(args: list[str]) -> dict[str, object]:
    completed = subprocess.run(
        args,
        cwd=ROOT,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    return {
        "command": args,
        "returncode": completed.returncode,
        "stdout": completed.stdout,
        "stderr": completed.stderr,
    }


def has_source_support() -> dict[str, object]:
    parser_text = read(PARSER)
    item_text = read(AST_ITEM)
    validate_text = read(AST_VALIDATE)
    checks = {
        "item_kind_maps_intrinsic": 'if keyword == "intrinsic"' in parser_text,
        "item_kind_maps_query": 'if keyword == "query"' in parser_text,
        "ast_builder_handles_intrinsic_query": 'if keyword == "intrinsic" or keyword == "query"' in parser_text,
        "ast_item_kind_name_intrinsic": 'elif kind == AstItemKind.Intrinsic { give "intrinsic"; }' in item_text,
        "ast_item_kind_name_query": 'elif kind == AstItemKind.Query { give "query"; }' in item_text,
        "ast_validate_requires_intrinsic_name": "AstItemKind.Intrinsic" in validate_text,
        "ast_validate_requires_query_name": "AstItemKind.Query" in validate_text,
    }
    return {
        "status": "pass" if all(checks.values()) else "fail",
        "checks": checks,
    }


def bootstrap_support() -> dict[str, object]:
    parse_cmd = ["bin/vitte", "parse", "--src", str(HOST_RUNTIME), "--dump-ast-json"]
    check_cmd = ["bin/vitte", "check", "--src", str(HOST_RUNTIME), "--dump-hir-json"]
    parse_result = run_command(parse_cmd)
    check_result = run_command(check_cmd)

    parse_combined = f"{parse_result['stdout']}{parse_result['stderr']}"
    check_combined = f"{check_result['stdout']}{check_result['stderr']}"
    parse_bootstrap_limited = "E_BOOTSTRAP_TOP_LEVEL" in parse_combined
    check_parser_rejects_intrinsic = "expected one of: proc, const, form, pick, use, import, export, share" in check_combined

    status = "pass"
    if parse_result["returncode"] != 0 or check_result["returncode"] != 0:
        status = "limited" if parse_bootstrap_limited or check_parser_rejects_intrinsic else "fail"

    return {
        "status": status,
        "parse": parse_result,
        "check": check_result,
        "observations": {
            "bootstrap_top_level_limited": parse_bootstrap_limited,
            "parser_rejects_intrinsic_toplevel": check_parser_rejects_intrinsic,
        },
    }


def main() -> int:
    source = has_source_support()
    bootstrap = bootstrap_support()

    if source["status"] == "pass" and bootstrap["status"] == "pass":
        overall = "pass"
    elif source["status"] == "pass" and bootstrap["status"] == "limited":
        overall = "partial"
    else:
        overall = "fail"

    payload = {
        "schema": "vitte.compiler.intrinsic_surface_audit",
        "schema_version": "1.0.0",
        "status": overall,
        "source_support": source,
        "bootstrap_support": bootstrap,
        "target_module": str(HOST_RUNTIME.relative_to(ROOT)),
    }

    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, ensure_ascii=True, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    print(f"[compiler-intrinsic-surface] status={overall} report={REPORT.relative_to(ROOT)}")
    print(f"[compiler-intrinsic-surface] source_support={source['status']} bootstrap_support={bootstrap['status']}")
    return 0 if overall in {"pass", "partial"} else 1


if __name__ == "__main__":
    raise SystemExit(main())
