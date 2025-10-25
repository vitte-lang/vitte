//! Boucle REPL de base (scaffolding multi-ligne).

use crate::buffer::SessionBuffer;
use crate::document::{CellOutcome, SessionDocument};
use crate::history::{HistoryConfig, HistoryManager};
use crate::lsp_client::{discover_sessions, CellDigest, LspClient, LspClientError};
use crate::viz::ValueViz;
use std::io::{self, Write};
use thiserror::Error;
use url::Url;

/// Erreurs retournées par le REPL.
#[derive(Debug, Error)]
pub enum ReplError {
    /// Erreur générique.
    #[error("{0}")]
    Generic(String),
    /// Erreur de persistance d'historique.
    #[error("history error: {0}")]
    History(String),
    /// Erreur LSP.
    #[error("lsp error: {0}")]
    Lsp(String),
}

/// Alias de résultat.
pub type Result<T> = std::result::Result<T, ReplError>;

/// Configuration du REPL.
#[derive(Debug, Clone)]
pub struct ReplOptions {
    /// Indique si le prompt doit afficher les numéros de cellules.
    pub show_cell_numbers: bool,
    /// Configuration de l'historique.
    pub history: HistoryConfig,
}

impl Default for ReplOptions {
    fn default() -> Self {
        Self {
            show_cell_numbers: false,
            history: HistoryConfig::default(),
        }
    }
}

/// Résultat d'un appel à `Repl::eval`.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum EvalResult {
    /// Le bloc n'est pas encore complet, le REPL attend davantage d'entrées.
    Pending,
    /// Le bloc a été évalué et un résultat textuel est disponible.
    Output(String),
    /// Aucun changement (le code est identique à la version précédente).
    NoChange,
}

/// REPL principal.
#[derive(Debug)]
pub struct Repl {
    opts: ReplOptions,
    buffer: SessionBuffer,
    doc: SessionDocument,
    history: Option<HistoryManager>,
    lsp: Option<LspClient>,
    cell_counter: usize,
}

impl Repl {
    /// Crée un nouveau REPL.
    pub fn new(opts: ReplOptions) -> Self {
        let history = HistoryManager::from_config(&opts.history)
            .unwrap_or_else(|err| {
                eprintln!("vitte-repl: unable to load history: {err}");
                None
            });
        Self {
            opts,
            buffer: SessionBuffer::default(),
            doc: SessionDocument::default(),
            history,
            lsp: None,
            cell_counter: 0,
        }
    }

    /// Boucle interactive simple (sans édition avancée).
    pub fn run(&mut self) -> Result<()> {
        let stdin = io::stdin();
        let mut stdout = io::stdout();
        loop {
            self.print_prompt(&mut stdout);
            stdout.flush().map_err(|e| ReplError::Generic(e.to_string()))?;

            let mut line = String::new();
            let bytes = stdin
                .read_line(&mut line)
                .map_err(|e| ReplError::Generic(e.to_string()))?;
            if bytes == 0 {
                break;
            }

            match self.eval(line.trim_end_matches('\n'))? {
                EvalResult::Pending => {}
                EvalResult::Output(out) => {
                    if !out.is_empty() {
                        writeln!(stdout, "{out}")
                            .map_err(|e| ReplError::Generic(e.to_string()))?;
                    }
                }
                EvalResult::NoChange => {
                    writeln!(stdout, "// no change")
                        .map_err(|e| ReplError::Generic(e.to_string()))?;
                }
            }
        }
        Ok(())
    }

    /// Soumet une ligne au REPL (utilisable dans les tests).
    pub fn eval(&mut self, line: &str) -> Result<EvalResult> {
        if self.buffer.is_empty() {
            let trimmed = line.trim();
            if trimmed.starts_with(':') {
                return self.eval_command(trimmed);
            }
        }
        if self.buffer.push_line(line) {
            let chunk = self.buffer.flush();
            if let Some(result) = self.handle_visualization(&chunk) {
                return result;
            }
            self.cell_counter += 1;
            let chunk_clone = chunk.clone();
            match self.doc.submit(chunk) {
                CellOutcome::Evaluated { output } => {
                    if let Some(history) = self.history.as_mut() {
                        history
                            .record(&chunk_clone)
                            .map_err(|e| ReplError::History(e.to_string()))?;
                    }
                    Ok(EvalResult::Output(output))
                }
                CellOutcome::NoChange => Ok(EvalResult::NoChange),
            }
        } else {
            Ok(EvalResult::Pending)
        }
    }

    fn print_prompt(&self, stdout: &mut impl Write) {
        let prompt = if self.buffer.is_empty() {
            if self.opts.show_cell_numbers {
                format!(">>> [{}] ", self.cell_counter + 1)
            } else {
                ">>> ".to_string()
            }
        } else {
            "... ".to_string()
        };
        let _ = stdout.write_all(prompt.as_bytes());
    }

    /// Retourne l'historique si disponible (utilisé pour les tests).
    pub fn history_entries(&self) -> Option<&[String]> {
        self.history.as_ref().map(|h| h.entries())
    }

    fn handle_visualization(&self, chunk: &str) -> Option<Result<EvalResult>> {
        let trimmed = chunk.trim();
        if trimmed.starts_with(":viz-json ") {
            let json = trimmed.trim_start_matches(":viz-json ").trim();
            Some(match ValueViz::from_json_str(json) {
                Ok(value) => Ok(EvalResult::Output(value.render_ascii())),
                Err(err) => Err(ReplError::Generic(format!("invalid viz json: {err}"))),
            })
        } else if trimmed.starts_with(":viz-ascii ") {
            let json = trimmed.trim_start_matches(":viz-ascii ").trim();
            Some(match ValueViz::from_json_str(json) {
                Ok(value) => Ok(EvalResult::Output(value.render_ascii())),
                Err(err) => Err(ReplError::Generic(format!("invalid viz json: {err}"))),
            })
        } else if trimmed == ":viz-help" {
            Some(Ok(EvalResult::Output(
                "Usage:\n  :viz-json <json>\n  :viz-ascii <json>".into(),
            )))
        } else {
            None
        }
    }

    fn eval_command(&mut self, command: &str) -> Result<EvalResult> {
        match command {
            ":lsp-status" => {
                let msg = self
                    .lsp
                    .as_ref()
                    .map(|client| client.status_line())
                    .unwrap_or_else(|| "LSP: disconnected".into());
                Ok(EvalResult::Output(msg))
            }
            ":lsp-sync" => {
                let client = self
                    .lsp
                    .as_mut()
                    .ok_or_else(|| ReplError::Lsp("not connected".into()))?;
                let digests: Vec<CellDigest> = self.doc.digests();
                match client.sync_state(&digests) {
                    Ok(resp) => Ok(EvalResult::Output(format!(
                        "LSP sync: {} missing cells, {} exports",
                        resp.missing_cells.len(),
                        resp.exports.len()
                    ))),
                    Err(err) => Err(ReplError::Lsp(err.to_string())),
                }
            }
            ":lsp-disconnect" => {
                if let Some(client) = self.lsp.take() {
                    client.close();
                    Ok(EvalResult::Output("LSP disconnected".into()))
                } else {
                    Ok(EvalResult::Output("LSP already disconnected".into()))
                }
            }
            other if other.starts_with(":lsp-connect") => {
                let endpoint = other[":lsp-connect".len()..].trim();
                let client = if endpoint.is_empty() {
                    connect_default_session()
                } else {
                    match Url::parse(endpoint) {
                        Ok(url) => LspClient::connect_endpoint(url, None),
                        Err(err) => Err(LspClientError::Network(err.to_string())),
                    }
                }
                .map_err(|e| ReplError::Lsp(e.to_string()))?;
                let summary = client.status_line();
                self.lsp = Some(client);
                Ok(EvalResult::Output(format!("LSP connected: {summary}")))
            }
            ":help" => Ok(EvalResult::Output(
                "Commands: :lsp-connect [endpoint], :lsp-status, :lsp-sync, :lsp-disconnect".into(),
            )),
            other => Ok(EvalResult::Output(format!("unknown command: {other}"))),
        }
    }
}

fn connect_default_session() -> std::result::Result<LspClient, LspClientError> {
    match LspClient::connect_default() {
        Ok(client) => Ok(client),
        Err(LspClientError::NoSession) => {
            let sessions = discover_sessions()?;
            let info = sessions
                .into_iter()
                .find(|s| s.primary)
                .ok_or(LspClientError::NoSession)?;
            LspClient::connect(info)
        }
        Err(err) => Err(err),
    }
}
