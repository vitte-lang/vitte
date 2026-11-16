#!/usr/bin/env python3
"""Generate the native Vitte XID lookup module from the pinned C table."""

from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "src/bootstrap/unicode/unicode_xid_table.h"
OUTPUT = ROOT / "src/compiler/unicode/unicode_xid_table.vitte"
RANGE_RE = re.compile(
    r"\{\s*UINT32_C\(0x([0-9a-fA-F]+)\),\s*"
    r"UINT32_C\(0x([0-9a-fA-F]+)\)\s*\}"
)


def read_ranges(source: str, name: str) -> list[tuple[int, int]]:
    marker = f"static const vitte_unicode_xid_range_t {name}[] = {{"
    try:
        start = source.index(marker) + len(marker)
        end = source.index("};", start)
    except ValueError as error:
        raise SystemExit(f"could not find {name} in {HEADER}") from error

    ranges = [(int(first, 16), int(last, 16)) for first, last in RANGE_RE.findall(source[start:end])]
    if not ranges:
        raise SystemExit(f"no ranges found for {name} in {HEADER}")

    previous_last = -1
    for first, last in ranges:
        if first > last or first <= previous_last or last > 0x10FFFF:
            raise SystemExit(f"invalid or unsorted range in {name}: {first:#x}..{last:#x}")
        previous_last = last
    return ranges


def render_array(name: str, ranges: list[tuple[int, int]]) -> str:
    values = [value for pair in ranges for value in pair]
    lines = [f"const {name}: [i64] = ["]
    for offset in range(0, len(values), 8):
        lines.append("    " + ", ".join(f"0x{value:06x}" for value in values[offset : offset + 8]) + ",")
    lines.append("];\n")
    return "\n".join(lines)


def render_lookup(name: str, ranges_name: str) -> str:
    return f'''export proc {name}(codepoint: u64) -> bool {{
    if codepoint > 0x10ffff {{ return false; }}
    let mut first: usize = 0;
    let mut last: usize = {ranges_name}.length / 2;
    while first < last {{
        let middle: usize = first + (last - first) / 2;
        let range_first: u64 = {ranges_name}[middle * 2] as u64;
        let range_last: u64 = {ranges_name}[middle * 2 + 1] as u64;
        if codepoint < range_first {{
            set last = middle;
        }} else if codepoint > range_last {{
            set first = middle + 1;
        }} else {{
            return true;
        }}
    }}
    return false;
}}
'''


def main() -> None:
    source = HEADER.read_text(encoding="utf-8")
    version_match = re.search(
        r'^#define VITTE_UNICODE_GENERATED_DATA_VERSION_STRING "([^"]+)"',
        source,
        re.MULTILINE,
    )
    if version_match is None:
        raise SystemExit(f"could not find Unicode data version in {HEADER}")
    version = version_match.group(1)

    start_ranges = read_ranges(source, "vitte_unicode_xid_start_ranges")
    continue_ranges = read_ranges(source, "vitte_unicode_xid_continue_ranges")
    output = [
        "/// Generated from src/bootstrap/unicode/unicode_xid_table.h; do not edit.",
        f"/// Unicode XID properties, version {version}.",
        "space compiler::unicode::unicode_xid_table;",
        "",
        render_array("UNICODE_XID_START_RANGES", start_ranges),
        render_array("UNICODE_XID_CONTINUE_RANGES", continue_ranges),
        render_lookup("vitte_unicode_xid_start", "UNICODE_XID_START_RANGES"),
        render_lookup("vitte_unicode_xid_continue", "UNICODE_XID_CONTINUE_RANGES"),
        f'export proc vitte_unicode_xid_data_version() -> string {{ return "{version}"; }}',
        "",
    ]
    with OUTPUT.open("w", encoding="utf-8", newline="\n") as output_file:
        output_file.write("\n".join(output))
    print(f"wrote {OUTPUT.relative_to(ROOT)} ({len(start_ranges)} start, {len(continue_ranges)} continue ranges; Unicode {version})")


if __name__ == "__main__":
    main()
