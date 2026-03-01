#!/usr/bin/env python3
from __future__ import annotations
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read_json(path: Path):
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception:
        return None


def main() -> int:
    p = ROOT / 'plugins/vitte_analyzer_pack.cpp'
    txt = p.read_text(encoding='utf-8') if p.exists() else ''
    errs = []
    for need in ['commands_csv', 'run_command', 'VitteIdePluginV1', 'vitte-std.doctor', 'vitte-process.doctor']:
        if need not in txt:
            errs.append(f'missing {need}')

    s1 = read_json(ROOT / 'plugins/abi/plugin_payload_schema_v1.json')
    s2 = read_json(ROOT / 'plugins/abi/plugin_payload_schema_v2.json')
    if not s1 or s1.get('schema_version') != 'v1':
        errs.append('invalid plugins/abi/plugin_payload_schema_v1.json')
    if not s2 or s2.get('schema_version') != 'v2':
        errs.append('invalid plugins/abi/plugin_payload_schema_v2.json')

    if s1 and s2:
        c1 = set(s1.get('commands', []))
        c2 = set(s2.get('commands', []))
        missing = sorted(c1 - c2)
        if missing:
            errs.append('v2 schema missing v1 commands: ' + ','.join(missing))

    if errs:
        for e in errs:
            print(f'[plugin-abi-compat][error] {e}')
        return 1
    print('[plugin-abi-compat] OK')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
