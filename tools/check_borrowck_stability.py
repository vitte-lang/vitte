#!/usr/bin/env python3
"""Check borrowck ownership, loans, moves, lifetimes, and region contracts."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BORROWCK = ROOT / "src/vitte/compiler/analysis/borrowck"
PIPELINE = ROOT / "src/vitte/compiler/analysis/pipeline.vit"
REPORT_DIR = ROOT / "target/reports"
REPORT = REPORT_DIR / "borrowck_stability.json"
MARKDOWN = REPORT_DIR / "borrowck_stability.md"

EXPECTED_PICKS = {
    "BorrowErrorKind": ["None", "MoveAfterMove", "PartialMove", "UseAfterMove", "BorrowOfMovedValue", "MutableBorrowConflict", "SharedBorrowConflict", "WriteWhileBorrowed", "MoveWhileBorrowed", "DropWhileBorrowed", "AssignWhileBorrowed", "ReturnRefToLocal", "ReturnBorrowOfLocal", "DanglingReference", "LifetimeTooShort", "ImmutableAssign", "UseAfterDrop", "DoubleDrop", "UninitializedUse", "Internal"],
    "BorrowPhase": ["CollectMoves", "CollectLoans", "AliasAnalysis", "CaptureAnalysis", "DropAnalysis", "Dataflow", "RegionCheck", "Validate", "Report", "Unknown"],
    "MoveState": ["Available", "Moved", "MaybeMoved", "Reinitialized", "Dropped", "Dead", "Unknown"],
    "LoanState": ["Active", "Inactive", "Expired", "Killed", "Invalid"],
    "LifetimeEscapeKind": ["None", "Return", "StoreGlobal", "StoreExternal", "Capture", "AsyncSuspend", "Unknown"],
    "MemoryRegionKind": ["Unknown", "Stack", "Heap", "Arena", "Static", "Mmio", "Dma"],
}

REQUIRED_PROCS = (
    "borrow_check_hir",
    "borrow_check_mir",
    "borrow_check_hir_mir",
    "borrow_check_source",
    "tracker_record_move",
    "tracker_record_use",
    "add_loan",
    "deactivate_expired_loans",
    "solve_regions",
    "finalize_lifetimes",
    "place_overlap",
    "mark_moved",
    "mark_reinitialized",
)

REQUIRED_CONTRACTS = (
    "MoveConflictKind.MoveAfterMove",
    "MoveConflictKind.UseAfterMove",
    "LoanConflictKind.MutableWhileShared",
    "LoanConflictKind.SharedWhileMutable",
    "BorrowErrorKind.ReturnRefToLocal",
    "BorrowErrorKind.DanglingReference",
    "RegionConstraintKind.Outlives",
    "RegionConstraintKind.EndsBefore",
    "borrow_check_hir_mir",
    "diagnostics_has_fatal",
    "not diagnostics_has_fatal(current0.diagnostics)",
)


def sources() -> dict[str, str]:
    return {
        str(path.relative_to(ROOT)): path.read_text(encoding="utf-8")
        for path in sorted(BORROWCK.glob("*.vit"))
        if path.is_file()
    }


def pick_members(text: str, name: str) -> list[str] | None:
    match = re.search(rf"pick\s+{re.escape(name)}\s*\{{(?P<body>.*?)\}}", text, re.S)
    if not match:
        return None
    return [
        item
        for raw in match.group("body").splitlines()
        if (item := raw.strip().rstrip(",")) and re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", item)
    ]


def main() -> int:
    source_map = sources()
    bundle = "\n".join(source_map.values())
    pipeline = PIPELINE.read_text(encoding="utf-8")
    failures: list[str] = []

    for name, expected in EXPECTED_PICKS.items():
        found = None
        owner = ""
        for rel, text in source_map.items():
            members = pick_members(text, name)
            if members is not None:
                found, owner = members, rel
                break
        if found is None:
            failures.append(f"missing borrowck pick {name}")
        elif found != expected:
            failures.append(f"{owner}: {name} changed from {expected} to {found}")
    for proc in REQUIRED_PROCS:
        if not re.search(rf"\bproc\s+{re.escape(proc)}\s*\(", bundle):
            failures.append(f"missing borrowck procedure: {proc}")
    for contract in REQUIRED_CONTRACTS:
        if contract not in bundle:
            failures.append(f"missing borrowck contract: {contract}")

    if "borrow_check_hir_mir" not in pipeline and "borrow_check_hir_mir" not in bundle:
        failures.append("analysis pipeline does not expose the canonical HIR/MIR borrowck boundary")

    payload = {
        "schema": "vitte.compiler.borrowck-stability",
        "status": "error" if failures else "ok",
        "borrowck_files": len(source_map),
        "pick_families": len(EXPECTED_PICKS),
        "procedures": len(REQUIRED_PROCS),
        "failures": failures,
    }
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    MARKDOWN.write_text(
        "# Borrowck stability\n\n"
        f"- borrowck files: {payload['borrowck_files']}\n"
        f"- pick families: {payload['pick_families']}\n"
        f"- procedures: {payload['procedures']}\n"
        f"- status: {payload['status']}\n",
        encoding="utf-8",
    )
    if failures:
        for failure in failures:
            print(f"[borrowck-stability][error] {failure}", file=sys.stderr)
        return 1
    print(
        "[borrowck-stability] "
        f"borrowck_files={payload['borrowck_files']} picks={payload['pick_families']} "
        f"procedures={payload['procedures']} status=ok"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
