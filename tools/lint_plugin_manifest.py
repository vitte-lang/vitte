#!/usr/bin/env python3
from __future__ import annotations

import json
import re
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "plugins/plugin.toml"
SCHEMA_V1 = ROOT / "plugins/abi/plugin_payload_schema_v1.json"
SCHEMA_V2 = ROOT / "plugins/abi/plugin_payload_schema_v2.json"


def commands_from_cpp(path: Path) -> set[str]:
    if not path.exists():
        return set()
    txt = path.read_text(encoding="utf-8", errors="ignore")
    m = re.search(r"const char\* commands_csv\(\)\s*\{\s*return\s*\"(.*?)\";", txt, re.S)
    if not m:
        return set()
    raw = m.group(1).replace('"\n           "', "")
    return {x.strip() for x in raw.split(",") if x.strip()}


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

    lib = ROOT / str(data.get("library", ""))
    if not lib.exists():
        errs.append(f"manifest library missing: {lib}")

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

    cpp_cmds = commands_from_cpp(lib)
    if not cpp_cmds:
        errs.append("unable to parse commands_csv from plugin source")

    missing_in_cpp = sorted(declared - cpp_cmds)
    if missing_in_cpp:
        errs.append("commands declared in plugin.toml but not in commands_csv: " + ",".join(missing_in_cpp[:20]))

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
