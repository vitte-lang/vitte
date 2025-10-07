//! vitte-borrowck — borrow checker for Vitte
//!
//! Analyse et vérification des emprunts et durées de vie dans les programmes Vitte.
//! Inspiré du borrow checker de Rust mais adapté à la sémantique de Vitte.
//!
//! Fonctionnalités prévues :
//! - Analyse des fonctions, blocs et variables.
//! - Vérification des prêts immuables et mutables.
//! - Détection des durées de vie invalides.
//! - Intégration avec l’IR (`vitte-ir`) et la couche erreurs (`vitte-errors`).
//!
//! CLI “ultra complète” : supporte sous-commandes, options et diagnostics.

use std::env;
use std::process;

fn print_help() {
    eprintln!("Usage: vitte-borrowck <command> [OPTIONS]");
    eprintln!();
    eprintln!("Commands:");
    eprintln!("  check <input>     Analyse un fichier source Vitte");
    eprintln!("  lint <input>      Vérifie rapidement sans analyse complète");
    eprintln!("  version           Affiche la version");
    eprintln!("  help              Affiche cette aide");
    eprintln!();
    eprintln!("Options:");
    eprintln!("  -q, --quiet       Mode silencieux");
    eprintln!("  -v, --verbose     Mode verbeux");
}

fn print_version() {
    println!("vitte-borrowck {}", env!("CARGO_PKG_VERSION"));
}

fn cmd_check(file: &str) {
    eprintln!("[borrowck] analyse complète sur {file}");
    // TODO: charger vitte-ir, exécuter borrow checker et rapporter diagnostics.
}

fn cmd_lint(file: &str) {
    eprintln!("[borrowck] lint rapide sur {file}");
    // TODO: implémenter un mode allégé de vérification.
}

fn main() {
    let mut args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        print_help();
        process::exit(1);
    }

    let cmd = args.remove(1);
    match cmd.as_str() {
        "help" | "-h" | "--help" => {
            print_help();
        },
        "version" | "-V" | "--version" => {
            print_version();
        },
        "check" => {
            if args.len() < 2 {
                eprintln!("error: missing input file for `check`");
                process::exit(1);
            }
            let file = &args[1];
            cmd_check(file);
        },
        "lint" => {
            if args.len() < 2 {
                eprintln!("error: missing input file for `lint`");
                process::exit(1);
            }
            let file = &args[1];
            cmd_lint(file);
        },
        other => {
            eprintln!("error: unknown command `{other}`");
            print_help();
            process::exit(1);
        },
    }
}
