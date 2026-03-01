#!/usr/bin/env python3
from __future__ import annotations

import json
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "plugins/plugin.toml"
POLICY = ROOT / "plugins/plugin_sandbox_permissions.json"


def main() -> int:
    errs = []
    if not MANIFEST.exists() or not POLICY.exists():
        print("[plugin-sandbox][error] missing plugin.toml or plugin_sandbox_permissions.json")
        return 1

    m = tomllib.loads(MANIFEST.read_text(encoding="utf-8"))
    p = json.loads(POLICY.read_text(encoding="utf-8"))

    allowed = set(p.get("allowed_permissions", []))
    cmd_policy = p.get("commands", {})

    for cmd in m.get("command", []):
        name = cmd.get("name", "")
        perms = set(cmd.get("permissions", []))
        if not name:
            errs.append("manifest command with empty name")
            continue
        if name not in cmd_policy:
            errs.append(f"{name}: missing in sandbox policy map")
            continue
        policy_perms = set(cmd_policy[name])
        if not perms.issubset(allowed):
            errs.append(f"{name}: unknown permissions in manifest")
        if not policy_perms.issubset(allowed):
            errs.append(f"{name}: unknown permissions in policy")
        if perms != policy_perms:
            errs.append(f"{name}: manifest permissions != policy permissions")
        if "process" in name and "process:exec" not in perms:
            errs.append(f"{name}: expected process:exec")
        if "http" in name and "net:outbound" not in perms and "doctor" in name:
            errs.append(f"{name}: expected net:outbound for http diagnostics")

    if errs:
        for e in errs:
            print(f"[plugin-sandbox][error] {e}")
        return 1
    print(f"[plugin-sandbox] OK commands={len(m.get('command', []))}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
