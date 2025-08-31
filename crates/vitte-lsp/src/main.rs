//! Entrée binaire pour le serveur LSP de Vitte (transport: stdio).
//!
//! Usage :
//!   vitte-lsp --stdio      # par défaut
//!   vitte-lsp --version
//!   vitte-lsp --help

use std::env;

#[tokio::main]
async fn main() {
    // Mini parseur d’arguments (pas de dépendance clap, keep it lean).
    let mut mode_stdio = true;

    for arg in env::args().skip(1) {
        match arg.as_str() {
            "--stdio" => mode_stdio = true,
            "--version" | "-V" => {
                println!("vitte-lsp {}", env!("CARGO_PKG_VERSION"));
                return;
            }
            "--help" | "-h" => {
                println!(
                    "vitte-lsp {}\n\nUsage:\n  vitte-lsp [--stdio]\n  vitte-lsp --version\n  vitte-lsp --help",
                    env!("CARGO_PKG_VERSION")
                );
                return;
            }
            other => {
                eprintln!("warning: unknown argument: {other}");
            }
        }
    }

    if mode_stdio {
        if let Err(e) = vitte_lsp::start_stdio().await {
            // On loggue sur stderr pour que le client LSP ne soit pas perturbé sur stdout.
            eprintln!("vitte-lsp: fatal error: {e}");
            std::process::exit(1);
        }
    }
}
