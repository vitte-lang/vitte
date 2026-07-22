#!/usr/bin/env python3
from __future__ import annotations
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MATRIX = ROOT / "src" / "vitte" / "stdlib" / "STDLIB_COVERAGE_MATRIX.vitl"

SECTION = '''
// ABI Coverage by OS/Arch:
//   ✓ linux-x86_64 -> sysv_x64
//   ✓ linux-arm64 -> aapcs_arm64
//   ✓ linux-riscv64 -> riscv64_lp64d
//   ✓ macos-x86_64 -> sysv_x64
//   ✓ macos-arm64 -> aapcs_arm64
//   ✓ windows-x86_64 -> ms_x64
//   • Evidence artifacts:
//     → src/vitte/stdlib/ffi/abi.vitl
//     → data/ffi/abi/abi_profiles.csv
//     → data/ffi/abi/abi_profiles.json
//     → tools/ffi/validate_abi_profiles.py
'''

def main() -> int:
    s = MATRIX.read_text(encoding="utf-8")
    if SECTION.strip() in s:
        print("[ffi-abi] matrix ABI section already present")
        return 0
    markers = (
        "\n\n// ============================================================================\n// INTEGRATION CHECKLIST",
        "\nconst STDLIB_COVERAGE_MATRIX_VERSION",
    )
    insert_at = -1
    for marker in markers:
        insert_at = s.find(marker)
        if insert_at >= 0:
            break
    if insert_at < 0:
        raise SystemExit("matrix insertion marker not found")
    out = s[:insert_at] + "\n" + SECTION + s[insert_at:]
    MATRIX.write_text(out, encoding="utf-8")
    print("[ffi-abi] matrix ABI coverage section inserted")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
