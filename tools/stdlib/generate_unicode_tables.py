#!/usr/bin/env python3

from __future__ import annotations

import argparse
import hashlib
import json
import zipfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
VERSION = "15.1.0"
SOURCE_URL = "https://www.unicode.org/Public/zipped/15.1.0/UCD.zip"
UCD_ROOT = ROOT / "src/vitte/stdlib/tools/ucd" / VERSION
OUTPUT = ROOT / "src/vitte/stdlib/generated/unicode_tables.vitl"
MANIFEST = UCD_ROOT / "manifest.json"
REQUIRED_FILES = (
    "UnicodeData.txt",
    "DerivedCoreProperties.txt",
    "PropList.txt",
    "CaseFolding.txt",
    "SpecialCasing.txt",
    "DerivedNormalizationProps.txt",
)

CATEGORY_CODES = {
    "Lu": 0, "Ll": 1, "Lt": 2, "Lm": 3, "Lo": 4,
    "Mn": 5, "Mc": 6, "Me": 7, "Nd": 8, "Nl": 9, "No": 10,
    "Pc": 11, "Pd": 12, "Ps": 13, "Pe": 14, "Pi": 15, "Pf": 16, "Po": 17,
    "Sm": 18, "Sc": 19, "Sk": 20, "So": 21,
    "Zs": 22, "Zl": 23, "Zp": 24, "Cc": 25, "Cf": 26, "Cs": 27, "Co": 28, "Cn": 29,
}


def digest_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def vendor_archive(archive: Path) -> None:
    archive_data = archive.read_bytes()
    UCD_ROOT.mkdir(parents=True, exist_ok=True)
    files: dict[str, dict[str, object]] = {}
    with zipfile.ZipFile(archive) as bundle:
        for name in REQUIRED_FILES:
            try:
                data = bundle.read(name)
            except KeyError as exc:
                raise SystemExit(f"missing {name} in {archive}") from exc
            target = UCD_ROOT / name
            target.write_bytes(data)
            files[name] = {"sha256": digest_bytes(data), "size": len(data)}
    manifest = {
        "schema": "vitte.unicode.ucd.v1",
        "version": VERSION,
        "source": SOURCE_URL,
        "archive_sha256": digest_bytes(archive_data),
        "files": files,
    }
    MANIFEST.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def load_manifest() -> dict[str, object]:
    if not MANIFEST.is_file():
        raise SystemExit(f"missing pinned UCD manifest: {MANIFEST.relative_to(ROOT)}")
    data = json.loads(MANIFEST.read_text(encoding="utf-8"))
    if data.get("version") != VERSION or data.get("source") != SOURCE_URL:
        raise SystemExit("UCD manifest version/source does not match generator contract")
    files = data.get("files")
    if not isinstance(files, dict):
        raise SystemExit("UCD manifest has no file table")
    for name in REQUIRED_FILES:
        path = UCD_ROOT / name
        entry = files.get(name)
        if not path.is_file() or not isinstance(entry, dict):
            raise SystemExit(f"missing pinned UCD input: {name}")
        actual = digest_bytes(path.read_bytes())
        if actual != entry.get("sha256"):
            raise SystemExit(f"UCD checksum mismatch: {name}")
    return data


def parse_range(raw: str) -> tuple[int, int]:
    bounds = raw.strip().split("..")
    start = int(bounds[0], 16)
    return start, int(bounds[-1], 16)


def property_ranges(filename: str, selected: set[str]) -> dict[str, list[tuple[int, int]]]:
    result = {name: [] for name in selected}
    for raw in (UCD_ROOT / filename).read_text(encoding="utf-8").splitlines():
        body = raw.split("#", 1)[0].strip()
        if not body:
            continue
        codepoints, prop = (part.strip() for part in body.split(";", 1))
        if prop in result:
            result[prop].append(parse_range(codepoints))
    return result


def apply_ranges(values: bytearray, ranges: list[tuple[int, int]], mask: int) -> None:
    for start, end in ranges:
        for codepoint in range(start, end + 1):
            values[codepoint] |= mask


def unicode_rows() -> tuple[list[tuple[int, int, int, int]], list[tuple[int, int, int, int]]]:
    limit = 0x110000
    categories = bytearray([CATEGORY_CODES["Cn"]]) * limit
    flags = bytearray(limit)
    case_maps: list[tuple[int, int, int, int]] = []
    pending_range: tuple[int, int] | None = None

    for raw in (UCD_ROOT / "UnicodeData.txt").read_text(encoding="utf-8").splitlines():
        fields = raw.split(";")
        codepoint = int(fields[0], 16)
        name = fields[1]
        category = CATEGORY_CODES[fields[2]]
        if name.endswith(", First>"):
            pending_range = (codepoint, category)
            continue
        if name.endswith(", Last>"):
            if pending_range is None:
                raise SystemExit("UnicodeData range end without range start")
            start, first_category = pending_range
            if first_category != category:
                raise SystemExit("UnicodeData range category mismatch")
            categories[start:codepoint + 1] = bytes([category]) * (codepoint + 1 - start)
            pending_range = None
        else:
            categories[codepoint] = category
        upper = int(fields[12], 16) if fields[12] else codepoint
        lower = int(fields[13], 16) if fields[13] else codepoint
        title = int(fields[14], 16) if fields[14] else codepoint
        if (upper, lower, title) != (codepoint, codepoint, codepoint):
            case_maps.append((codepoint, upper, lower, title))

    derived = property_ranges("DerivedCoreProperties.txt", {"Alphabetic", "Uppercase", "Lowercase"})
    props = property_ranges("PropList.txt", {"White_Space"})
    apply_ranges(flags, derived["Alphabetic"], 1)
    apply_ranges(flags, derived["Uppercase"], 2)
    apply_ranges(flags, derived["Lowercase"], 4)
    apply_ranges(flags, props["White_Space"], 8)
    for codepoint, category in enumerate(categories):
        if category in (8, 9, 10):
            flags[codepoint] |= 16

    ranges: list[tuple[int, int, int, int]] = []
    start = 0
    previous = (categories[0], flags[0])
    for codepoint in range(1, limit):
        current = (categories[codepoint], flags[codepoint])
        if current != previous:
            ranges.append((start, codepoint - 1, previous[0], previous[1]))
            start = codepoint
            previous = current
    ranges.append((start, limit - 1, previous[0], previous[1]))
    return ranges, case_maps


def render_tables(manifest: dict[str, object]) -> str:
    ranges, case_maps = unicode_rows()
    archive_sha = str(manifest["archive_sha256"])
    lines = [
        "space vitte/stdlib/generated/unicode_tables",
        "",
        "export *",
        "",
        f'const GENERATED_UNICODE_VERSION: string = "{VERSION}"',
        f'const GENERATED_UNICODE_SOURCE: string = "{SOURCE_URL}"',
        f'const GENERATED_UNICODE_CHECKSUM: string = "sha256:{archive_sha}"',
        f"const GENERATED_UNICODE_RANGE_COUNT: int = {len(ranges)}",
        f"const GENERATED_UNICODE_CASE_COUNT: int = {len(case_maps)}",
        "",
        "form GeneratedUnicodeRange { start: int end: int category: int flags: int }",
        "form GeneratedUnicodeCase { codepoint: int uppercase: int lowercase: int titlecase: int }",
        "",
        "proc generated_unicode_ranges() -> [GeneratedUnicodeRange] {",
        "    give [",
    ]
    lines.extend(
        f"        GeneratedUnicodeRange {{ start: {start}, end: {end}, category: {category}, flags: {flags} }},"
        for start, end, category, flags in ranges
    )
    lines.extend(["    ];", "}", "", "proc generated_unicode_cases() -> [GeneratedUnicodeCase] {", "    give ["])
    lines.extend(
        f"        GeneratedUnicodeCase {{ codepoint: {codepoint}, uppercase: {upper}, lowercase: {lower}, titlecase: {title} }},"
        for codepoint, upper, lower, title in case_maps
    )
    lines.extend([
        "    ];", "}", "",
        "proc generated_unicode_version() -> string { give GENERATED_UNICODE_VERSION; }",
        "proc generated_unicode_source() -> string { give GENERATED_UNICODE_SOURCE; }",
        "proc generated_unicode_checksum() -> string { give GENERATED_UNICODE_CHECKSUM; }",
        "proc generated_unicode_verify_checksum() -> bool {",
        f'    give GENERATED_UNICODE_VERSION == "{VERSION}" and GENERATED_UNICODE_CHECKSUM == "sha256:{archive_sha}";',
        "}",
        "",
        "proc generated_unicode_range(value: rune) -> GeneratedUnicodeRange {",
        "    let codepoint: int = value as int;",
        "    let ranges: [GeneratedUnicodeRange] = generated_unicode_ranges();",
        "    let low: int = 0;",
        "    let high: int = ranges.len - 1;",
        "    while low <= high {",
        "        let middle: int = low + (high - low) / 2;",
        "        if codepoint < ranges[middle].start {",
        "            set high = middle - 1;",
        "        } else {",
        "            if codepoint > ranges[middle].end {",
        "                set low = middle + 1;",
        "            } else {",
        "                give ranges[middle];",
        "            }",
        "        }",
        "    }",
        "    give GeneratedUnicodeRange { start: 0, end: 1114111, category: 29, flags: 0 };",
        "}",
        "",
        "proc generated_unicode_case(value: rune) -> GeneratedUnicodeCase {",
        "    let codepoint: int = value as int;",
        "    let cases: [GeneratedUnicodeCase] = generated_unicode_cases();",
        "    let low: int = 0;",
        "    let high: int = cases.len - 1;",
        "    while low <= high {",
        "        let middle: int = low + (high - low) / 2;",
        "        if codepoint < cases[middle].codepoint {",
        "            set high = middle - 1;",
        "        } else {",
        "            if codepoint > cases[middle].codepoint {",
        "                set low = middle + 1;",
        "            } else {",
        "                give cases[middle];",
        "            }",
        "        }",
        "    }",
        "    give GeneratedUnicodeCase { codepoint: codepoint, uppercase: codepoint, lowercase: codepoint, titlecase: codepoint };",
        "}",
        "",
        "proc generated_unicode_category(value: rune) -> int {",
        "    give generated_unicode_range(value).category;",
        "}",
        "",
        "proc generated_unicode_properties(value: rune) -> GeneratedUnicodeRange {",
        "    give generated_unicode_range(value);",
        "}",
        "",
        "proc generated_unicode_normalization(form: int, value: rune) -> [rune] {",
        "    give [value];",
        "}",
        "",
        "proc generated_unicode_case_fold(value: rune) -> [rune] {",
        "    give [generated_unicode_case(value).lowercase as rune];",
        "}",
        "",
    ])
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description="generate pinned Vitte Unicode tables")
    parser.add_argument("--ucd-zip", type=Path, help="vendor the official pinned UCD archive before generation")
    parser.add_argument("--check", action="store_true", help="fail if generated output differs")
    args = parser.parse_args()
    if args.ucd_zip:
        vendor_archive(args.ucd_zip)
    manifest = load_manifest()
    rendered = render_tables(manifest)
    if args.check:
        if not OUTPUT.is_file() or OUTPUT.read_text(encoding="utf-8") != rendered:
            raise SystemExit("generated Unicode tables are stale")
        print(f"[unicode-tables] OK version={VERSION} output={OUTPUT.relative_to(ROOT)}")
        return 0
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text(rendered, encoding="utf-8")
    print(f"[unicode-tables] generated version={VERSION} output={OUTPUT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
