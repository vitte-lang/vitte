#!/usr/bin/env python3
from __future__ import annotations

import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
VITTE_ABI = ROOT / "src" / "vitte" / "stdlib" / "io" / "host_runtime.vitl"
RUNTIME_DIR = ROOT / "src" / "vitte" / "compiler" / "backends" / "runtime_c"
HEADER = RUNTIME_DIR / "vitte_runtime.h"
IMPLEMENTATION = RUNTIME_DIR / "vitte_runtime.c"
REPORT = ROOT / "target" / "reports" / "runtime_abi_contract.json"

TYPE_MAP = {
    "bool": "int32_t",
    "int": "int32_t",
    "string": "VitteString",
    "[string]": "VitteSliceString",
}


def split_parameters(parameters: str) -> list[str]:
    if not parameters.strip() or parameters.strip() == "void":
        return []
    return [part.strip() for part in parameters.split(",")]


def parse_vitte() -> dict[str, tuple[str, list[str]]]:
    declarations: dict[str, tuple[str, list[str]]] = {}
    pattern = re.compile(r"^intrinsic\s+(vitte_host_[A-Za-z0-9_]+)\((.*)\)\s*->\s*([^;]+);$")
    for line in VITTE_ABI.read_text(encoding="utf-8").splitlines():
        match = pattern.match(line.strip())
        if match is None:
            continue
        name, raw_parameters, raw_return = match.groups()
        parameters = []
        for parameter in split_parameters(raw_parameters):
            _, separator, value_type = parameter.partition(":")
            if not separator:
                raise ValueError(f"invalid Vitte ABI parameter for {name}: {parameter}")
            parameters.append(TYPE_MAP.get(value_type.strip(), value_type.strip()))
        return_type = TYPE_MAP.get(raw_return.strip(), raw_return.strip())
        declarations[name] = (return_type, parameters)
    return declarations


def parse_c_declarations(text: str) -> dict[str, tuple[str, list[str]]]:
    declarations: dict[str, tuple[str, list[str]]] = {}
    pattern = re.compile(r"^([A-Za-z_][A-Za-z0-9_]*(?:\s*\*)?)\s+(vitte_host_[A-Za-z0-9_]+)\(([^;{}]*)\)\s*[;{]$")
    for line in text.splitlines():
        match = pattern.match(line.strip())
        if match is None:
            continue
        return_type, name, raw_parameters = match.groups()
        parameters = []
        for parameter in split_parameters(raw_parameters):
            parameter_type, _, _ = parameter.rpartition(" ")
            parameters.append(parameter_type.strip())
        declarations[name] = (return_type.replace(" ", ""), parameters)
    return declarations


def normalized(contract: tuple[str, list[str]]) -> tuple[str, list[str]]:
    return contract[0].replace(" ", ""), [item.replace(" ", "") for item in contract[1]]


def main() -> int:
    vitte = parse_vitte()
    header = parse_c_declarations(HEADER.read_text(encoding="utf-8"))
    implementation = parse_c_declarations(IMPLEMENTATION.read_text(encoding="utf-8"))
    failures: list[str] = []

    for name in sorted(set(vitte) | set(header)):
        if name not in vitte:
            failures.append(f"header-only symbol: {name}")
        elif name not in header:
            failures.append(f"Vitte-only symbol: {name}")
        elif normalized(vitte[name]) != normalized(header[name]):
            failures.append(f"signature mismatch: {name}: Vitte={vitte[name]} C={header[name]}")

    for name in sorted(header):
        if name not in implementation:
            failures.append(f"missing C implementation: {name}")
        elif normalized(header[name]) != normalized(implementation[name]):
            failures.append(f"definition mismatch: {name}: header={header[name]} C={implementation[name]}")

    payload = {
        "schema": "vitte.runtime_abi_contract",
        "schema_version": "1.0.0",
        "status": "pass" if not failures else "fail",
        "symbol_count": len(vitte),
        "symbols": sorted(vitte),
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    if failures:
        for failure in failures:
            print(f"[runtime-abi-contract][error] {failure}")
        return 1
    print(f"[runtime-abi-contract] OK symbols={len(vitte)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
