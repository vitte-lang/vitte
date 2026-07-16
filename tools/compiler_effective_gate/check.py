#!/usr/bin/env python3
from collections import deque
from pathlib import Path
import os
import re
import subprocess
from typing import Dict, List, Optional, Set, Tuple

ROOT = Path(__file__).resolve().parents[2]
ACTIVE = ROOT / "tools/compiler_effective_gate/ACTIVE_FILES.txt"
CONTRACTS = ROOT / "tools/compiler_effective_gate/VERSIONED_CONTRACTS.txt"
TEST_OWNERS = ROOT / "tools/compiler_effective_gate/TEST_OWNERS.txt"
ENTRY = ROOT / "src/vitte/compiler/main.vit"
SEED = ROOT / "toolchain/seed/vittec0.seed"
WORKFLOW = ROOT / ".github/workflows/compiler-effective-gate.yml"
USE_RE = re.compile(r"^\s*use\s+(vitte/compiler/[A-Za-z0-9_/-]+)", re.MULTILINE)


def read_lines(path: Path) -> List[str]:
    return [
        line.strip()
        for line in path.read_text(encoding="utf-8").splitlines()
        if line.strip() and not line.lstrip().startswith("#")
    ]


def relative(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def resolve_module(module: str) -> Optional[Path]:
    base = ROOT / "src" / module
    direct = base.with_suffix(".vit")
    facade = base / "mod.vit"
    if direct.exists():
        return direct
    if facade.exists():
        return facade
    return None


def compiler_graph() -> Dict[Path, Set[Path]]:
    graph: Dict[Path, Set[Path]] = {}
    for source in (ROOT / "src/vitte/compiler").rglob("*.vit"):
        text = source.read_text(encoding="utf-8", errors="ignore")
        graph[source] = {
            target
            for module in USE_RE.findall(text)
            if (target := resolve_module(module)) is not None
        }
    return graph


def reachable_from_entry(graph: Dict[Path, Set[Path]]) -> Set[Path]:
    reached: Set[Path] = set()
    queue = deque([ENTRY])
    while queue:
        source = queue.popleft()
        if source in reached:
            continue
        reached.add(source)
        queue.extend(graph.get(source, set()) - reached)
    return reached


def read_test_owners(path: Path) -> Dict[str, Tuple[str, str]]:
    owners: Dict[str, Tuple[str, str]] = {}
    for line in read_lines(path):
        fields = [field.strip() for field in line.split("|")]
        if len(fields) != 3:
            raise ValueError(f"invalid test owner row: {line}")
        source, positive, negative = fields
        if source in owners:
            raise ValueError(f"duplicate test owner: {source}")
        owners[source] = (positive, negative)
    return owners


def direct_check(source: Path) -> Optional[str]:
    env = os.environ.copy()
    env["VITTE_ROOT"] = str(ROOT)
    result = subprocess.run(
        ["sh", str(SEED), "check", relative(source)],
        cwd=ROOT,
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    if result.returncode == 0:
        return None
    detail = result.stderr.strip() or result.stdout.strip() or f"exit {result.returncode}"
    return f"source check failed: {relative(source)}: {detail}"


def main() -> int:
    errors: List[str] = []
    active_names = read_lines(ACTIVE)
    contract_names = read_lines(CONTRACTS)

    if len(active_names) != len(set(active_names)):
        errors.append("ACTIVE_FILES.txt contains duplicate paths")
    if len(contract_names) != len(set(contract_names)):
        errors.append("VERSIONED_CONTRACTS.txt contains duplicate paths")

    active = [ROOT / name for name in active_names]
    contracts = set(contract_names)
    active_set = set(active_names)

    if active_set != contracts:
        for name in sorted(active_set - contracts):
            errors.append(f"missing versioned contract registration: {name}")
        for name in sorted(contracts - active_set):
            errors.append(f"contract registered for non-active implementation: {name}")

    try:
        test_owners = read_test_owners(TEST_OWNERS)
    except (OSError, ValueError) as exc:
        errors.append(str(exc))
        test_owners = {}

    for name in sorted(active_set - set(test_owners)):
        errors.append(f"missing test owner registration: {name}")
    for name in sorted(set(test_owners) - active_set):
        errors.append(f"test owner registered for non-active implementation: {name}")

    if not ENTRY.exists():
        errors.append(f"missing compiler entry: {relative(ENTRY)}")
    if not SEED.exists():
        errors.append(f"missing portable seed checker: {relative(SEED)}")
    if not WORKFLOW.exists():
        errors.append(f"missing CI workflow: {relative(WORKFLOW)}")

    graph = compiler_graph()
    reachable = reachable_from_entry(graph)
    for source in active:
        name = relative(source)
        if source.name == "mod.vit":
            errors.append(f"facade registered as active implementation: {name}")
        if not source.exists():
            errors.append(f"missing active implementation: {name}")
            continue
        if source.stat().st_size == 0:
            errors.append(f"empty active implementation: {name}")
        if source not in reachable:
            errors.append(f"not reachable from {relative(ENTRY)}: {name}")

        owner = test_owners.get(name)
        if owner is not None:
            for kind, test_name in zip(("positive", "negative"), owner):
                test_path = ROOT / test_name
                if not test_path.exists() or test_path.stat().st_size == 0:
                    errors.append(f"missing {kind} test for {name}: {test_name}")

        if SEED.exists():
            check_error = direct_check(source)
            if check_error is not None:
                errors.append(check_error)

    observable_contracts = {
        "src/vitte/compiler/driver/compile.vit": ("diagnostics: [DriverDiagnostic]",),
        "src/vitte/compiler/driver/pipeline.vit": ("pipeline_failed_at: string", "stage_logs: [string]"),
    }
    for name, needles in observable_contracts.items():
        path = ROOT / name
        text = path.read_text(encoding="utf-8", errors="ignore") if path.exists() else ""
        for needle in needles:
            if needle not in text:
                errors.append(f"missing observable pipeline contract in {name}: {needle}")

    if errors:
        print("compiler-effective-gate: FAIL")
        for error in errors:
            print(" -", error)
        return 1

    print(
        "compiler-effective-gate: PASS "
        f"active={len(active)} reachable={len(reachable)} direct_checks={len(active)}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
