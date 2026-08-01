#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
EN_FTL = ROOT / "locales" / "en" / "diagnostics.ftl"
CATALOG = ROOT / "src/vitte/compiler/infrastructure/diagnostics/fluent_catalog.vit"

REQUIRED_CODES: dict[str, str] = {
    "DRIVER_E_MISSING_COMMAND": "missing compiler command",
    "DRIVER_E_MISSING_OPTION_VALUE": "missing compiler option value",
    "DRIVER_E_SOURCE_UNREADABLE": "missing or unreadable input source",
    "DRIVER_E_INVALID_CONFIGURATION": "invalid compiler configuration",
    "DRIVER_E_MISSING_TARGET": "missing compiler target",
    "DRIVER_E_MISSING_OUTPUT_DIR": "missing compiler output directory",
    "DRIVER_E_MISSING_OUTPUT_PATH": "missing compiler output path",
    "DRIVER_E_INVALID_LINK_TARGET": "invalid compiler link target",
    "INSTALL_E_VITTE_HELP_FAILED": "installed vitte help command failed",
    "INSTALL_E_VITTEC_HELP_FAILED": "installed vittec help command failed",
    "INSTALL_E_PACKAGE_IMPORT_TEST_FAILED": "installed package import test failed",
    "INSTALL_E_COMPLETION_MISSING": "installed shell completion is missing",
    "INSTALL_E_CC_MISSING": "macOS package build requires cc",
    "INSTALL_E_LIPO_MISSING": "universal macOS package build requires lipo",
    "INSTALL_E_MACOS_REQUIRED": "macOS package build requires macOS",
    "INSTALL_E_PACKAGE_SOURCE_MISSING": "package source file is missing",
    "INSTALL_E_POSTINSTALL_HELP_FAILED": "postinstall vitte help check failed",
    "INSTALL_E_POSTINSTALL_IMPORT_TEST_FAILED": "postinstall package import check failed",
    "INSTALL_E_POSTINSTALL_COMPLETION_MISSING": "postinstall shell completion check failed",
}

SURFACE_NEEDLES: dict[Path, tuple[str, ...]] = {
    ROOT / "src/vitte/compiler/driver/compiler.vit": (
        "compiler_diagnostic_cli_error(",
        'compiler_diagnostic_io_error("E_IO_FILE_NOT_FOUND"',
        'compiler_diagnostic_io_error("E_IO_FILE_UNREADABLE"',
        "compiler_diagnostic_linker_error(",
        "compiler_diagnostic_render(",
        "request.error_format",
        "request.lang",
    ),
    ROOT / "toolchain/scripts/package/make-macos-pkg.sh": (
        "vitte --help failed",
        "vittec --help failed",
        "vitte check package import test failed",
        "missing completion ",
    ),
    ROOT / "toolchain/scripts/package/make-debian-deb.sh": (
        "postinst check failed: vitte --help",
        "postinst check failed: package import test",
        "postinst check failed: missing completion ",
    ),
    ROOT / "Makefile": (
        "target requires macOS",
        "missing cc",
        "missing lipo",
        "missing source ",
    ),
}


def parse_ftl(path: Path) -> dict[str, str]:
    entries: dict[str, str] = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        if raw.startswith("#") or "=" not in raw:
            continue
        key, value = raw.split("=", 1)
        entries[key.strip()] = value.strip()
    return entries


def main() -> int:
    failures: list[str] = []
    en = parse_ftl(EN_FTL)
    catalog = CATALOG.read_text(encoding="utf-8")

    for code, expected in REQUIRED_CODES.items():
        actual = en.get(code)
        if actual != expected:
            failures.append(f"{EN_FTL.relative_to(ROOT)} missing {code} = {expected!r}")
        if re.search(rf'if code == "{re.escape(code)}"\s*\{{\s*give fluent_diagnostic_fields\(', catalog) is None:
            failures.append(f"{CATALOG.relative_to(ROOT)} missing generated lookup for {code}")

    for path, needles in SURFACE_NEEDLES.items():
        text = path.read_text(encoding="utf-8")
        for needle in needles:
            if needle not in text:
                failures.append(f"{path.relative_to(ROOT)} no longer contains covered surface {needle!r}")

    if failures:
        print("[compiler-install-fluent-coverage][error] coverage failed", file=sys.stderr)
        for failure in failures:
            print(f"  - {failure}", file=sys.stderr)
        return 1

    print(f"[compiler-install-fluent-coverage] codes={len(REQUIRED_CODES)} surfaces={sum(len(v) for v in SURFACE_NEEDLES.values())} status=ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
