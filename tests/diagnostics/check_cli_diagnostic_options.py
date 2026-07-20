#!/usr/bin/env python3
from __future__ import annotations

import re
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
BIN = ROOT / "bin" / "vitte"
FIXTURE = ROOT / "tests" / "diagnostics" / "negative" / "symbol_resolution" / "unknown_identifier.vit"
BEGINNER_FIXTURE = ROOT / "target" / "diagnostic-beginner-missing-call.vit"


def run(*args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [str(BIN), *args],
        cwd=str(ROOT),
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )


def clean(text: str) -> str:
    return re.sub(r"\x1b\[[0-9;]*m", "", text)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"[cli-diagnostic-options][error] {message}")


def main() -> int:
    check = run("check", str(FIXTURE), "--color", "never", "--max-errors", "1", "--diagnostic-width", "72")
    build_out = ROOT / "target" / "diagnostic-options-build"
    build = run("build", str(FIXTURE), "-o", str(build_out), "--color=never", "--max-errors=1")

    check_text = clean(check.stdout)
    build_text = clean(build.stdout)
    require(check.returncode != 0, "check accepted diagnostic fixture")
    require(build.returncode != 0, "build accepted diagnostic fixture")
    for needle in (
        "error[SEMA_E_UNKNOWN_IDENTIFIER] symbol_resolution: unknown identifier",
        "missing_negative_symbol",
        "name not found in this scope",
    ):
        require(needle in check_text, f"check missing {needle!r}\n{check_text}")
        require(needle in build_text, f"build missing {needle!r}\n{build_text}")
    require("\x1b[" not in check.stdout, "--color never emitted ANSI escape sequences")

    json_result = run("check", str(FIXTURE), "--error-format", "json")
    require(json_result.returncode != 0, "--error-format json accepted diagnostic fixture")
    require('"code":"SEMA_E_UNKNOWN_IDENTIFIER"' in json_result.stdout, "json format missing diagnostic code")

    allowed = run("check", str(FIXTURE), "--allow", "SEMA_E_UNKNOWN_IDENTIFIER")
    require(allowed.returncode != 0, "--allow suppressed a structural error")
    require("error[SEMA_E_UNKNOWN_IDENTIFIER]" in clean(allowed.stdout), "--allow changed structural error output")

    warned = run("check", str(FIXTURE), "--warn", "SEMA_E_UNKNOWN_IDENTIFIER", "--color", "never")
    require(warned.returncode != 0, "--warn downgraded a structural error")
    require("error[SEMA_E_UNKNOWN_IDENTIFIER]" in clean(warned.stdout), "--warn changed structural error severity")

    denied_warning = run("check", str(FIXTURE), "--warn", "SEMA_E_UNKNOWN_IDENTIFIER", "--deny-warnings")
    require(denied_warning.returncode != 0, "--deny-warnings accepted an invalid compilation")

    denied = run("check", str(FIXTURE), "--allow", "SEMA_E_UNKNOWN_IDENTIFIER", "--deny", "SEMA_E_UNKNOWN_IDENTIFIER")
    require(denied.returncode != 0, "--deny should override --allow for the same diagnostic code")

    allow_structural_group = run("check", str(FIXTURE), "--allow", "structural")
    require(allow_structural_group.returncode != 0, "--allow structural suppressed a structural error")

    warn_style_group = run("check", "tests/check/main.vit", "--warn", "style", "--allow", "unused")
    require(warn_style_group.returncode == 0, "warning groups should not affect valid code")

    BEGINNER_FIXTURE.parent.mkdir(parents=True, exist_ok=True)
    BEGINNER_FIXTURE.write_text(
        "space diagnostics/beginner/missing_call\n\n"
        "proc main() -> int {\n"
        "  give missing_call()\n"
        "}\n",
        encoding="utf-8",
    )
    beginner = run("check", str(BEGINNER_FIXTURE), "--color", "never")
    beginner_text = clean(beginner.stdout)
    require(beginner.returncode != 0, "missing procedure call compiled successfully")
    require("error[SEMA_E_UNKNOWN_FUNCTION] symbol_resolution: unknown procedure" in beginner_text, "beginner title is not short or consistent")
    require("the call names missing_call" in beginner_text, "beginner label missing code-oriented detail")
    require("look for a proc declaration" in beginner_text, "beginner note missing declaration guidance")
    require("declare proc missing_call" in beginner_text, "beginner help missing concrete correction")
    human_beginner_text = beginner_text.replace("SEMA_E_UNKNOWN_FUNCTION", "")
    require("function" not in human_beginner_text.lower(), "beginner diagnostic used inconsistent terminology")

    print("[cli-diagnostic-options] checked check/build diagnostic options")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
