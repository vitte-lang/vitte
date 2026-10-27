#!/usr/bin/env python3
"""Run deterministic compiler fuzz regression binaries continuously.

The individual C targets remain bounded and reproducible. This driver repeats
them for a time budget so CI and local stress runs exercise fresh process
lifetimes, allocator teardown, and import/configuration state repeatedly.
"""

from __future__ import annotations

import argparse
import os
import subprocess
import time


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--seconds", type=float, default=60.0)
    parser.add_argument("--iterations", type=int, default=1000)
    parser.add_argument("tests", nargs="+")
    args = parser.parse_args()

    if args.seconds <= 0 or args.iterations < 0:
        parser.error("seconds must be positive and iterations non-negative")

    environment = os.environ.copy()
    environment["VITTE_FUZZ_ITERATIONS"] = str(args.iterations)
    deadline = time.monotonic() + args.seconds
    rounds = 0
    while time.monotonic() < deadline:
        rounds += 1
        for test in args.tests:
            result = subprocess.run([test], env=environment)
            if result.returncode != 0:
                return result.returncode
            if time.monotonic() >= deadline:
                break
    print(f"fuzzing completed: {rounds} rounds, {args.seconds:.1f}s")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
