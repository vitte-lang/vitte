/// Retourne l'extension en lowercase (sans le point)
pub fn ext_lower(path: impl AsRef<std::path::Path>) -> Option<String> {
    path.as_ref()
        .extension()
        .and_then(|e| e.to_str())
        .map(|e| e.to_ascii_lowercase())
}

/// Indique si le fichier correspond à un bytecode Vitte `.vtbc`.
pub fn is_bytecode(path: impl AsRef<std::path::Path>) -> bool {
    matches!(ext_lower(path).as_deref(), Some("vtbc"))
}

/// Calcule le chemin de sortie pour un artefact demandé.
/// `emit` peut valoir "bc" | "obj" | "exe"; défaut → `.vtbc`.
pub fn resolve_output(input: &std::path::Path, emit: &str) -> std::path::PathBuf {
    match emit {
        "obj" => input.with_extension("o"),
        "exe" => {
            if cfg!(windows) { input.with_extension("exe") } else { input.with_extension("") }
        }
        _ => input.with_extension("vtbc"),
    }
}

use anyhow::Result;
use clap::{Parser, Subcommand};
use env_logger;
use log::error;

#[derive(Debug, Parser)]
#[command(name = "vitte-bin", version, about = "Vitte CLI (binaire workspace)")]
struct Cli {
    #[command(subcommand)]
    command: Command,
}

#[derive(Debug, Subcommand)]
enum Command {
    /// Affiche la version détaillée
    Version,
}

fn init_logger() {
    let _ = env_logger::Builder::from_env(env_logger::Env::default().default_filter_or("info"))
        .format_timestamp_millis()
        .try_init();
}

fn main() {
    init_logger();
    let cli = Cli::parse();
    if let Err(e) = real_main(cli) {
        error!("{e:#}");
        std::process::exit(1);
    }
}

fn real_main(cli: Cli) -> Result<()> {
    match cli.command {
        Command::Version => {
            println!("{}", vitte_bin::version_string());
        }
    }
    Ok(())
}
