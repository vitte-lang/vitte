

//! vitte-new — Générateur de nouveaux projets Vitte
//!
//! Utilisation :
//! ```bash
//! vitte-new <commande>
//! ```
//! Commandes :
//! - `new <nom>` : crée un squelette de projet.
//! - `help` : affiche l’aide.
//! - `version` : affiche la version du binaire.

use std::env;
use std::process;

fn print_help() {
    println!("Vitte New — Créateur de projets Vitte");
    println!("Usage : vitte-new <commande>");
    println!("\nCommandes :");
    println!("  new <nom>     Crée un nouveau projet dans le dossier courant");
    println!("  help          Affiche cette aide");
    println!("  version       Affiche la version");
}

fn print_version() {
    println!("vitte-new 0.1.0");
}

fn create_project(name: &str) {
    println!("Création du projet : {}", name);
    std::fs::create_dir_all(name).unwrap_or_else(|e| {
        eprintln!("Erreur : {}", e);
        process::exit(1);
    });
    println!("Projet initialisé dans ./{}", name);
}

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        print_help();
        process::exit(1);
    }

    match args[1].as_str() {
        "help" => print_help(),
        "version" => print_version(),
        "new" => {
            if args.len() < 3 {
                eprintln!("Erreur : aucun nom de projet fourni.");
                process::exit(1);
            }
            create_project(&args[2]);
        }
        _ => {
            eprintln!("Commande inconnue : {}", args[1]);
            print_help();
            process::exit(1);
        }
    }
}