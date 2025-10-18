use std::borrow::Cow;
use std::fs;
use std::path::PathBuf;

use anyhow::{Context, Result};
use log::{error, info, warn};
use dirs;
use rustyline::error::ReadlineError;
use rustyline::{config::Configurer, DefaultEditor};

const PRIMARY_PROMPT: &str = ">>> ";
const CONTINUATION_PROMPT: &str = "... ";
const HISTORY_FILE: &str = "repl-history";

pub fn start() -> Result<()> {
    info!("💬 REPL démarré");
    let mut repl = Repl::new(PRIMARY_PROMPT, CONTINUATION_PROMPT)?;
    repl.load_history();
    repl.run()?;
    repl.save_history();
    Ok(())
}

struct Repl<'a> {
    prompt_primary: Cow<'a, str>,
    prompt_continuation: Cow<'a, str>,
    editor: DefaultEditor,
    history_path: Option<PathBuf>,
}

impl<'a> Repl<'a> {
    fn new(primary: &'a str, continuation: &'a str) -> Result<Self> {
        let mut editor = DefaultEditor::new().context("initialisation du REPL (rustyline)")?;
        editor.set_auto_add_history(false);
        editor.set_check_cursor_position(true);

        Ok(Self {
            prompt_primary: Cow::Borrowed(primary),
            prompt_continuation: Cow::Borrowed(continuation),
            editor,
            history_path: history_file_path()?,
        })
    }

    fn run(&mut self) -> Result<()> {
        let mut buffer = String::new();
        loop {
            let prompt = if buffer.is_empty() {
                &self.prompt_primary
            } else {
                &self.prompt_continuation
            };

            match self.editor.readline(prompt.as_ref()) {
                Ok(line) => {
                    if self.handle_input(line, &mut buffer)? {
                        break;
                    }
                },
                Err(ReadlineError::Interrupted) => {
                    if buffer.is_empty() {
                        println!("^C");
                    } else {
                        println!("^C (bloc ignoré)");
                        buffer.clear();
                    }
                    continue;
                },
                Err(ReadlineError::Eof) => {
                    println!();
                    break;
                },
                Err(err) => {
                    error!("REPL interrompu: {err}");
                    break;
                },
            }
        }
        Ok(())
    }

    fn handle_input(&mut self, line: String, buffer: &mut String) -> Result<bool> {
        let trimmed = line.trim_end();
        if buffer.is_empty() && trimmed.starts_with(':') {
            return self.handle_command(trimmed);
        }

        if needs_continuation(trimmed) {
            buffer.push_str(trimmed.trim_end_matches('\\'));
            buffer.push('\n');
            return Ok(false);
        }

        let to_eval = if buffer.is_empty() {
            trimmed.to_string()
        } else {
            buffer.push_str(trimmed);
            let completed = buffer.clone();
            buffer.clear();
            completed
        };

        if to_eval.trim().is_empty() {
            return Ok(false);
        }

        match crate::engine::vm::eval_line(to_eval.trim()) {
            Ok(result) => println!("= {}", result),
            Err(err) => println!("! erreur: {err}"),
        }

        if let Err(err) = self.editor.add_history_entry(to_eval.trim()) {
            warn!("Impossible d'ajouter l'entrée à l'historique: {err}");
        }

        Ok(false)
    }

    fn handle_command(&mut self, cmd: &str) -> Result<bool> {
        let cmd = cmd.trim();
        match cmd {
            ":quit" | ":exit" => Ok(true),
            ":history" => {
                for (idx, entry) in self.editor.history().iter().enumerate() {
                    println!("{idx:>4}  {entry}");
                }
                Ok(false)
            },
            ":clear" => {
                self.editor.clear_screen()?;
                Ok(false)
            },
            ":help" => {
                println!("Commandes :");
                println!("  :help      afficher cette aide");
                println!("  :history   lister l'historique");
                println!("  :clear     nettoyer l'écran");
                println!("  :quit      quitter le REPL");
                Ok(false)
            },
            other => {
                println!("Commande inconnue `{other}`. Tape :help pour la liste.");
                Ok(false)
            },
        }
    }

    fn load_history(&mut self) {
        if let Some(path) = &self.history_path {
            if path.exists() {
                if let Err(err) = self.editor.load_history(path) {
                    warn!("Impossible de charger l'historique (`{}`): {err}", path.display());
                }
            }
        }
    }

    fn save_history(&mut self) {
        if let Some(path) = &self.history_path {
            if let Err(err) = self.editor.save_history(path) {
                warn!("Impossible d'enregistrer l'historique (`{}`): {err}", path.display());
            }
        }
    }
}

fn history_file_path() -> Result<Option<PathBuf>> {
    if let Some(mut dir) = dirs::data_dir() {
        dir.push("vitte");
        fs::create_dir_all(&dir).context("création du dossier d'historique REPL")?;
        dir.push(HISTORY_FILE);
        Ok(Some(dir))
    } else {
        Ok(None)
    }
}

fn needs_continuation(line: &str) -> bool {
    let trimmed = line.trim_end();
    trimmed.ends_with('\\')
        || trimmed.ends_with('{')
        || trimmed.ends_with('(')
        || trimmed.ends_with('[')
        || trimmed.ends_with(':')
}
