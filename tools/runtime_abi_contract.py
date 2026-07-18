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
MANIFEST = ROOT / "toolchain" / "scripts" / "interop" / "vitte_c_abi_v1.json"
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
    pattern = re.compile(
        r"^((?:(?:static|const)\s+)*[A-Za-z_][A-Za-z0-9_]*)\s+(\**)[ \t]*"
        r"([A-Za-z_][A-Za-z0-9_]*)\(([^;{}]*)\)\s*[;{]$"
    )
    for line in text.splitlines():
        match = pattern.match(line.strip())
        if match is None:
            continue
        return_type, return_pointers, name, raw_parameters = match.groups()
        if return_type == "return":
            continue
        return_type += return_pointers
        parameters = []
        for parameter in split_parameters(raw_parameters):
            parameter_type, separator, parameter_name = parameter.rpartition(" ")
            if not separator:
                raise ValueError(f"invalid C ABI parameter for {name}: {parameter}")
            while parameter_name.startswith("*"):
                parameter_type += "*"
                parameter_name = parameter_name[1:]
            parameters.append(parameter_type.strip())
        declarations[name] = (return_type.replace(" ", ""), parameters)
    return declarations


def normalized(contract: tuple[str, list[str]]) -> tuple[str, list[str]]:
    return contract[0].replace(" ", ""), [item.replace(" ", "") for item in contract[1]]


def parse_c_structs(header_text: str) -> dict[str, list[tuple[str, str]]]:
    structs: dict[str, list[tuple[str, str]]] = {}
    pattern = re.compile(r"typedef\s+struct\s*\{(.*?)\}\s*([A-Za-z_][A-Za-z0-9_]*)\s*;", re.DOTALL)
    for body, name in pattern.findall(header_text):
        fields = []
        for declaration in body.split(";"):
            declaration = declaration.strip()
            if not declaration:
                continue
            field_type, separator, field_name = declaration.rpartition(" ")
            if not separator:
                raise ValueError(f"invalid C ABI field for {name}: {declaration}")
            while field_name.startswith("*"):
                field_type += "*"
                field_name = field_name[1:]
            fields.append((field_type.replace(" ", ""), field_name))
        structs[name] = fields
    return structs


def manifest_functions(manifest: dict[str, object]) -> dict[str, tuple[str, list[str]]]:
    functions: dict[str, tuple[str, list[str]]] = {}
    for function in manifest["functions"]:
        functions[function["name"]] = (
            function["ret"],
            [parameter["type"] for parameter in function["params"]],
        )
    return functions


def main() -> int:
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    header_text = HEADER.read_text(encoding="utf-8")
    vitte = parse_vitte()
    header_all = parse_c_declarations(header_text)
    implementation_all = parse_c_declarations(IMPLEMENTATION.read_text(encoding="utf-8"))
    header = {name: contract for name, contract in header_all.items() if name.startswith("vitte_host_")}
    implementation = {
        name: contract for name, contract in implementation_all.items() if name.startswith("vitte_host_")
    }
    manifest_abi = manifest_functions(manifest)
    header_structs = parse_c_structs(header_text)
    failures: list[str] = []

    for macro in manifest["macros"]:
        expected = f"#define {macro['name']} {macro['value']}"
        if expected not in header_text:
            failures.append(f"missing ABI version macro: {expected}")
    version_define = f'#define VITTE_C_ABI_VERSION "{manifest["version"]}"'
    if version_define not in header_text:
        failures.append(f"missing ABI version string: {version_define}")
    for struct in manifest["types"]:
        expected_fields = [(field["type"].replace(" ", ""), field["name"]) for field in struct["fields"]]
        actual_fields = header_structs.get(struct["name"])
        if actual_fields is None:
            failures.append(f"missing ABI type: {struct['name']}")
        elif actual_fields != expected_fields:
            failures.append(f"layout mismatch: {struct['name']}: manifest={expected_fields} C={actual_fields}")
    for name, expected in manifest_abi.items():
        if name not in header_all:
            failures.append(f"manifest function missing from header: {name}")
        elif normalized(expected) != normalized(header_all[name]):
            failures.append(f"manifest signature mismatch: {name}: manifest={expected} C={header_all[name]}")
        if name not in implementation_all:
            failures.append(f"manifest function missing from implementation: {name}")
        elif normalized(expected) != normalized(implementation_all[name]):
            failures.append(
                f"manifest definition mismatch: {name}: manifest={expected} C={implementation_all[name]}"
            )

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
        "abi_version": manifest["version"],
        "symbol_count": len(vitte),
        "layout_count": len(manifest["types"]),
        "manifest_function_count": len(manifest_abi),
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
