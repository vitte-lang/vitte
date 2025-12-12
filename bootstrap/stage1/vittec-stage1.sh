#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parent
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

try:
    from vittec_stage1 import main
except Exception as exc:  # pragma: no cover - garde-fou minimal
    print(f"[vittec-stage1] wrapper Python indisponible: {exc}", file=sys.stderr)
    sys.exit(2)


if __name__ == "__main__":
    sys.exit(main())
