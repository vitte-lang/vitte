#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
BIN = ROOT / "bin" / "vitte"


CASES = [
    ("ascii", "check", "space matrix/ascii\n\nproc main() -> int {\n  give missing_ascii\n}\n", "SEMA_E_UNKNOWN_IDENTIFIER"),
    ("accents", "check", "space matrix/accents\n// commentaire accentué: déjà vu\nproc main() -> int {\n  give missing_accent\n}\n", "SEMA_E_UNKNOWN_IDENTIFIER"),
    ("unicode", "check", "space matrix/unicode\n// unicode: 値 λ 🚀\nproc main() -> int {\n  give missing_unicode\n}\n", "SEMA_E_UNKNOWN_IDENTIFIER"),
    ("tabs", "check", "space matrix/tabs\n\nproc main() -> int {\n\tgive missing_tab\n}\n", "SEMA_E_UNKNOWN_IDENTIFIER"),
    ("long-line", "check", "space matrix/long_line\n\nproc main() -> int {\n  give " + ("missing_" + "x" * 240) + "\n}\n", "LEX_E_TOKEN_TOO_LARGE"),
    ("empty", "parse", "", "E_BOOTSTRAP_EXPORT"),
    ("lf", "check", "space matrix/lf\n\nproc main() -> int {\n  give missing_lf\n}\n", "SEMA_E_UNKNOWN_IDENTIFIER"),
    ("crlf", "check", "space matrix/crlf\r\n\r\nproc main() -> int {\r\n  give missing_crlf\r\n}\r\n", "SEMA_E_UNKNOWN_IDENTIFIER"),
]


def check_case(tmp: Path, name: str, command: str, source: str, expected: str) -> None:
    path = tmp / f"{name}.vit"
    path.write_text(source, encoding="utf-8", newline="")
    proc = subprocess.run(
        [str(BIN), command, "--lang=en", str(path)],
        cwd=str(ROOT),
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    if proc.returncode == 0:
        raise SystemExit(f"[diagnostic-inputs][error] {name}: expected diagnostic failure")
    if expected not in proc.stdout:
        raise SystemExit(f"[diagnostic-inputs][error] {name}: missing {expected!r}\n{proc.stdout}")
    if "Traceback" in proc.stdout or "panic" in proc.stdout.lower():
        raise SystemExit(f"[diagnostic-inputs][error] {name}: diagnostic crashed\n{proc.stdout}")


def main() -> int:
    if not BIN.exists():
        raise SystemExit(f"[diagnostic-inputs][error] missing compiler binary: {BIN}")
    with tempfile.TemporaryDirectory(prefix="vitte-diagnostic-inputs.", dir=ROOT / "target") as raw:
        tmp = Path(raw)
        for name, command, source, expected in CASES:
            check_case(tmp, name, command, source, expected)
    print(f"[diagnostic-inputs] checked {len(CASES)} input shape(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
