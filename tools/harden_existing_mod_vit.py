#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
from pathlib import Path


ROLE_START = "<<< ROLE-CONTRACT"
ROLE_END = ">>>"


def pascal_case(name: str) -> str:
    parts = re.split(r"[/_\-]+", name)
    return "".join(p[:1].upper() + p[1:] for p in parts if p)


def diag_prefix(name: str) -> str:
    raw = re.sub(r"[^A-Za-z0-9]", "", name).upper()
    if not raw:
        raw = "PKG"
    return raw[:8]


def has_form(text: str, form_name: str) -> bool:
    return re.search(rf"^\s*form\s+{re.escape(form_name)}\b", text, flags=re.M) is not None


def has_pick(text: str, pick_name: str) -> bool:
    return re.search(rf"^\s*pick\s+{re.escape(pick_name)}\b", text, flags=re.M) is not None


def has_proc(text: str, proc_name: str) -> bool:
    return re.search(rf"^\s*proc\s+{re.escape(proc_name)}\s*\(", text, flags=re.M) is not None


def insert_before_role(text: str, block: str) -> str:
    idx = text.find(ROLE_START)
    if idx == -1:
        if not text.endswith("\n"):
            text += "\n"
        return text + "\n" + block + "\n"
    return text[:idx].rstrip() + "\n\n" + block + "\n\n" + text[idx:]


def parse_info_meta(info_path: Path) -> dict[str, str]:
    out: dict[str, str] = {}
    if not info_path.exists():
        return out
    for raw in info_path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if ":" not in line:
            continue
        k, v = line.split(":", 1)
        out[k.strip()] = v.strip()
    return out


def ensure_imports(text: str, pkg: str, alias_base: str) -> str:
    imports = [
        f"use vitte/{pkg}/internal/runtime as {alias_base}_runtime_pkg",
        f"use vitte/{pkg}/internal/validate as {alias_base}_validate_pkg",
        f"use vitte/{pkg}/internal/policy as {alias_base}_policy_pkg",
    ]
    missing = [imp for imp in imports if imp not in text]
    if not missing:
        return text

    m = re.search(r"^\s*space\s+vitte/[^\n]+\n", text, flags=re.M)
    if not m:
        return text
    insert_at = m.end()
    ins = "\n" + "\n".join(missing) + "\n"
    return text[:insert_at] + ins + text[insert_at:]


def ensure_role_fields(text: str, meta: dict[str, str]) -> str:
    start = text.find(ROLE_START)
    if start == -1:
        return text
    end = text.find(ROLE_END, start)
    if end == -1:
        return text

    block = text[start:end]
    add_lines: list[str] = []
    required = [
        ("owner", meta.get("owner", "@vitte/platform")),
        ("stability", meta.get("stability", "stable")),
        ("since", meta.get("since", "3.0.0")),
        ("deprecated_in", meta.get("deprecated_in", "-")),
        ("compat_policy", "additive-only"),
        ("api_version", "v1"),
    ]
    for key, default in required:
        if re.search(rf"^\s*{re.escape(key)}\s*:", block, flags=re.M) is None:
            add_lines.append(f"{key}: {default}")

    if not add_lines:
        return text

    insert = "\n" + "\n".join(add_lines) + "\n"
    return text[:end] + insert + text[end:]


def ensure_role_block(text: str, pkg: str, meta: dict[str, str]) -> str:
    if ROLE_START in text:
        return ensure_role_fields(text, meta)
    owner = meta.get("owner", "@vitte/platform")
    stability = meta.get("stability", "stable")
    since = meta.get("since", "3.0.0")
    deprecated = meta.get("deprecated_in", "-")
    block = f"""<<< ROLE-CONTRACT
package: vitte/{pkg}
owner: {owner}
stability: {stability}
since: {since}
deprecated_in: {deprecated}
role: Facade publique stable
input_contract: Config explicite et typable
output_contract: Resultats stables et predictibles
boundary: Implementation privee sous vitte/{pkg}/internal/*
compat_policy: additive-only
api_version: v1
>>>"""
    if not text.endswith("\n"):
        text += "\n"
    return text + "\n" + block + "\n"


def ensure_internal_files(pkg_dir: Path, pkg: str, alias_base: str) -> None:
    internal = pkg_dir / "internal"
    internal.mkdir(parents=True, exist_ok=True)

    runtime = internal / "runtime.vit"
    if not runtime.exists():
        runtime.write_text(
            f"""space vitte/{pkg}/internal/runtime

proc runtime_status() -> bool {{
  give true
}}
""",
            encoding="utf-8",
        )

    validate = internal / "validate.vit"
    if not validate.exists():
        validate.write_text(
            f"""space vitte/{pkg}/internal/validate

proc valid_timeout_ms(timeout_ms: int) -> bool {{
  give timeout_ms > 0 && timeout_ms <= 60000
}}

proc valid_retry_budget(retry_budget: int) -> bool {{
  give retry_budget >= 0 && retry_budget <= 32
}}
""",
            encoding="utf-8",
        )

    policy = internal / "policy.vit"
    if not policy.exists():
        policy.write_text(
            f"""space vitte/{pkg}/internal/policy

proc default_timeout_ms() -> int {{
  give 1000
}}

proc default_retry_budget() -> int {{
  give 1
}}
""",
            encoding="utf-8",
        )


def harden_mod(mod_path: Path, pkg: str, meta: dict[str, str]) -> tuple[str, bool]:
    text = mod_path.read_text(encoding="utf-8")
    original = text
    base = pascal_case(pkg)
    alias_base = pkg.replace("/", "_")
    pfx = diag_prefix(pkg)

    text = ensure_imports(text, pkg, alias_base)

    config_t = f"{base}Config"
    error_t = f"{base}Error"
    result_t = f"{base}Result"

    if not has_form(text, config_t):
        text = insert_before_role(
            text,
            f"""form {config_t} {{
  strict: bool
  timeout_ms: int
  retry_budget: int
}}""",
        )

    if not has_pick(text, error_t):
        text = insert_before_role(
            text,
            f"""pick {error_t} {{
  InvalidConfig
  Unsupported
  Timeout
  Internal
}}""",
        )

    if not has_pick(text, result_t):
        text = insert_before_role(
            text,
            f"""pick {result_t}[T] {{
  Ok(value: T)
  Err(error: {error_t})
}}""",
        )

    if not has_proc(text, "default_config"):
        text = insert_before_role(
            text,
            f"""proc default_config() -> {config_t} {{
  give {config_t}(true, {alias_base}_policy_pkg.default_timeout_ms(), {alias_base}_policy_pkg.default_retry_budget())
}}""",
        )

    if not has_proc(text, "validate_config"):
        text = insert_before_role(
            text,
            f"""proc validate_config(cfg: {config_t}) -> {result_t}[bool] {{
  if {alias_base}_validate_pkg.valid_timeout_ms(cfg.timeout_ms) == false {{
    give {result_t}.Err({error_t}.InvalidConfig)
  }}
  if {alias_base}_validate_pkg.valid_retry_budget(cfg.retry_budget) == false {{
    give {result_t}.Err({error_t}.InvalidConfig)
  }}
  give {result_t}.Ok(true)
}}""",
        )

    if not has_proc(text, "healthcheck"):
        text = insert_before_role(
            text,
            f"""proc healthcheck(cfg: {config_t}) -> {result_t}[bool] {{
  let vr: {result_t}[bool] = validate_config(cfg)
  when vr is {result_t}.Err {{
    give {result_t}.Err(vr.error)
  }}
  if {alias_base}_runtime_pkg.runtime_status() == false {{
    give {result_t}.Err({error_t}.Internal)
  }}
  give {result_t}.Ok(true)
}}""",
        )

    if not has_proc(text, "version"):
        text = insert_before_role(
            text,
            """proc version() -> string {
  give "v1"
}""",
        )

    if not has_proc(text, "capabilities"):
        text = insert_before_role(
            text,
            """proc capabilities() -> [string] {
  give ["config", "validate", "healthcheck"]
}""",
        )

    if not has_proc(text, "diagnostics_message"):
        text = insert_before_role(
            text,
            f"""proc diagnostics_message(code: string) -> string {{
  if code == "VITTE-{pfx}0001" {{
    give "invalid package configuration"
  }}
  give "{pkg} diagnostic " + code
}}""",
        )

    if not has_proc(text, "diagnostics_quickfix"):
        text = insert_before_role(
            text,
            """proc diagnostics_quickfix(code: string) -> string {
  let _ = code
  give "review package configuration"
}""",
        )

    if not has_proc(text, "diagnostics_doc_url"):
        text = insert_before_role(
            text,
            """proc diagnostics_doc_url(code: string) -> string {
  give "https://docs.vitte.dev/diagnostics/" + code
}""",
        )

    if not has_proc(text, "package_meta"):
        text = insert_before_role(
            text,
            f"""proc package_meta() -> string {{
  give "vitte/{pkg}"
}}""",
        )

    text = ensure_role_block(text, pkg, meta)
    return text, text != original


def main() -> int:
    ap = argparse.ArgumentParser(description="Harden existing mod.vit files with missing standard blocks")
    ap.add_argument("packages", nargs="+", help="package names under src/vitte/packages (e.g. kernel std/io)")
    ap.add_argument("--write", action="store_true", help="write changes (default: dry-run)")
    args = ap.parse_args()

    repo = Path(__file__).resolve().parents[1]
    root = repo / "src" / "vitte" / "packages"
    changed = 0

    for pkg in args.packages:
        pkg_dir = root / pkg
        mod = pkg_dir / "mod.vit"
        info = pkg_dir / "info.vit"
        if not mod.exists():
            print(f"[harden-mod][warn] missing mod.vit for package: {pkg}")
            continue

        meta = parse_info_meta(info)
        alias_base = pkg.replace("/", "_")
        ensure_internal_files(pkg_dir, pkg, alias_base)
        new_text, is_changed = harden_mod(mod, pkg, meta)
        if is_changed:
            changed += 1
            if args.write:
                mod.write_text(new_text, encoding="utf-8")
                print(f"[harden-mod] updated {mod.relative_to(repo)}")
            else:
                print(f"[harden-mod] would update {mod.relative_to(repo)}")
        else:
            print(f"[harden-mod] already hardened {mod.relative_to(repo)}")

    print(f"[harden-mod] packages changed: {changed}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
