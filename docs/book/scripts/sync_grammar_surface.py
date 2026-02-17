#!/usr/bin/env python3
from pathlib import Path
import argparse


def main() -> int:
    parser = argparse.ArgumentParser(description="Sync grammar doc copies from src/vitte/grammar/vitte.ebnf")
    parser.add_argument("--check", action="store_true", help="fail if copies are out of date")
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[3]
    src = repo / "src/vitte/grammar/vitte.ebnf"
    copies = [
        repo / "docs/book/grammar-surface.ebnf",
        repo / "docs/grammar/vitte.ebnf",
    ]

    if not src.exists():
        print(f"[grammar-sync] missing source: {src}")
        return 1

    source_text = src.read_text(encoding="utf-8")
    mismatches = []

    for target in copies:
        if not target.exists() or target.read_text(encoding="utf-8") != source_text:
            mismatches.append(target)

    if args.check:
        if mismatches:
            print("[grammar-sync] FAILED")
            for path in mismatches:
                print(f"- out of sync: {path}")
            print(f"- source: {src}")
            return 1
        print("[grammar-sync] OK")
        return 0

    for target in copies:
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(source_text, encoding="utf-8")
        print(f"[grammar-sync] wrote {target}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
