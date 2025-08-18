//! vitte-debug.rs — Binaire de débogage de la VM Vitte
//!
//! Permet d’exécuter un fichier bytecode avec traces, breakpoints,
//! et inspection interactive (REPL).
//!
//! Usage :
//!   cargo run --bin vitte-debug -- path/to/file.vitbc [options]
//!
//! Options :
//!   --trace       : affiche chaque instruction exécutée
//!   --dump        : affiche le bytecode désassemblé
//!   --step        : mode pas-à-pas interactif
//!   --break <n>   : ajoute un breakpoint à l’instruction n
//!   --stack       : dump la pile à chaque étape

use std::path::PathBuf;
use std::fs;
use clap::{Parser, ArgAction};
use color_eyre::eyre::{Result, eyre};

use vitte_core::bytecode::{Chunk, disasm};
use vitte_vm::{VM, Debugger};

/// CLI de débogage de la VM Vitte
#[derive(Parser, Debug)]
#[command(author, version, about = "Vitte Debugger", long_about = None)]
struct Args {
    /// Fichier bytecode (.vitbc)
    file: PathBuf,

    /// Trace complète de l’exécution
    #[arg(long, action=ArgAction::SetTrue)]
    trace: bool,

    /// Dump le bytecode désassemblé
    #[arg(long, action=ArgAction::SetTrue)]
    dump: bool,

    /// Mode pas-à-pas (interactive)
    #[arg(long, action=ArgAction::SetTrue)]
    step: bool,

    /// Breakpoints (peut être répété)
    #[arg(long, num_args=1.., value_delimiter=',')]
    breakpoints: Vec<usize>,

    /// Dump la pile à chaque étape
    #[arg(long, action=ArgAction::SetTrue)]
    stack: bool,
}

fn main() -> Result<()> {
    color_eyre::install()?;
    let args = Args::parse();

    let data = fs::read(&args.file)
        .map_err(|e| eyre!("Impossible de lire {:?}: {}", args.file, e))?;

    let chunk: Chunk = bincode::deserialize(&data)
        .map_err(|e| eyre!("Bytecode invalide: {}", e))?;

    if args.dump {
        println!("=== Désassemblage de {:?} ===", args.file);
        println!("{}", disasm(&chunk));
        return Ok(());
    }

    let mut vm = VM::new();
    let mut dbg = Debugger::new(&mut vm);

    // configure le débogueur
    dbg.set_trace(args.trace);
    dbg.set_stack_dump(args.stack);
    for bp in args.breakpoints {
        dbg.add_breakpoint(bp);
    }

    if args.step {
        println!("Mode pas-à-pas activé (REPL : help, next, cont, regs, stack)");
        dbg.run_interactive(&chunk)?;
    } else {
        dbg.run(&chunk)?;
    }

    Ok(())
}

