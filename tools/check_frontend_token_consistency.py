#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path
import subprocess
import tempfile
import os
import json


TOKEN_KIND_DEF_RE = re.compile(r"pick\s+TokenKind\s*\{(?P<body>.*?)\}", re.S)
TOKEN_KIND_REF_RE = re.compile(r"TokenKind\.([A-Za-z_][A-Za-z0-9_]*)")
STALE_IMPORT_RE = re.compile(r"\bfrontend/lex/")


REPORT_DIR = Path("target/reports")


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


def compiler_path(repo: Path) -> Path:
    env_path = os.environ.get("VITTE_TOKEN_CONTRACT_COMPILER", "")
    candidates = []
    if env_path:
        candidates.append(Path(env_path))
    candidates.extend(
        [
            repo / "target/bootstrap-c17/vitte-bootstrap",
            repo / "bin/vitte",
        ]
    )

    for candidate in candidates:
        resolved = candidate if candidate.is_absolute() else repo / candidate
        if resolved.is_file() and os.access(resolved, os.X_OK):
            return resolved

    raise SystemExit(
        "[frontend-token-consistency][error] missing executable compiler; run `make bootstrap-c17`"
    )


def display_path(repo: Path, path: Path) -> str:
    if path.is_relative_to(repo):
        return str(path.relative_to(repo))
    return str(path)


def write_reports(repo: Path, manifest: dict[str, object]) -> None:
    report_dir = repo / REPORT_DIR
    report_dir.mkdir(parents=True, exist_ok=True)
    (report_dir / "frontend_token_consistency.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    md = [
        "# Frontend Token Consistency",
        "",
        f"- checked files: {manifest['checked_files']}",
        f"- token kinds: {manifest['token_kinds']}",
        f"- compiler: `{manifest['compiler']}`",
        f"- regression fixture: `{manifest['regression_fixture']}`",
        f"- status: {manifest['status']}",
        "",
    ]
    (report_dir / "frontend_token_consistency.md").write_text("\n".join(md), encoding="utf-8")


def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    frontend_root = repo / "src/vitte/compiler/frontend"
    token_file = frontend_root / "lexer/token.vit"
    regression_fixture = repo / "src/vitte/compiler/tests/frontend_token_contract_tests.vit"
    vitte_bin = compiler_path(repo)
    valid_kinds = parse_token_kinds(token_file)

    failures: list[str] = []
    checked_files = 0
    compiler_display = display_path(repo, vitte_bin)

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
    else:
        env = os.environ.copy()
        for name in ("VITTE_ROOT", "VITTE_SYSROOT", "VITTE_HOME"):
            env.pop(name, None)
        with tempfile.TemporaryDirectory(prefix="vitte-token-consistency-") as tmpdir:
            check = subprocess.run(
                [str(vitte_bin), "check", str(regression_fixture)],
                cwd=tmpdir,
                capture_output=True,
                text=True,
                env=env,
            )
        if check.returncode != 0:
            failures.append(
                f"{regression_fixture.relative_to(repo)}: compiler check failed for canonical token API regression fixture"
            )
            stderr = check.stderr.strip()
            stdout = check.stdout.strip()
            if stdout:
                failures.append(f"compiler stdout: {stdout}")
            if stderr:
                failures.append(f"compiler stderr: {stderr}")

    if failures:
        write_reports(
            repo,
            {
                "checked_files": checked_files,
                "compiler": compiler_display,
                "failures": failures,
                "regression_fixture": str(regression_fixture.relative_to(repo)),
                "status": "error",
                "token_kinds": len(valid_kinds),
            },
        )
        print(f"[frontend-token-consistency][error] checked_files={checked_files} failures={len(failures)}")
        for failure in failures:
            print(f" - {failure}")
        return 1

    write_reports(
        repo,
        {
            "checked_files": checked_files,
            "compiler": compiler_display,
            "failures": [],
            "regression_fixture": str(regression_fixture.relative_to(repo)),
            "status": "ok",
            "token_kinds": len(valid_kinds),
        },
    )
    print(
        "[frontend-token-consistency] "
        f"checked_files={checked_files} token_kinds={len(valid_kinds)} compiler={compiler_display} regression_fixture={regression_fixture.relative_to(repo)} status=ok"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
