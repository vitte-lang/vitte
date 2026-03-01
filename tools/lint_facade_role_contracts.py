#!/usr/bin/env python3
from __future__ import annotations
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CFG = ROOT / "tools/facade_packages.json"

REQUIRED_KEYS = [
    "package",
    "role",
    "input_contract",
    "output_contract",
    "boundary",
    "versioning",
    "api_surface_stable",
    "diagnostics",
    "compat_policy",
    "internal_exports",
    "api_version",
]


def parse_role_contract(txt: str) -> dict[str, str]:
    m = re.search(r"<<< ROLE-CONTRACT\n(.*?)\n>>>", txt, flags=re.S)
    if not m:
        return {}
    out: dict[str, str] = {}
    for line in m.group(1).splitlines():
        line = line.strip()
        if not line or ":" not in line:
            continue
        k, v = line.split(":", 1)
        out[k.strip()] = v.strip()
    return out


def main() -> int:
    cfg = json.loads(CFG.read_text(encoding="utf-8"))
    errs: list[str] = []

    for ent in cfg["packages"]:
        pkg = ent["name"]
        diag_pref = ent["diag_prefix"]
        mod = ROOT / f"src/vitte/packages/{pkg}/mod.vit"
        txt = mod.read_text(encoding="utf-8") if mod.exists() else ""
        if not txt:
            errs.append(f"{pkg}: missing mod.vit")
            continue
        if "PREAMBLE (API stable facade)" not in txt:
            errs.append(f"{pkg}: missing PREAMBLE")
        if re.search(r"^\s*entry\s+", txt, flags=re.M):
            errs.append(f"{pkg}: entry forbidden in facade")

        block = parse_role_contract(txt)
        if not block:
            errs.append(f"{pkg}: missing ROLE-CONTRACT block")
            continue

        for k in REQUIRED_KEYS:
            if k not in block:
                errs.append(f"{pkg}: ROLE-CONTRACT missing key '{k}'")

        if block.get("package") != f"vitte/{pkg}":
            errs.append(f"{pkg}: ROLE-CONTRACT package mismatch")
        if block.get("compat_policy") != "additive-only":
            errs.append(f"{pkg}: compat_policy must be additive-only")
        if block.get("internal_exports") != "forbidden":
            errs.append(f"{pkg}: internal_exports must be forbidden")
        if not re.fullmatch(r"v[0-9]+(?:\.[0-9]+)?", block.get("api_version", "")):
            errs.append(f"{pkg}: api_version must match v<major>[.<minor>]")

        diagnostics = block.get("diagnostics", "")
        if diag_pref not in diagnostics:
            errs.append(f"{pkg}: diagnostics range must contain {diag_pref}")
        if not re.search(rf"{re.escape(diag_pref)}[A-Z0-9x]*", diagnostics):
            errs.append(f"{pkg}: diagnostics format invalid")

        for need_proc in ["proc api_version()", "proc doctor_status()", "proc diagnostics_message(", "proc diagnostics_quickfix(", "proc diagnostics_doc_url("]:
            if need_proc not in txt:
                errs.append(f"{pkg}: missing facade proc {need_proc}")

    if errs:
        for e in errs:
            print(f"[facade-role-contracts][error] {e}")
        return 1
    print("[facade-role-contracts] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
