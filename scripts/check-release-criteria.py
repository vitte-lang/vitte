#!/usr/bin/env python3
"""Fail release gates when diagnostics or native compiler routes are unowned."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ERRORS: list[str] = []
NATIVE_ORIGINS = {
    "Unknown", "Io", "Scanner", "Lexer", "Parser", "Import",
    "NameResolution", "TypeCheck", "Contract", "ConstantEval", "Hir",
    "Ir", "Codegen", "C17Backend", "CCompiler", "Linker", "Runtime",
    "Driver", "Internal",
}
NATIVE_SEVERITIES = {"Note", "Help", "Warning", "Error", "Fatal"}
C_ORIGINS = {
    "VITTE_DIAGNOSTIC_ORIGIN_UNKNOWN", "VITTE_DIAGNOSTIC_ORIGIN_IO",
    "VITTE_DIAGNOSTIC_ORIGIN_SCANNER", "VITTE_DIAGNOSTIC_ORIGIN_LEXER",
    "VITTE_DIAGNOSTIC_ORIGIN_PARSER", "VITTE_DIAGNOSTIC_ORIGIN_IMPORT",
    "VITTE_DIAGNOSTIC_ORIGIN_NAME_RESOLUTION", "VITTE_DIAGNOSTIC_ORIGIN_TYPE_CHECK",
    "VITTE_DIAGNOSTIC_ORIGIN_CONTRACT", "VITTE_DIAGNOSTIC_ORIGIN_CONSTANT_EVAL",
    "VITTE_DIAGNOSTIC_ORIGIN_HIR", "VITTE_DIAGNOSTIC_ORIGIN_IR",
    "VITTE_DIAGNOSTIC_ORIGIN_CODEGEN", "VITTE_DIAGNOSTIC_ORIGIN_C17_BACKEND",
    "VITTE_DIAGNOSTIC_ORIGIN_C_COMPILER", "VITTE_DIAGNOSTIC_ORIGIN_LINKER",
    "VITTE_DIAGNOSTIC_ORIGIN_RUNTIME", "VITTE_DIAGNOSTIC_ORIGIN_DRIVER",
    "VITTE_DIAGNOSTIC_ORIGIN_INTERNAL",
}
C_SEVERITIES = {
    "VITTE_DIAGNOSTIC_NOTE", "VITTE_DIAGNOSTIC_HELP",
    "VITTE_DIAGNOSTIC_WARNING", "VITTE_DIAGNOSTIC_ERROR",
    "VITTE_DIAGNOSTIC_FATAL",
}


def fail(message: str) -> None:
    ERRORS.append(message)


def read(relative: str) -> str:
    path = ROOT / relative
    try:
        return path.read_text(encoding="utf-8")
    except OSError as error:
        fail(f"cannot read {relative}: {error}")
        return ""


def strip_comments(source: str) -> str:
    """Remove C/Vitte comments while preserving strings and line positions."""
    output: list[str] = []
    index = 0
    quote: str | None = None
    escaped = False
    while index < len(source):
        char = source[index]
        next_char = source[index + 1] if index + 1 < len(source) else ""
        if quote is not None:
            output.append(char)
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == quote:
                quote = None
            index += 1
            continue
        if char in ('"', "'"):
            quote = char
            output.append(char)
            index += 1
            continue
        if char == "/" and next_char == "/":
            output.extend("  ")
            index += 2
            while index < len(source) and source[index] != "\n":
                output.append(" ")
                index += 1
            continue
        if char == "/" and next_char == "*":
            output.extend("  ")
            index += 2
            while index < len(source):
                if source[index] == "*" and index + 1 < len(source) and source[index + 1] == "/":
                    output.extend("  ")
                    index += 2
                    break
                output.append("\n" if source[index] == "\n" else " ")
                index += 1
            continue
        output.append(char)
        index += 1
    return "".join(output)


def fields(entry: str, names: tuple[str, ...]) -> dict[str, str]:
    result: dict[str, str] = {}
    for name in names:
        match = re.search(rf"\b{name}\s*:\s*\"([^\"]*)\"", entry)
        if match:
            result[name] = match.group(1)
    origin = re.search(r"\borigin\s*:\s*DiagnosticOrigin\.(\w+)", entry)
    if origin:
        result["origin"] = origin.group(1)
    severity = re.search(r"\bdefault_severity\s*:\s*DiagnosticSeverity\.(\w+)", entry)
    if severity:
        result["default_severity"] = severity.group(1)
    return result


def parse_vitte_registry(source: str) -> dict[str, str]:
    source = strip_comments(source)
    codes: dict[str, str] = {}
    entries = re.findall(r"DiagnosticRegistryEntry\s*\{([^{}]*)\}", source)
    if not entries:
        fail("native diagnostic registry contains no entries")
    for entry in entries:
        if not re.search(r'internal_code\s*:\s*"', entry):
            continue
        value = fields(entry, ("internal_code", "public_code", "category", "title", "explanation"))
        required = ("internal_code", "public_code", "category", "title", "explanation", "origin", "default_severity")
        missing = [name for name in required if not value.get(name)]
        if missing:
            fail(f"native registry entry is missing {', '.join(missing)}: {entry.strip()[:100]}")
            continue
        internal = value["internal_code"]
        public = value["public_code"]
        if not re.fullmatch(r"[EWNH]\d{4}", public):
            fail(f"native registry has malformed public code {public!r} for {internal}")
        if value["origin"] not in NATIVE_ORIGINS or value["origin"] == "Unknown":
            fail(f"native registry entry {internal} has invalid origin {value['origin']}")
        if value["default_severity"] not in NATIVE_SEVERITIES:
            fail(f"native registry entry {internal} has invalid severity {value['default_severity']}")
        if internal in codes:
            fail(f"native registry duplicates internal code {internal}")
        if public in codes.values():
            fail(f"native registry duplicates public code {public}")
        codes[internal] = public
    return codes


def split_arguments(value: str) -> list[str]:
    arguments: list[str] = []
    start = 0
    depth = 0
    quote: str | None = None
    escaped = False
    for index, char in enumerate(value):
        if quote is not None:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == quote:
                quote = None
            continue
        if char in ('"', "'"):
            quote = char
        elif char in "([{":
            depth += 1
        elif char in ")]}":
            depth -= 1
        elif char == "," and depth == 0:
            arguments.append(value[start:index].strip())
            start = index + 1
    arguments.append(value[start:].strip())
    return arguments


def macro_calls(source: str, macro: str):
    for match in re.finditer(rf"\b{macro}\s*\(", source):
        open_index = source.find("(", match.start())
        depth = 0
        quote: str | None = None
        escaped = False
        for index in range(open_index, len(source)):
            char = source[index]
            if quote is not None:
                if escaped:
                    escaped = False
                elif char == "\\":
                    escaped = True
                elif char == quote:
                    quote = None
                continue
            if char in ('"', "'"):
                quote = char
            elif char == "(":
                depth += 1
            elif char == ")":
                depth -= 1
                if depth == 0:
                    yield split_arguments(source[open_index + 1:index])
                    break


def unquote(value: str) -> str:
    value = value.strip()
    if len(value) >= 2 and value[0] == value[-1] == '"':
        return value[1:-1]
    return value


def parse_c_registry(source: str) -> dict[str, str]:
    source = strip_comments(source)
    begin = source.find("vitte_registry_entries[] = {")
    if begin < 0:
        fail("bootstrap diagnostic registry table was not found")
        return {}
    source = source[begin:]
    end = source.find("\n};")
    if end >= 0:
        source = source[:end]
    codes: dict[str, str] = {}
    for macro in ("ENTRY", "WARNING_ENTRY"):
        for arguments in macro_calls(source, macro):
            expected = 7 if macro == "ENTRY" else 6
            if len(arguments) != expected:
                # The macro definition is before the registry table; all calls
                # in the table must have the public registry arity.
                fail(f"bootstrap registry {macro} has {len(arguments)} arguments, expected {expected}")
                continue
            internal = unquote(arguments[0])
            public = unquote(arguments[1])
            origin_index = 3 if macro == "ENTRY" else 2
            origin = arguments[origin_index].strip()
            if origin not in C_ORIGINS or origin == "VITTE_DIAGNOSTIC_ORIGIN_UNKNOWN":
                fail(f"bootstrap registry entry {internal} has invalid origin {origin}")
            if macro == "ENTRY" and arguments[2].strip() not in C_SEVERITIES:
                fail(f"bootstrap registry entry {internal} has invalid severity {arguments[2].strip()}")
            metadata = [unquote(argument) for argument in arguments[origin_index + 1:]]
            if not internal or not re.fullmatch(r"[EWNH]\d{4}", public):
                fail(f"bootstrap registry has malformed code pair {internal!r} / {public!r}")
                continue
            if "UNKNOWN" in origin.upper() or not origin:
                fail(f"bootstrap registry entry {internal} has unknown origin")
            if any(not item for item in metadata):
                fail(f"bootstrap registry entry {internal} has empty metadata")
            if internal in codes:
                fail(f"bootstrap registry duplicates internal code {internal}")
            if public in codes.values():
                fail(f"bootstrap registry duplicates public code {public}")
            codes[internal] = public
    if not codes:
        fail("bootstrap diagnostic registry contains no parsed entries")
    return codes


def parse_help_codes(source: str) -> set[str]:
    source = strip_comments(source)
    match = re.search(r"const\s+HELP_RULE_CODES\s*:\s*\[string\]\s*=\s*\[(.*?)\];", source, re.S)
    if not match:
        fail("diagnostic help code inventory was not found")
        return set()
    codes = re.findall(r'"([EWNH]\d{4})"', match.group(1))
    if len(codes) != len(set(codes)):
        fail("diagnostic help code inventory contains duplicates")
    return set(codes)


def check_diagnostic_ownership(config: dict) -> int:
    native = parse_vitte_registry(read(config["native"]))
    bootstrap = parse_c_registry(read(config["bootstrap"]))
    native_pairs = set(native.items())
    bootstrap_pairs = set(bootstrap.items())
    for internal, public in sorted(native_pairs - bootstrap_pairs):
        fail(f"bootstrap registry does not own native code {internal} ({public})")
    for internal, public in sorted(bootstrap_pairs - native_pairs):
        fail(f"native registry does not own bootstrap code {internal} ({public})")

    help_codes = parse_help_codes(read(config["help"]))
    registry_codes = set(native.values())
    for code in sorted(registry_codes - help_codes):
        fail(f"diagnostic {code} has no contextual-help policy")
    for code in sorted(help_codes - registry_codes):
        fail(f"contextual-help policy refers to unregistered diagnostic {code}")

    known_internal = set(native)
    known_public = set(native.values())
    source_files = []
    for base in (ROOT / "src/bootstrap", ROOT / "src/compiler"):
        for path in base.rglob("*"):
            if not path.is_file() or path.suffix not in {".c", ".h", ".vitte"}:
                continue
            relative = path.relative_to(ROOT).as_posix()
            if ".before-" in path.name or relative.endswith("/diagnostic.h"):
                continue
            if relative in {config["bootstrap"], config["native"]}:
                continue
            source_files.append(path)

    internal_pattern = re.compile(
        r"\b(VITTE_[A-Z0-9_]+_(?:E|W|N|H)_[A-Z0-9_]+|VITTE_(?:WARNING|NOTE)_[A-Z0-9_]+)\b"
    )
    public_pattern = re.compile(r"\b[EWNH]\d{4}\b")
    for path in source_files:
        text = strip_comments(path.read_text(encoding="utf-8"))
        relative = path.relative_to(ROOT).as_posix()
        for internal in sorted(set(internal_pattern.findall(text)) - known_internal):
            fail(f"{relative} uses unowned diagnostic identifier {internal}")
        for public in sorted(set(public_pattern.findall(text)) - known_public):
            fail(f"{relative} refers to unregistered public diagnostic {public}")
        if path.suffix in {".c", ".h"}:
            diagnostic_calls = (
                ("vitte_main_render_diagnostic", 2),
                ("vitte_main_report_process_error", 0),
                ("vitte_api_emit_pipeline_diagnostic", 3),
            )
            for function, code_index in diagnostic_calls:
                for arguments in macro_calls(text, function):
                    if len(arguments) <= code_index:
                        fail(f"{relative} has malformed {function} diagnostic call")
                        continue
                    code = unquote(arguments[code_index])
                    if code.startswith("VITTE_") and code not in known_internal:
                        fail(f"{relative} emits unowned diagnostic identifier {code}")
        if path.suffix == ".vitte":
            for code in re.findall(r'native_pipeline_c_string\s*\(\s*"(VITTE_[A-Z0-9_]+)"', text):
                if code not in known_internal:
                    fail(f"{relative} emits unowned diagnostic identifier {code}")
    return len(native)


def make_target_body(makefile: str, target: str) -> str | None:
    lines = makefile.splitlines()
    for index, line in enumerate(lines):
        if re.match(rf"^{re.escape(target)}\s*:", line):
            body = [line]
            while body[-1].rstrip().endswith("\\") and index + 1 < len(lines):
                index += 1
                body.append(lines[index])
            return "\n".join(body)
    return None


def check_compiler_paths(config: dict) -> int:
    pipeline_path = "src/compiler/driver/pipeline.vitte"
    pipeline = strip_comments(read(pipeline_path))
    makefile = read("Makefile")
    action_names = set(re.findall(r"NATIVE_FRONTEND_ACTION_(\w+)\s*:\s*[^=]+=", pipeline))
    expected_actions = {item["action"] for item in config["compiler_actions"]}
    for action in sorted(action_names - expected_actions):
        fail(f"native compiler action {action} has no release-path witness")
    for action in sorted(expected_actions - action_names):
        fail(f"release-path inventory refers to missing native compiler action {action}")
    for action in sorted(expected_actions & action_names):
        if f"NATIVE_FRONTEND_ACTION_{action}" not in pipeline.split("NATIVE_FRONTEND_ACTION_" + action, 1)[-1]:
            fail(f"native compiler action {action} is declared but has no executable use")

    function_match = re.search(
        r"proc\s+native_pipeline_requires_ast_codegen\([^)]*\)\s*->\s*bool\s*\{(.*?)\n\}",
        pipeline,
        re.S,
    )
    if not function_match:
        fail("native AST fallback selector was not found")
        fallback_nodes: set[str] = set()
    else:
        fallback_nodes = set(re.findall(r"ASTNodeKind\.(\w+)", function_match.group(1)))
    if "c17_emit_ast(&*((*input).parser))" not in pipeline or "c17_emit_ir(&*((*input).ir))" not in pipeline:
        fail("native C17 dispatch no longer has checked AST and IR emitter routes")
    if "native_pipeline_requires_ast_codegen(&parser)" not in pipeline:
        fail("native compile pipeline no longer classifies AST fallback paths")
    expected_nodes = {item["node_kind"] for item in config["ast_fallbacks"]}
    for node in sorted(fallback_nodes - expected_nodes):
        fail(f"AST fallback for {node} has no release-path witness")
    for node in sorted(expected_nodes - fallback_nodes):
        fail(f"release-path inventory refers to missing AST fallback {node}")

    power_body = make_target_body(makefile, "compiler-power-gate") or ""
    for item in config["compiler_actions"]:
        target = item["target"]
        target_body = make_target_body(makefile, target)
        if target_body is None:
            fail(f"compiler action {item['action']} references missing Make target {target}")
        elif not re.search(rf"(?m)^\s*{re.escape(target)}\s*(?:\\)?$", power_body):
            fail(f"compiler action {item['action']} target {target} is not included in compiler-power-gate")
        witness = read(item["witness"])
        for marker in item["markers"]:
            if marker not in witness:
                fail(f"compiler action {item['action']} witness {item['witness']} is missing {marker!r}")

    for item in config["ast_fallbacks"]:
        target_body = make_target_body(makefile, item["target"])
        if target_body is None:
            fail(f"AST fallback {item['node_kind']} references missing Make target {item['target']}")
        elif not re.search(rf"(?m)^\s*{re.escape(item['target'])}\s*(?:\\)?$", power_body):
            fail(f"AST fallback {item['node_kind']} target {item['target']} is not included in compiler-power-gate")
        witness = read(item["witness"])
        for marker in (item["case"], item["assertion"]):
            if marker not in witness:
                fail(f"AST fallback {item['node_kind']} witness {item['witness']} is missing {marker!r}")

    release_body = make_target_body(makefile, "compiler-release-criteria")
    release_recipe = re.search(
        r"(?ms)^compiler-release-criteria\s*:\s*\n((?:[ \t].*\n)*)",
        makefile,
    )
    if release_body is None or release_recipe is None or "check-release-criteria.py" not in release_recipe.group(1):
        fail("compiler-release-criteria target does not run the release criteria checker")
    if "compiler-release-criteria" not in power_body:
        fail("compiler-power-gate does not depend on compiler-release-criteria")
    full_gate = make_target_body(makefile, "compiler-gate") or ""
    if "compiler-release-criteria" not in full_gate:
        fail("compiler-gate does not include compiler-release-criteria")
    workflow = read(".github/workflows/ci.yml")
    if "make compiler-power-gate" not in workflow:
        fail("compiler CI workflow does not run compiler-power-gate")
    return len(action_names) + len(fallback_nodes)


def main() -> int:
    config_path = ROOT / "scripts/release_criteria.json"
    try:
        config = json.loads(config_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        print(f"release criteria: cannot read inventory: {error}", file=sys.stderr)
        return 2

    diagnostic_count = check_diagnostic_ownership(config["diagnostic_registry"])
    path_count = check_compiler_paths(config)
    if ERRORS:
        print(f"Release criteria failed with {len(ERRORS)} issue(s):", file=sys.stderr)
        for error in ERRORS:
            print(f"  - {error}", file=sys.stderr)
        return 1
    print(
        "Release criteria passed: "
        f"{diagnostic_count} owned diagnostics; {path_count} checked compiler paths."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
