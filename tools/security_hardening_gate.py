#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "target/reports/security_hardening.json"


def has_any(path: Path, needles: list[str]) -> bool:
    if not path.exists():
        return False
    txt = path.read_text(encoding="utf-8", errors="ignore").lower()
    return any(n.lower() in txt for n in needles)


def check(name: str, ok: bool, weight: int, why: str) -> dict:
    return {
        "name": name,
        "ok": ok,
        "weight": weight,
        "score": weight if ok else 0,
        "why": why,
    }


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--min-score", type=int, default=75)
    args = ap.parse_args()

    checks = []

    http_mod = ROOT / "src/vitte/packages/http/mod.vit"
    http_rt = ROOT / "src/vitte/packages/http/internal/runtime.vit"
    httpc_mod = ROOT / "src/vitte/packages/http_client/mod.vit"
    httpc_rt = ROOT / "src/vitte/packages/http_client/internal/runtime.vit"
    process_mod = ROOT / "src/vitte/packages/process/mod.vit"
    process_rt = ROOT / "src/vitte/packages/process/internal/runtime.vit"
    fs_mod = ROOT / "src/vitte/packages/fs/mod.vit"
    fs_rt = ROOT / "src/vitte/packages/fs/internal/runtime.vit"

    checks.append(
        check(
            "ssrf_guards",
            has_any(http_mod, ["ssrf", "private cidr", "allowlist"]) or has_any(httpc_mod, ["ssrf", "private cidr", "allowlist"]),
            20,
            "HTTP/HTTP client doivent exposer des garde-fous SSRF.",
        )
    )
    checks.append(
        check(
            "tls_hardening",
            has_any(http_mod, ["tls", "insecure", "cert", "redirect"]) or has_any(httpc_mod, ["tls", "insecure", "cert", "redirect"]),
            20,
            "HTTP/HTTP client doivent couvrir policy TLS/redirects.",
        )
    )
    checks.append(
        check(
            "crlf_header_injection",
            has_any(http_rt, ["crlf", "header injection"]) or has_any(httpc_rt, ["crlf", "header injection"]),
            20,
            "Runtime HTTP doit détecter CRLF/header injection.",
        )
    )
    checks.append(
        check(
            "process_policy",
            has_any(process_mod, ["allow_shell", "allowlist", "policy"]) and has_any(process_rt, ["denylisted", "shell", "allowlist"]),
            20,
            "Process doit imposer policy shell/allowlist explicite.",
        )
    )
    checks.append(
        check(
            "fs_allowlist_workspace",
            has_any(fs_mod, ["allowlist", "workspace", "roots", "sensitive"]) or has_any(fs_rt, ["allowlist", "workspace", "roots"]),
            20,
            "FS doit contrôler les roots/allowlist de chemins.",
        )
    )

    total = sum(c["score"] for c in checks)
    max_score = sum(c["weight"] for c in checks)
    data = {
        "schema_version": "1.0",
        "min_score": args.min_score,
        "score": total,
        "max_score": max_score,
        "checks": checks,
        "pass": total >= args.min_score,
    }
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    if data["pass"]:
        print(f"[security-hardening] OK score={total}/{max_score} min={args.min_score}")
        return 0
    print(f"[security-hardening][error] score={total}/{max_score} below min={args.min_score}")
    for c in checks:
        if not c["ok"]:
            print(f"[security-hardening][error] missing: {c['name']} ({c['why']})")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
