//! vitte-install — gestionnaire d’installation pour Vitte
//!
//! Ce binaire gère l’installation, la mise à jour et la désinstallation
//! des composants Vitte. Inspiré d’outils comme `rustup`.
//!
//! Fonctionnalités :
//! - Téléchargement de binaires depuis GitHub Releases ou un miroir local.
//! - Décompression et installation dans ~/.vitte (ou dossier défini par $VITTE_HOME).
//! - Mise à jour automatique à la dernière version stable.
//! - Désinstallation complète et nettoyage des caches.
//!
//! Implémentation actuelle : CLI fonctionnelle avec structure extensible.

use std::env;
use std::fs;
use std::path::PathBuf;
use std::process::{self, Command};

const VERSION: &str = env!("CARGO_PKG_VERSION");

fn print_help() {
    println!(
        "vitte-install {VERSION}

Gestionnaire d'installation pour Vitte

USAGE:
    vitte-install <command> [OPTIONS]

COMMANDS:
    install [version]   Installe une version de Vitte (par défaut: latest)
    update              Met à jour Vitte vers la dernière version
    uninstall           Désinstalle Vitte complètement
    doctor              Vérifie l’installation et les dépendances
    version             Affiche la version de vitte-install
    help                Affiche cette aide"
    );
}

fn print_version() {
    println!("vitte-install {}", VERSION);
}

/// Renvoie le chemin d’installation (~/.vitte par défaut ou $VITTE_HOME)
fn install_dir() -> PathBuf {
    if let Ok(home) = env::var("VITTE_HOME") {
        PathBuf::from(home)
    } else if let Some(home_dir) = dirs_next::home_dir() {
        home_dir.join(".vitte")
    } else {
        PathBuf::from("/tmp/vitte")
    }
}

fn ensure_install_dir() -> std::io::Result<PathBuf> {
    let dir = install_dir();
    if !dir.exists() {
        fs::create_dir_all(&dir)?;
    }
    Ok(dir)
}

fn cmd_install(version: Option<&str>) {
    let dir = match ensure_install_dir() {
        Ok(p) => p,
        Err(e) => {
            eprintln!("Erreur: impossible de créer le répertoire d’installation: {e}");
            process::exit(1);
        }
    };

    let version = version.unwrap_or("latest");
    println!("[vitte-install] Installation de Vitte {version} dans {}", dir.display());
    println!("(fonctionnalité non encore implémentée — future intégration avec vitte-update)");
}

fn cmd_update() {
    println!("[vitte-install] Mise à jour de Vitte vers la dernière version…");
    println!("(fonctionnalité non encore implémentée)");
}

fn cmd_uninstall() {
    let dir = install_dir();
    if dir.exists() {
        match fs::remove_dir_all(&dir) {
            Ok(_) => println!("Vitte a été désinstallé avec succès ({})", dir.display()),
            Err(e) => eprintln!("Erreur lors de la désinstallation: {e}"),
        }
    } else {
        println!("Aucune installation trouvée ({})", dir.display());
    }
}

fn cmd_doctor() {
    println!("[vitte-install doctor]");
    let rustc = Command::new("rustc").arg("--version").output();
    let cargo = Command::new("cargo").arg("--version").output();
    let git = Command::new("git").arg("--version").output();

    for (name, result) in [("rustc", rustc), ("cargo", cargo), ("git", git)] {
        match result {
            Ok(out) => println!("{name}: {}", String::from_utf8_lossy(&out.stdout).trim()),
            Err(_) => println!("{name}: non trouvé dans le PATH"),
        }
    }

    println!("Dossier d’installation : {}", install_dir().display());
}

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        print_help();
        process::exit(1);
    }

    match args[1].as_str() {
        "help" | "-h" | "--help" => print_help(),
        "version" | "-V" | "--version" => print_version(),
        "install" => {
            let version = if args.len() >= 3 { Some(args[2].as_str()) } else { None };
            cmd_install(version);
        }
        "update" => cmd_update(),
        "uninstall" => cmd_uninstall(),
        "doctor" => cmd_doctor(),
        other => {
            eprintln!("Erreur: commande inconnue `{other}`");
            print_help();
            process::exit(1);
        }
    }
}
