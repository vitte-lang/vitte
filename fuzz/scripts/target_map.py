#!/usr/bin/env python3
from __future__ import annotations

import argparse
from dataclasses import dataclass


@dataclass(frozen=True)
class TargetDefaults:
    dict_path: str | None = None


TARGETS: dict[str, TargetDefaults] = {
    "fuzz_lexer": TargetDefaults(dict_path="fuzz/dict/lexer_tokers.dict"),
    "fuzz_vittec_lexer_roundtrip": TargetDefaults(dict_path="fuzz/dict/vittec_front.dict"),
    "fuzz_vittec_parser_toplevel": TargetDefaults(dict_path="fuzz/dict/vittec_front.dict"),
    "fuzz_parser": TargetDefaults(dict_path="fuzz/dict/parsergrammar.dict"),
    "fuzz_parser_recovery": TargetDefaults(dict_path="fuzz/dict/parsergrammar.dict"),
    "fuzz_lowering": TargetDefaults(dict_path="fuzz/dict/parsergrammar.dict"),
    "fuzz_vitte_parser": TargetDefaults(dict_path="fuzz/dict/parsergrammar.dict"),
}

SMOKE_TARGETS: list[str] = [
    "fuzz_lexer",
    "fuzz_parser",
    "fuzz_parser_recovery",
    "fuzz_ast_invariants",
    "fuzz_lowering",
    "fuzz_ast_printer",
    "fuzz_vitte_parser",
    "fuzz_typecheck",
    "fuzz_lockfile",
    "fuzz_muf_parser",
    "fuzz_vm_exec",
    "fuzir_verify",
    "fuzz_diag_json",
    "fuze_vm_decode",
    "fuzz_asm_verify",
]


def cmd_dict(target: str) -> int:
    defaults = TARGETS.get(target)
    print(defaults.dict_path if defaults and defaults.dict_path else "")
    return 0


def cmd_campaign_corpus_root() -> int:
    print("fuzz/corpora")
    return 0


def cmd_standalone_corpus_root() -> int:
    print("fuzz/targets/corpus")
    return 0


def cmd_smoke_targets() -> int:
    for target in SMOKE_TARGETS:
        print(target)
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(prog="target_map.py")
    sub = parser.add_subparsers(dest="cmd", required=True)

    p_dict = sub.add_parser("dict", help="Print dictionary path for a target (or empty).")
    p_dict.add_argument("target")

    sub.add_parser("campaign-corpus-root", help="Print corpus root for campaign fuzzing.")
    sub.add_parser("standalone-corpus-root", help="Print corpus root for standalone smoke tests.")
    sub.add_parser("smoke-targets", help="Print smoke targets list, one per line.")

    args = parser.parse_args()

    if args.cmd == "dict":
        return cmd_dict(args.target)
    if args.cmd == "campaign-corpus-root":
        return cmd_campaign_corpus_root()
    if args.cmd == "standalone-corpus-root":
        return cmd_standalone_corpus_root()
    if args.cmd == "smoke-targets":
        return cmd_smoke_targets()
    raise AssertionError(f"unhandled cmd: {args.cmd}")


if __name__ == "__main__":
    raise SystemExit(main())

