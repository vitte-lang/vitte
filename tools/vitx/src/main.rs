//! vitx — mini CLI pour le langage Vitte (light)

use std::env;
use std::fs;
use std::io::{self, Write};

fn usage() {
    eprintln!("Usage:");
    eprintln!("  vitx run <file.vt>   # exécuter un fichier");
    eprintln!("  vitx repl            # lancer le REPL");
}

fn run_file(path: &str) -> anyhow::Result<i32> {
    let src = fs::read_to_string(path)?;
    println!("=== Running {path} ===");
    println!("{src}");
    // Ici tu brancheras vitte-compiler + vitte-vm pour exécuter
    Ok(0)
}

fn repl() -> anyhow::Result<()> {
    println!("Vitx REPL — tape `exit` pour quitter.");
    let mut line = String::new();
    loop {
        print!("vitx> ");
        io::stdout().flush()?;
        line.clear();
        if io::stdin().read_line(&mut line)? == 0 {
            break;
        }
        let input = line.trim();
        if input == "exit" || input == "quit" {
            break;
        }
        println!("(echo) {input}");
        // plus tard -> parser + évaluer
    }
    Ok(())
}

fn main() -> anyhow::Result<()> {
    let mut args = env::args().skip(1);
    match args.next().as_deref() {
        Some("run") => {
            if let Some(file) = args.next() {
                let code = run_file(&file)?;
                std::process::exit(code);
            } else {
                usage();
            }
        }
        Some("repl") => repl()?,
        _ => usage(),
    }
    Ok(())
}
