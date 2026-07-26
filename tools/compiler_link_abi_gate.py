#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import platform
import re
import subprocess
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
BIN = ROOT / "target/release/vitte"
COMPILER_ROOT = ROOT / "src/vitte/compiler"
MODULE_MANIFEST = COMPILER_ROOT / "modules.vitte.json"
ABI_SOURCE = ROOT / "src/vitte/compiler/backend/c/abi.vit"
ARCH_SOURCE = ROOT / "src/vitte/compiler/backend/c/architecture.vit"
LINK_SOURCE = ROOT / "src/vitte/compiler/backend/link/artifact.vit"
IR_SOURCE = ROOT / "src/vitte/compiler/backend/ir/ir.vit"
LOWER_SOURCE = ROOT / "src/vitte/compiler/middle/lower/mir_to_ir.vit"
LOCALE = ROOT / "locales/en/diagnostics.ftl"
REPORT_DIR = ROOT / "target/reports"
REPORT_JSON = REPORT_DIR / "compiler_link_abi_gate.json"
REPORT_MD = REPORT_DIR / "compiler_link_abi_gate.md"
ABI_MANIFEST = REPORT_DIR / "local_abi_manifest.json"
OUT_DIR = ROOT / "target/compiler-link-abi-gate"

TARGET_ABIS = {
    "x86_64-unknown-linux-gnu": {
        "abi_name": "vitte-c-abi-v1",
        "abi_schema_version": 1,
        "profile": "x86_64-linux-c11",
        "calling_convention": "sysv64",
        "pointer_width": 64,
        "stable": True,
    },
    "aarch64-apple-darwin": {
        "abi_name": "vitte-c-abi-v1",
        "abi_schema_version": 1,
        "profile": "aarch64-darwin-c11",
        "calling_convention": "aapcs64",
        "pointer_width": 64,
        "stable": True,
    },
    "x86_64-apple-darwin": {
        "abi_name": "vitte-c-abi-v1",
        "abi_schema_version": 1,
        "profile": "x86_64-darwin-c11",
        "calling_convention": "sysv64",
        "pointer_width": 64,
        "stable": True,
    },
    "x86_64-pc-windows-msvc": {
        "abi_name": "vitte-c-abi-v1",
        "abi_schema_version": 1,
        "profile": "x86_64-windows-c11",
        "calling_convention": "win64",
        "pointer_width": 64,
        "stable": True,
    },
}

REQUIRED_DIAGNOSTICS = {
    "MOD_E_MODULE_NOT_FOUND": "module not found",
    "MOD_E_SYMBOL_NOT_EXPORTED": "symbol is not exported by module",
    "BACKEND_E_ABI_MISMATCH": "abi mismatch",
    "MODULE_E_UNSUPPORTED_TARGET": "unsupported target",
}

REQUIRED_LINK_EVIDENCE = {
    "imports_resolved_from_manifest": [
        (MODULE_MANIFEST, '"module_resolution"'),
        (MODULE_MANIFEST, '"modules"'),
    ],
    "typed_exports_and_external_symbols": [
        (IR_SOURCE, "exports: [string]"),
        (IR_SOURCE, "external_symbols: [string]"),
        (IR_SOURCE, "module_add_export"),
        (IR_SOURCE, "module_add_external_symbol"),
    ],
    "compiler_module_link_symbols": [
        (LINK_SOURCE, "LinkSymbol"),
        (LINK_SOURCE, "LinkSymbolResolution"),
        (LINK_SOURCE, "resolved_import_count"),
        (LINK_SOURCE, "unresolved_import_count"),
        (LOWER_SOURCE, "module_add_external_symbol(built, lowered_fn.name)"),
    ],
}

REQUIRED_ABI_EVIDENCE = {
    "versioned_abi": [
        (ABI_SOURCE, 'C_ABI_NAME: string = "vitte-c-abi-v1"'),
        (ABI_SOURCE, "C_ABI_SCHEMA_VERSION: u64 = 1"),
        (ABI_SOURCE, "c_abi_contract(profile)"),
    ],
    "target_layout_and_calling_convention": [
        (ARCH_SOURCE, "target_layout_for(triple)"),
        (ARCH_SOURCE, "calling_convention_for_triple(triple)"),
        (ARCH_SOURCE, "pointer_width: layout.pointer_size * 8"),
    ],
    "unknown_or_unstable_target_refused": [
        (ARCH_SOURCE, "valid: false"),
        (ARCH_SOURCE, "abi_name: \"\""),
        (ABI_SOURCE, "c_abi_name_supported"),
        (ROOT / "src/vitte/compiler/backend/c/lowering.vit", "CBACKEND_E_ABI"),
    ],
}


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def clean_env() -> dict[str, str]:
    env = dict(os.environ)
    env.pop("VITTE_ROOT", None)
    env["VITTE_COMPILER"] = str(BIN)
    env["VITTE_PACKAGE_OFFLINE"] = "1"
    return env


def run(args: list[str]) -> dict[str, Any]:
    proc = subprocess.run(
        [str(BIN), *args],
        cwd=ROOT,
        env=clean_env(),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    return {"args": args, "exit_code": proc.returncode, "stdout": proc.stdout.strip(), "stderr": proc.stderr.strip()}


def file_kind(path: Path) -> str:
    proc = subprocess.run(["file", "-b", str(path)], cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, check=False)
    return proc.stdout.strip()


def load_modules(failures: list[str]) -> list[dict[str, Any]]:
    try:
        payload = json.loads(MODULE_MANIFEST.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        failures.append(f"compiler module manifest is invalid JSON: {exc}")
        return []
    modules = payload.get("modules")
    if not isinstance(modules, list):
        failures.append("compiler module manifest must contain modules list")
        return []
    return [item for item in modules if isinstance(item, dict)]


def validate_manifest_modules(failures: list[str]) -> dict[str, Any]:
    modules = load_modules(failures)
    paths = {str(item.get("path")) for item in modules if isinstance(item.get("path"), str)}
    discovered = {rel(path) for path in sorted(COMPILER_ROOT.rglob("*.vit"))}
    missing = sorted(discovered - paths)
    stale = sorted(paths - discovered)
    for path in missing:
        failures.append(f"compiler source missing from manifest: {path}")
    for path in stale:
        failures.append(f"compiler manifest stale path: {path}")
    return {"manifest_modules": len(paths), "compiler_sources": len(discovered), "missing": missing, "stale": stale}


def validate_evidence(groups: dict[str, list[tuple[Path, str]]], failures: list[str], label: str) -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    for name, checks in groups.items():
        missing: list[str] = []
        for path, needle in checks:
            if not path.is_file() or needle not in read(path):
                missing.append(f"{rel(path)}: {needle}")
        if missing:
            failures.append(f"{label} evidence missing for {name}: {', '.join(missing)}")
        rows.append({"name": name, "checks": len(checks), "missing": missing})
    return rows


def validate_stable_diagnostics(failures: list[str]) -> list[dict[str, Any]]:
    text = read(LOCALE)
    rows: list[dict[str, Any]] = []
    for code, phrase in REQUIRED_DIAGNOSTICS.items():
        pattern = f"{code} = {phrase}"
        present = pattern in text
        if not present:
            failures.append(f"missing stable diagnostic mapping: {pattern}")
        rows.append({"code": code, "expected": phrase, "present": present})
    return rows


def abi_manifest_payload() -> dict[str, Any]:
    abi_text = read(ABI_SOURCE)
    arch_text = read(ARCH_SOURCE)
    name_match = re.search(r'C_ABI_NAME: string = "([^"]+)"', abi_text)
    version_match = re.search(r"C_ABI_SCHEMA_VERSION: u64 = ([0-9]+)", abi_text)
    profile_match = re.search(r'name: "([^"]+)"', arch_text)
    local_target = local_target_triple()
    local_contract = TARGET_ABIS.get(local_target, {})
    return {
        "schema": "vitte.local.abi.manifest.v1",
        "abi_name": name_match.group(1) if name_match else "",
        "abi_schema_version": int(version_match.group(1)) if version_match else 0,
        "targets": TARGET_ABIS,
        "local": {
            "system": platform.system().lower(),
            "machine": platform.machine().lower(),
            "target": local_target,
            "executable_format": file_kind(BIN) if BIN.is_file() else "",
            "contract": local_contract,
        },
        "target_profile": local_contract.get("profile") or (profile_match.group(1) if profile_match else ""),
        "target_source": rel(ARCH_SOURCE),
        "abi_source": rel(ABI_SOURCE),
        "stable": bool(local_contract.get("stable") is True),
        "unknown_target_policy": "refuse",
        "unstable_target_policy": "refuse",
    }


def local_target_triple() -> str:
    system = platform.system().lower()
    machine = platform.machine().lower()
    if system == "darwin" and machine in {"arm64", "aarch64"}:
        return "aarch64-apple-darwin"
    if system == "darwin" and machine in {"x86_64", "amd64"}:
        return "x86_64-apple-darwin"
    if system == "linux" and machine in {"x86_64", "amd64"}:
        return "x86_64-unknown-linux-gnu"
    if system == "windows" and machine in {"x86_64", "amd64"}:
        return "x86_64-pc-windows-msvc"
    return f"unknown-{machine}-{system}"


def validate_abi_manifest(failures: list[str]) -> dict[str, Any]:
    payload = abi_manifest_payload()
    ABI_MANIFEST.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    if payload["abi_name"] != "vitte-c-abi-v1":
        failures.append(f"unexpected ABI name: {payload['abi_name']}")
    if payload["abi_schema_version"] != 1:
        failures.append(f"unexpected ABI schema version: {payload['abi_schema_version']}")
    if not payload["local"]["executable_format"]:
        failures.append("missing local executable format in ABI manifest")
    if not payload["target_profile"]:
        failures.append("missing target profile in ABI manifest")
    targets = payload.get("targets")
    if not isinstance(targets, dict) or not targets:
        failures.append("ABI manifest has no versioned target contracts")
    else:
        for target, contract in targets.items():
            if not isinstance(contract, dict):
                failures.append(f"ABI target contract is not an object: {target}")
                continue
            if contract.get("abi_name") != payload["abi_name"]:
                failures.append(f"ABI target {target} has wrong ABI name: {contract.get('abi_name')}")
            if contract.get("abi_schema_version") != payload["abi_schema_version"]:
                failures.append(f"ABI target {target} has wrong schema version: {contract.get('abi_schema_version')}")
            if contract.get("stable") is not True:
                failures.append(f"ABI target {target} is not stable")
            if not contract.get("profile") or not contract.get("calling_convention") or not contract.get("pointer_width"):
                failures.append(f"ABI target {target} is missing profile/calling convention/pointer width")
    local_target = payload["local"]["target"]
    if local_target not in TARGET_ABIS:
        failures.append(f"unknown local ABI target refused: {local_target}")
    if payload["stable"] is not True:
        failures.append(f"unstable local ABI target refused: {local_target}")
    return payload


def validate_unknown_target_refusal(failures: list[str]) -> dict[str, Any]:
    arch_text = read(ARCH_SOURCE)
    abi_text = read(ABI_SOURCE)
    checks = {
        "unknown_target_invalid_profile": 'abi_name: ""' in arch_text and "valid: false" in arch_text,
        "unsupported_abi_rejected": 'give "invalid";' in abi_text and "c_abi_name_supported" in abi_text,
        "invalid_contract_follows_profile": "valid: profile.valid" in abi_text or "and profile.abi_name == C_ABI_NAME" in abi_text,
    }
    for name, ok in checks.items():
        if not ok:
            failures.append(f"missing unknown/unstable ABI refusal evidence: {name}")
    return checks


def validate_multimodule_build(failures: list[str], commands: list[dict[str, Any]]) -> dict[str, Any]:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    source = "tests/compiler_snapshots/multifile/main.vit"
    output = OUT_DIR / "compiler-multimodule"
    check = run(["check", source])
    build = run(["build", source, "-o", rel(output)])
    commands.extend([check, build])
    if check["exit_code"] != 0:
        failures.append(f"multi-module compiler check failed: {check['stderr'] or check['stdout']}")
    if build["exit_code"] != 0 or not output.is_file():
        failures.append(f"multi-module compiler build failed: {build['stderr'] or build['stdout']}")
    kind = file_kind(output) if output.is_file() else ""
    if output.is_file() and not any(token in kind for token in ("Mach-O", "ELF", "PE32")):
        failures.append(f"multi-module compiler output is not native: {kind}")
    return {"source": source, "output": rel(output), "file_kind": kind}


def write_reports(
    status: str,
    manifest: dict[str, Any],
    link_evidence: list[dict[str, Any]],
    abi_evidence: list[dict[str, Any]],
    diagnostics: list[dict[str, Any]],
    abi_manifest: dict[str, Any],
    unknown_target_refusal: dict[str, Any],
    multimodule: dict[str, Any],
    commands: list[dict[str, Any]],
    failures: list[str],
) -> None:
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    payload = {
        "schema": "vitte.compiler.link.abi.gate.v1",
        "status": status,
        "manifest": manifest,
        "link_evidence": link_evidence,
        "abi_evidence": abi_evidence,
        "stable_diagnostics": diagnostics,
        "abi_manifest": rel(ABI_MANIFEST),
        "abi_manifest_payload": abi_manifest,
        "unknown_target_refusal": unknown_target_refusal,
        "multimodule": multimodule,
        "commands": commands,
        "failures": failures,
    }
    REPORT_JSON.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    lines = [
        "# compiler link ABI gate",
        "",
        f"- status: {status}",
        f"- manifest modules: {manifest.get('manifest_modules', 0)}",
        f"- compiler sources: {manifest.get('compiler_sources', 0)}",
        f"- ABI: {abi_manifest.get('abi_name', '')} schema={abi_manifest.get('abi_schema_version', '')}",
        f"- commands: {len(commands)}",
    ]
    if failures:
        lines.append("")
        lines.append("## Failures")
        lines.extend(f"- {failure}" for failure in failures)
    REPORT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    failures: list[str] = []
    commands: list[dict[str, Any]] = []
    manifest = validate_manifest_modules(failures)
    link_evidence = validate_evidence(REQUIRED_LINK_EVIDENCE, failures, "link")
    abi_evidence = validate_evidence(REQUIRED_ABI_EVIDENCE, failures, "ABI")
    diagnostics = validate_stable_diagnostics(failures)
    abi_manifest = validate_abi_manifest(failures)
    unknown_target_refusal = validate_unknown_target_refusal(failures)
    multimodule = validate_multimodule_build(failures, commands) if BIN.is_file() else {}
    if not BIN.is_file():
        failures.append("missing target/release/vitte")
    status = "fail" if failures else "pass"
    write_reports(
        status,
        manifest,
        link_evidence,
        abi_evidence,
        diagnostics,
        abi_manifest,
        unknown_target_refusal,
        multimodule,
        commands,
        failures,
    )
    if failures:
        print("[compiler-link-abi-gate][error] gate failed", file=sys.stderr)
        for failure in failures:
            print(f" - {failure}", file=sys.stderr)
        return 1
    print(f"[compiler-link-abi-gate] ok abi={abi_manifest['abi_name']} modules={manifest['manifest_modules']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
