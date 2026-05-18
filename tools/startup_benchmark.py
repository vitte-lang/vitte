#!/usr/bin/env python3
"""
Startup-time benchmark gate for Vitte compiler.

Measures compiler startup time and tracks regression.
Usage:
  ./tools/startup_benchmark.py [--strict] [--update-baseline]
"""

import sys
import os
import json
import subprocess
import time
import csv
from pathlib import Path
from datetime import datetime

# Configuration
BIN_PATH = "bin/vitte"
BASELINE_FILE = "target/startup_benchmarks/baseline.json"
REPORT_FILE = "target/reports/startup_benchmark_report.md"
CSV_FILE = "target/startup_benchmarks/results.csv"
RESULTS_DIR = Path("target/startup_benchmarks")

# Thresholds (in milliseconds)
MAX_STARTUP_TIME_MS = 500  # Startup should be < 500ms
REGRESSION_THRESHOLD = 1.15  # Allow 15% regression before failing

def ensure_dirs():
    """Create necessary directories."""
    RESULTS_DIR.mkdir(parents=True, exist_ok=True)
    Path("target/reports").mkdir(parents=True, exist_ok=True)

def get_baseline():
    """Load baseline startup time or return None."""
    baseline_path = Path(BASELINE_FILE)
    if baseline_path.exists():
        try:
            with open(baseline_path) as f:
                data = json.load(f)
                return data.get("startup_time_ms", None)
        except:
            return None
    return None

def save_baseline(startup_time_ms):
    """Save baseline startup time."""
    baseline = {
        "startup_time_ms": startup_time_ms,
        "timestamp": datetime.now().isoformat(),
        "vitte_version": get_vitte_version()
    }
    with open(BASELINE_FILE, "w") as f:
        json.dump(baseline, f, indent=2)

def get_vitte_version():
    """Get Vitte compiler version."""
    try:
        output = subprocess.check_output([BIN_PATH, "--version"], 
                                        stderr=subprocess.DEVNULL,
                                        text=True)
        return output.strip().split("\n")[0]
    except:
        return "unknown"

def measure_startup_time(iterations=5):
    """Measure compiler startup time using --help (minimal work)."""
    times = []
    
    for i in range(iterations):
        try:
            start = time.perf_counter()
            subprocess.run([BIN_PATH, "--help"], 
                          capture_output=True,
                          timeout=10)
            elapsed = (time.perf_counter() - start) * 1000  # Convert to ms
            times.append(elapsed)
            print(f"[startup-benchmark] iteration {i+1}: {elapsed:.2f}ms")
        except Exception as e:
            print(f"[startup-benchmark][error] iteration {i+1} failed: {e}", 
                  file=sys.stderr)
            return None
    
    # Return median to reduce noise
    times.sort()
    if len(times) % 2 == 1:
        return times[len(times) // 2]
    else:
        return (times[len(times)//2 - 1] + times[len(times)//2]) / 2

def check_binary():
    """Verify that binary exists and is executable."""
    if not os.path.exists(BIN_PATH):
        print(f"[startup-benchmark][error] binary not found: {BIN_PATH}", 
              file=sys.stderr)
        print("[startup-benchmark][error] run: make build", file=sys.stderr)
        return False
    
    if not os.access(BIN_PATH, os.X_OK):
        print(f"[startup-benchmark][error] binary not executable: {BIN_PATH}", 
              file=sys.stderr)
        return False
    
    return True

def record_result(startup_time_ms):
    """Record result in CSV for historical tracking."""
    csv_path = Path(CSV_FILE)
    result = {
        "timestamp": datetime.now().isoformat(),
        "startup_time_ms": f"{startup_time_ms:.2f}",
        "vitte_version": get_vitte_version(),
        "iteration_count": 5
    }
    
    # Append to CSV
    file_exists = csv_path.exists()
    with open(csv_path, "a", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=result.keys())
        if not file_exists:
            writer.writeheader()
        writer.writerow(result)
    
    print(f"[startup-benchmark] recorded in {CSV_FILE}")

def generate_report(startup_time_ms, baseline_ms, passed):
    """Generate markdown report."""
    report_path = Path(REPORT_FILE)
    
    if baseline_ms:
        change_pct = ((startup_time_ms - baseline_ms) / baseline_ms) * 100
        change_str = f"{change_pct:+.1f}%"
        regression = startup_time_ms / baseline_ms
    else:
        change_str = "N/A (baseline not set)"
        regression = 1.0
    
    status = "✅ PASS" if passed else "❌ FAIL"
    
    report = f"""# Startup Time Benchmark Report

**Date**: {datetime.now().isoformat()}

## Results

| Metric | Value |
|--------|-------|
| Startup Time | {startup_time_ms:.2f}ms |
| Max Allowed | {MAX_STARTUP_TIME_MS}ms |
| Baseline | {baseline_ms:.2f}ms if baseline_ms else "Not set"} |
| Change vs Baseline | {change_str} |
| Status | {status} |

## Analysis

- **Current startup time**: {startup_time_ms:.2f}ms
- **Threshold**: {MAX_STARTUP_TIME_MS}ms
- **Baseline**: {baseline_ms:.2f}ms if baseline_ms else "No baseline set (first run)"}
- **Regression factor**: {regression:.2f}x

## Gate Status

- Startup time acceptable: {"✅ YES" if startup_time_ms <= MAX_STARTUP_TIME_MS else "❌ NO"}
- No regression detected: {"✅ YES" if regression < REGRESSION_THRESHOLD else "❌ NO (>" + str(REGRESSION_THRESHOLD) + "x)"}
- **Overall**: {status}

---
Generated by `tools/startup_benchmark.py`
"""
    
    with open(report_path, "w") as f:
        f.write(report)
    
    print(f"[startup-benchmark] report: {report_path}")

def main():
    """Main entry point."""
    args = sys.argv[1:]
    strict_mode = "--strict" in args
    update_baseline = "--update-baseline" in args
    
    ensure_dirs()
    
    # Check binary exists
    if not check_binary():
        return 2
    
    # Measure startup time
    print("[startup-benchmark] measuring compiler startup time...")
    startup_time_ms = measure_startup_time(iterations=5)
    
    if startup_time_ms is None:
        print("[startup-benchmark][error] measurement failed", file=sys.stderr)
        return 1
    
    print(f"[startup-benchmark] median startup time: {startup_time_ms:.2f}ms")
    
    # Get baseline
    baseline_ms = get_baseline()
    
    # Check thresholds
    time_ok = startup_time_ms <= MAX_STARTUP_TIME_MS
    
    regression = 1.0
    if baseline_ms:
        regression = startup_time_ms / baseline_ms
        print(f"[startup-benchmark] regression vs baseline: {regression:.2f}x")
    
    regression_ok = regression < REGRESSION_THRESHOLD
    
    # Record result
    record_result(startup_time_ms)
    
    # Generate report
    passed = time_ok and regression_ok
    generate_report(startup_time_ms, baseline_ms, passed)
    
    # Update baseline if requested
    if update_baseline:
        save_baseline(startup_time_ms)
        print(f"[startup-benchmark] baseline updated: {startup_time_ms:.2f}ms")
    elif not baseline_ms:
        save_baseline(startup_time_ms)
        print(f"[startup-benchmark] baseline set (first run): {startup_time_ms:.2f}ms")
    
    # Print results
    print(f"\n[startup-benchmark] results:")
    print(f"  Startup time: {startup_time_ms:.2f}ms (max: {MAX_STARTUP_TIME_MS}ms)")
    if baseline_ms:
        print(f"  vs Baseline: {regression:.2f}x (threshold: {REGRESSION_THRESHOLD}x)")
    print(f"  Status: {'✅ PASS' if passed else '❌ FAIL'}")
    
    # Return exit code
    if strict_mode and not passed:
        print("\n[startup-benchmark][error] gate failed in strict mode", 
              file=sys.stderr)
        return 1
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
