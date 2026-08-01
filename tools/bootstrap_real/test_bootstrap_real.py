#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import shutil
import stat
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MODULE_PATH = ROOT / "tools/bootstrap_real/bootstrap_real.py"

spec = importlib.util.spec_from_file_location("bootstrap_real", MODULE_PATH)
assert spec is not None and spec.loader is not None
bootstrap_real = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bootstrap_real)


def assert_true(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def test_output_path_must_stay_under_artifact_root() -> None:
    outside = ROOT / "target/not-bootstrap-real/vitte"
    errors = bootstrap_real.validate_output_path(outside)
    assert_true(errors, "outside output path should be rejected")
    assert_true("target/bootstrap-real" in errors[0], "error should name the artifact root")


def test_forbidden_bridge_marker_is_rejected() -> None:
    work = ROOT / "target/bootstrap-real/test-work"
    work.mkdir(parents=True, exist_ok=True)
    candidate = work / "fake-vitte"
    candidate.write_bytes(
        b"\xca\xfe\xba\xbe"
        b"run_cli_main_with_ice_boundary\0"
        b"COMPILER_ENTRY_POINT=src/vitte/compiler/main.vit\0"
        b"VITTE_BOOTSTRAP_ALLOW_FULL_COMPILER_BRIDGE\0"
    )
    candidate.chmod(candidate.stat().st_mode | stat.S_IXUSR)
    errors, _commands = bootstrap_real.validate_candidate(candidate)
    assert_true(
        any("VITTE_BOOTSTRAP_ALLOW_FULL_COMPILER_BRIDGE" in error for error in errors),
        "bridge marker should reject the candidate",
    )


def test_stage0_must_be_single_trusted_path() -> None:
    work = ROOT / "target/bootstrap-real/test-work"
    work.mkdir(parents=True, exist_ok=True)
    stage0 = work / "stage0"
    stage0.write_bytes(
        b"\xca\xfe\xba\xbe"
        b"run_cli_main_with_ice_boundary\0"
        b"COMPILER_ENTRY_POINT=src/vitte/compiler/main.vit\0"
    )
    stage0.chmod(stage0.stat().st_mode | stat.S_IXUSR)
    errors, _commands = bootstrap_real.validate_stage0(stage0)
    assert_true(
        any("single trusted Vitte compiler" in error for error in errors),
        "stage0 outside target/bootstrap-real/stage0/vitte should be rejected",
    )


def test_report_writer_records_failure() -> None:
    work = ROOT / "target/bootstrap-real/test-work"
    reports = work / "reports"
    reports.mkdir(parents=True, exist_ok=True)
    original_json = bootstrap_real.REPORT_JSON
    original_md = bootstrap_real.REPORT_MD
    try:
        bootstrap_real.REPORT_JSON = reports / "bootstrap_real_gate.json"
        bootstrap_real.REPORT_MD = reports / "bootstrap_real_gate.md"
        candidate = work / "missing-vitte"
        bootstrap_real.write_reports("fail", candidate, None, ["example failure"], [], [], [])
        assert_true(bootstrap_real.REPORT_JSON.is_file(), "json report should be written")
        assert_true(bootstrap_real.REPORT_MD.is_file(), "markdown report should be written")
        assert_true("example failure" in bootstrap_real.REPORT_MD.read_text(encoding="utf-8"), "markdown should list errors")
    finally:
        bootstrap_real.REPORT_JSON = original_json
        bootstrap_real.REPORT_MD = original_md


def main() -> int:
    work = ROOT / "target/bootstrap-real/test-work"
    if work.exists():
        shutil.rmtree(work)
    test_output_path_must_stay_under_artifact_root()
    test_forbidden_bridge_marker_is_rejected()
    test_stage0_must_be_single_trusted_path()
    test_report_writer_records_failure()
    if work.exists():
        shutil.rmtree(work)
    print("[bootstrap-real-test] ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
