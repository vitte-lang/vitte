#!/usr/bin/env python3
from __future__ import annotations

import json
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "plugins/plugin.toml"
SCHEMA_V1 = ROOT / "plugins/abi/plugin_payload_schema_v1.json"
SCHEMA_V2 = ROOT / "plugins/abi/plugin_payload_schema_v2.json"


def read_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8")) if path.exists() else {}


def main() -> int:
    errs = []
    if not MANIFEST.exists():
        print(f"[plugin-manifest][error] missing {MANIFEST}")
        return 1

    data = tomllib.loads(MANIFEST.read_text(encoding="utf-8"))
    if data.get("abi") not in {"v1", "v2"}:
        errs.append("manifest abi must be v1 or v2")

    manifest_cmds = data.get("command", [])
    if not isinstance(manifest_cmds, list) or not manifest_cmds:
        errs.append("manifest command entries missing")
        manifest_cmds = []

    declared = set()
    for c in manifest_cmds:
        name = c.get("name", "")
        sch = c.get("payload_schema", "")
        perms = c.get("permissions", [])
        if not name:
            errs.append("empty command name in manifest")
            continue
        declared.add(name)
        if sch not in {"v1", "v2"}:
            errs.append(f"{name}: payload_schema must be v1 or v2")
        if not isinstance(perms, list) or not perms:
            errs.append(f"{name}: permissions required")

    s1 = set(read_json(SCHEMA_V1).get("commands", []))
    s2 = set(read_json(SCHEMA_V2).get("commands", []))
    warns = []
    for c in manifest_cmds:
        name = c.get("name", "")
        sch = c.get("payload_schema", "")
        if sch == "v1" and name not in s1:
            warns.append(f"{name}: not present in payload schema v1")
        if sch == "v2" and name not in s2:
            warns.append(f"{name}: not present in payload schema v2")

    if errs:
        for e in errs:
            print(f"[plugin-manifest][error] {e}")
        return 1
    for w in warns:
        print(f"[plugin-manifest][warn] {w}")

    print(f"[plugin-manifest] OK commands={len(declared)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
