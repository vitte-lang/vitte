#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
BIN = ROOT / "bin" / "vitte"
FIXTURE = "tests/pipeline/hello_world.vit"
REPORT = ROOT / "target" / "reports" / "cli_early_diagnostics.json"
RAW_ERROR_MARKER = b"[vitte][error]"
RAW_IO_MARKER = b"E_CLI_IO: cannot read"


@dataclass(frozen=True)
class Probe:
    probe_id: str
    argv: tuple[str, ...]
    surface: str
    expected_code: str
    expected_locale: str
    env_locale: str | None


PROBES = (
    Probe(
        "unknown-command-json-prescan",
        ("unknown-command", "--diagnostics-json", "--lang", "fr"),
        "json",
        "E_CLI_UNKNOWN_COMMAND",
        "fr",
        "en",
    ),
    Probe(
        "unknown-option-lsp-prescan",
        ("build", FIXTURE, "--unknown-option", "--diagnostics-lsp", "--lang", "fr"),
        "lsp",
        "E_CLI_UNKNOWN_OPTION",
        "fr",
        "en",
    ),
    Probe(
        "invalid-format-preserves-json",
        ("build", FIXTURE, "--diagnostics-json", "--format", "yaml", "--lang", "fr"),
        "json",
        "E_CLI_INVALID_ARGUMENT",
        "fr",
        "en",
    ),
    Probe(
        "invalid-format-prescans-lsp",
        ("build", FIXTURE, "--format", "yaml", "--diagnostics-lsp", "--lang", "fr"),
        "lsp",
        "E_CLI_INVALID_ARGUMENT",
        "fr",
        "en",
    ),
    Probe(
        "explicit-lang-overrides-environment",
        ("build", FIXTURE, "--diagnostics-json", "--lang", "en"),
        "json",
        "E_CLI_MISSING_ARG",
        "en",
        "fr",
    ),
    Probe(
        "environment-lang",
        ("build", FIXTURE, "--diagnostics-json"),
        "json",
        "E_CLI_MISSING_ARG",
        "fr",
        "fr",
    ),
    Probe(
        "default-lang",
        ("build", FIXTURE, "--diagnostics-json"),
        "json",
        "E_CLI_MISSING_ARG",
        "en",
        None,
    ),
    Probe(
        "missing-lang-value-json",
        ("build", FIXTURE, "--lang", "--diagnostics-json"),
        "json",
        "E_CLI_MISSING_ARG",
        "fr",
        "fr",
    ),
    Probe(
        "fr-fr-alias",
        ("build", FIXTURE, "--diagnostics-json", "--lang", "fr-FR"),
        "json",
        "E_CLI_MISSING_ARG",
        "fr",
        "en",
    ),
    Probe(
        "pt-br-environment-alias",
        ("build", FIXTURE, "--diagnostics-json"),
        "json",
        "E_CLI_MISSING_ARG",
        "pt-BR",
        "pt_BR",
    ),
    Probe(
        "zh-cn-environment-alias",
        ("build", FIXTURE, "--diagnostics-json"),
        "json",
        "E_CLI_MISSING_ARG",
        "zh-CN",
        "zh_CN",
    ),
    Probe(
        "empty-environment-falls-back-to-en",
        ("build", FIXTURE, "--diagnostics-json"),
        "json",
        "E_CLI_MISSING_ARG",
        "en",
        "",
    ),
    Probe(
        "unknown-environment-falls-back-to-en",
        ("build", FIXTURE, "--diagnostics-json"),
        "json",
        "E_CLI_MISSING_ARG",
        "en",
        "xx-UNKNOWN",
    ),
    Probe(
        "last-explicit-language-wins",
        ("build", FIXTURE, "--diagnostics-json", "--lang", "en", "--lang", "fr"),
        "json",
        "E_CLI_MISSING_ARG",
        "fr",
        "en",
    ),
)


def catalog_message(locale: str, code: str) -> str:
    catalog = ROOT / "locales" / locale / "diagnostics.ftl"
    prefix = f"{code} ="
    for line in catalog.read_text(encoding="utf-8").splitlines():
        if line.startswith(prefix):
            return line.split("=", 1)[1].strip()
    raise RuntimeError(f"missing {code} in {catalog.relative_to(ROOT)}")


def diagnostic_from_payload(payload: dict[str, Any], surface: str) -> dict[str, Any] | None:
    if surface == "json":
        diagnostics = payload.get("primary_report", {}).get("diagnostics", [])
    else:
        if payload.get("jsonrpc") != "2.0" or payload.get("method") != "textDocument/publishDiagnostics":
            return None
        diagnostics = payload.get("params", {}).get("diagnostics", [])
    if not isinstance(diagnostics, list) or len(diagnostics) != 1 or not isinstance(diagnostics[0], dict):
        return None
    return diagnostics[0]


def run_probe(probe: Probe) -> dict[str, Any]:
    env = os.environ.copy()
    if probe.env_locale is None:
        env.pop("VITTE_LANG", None)
    else:
        env["VITTE_LANG"] = probe.env_locale
    proc = subprocess.run(
        [str(BIN), *probe.argv],
        cwd=ROOT,
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    failures: list[str] = []
    payload: dict[str, Any] | None = None
    diagnostic: dict[str, Any] | None = None
    if proc.returncode == 0:
        failures.append("invalid CLI request exited with zero")
    if proc.stderr:
        failures.append("structured diagnostic wrote to stderr")
    try:
        decoded = json.loads(proc.stdout)
        if isinstance(decoded, dict):
            payload = decoded
        else:
            failures.append("structured diagnostic root is not an object")
    except json.JSONDecodeError as exc:
        failures.append(f"structured diagnostic is not parseable JSON: {exc}")
    if payload is not None:
        diagnostic = diagnostic_from_payload(payload, probe.surface)
        if diagnostic is None:
            failures.append(f"invalid {probe.surface} diagnostic envelope")
    if diagnostic is not None:
        if diagnostic.get("code") != probe.expected_code:
            failures.append(f"expected {probe.expected_code}, got {diagnostic.get('code')!r}")
        expected_message = catalog_message(probe.expected_locale, probe.expected_code)
        if diagnostic.get("message") != expected_message:
            failures.append(
                f"expected {probe.expected_locale} message {expected_message!r}, got {diagnostic.get('message')!r}"
            )
    return {
        "id": probe.probe_id,
        "argv": ["bin/vitte", *probe.argv],
        "surface": probe.surface,
        "expected_code": probe.expected_code,
        "expected_locale": probe.expected_locale,
        "environment_locale": probe.env_locale,
        "exit_code": proc.returncode,
        "stdout": proc.stdout,
        "stderr": proc.stderr,
        "failures": failures,
        "passed": not failures,
    }


def runtime_state() -> str:
    if not BIN.is_file():
        return "binary-missing"
    data = BIN.read_bytes()
    if RAW_ERROR_MARKER in data or RAW_IO_MARKER in data:
        return "runtime-obsolete"
    if b"_command_build" in data and b"run_cli_main" not in data:
        return "runtime-obsolete"
    return "runtime-current"


def main() -> int:
    rows: list[dict[str, Any]] = []
    blocker = ""
    state = runtime_state()
    if not BIN.is_file():
        blocker = "runtime compiler artifact is missing"
    else:
        rows = [run_probe(probe) for probe in PROBES]
    failure_count = sum(len(row["failures"]) for row in rows)
    if blocker:
        failure_count = 1
    elif state == "runtime-obsolete":
        blocker = "runtime compiler artifact does not execute the current Vitte driver"
        failure_count += 1
    report = {
        "schema": "vitte.cli-early-diagnostics.v1",
        "binary": "bin/vitte",
        "runtime_state": state,
        "probe_count": len(PROBES),
        "passed_count": sum(1 for row in rows if row["passed"]),
        "failure_count": failure_count,
        "blocker": blocker,
        "probes": rows,
        "status": "ok" if failure_count == 0 else "failed",
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    if failure_count:
        print(
            f"[cli-early-diagnostics][error] passed={report['passed_count']}/{len(PROBES)} "
            f"failures={failure_count}; report={REPORT.relative_to(ROOT)}",
            file=sys.stderr,
        )
        return 1
    print(f"[cli-early-diagnostics] passed={len(PROBES)}/{len(PROBES)} report={REPORT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
