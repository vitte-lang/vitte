use std::fs;

use assert_cmd::Command;
use insta::assert_snapshot;
use predicates::prelude::*;
use tempfile::tempdir;

fn base_cmd() -> Command {
    let mut cmd = Command::cargo_bin("vitte").expect("binary `vitte`");
    cmd.env("NO_COLOR", "1");
    cmd.env("CLICOLOR_FORCE", "0");
    cmd
}

fn capture_output(mut cmd: Command) -> Result<(i32, String, String), std::io::Error> {
    let out = cmd.output()?;
    let status = out.status.code().unwrap_or_else(|| if out.status.success() { 0 } else { -1 });
    let stdout = String::from_utf8_lossy(&out.stdout).to_string();
    let stderr = String::from_utf8_lossy(&out.stderr).to_string();
    Ok((status, stdout, stderr))
}

fn snapshot(label: &str, status: i32, stdout: &str, stderr: &str) {
    assert_snapshot!(
        label,
        format!(
            "status: {status}\nstdout:\n{stdout}\
            \n---- stderr ----\n{stderr}"
        )
    );
}

#[test]
fn compile_source_after_options_is_rejected() {
    let mut cmd = base_cmd();
    cmd.args(["compile", "--optimize", "foo.vitte"]);
    let (status, stdout, stderr) = capture_output(cmd).expect("run command");
    snapshot("compile_source_after_options", status, &stdout, &stderr);
}

#[test]
fn run_program_after_options_requires_reordering() {
    let mut cmd = base_cmd();
    cmd.args(["run", "--auto-compile", "main.vitte"]);
    let (status, stdout, stderr) = capture_output(cmd).expect("run command");
    snapshot("run_program_after_options", status, &stdout, &stderr);
}

#[test]
fn global_option_after_command_is_rejected() {
    let mut cmd = base_cmd();
    cmd.args(["compile", "-v"]);
    let (status, stdout, stderr) = capture_output(cmd).expect("run command");
    snapshot("compile_global_option_after_command", status, &stdout, &stderr);
}

fn help_output(args: &[&str]) -> (i32, String, String) {
    let mut cmd = base_cmd();
    cmd.args(args);
    capture_output(cmd).expect("run command")
}

#[test]
fn help_main_command_matches_snapshot() {
    let (status, stdout, stderr) = help_output(&["--help"]);
    snapshot("help_main", status, &stdout, &stderr);
}

#[test]
fn help_compile_command_matches_snapshot() {
    let (status, stdout, stderr) = help_output(&["compile", "--help"]);
    snapshot("help_compile", status, &stdout, &stderr);
}

#[test]
fn help_run_command_matches_snapshot() {
    let (status, stdout, stderr) = help_output(&["run", "--help"]);
    snapshot("help_run", status, &stdout, &stderr);
}

#[test]
fn help_fmt_command_matches_snapshot() {
    let (status, stdout, stderr) = help_output(&["fmt", "--help"]);
    snapshot("help_fmt", status, &stdout, &stderr);
}

#[test]
fn help_inspect_command_matches_snapshot() {
    let (status, stdout, stderr) = help_output(&["inspect", "--help"]);
    snapshot("help_inspect", status, &stdout, &stderr);
}

#[test]
fn help_disasm_command_matches_snapshot() {
    let (status, stdout, stderr) = help_output(&["disasm", "--help"]);
    snapshot("help_disasm", status, &stdout, &stderr);
}

#[test]
fn help_modules_command_matches_snapshot() {
    let (status, stdout, stderr) = help_output(&["modules", "--help"]);
    snapshot("help_modules", status, &stdout, &stderr);
}

#[test]
fn version_flag_prints_version() {
    let mut cmd = base_cmd();
    cmd.args(["--version"]);
    let (status, stdout, stderr) = capture_output(cmd).expect("run command");
    assert_eq!(status, 0);
    assert!(stdout.starts_with("vitte "));
    assert!(stderr.is_empty());
}

#[cfg(feature = "vm")]
#[test]
fn compile_command_writes_bytecode() {
    let dir = tempdir().expect("tempdir");
    let src_path = dir.path().join("main.vitte");
    fs::write(&src_path, "module main;").expect("write source");
    let out_path = dir.path().join("main.vitbc");

    let mut cmd = base_cmd();
    cmd.current_dir(dir.path());
    cmd.args([
        "compile",
        src_path.file_name().unwrap().to_str().unwrap(),
        "--output",
        out_path.file_name().unwrap().to_str().unwrap(),
    ]);

    cmd.assert()
        .success()
        .stdout(predicate::str::is_empty())
        .stderr(predicate::str::contains("COMPILE"));

    assert!(out_path.exists(), "bytecode file should be produced");
}

#[cfg(feature = "fmt")]
#[test]
fn fmt_in_place_success() {
    let dir = tempdir().expect("tempdir");
    let src_path = dir.path().join("format_me.vitte");
    fs::write(&src_path, "module sample;").expect("write source");

    let mut cmd = base_cmd();
    cmd.current_dir(dir.path());
    cmd.args(["fmt", src_path.file_name().unwrap().to_str().unwrap(), "--in-place"]);

    cmd.assert()
        .success()
        .stdout(predicate::str::is_empty())
        .stderr(predicate::str::contains("FMT"));
    assert!(fs::read_to_string(&src_path).unwrap().contains("module sample;"));
}

#[cfg(feature = "fmt")]
#[test]
fn fmt_in_place_and_output_conflict() {
    let mut cmd = base_cmd();
    cmd.args(["fmt", "file.vitte", "--in-place", "--output", "out.vitte"]);
    let (status, stdout, stderr) = capture_output(cmd).expect("run command");
    snapshot("fmt_in_place_output_conflict", status, &stdout, &stderr);
}

#[cfg(feature = "fmt")]
#[test]
fn fmt_in_place_with_stdin_conflict() {
    let mut cmd = base_cmd();
    cmd.args(["fmt", "--in-place"]);
    let (status, stdout, stderr) = capture_output(cmd).expect("run command");
    snapshot("fmt_in_place_stdin_conflict", status, &stdout, &stderr);
}

#[cfg(feature = "fmt")]
#[test]
fn fmt_in_place_with_check_conflict() {
    let mut cmd = base_cmd();
    cmd.args(["fmt", "file.vitte", "--in-place", "--check"]);
    let (status, stdout, stderr) = capture_output(cmd).expect("run command");
    snapshot("fmt_in_place_check_conflict", status, &stdout, &stderr);
}

#[test]
fn compile_invalid_source_reports_parse_error() {
    let dir = tempdir().expect("tempdir");
    let src_path = dir.path().join("bad.vitte");
    fs::write(&src_path, "module main {").expect("write source");

    let mut cmd = base_cmd();
    cmd.current_dir(dir.path());
    cmd.args(["compile", src_path.file_name().unwrap().to_str().unwrap(), "--output", "out.vitbc"]);

    let (status, stdout, stderr) = capture_output(cmd).expect("run command");
    snapshot("compile_invalid_source_parse_error", status, &stdout, &stderr);
}
