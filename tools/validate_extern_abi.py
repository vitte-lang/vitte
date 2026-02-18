#!/usr/bin/env python3
import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
STD_ROOT = ROOT / "src" / "vitte" / "packages"
HOST_RUNTIME_HDR = ROOT / "src" / "compiler" / "backends" / "runtime" / "vitte_runtime.hpp"
ARDUINO_RUNTIME_HDR = ROOT / "target" / "arduino" / "include" / "vitte_runtime.hpp"
KERNEL_GRUB_RUNTIME_HDR = ROOT / "target" / "kernel" / "x86_64" / "grub" / "include" / "vitte_runtime.hpp"
KERNEL_UEFI_RUNTIME_HDR = ROOT / "target" / "kernel" / "x86_64" / "uefi" / "include" / "vitte_runtime.hpp"
ALLOWLIST_FILE = ROOT / "tools" / "extern_abi_allowlist.txt"

EXTERN_PROC_RE = re.compile(r"^\s*proc\s+([A-Za-z_][A-Za-z0-9_]*)\s*\((.*)\)\s*(?:->\s*(.+))?\s*$")
HDR_PROTO_RE = re.compile(r"^\s*(.+?)\s+([A-Za-z_][A-Za-z0-9_]*)\s*\((.*)\)\s*;\s*$")


def normalize(s: str) -> str:
    return " ".join(s.strip().split())


def split_top_level_commas(s: str):
    out = []
    cur = []
    depth = 0
    for ch in s:
        if ch in "[<(":
            depth += 1
        elif ch in "]>)":
            depth = max(0, depth - 1)
        elif ch == "," and depth == 0:
            out.append("".join(cur).strip())
            cur = []
            continue
        cur.append(ch)
    tail = "".join(cur).strip()
    if tail:
        out.append(tail)
    return out


def profile_allows(profile: str, rel: Path) -> bool:
    rel_posix = rel.as_posix()
    if profile == "all":
        return True
    if profile == "host":
        return not rel_posix.startswith("src/vitte/packages/arduino/") and not rel_posix.startswith("src/vitte/packages/kernel/")
    if profile == "arduino":
        return rel_posix.startswith("src/vitte/packages/arduino/")
    if profile == "kernel":
        return rel_posix.startswith("src/vitte/packages/kernel/")
    return True


def parse_std_externs(std_root: Path, profile: str):
    entries = {}
    for vit in sorted(std_root.rglob("*.vit")):
        rel = vit.relative_to(ROOT)
        if not profile_allows(profile, rel):
            continue
        lines = vit.read_text(encoding="utf-8").splitlines()
        i = 0
        while i < len(lines):
            if lines[i].strip() != "#[extern]":
                i += 1
                continue
            j = i + 1
            while j < len(lines) and not lines[j].strip():
                j += 1
            if j >= len(lines):
                break
            m = EXTERN_PROC_RE.match(lines[j].strip())
            if not m:
                i = j + 1
                continue
            name = m.group(1)
            params_raw = m.group(2).strip()
            ret_raw = normalize(m.group(3) or "Unit")
            params = []
            if params_raw:
                for p in split_top_level_commas(params_raw):
                    if ":" not in p:
                        continue
                    _, ty = p.split(":", 1)
                    params.append(normalize(ty))
            sig = (ret_raw, tuple(params))
            entries.setdefault(name, []).append((sig, vit))
            i = j + 1
    return entries


def parse_runtime_externs(path: Path):
    lines = path.read_text(encoding="utf-8").splitlines()
    in_extern = False
    out = {}
    for line in lines:
        s = line.strip()
        if s == 'extern "C" {':
            in_extern = True
            continue
        if in_extern and s == '}':
            in_extern = False
            continue
        if not in_extern or not s or s.startswith("//"):
            continue
        m = HDR_PROTO_RE.match(s)
        if not m:
            continue
        ret = normalize(m.group(1))
        name = m.group(2)
        params_raw = m.group(3).strip()
        params = []
        if params_raw and params_raw != "void":
            for p in split_top_level_commas(params_raw):
                p = normalize(p)
                if not p:
                    continue
                if " " in p:
                    p = p.rsplit(" ", 1)[0]
                params.append(p)
        out[name] = (ret, tuple(params))
    return out


def runtime_header_for_profile(profile: str, kernel_variant: str) -> Path:
    if profile == "host" or profile == "all":
        return HOST_RUNTIME_HDR
    if profile == "arduino":
        return ARDUINO_RUNTIME_HDR
    if profile == "kernel":
        return KERNEL_UEFI_RUNTIME_HDR if kernel_variant == "uefi" else KERNEL_GRUB_RUNTIME_HDR
    return HOST_RUNTIME_HDR


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Validate std #[extern] ABI against runtime header")
    p.add_argument("--profile", choices=["host", "arduino", "kernel", "all"], default="host")
    p.add_argument("--kernel-variant", choices=["grub", "uefi"], default="grub")
    p.add_argument("--runtime-header", default="", help="override runtime header path")
    p.add_argument("--allowlist", default=str(ALLOWLIST_FILE), help="allowlist file path")
    p.add_argument("--strict-warnings", action="store_true", help="fail on warnings not in allowlist")
    p.add_argument("--dump-warnings", action="store_true", help="print all warnings")
    return p.parse_args()


def parse_allowlist(path: Path):
    rules = []
    if not path.exists():
        return rules
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        parts = [p.strip() for p in line.split("|", 2)]
        if len(parts) != 3:
            continue
        prof, kind, name = parts
        rules.append((prof, kind, name))
    return rules


def allowed_warning(profile: str, kind: str, name: str, rules) -> bool:
    for prof, k, n in rules:
        prof_ok = prof in (profile, "*")
        kind_ok = k in (kind, "*")
        name_ok = n in (name, "*")
        if prof_ok and kind_ok and name_ok:
            return True
    return False


def main() -> int:
    args = parse_args()

    if not STD_ROOT.exists():
        print("[extern-abi][error] missing std root", file=sys.stderr)
        return 2

    runtime_hdr = Path(args.runtime_header) if args.runtime_header else runtime_header_for_profile(args.profile, args.kernel_variant)
    if not runtime_hdr.exists():
        print(f"[extern-abi][error] missing runtime header: {runtime_hdr}", file=sys.stderr)
        return 2

    std_entries = parse_std_externs(STD_ROOT, args.profile)
    runtime_entries = parse_runtime_externs(runtime_hdr)

    failures = []
    warnings = []
    allow_rules = parse_allowlist(Path(args.allowlist))

    for name, lst in sorted(std_entries.items()):
        sigs = {sig for sig, _ in lst}
        if len(sigs) > 1:
            details = ", ".join(str(p.relative_to(ROOT)) for _, p in lst)
            failures.append(f"conflicting std extern signatures for {name}: {details}")

    for name, (std_sig, path) in sorted((n, lst[0]) for n, lst in std_entries.items()):
        if name in runtime_entries:
            rt_sig = runtime_entries[name]
            if len(std_sig[1]) != len(rt_sig[1]):
                failures.append(
                    f"arity mismatch for {name}: std={len(std_sig[1])} runtime={len(rt_sig[1])} ({path.relative_to(ROOT)})"
                )
        else:
            warnings.append(("missing_in_runtime", name, str(path.relative_to(ROOT))))

    for name in sorted(runtime_entries):
        if name.startswith("vitte_"):
            continue
        if name not in std_entries:
            warnings.append(("missing_in_std", name, ""))

    tag = f"[extern-abi:{args.profile}]"
    if failures:
        print(f"{tag} FAIL")
        for f in failures:
            print(f" - {f}")
        return 1

    unexpected = []
    allowed = []
    for kind, name, info in warnings:
        if allowed_warning(args.profile, kind, name, allow_rules):
            allowed.append((kind, name, info))
        else:
            unexpected.append((kind, name, info))

    print(f"{tag} OK")
    print(f"{tag} std externs: {len(std_entries)} | runtime externs: {len(runtime_entries)}")
    print(f"{tag} runtime header: {runtime_hdr}")
    print(f"{tag} warnings: total={len(warnings)} allowed={len(allowed)} unexpected={len(unexpected)}")
    dump_list = warnings if args.dump_warnings else unexpected
    if dump_list:
        limit = len(dump_list) if args.dump_warnings else min(20, len(dump_list))
        for kind, name, info in dump_list[:limit]:
            suffix = f" ({info})" if info else ""
            print(f" - [{kind}] {name}{suffix}")
        if not args.dump_warnings and len(dump_list) > limit:
            print(f" - ... {len(dump_list)-limit} more")

    if args.strict_warnings and unexpected:
        print(f"{tag} FAIL (unexpected warnings)")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
