#!/usr/bin/env python3
from __future__ import annotations
import argparse, re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def parse_ms(path: Path) -> float | None:
    if not path.exists():
        return None
    txt = path.read_text(encoding='utf-8')
    m = re.search(r'measured_ms:\s*([0-9]+(?:\.[0-9]+)?)', txt)
    if m:
        return float(m.group(1))
    return None


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument('--threshold-pct', type=float, default=15.0)
    args = ap.parse_args()

    pairs = [
        (ROOT/'tests/core/bench/core_bench_micro.must', ROOT/'target/bench/core_bench_micro.out', 'core'),
        (ROOT/'tests/std/bench/std_bench_micro.must', ROOT/'target/bench/std_bench_micro.out', 'std'),
        (ROOT/'tests/log/bench/log_bench_micro.must', ROOT/'target/bench/log_bench_micro.out', 'log'),
        (ROOT/'tests/fs/bench/fs_bench_micro.must', ROOT/'target/bench/fs_bench_micro.out', 'fs'),
        (ROOT/'tests/db/bench/db_bench_micro.must', ROOT/'target/bench/db_bench_micro.out', 'db'),
        (ROOT/'tests/http/bench/http_bench_micro.must', ROOT/'target/bench/http_bench_micro.out', 'http'),
        (ROOT/'tests/http_client/bench/http_client_bench_micro.must', ROOT/'target/bench/http_client_bench_micro.out', 'http_client'),
        (ROOT/'tests/process/bench/process_bench_micro.must', ROOT/'target/bench/process_bench_micro.out', 'process'),
        (ROOT/'tests/json/bench/json_bench_micro.must', ROOT/'target/bench/json_bench_micro.out', 'json'),
        (ROOT/'tests/yaml/bench/yaml_bench_micro.must', ROOT/'target/bench/yaml_bench_micro.out', 'yaml'),
        (ROOT/'tests/test/bench/test_bench_micro.must', ROOT/'target/bench/test_bench_micro.out', 'test'),
        (ROOT/'tests/lint/bench/lint_bench_micro.must', ROOT/'target/bench/lint_bench_micro.out', 'lint'),
    ]

    errs = []
    for base, cur, name in pairs:
        b = parse_ms(base)
        c = parse_ms(cur)
        if b is None or c is None or b <= 0:
            print(f'[perf-regression][warn] skip {name} baseline/current unavailable')
            continue
        delta = ((c - b) / b) * 100.0
        print(f'[perf-regression] {name}: baseline={b} current={c} delta={delta:.2f}%')
        if delta > args.threshold_pct:
            errs.append(f'{name}: regression {delta:.2f}% > {args.threshold_pct:.2f}%')

    if errs:
        for e in errs:
            print(f'[perf-regression][error] {e}')
        return 1
    print('[perf-regression] OK')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
