//! vitte-cli — binaire `vitte`
use std::env; use std::fs;
use vitte_compiler::compile_to_vitbc; use vitte_vm::Vm;

fn usage() { eprintln!("vitte <run|repl> [file]"); }
fn repl() { println!("Vitte REPL (squelette). Ctrl+C pour quitter."); }
fn run_file(path: &str) -> anyhow::Result<i32> {
    let src = fs::read_to_string(path)?;
    let bc = compile_to_vitbc(&src);
    let mut vm = Vm::new();
    Ok(vm.run_bytecode(&bc))
}
fn main() -> anyhow::Result<()> {
    let mut args = env::args().skip(1);
    match args.next().as_deref() {
        Some("repl") => { repl(); Ok(()) }
        Some("run") => { if let Some(file) = args.next() { let code = run_file(&file)?; std::process::exit(code); } else { usage(); Ok(()) } }
        _ => { usage(); Ok(()) }
    }
}
