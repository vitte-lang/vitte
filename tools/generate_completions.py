#!/usr/bin/env python3
"""
Generate shell completions for the Vitte toolchain.

Generated files must not be edited manually.

Primary commands:
    vitte
    vittec

Source extensions:
    .vit
    .vitl
    .vitte
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import shlex
import sys
from pathlib import Path
from typing import Final


GENERATOR_VERSION: Final = "2.1"
SCHEMA_VERSION: Final = 2

ROOT_DIR: Final = Path(__file__).resolve().parent.parent
DEFAULT_OUTPUT: Final = ROOT_DIR / "completions" / "vitte.bash"

SOURCE_EXTENSIONS: Final = (
    ".vit",
    ".vitl",
    ".vitte",
)

LANGUAGES: Final = (
    "en",
    "fr",
    "es",
    "de",
    "it",
    "pt-BR",
    "nl",
    "pl",
    "ru",
    "uk",
    "zh-CN",
    "ja",
    "ko",
    "tr",
    "ar",
)

RUNTIME_PROFILES: Final = (
    "core",
    "system",
    "desktop",
    "arduino",
)

STDLIB_PROFILES: Final = (
    "minimal",
    "full",
    "kernel",
    "arduino",
)

STAGES: Final = (
    "parse",
    "resolve",
    "ir",
    "backend",
)

TARGETS: Final = (
    "vitte",
    "arduino-uno",
)

FQBNS: Final = (
    "arduino:avr:uno",
)

DIAGNOSTIC_CODES: Final = (
    "E0001",
    "E0002",
    "E0003",
)

COMMANDS: Final = (
    "build",
    "check",
    "clean-cache",
    "doctor",
    "emit",
    "explain",
    "help",
    "init",
    "mod",
    "parse",
    "profile",
    "reduce",
)

MOD_COMMANDS: Final = (
    "contract-diff",
    "doctor",
    "graph",
)

GLOBAL_OPTIONS: Final = (
    "--allow-experimental",
    "--allow-internal",
    "--debug",
    "--deny-internal",
    "--fail-on-warning",
    "--freestanding",
    "--help",
    "--lang",
    "--runtime-profile",
    "--stdlib-profile",
    "--warn-experimental",
    "-O0",
    "-O1",
    "-O2",
    "-O3",
    "-h",
)

OPTIONS_BY_COMMAND: Final = {
    "build": (
        "--cache-report",
        "--fqbn",
        "--port",
        "--repro",
        "--repro-strict",
        "--runtime-include",
        "--stage",
        "--target",
        "--upload",
        "-o",
    ),
    "check": (
        "--cache-report",
        "--deterministic",
        "--diag-code-only",
        "--diag-filter",
        "--diag-json",
        "--diag-json-pretty",
        "--dump-hir",
        "--dump-hir-compact",
        "--dump-hir-json",
        "--dump-ir",
        "--dump-mir",
        "--dump-resolve",
        "--hir-only",
        "--mir-only",
        "--repro",
        "--repro-strict",
        "--resolve-only",
        "--stage",
        "--strict-bridge",
        "--strict-imports",
        "--strict-modules",
        "--strict-types",
    ),
    "clean-cache": (
        "--help",
        "-h",
    ),
    "doctor": (
        "--help",
        "--lang",
        "-h",
    ),
    "emit": (
        "--dump-ir",
        "--dump-mir",
        "--emit-vitte",
        "--repro",
        "--repro-strict",
        "--runtime-include",
        "--stage",
        "--stdout",
        "--target",
        "-o",
    ),
    "explain": (
        "--explain",
        "--help",
        "--lang",
        "-h",
    ),
    "help": (
        "--help",
        "--lang",
        "-h",
    ),
    "init": (
        "--help",
        "--lang",
        "-h",
    ),
    "parse": (
        "--diag-code-only",
        "--diag-filter",
        "--diag-json",
        "--diag-json-pretty",
        "--dump-ast",
        "--parse-modules",
        "--parse-only",
        "--parse-silent",
        "--stage",
        "--strict-parse",
    ),
    "profile": (
        "--cache-report",
        "--repro",
        "--repro-strict",
        "--stage",
        "--target",
        "-o",
    ),
    "reduce": (
        "--diag-filter",
        "--lang",
        "--stage",
        "--target",
    ),
}

MOD_OPTIONS: Final = {
    "contract-diff": (
        "--new",
        "--old",
    ),
    "doctor": (
        "--fix",
        "--max-imports",
    ),
    "graph": (
        "--from",
        "--json",
    ),
}

ALL_CONTEXT_OPTIONS: Final = (
    "--allow-experimental",
    "--allow-internal",
    "--cache-report",
    "--debug",
    "--deny-internal",
    "--deterministic",
    "--diag-code-only",
    "--diag-filter",
    "--diag-json",
    "--diag-json-pretty",
    "--dump-ast",
    "--dump-hir",
    "--dump-hir-compact",
    "--dump-hir-json",
    "--dump-ir",
    "--dump-mir",
    "--dump-module-index",
    "--dump-resolve",
    "--dump-stdlib-map",
    "--emit-vitte",
    "--explain",
    "--fail-on-warning",
    "--fix",
    "--fqbn",
    "--freestanding",
    "--from",
    "--help",
    "--hir-only",
    "--json",
    "--lang",
    "--max-imports",
    "--mir-only",
    "--new",
    "--old",
    "--parse-modules",
    "--parse-only",
    "--parse-silent",
    "--port",
    "--repro",
    "--repro-strict",
    "--resolve-only",
    "--runtime-include",
    "--runtime-profile",
    "--stage",
    "--stdlib-profile",
    "--stdout",
    "--strict-bridge",
    "--strict-imports",
    "--strict-modules",
    "--strict-parse",
    "--strict-types",
    "--target",
    "--upload",
    "--warn-experimental",
    "-O0",
    "-O1",
    "-O2",
    "-O3",
    "-h",
    "-o",
)

VALUE_OPTIONS: Final = {
    "--diag-filter": "file",
    "--explain": "diagnostic",
    "--fqbn": "fqbn",
    "--from": "module",
    "--lang": "language",
    "--max-imports": "integer",
    "--new": "source",
    "--old": "source",
    "--port": "file",
    "--runtime-include": "directory",
    "--runtime-profile": "runtime-profile",
    "--stage": "stage",
    "--stdlib-profile": "stdlib-profile",
    "--target": "target",
    "-o": "file",
}


def canonical_spec() -> dict[str, object]:
    return {
        "schema_version": SCHEMA_VERSION,
        "commands": COMMANDS,
        "mod_commands": MOD_COMMANDS,
        "global_options": GLOBAL_OPTIONS,
        "options_by_command": OPTIONS_BY_COMMAND,
        "mod_options": MOD_OPTIONS,
        "all_context_options": ALL_CONTEXT_OPTIONS,
        "value_options": VALUE_OPTIONS,
        "source_extensions": SOURCE_EXTENSIONS,
        "languages": LANGUAGES,
        "runtime_profiles": RUNTIME_PROFILES,
        "stdlib_profiles": STDLIB_PROFILES,
        "stages": STAGES,
        "targets": TARGETS,
        "fqbns": FQBNS,
        "diagnostic_codes": DIAGNOSTIC_CODES,
    }


def spec_sha256() -> str:
    payload = json.dumps(
        canonical_spec(),
        ensure_ascii=False,
        sort_keys=True,
        separators=(",", ":"),
    ).encode("utf-8")

    return hashlib.sha256(payload).hexdigest()


def shell_words(values: tuple[str, ...] | list[str]) -> str:
    return " ".join(values)


def bash_case_options(values: tuple[str, ...]) -> str:
    return shell_words(values)


def generate_bash() -> str:
    digest = spec_sha256()

    command_cases: list[str] = []

    for command, options in OPTIONS_BY_COMMAND.items():
        command_cases.append(
            f'    {command}) '
            f'echo "{bash_case_options(options)}"; '
            "return 0 ;;"
        )

    mod_cases: list[str] = []

    for subcommand, options in MOD_OPTIONS.items():
        mod_cases.append(
            f'      {subcommand}) '
            f'echo "{bash_case_options(options)}"; '
            "return 0 ;;"
        )

    source_case = " | ".join(
        f"*{extension}"
        for extension in SOURCE_EXTENSIONS
    )

    commands = shell_words(COMMANDS)
    mod_commands = shell_words(MOD_COMMANDS)
    globals_ = shell_words(GLOBAL_OPTIONS)
    all_options = shell_words(ALL_CONTEXT_OPTIONS)
    languages = shell_words(LANGUAGES)
    runtime_profiles = shell_words(RUNTIME_PROFILES)
    stdlib_profiles = shell_words(STDLIB_PROFILES)
    stages = shell_words(STAGES)
    targets = shell_words(TARGETS)
    fqbns = shell_words(FQBNS)
    diagnostic_codes = shell_words(DIAGNOSTIC_CODES)

    command_case_text = "\n".join(command_cases)
    mod_case_text = "\n".join(mod_cases)

    return f"""# Auto-generated by tools/generate_completions.py. Do not edit manually.
# completion-generator-version: {GENERATOR_VERSION}
# completion-schema-version: {SCHEMA_VERSION}
# completion-spec-sha256: {digest}
# completion-help-mode: dynamic

_vitte_commands() {{
  echo "{commands}"
}}

_vitte_mod_commands() {{
  echo "{mod_commands}"
}}

_vitte_global_options() {{
  echo "{globals_}"
}}

_vitte_options_for_context() {{
  local cmd="$1" subcmd="$2"

  case "$cmd" in
{command_case_text}
    mod)
      case "$subcmd" in
{mod_case_text}
      esac

      echo "--fix --from --json --max-imports --new --old"
      return 0
      ;;
  esac

  echo "{all_options}"
}}

_vitte_complete_words() {{
  local words="$1"
  local cur="$2"

  COMPREPLY=( $(compgen -W "$words" -- "$cur") )
}}

_vitte_complete_files() {{
  local cur="$1"

  COMPREPLY=( $(compgen -f -- "$cur") )
  compopt -o filenames 2>/dev/null || true
}}

_vitte_complete_directories() {{
  local cur="$1"

  COMPREPLY=( $(compgen -d -- "$cur") )
  compopt -o filenames 2>/dev/null || true
}}

_vitte_complete_sources() {{
  local cur="$1"
  local candidate
  local -a matches=()

  while IFS= read -r candidate; do
    case "$candidate" in
      {source_case} | */)
        matches+=("$candidate")
        ;;
    esac
  done < <(compgen -f -- "$cur")

  while IFS= read -r candidate; do
    case "$candidate" in
      */) ;;
      *) candidate="${{candidate}}/" ;;
    esac

    matches+=("$candidate")
  done < <(compgen -d -- "$cur")

  COMPREPLY=("${{matches[@]}}")
  compopt -o filenames 2>/dev/null || true
}}

_vitte_option_takes_value() {{
  case "$1" in
    --diag-filter | \\
    --explain | \\
    --fqbn | \\
    --from | \\
    --lang | \\
    --max-imports | \\
    --new | \\
    --old | \\
    --port | \\
    --runtime-include | \\
    --runtime-profile | \\
    --stage | \\
    --stdlib-profile | \\
    --target | \\
    -o)
      return 0
      ;;
  esac

  return 1
}}

_vitte_complete_option_value() {{
  local option="$1"
  local cur="$2"

  case "$option" in
    --diag-filter)
      _vitte_complete_files "$cur"
      ;;

    --explain)
      _vitte_complete_words "{diagnostic_codes}" "$cur"
      ;;

    --fqbn)
      _vitte_complete_words "{fqbns}" "$cur"
      ;;

    --from)
      _vitte_complete_words "__root__" "$cur"
      ;;

    --lang)
      _vitte_complete_words "{languages}" "$cur"
      ;;

    --max-imports)
      COMPREPLY=()
      ;;

    --new | --old)
      _vitte_complete_sources "$cur"
      ;;

    --port)
      _vitte_complete_files "$cur"
      ;;

    --runtime-include)
      _vitte_complete_directories "$cur"
      ;;

    --runtime-profile)
      _vitte_complete_words "{runtime_profiles}" "$cur"
      ;;

    --stage)
      _vitte_complete_words "{stages}" "$cur"
      ;;

    --stdlib-profile)
      _vitte_complete_words "{stdlib_profiles}" "$cur"
      ;;

    --target)
      _vitte_complete_words "{targets}" "$cur"
      ;;

    -o)
      _vitte_complete_files "$cur"
      ;;

    *)
      return 1
      ;;
  esac

  return 0
}}

_vitte_complete_equals_option() {{
  local cur="$1"
  local option value candidate words
  local -a values=()

  case "$cur" in
    --*=*)
      option=${{cur%%=*}}
      value=${{cur#*=}}
      ;;

    *)
      return 1
      ;;
  esac

  case "$option" in
    --explain)
      words="{diagnostic_codes}"
      ;;

    --fqbn)
      words="{fqbns}"
      ;;

    --from)
      words="__root__"
      ;;

    --lang)
      words="{languages}"
      ;;

    --runtime-profile)
      words="{runtime_profiles}"
      ;;

    --stage)
      words="{stages}"
      ;;

    --stdlib-profile)
      words="{stdlib_profiles}"
      ;;

    --target)
      words="{targets}"
      ;;

    *)
      return 1
      ;;
  esac

  while IFS= read -r candidate; do
    values+=("$option=$candidate")
  done < <(compgen -W "$words" -- "$value")

  COMPREPLY=("${{values[@]}}")
  return 0
}}

_vitte_complete() {{
  local cur prev cmd subcmd opts
  local i
  local end_of_options=0

  COMPREPLY=()

  cur="${{COMP_WORDS[COMP_CWORD]}}"
  prev=""

  if (( COMP_CWORD > 0 )); then
    prev="${{COMP_WORDS[COMP_CWORD-1]}}"
  fi

  cmd="${{COMP_WORDS[1]:-}}"
  subcmd="${{COMP_WORDS[2]:-}}"

  for ((i = 1; i < COMP_CWORD; i++)); do
    if [[ "${{COMP_WORDS[i]}}" == "--" ]]; then
      end_of_options=1
      break
    fi
  done

  if (( COMP_CWORD == 1 )); then
    _vitte_complete_words \\
      "$(_vitte_commands) $(_vitte_global_options)" \\
      "$cur"
    return 0
  fi

  if [[ "$cmd" == "mod" && $COMP_CWORD -eq 2 ]]; then
    _vitte_complete_words \\
      "$(_vitte_mod_commands)" \\
      "$cur"
    return 0
  fi

  if _vitte_complete_equals_option "$cur"; then
    return 0
  fi

  if _vitte_option_takes_value "$prev"; then
    _vitte_complete_option_value "$prev" "$cur"
    return 0
  fi

  if (( end_of_options )); then
    case "$cmd" in
      parse | check | emit | build | profile | reduce)
        _vitte_complete_sources "$cur"
        ;;
      *)
        _vitte_complete_files "$cur"
        ;;
    esac

    return 0
  fi

  case "$cmd" in
    parse | check | emit | build | profile | reduce)
      if [[ "$cur" != -* ]]; then
        _vitte_complete_sources "$cur"
        return 0
      fi
      ;;
  esac

  opts="$(_vitte_options_for_context "$cmd" "$subcmd")"

  _vitte_complete_words "$opts" "$cur"
  return 0
}}

complete -o bashdefault -o default -F _vitte_complete vitte
complete -o bashdefault -o default -F _vitte_complete vittec
"""


def write_if_changed(path: Path, content: str) -> bool:
    if path.is_file():
        existing = path.read_text(encoding="utf-8")

        if existing == content:
            return False

    path.parent.mkdir(
        parents=True,
        exist_ok=True,
    )

    path.write_text(
        content,
        encoding="utf-8",
        newline="\n",
    )

    return True


def check_file(path: Path, expected: str) -> bool:
    if not path.is_file():
        print(
            f"missing generated completion: {path}",
            file=sys.stderr,
        )
        return False

    actual = path.read_text(encoding="utf-8")

    if actual != expected:
        print(
            f"generated completion is stale: {path}",
            file=sys.stderr,
        )
        return False

    return True


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate Vitte shell completions.",
    )

    parser.add_argument(
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT,
        help=(
            "Bash completion output path "
            f"(default: {DEFAULT_OUTPUT})"
        ),
    )

    parser.add_argument(
        "--stdout",
        action="store_true",
        help="Write generated completion to stdout.",
    )

    parser.add_argument(
        "--check",
        action="store_true",
        help=(
            "Verify that the generated completion is "
            "already up to date."
        ),
    )

    parser.add_argument(
        "--print-spec-sha256",
        action="store_true",
        help="Print the canonical completion specification SHA-256.",
    )

    parser.add_argument(
        "--dump-spec",
        action="store_true",
        help="Print the canonical completion specification as JSON.",
    )

    return parser.parse_args()


def main() -> int:
    args = parse_arguments()

    if args.print_spec_sha256:
        print(spec_sha256())
        return 0

    if args.dump_spec:
        json.dump(
            canonical_spec(),
            sys.stdout,
            ensure_ascii=False,
            indent=2,
            sort_keys=True,
        )
        sys.stdout.write("\n")
        return 0

    content = generate_bash()

    if args.stdout:
        sys.stdout.write(content)
        return 0

    output = args.output

    if not output.is_absolute():
        output = ROOT_DIR / output

    if args.check:
        if check_file(output, content):
            print(
                f"[generate_completions] OK: {output}"
            )
            return 0

        return 1

    changed = write_if_changed(
        output,
        content,
    )

    if changed:
        print(
            f"[generate_completions] wrote {output}"
        )
    else:
        print(
            f"[generate_completions] unchanged {output}"
        )

    print(
        "[generate_completions] "
        f"spec-sha256={spec_sha256()}"
    )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())