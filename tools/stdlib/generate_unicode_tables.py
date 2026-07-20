#!/usr/bin/env python3

from __future__ import annotations

import hashlib
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
UCD_DIR = ROOT / "src/vitte/stdlib/tools/ucd"
OUTPUT = ROOT / "src/vitte/stdlib/generated/unicode_tables.vitl"
VERSION = "15.1.0"


def ucd_checksum() -> str:
    digest = hashlib.sha256()
    if not UCD_DIR.exists():
        digest.update(b"local-ucd-contract")
        return "sha256:" + digest.hexdigest()
    for path in sorted(UCD_DIR.glob("*.txt")):
        digest.update(path.name.encode("utf-8"))
        digest.update(path.read_bytes())
    return "sha256:" + digest.hexdigest()


def main() -> int:
    checksum = ucd_checksum()
    OUTPUT.write_text(
        "\n".join([
            "space vitte/stdlib/generated/unicode_tables",
            "",
            "export *",
            "",
            f"const GENERATED_UNICODE_VERSION: string = \"{VERSION}\"",
            "const GENERATED_UNICODE_SOURCE: string = \"Unicode Character Database\"",
            f"const GENERATED_UNICODE_CHECKSUM: string = \"{checksum}\"",
            "",
            "proc generated_unicode_version() -> string { give GENERATED_UNICODE_VERSION; }",
            "proc generated_unicode_source() -> string { give GENERATED_UNICODE_SOURCE; }",
            "proc generated_unicode_checksum() -> string { give GENERATED_UNICODE_CHECKSUM; }",
            "proc generated_unicode_category(value: rune) -> UnicodeCategory { give compiler_generated_unicode_category(value); }",
            "proc generated_unicode_properties(value: rune) -> UnicodeProperties { give compiler_generated_unicode_properties(value); }",
            "proc generated_unicode_normalization(form: NormalizationForm, value: rune) -> Iterator<rune> { give compiler_generated_unicode_normalization(form, value); }",
            "proc generated_unicode_case_fold(value: rune) -> Iterator<rune> { give compiler_generated_unicode_case_fold(value); }",
            "",
        ]),
        encoding="utf-8",
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
