#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path
import subprocess


TOKEN_KIND_DEF_RE = re.compile(r"pick\s+TokenKind\s*\{(?P<body>.*?)\}", re.S)
TOKEN_KIND_REF_RE = re.compile(r"TokenKind\.([A-Za-z_][A-Za-z0-9_]*)")
STALE_IMPORT_RE = re.compile(r"\bfrontend/lex/")


def parse_token_kinds(token_file: Path) -> set[str]:
    text = token_file.read_text(encoding="utf-8")
    match = TOKEN_KIND_DEF_RE.search(text)
    if not match:
        raise SystemExit("[frontend-token-consistency][error] could not find TokenKind definition")
    body = match.group("body")
    kinds: set[str] = set()
    for raw_line in body.splitlines():
        line = raw_line.strip().rstrip(",")
        if not line:
            continue
        if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", line):
            continue
        kinds.add(line)
    return kinds


def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    frontend_root = repo / "src/vitte/compiler/frontend"
    token_file = frontend_root / "lexer/token.vit"
    regression_fixture = repo / "src/vitte/compiler/tests/frontend_token_contract_tests.vit"
    vitte_bin = repo / "bin/vitte"
    valid_kinds = parse_token_kinds(token_file)

    failures: list[str] = []
    checked_files = 0

    for path in sorted(frontend_root.rglob("*.vit")):
        text = path.read_text(encoding="utf-8")
        checked_files += 1
        rel = path.relative_to(repo)

        if STALE_IMPORT_RE.search(text):
            failures.append(f"{rel}: stale import path contains `frontend/lex/`")

        for ref in sorted(set(TOKEN_KIND_REF_RE.findall(text))):
            if ref not in valid_kinds:
                failures.append(f"{rel}: unknown token kind `TokenKind.{ref}`")

    if not regression_fixture.is_file():
        failures.append(f"{regression_fixture.relative_to(repo)}: missing regression fixture")
    elif not vitte_bin.is_file():
        failures.append(f"{vitte_bin.relative_to(repo)}: missing compiler binary required for token contract check")
    else:
        check = subprocess.run(
            [str(vitte_bin), "check", str(regression_fixture)],
            cwd=repo,
            capture_output=True,
            text=True,
        )
        if check.returncode != 0:
            failures.append(
                f"{regression_fixture.relative_to(repo)}: `bin/vitte check` failed for canonical token API regression fixture"
            )
            stderr = check.stderr.strip()
            stdout = check.stdout.strip()
            if stdout:
                failures.append(f"compiler stdout: {stdout}")
            if stderr:
                failures.append(f"compiler stderr: {stderr}")

    if failures:
        print(f"[frontend-token-consistency][error] checked_files={checked_files} failures={len(failures)}")
        for failure in failures:
            print(f" - {failure}")
        return 1

    print(
        "[frontend-token-consistency] "
        f"checked_files={checked_files} token_kinds={len(valid_kinds)} regression_fixture={regression_fixture.relative_to(repo)} status=ok"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
