//! Benchmarks the CLI pipeline (source on disk → compile → run via VM hook).

use std::{
    io::Write,
    path::{Path, PathBuf},
};

use anyhow::{anyhow, Result};
use criterion::{black_box, BenchmarkId, Criterion, Throughput};
use tempfile::NamedTempFile;
use vitte_cli::{
    context::{Profile, ProfileConfig},
    Command, CompileOptions, Hooks, InputKind, RunOptions, RunTask,
};
use vitte_compiler::{Compiler, CompilerOptions, Severity};
use vitte_core::SourceId;
use vitte_lexer::{Lexer, LexerOptions};
use vitte_parser::Parser;
use vitte_vm::Vm;

fn compile_hook(source: &str, _opts: &CompileOptions) -> Result<Vec<u8>> {
    let tokens = Lexer::with_options(source, SourceId(0), LexerOptions::default())
        .tokenize()
        .map_err(|err| anyhow!("lexing failed: {err}"))?;
    // Reconstruct parser input from source (tokens used to fail fast on lex errors).
    drop(tokens);

    let mut parser = Parser::new(source, SourceId(0));
    let program = parser.parse_program().map_err(|err| anyhow!("parse failed: {err}"))?;

    let mut compiler = Compiler::new(CompilerOptions::default());
    let bytecode = compiler.compile(&program).map_err(|err| anyhow!(format_compile_error(err)))?;
    Ok(bytecode.to_bytes(false))
}

fn run_hook(bytecode: &[u8], _opts: &RunOptions) -> Result<i32> {
    let mut vm = Vm::new();
    vm.run_bytecode(bytecode).map_err(|err| anyhow!("vm execution failed: {err}"))
}

fn format_compile_error(err: vitte_compiler::CompileError) -> String {
    let mut out = String::from("compilation failed");
    for diag in err.diagnostics {
        let sev = match diag.severity {
            Severity::Info => "info",
            Severity::Warning => "warning",
            Severity::Error => "error",
        };
        out.push_str(&format!("\n- {sev}: {}", diag.message));
    }
    out
}

struct DiskCase {
    label: &'static str,
    length: usize,
    path: PathBuf,
    _file: NamedTempFile,
}

fn create_disk_cases() -> Vec<DiskCase> {
    let base = r#"
fn helper(value: i64) -> i64 {
    let doubled = value + value;
    return doubled + 1;
}

fn main() -> i64 {
    let mut acc = 0;
    acc = acc + helper(10);
    acc = acc + helper(20);
    return acc;
}
"#;

    let sources = vec![
        ("cli-small", base.to_owned()),
        ("cli-medium", base.repeat(4)),
        ("cli-large", base.repeat(16)),
    ];

    sources
        .into_iter()
        .map(|(label, src)| {
            let mut file = NamedTempFile::new().expect("temp source file");
            file.write_all(src.as_bytes()).expect("write source");
            file.flush().expect("flush source");
            let path = file.path().to_path_buf();
            DiskCase { label, length: src.len(), path, _file: file }
        })
        .collect()
}

fn pipeline_hooks() -> Hooks {
    let mut hooks = Hooks::default();
    hooks.compile = Some(compile_hook);
    hooks.run_bc = Some(run_hook);
    hooks
}

fn run_cli_pipeline(path: &Path, hooks: &Hooks) -> Result<i32> {
    let cmd = Command::Run(RunTask {
        program: InputKind::SourcePath(path.to_path_buf()),
        args: Vec::new(),
        auto_compile: true,
        optimize: false,
        time: false,
        profile: ProfileConfig::defaults(Profile::Dev),
    });
    vitte_cli::execute(cmd, hooks)
}

/// Registers CLI end-to-end pipeline benchmarks with Criterion.
pub fn criterion_benchmark(c: &mut Criterion) {
    let hooks = pipeline_hooks();
    let disk_cases = create_disk_cases();

    let mut group = c.benchmark_group("pipeline/cli");
    for case in &disk_cases {
        group.throughput(Throughput::Bytes(case.length as u64));
        group.bench_with_input(BenchmarkId::from_parameter(case.label), &case.path, |b, p| {
            b.iter(|| {
                let exit_code = run_cli_pipeline(p, &hooks).expect("cli pipeline failure");
                black_box(exit_code);
            });
        });
    }

    // Ensure the temp files stay alive for the duration of the benchmarks.
    group.finish();
}
