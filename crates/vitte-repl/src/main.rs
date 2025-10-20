//! vitte-repl — boucle interactive minimale.
//!
//! Cette version est autonome pour assurer la compilation du binaire
//! même si les crates `vitte_runtime`, `vitte_parser`, `vitte_core`,
//! ou `vitte_eval` ne sont pas disponibles. Elle offre :
//! - saisie interactive
//! - commandes :help, :quit, :load, :save
//! - un évaluateur factice qui « évalue » en renvoyant l'entrée

use std::fs;
use std::io::{self, Write};
use std::path::PathBuf;

/// Constante d'identification de version.
const VERSION: &str = env!("CARGO_PKG_VERSION");

/// Runtime minimal stockant l'historique d'évaluation.
#[derive(Default)]
struct Runtime {
    history: Vec<String>,
}

impl Runtime {
    fn dump_state(&self) -> String {
        self.history.join("\n")
    }
}

/// Évaluateur factice (échoue jamais, renvoie l'entrée normalisée).
struct Evaluator;

impl Evaluator {
    fn new() -> Self {
        Self
    }

    fn eval(&mut self, rt: &mut Runtime, src: &str) -> Result<String, String> {
        let out = src.trim().to_string();
        if !out.is_empty() {
            rt.history.push(out.clone());
        }
        Ok(out)
    }
}

/// Analyseur factice: renvoie la chaîne telle quelle.
fn parse(input: &str) -> Result<String, String> {
    Ok(input.to_string())
}

fn main() -> io::Result<()> {
    println!("Vitte REPL v{} — Tapez `:help` pour l’aide.", VERSION);

    let mut rt = Runtime::default();
    let mut eval = Evaluator::new();
    let mut buffer = String::new();

    loop {
        print!("vitte> ");
        io::stdout().flush()?;
        buffer.clear();

        if io::stdin().read_line(&mut buffer)? == 0 {
            println!();
            break;
        }

        let line = buffer.trim();
        if line.is_empty() {
            continue;
        }

        if line.starts_with(':') {
            if !handle_command(line, &mut rt)? {
                break;
            }
            continue;
        }

        match parse(line) {
            Ok(ast) => match eval.eval(&mut rt, &ast) {
                Ok(val) => println!("=> {}", val),
                Err(e) => eprintln!("Erreur d’exécution : {}", e),
            },
            Err(e) => eprintln!("Erreur de syntaxe : {}", e),
        }
    }

    println!("Au revoir !");
    Ok(())
}

/// Gestion des commandes spéciales du REPL (`:quit`, `:help`, `:load`, `:save`).
fn handle_command(cmd: &str, rt: &mut Runtime) -> io::Result<bool> {
    let parts: Vec<&str> = cmd.split_whitespace().collect();
    match parts.get(0).copied().unwrap_or("") {
        ":quit" | ":exit" => return Ok(false),
        ":help" => {
            println!(
                "Commandes disponibles :\n  :help        — affiche cette aide\n  :quit        — quitte le REPL\n  :load <f>    — charge et ‘évalue’ chaque ligne du fichier\n  :save <f>    — sauvegarde l’historique de la session"
            );
        }
        ":load" => {
            if let Some(path) = parts.get(1) {
                match fs::read_to_string(path) {
                    Ok(src) => {
                        println!("Chargement de `{}`...", path);
                        let mut eval = Evaluator::new();
                        for (i, line) in src.lines().enumerate() {
                            let line = line.trim();
                            if line.is_empty() {
                                continue;
                            }
                            match parse(line) {
                                Ok(ast) => match eval.eval(rt, &ast) {
                                    Ok(val) => println!("[{}] => {}", i + 1, val),
                                    Err(e) => eprintln!("[{}] Erreur d’exécution : {}", i + 1, e),
                                },
                                Err(e) => eprintln!("[{}] Erreur de parsing : {}", i + 1, e),
                            }
                        }
                    }
                    Err(e) => eprintln!("Impossible de lire le fichier : {}", e),
                }
            } else {
                eprintln!("Usage : :load <fichier>");
            }
        }
        ":save" => {
            if let Some(path) = parts.get(1) {
                let p = PathBuf::from(path);
                let dump = rt.dump_state();
                fs::write(&p, dump)?;
                println!("État sauvegardé dans {}", p.display());
            } else {
                eprintln!("Usage : :save <fichier>");
            }
        }
        _ => eprintln!("Commande inconnue : {}", cmd),
    }
    Ok(true)
}
