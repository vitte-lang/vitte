#!/usr/bin/env python3
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
ACTIVE = ROOT / "tools/compiler_effective_gate/ACTIVE_FILES.txt"
CONTRACTS = ROOT / "tools/compiler_effective_gate/VERSIONED_CONTRACTS.txt"


def read_lines(p: Path):
    return [l.strip() for l in p.read_text(encoding="utf-8").splitlines() if l.strip() and not l.strip().startswith("#")]


def find_parent_mod(path: Path):
    parent = path.parent
    mod = parent / "mod.vit"
    return mod if mod.exists() else None


def has_use_of(mod: Path, child: Path):
    txt = mod.read_text(encoding="utf-8", errors="ignore")
    stem = child.stem
    return re.search(rf"\buse\b[^\n]*\b{re.escape(stem)}\b", txt) is not None


def referenced_in_driver(path: Path, driver_pipeline: Path):
    txt = driver_pipeline.read_text(encoding="utf-8", errors="ignore")
    rel = str(path).replace("\\", "/")
    stem = path.stem
    return (rel in txt) or (stem in txt)


def has_observable_signal(path: Path):
    txt = path.read_text(encoding="utf-8", errors="ignore")
    patterns = ["diagnostic", "diag", "print_", "message", "trace", "metric", "error", "warning", "report"]
    return any(p in txt for p in patterns)


def has_test_for(path: Path):
    tests = (ROOT / "tests")
    if not tests.exists():
        return False
    stem = path.stem
    mod = path.parent.name
    for t in tests.rglob("*"):
        if not t.is_file():
            continue
        name = t.name.lower()
        if stem.lower() in name or mod.lower() in name:
            return True
    return False


def file_non_empty(path: Path):
    return path.exists() and path.stat().st_size > 0


def main():
    active = [ROOT / p for p in read_lines(ACTIVE)]
    contracts = {str(ROOT / p) for p in read_lines(CONTRACTS)}

    driver_pipeline = ROOT / "src/vitte/compiler/driver/pipeline.vit"
    if not driver_pipeline.exists():
        print("FAIL: missing driver pipeline")
        return 1

    errors = []
    for f in active:
        if not f.exists():
            errors.append(f"missing file: {f}")
            continue

        # 1) exported by parent mod
        pm = find_parent_mod(f)
        if pm and f.name != "mod.vit":
            if not has_use_of(pm, f):
                errors.append(f"not exported by parent mod: {f} (parent {pm})")

        # 2) imported by real upper layer (approx): appear in some use statement outside own dir
        imported = False
        stem = f.stem
        for g in (ROOT / "src/vitte/compiler").rglob("*.vit"):
            if g == f:
                continue
            txt = g.read_text(encoding="utf-8", errors="ignore")
            if re.search(rf"\buse\b[^\n]*\b{re.escape(stem)}\b", txt):
                imported = True
                break
        if not imported:
            errors.append(f"not imported by upstream layer: {f}")

        # 3) called from runtime path via driver pipeline (approx transitively)
        if not referenced_in_driver(f, driver_pipeline) and "driver/" not in str(f).replace("\\", "/"):
            # allow transitive modules if referenced by backend/analysis/frontend pipeline
            pipelines = [
                ROOT / "src/vitte/compiler/frontend/pipeline.vit",
                ROOT / "src/vitte/compiler/analysis/pipeline.vit",
                ROOT / "src/vitte/compiler/backend/pipeline.vit",
            ]
            stemref = any((p.exists() and stem in p.read_text(encoding="utf-8", errors="ignore")) for p in pipelines)
            if not stemref:
                errors.append(f"not on runtime path from driver pipeline: {f}")

        # 4/5 tests nominal + error (heuristic: at least one test file contains mod/stem + one negative/err file exists)
        if not has_test_for(f):
            errors.append(f"missing nominal test mapping: {f}")
        neg_exists = any(p.name.endswith((".err.must", ".must", ".expect")) for p in (ROOT / "tests").rglob("*") if p.is_file())
        if not neg_exists:
            errors.append(f"missing error-test corpus globally (required by policy): {f}")

        # 6 observable signal
        if not has_observable_signal(f):
            errors.append(f"no observable signal (diag/log/metric/trace): {f}")

        # 7 covered by CI -> enforce workflow exists in repo
        if not (ROOT / ".github/workflows/compiler-effective-gate.yml").exists():
            errors.append("missing CI workflow: compiler-effective-gate.yml")

        # 8 contract versioned
        if str(f) not in contracts:
            errors.append(f"missing versioned contract registration: {f}")

        if not file_non_empty(f):
            errors.append(f"file is empty (cannot be active): {f}")

    if errors:
        print("compiler-effective-gate: FAIL")
        for e in errors:
            print(" -", e)
        return 1

    print("compiler-effective-gate: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
