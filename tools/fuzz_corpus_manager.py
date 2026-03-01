#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT_JSON = ROOT / "target/reports/fuzz_corpus.json"
OUT_TXT = ROOT / "target/reports/fuzz_corpus.report"
CORPUS_DIR = ROOT / "target/fuzz/corpus"


def sha256_bytes(data: bytes) -> str:
    h = hashlib.sha256()
    h.update(data)
    return h.hexdigest()


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--write-corpus", action="store_true")
    ap.add_argument("--minimize", action="store_true")
    args = ap.parse_args()

    seed_dirs = sorted((ROOT / "tests").glob("**/fuzz_seeds"))
    files = []
    for d in seed_dirs:
        files.extend([p for p in d.glob("*") if p.is_file()])

    by_hash: dict[str, list[Path]] = {}
    for p in files:
        b = p.read_bytes()
        by_hash.setdefault(sha256_bytes(b), []).append(p)

    minimized = {}
    for h, paths in by_hash.items():
        if args.minimize:
            chosen = min(paths, key=lambda p: p.stat().st_size)
        else:
            chosen = sorted(paths, key=lambda p: str(p))[0]
        minimized[h] = chosen

    if args.write_corpus:
        CORPUS_DIR.mkdir(parents=True, exist_ok=True)
        for h, p in minimized.items():
            out = CORPUS_DIR / f"{h[:16]}_{p.name}"
            out.write_bytes(p.read_bytes())

    crash_files = sorted((ROOT / "target").glob("**/*crash*"))
    triage = {}
    for p in crash_files:
        if not p.is_file():
            continue
        txt = p.read_text(encoding="utf-8", errors="ignore")
        sig = (txt.splitlines()[0] if txt else p.name)[:180]
        triage.setdefault(sig, []).append(str(p.relative_to(ROOT)))

    report = {
        "schema_version": "1.0",
        "seed_files": len(files),
        "seed_unique": len(minimized),
        "seed_duplicates": len(files) - len(minimized),
        "seed_dirs": [str(d.relative_to(ROOT)) for d in seed_dirs],
        "corpus_dir": str(CORPUS_DIR.relative_to(ROOT)),
        "crash_signatures": [{"signature": k, "count": len(v), "files": v} for k, v in sorted(triage.items())],
    }

    OUT_JSON.parent.mkdir(parents=True, exist_ok=True)
    OUT_JSON.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    lines = [
        "Fuzz Corpus Manager",
        f"seed_files={report['seed_files']}",
        f"seed_unique={report['seed_unique']}",
        f"seed_duplicates={report['seed_duplicates']}",
        f"crash_signatures={len(report['crash_signatures'])}",
    ]
    OUT_TXT.write_text("\n".join(lines) + "\n", encoding="utf-8")

    print(f"[fuzz-corpus] seeds={report['seed_files']} unique={report['seed_unique']} duplicates={report['seed_duplicates']}")
    print(f"[fuzz-corpus] wrote {OUT_JSON}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
