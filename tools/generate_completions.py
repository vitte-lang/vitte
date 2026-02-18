#!/usr/bin/env python3
import argparse
import copy
import hashlib
import json
import os
import re
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
SPEC_PATH = ROOT / "tools" / "completions" / "spec.json"
BASH_OUT = ROOT / "completions" / "bash" / "vitte"
ZSH_OUT = ROOT / "completions" / "zsh" / "_vitte"
FISH_OUT = ROOT / "completions" / "fish" / "vitte.fish"
GENERATOR_VERSION = "2.0"


def _sorted_unique(items):
    return sorted(set(items))


def _stable_hash(obj) -> str:
    blob = json.dumps(obj, sort_keys=True, separators=(",", ":")).encode("utf-8")
    return hashlib.sha256(blob).hexdigest()


def _warn(msg: str) -> None:
    print(f"[completions] warning: {msg}", file=sys.stderr)


def shjoin(items):
    return " ".join(items)


def load_spec() -> dict:
    with SPEC_PATH.open("r", encoding="utf-8") as f:
        return json.load(f)


def resolve_vitte_binary() -> Path | None:
    env_override = os.environ.get("VITTE_COMPLETIONS_BIN")
    if env_override is not None:
        cand = Path(env_override)
        return cand if cand.exists() and cand.is_file() else None
    for cand in [ROOT / "bin" / "vitte", ROOT / "target" / "bin" / "vitte"]:
        if cand.exists() and cand.is_file():
            return cand
    return None


def run_help(binary: Path, args: list[str]) -> tuple[str, bool]:
    cmd = [str(binary), *args]
    try:
        proc = subprocess.run(cmd, cwd=ROOT, check=False, capture_output=True, text=True)
    except OSError as e:
        _warn(f"failed to run {' '.join(cmd)}: {e}")
        return "", False
    text = (proc.stdout or "") + "\n" + (proc.stderr or "")
    ok = "Commands:" in text and "Options:" in text
    return text, ok


def parse_help_text(text: str) -> dict:
    commands: list[str] = []
    mod_subcommands: list[str] = []
    options: list[str] = []
    command_desc: dict[str, str] = {}
    mod_subcommand_desc: dict[str, str] = {}
    option_desc: dict[str, str] = {}

    section = None
    for raw in text.splitlines():
        line = raw.rstrip()
        if line.startswith("Commands:"):
            section = "commands"
            continue
        if line.startswith("Options:"):
            section = "options"
            continue
        if section == "commands":
            m = re.match(r"^\s{2,}([a-z][a-z0-9\-]*(?:\s+[a-z][a-z0-9\-]*)?)\s{2,}(.*)$", line)
            if not m:
                if line.strip() == "":
                    continue
                if not line.startswith("  "):
                    section = None
                continue
            token = m.group(1).strip()
            desc = m.group(2).strip()
            parts = token.split()
            if len(parts) == 1:
                commands.append(parts[0])
                if desc and parts[0] not in command_desc:
                    command_desc[parts[0]] = desc
            elif len(parts) == 2 and parts[0] == "mod":
                commands.append("mod")
                mod_subcommands.append(parts[1])
                if desc and parts[1] not in mod_subcommand_desc:
                    mod_subcommand_desc[parts[1]] = desc
        elif section == "options":
            if not line.startswith("  "):
                continue
            m = re.match(r"^\s{2,}(.+?)(?:\s{2,}(.*))?$", line)
            if not m:
                continue
            token_part = m.group(1).strip()
            desc = (m.group(2) or "").strip()
            found = re.findall(r"(?<!\w)(?:--[a-z0-9][a-z0-9\-]*|-h|-o|-O[0-3])\b", token_part)
            for tok in found:
                options.append(tok)
                if desc and tok not in option_desc:
                    option_desc[tok] = desc

    return {
        "commands": _sorted_unique(commands),
        "mod_subcommands": _sorted_unique(mod_subcommands),
        "options": _sorted_unique(options),
        "command_descriptions": command_desc,
        "mod_subcommand_descriptions": mod_subcommand_desc,
        "option_descriptions": option_desc,
    }


def parse_help_dynamic(spec: dict) -> dict:
    binary = resolve_vitte_binary()
    if binary is None:
        _warn("vitte binary not found; using static completion spec only")
        return {
            "dynamic_enabled": False,
            "commands": [],
            "mod_subcommands": [],
            "options": [],
            "command_descriptions": {},
            "mod_subcommand_descriptions": {},
            "option_descriptions": {},
            "command_options": {},
        }

    root_text, root_ok = run_help(binary, ["--help"])
    if not root_ok:
        _warn("could not parse vitte --help; falling back to static completion spec")
        return {
            "dynamic_enabled": False,
            "commands": [],
            "mod_subcommands": [],
            "options": [],
            "command_descriptions": {},
            "mod_subcommand_descriptions": {},
            "option_descriptions": {},
            "command_options": {},
        }

    root = parse_help_text(root_text)
    dyn_command_options: dict[str, list[str]] = {}
    global_opt_set = set(root.get("options", []))

    candidate_commands = _sorted_unique(root.get("commands", []) + spec.get("commands", []))
    candidate_mod_sub = _sorted_unique(root.get("mod_subcommands", []) + spec.get("mod_subcommands", []))

    for cmd in candidate_commands:
        text, ok = run_help(binary, [cmd, "--help"])
        if not ok:
            continue
        parsed = parse_help_text(text)
        opts = parsed.get("options", [])
        if opts and set(opts) != global_opt_set:
            dyn_command_options[cmd] = _sorted_unique(opts)

    # Probe mod subcommands (future-proof once CLI exposes subcommand-specific help).
    for sub in candidate_mod_sub:
        text, ok = run_help(binary, ["mod", sub, "--help"])
        if not ok:
            continue
        parsed = parse_help_text(text)
        opts = parsed.get("options", [])
        if opts and set(opts) != global_opt_set:
            dyn_command_options[f"mod {sub}"] = _sorted_unique(opts)

    return {
        "dynamic_enabled": True,
        "commands": root.get("commands", []),
        "mod_subcommands": root.get("mod_subcommands", []),
        "options": root.get("options", []),
        "command_descriptions": root.get("command_descriptions", {}),
        "mod_subcommand_descriptions": root.get("mod_subcommand_descriptions", {}),
        "option_descriptions": root.get("option_descriptions", {}),
        "command_options": dyn_command_options,
    }


def discover_modules(spec: dict, module_cfg: dict | None = None, max_items: int | None = None) -> list[str]:
    modules: list[str] = []
    cfg = module_cfg or {}
    roots = cfg.get("roots", spec.get("module_roots", ["src/vitte/packages"]))
    prefixes = cfg.get("prefixes", spec.get("module_prefixes", ["vitte"]))
    extra = cfg.get("values", spec.get("module_values", ["__root__"]))

    for extra_module in extra:
        modules.append(extra_module)

    for rel_root in roots:
        root = ROOT / rel_root
        if not root.exists():
            _warn(f"module root not found: {root}")
            continue
        for path in root.rglob("*.vit"):
            rel = path.relative_to(root).as_posix()
            if rel.endswith("/info.vit") or rel == "info.vit":
                continue
            if rel.endswith("/mod.vit"):
                mod = rel[: -len("/mod.vit")]
            elif rel == "mod.vit":
                mod = ""
            else:
                mod = rel[: -len(".vit")]
            if not mod:
                continue
            for prefix in prefixes:
                modules.append(f"{prefix}/{mod}")

    unique = _sorted_unique(modules)
    if max_items is not None and max_items > 0:
        return unique[:max_items]
    return unique


def merge(spec: dict, dyn: dict, max_module_suggestions: int | None = None) -> dict:
    out = copy.deepcopy(spec)

    out["commands"] = _sorted_unique(spec.get("commands", []) + dyn.get("commands", []))
    out["mod_subcommands"] = _sorted_unique(spec.get("mod_subcommands", []) + dyn.get("mod_subcommands", []))
    out["options"] = _sorted_unique(spec.get("options", []) + dyn.get("options", []))

    out["command_descriptions"] = {
        **dyn.get("command_descriptions", {}),
        **spec.get("command_descriptions", {}),
    }
    out["mod_subcommand_descriptions"] = {
        **dyn.get("mod_subcommand_descriptions", {}),
        **spec.get("mod_subcommand_descriptions", {}),
    }
    out["option_descriptions"] = {
        **dyn.get("option_descriptions", {}),
        **spec.get("option_descriptions", {}),
    }

    merged_command_options: dict[str, list[str]] = {}
    spec_map = spec.get("command_options", {})
    dyn_map = dyn.get("command_options", {})
    for key in set(spec_map.keys()) | set(dyn_map.keys()):
        merged_command_options[key] = _sorted_unique(spec_map.get(key, []) + dyn_map.get(key, []))
    out["command_options"] = merged_command_options

    option_args = copy.deepcopy(spec.get("option_args", {}))
    # Backward compatibility with previous schema keys.
    for opt, vals in spec.get("value_options", {}).items():
        option_args.setdefault(opt, {"type": "enum", "values": vals})
    for opt in spec.get("file_value_options", []):
        option_args.setdefault(opt, {"type": "file"})
    for opt in out["options"]:
        option_args.setdefault(opt, {"type": "flag"})
    out["option_args"] = option_args

    out["common_options"] = _sorted_unique(spec.get("common_options", []))

    for key in ("commands_exclude", "mod_subcommands_exclude", "options_exclude"):
        excluded = set(spec.get(key, []))
        if not excluded:
            continue
        target = key.replace("_exclude", "")
        out[target] = [v for v in out.get(target, []) if v not in excluded]

    all_options_set = set(out.get("options", []))
    cleaned_command_options: dict[str, list[str]] = {}
    for key, values in out.get("command_options", {}).items():
        cleaned = [v for v in values if v in all_options_set]
        if cleaned:
            cleaned_command_options[key] = _sorted_unique(cleaned)
    out["command_options"] = cleaned_command_options

    resolved_max_modules = (
        int(max_module_suggestions)
        if max_module_suggestions is not None
        else int(spec.get("max_module_suggestions", 200))
    )
    out["max_module_suggestions"] = resolved_max_modules
    out["module_values_generated"] = discover_modules(spec, max_items=resolved_max_modules)
    per_option: dict[str, list[str]] = {}
    for opt, cfg in spec.get("module_values_by_option", {}).items():
        per_option[opt] = discover_modules(spec, module_cfg=cfg, max_items=resolved_max_modules)
    out["module_values_by_option_generated"] = per_option
    out["dynamic_enabled"] = bool(dyn.get("dynamic_enabled", False))
    return out


def _header(spec_hash: str, schema_version: int, dynamic_enabled: bool) -> str:
    mode = "dynamic" if dynamic_enabled else "static-fallback"
    return (
        "# Auto-generated by tools/generate_completions.py. Do not edit manually.\n"
        f"# completion-generator-version: {GENERATOR_VERSION}\n"
        f"# completion-schema-version: {schema_version}\n"
        f"# completion-spec-sha256: {spec_hash}\n"
        f"# completion-help-mode: {mode}\n"
    )


def _quote_zsh(s: str) -> str:
    return s.replace("'", "'\\''")


def _context_cases_bash(command_options: dict, all_options: list[str]) -> str:
    normal_cases = []
    mod_cases = []
    for key in sorted(command_options.keys()):
        vals = shjoin(command_options[key])
        if not vals:
            continue
        if key.startswith("mod "):
            sub = key.split(" ", 1)[1]
            mod_cases.append(f'      {sub}) echo "{vals}"; return 0 ;;')
        elif key != "mod":
            normal_cases.append(f'    {key}) echo "{vals}"; return 0 ;;')
    mod_default = shjoin(command_options.get("mod", all_options))
    all_opts = shjoin(all_options)
    mod_block = "\n".join(mod_cases) if mod_cases else "      *) ;;"
    normal_block = "\n".join(normal_cases)
    return f"""_vitte_options_for_context() {{
  local cmd="$1" subcmd="$2"
  case "$cmd" in
{normal_block}
    mod)
      case "$subcmd" in
{mod_block}
      esac
      echo "{mod_default}"
      return 0
      ;;
  esac
  echo "{all_opts}"
}}
"""


def _value_case_bash(data: dict) -> str:
    lines = []
    for opt in sorted(data.get("option_args", {}).keys()):
        meta = data["option_args"][opt]
        typ = meta.get("type", "flag")
        if typ == "enum":
            vals = shjoin(meta.get("values", []))
            lines.append(f'    {opt}) COMPREPLY=( $(compgen -W "{vals}" -- "${{cur}}") ); return 0 ;;')
        elif typ == "module":
            vals = shjoin(
                data.get("module_values_by_option_generated", {}).get(opt, data.get("module_values_generated", []))
            )
            lines.append(f'    {opt}) COMPREPLY=( $(compgen -W "{vals}" -- "${{cur}}") ); return 0 ;;')
        elif typ == "file":
            lines.append(f'    {opt}) COMPREPLY=( $(compgen -f -- "${{cur}}") ); return 0 ;;')
        elif typ == "file_vit":
            lines.append(f"    {opt}) COMPREPLY=( $(compgen -f -X '!*.vit' -- \"${{cur}}\") ); return 0 ;;")
        elif typ == "dir":
            lines.append(f'    {opt}) COMPREPLY=( $(compgen -d -- "${{cur}}") ); return 0 ;;')
        elif typ == "int":
            lines.append(f"    {opt}) return 0 ;;")
    return "\n".join(lines)


def render_bash(data: dict, header: str) -> str:
    commands = shjoin(data["commands"])
    mod_sub = shjoin(data["mod_subcommands"])
    all_options = data["options"]
    vit_cmds = shjoin(data.get("vit_file_commands", []))
    common_opts = shjoin(data.get("common_options", []))
    value_case_block = _value_case_bash(data)
    context_helper = _context_cases_bash(data.get("command_options", {}), all_options)
    return f"""{header}{context_helper}_vitte_complete() {{
  local cur prev cmd subcmd opts
  COMPREPLY=()
  cur="${{COMP_WORDS[COMP_CWORD]}}"
  prev="${{COMP_WORDS[COMP_CWORD-1]}}"
  cmd="${{COMP_WORDS[1]:-}}"
  subcmd="${{COMP_WORDS[2]:-}}"

  case "${{prev}}" in
{value_case_block}
  esac

  if [[ ${{COMP_CWORD}} -eq 1 ]]; then
    COMPREPLY=( $(compgen -W "{commands} {common_opts}" -- "${{cur}}") )
    return 0
  fi

  if [[ "${{cmd}}" == "mod" && ${{COMP_CWORD}} -eq 2 ]]; then
    COMPREPLY=( $(compgen -W "{mod_sub}" -- "${{cur}}") )
    return 0
  fi

  if [[ " {vit_cmds} " == *" ${{cmd}} "* && "${{cur}}" != -* ]]; then
    COMPREPLY=( $(compgen -f -X '!*.vit' -- "${{cur}}") )
    return 0
  fi

  opts="$(_vitte_options_for_context "${{cmd}}" "${{subcmd}}")"
  COMPREPLY=( $(compgen -W "${{opts}}" -- "${{cur}}") )
  return 0
}}

complete -F _vitte_complete vitte
complete -F _vitte_complete vittec
complete -F _vitte_complete vitte-linker
"""


def _context_cases_zsh(data: dict) -> str:
    lines = []
    all_opts = " ".join(data.get("options", []))
    for key in sorted(data.get("command_options", {}).keys()):
        vals = " ".join(data["command_options"][key])
        lines.append(f'    "{key}") ctx_options=({vals}) ;;')
    lines.append(f'    *) ctx_options=({all_opts}) ;;')
    return "\n".join(lines)


def _value_case_zsh(data: dict) -> str:
    lines = []
    for opt in sorted(data.get("option_args", {}).keys()):
        meta = data["option_args"][opt]
        typ = meta.get("type", "flag")
        if typ == "enum":
            vals = " ".join(meta.get("values", []))
            lines.append(f"    {opt}) compadd -- {vals}; return ;;")
        elif typ == "module":
            module_values = " ".join(
                data.get("module_values_by_option_generated", {}).get(opt, data.get("module_values_generated", []))
            )
            lines.append(f"    {opt}) compadd -- {module_values}; return ;;")
        elif typ == "file":
            lines.append(f"    {opt}) _files; return ;;")
        elif typ == "file_vit":
            lines.append(f"    {opt}) _files -g '*.vit'; return ;;")
        elif typ == "dir":
            lines.append(f"    {opt}) _files -/; return ;;")
        elif typ == "int":
            lines.append(f"    {opt}) _message 'integer'; return ;;")
    return "\n".join(lines)


def render_zsh(data: dict, header: str) -> str:
    cmd_desc = data.get("command_descriptions", {})
    sub_desc = data.get("mod_subcommand_descriptions", {})
    opt_desc = data.get("option_descriptions", {})

    commands_desc = "\n".join(
        [f"    '{_quote_zsh(c)}:{_quote_zsh(cmd_desc.get(c, c))}'" for c in data["commands"]]
    )
    mod_desc = "\n".join(
        [f"    '{_quote_zsh(m)}:{_quote_zsh(sub_desc.get(m, m))}'" for m in data["mod_subcommands"]]
    )
    option_specs = "\n".join(
        [f"    '{_quote_zsh(o)}:{_quote_zsh(opt_desc.get(o, o))}'" for o in data["options"]]
    )

    context_cases = _context_cases_zsh(data)
    value_cases = _value_case_zsh(data)

    return f"""#compdef vitte vittec vitte-linker
{header}_vitte() {{
  local curcontext="$curcontext"
  local context
  local -a commands mod_subcommands option_specs ctx_options

  commands=(
{commands_desc}
  )
  mod_subcommands=(
{mod_desc}
  )
  option_specs=(
{option_specs}
  )

  if (( CURRENT == 2 )); then
    _describe -t commands 'vitte command' commands
    return
  fi

  if [[ "${{words[2]}}" == "mod" && CURRENT == 3 ]]; then
    _describe -t mod_subcommands 'mod command' mod_subcommands
    return
  fi

  case "${{words[CURRENT-1]}}" in
{value_cases}
  esac

  if [[ "${{words[2]}}" == "mod" && -n "${{words[3]:-}}" ]]; then
    context="mod ${{words[3]}}"
  else
    context="${{words[2]:-}}"
  fi

  case "$context" in
{context_cases}
  esac

  if [[ "${{words[CURRENT]}}" == -* ]]; then
    _describe -t options 'vitte option' option_specs
    return
  fi

  compadd -- $ctx_options
}}

_vitte "$@"
"""


def _fish_context_expr(context_key: str) -> str:
    if context_key.startswith("mod "):
        sub = context_key.split(" ", 1)[1]
        return f'"__fish_seen_subcommand_from mod; and __fish_seen_subcommand_from {sub}"'
    if context_key == "mod":
        return '"__fish_seen_subcommand_from mod"'
    return f'"__fish_seen_subcommand_from {context_key}"'


def _fish_opt_flags(opt: str) -> str:
    if opt.startswith("--"):
        return f"-l {opt[2:]}"
    if opt.startswith("-") and len(opt) == 2:
        return f"-s {opt[1:]}"
    return f"-a {opt}"


def render_fish(data: dict, header: str) -> str:
    cmd_desc = data.get("command_descriptions", {})
    sub_desc = data.get("mod_subcommand_descriptions", {})
    opt_desc = data.get("option_descriptions", {})

    lines = [header.rstrip("\n")]

    for tool in ("vitte", "vittec", "vitte-linker"):
        for cmd in data["commands"]:
            desc = cmd_desc.get(cmd, cmd)
            lines.append(f'complete -c {tool} -n "__fish_use_subcommand" -a "{cmd}" -d "{desc}"')

    for tool in ("vitte", "vittec", "vitte-linker"):
        for sub in data["mod_subcommands"]:
            desc = sub_desc.get(sub, sub)
            lines.append(
                f'complete -c {tool} -n "__fish_seen_subcommand_from mod; and __fish_use_subcommand" -a "{sub}" -d "{desc}"'
            )

    for tool in ("vitte", "vittec", "vitte-linker"):
        for ctx, options in sorted(data.get("command_options", {}).items()):
            cond = _fish_context_expr(ctx)
            for opt in options:
                meta = data.get("option_args", {}).get(opt, {"type": "flag"})
                typ = meta.get("type", "flag")
                desc = opt_desc.get(opt, opt).replace('"', "'")
                flags = _fish_opt_flags(opt)
                if typ == "enum":
                    vals = " ".join(meta.get("values", []))
                    lines.append(f'complete -c {tool} -n {cond} {flags} -r -a "{vals}" -d "{desc}"')
                elif typ == "module":
                    vals = " ".join(
                        data.get("module_values_by_option_generated", {}).get(opt, data.get("module_values_generated", []))
                    )
                    lines.append(f'complete -c {tool} -n {cond} {flags} -r -a "{vals}" -d "{desc}"')
                elif typ == "file":
                    lines.append(f'complete -c {tool} -n {cond} {flags} -r -a "(__fish_complete_path)" -d "{desc}"')
                elif typ == "file_vit":
                    lines.append(f'complete -c {tool} -n {cond} {flags} -r -a "(__fish_complete_suffix .vit)" -d "{desc}"')
                elif typ == "dir":
                    lines.append(f'complete -c {tool} -n {cond} {flags} -r -a "(__fish_complete_directories)" -d "{desc}"')
                elif typ == "int":
                    lines.append(f'complete -c {tool} -n {cond} {flags} -r -d "{desc}"')
                else:
                    lines.append(f'complete -c {tool} -n {cond} {flags} -d "{desc}"')

    return "\n".join(lines) + "\n"


def write_or_check(path: Path, content: str, check: bool) -> bool:
    if check:
        if not path.exists():
            print(f"[check] missing: {path}", file=sys.stderr)
            return False
        current = path.read_text(encoding="utf-8")
        if current != content:
            print(f"[check] mismatch: {path}", file=sys.stderr)
            return False
        return True
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")
    return True


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true", help="verify files are up to date")
    parser.add_argument(
        "--max-module-suggestions",
        type=int,
        default=None,
        help="override maximum number of generated module completion entries",
    )
    parser.add_argument(
        "--print-mode",
        action="store_true",
        help="print detected help mode (dynamic|static-fallback) and exit",
    )
    args = parser.parse_args()

    spec = load_spec()
    dyn = parse_help_dynamic(spec)
    data = merge(spec, dyn, max_module_suggestions=args.max_module_suggestions)

    header = _header(
        spec_hash=_stable_hash(spec),
        schema_version=int(spec.get("schema_version", 1)),
        dynamic_enabled=bool(data.get("dynamic_enabled", False)),
    )

    if args.print_mode:
        print("dynamic" if data.get("dynamic_enabled", False) else "static-fallback")
        return 0

    ok = True
    ok &= write_or_check(BASH_OUT, render_bash(data, header), args.check)
    ok &= write_or_check(ZSH_OUT, render_zsh(data, header), args.check)
    ok &= write_or_check(FISH_OUT, render_fish(data, header), args.check)
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
