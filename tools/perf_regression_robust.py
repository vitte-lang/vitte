#!/usr/bin/env python3
from __future__ import annotations
import argparse
import json
import re
from pathlib import Path
from statistics import median

ROOT = Path(__file__).resolve().parents[1]


def parse_values(path: Path) -> list[float]:
    if not path.exists():
        return []
    txt = path.read_text(encoding='utf-8')
    vals = [float(x) for x in re.findall(r'measured_ms:\s*([0-9]+(?:\.[0-9]+)?)', txt)]
    if vals:
        return vals
    single = re.search(r'([0-9]+(?:\.[0-9]+)?)', txt)
    return [float(single.group(1))] if single else []


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument('--profile', default='ci', choices=['ci', 'dev'])
    ap.add_argument('--threshold-pct', type=float, default=20.0)
    ap.add_argument('--warmup-min-ms', type=float, default=0.0)
    ap.add_argument('--variance-cap-pct', type=float, default=75.0)
    args = ap.parse_args()

    baseline_file = ROOT / f'tools/perf_baseline.{args.profile}.json'
    baseline = json.loads(baseline_file.read_text(encoding='utf-8')) if baseline_file.exists() else {}

    pairs = {
        'json': ROOT/'target/bench/json_bench_micro.out',
        'yaml': ROOT/'target/bench/yaml_bench_micro.out',
        'test': ROOT/'target/bench/test_bench_micro.out',
        'lint': ROOT/'target/bench/lint_bench_micro.out',
    }

    errs = []
    for pkg, path in pairs.items():
        vals = parse_values(path)
        if not vals:
            print(f'[perf-robust][warn] skip {pkg}: no values')
            continue
        vals = [v for v in vals if v >= args.warmup_min_ms]
        if not vals:
            print(f'[perf-robust][warn] skip {pkg}: filtered by warmup-min-ms')
            continue
        med = median(vals)
        spread = (max(vals) - min(vals)) / med * 100.0 if med > 0 else 0.0
        b = float(baseline.get(pkg, med))
        delta = ((med - b) / b) * 100.0 if b > 0 else 0.0
        print(f'[perf-robust] {pkg}: median={med:.2f} baseline={b:.2f} delta={delta:.2f}% variance={spread:.2f}%')
        if spread > args.variance_cap_pct:
            errs.append(f'{pkg}: variance {spread:.2f}% > cap {args.variance_cap_pct:.2f}%')
        if delta > args.threshold_pct:
            errs.append(f'{pkg}: regression {delta:.2f}% > {args.threshold_pct:.2f}%')

    if errs:
        for e in errs:
            print(f'[perf-robust][error] {e}')
        return 1
    print('[perf-robust] OK')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
