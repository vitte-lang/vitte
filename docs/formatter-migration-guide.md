# Formatter Migration Guide

This guide covers migration to the official Vitte formatter.

1. Run `python3 tools/vitte_format.py --check --changed` before committing.
2. Use `--edition 2026` for new code.
3. Use `--edition 2024` only when updating old snapshots.
4. Keep comments in place; the formatter must preserve them.
5. Keep import groups simple. Single-line `use` imports are sorted within each contiguous group.

CI runs `make formatter-gate`, which generates formatter snapshots and rejects unformatted changed Vitte files.
