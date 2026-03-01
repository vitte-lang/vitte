#!/usr/bin/env python3
from __future__ import annotations

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GRAMMAR = ROOT / "editors/grammar/vitte_highlight_grammar.json"
OUT = ROOT / "target/reports/highlights_coverage.json"


FILES = {
    "vim": ROOT / "editors/vim/syntax/vitte.vim",
    "emacs": ROOT / "editors/emacs/vitte-mode.el",
    "nano": ROOT / "editors/nano/vitte.nanorc",
}


def coverage(words: list[str], text: str) -> dict:
    hits = [w for w in words if re.search(rf"\b{re.escape(w)}\b", text)]
    return {"hit": len(hits), "total": len(words), "missing": sorted(list(set(words) - set(hits)))}


def main() -> int:
    g = json.loads(GRAMMAR.read_text(encoding="utf-8"))
    strict = g["strict_patterns"]

    data = {"schema_version": "1.0", "editors": {}, "gate": {}}
    failed = []

    for ed, path in FILES.items():
        txt = path.read_text(encoding="utf-8", errors="ignore")
        ed_data = {
            "keywords": coverage(g["keywords"], txt),
            "decl_keywords": coverage(g["decl_keywords"], txt),
            "types": coverage(g["types"], txt),
            "builtins": coverage(g["builtins"], txt),
            "strict": {
                "diag_code": "VITTE-" in txt,
                "alias_pkg": "_pkg" in txt,
                "contract_block": "ROLE-CONTRACT" in txt,
                "api_prefixes": all(p in txt for p in strict["api_prefixes"]),
            },
        }
        data["editors"][ed] = ed_data

    # CI gate: each declaration keyword must be present in BOTH vim and emacs.
    missing_vim = set(data["editors"]["vim"]["decl_keywords"]["missing"])
    missing_emacs = set(data["editors"]["emacs"]["decl_keywords"]["missing"])
    missing_both = sorted(list(missing_vim | missing_emacs))

    strict_vim = data["editors"]["vim"]["strict"]
    strict_emacs = data["editors"]["emacs"]["strict"]
    strict_ok = all(strict_vim.values()) and all(strict_emacs.values())

    if missing_both:
        failed.append("decl_keywords_missing_vim_or_emacs")
    if not strict_ok:
        failed.append("strict_highlights_missing_vim_or_emacs")

    data["gate"] = {
        "decl_keywords_missing_vim_or_emacs": missing_both,
        "strict_ok_vim": strict_vim,
        "strict_ok_emacs": strict_emacs,
        "pass": len(failed) == 0,
        "failed_checks": failed,
    }

    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    if failed:
        print("[highlights-coverage][error] failed checks: " + ",".join(failed))
        if missing_both:
            print("[highlights-coverage][error] decl keywords missing: " + ",".join(missing_both))
        return 1

    print("[highlights-coverage] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
