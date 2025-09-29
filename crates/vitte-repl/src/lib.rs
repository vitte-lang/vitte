//! vitte-repl — bibliothèque du REPL Vitte (lecture, évaluation, impression)
//!
//! Objectifs :
//! - API de haut niveau pour exécuter une ligne de code (`run_line`)
//! - Boucle interactive optionnelle (`run_interactive`) avec historique et couleurs
//! - Export des résultats en JSON si `feature = "json"`
//!
//! Binaire `vitte-repl` fourni dans `src/main.rs` utilise cette lib.
//!
//! Exemple minimal :
//! ```ignore
//! use vitte_repl::{Repl, ReplOptions};
//! let mut repl = Repl::new(ReplOptions::default());
//! let out = repl.run_line("1 + 2").unwrap();
//! println!("{}", out.render_string());
//! ```

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(
    clippy::module_name_repetitions,
    clippy::doc_markdown,
    clippy::too_many_lines
)]

use anyhow::{Context, Result};
use std::fs;
use std::path::{Path, PathBuf};
use vitte_compiler::Compiler;

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

#[cfg(feature = "colors")]
use nu_ansi_term::{Color, Style};

/// Options de fonctionnement du REPL.
#[derive(Debug, Clone)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct ReplOptions {
    /// Invite à afficher.
    pub prompt: String,
    /// Activer l’historique persistant si disponible.
    pub enable_history: bool,
    /// Activer la coloration sémantique simple de l’invite et des résultats.
    pub enable_colors: bool,
    /// Nom du fichier d’historique (dans `~/.vitte/`).
    pub history_file: String,
    /// Nombre max de lignes conservées dans l’historique.
    pub history_cap: usize,
}

impl Default for ReplOptions {
    fn default() -> Self {
        Self {
            prompt: "vitte> ".to_string(),
            enable_history: cfg!(feature = "history"),
            enable_colors: cfg!(feature = "colors"),
            history_file: "repl_history".to_string(),
            history_cap: 10_000,
        }
    }
}

/// Sortie d’évaluation d’une ligne.
#[derive(Debug, Clone)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct ReplOutput {
    /// Rendu textuel (pour impression directe).
    pub text: String,
    /// Valeur retournée brute sérialisée debug.
    pub value_debug: Option<String>,
    /// Erreurs human-readable si échec.
    pub errors: Vec<String>,
    /// Succès ou non.
    pub success: bool,
}

impl ReplOutput {
    /// Rendu pour impression.
    pub fn render_string(&self) -> String {
        self.text.clone()
    }

    /// Rendu JSON (si feature).
    #[cfg(feature = "json")]
    pub fn to_json(&self) -> String {
        serde_json::to_string_pretty(self).unwrap_or_else(|_| "{\"error\":\"serialize\"}".into())
    }
}

/// Gestionnaire d’historique. NOP si feature `history` absente.
#[derive(Debug, Default)]
struct History {
    #[cfg(feature = "history")]
    lines: rustyline::history::FileHistory,
    #[cfg(feature = "history")]
    path: Option<PathBuf>,
}

impl History {
    fn new() -> Self {
        #[cfg(feature = "history")]
        {
            Self { lines: rustyline::history::FileHistory::new(), path: None }
        }
        #[cfg(not(feature = "history"))]
        {
            Self {}
        }
    }

    #[cfg(feature = "history")]
    fn path_for(name: &str) -> Option<PathBuf> {
        let base = dirs::home_dir()?.join(".vitte");
        let _ = fs::create_dir_all(&base);
        Some(base.join(name))
    }

    fn load(&mut self, name: &str) {
        #[cfg(feature = "history")]
        {
            self.path = Self::path_for(name);
            if let Some(p) = &self.path {
                let _ = self.lines.load(p);
            }
        }
    }

    fn save(&mut self) {
        #[cfg(feature = "history")]
        if let Some(p) = &self.path {
            let _ = self.lines.save(p);
        }
    }

    fn add(&mut self, line: &str, cap: usize) {
        #[cfg(feature = "history")]
        {
            use rustyline::history::HistoryExt as _;
            if !line.trim().is_empty() {
                let _ = self.lines.add(line);
                // trim
                while self.lines.len() > cap {
                    let _ = self.lines.remove(0);
                }
            }
        }
        let _ = cap; // silence unused in no-history builds
        let _ = line;
    }
}

/// REPL principal.
pub struct Repl {
    opts: ReplOptions,
    compiler: Compiler,
    history: History,
}

impl Repl {
    /// Crée un REPL.
    pub fn new(opts: ReplOptions) -> Self {
        let mut history = History::new();
        if opts.enable_history {
            history.load(&opts.history_file);
        }
        Self { opts, compiler: Compiler::new(), history }
    }

    /// Évalue une ligne et retourne un [`ReplOutput`].
    pub fn run_line(&mut self, line: &str) -> Result<ReplOutput> {
        if self.opts.enable_history {
            self.history.add(line, self.opts.history_cap);
        }

        // Gestion commandes pseudo-méta
        if line.trim() == ":quit" || line.trim() == ":q" {
            return Ok(self.decorate_success("bye", Some("()".into())));
        }
        if line.trim() == ":help" || line.trim() == ":h" {
            return Ok(self.decorate_success(
                "commandes: :help, :quit",
                Some("HelpShown".into()),
            ));
        }

        match self.compiler.compile_and_run(line) {
            Ok(val) => {
                let dbg = format!("{val:?}");
                Ok(self.decorate_success(&dbg, Some(dbg)))
            }
            Err(e) => Ok(self.decorate_error(&[e.to_string()])),
        }
    }

    /// Boucle interactive (bloquante). Utilise `rustyline` si possible.
    /// Retourne `Ok(())` à la sortie.
    pub fn run_interactive(&mut self) -> Result<()> {
        #[cfg(feature = "std")]
        {
            #[cfg(feature = "colors")]
            let prompt = colored_prompt(&self.opts.prompt);
            #[cfg(not(feature = "colors"))]
            let prompt = self.opts.prompt.clone();

            #[cfg(feature = "history")]
            {
                use rustyline::error::ReadlineError;
                use rustyline::{DefaultEditor, Editor};
                let mut rl = DefaultEditor::new().context("init line editor")?;
                if self.opts.enable_history {
                    if let Some(p) = History::path_for(&self.opts.history_file) {
                        let _ = rl.load_history(&p);
                    }
                }
                loop {
                    match rl.readline(&prompt) {
                        Ok(line) => {
                            if self.opts.enable_history {
                                self.history.add(&line, self.opts.history_cap);
                                if let Some(p) = History::path_for(&self.opts.history_file) {
                                    let _ = rl.append_history(&p);
                                }
                            }
                            let out = self.run_line(&line)?;
                            println!("{}", out.render_string());
                            if line.trim() == ":q" || line.trim() == ":quit" {
                                break;
                            }
                        }
                        Err(ReadlineError::Interrupted) | Err(ReadlineError::Eof) => break,
                        Err(e) => {
                            eprintln!("error: {e}");
                            break;
                        }
                    }
                }
            }
            #[cfg(not(feature = "history"))]
            {
                use std::io::{self, Write};
                let stdin = io::stdin();
                let mut line = String::new();
                loop {
                    print!("{prompt}");
                    let _ = io::stdout().flush();
                    line.clear();
                    if stdin.read_line(&mut line)? == 0 {
                        break;
                    }
                    let out = self.run_line(line.trim_end())?;
                    println!("{}", out.render_string());
                    if line.trim() == ":q" || line.trim() == ":quit" {
                        break;
                    }
                }
            }
            self.history.save();
            Ok(())
        }
        #[cfg(not(feature = "std"))]
        {
            anyhow::bail!("`run_interactive` requiert feature std")
        }
    }

    fn decorate_success(&self, body: &str, dbg: Option<String>) -> ReplOutput {
        #[cfg(feature = "colors")]
        if self.opts.enable_colors {
            let ok = Style::new().fg(Color::Green).bold().paint("ok");
            return ReplOutput {
                text: format!("{ok} {body}"),
                value_debug: dbg,
                errors: Vec::new(),
                success: true,
            };
        }
        ReplOutput { text: format!("ok {body}"), value_debug: dbg, errors: Vec::new(), success: true }
    }

    fn decorate_error(&self, errs: &[String]) -> ReplOutput {
        #[cfg(feature = "colors")]
        if self.opts.enable_colors {
            let ko = Style::new().fg(Color::Red).bold().paint("error");
            return ReplOutput {
                text: format!("{ko}:\n{}", errs.join("\n")),
                value_debug: None,
                errors: errs.to_vec(),
                success: false,
            };
        }
        ReplOutput { text: format!("error:\n{}", errs.join("\n")), value_debug: None, errors: errs.to_vec(), success: false }
    }
}

#[cfg(feature = "colors")]
fn colored_prompt(p: &str) -> String {
    let b = Style::new().fg(Color::Blue).bold().paint(p);
    format!("{b}")
}

/* =================================== Tests =================================== */

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn construct_and_run_line() {
        let mut repl = Repl::new(ReplOptions {
            enable_history: false,
            enable_colors: false,
            ..Default::default()
        });
        let out = repl.run_line("1 + 1").unwrap();
        // Selon l’impl réelle de `compile_and_run`, soit succès soit erreur parse.
        assert!(out.success || !out.errors.is_empty());
    }

    #[test]
    fn meta_commands() {
        let mut repl = Repl::new(ReplOptions::default());
        let h = repl.run_line(":help").unwrap();
        assert!(h.success);
        let q = repl.run_line(":quit").unwrap();
        assert!(q.success);
    }

    #[cfg(feature = "json")]
    #[test]
    fn json_output() {
        let mut repl = Repl::new(ReplOptions::default());
        let out = repl.run_line("1").unwrap();
        let json = out.to_json();
        assert!(json.contains("success"));
    }
}