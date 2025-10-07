//! vitte-bin — bibliothèque et intégration CLI pour le langage Vitte
//! Fournit la logique des sous-commandes `run`, `compile`, `repl`, `version` + API appelée par le binaire.
//! Fournit la logique principale des sous-commandes `run`, `compile`, `repl`, `version`.
//! Peut être intégrée dans des binaires externes comme `vitte-bin`.

use anyhow::Result;
use clap::{Parser, Subcommand};
use log::info;
use std::path::{Path, PathBuf};
#[allow(unused_imports)]
use which::which;

/* -------------------------------------------------------------------------- */
/* Structures principales                                                     */
/* -------------------------------------------------------------------------- */

/// Interface CLI du langage Vitte
#[derive(Debug, Parser)]
#[command(
    name = "vitte",
    version,
    about = "Vitte CLI — moteur et outils universels"
)]
pub struct Cli {
    /// Niveau de verbosité (-v, -vv)
    #[arg(short, long, global = true, action = clap::ArgAction::Count)]
    pub verbose: u8,

    /// Silence toutes les sorties non essentielles
    #[arg(short, long, global = true)]
    pub quiet: bool,

    #[command(subcommand)]
    pub command: Command,
}

/// Sous-commandes disponibles
#[derive(Debug, Subcommand)]
pub enum Command {
    /// Exécute un fichier source ou bytecode
    Run {
        input: PathBuf,
        #[arg(trailing_var_arg = true)]
        args: Vec<String>,
    },

    /// Compile un fichier source
    Compile {
        input: PathBuf,
        #[arg(short, long)]
        out: Option<PathBuf>,
        #[arg(long, default_value = "bc")]
        emit: String,
    },

    /// Démarre le REPL interactif
    Repl,

    /// Affiche la version du binaire
    Version,
}

/* -------------------------------------------------------------------------- */
/* Moteur embarqué (engine::vm + engine::compiler)                            */
/* -------------------------------------------------------------------------- */
mod engine {
    use anyhow::{Context, Result};
    use std::fs;
    use std::path::{Path, PathBuf};

    pub mod vm {
        use super::*;
        use log::{debug, info};

        /// Exécute du bytecode (.vtbc). Ici: stub minimal qui affiche et retourne Ok.
        pub fn run_bc(input: &Path, args: &[String]) -> Result<()> {
            info!("[engine/vm] run_bc: {} {:?}", input.display(), args);
            // TODO: charger et exécuter le bytecode réel.
            println!("[engine] exécution bytecode: {} {:?}", input.display(), args);
            Ok(())
        }

        /// Exécute une source (.vt/.vit/.vitte). Ici: stub qui 'interprète' en lisant le fichier.
        pub fn run_source(input: &Path, args: &[String]) -> Result<()> {
            info!("[engine/vm] run_source: {} {:?}", input.display(), args);
            let code = fs::read_to_string(input)
                .with_context(|| format!("impossible de lire {}", input.display()))?;
            debug!("[engine] source size = {} bytes", code.len());
            // TODO: parser/évaluer. Pour le moment, afficher la première ligne.
            if let Some(line) = code.lines().next() {
                println!("[engine] → {}", line);
            }
            Ok(())
        }

        /// Évalue une ligne dans le REPL. Stub: renvoie la ligne trimée.
        pub fn eval_line(s: &str) -> Result<String> {
            let out = s.trim().to_string();
            Ok(out)
        }
    }

    pub mod compiler {
        use super::*;
        use log::info;

        /// Compile un fichier source vers un artefact (par défaut .vtbc).
        /// Stub: copie/transforme simplement selon `emit`.
        pub fn compile_file(
            input: &Path,
            out: Option<&Path>,
            _opt_level: u8,
            _target: Option<&str>,
            _threads: Option<u32>,
            emit: &str,
        ) -> Result<()> {
            info!("[engine/compiler] compile: {} (emit={})", input.display(), emit);
            let out_path: PathBuf = match out {
                Some(p) => p.to_path_buf(),
                None => match emit {
                    "obj" => input.with_extension("o"),
                    "exe" => {
                        if cfg!(windows) {
                            input.with_extension("exe")
                        } else {
                            input.with_extension("")
                        }
                    },
                    _ => input.with_extension("vtbc"),
                },
            };

            // Stub: pour l'instant on copie le fichier d'entrée vers la sortie.
            // Dans une implémentation réelle, on générerait du bytecode.
            fs::copy(input, &out_path)
                .with_context(|| format!("impossible d’écrire {}", out_path.display()))?;
            println!("[engine] → {}", out_path.display());
            Ok(())
        }
    }
}

/* -------------------------------------------------------------------------- */
/* API publique                                                               */
/* -------------------------------------------------------------------------- */

/// Point d’entrée CLI principal
pub fn run() -> Result<()> {
    env_logger::init();
    let cli = Cli::parse();

    match cli.command {
        Command::Version => print_version(),
        Command::Run { input, args } => run_program(&input, &args),
        Command::Compile { input, out, emit } => compile_file(&input, out.as_ref(), &emit),
        Command::Repl => repl_loop(),
    }
}

/* -------------------------------------------------------------------------- */
/* Implémentations des sous-commandes                                         */
/* -------------------------------------------------------------------------- */

/// Affiche la version complète
fn print_version() -> Result<()> {
    println!(
        "Vitte CLI {}\ncommit: {}\nbuild: {}\nrustc: {}",
        env!("CARGO_PKG_VERSION"),
        option_env!("GIT_COMMIT").unwrap_or("unknown"),
        option_env!("VERGEN_BUILD_DATE").unwrap_or("unknown"),
        option_env!("VERGEN_RUSTC_SEMVER").unwrap_or("unknown")
    );
    Ok(())
}

/// Exécution du moteur ou fallback simulé
fn run_program(input: &Path, args: &[String]) -> Result<()> {
    info!("▶️  run: {}", input.display());
    let is_bc = is_bytecode(input);
    if is_bc {
        engine::vm::run_bc(input, args)?;
    } else {
        engine::vm::run_source(input, args)?;
    }
    Ok(())
}

/// Compilation d’un fichier (réelle ou simulée)
fn compile_file(input: &Path, out: Option<&PathBuf>, emit: &str) -> Result<()> {
    info!("🧩 Compilation {} (emit={})", input.display(), emit);
    engine::compiler::compile_file(
        input,
        out.map(|p| p.as_path()),
        2,    // opt level (stub)
        None, // target (stub)
        None, // threads (stub)
        emit,
    )
}

/// REPL interactif basique (fallback)
fn repl_loop() -> Result<()> {
    info!("💬 REPL démarré");
    use std::io::{self, Write};
    let mut line = String::new();
    let mut stdout = io::stdout();
    loop {
        print!("vitte> ");
        stdout.flush().ok();
        line.clear();
        if io::stdin().read_line(&mut line)? == 0 {
            break;
        }
        let s = line.trim_end();
        if s == ":quit" || s == ":exit" {
            break;
        }
        match engine::vm::eval_line(s) {
            Ok(res) => println!("= {}", res),
            Err(e) => println!("! erreur: {e}"),
        }
    }
    Ok(())
}

/* -------------------------------------------------------------------------- */
/* API attendue par le binaire vitte-bin/src/main.rs                          */
/* -------------------------------------------------------------------------- */

/// Retourne une chaîne de version détaillée (utilisée par la commande `Version`).
pub fn version_string() -> String {
    format!(
        "Vitte CLI {}\ncommit: {}\nbuild: {}\nrustc: {}",
        env!("CARGO_PKG_VERSION"),
        option_env!("GIT_COMMIT").unwrap_or("unknown"),
        option_env!("VERGEN_BUILD_DATE").unwrap_or("unknown"),
        option_env!("VERGEN_RUSTC_SEMVER").unwrap_or("unknown"),
    )
}

/// Lance le REPL moteur (fallback intégré si le moteur n'est pas présent).
pub fn engine_repl() -> Result<()> {
    repl_loop()
}

/// Détecte grossièrement si un fichier ressemble à du bytecode.
/// Heuristique simple: extensions ".vtbc" ou ".bc".
pub fn is_bytecode(path: &Path) -> bool {
    match path.extension().and_then(|e| e.to_str()) {
        Some(ext) => matches!(ext, "vtbc" | "bc"),
        None => false,
    }
}

/// Exécute un fichier via le moteur ou via le fallback intégré.
pub fn engine_run(input: &Path, args: &[String]) -> Result<()> {
    run_program(input, args)
}

/// Compile un fichier via le moteur ou via le fallback intégré.
pub fn engine_compile(input: &Path, out: Option<&Path>, emit: &str) -> Result<()> {
    let out_buf: Option<PathBuf> = out.map(|p| p.to_path_buf());
    compile_file(input, out_buf.as_ref(), emit)
}

/// Calcule le chemin de sortie attendu lorsque `out` n'est pas fourni.
pub fn resolve_output(input: &Path, emit: &str) -> PathBuf {
    input.with_extension(emit)
}
