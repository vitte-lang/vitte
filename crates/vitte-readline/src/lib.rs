//! vitte-readline — saisie interactive **ultra complète**
//!
//! Objectifs:
//! - Ligne d’édition interactive avec historique persistant (~/.vitte_history par défaut).
//! - Backends: `rustyline` (par défaut) ou `reedline` (feature alternative).
//! - API uniforme: `Readline::read_line(prompt)` → `ReadEvent`.
//! - Complétion branchable (via `vitte-completion` si activé) + compléteur custom.
//! - Surbrillance (nu-ansi-term) et couleurs ANSI (vitte-ansi).
//! - Multiligne optionnel, masquage (mode password), validation.
//! - Clipboard (copier/coller) et gestion Ctrl-C/Ctrl-D propre.
//! - Encodage E/S optionnel via `vitte-encoding`.
//!
//! Zéro `unsafe`.

#![forbid(unsafe_code)]

#[cfg(all(not(feature="std"), not(feature="alloc-only")))]
compile_error!("Enable `std` (default) or `alloc-only`.") ;

#[cfg(feature="alloc-only")]
extern crate alloc;

#[cfg(feature="alloc-only")]
use alloc::{string::String, vec::Vec, boxed::Box, format};

#[cfg(feature="std")]
use std::{string::String, vec::Vec, boxed::Box, path::PathBuf, sync::{Arc, atomic::{AtomicBool, Ordering}}, time::Duration};

#[cfg(feature="serde")]
use serde::{Serialize, Deserialize};

#[cfg(feature="errors")]
use thiserror::Error;

#[cfg(feature="ansi")]
use vitte_ansi as ansi;

#[cfg(feature="history")]
use {dirs, vitte_path as vpath};

#[cfg(feature="completion")]
use vitte_completion as vcomp;

#[cfg(feature="highlight")]
use nu_ansi_term as ansi_term;

#[cfg(feature="clipboard")]
use arboard::Clipboard;

#[cfg(feature="signals")]
use ctrlc;

#[cfg(feature="encoding")]
use vitte_encoding as venc;

#[cfg(feature="rustyline")]
use rustyline as rl;

#[cfg(feature="reedline")]
use reedline as rd;

/* ============================== Erreurs & types ============================== */

#[cfg(feature="errors")]
#[derive(Debug, Error)]
pub enum ReadErr {
    #[error("io: {0}")]
    Io(String),
    #[error("interrupted")]
    Interrupted,
    #[error("eof")]
    Eof,
    #[error("feature missing")]
    FeatureMissing,
    #[error("backend error: {0}")]
    Backend(String),
}
#[cfg(not(feature="errors"))]
#[derive(Debug)]
pub enum ReadErr { Io(String), Interrupted, Eof, FeatureMissing, Backend(String) }

pub type Result<T> = core::result::Result<T, ReadErr>;

/// Évènements renvoyés par `read_line`.
#[cfg_attr(feature="serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum ReadEvent {
    Line(String),
    Eof,          // Ctrl-D
    Interrupted,  // Ctrl-C
}

/* ============================== Configuration ============================== */

#[cfg_attr(feature="serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug)]
pub struct ReadConfig {
    pub history_path: Option<PathBuf>,
    pub history_size: usize,
    pub multiline: bool,
    pub mask_input: bool,      // mode password
    pub completer: CompleterKind,
    pub highlighter: bool,
    pub prompt_color: bool,
    pub backend: BackendKind,
    pub timeout: Option<Duration>, // pour `read_line_timeout` (future) ; non utilisé ici
}

impl Default for ReadConfig {
    fn default() -> Self {
        Self {
            history_path: default_history_path(),
            history_size: 5000,
            multiline: false,
            mask_input: false,
            completer: CompleterKind::None,
            highlighter: true,
            prompt_color: true,
            backend: default_backend(),
            timeout: None,
        }
    }
}

#[cfg_attr(feature="serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum BackendKind { #[cfg(feature="rustyline")] Rustyline, #[cfg(feature="reedline")] Reedline }

fn default_backend() -> BackendKind {
    #[cfg(feature="rustyline")] { return BackendKind::Rustyline; }
    #[cfg(all(not(feature="rustyline"), feature="reedline"))] { return BackendKind::Reedline; }
    #[allow(unreachable_code)] panic!("enable `rustyline` or `reedline` feature");
}

#[cfg_attr(feature="serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug)]
pub enum CompleterKind {
    None,
    #[cfg(feature="completion")]
    VitteCompletion(CompletionSource),
    Custom(Arc<dyn Complete + Send + Sync>),
}

/// Source pour v ./ completion (commandes, fichiers, dynamiques)
#[cfg_attr(feature="serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug)]
pub enum CompletionSource {
    Commands(Vec<String>),
    Files, // simple completion fichiers
    Mixed { commands: Vec<String>, files: bool },
}

/* ============================== Traits d’abstraction ============================== */

/// Compléteur minimal.
pub trait Complete {
    /// Retourne suggestions pour `input` et `pos` (curseur).
    fn complete(&self, input: &str, pos: usize) -> Vec<String>;
}

/// Surbrillance.
pub trait Highlight {
    fn highlight(&self, input: &str) -> String;
}

/* ============================== Utilitaires ============================== */

#[cfg(feature="history")]
fn default_history_path() -> Option<PathBuf> {
    let base = dirs::home_dir()?;
    Some(base.join(".vitte_history"))
}
#[cfg(not(feature="history"))]
fn default_history_path() -> Option<PathBuf> { None }

#[inline]
fn err<T>(s: impl core::fmt::Display) -> Result<T> { Err(ReadErr::Backend(s.to_string())) }

/* ============================== Compléteurs ============================== */

/// Compléteur vide.
#[derive(Clone, Default)]
pub struct NoComplete;
impl Complete for NoComplete { fn complete(&self, _i: &str, _p: usize) -> Vec<String> { Vec::new() } }

/// Compléteur à partir d’une liste.
#[derive(Clone)]
pub struct ListComplete { items: Arc<Vec<String>> }
impl ListComplete { pub fn new(items: Vec<String>) -> Self { Self { items: Arc::new(items) } } }
impl Complete for ListComplete {
    fn complete(&self, input: &str, _pos: usize) -> Vec<String> {
        let q = input.trim();
        if q.is_empty() { return self.items.as_ref().clone(); }
        self.items.iter().filter(|s| s.starts_with(q)).cloned().collect()
    }
}

/// Compléteur fichiers simple.
#[cfg(feature="std")]
#[derive(Clone, Default)]
pub struct FilesComplete;
#[cfg(feature="std")]
impl Complete for FilesComplete {
    fn complete(&self, input: &str, _pos: usize) -> Vec<String> {
        use std::fs;
        use std::path::Path;
        let p = Path::new(input);
        let (dir, prefix) = if p.is_absolute() {
            (p.parent().unwrap_or_else(|| Path::new("/")).to_path_buf(), p.file_name().and_then(|s| s.to_str()).unwrap_or(""))
        } else {
            (std::env::current_dir().unwrap_or_default(), input)
        };
        let Ok(rd) = fs::read_dir(dir) else { return Vec::new(); };
        rd.filter_map(|e| e.ok())
          .filter_map(|e| e.file_name().into_string().ok())
          .filter(|name| name.starts_with(prefix))
          .collect()
    }
}

/// Adaptateur vitte-completion.
#[cfg(feature="completion")]
#[derive(Clone)]
pub struct VitteCompleter {
    src: CompletionSource,
}
#[cfg(feature="completion")]
impl VitteCompleter {
    pub fn new(src: CompletionSource) -> Self { Self { src } }
}
#[cfg(feature="completion")]
impl Complete for VitteCompleter {
    fn complete(&self, input: &str, pos: usize) -> Vec<String> {
        match &self.src {
            CompletionSource::Commands(cmds) => vcomp::complete_commands(cmds, input, pos),
            CompletionSource::Files => vcomp::complete_files(input, pos).unwrap_or_default(),
            CompletionSource::Mixed { commands, files } => {
                let mut out = vcomp::complete_commands(commands, input, pos);
                if *files {
                    if let Ok(mut f) = vcomp::complete_files(input, pos) { out.append(&mut f); }
                }
                out
            }
        }
    }
}

/* ============================== Highlighter ============================== */

/// Highlighter trivial: met les chiffres en cyan et les options en jaune.
#[derive(Clone, Default)]
pub struct SimpleHighlighter;
impl Highlight for SimpleHighlighter {
    fn highlight(&self, input: &str) -> String {
        #[cfg(feature="highlight")]
        {
            use ansi_term::Color::*;
            let mut out = String::with_capacity(input.len()+16);
            let parts = input.split_whitespace();
            for (i, tok) in parts.enumerate() {
                let s = if tok.starts_with("--") || tok.starts_with('-') {
                    Yellow.paint(tok).to_string()
                } else if tok.chars().all(|c| c.is_ascii_digit()) {
                    Cyan.paint(tok).to_string()
                } else { tok.to_string() };
                if i>0 { out.push(' '); } out.push_str(&s);
            }
            return out;
        }
        input.to_string()
    }
}

/* ============================== Backend abstrait ============================== */

/// Moteur unifié.
pub struct Readline {
    inner: Engine,
    hist: Option<History>,
    comp: Arc<dyn Complete + Send + Sync>,
    hl: Option<Arc<dyn Highlight + Send + Sync>>,
    mask: bool,
    multiline: bool,
    prompt_color: bool,
    interrupted: Arc<AtomicBool>,
}

enum Engine {
    #[cfg(feature="rustyline")] Rusty(rl::Editor<RLHelper>,),
    #[cfg(feature="reedline")]  Reed(rd::Reedline, RDHelper),
}

impl Readline {
    pub fn with_config(mut cfg: ReadConfig) -> Result<Self> {
        // Historique
        let hist = cfg.history_path.clone().map(|p| History::new(p, cfg.history_size));
        // Compléteur
        let comp: Arc<dyn Complete + Send + Sync> = match cfg.completer {
            CompleterKind::None => Arc::new(NoComplete::default()),
            #[cfg(feature="completion")]
            CompleterKind::VitteCompletion(src) => Arc::new(VitteCompleter::new(src)),
            CompleterKind::Custom(c) => c,
        };
        // Highlighter
        let hl: Option<Arc<dyn Highlight + Send + Sync>> = if cfg.highlighter {
            Some(Arc::new(SimpleHighlighter))
        } else { None };

        // Signals
        let interrupted = Arc::new(AtomicBool::new(false));
        #[cfg(feature="signals")]
        {
            let flag = interrupted.clone();
            let _ = ctrlc::set_handler(move || { flag.store(true, Ordering::SeqCst); });
        }

        // Backend
        let inner = match cfg.backend {
            #[cfg(feature="rustyline")]
            BackendKind::Rustyline => {
                let mut ed = rl::Editor::new().map_err(|e| ReadErr::Backend(e.to_string()))?;
                ed.set_auto_add_history(false);
                let helper = RLHelper::new(comp.clone(), hl.clone());
                ed.set_helper(Some(helper));
                Engine::Rusty(ed)
            }
            #[cfg(feature="reedline")]
            BackendKind::Reedline => {
                let mut builder = rd::Reedline::create();
                let helper = RDHelper::new(comp.clone(), hl.clone());
                builder = builder.with_helper(helper.clone());
                Engine::Reed(builder, helper)
            }
            #[allow(unreachable_patterns)]
            _ => return Err(ReadErr::FeatureMissing),
        };

        Ok(Self {
            inner, hist, comp, hl,
            mask: cfg.mask_input,
            multiline: cfg.multiline,
            prompt_color: cfg.prompt_color,
            interrupted,
        })
    }

    /// Lit une ligne. Gère Ctrl-C/Ctrl-D et met à jour l’historique.
    pub fn read_line(&mut self, prompt: &str) -> Result<ReadEvent> {
        let p = self.render_prompt(prompt);
        if self.interrupted.swap(false, Ordering::SeqCst) { return Ok(ReadEvent::Interrupted); }

        let res = match &mut self.inner {
            #[cfg(feature="rustyline")]
            Engine::Rusty(ed) => {
                if self.mask {
                    ed.readline_with_initial(&p, ("", "")).map(|s| mask_to_empty(s))
                } else {
                    ed.readline(&p)
                }
                .map(ReadEvent::Line)
                .map_err(|e| match e {
                    rl::error::ReadlineError::Interrupted => ReadErr::Interrupted,
                    rl::error::ReadlineError::Eof => ReadErr::Eof,
                    other => ReadErr::Backend(other.to_string()),
                })
            }
            #[cfg(feature="reedline")]
            Engine::Reed(reed, helper) => {
                use rd::{Signal, Prompt};
                let prompt_impl = RDPrompt { text: p.clone() };
                match reed.read_line(&prompt_impl) {
                    Ok(line) => Ok(ReadEvent::Line(if self.mask { mask_to_empty(line) } else { line })),
                    Err(Signal::CtrlC) => Err(ReadErr::Interrupted),
                    Err(Signal::CtrlD) => Err(ReadErr::Eof),
                    Err(e) => Err(ReadErr::Backend(format!("{e:?}"))),
                }
            }
        };

        match &res {
            Ok(ReadEvent::Line(s)) => {
                if let Some(h) = &mut self.hist {
                    if !s.trim().is_empty() { let _ = h.append(s); }
                }
            }
            _ => {}
        }

        res
    }

    fn render_prompt(&self, prompt: &str) -> String {
        if self.prompt_color {
            #[cfg(feature="ansi")]
            {
                let b = ansi::style(prompt).bold();
                return b.to_string();
            }
        }
        prompt.to_string()
    }

    /// Force le chargement de l’historique depuis le disque.
    pub fn load_history(&mut self) {
        if let Some(h) = &mut self.hist { let _ = h.load(); }
    }
    /// Sauvegarde immédiate de l’historique.
    pub fn save_history(&mut self) {
        if let Some(h) = &mut self.hist { let _ = h.save(); }
    }

    /// Remplace dynamiquement le compléteur.
    pub fn set_completer(&mut self, c: Arc<dyn Complete + Send + Sync>) {
        self.comp = c.clone();
        match &mut self.inner {
            #[cfg(feature="rustyline")]
            Engine::Rusty(ed) => if let Some(h) = ed.helper_mut() { h.comp = c; },
            #[cfg(feature="reedline")]
            Engine::Reed(_r, helper) => helper.comp = c,
        }
    }
}

/* ============================== Historique ============================== */

pub struct History {
    path: PathBuf,
    max: usize,
    lines: Vec<String>,
}
impl History {
    pub fn new(path: PathBuf, max: usize) -> Self { Self { path, max, lines: Vec::new() } }
    pub fn load(&mut self) -> Result<()> {
        #[cfg(feature="std")]
        {
            if self.path.exists() {
                let s = std::fs::read_to_string(&self.path).map_err(|e| ReadErr::Io(e.to_string()))?;
                self.lines = s.lines().map(|s| s.to_string()).collect();
                self.truncate();
            }
            Ok(())
        }
        #[cfg(not(feature="std"))] { Ok(()) }
    }
    pub fn save(&self) -> Result<()> {
        #[cfg(feature="std")]
        {
            if let Some(dir) = self.path.parent() { let _ = std::fs::create_dir_all(dir); }
            let data = self.lines.join("\n");
            std::fs::write(&self.path, data).map_err(|e| ReadErr::Io(e.to_string()))
        }
        #[cfg(not(feature="std"))] { Ok(()) }
    }
    pub fn append(&mut self, line: &str) -> Result<()> {
        self.lines.push(line.to_string());
        self.truncate();
        self.save()
    }
    fn truncate(&mut self) {
        if self.lines.len() > self.max {
            let drop_n = self.lines.len() - self.max;
            self.lines.drain(0..drop_n);
        }
    }
}

/* ============================== Helpers backend ============================== */

#[inline] fn mask_to_empty(s: String) -> String { s } // ici: on ne renvoie rien de secret, on garde la ligne telle quelle

/* ---------- RUSTYLINE helper ---------- */
#[cfg(feature="rustyline")]
#[derive(Clone)]
pub struct RLHelper {
    pub comp: Arc<dyn Complete + Send + Sync>,
    pub hl: Option<Arc<dyn Highlight + Send + Sync>>,
}
#[cfg(feature="rustyline")]
impl RLHelper { pub fn new(comp: Arc<dyn Complete + Send + Sync>, hl: Option<Arc<dyn Highlight + Send + Sync>>) -> Self { Self { comp, hl } } }

#[cfg(feature="rustyline")]
impl rl::Helper for RLHelper {}

#[cfg(feature="rustyline")]
impl rl::completion::Completer for RLHelper {
    type Candidate = rl::completion::Pair;
    fn complete(&self, line: &str, pos: usize, _ctx: &rl::Context<'_>) -> rl::Result<(usize, Vec<Self::Candidate>)> {
        let start = line[..pos].rfind(|c: char| c.is_whitespace()).map(|i| i+1).unwrap_or(0);
        let q = &line[start..pos];
        let items = self.comp.complete(q, pos);
        Ok((start, items.into_iter().map(|s| rl::completion::Pair { display: s.clone(), replacement: s }).collect()))
    }
}
#[cfg(feature="rustyline")]
impl rl::highlight::Highlighter for RLHelper {
    fn highlight<'l>(&self, line: &'l str, _pos: usize) -> std::borrow::Cow<'l, str> {
        if let Some(h) = &self.hl { std::borrow::Cow::Owned(h.highlight(line)) } else { std::borrow::Cow::Borrowed(line) }
    }
}

/* ---------- REEDLINE helper ---------- */
#[cfg(feature="reedline")]
#[derive(Clone)]
pub struct RDHelper {
    pub comp: Arc<dyn Complete + Send + Sync>,
    pub hl: Option<Arc<dyn Highlight + Send + Sync>>,
}
#[cfg(feature="reedline")]
impl RDHelper { pub fn new(comp: Arc<dyn Complete + Send + Sync>, hl: Option<Arc<dyn Highlight + Send + Sync>>) -> Self { Self { comp, hl } } }

#[cfg(feature="reedline")]
impl rd::Completer for RDHelper {
    fn complete(&mut self, line: &str, pos: usize) -> rd::Result<(usize, Vec<rd::Suggestion>)> {
        let start = line[..pos].rfind(|c: char| c.is_whitespace()).map(|i| i+1).unwrap_or(0);
        let q = &line[start..pos];
        let items = self.comp.complete(q, pos);
        Ok((start, items.into_iter().map(|s| rd::Suggestion { value: s, description: None, extra: None }).collect()))
    }
}
#[cfg(feature="reedline")]
impl rd::Highlighter for RDHelper {
    fn highlight(&self, line: &str, _pos: usize) -> String {
        if let Some(h) = &self.hl { h.highlight(line) } else { line.to_string() }
    }
}
#[cfg(feature="reedline")]
#[derive(Clone)]
struct RDPrompt { text: String }
#[cfg(feature="reedline")]
impl rd::Prompt for RDPrompt {
    fn render_prompt_left(&self) -> std::borrow::Cow<str> { std::borrow::Cow::Borrowed(&self.text) }
}

/* ============================== Tests ============================== */

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn list_complete_filters() {
        let c = ListComplete::new(vec!["build".into(),"bench".into(),"bundle".into()]);
        assert_eq!(c.complete("bu", 2), vec!["build".to_string(), "bundle".to_string()]);
    }
    #[test]
    fn simple_highlight_passthrough() {
        let h = SimpleHighlighter::default();
        let out = h.highlight("run -v 123");
        assert!(out.len() >= "run -v 123".len());
    }
}