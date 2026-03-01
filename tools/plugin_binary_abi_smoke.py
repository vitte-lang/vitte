#!/usr/bin/env python3
from __future__ import annotations

import ctypes
import json
import os
import platform
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "plugins/vitte_analyzer_pack.cpp"
OUT = ROOT / "target/reports/plugin_binary_abi.json"


class VitteIdePluginV1(ctypes.Structure):
    _fields_ = [
        ("abi_version", ctypes.c_int),
        ("plugin_name", ctypes.c_char_p),
        ("commands_csv", ctypes.c_void_p),
        ("run_command", ctypes.c_void_p),
        ("provide_completion", ctypes.c_void_p),
    ]


def lib_path() -> Path:
    ext = ".dylib" if platform.system().lower() == "darwin" else ".so"
    return ROOT / "target/plugins" / f"libvitte_analyzer_pack{ext}"


def main() -> int:
    OUT.parent.mkdir(parents=True, exist_ok=True)
    (ROOT / "target/plugins").mkdir(parents=True, exist_ok=True)
    lp = lib_path()

    cmd = ["c++", "-std=c++17", "-shared", "-fPIC", "-O2", str(SRC), "-o", str(lp)]
    build = subprocess.run(cmd, capture_output=True, text=True)
    data = {
        "schema_version": "1.0",
        "build_cmd": cmd,
        "build_rc": build.returncode,
        "build_stdout": build.stdout[-4000:],
        "build_stderr": build.stderr[-4000:],
        "library": str(lp.relative_to(ROOT)),
    }

    if build.returncode != 0:
        OUT.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        print("[plugin-binary-abi][error] build failed")
        return 1

    lib = ctypes.CDLL(str(lp))
    fn = lib.vitte_ide_plugin_v1
    fn.restype = ctypes.POINTER(VitteIdePluginV1)
    ptr = fn()
    if not ptr:
        data["load_error"] = "null plugin pointer"
        OUT.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        print("[plugin-binary-abi][error] null plugin pointer")
        return 1

    obj = ptr.contents
    name = obj.plugin_name.decode("utf-8", errors="replace") if obj.plugin_name else ""
    data["abi_version"] = int(obj.abi_version)
    data["plugin_name"] = name
    data["symbols_ok"] = bool(obj.commands_csv and obj.run_command and obj.provide_completion)

    ok = obj.abi_version == 1 and bool(name) and data["symbols_ok"]
    data["pass"] = ok
    OUT.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    if ok:
        print(f"[plugin-binary-abi] OK name={name}")
        return 0
    print("[plugin-binary-abi][error] ABI mismatch or missing symbols")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
