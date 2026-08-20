#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CLI_SURFACES = (
    ROOT / "src" / "vitte" / "compiler" / "driver" / "compiler.vit",
    ROOT / "src" / "vitte" / "compiler" / "driver" / "exit_status.vit",
)
EN_FTL = ROOT / "locales" / "en" / "diagnostics.ftl"

CODE_RE = re.compile(r"\b(?:[A-Z][A-Z0-9]*_)+(?:E|W|F|I)_[A-Z0-9_]+\b|\bE[0-9]{4}\b|\bP[0-9A-Z_]+\b")
FTL_KEY_RE = re.compile(r"^([A-Z][A-Z0-9_]*|E[0-9]{4}|P[0-9A-Z_]+)\s*=", re.MULTILINE)
CALL_RE = re.compile(
    r"\b(?:emit|diag|cli_error)\s*\(?\s*[\"']"
    r"([A-Z][A-Z0-9_]*|E[0-9]{4}|P[0-9A-Z_]+)[\"']"
)

IGNORED_TOKENS = {
    "PARSE_",
    "P1",
    "P2",
    "P3",
    "P4",
    "P6",
    "P8",
    "P9",
    "PKP",
    "PP",
    "PATH",
    "PL",
    "PT",
    "PWD",
}


def looks_like_diagnostic(code: str) -> bool:
    if code in IGNORED_TOKENS:
        return False
    if re.fullmatch(r"P[0-9A-Z_]{1,3}", code) is not None:
        return False
    return (
        "_E_" in code
        or "_W_" in code
        or code.startswith(("E_BOOTSTRAP_", "E_CLI_", "E_BACKEND_", "E_BUILD_", "E_STRICT_"))
        or re.fullmatch(r"E[0-9]{4}", code) is not None
        or re.fullmatch(r"P[0-9A-Z_]+", code) is not None
    )


def emitted_cli_codes(cli_text: str) -> set[str]:
    codes = {match.group(1) for match in CALL_RE.finditer(cli_text)}
    codes.update(code for code in CODE_RE.findall(cli_text) if looks_like_diagnostic(code))
    return codes


def ftl_codes(ftl_text: str) -> set[str]:
    return {match.group(1) for match in FTL_KEY_RE.finditer(ftl_text)}


def main() -> int:
    cli_text = "\n".join(path.read_text(encoding="utf-8", errors="ignore") for path in CLI_SURFACES)
    ftl_text = EN_FTL.read_text(encoding="utf-8")
    emitted = emitted_cli_codes(cli_text)
    catalog = ftl_codes(ftl_text)
    missing = sorted(emitted - catalog)
    if missing:
        print("[cli-diagnostics-fluent][error] CLI emits codes missing from locales/en/diagnostics.ftl", file=sys.stderr)
        for code in missing:
            print(f"  - {code}", file=sys.stderr)
        return 1
    print(f"[cli-diagnostics-fluent] emitted={len(emitted)} catalog={len(catalog)} status=ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
