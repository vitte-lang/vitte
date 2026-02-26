#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import argparse
import re
from dataclasses import dataclass
from datetime import date


IMPORT_RE = re.compile(r"^\s*(use|pull)\s+([^\s]+)")
ENTRY_RE = re.compile(r"^(?P<key>[^#\s]+)(?:\s*#\s*(?P<meta>.*))?$")


@dataclass(frozen=True)
class AllowEntry:
    key: str
    expires: str
    issue: str


def load_allowlist(path: Path) -> list[AllowEntry]:
    if not path.exists():
        return []
    out: list[AllowEntry] = []
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        m = ENTRY_RE.match(line)
        if not m:
            continue
        key = (m.group("key") or "").strip()
        meta_raw = (m.group("meta") or "").strip()
        meta_parts = {}
        for token in meta_raw.split():
            if "=" not in token:
                continue
            k, v = token.split("=", 1)
            meta_parts[k.strip()] = v.strip()
        out.append(
            AllowEntry(
                key=key,
                expires=meta_parts.get("expires", ""),
                issue=meta_parts.get("issue", ""),
            )
        )
    return out


def path_has_self_leaf(import_path: str) -> bool:
    # Drop trailing import selectors like .*, .{...} and leading relative dots.
    base = import_path.lstrip(".")
    base = base.split(".{", 1)[0]
    base = base.split(".*", 1)[0]
    segments = [s for s in base.split("/") if s and s != "."]
    if len(segments) < 2:
        return False
    return segments[-1] == segments[-2]


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Forbid legacy self-leaf imports (/<pkg>/<pkg>) in tests/examples"
    )
    parser.add_argument(
        "--roots",
        nargs="+",
        default=["src", "tests", "examples", "book", "docs"],
        help="directories to scan",
    )
    parser.add_argument(
        "--allowlist",
        default="tools/legacy_import_path_allowlist.txt",
        help="optional allowlist file with entries path:line",
    )
    parser.add_argument(
        "--max-entries",
        type=int,
        default=5,
        help="max number of allowlist entries permitted for this release",
    )
    parser.add_argument(
        "--no-require-issue",
        action="store_true",
        help="do not require issue=<ticket> metadata on allowlist lines",
    )
    parser.add_argument(
        "--no-require-expires",
        action="store_true",
        help="do not require expires=YYYY-MM-DD metadata on allowlist lines",
    )
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    allow_entries = load_allowlist(repo / args.allowlist)
    allow = {e.key for e in allow_entries}
    allow_meta = {e.key: e for e in allow_entries}

    violations: list[str] = []
    matched_allowlist: set[str] = set()
    scanned = 0
    today = date.today()

    if len(allow_entries) > args.max_entries:
        violations.append(
            f"allowlist budget exceeded: {len(allow_entries)} > max {args.max_entries}"
        )

    for entry in allow_entries:
        if not args.no_require_issue and not entry.issue:
            violations.append(
                f"{entry.key}: missing issue=<ticket> metadata in allowlist"
            )
        if not args.no_require_expires:
            if not entry.expires:
                violations.append(
                    f"{entry.key}: missing expires=YYYY-MM-DD metadata in allowlist"
                )
            else:
                try:
                    exp = date.fromisoformat(entry.expires)
                    if exp < today:
                        violations.append(
                            f"{entry.key}: allowlist entry expired on {entry.expires}"
                        )
                except ValueError:
                    violations.append(
                        f"{entry.key}: invalid expires date '{entry.expires}' (expected YYYY-MM-DD)"
                    )

    for root in args.roots:
        root_path = (repo / root).resolve()
        if not root_path.exists():
            continue
        for file in sorted(root_path.rglob("*.vit")):
            scanned += 1
            rel = file.relative_to(repo).as_posix()
            for i, line in enumerate(file.read_text(encoding="utf-8").splitlines(), start=1):
                m = IMPORT_RE.match(line)
                if not m:
                    continue
                import_path = m.group(2)
                if not path_has_self_leaf(import_path):
                    continue
                key = f"{rel}:{i}"
                if key in allow:
                    matched_allowlist.add(key)
                    continue
                violations.append(
                    f"{key}: legacy import path '{import_path}' is forbidden (use canonical parent path)"
                )

    orphan_allow = sorted(allow - matched_allowlist)
    for orphan in orphan_allow:
        meta = allow_meta.get(orphan)
        suffix = f" (issue={meta.issue}, expires={meta.expires})" if meta else ""
        violations.append(
            f"{orphan}: allowlist entry is orphan (no matching legacy import line){suffix}"
        )

    print(f"[legacy-import-lint] scanned files: {scanned}")
    print(f"[legacy-import-lint] allowlist entries: {len(allow_entries)} (budget: {args.max_entries})")
    for v in violations:
        print(f"[legacy-import-lint][error] {v}")

    if violations:
        print(f"[legacy-import-lint] FAILED: {len(violations)} error(s)")
        return 1

    print("[legacy-import-lint] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
