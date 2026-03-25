#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import tempfile
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BIN = ROOT / "bin" / "vitte"
OUT = ROOT / "target" / "bench" / "dx_hello_prod.latest.json"
DEFAULT_BASELINE_MS = 2500.0


def run_checked(cmd: list[str], cwd: Path) -> float:
    t0 = time.perf_counter()
    subprocess.run(cmd, cwd=str(cwd), check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return (time.perf_counter() - t0) * 1000.0


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--template", default="cli", choices=["cli", "service", "lib-native"])
    ap.add_argument("--strict", action="store_true", help="fail if target reduction is not met")
    ap.add_argument(
        "--baseline-ms",
        type=float,
        default=float(os.environ.get("DX_HELLO_BASELINE_MS", DEFAULT_BASELINE_MS)),
        help="baseline hello->prod duration in ms (target is 60%% of this value)",
    )
    args = ap.parse_args()

    if not BIN.exists():
        raise SystemExit(f"missing compiler binary: {BIN}")

    tmp_root = Path(tempfile.mkdtemp(prefix="vitte-dx-bench-"))
    proj = tmp_root / "app"

    try:
        init_ms = run_checked([str(BIN), "init", str(proj), "--template", args.template], ROOT)
        check_ms = run_checked([str(BIN), "check", "--no-auto-reduce", "src/main.vit"], proj)
        emit_ms = run_checked(
            [
                str(BIN),
                "emit",
                "--no-auto-reduce",
                "--runtime-include",
                str(ROOT / "src" / "compiler" / "backends" / "runtime"),
                "src/main.vit",
            ],
            proj,
        )
        total_ms = init_ms + check_ms + emit_ms

        target_ms = args.baseline_ms * 0.60
        reduction_percent = 100.0 * (1.0 - (total_ms / args.baseline_ms))
        met_target = total_ms <= target_ms

        report = {
            "schema_version": "1.0",
            "template": args.template,
            "baseline_ms": args.baseline_ms,
            "target_ms": target_ms,
            "timings_ms": {
                "init": round(init_ms, 3),
                "check": round(check_ms, 3),
                "emit": round(emit_ms, 3),
                "hello_to_prod_total": round(total_ms, 3),
            },
            "reduction_percent": round(reduction_percent, 3),
            "target_met": met_target,
        }
        OUT.parent.mkdir(parents=True, exist_ok=True)
        OUT.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        print(f"[dx-hello-prod] wrote {OUT}")
        print(
            f"[dx-hello-prod] total={total_ms:.3f}ms target={target_ms:.3f}ms "
            f"reduction={reduction_percent:.2f}% met={met_target}"
        )

        if args.strict and not met_target:
            print("[dx-hello-prod][error] KPI not met: expected >=40% reduction versus baseline")
            return 1
        return 0
    finally:
        shutil.rmtree(tmp_root, ignore_errors=True)


if __name__ == "__main__":
    raise SystemExit(main())
