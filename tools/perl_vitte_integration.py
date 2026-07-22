#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import os
import re
import subprocess
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
PERL_ROOT = ROOT / "src" / "vitte" / "packages" / "perl"
INDEX = PERL_ROOT / "packages.json"
CONTRACT = PERL_ROOT / "integration.json"
OUT = ROOT / "target" / "vitte-perl-integration"
CACHE = OUT / "cache"
REPORTS = ROOT / "target" / "reports"
REPORT = REPORTS / "perl_vitte_integration.json"
REPORT_MD = REPORTS / "perl_vitte_integration.md"
VERSION = "vitte-perl-integration-1"


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def sha256_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def read_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def write_json(path: Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def run(cmd: list[str], *, cwd: Path, env: dict[str, str]) -> tuple[int, str]:
    proc = subprocess.run(
        cmd,
        cwd=cwd,
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    return proc.returncode, proc.stdout.strip()


def export_ok_symbols(path: Path) -> list[str]:
    text = path.read_text(encoding="utf-8")
    match = re.search(r"our\s+\@EXPORT_OK\s*=\s*qw\((.*?)\);", text, re.S)
    if not match:
        return []
    return match.group(1).split()


def package_files(pkgdir: Path) -> list[Path]:
    return sorted(path for path in pkgdir.rglob("*") if path.is_file())


def package_checksum(pkgdir: Path) -> str:
    h = hashlib.sha256()
    for path in package_files(pkgdir):
        rel = path.relative_to(pkgdir).as_posix()
        h.update(rel.encode("utf-8"))
        h.update(b"\0")
        h.update(sha256_file(path).encode("ascii"))
        h.update(b"\0")
    return h.hexdigest()


def typed_exports(exports: list[str], module: str) -> list[dict[str, str]]:
    return [
        {
            "abi": "perl-callable-v1",
            "module": module,
            "name": name,
            "type": "fn(dynamic...) -> dynamic",
        }
        for name in exports
    ]


def complete_package(entry: dict[str, Any]) -> bool:
    pkgdir = PERL_ROOT / entry["name"]
    return (
        pkgdir.is_dir()
        and (pkgdir / entry.get("meta", "META.json")).is_file()
        and (pkgdir / entry.get("module_file", "")).is_file()
        and (pkgdir / entry.get("test", "t/basic.t")).is_file()
    )


def compile_perl_package(entry: dict[str, Any]) -> tuple[dict[str, Any], list[str]]:
    name = entry["name"]
    pkgdir = PERL_ROOT / name
    module = entry["module"]
    module_file = pkgdir / entry["module_file"]
    lib = pkgdir / "lib"
    meta = read_json(pkgdir / entry.get("meta", "META.json"))
    exports = export_ok_symbols(module_file) or list(entry.get("exports", []))
    env = dict(os.environ)
    env["PERL5LIB"] = str(lib)

    diagnostics: list[str] = []
    compiled_ok = True
    checked_modules = []
    for pm in sorted(lib.rglob("*.pm")):
        code, output = run(["perl", "-c", str(pm)], cwd=ROOT, env=env)
        checked_modules.append(pm.relative_to(pkgdir).as_posix())
        if code != 0:
            compiled_ok = False
            diagnostics.append(f"{name}: perl -c failed for {pm.relative_to(pkgdir)}: {output}")

    code, output = run(
        [
            "perl",
            "-I",
            str(lib),
            "-M" + module,
            "-e",
            f"die unless ${module}::VERSION eq q({entry['version']}); print q(ok);",
        ],
        cwd=ROOT,
        env=env,
    )
    if code != 0:
        compiled_ok = False
        diagnostics.append(f"{name}: module load failed: {output}")

    cache_dir = CACHE / name
    cache_dir.mkdir(parents=True, exist_ok=True)
    export_rows = typed_exports(exports, module)
    source_hash = package_checksum(pkgdir)
    artifact = {
        "artifact_kind": "vitte-perl-package",
        "compiled": compiled_ok,
        "diagnostics_hash": sha256_text("\n".join(diagnostics)),
        "distribution": entry["distribution"],
        "exports": exports,
        "import_paths": [f"perl/{name}", module],
        "integration_version": VERSION,
        "lib_path": str(lib.resolve()),
        "module": module,
        "name": name,
        "package_root": str(pkgdir.resolve()),
        "perl5lib": str(lib.resolve()),
        "runtime": {
            "command": "perl",
            "minimum_perl": meta.get("prereqs", {}).get("runtime", {}).get("requires", {}).get("perl", "5.010"),
        },
        "source_checksum": source_hash,
        "typed_exports": export_rows,
        "version": entry["version"],
    }
    write_json(cache_dir / f"{name}.vitte-perl.json", artifact)
    write_json(cache_dir / "exports.typed.json", export_rows)
    (cache_dir / "diagnostics.txt").write_text("\n".join(diagnostics) + ("\n" if diagnostics else ""), encoding="utf-8")
    loader = (
        "#!/usr/bin/env perl\n"
        "use strict;\nuse warnings;\n"
        f"use lib q({lib.resolve()});\n"
        f"use {module} ();\n"
        f"print qq({module} {entry['version']}\\n);\n"
    )
    loader_path = cache_dir / "loader.pl"
    loader_path.write_text(loader, encoding="utf-8")
    loader_path.chmod(0o755)
    return artifact, diagnostics


def main() -> int:
    failures: list[str] = []
    if not INDEX.is_file():
        failures.append(f"missing {INDEX.relative_to(ROOT)}")
    if not CONTRACT.is_file():
        failures.append(f"missing {CONTRACT.relative_to(ROOT)}")
    if failures:
        for failure in failures:
            print(f"[perl-vitte-integration][error] {failure}", file=sys.stderr)
        return 1

    contract = read_json(CONTRACT)
    if contract.get("integration_version") != VERSION:
        failures.append("integration.json has wrong integration_version")

    index = read_json(INDEX)
    packages = index.get("packages", [])
    complete = [entry for entry in packages if complete_package(entry)]
    pending = [entry["name"] for entry in packages if not complete_package(entry)]
    if not complete:
        failures.append("no complete Perl packages available for Vitte integration")

    OUT.mkdir(parents=True, exist_ok=True)
    CACHE.mkdir(parents=True, exist_ok=True)
    REPORTS.mkdir(parents=True, exist_ok=True)

    registry_packages = []
    import_map: dict[str, dict[str, Any]] = {}
    all_typed_exports: dict[str, list[dict[str, str]]] = {}
    all_diagnostics: list[str] = []

    for entry in complete:
        artifact, diagnostics = compile_perl_package(entry)
        registry_packages.append(
            {
                "artifact": f"cache/{entry['name']}/{entry['name']}.vitte-perl.json",
                "exports_count": len(artifact["exports"]),
                "import_paths": artifact["import_paths"],
                "module": artifact["module"],
                "name": artifact["name"],
                "source_checksum": artifact["source_checksum"],
                "version": artifact["version"],
            }
        )
        for import_path in artifact["import_paths"]:
            import_map[import_path] = {
                "artifact": f"cache/{entry['name']}/{entry['name']}.vitte-perl.json",
                "module": artifact["module"],
                "package": artifact["name"],
                "perl5lib": artifact["perl5lib"],
            }
        all_typed_exports[entry["name"]] = artifact["typed_exports"]
        all_diagnostics.extend(diagnostics)

    registry = {
        "compiled_package_count": len(registry_packages),
        "integration_version": VERSION,
        "pending_declared_packages": pending,
        "perl_root": str(PERL_ROOT.resolve()),
        "packages": sorted(registry_packages, key=lambda item: item["name"]),
        "schema": "vitte.perl.integration.registry",
    }
    write_json(OUT / "registry.json", registry)
    write_json(OUT / "import_map.json", import_map)
    write_json(OUT / "typed_exports.json", all_typed_exports)
    (OUT / "diagnostics.txt").write_text("\n".join(all_diagnostics) + ("\n" if all_diagnostics else ""), encoding="utf-8")
    (OUT / "PERL5LIB.paths").write_text(
        "\n".join(str((PERL_ROOT / entry["name"] / "lib").resolve()) for entry in complete) + "\n",
        encoding="utf-8",
    )

    wrapper = OUT / "vitte-perl-run"
    wrapper.write_text(
        "#!/usr/bin/env sh\n"
        "set -eu\n"
        "if [ \"$#\" -lt 1 ]; then echo 'usage: vitte-perl-run MODULE [args...]' >&2; exit 64; fi\n"
        "module=\"$1\"\n"
        "shift\n"
        f"PERL5LIB=$(paste -sd: '{(OUT / 'PERL5LIB.paths').resolve()}')\n"
        "export PERL5LIB\n"
        "if [ \"$#\" -eq 0 ]; then exec perl -M\"$module\" -e '1'; fi\n"
        "exec perl -M\"$module\" \"$@\"\n",
        encoding="utf-8",
    )
    wrapper.chmod(0o755)

    report = {
        "compiled_package_count": len(registry_packages),
        "failures": failures,
        "integration_version": VERSION,
        "outputs": [str((OUT / name).relative_to(ROOT)) for name in ["registry.json", "import_map.json", "typed_exports.json", "diagnostics.txt", "PERL5LIB.paths", "vitte-perl-run"]],
        "pending_declared_packages": pending,
        "schema": "vitte.perl.integration.report",
    }
    write_json(REPORT, report)
    REPORT_MD.write_text(
        "# Perl Vitte Integration\n\n"
        f"- Integration version: `{VERSION}`\n"
        f"- Compiled Perl packages: {len(registry_packages)}\n"
        f"- Pending declared packages: {len(pending)}\n"
        "- Outputs: registry, import map, typed exports, diagnostics, PERL5LIB paths, loader wrapper\n",
        encoding="utf-8",
    )

    if failures:
        for failure in failures:
            print(f"[perl-vitte-integration][error] {failure}", file=sys.stderr)
        return 1

    print(f"[perl-vitte-integration] OK compiled={len(registry_packages)} pending={len(pending)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
