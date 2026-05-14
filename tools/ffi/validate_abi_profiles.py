#!/usr/bin/env python3
from __future__ import annotations
import csv, json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
CSV_PATH = ROOT / "data" / "ffi" / "abi" / "abi_profiles.csv"
JSON_PATH = ROOT / "data" / "ffi" / "abi" / "abi_profiles.json"
ABI_VITL = ROOT / "src" / "vitte" / "stdlib" / "ffi" / "abi.vitl"

EXPECTED = {
    "linux-x86_64": "sysv_x64",
    "linux-arm64": "aapcs_arm64",
    "linux-riscv64": "riscv64_lp64d",
    "macos-x86_64": "sysv_x64",
    "macos-arm64": "aapcs_arm64",
    "windows-x86_64": "ms_x64",
}


def err(msg):
    print(f"[ffi-abi][error] {msg}", file=sys.stderr)
    return 1


def main() -> int:
    if not CSV_PATH.exists() or not JSON_PATH.exists() or not ABI_VITL.exists():
        return err("missing required ABI artifacts")

    rows = list(csv.DictReader(CSV_PATH.open(newline="", encoding="utf-8")))
    if len(rows) != len(EXPECTED):
        return err("unexpected number of ABI rows")

    with JSON_PATH.open(encoding="utf-8") as f:
        j = json.load(f)
    if "profiles" not in j or len(j["profiles"]) != len(EXPECTED):
        return err("invalid abi_profiles.json content")

    seen = set()
    for r in rows:
        t = r["target"]
        if t not in EXPECTED:
            return err(f"unexpected target {t}")
        if t in seen:
            return err(f"duplicate target {t}")
        seen.add(t)
        if r["calling_convention"] != EXPECTED[t]:
            return err(f"calling convention mismatch for {t}")
        for k in ["pointer_size", "int_alignment", "pointer_alignment", "float_alignment"]:
            if int(r[k]) <= 0:
                return err(f"invalid numeric ABI value for {t}:{k}")
        if not r["arg_registers"].strip() or not r["return_register"].strip() or not r["preserve_registers"].strip():
            return err(f"empty register set for {t}")

    src = ABI_VITL.read_text(encoding="utf-8")
    required_symbols = [
        "proc abi_profile_linux_x86_64()",
        "proc abi_profile_linux_arm64()",
        "proc abi_profile_linux_riscv64()",
        "proc abi_profile_macos_x86_64()",
        "proc abi_profile_macos_arm64()",
        "proc abi_profile_windows_x86_64()",
        "proc abi_profile_for(os_name: string, arch: string)",
    ]
    for sym in required_symbols:
        if sym not in src:
            return err(f"missing ABI symbol in abi.vitl: {sym}")

    print("[ffi-abi] ABI profiles valid")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
