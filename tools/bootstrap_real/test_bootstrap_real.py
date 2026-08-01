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


def test_forbidden_markers_are_checked_individually() -> None:
    for marker in bootstrap_real.FORBIDDEN_BINARY_MARKERS:
        errors = bootstrap_real.validate_forbidden_markers(marker, "candidate")
        assert_true(
            any(marker in error for error in errors),
            f"forbidden marker should be reported: {marker}",
        )
    clean = "\n".join(bootstrap_real.REQUIRED_ENTRY_MARKERS)
    assert_true(
        bootstrap_real.validate_forbidden_markers(clean, "candidate") == [],
        "clean marker text should not report forbidden markers",
    )


def test_required_markers_are_checked_individually() -> None:
    all_markers = "\n".join(bootstrap_real.REQUIRED_ENTRY_MARKERS)
    assert_true(
        bootstrap_real.validate_required_markers(all_markers, "candidate") == [],
        "all required markers should pass",
    )
    missing_driver = bootstrap_real.validate_required_markers(
        "COMPILER_ENTRY_POINT=src/vitte/compiler/main.vit",
        "candidate",
    )
    assert_true(
        any("run_cli_main_with_ice_boundary" in error for error in missing_driver),
        "missing driver entry marker should be reported",
    )
    missing_entrypoint = bootstrap_real.validate_required_markers(
        "run_cli_main_with_ice_boundary",
        "candidate",
    )
    assert_true(
        any("COMPILER_ENTRY_POINT=src/vitte/compiler/main.vit" in error for error in missing_entrypoint),
        "missing compiler entrypoint marker should be reported",
    )


def test_stage0_smoke_commands_are_exact() -> None:
    stage0 = ROOT / "target/bootstrap-real/stage0/vitte"
    commands = bootstrap_real.vitte_smoke_commands(stage0)
    assert_true(
        commands == [
            [str(stage0), "--version"],
            [str(stage0), "--help"],
            [str(stage0), "check", "src/vitte/compiler/main.vit"],
        ],
        "stage0 smoke commands should be version, help, then compiler check",
    )


def test_bootstrap_build_command_is_exact() -> None:
    stage0 = ROOT / "target/bootstrap-real/stage0/vitte"
    out = ROOT / "target/bootstrap-real/vitte"
    command = bootstrap_real.bootstrap_build_command(stage0, out)
    assert_true(
        command == [
            str(stage0),
            "build",
            "src/vitte/compiler/main.vit",
            "-o",
            str(out),
        ],
        "bootstrap build should compile main.vit into target/bootstrap-real/vitte",
    )


def test_stage1_build_command_is_exact() -> None:
    bootstrap_compiler = ROOT / "target/bootstrap-real/vitte"
    out = ROOT / "target/stage1/vitte"
    command = bootstrap_real.stage1_build_command(bootstrap_compiler, out)
    assert_true(
        command == [
            str(bootstrap_compiler),
            "build",
            "src/vitte/compiler/main.vit",
            "-o",
            str(out),
        ],
        "stage1 build should compile main.vit into target/stage1/vitte",
    )


def test_stage1_output_path_is_canonical() -> None:
    errors = bootstrap_real.validate_stage_output_path(
        ROOT / "target/not-stage1/vitte",
        "stage1",
        bootstrap_real.STAGE1_OUT,
    )
    assert_true(any("target/stage1/vitte" in error for error in errors), "stage1 output must be canonical")
    assert_true(
        bootstrap_real.validate_stage_output_path(bootstrap_real.STAGE1_OUT, "stage1", bootstrap_real.STAGE1_OUT) == [],
        "canonical stage1 output should pass",
    )


def test_stage1_build_quarantines_stale_output_before_dependency_check() -> None:
    work = ROOT / "target/bootstrap-real/test-work"
    stage1 = work / "stage1/vitte"
    sidecar = Path(str(stage1) + ".bootstrap-bridge")
    stage1.parent.mkdir(parents=True, exist_ok=True)
    stage1.write_bytes(b"stale-stage1")
    sidecar.write_bytes(b"stale-stage1-bridge")
    original_quarantine = bootstrap_real.QUARANTINE_DIR
    try:
        bootstrap_real.QUARANTINE_DIR = work / "quarantine"
        moved = bootstrap_real.quarantine_bootstrap_output(stage1)
    finally:
        bootstrap_real.QUARANTINE_DIR = original_quarantine
    assert_true(not stage1.exists(), "stage1 build should remove stale stage1 before dependency checks")
    assert_true(not sidecar.exists(), "stage1 build should remove stale stage1 bridge sidecar")
    assert_true(len(moved) == 2, "stage1 stale output and sidecar should be quarantined")


def test_stage0_build_outputs_are_canonical() -> None:
    out = bootstrap_real.DEFAULT_OUT
    assert_true(
        out == ROOT / "target/bootstrap-real/vitte",
        "bootstrap compiler output should be target/bootstrap-real/vitte",
    )
    assert_true(
        bootstrap_real.REPORT_JSON == ROOT / "target/reports/bootstrap_real_gate.json",
        "bootstrap json report should use the canonical report path",
    )
    assert_true(
        bootstrap_real.REPORT_MD == ROOT / "target/reports/bootstrap_real_gate.md",
        "bootstrap markdown report should use the canonical report path",
    )


def test_stage0_build_clears_stale_output_and_bridge_sidecar() -> None:
    work = ROOT / "target/bootstrap-real/test-work"
    stale = work / "stale-vitte"
    sidecar = Path(str(stale) + ".bootstrap-bridge")
    stale.parent.mkdir(parents=True, exist_ok=True)
    stale.write_bytes(b"stale")
    sidecar.write_bytes(b"bridge")
    original_quarantine = bootstrap_real.QUARANTINE_DIR
    try:
        bootstrap_real.QUARANTINE_DIR = work / "quarantine"
        moved = bootstrap_real.quarantine_bootstrap_output(stale)
    finally:
        bootstrap_real.QUARANTINE_DIR = original_quarantine
    assert_true(not stale.exists(), "stage0 build should remove stale output before compiling")
    assert_true(not sidecar.exists(), "stage0 build should remove stale bridge sidecar before compiling")
    assert_true(len(moved) == 2, "stale output and bridge sidecar should both be quarantined")
    assert_true(all(Path(str(item["to"])).is_file() for item in moved), "quarantined artifacts should be preserved")


def test_script_self_copy_bridge_and_private_tmp_are_rejected() -> None:
    work = ROOT / "target/bootstrap-real/test-work"
    work.mkdir(parents=True, exist_ok=True)
    script = work / "script-vitte"
    script.write_text("#!/bin/sh\nexit 0\n", encoding="utf-8")
    script.chmod(script.stat().st_mode | stat.S_IXUSR)
    script_errors, _commands = bootstrap_real.validate_vitte_binary(script, "stage0 source")
    assert_true(
        any("must be a native binary, got script" in error for error in script_errors),
        "script stage0 sources should be rejected",
    )

    self_copy = work / "self-copy-vitte"
    self_copy.write_bytes(
        b"\xca\xfe\xba\xbe"
        b"run_cli_main_with_ice_boundary\0"
        b"COMPILER_ENTRY_POINT=src/vitte/compiler/main.vit\0"
        b"_command_build\0_copy_file\0"
    )
    self_copy.chmod(self_copy.stat().st_mode | stat.S_IXUSR)
    self_copy_errors, _commands = bootstrap_real.validate_vitte_binary(self_copy, "stage0 source")
    assert_true(
        any("_command_build" in error for error in self_copy_errors)
        and any("_copy_file" in error for error in self_copy_errors),
        "self-copy stage0 sources should be rejected",
    )

    original_is_under = bootstrap_real.is_under
    try:
        bootstrap_real.is_under = lambda path, parent: str(parent) == "/private/tmp"
        tmp_errors, _commands = bootstrap_real.validate_stage0_install_source(work / "tmp-vitte")
        assert_true(
            any("/private/tmp" in error for error in tmp_errors),
            "/private/tmp stage0 sources should be rejected",
        )
    finally:
        bootstrap_real.is_under = original_is_under


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


def test_canonical_stage0_path_is_the_only_path_gate() -> None:
    work = ROOT / "target/bootstrap-real/test-work"
    canonical = work / "canonical-stage0/vitte"
    canonical.parent.mkdir(parents=True, exist_ok=True)
    canonical.write_bytes(
        b"\xca\xfe\xba\xbe"
        b"run_cli_main_with_ice_boundary\0"
        b"COMPILER_ENTRY_POINT=src/vitte/compiler/main.vit\0"
    )
    canonical.chmod(canonical.stat().st_mode | stat.S_IXUSR)
    original_trusted = bootstrap_real.TRUSTED_STAGE0
    try:
        bootstrap_real.TRUSTED_STAGE0 = canonical
        errors, _commands = bootstrap_real.validate_stage0(canonical)
        assert_true(
            not any("single trusted Vitte compiler" in error for error in errors),
            "canonical target/bootstrap-real/stage0/vitte should pass the path gate",
        )
    finally:
        bootstrap_real.TRUSTED_STAGE0 = original_trusted


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
        bootstrap_real.write_reports("fail", candidate, None, None, ["example failure"], [], [], [], [])
        assert_true(bootstrap_real.REPORT_JSON.is_file(), "json report should be written")
        assert_true(bootstrap_real.REPORT_MD.is_file(), "markdown report should be written")
        assert_true("example failure" in bootstrap_real.REPORT_MD.read_text(encoding="utf-8"), "markdown should list errors")
    finally:
        bootstrap_real.REPORT_JSON = original_json
        bootstrap_real.REPORT_MD = original_md


def test_install_stage0_copies_only_after_validation_path() -> None:
    work = ROOT / "target/bootstrap-real/test-work"
    source = work / "install-source"
    trusted = work / "installed/vitte"
    source.parent.mkdir(parents=True, exist_ok=True)
    source.write_bytes(b"\xca\xfe\xba\xbevalidated-stage0")
    source.chmod(source.stat().st_mode | stat.S_IXUSR)
    original_trusted = bootstrap_real.TRUSTED_STAGE0
    try:
        bootstrap_real.TRUSTED_STAGE0 = trusted
        bootstrap_real.install_stage0(source)
        assert_true(trusted.is_file(), "stage0 install should copy to the trusted path")
        assert_true(trusted.read_bytes() == source.read_bytes(), "stage0 install should preserve the source binary")
        assert_true(trusted.stat().st_mode & stat.S_IXUSR != 0, "installed stage0 should be executable")
    finally:
        bootstrap_real.TRUSTED_STAGE0 = original_trusted


def test_invalid_install_does_not_replace_existing_stage0() -> None:
    work = ROOT / "target/bootstrap-real/test-work"
    trusted = work / "existing/vitte"
    invalid = work / "invalid-source"
    trusted.parent.mkdir(parents=True, exist_ok=True)
    trusted.write_bytes(b"existing-stage0")
    trusted.chmod(trusted.stat().st_mode | stat.S_IXUSR)
    invalid.write_bytes(b"#!/bin/sh\nexit 0\n")
    invalid.chmod(invalid.stat().st_mode | stat.S_IXUSR)
    original_trusted = bootstrap_real.TRUSTED_STAGE0
    try:
        bootstrap_real.TRUSTED_STAGE0 = trusted
        errors, _commands = bootstrap_real.validate_stage0_install_source(invalid)
        if not errors:
            bootstrap_real.install_stage0(invalid)
        assert_true(errors, "invalid stage0 source should fail validation")
        assert_true(trusted.read_bytes() == b"existing-stage0", "invalid install should not replace existing stage0")
    finally:
        bootstrap_real.TRUSTED_STAGE0 = original_trusted


def test_install_failure_removes_partial_temp_file() -> None:
    work = ROOT / "target/bootstrap-real/test-work"
    trusted = work / "failing/vitte"
    source = work / "failing-source"
    trusted.parent.mkdir(parents=True, exist_ok=True)
    source.write_bytes(b"candidate")
    original_trusted = bootstrap_real.TRUSTED_STAGE0
    original_copy2 = bootstrap_real.shutil.copy2
    try:
        bootstrap_real.TRUSTED_STAGE0 = trusted

        def fail_after_write(_source: Path, destination: Path) -> None:
            destination.write_bytes(b"partial")
            raise OSError("simulated copy failure")

        bootstrap_real.shutil.copy2 = fail_after_write
        try:
            bootstrap_real.install_stage0(source)
        except OSError:
            pass
        else:
            raise AssertionError("install_stage0 should propagate copy failures")
        assert_true(not trusted.exists(), "failed install should not create trusted stage0")
        assert_true(not trusted.with_name("vitte.installing").exists(), "failed install should remove temporary file")
    finally:
        bootstrap_real.shutil.copy2 = original_copy2
        bootstrap_real.TRUSTED_STAGE0 = original_trusted


def test_install_report_records_source_and_installed_artifact() -> None:
    work = ROOT / "target/bootstrap-real/test-work"
    reports = work / "install-reports"
    source = work / "install-report-source"
    trusted = work / "install-report/vitte"
    source.parent.mkdir(parents=True, exist_ok=True)
    source.write_bytes(b"\xca\xfe\xba\xbevalidated-stage0")
    source.chmod(source.stat().st_mode | stat.S_IXUSR)
    original_json = bootstrap_real.REPORT_JSON
    original_md = bootstrap_real.REPORT_MD
    original_trusted = bootstrap_real.TRUSTED_STAGE0
    try:
        bootstrap_real.REPORT_JSON = reports / "bootstrap_real_gate.json"
        bootstrap_real.REPORT_MD = reports / "bootstrap_real_gate.md"
        bootstrap_real.TRUSTED_STAGE0 = trusted
        bootstrap_real.install_stage0(source)
        bootstrap_real.write_reports("ok", trusted, None, source, [], [], [], [], [])
        text = bootstrap_real.REPORT_JSON.read_text(encoding="utf-8")
        assert_true('"install_source"' in text, "install report should record source artifact")
        assert_true('"artifact"' in text, "install report should record installed artifact")
    finally:
        bootstrap_real.REPORT_JSON = original_json
        bootstrap_real.REPORT_MD = original_md
        bootstrap_real.TRUSTED_STAGE0 = original_trusted


def main() -> int:
    work = ROOT / "target/bootstrap-real/test-work"
    if work.exists():
        shutil.rmtree(work)
    test_output_path_must_stay_under_artifact_root()
    test_forbidden_bridge_marker_is_rejected()
    test_forbidden_markers_are_checked_individually()
    test_required_markers_are_checked_individually()
    test_stage0_smoke_commands_are_exact()
    test_bootstrap_build_command_is_exact()
    test_stage1_build_command_is_exact()
    test_stage1_output_path_is_canonical()
    test_stage1_build_quarantines_stale_output_before_dependency_check()
    test_stage0_build_outputs_are_canonical()
    test_stage0_build_clears_stale_output_and_bridge_sidecar()
    test_script_self_copy_bridge_and_private_tmp_are_rejected()
    test_stage0_must_be_single_trusted_path()
    test_canonical_stage0_path_is_the_only_path_gate()
    test_report_writer_records_failure()
    test_install_stage0_copies_only_after_validation_path()
    test_invalid_install_does_not_replace_existing_stage0()
    test_install_failure_removes_partial_temp_file()
    test_install_report_records_source_and_installed_artifact()
    if work.exists():
        shutil.rmtree(work)
    print("[bootstrap-real-test] ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
