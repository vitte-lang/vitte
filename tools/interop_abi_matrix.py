#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import subprocess
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "target" / "reports" / "interop"
OUT_JSON = OUT_DIR / "abi_matrix.json"
OUT_MD = OUT_DIR / "abi_matrix.md"


def run(cmd: list[str]) -> tuple[int, str]:
    proc = subprocess.run(cmd, cwd=ROOT, text=True, capture_output=True)
    out = (proc.stdout or "") + (proc.stderr or "")
    return proc.returncode, out


def status_of(name: str, cmd: list[str]) -> dict:
    rc, out = run(cmd)
    return {"name": name, "cmd": " ".join(cmd), "rc": rc, "ok": rc == 0, "output": out[-4000:]}


def main() -> int:
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    checks = [
        status_of("extern-abi-host", ["make", "extern-abi-host"]),
        status_of("extern-abi-arduino", ["make", "extern-abi-arduino"]),
        status_of("extern-abi-kernel", ["make", "extern-abi-kernel"]),
        status_of("extern-abi-kernel-uefi", ["make", "extern-abi-kernel-uefi"]),
        status_of("extern-abi-all", ["make", "extern-abi-all"]),
    ]

    host = {"os": os.uname().sysname, "arch": os.uname().machine}
    ok = all(c["ok"] for c in checks)
    data = {
        "timestamp_utc": datetime.now(timezone.utc).isoformat(),
        "host": host,
        "kpi_zero_abi_regression": ok,
        "checks": checks,
    }
    OUT_JSON.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")

    lines = [
        "# ABI Matrix",
        "",
        f"- Timestamp (UTC): `{data['timestamp_utc']}`",
        f"- Host: `{host['os']}-{host['arch']}`",
        f"- KPI (0 ABI regression): `{ok}`",
        "",
        "## Checks",
        "",
    ]
    for check in checks:
        lines.append(f"- `{check['name']}`: `{check['ok']}`")
    OUT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"[interop-abi-matrix] wrote {OUT_JSON.relative_to(ROOT)}")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
