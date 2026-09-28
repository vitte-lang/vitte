#!/usr/bin/env python3
"""Backward-compatible wrapper for grammar sync."""

from __future__ import annotations

import runpy
from pathlib import Path


if __name__ == "__main__":
    script = Path(__file__).resolve().parents[1] / "grammar" / "scripts" / "sync_grammar.py"
    runpy.run_path(str(script), run_name="__main__")
