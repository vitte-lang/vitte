#![cfg(feature = "repl-cli")]

// crates/vitte-tools/src/bin/vitte-repl.rs
//! REPL Vitte — interactif, multi-ligne, avec compilation à la volée.
//!
//! Exemples :
//!   vitte-repl
//!   vitte-repl --engine eval --prelude std/prelude.vit --load examples/hello/src/main.vit
//!
//! Commandes méta (en ligne de commande dans le REPL) :
//!   :help                 — aide
//!   :quit / :q            — quitte
//!   :clear                — efface le buffer REPL (état source)
//!   :reset                — reset total (buffer + dernier chunk)
//!   :load <file.vit>      — charge et exécute un fichier
//!   :save <file.vit>      — sauvegarde le buffer courant
//!   :engine [eval|vm]     — change le moteur d’exécution (si features disponibles)
//!   :time [on|off]        — active/désactive le chrono par commande
//!   :disasm [compact]     — désassemble le dernier chunk
//!   :bytes                — affiche la taille binaire du dernier chunk
//!   :history              — affiche l’emplacement du fichier d’historique
//!   :set prompt <txt>     — change le prompt (ex. ":set prompt vitte> ")
//!
//! Entrées multi-lignes : le REPL continue tant que la ligne semble incomplète
//! (parenthèses non fermées / absence de ';').

use std::fs;
use std::io::{self, Write};
use std::path::PathBuf;
use std::time::Instant;

use anyhow::{anyhow, Context, Result};
use camino::Utf8PathBuf;
use clap::{Parser, ValueEnum};
use yansi::{Color, Paint};

use rustyline::history::DefaultHistory;
use rustyline::{Config as RLConfig, Editor};

use vitte_core::{
    bytecode::chunk::Chunk as VChunk,
    disasm::{disassemble_compact, disassemble_full},
};
use vitte_tools::{history_path as tools_history_path, setup_colors as tools_setup_colors, ColorMode as ToolsColorMode};

#[cfg(feature = "eval")]
use vitte_core::runtime::eval::{eval_chunk, EvalOptions};

#[cfg(feature = "vm")]
use vitte_vm::Vm;

#[derive(Clone, Debug, ValueEnum)]
enum Engine {
    #[cfg(feature = "eval")]
    Eval,
    #[cfg(feature = "vm")]
    Vm,
}

#[derive(Parser, Debug)]
#[command(name="vitte-repl", version, about="REPL du langage Vitte")]
struct Args {
    /// Moteur d’exécution (eval=interpréteur léger, vm=VM complète)
    #[arg(long, value_enum)]
    engine: Option<Engine>,

    /// Fichier à charger au démarrage (plusieurs autorisés)
    #[arg(long)]
    load: Vec<PathBuf>,

    /// Fichier "pré-lude" injecté avant tout (types, helpers…)
    #[arg(long)]
    prelude: Option<PathBuf>,

    /// Désactive la bannière d’accueil
    #[arg(long)]
    no_banner: bool,

    /// Invite personnalisée (ex: "vitte> ")
    #[arg(long)]
    prompt: Option<String>,

    /// Désactive les couleurs
    #[arg(long)]
    no_color: bool,
}

fn main() {
    if let Err(e) = real_main() {
        eprintln!("❌ {e}");
        std::process::exit(1);
    }
}

fn real_main() -> Result<()> {
    color_eyre::install().ok();
    let args = Args::parse();

    // Couleurs
    let color_mode = if args.no_color {
        ToolsColorMode::Never
    } else {
        ToolsColorMode::Auto
    };
    tools_setup_colors(color_mode);

    // Choix moteur par défaut
    let mut engine = choose_default_engine(args.engine)?;

    // Bannière
    if !args.no_banner {
        banner(&engine);
    }

    // Historique (dans ~/.vitte/history)
    let hist_path = tools_history_path()?;
    let rl_cfg = RLConfig::builder()
        .history_ignore_dups(true)
        .map_err(|e| anyhow!("configuration REPL: {e}"))?
        .build();
    let mut rl: Editor<(), DefaultHistory> = Editor::with_config(rl_cfg)?;
    let _ = rl.load_history(&hist_path);

    // Session
    let mut session = Session::new(engine.clone());
    if let Some(p) = args.prelude.as_ref() {
        session.exec_file(p)?;
    }
    for p in &args.load {
        session.exec_file(p)?;
    }

    if let Some(p) = args.prompt {
        session.prompt = p;
    }

    // Boucle REPL
    loop {
        let line = match rl.readline(&session.prompt) {
            Ok(l) => l,
            Err(rustyline::error::ReadlineError::Interrupted) => {
                eprintln!("{}", "(^C)".paint(Color::Yellow));
                continue;
            }
            Err(rustyline::error::ReadlineError::Eof) => {
                eprintln!("{}", "(^D) bye".paint(Color::Cyan));
                break;
            }
            Err(e) => return Err(anyhow!("readline: {e}")),
        };

        let trimmed = line.trim();
        if trimmed.is_empty() {
            continue;
        }

        if trimmed.starts_with(':') {
            if let Some(done) = handle_meta(&mut session, trimmed)? {
                if done == MetaResult::Quit { break; }
            }
            rl.add_history_entry(trimmed)?;
            continue;
        }

        // Multi-ligne : collect tant que la saisie semble incomplète
        let mut buf = line;
        while !looks_complete(&buf) {
            let more = rl.readline("... ")?;
            if more.trim().is_empty() { break; }
            buf.push('\n');
            buf.push_str(&more);
        }

        rl.add_history_entry(buf.as_str())?;
        session.exec_snippet(&buf)?;
    }

    // Sauvegarde historique
    if let Some(dir) = hist_path.parent() {
        let _ = fs::create_dir_all(dir);
    }
    let _ = rl.save_history(&hist_path);

    Ok(())
}

/* -------------------------- Session & exécution -------------------------- */

struct Session {
    engine: Engine,
    buffer: String,                 // accumulation (pour “état” REPL)
    last_chunk: Option<VChunk>,
    timing: bool,
    prompt: String,
}

impl Session {
    fn new(engine: Engine) -> Self {
        Self {
            engine,
            buffer: String::new(),
            last_chunk: None,
            timing: true,
            prompt: "vitte> ".into(),
        }
    }

    fn exec_file(&mut self, path: &PathBuf) -> Result<()> {
        let p = Utf8PathBuf::from_path_buf(path.clone()).map_err(|_| anyhow!("Chemin non UTF-8"))?;
        let src = fs::read_to_string(&p).with_context(|| format!("lecture {p}"))?;
        eprintln!("▶️  charge {p}");
        self.exec_snippet(&src)
    }

    fn exec_snippet(&mut self, src: &str) -> Result<()> {
        // On accumule “l’état” (MVP : pas de vraie portée/variables persistantes)
        if !self.buffer.is_empty() {
            self.buffer.push('\n');
        }
        self.buffer.push_str(src);

        // Compile le *buffer entier* pour simuler un état global
        let t0 = Instant::now();
        let chunk = compile_repl_buffer(&self.buffer)?;
        let t1 = Instant::now();

        if self.timing {
            eprintln!("⏱️  compile: {}", human_millis(t1 - t0));
        }

        // Exécuter
        let t2 = Instant::now();
        let run_res = self.run_chunk(&chunk);
        let t3 = Instant::now();

        if self.timing {
            eprintln!("⏱️  run:     {}", human_millis(t3 - t2));
        }

        // Mémoriser le dernier chunk
        self.last_chunk = Some(chunk);

        run_res
    }

    fn run_chunk(&mut self, chunk: &vitte_core::bytecode::chunk::Chunk) -> Result<()> {
        match self.engine {
            #[cfg(feature = "eval")]
            Engine::Eval => {
                let out = eval_chunk(chunk, EvalOptions::default())?;
                if !out.stdout.is_empty() {
                    // on affiche déjà capturé (le Print de l’évaluateur)
                    print!("{0}", out.stdout);
                    io::stdout().flush().ok();
                }
                Ok(())
            }
            #[cfg(feature = "vm")]
            Engine::Vm => {
                let mut vm = Vm::new();
                vm.run(chunk).context("exécution VM")?;
                Ok(())
            }
        }
    }
}

/* ------------------------------ Meta commandes ------------------------------ */

#[derive(PartialEq, Eq)]
enum MetaResult { Continue, Quit }

fn handle_meta(sess: &mut Session, cmd: &str) -> Result<Option<MetaResult>> {
    let parts: Vec<&str> = cmd.split_whitespace().collect();
    if parts.is_empty() { return Ok(None); }
    let head = parts[0];

    match head {
        ":q" | ":quit" => return Ok(Some(MetaResult::Quit)),

        ":help" => {
            println!("{}", HELP_TEXT);
        }

        ":clear" => {
            String::clear(&mut sess.buffer);
            println!("(buffer vidé)");
        }

        ":reset" => {
            String::clear(&mut sess.buffer);
            sess.last_chunk = None;
            println!("(session réinitialisée)");
        }

        ":load" => {
            if parts.len() < 2 { return Err(anyhow!("usage: :load <fichier.vit>")); }
            let path = PathBuf::from(parts[1]);
            sess.exec_file(&path)?;
        }

        ":save" => {
            if parts.len() < 2 { return Err(anyhow!("usage: :save <fichier.vit>")); }
            let path = PathBuf::from(parts[1]);
            fs::write(&path, sess.buffer.as_bytes())
                .with_context(|| format!("écriture {}", Utf8PathBuf::from_path_buf(path.clone()).unwrap_or_else(|_| Utf8PathBuf::from("<out>"))))?;
            println!("(buffer sauvegardé)");
        }

        ":disasm" => {
            let compact = parts.get(1).map(|s| *s == "compact").unwrap_or(false);
            if let Some(ch) = &sess.last_chunk {
                let title = "<repl>";
                let s = if compact { disassemble_compact(ch) } else { disassemble_full(ch, title) };
                println!("{s}");
            } else {
                println!("(pas de chunk encore – exécutez un snippet)");
            }
        }

        ":bytes" => {
            if let Some(mut ch) = sess.last_chunk.clone() {
                let b = ch.to_bytes();
                println!("taille: {} octets", b.len());
            } else {
                println!("(pas de chunk)");
            }
        }

        ":engine" => {
            if parts.len() == 1 {
                println!("engine = {}", engine_label(&sess.engine));
            } else {
                let val = parts[1];
                let new = match val {
                    #[cfg(feature = "eval")]
                    "eval" => Some(Engine::Eval),
                    #[cfg(feature = "vm")]
                    "vm"   => Some(Engine::Vm),
                    _ => None,
                };
                if let Some(e) = new {
                    sess.engine = e;
                    println!("engine -> {}", engine_label(&sess.engine));
                } else {
                    println!("engine inconnu ou indisponible : {val}");
                }
            }
        }

        ":time" => {
            if parts.len() == 1 {
                println!("time = {}", if sess.timing { "on" } else { "off" });
            } else {
                match parts[1] {
                    "on"  => { sess.timing = true;  println!("time -> on"); }
                    "off" => { sess.timing = false; println!("time -> off"); }
                    _ => println!("usage: :time [on|off]"),
                }
            }
        }

        ":history" => {
            let p = tools_history_path()?;
            println!("history: {}", p.display());
        }

        ":set" => {
            if parts.get(1) == Some(&"prompt") {
                let rest = cmd.strip_prefix(":set prompt").unwrap().trim_start();
                if rest.is_empty() {
                    println!("usage: :set prompt <texte>");
                } else {
                    sess.prompt = rest.to_string();
                    println!("prompt -> {:?}", sess.prompt);
                }
            } else {
                println!("usage: :set prompt <texte>");
            }
        }

        _ => {
            println!("commande inconnue : {head}. Tapez :help");
        }
    }

    Ok(Some(MetaResult::Continue))
}

const HELP_TEXT: &str = r#"Commandes REPL:
  :help                 — aide
  :quit / :q            — quitte
  :clear                — efface le buffer REPL courant
  :reset                — reset total (buffer + dernier chunk)
  :load <file.vit>      — charge et exécute un fichier
  :save <file.vit>      — sauvegarde le buffer courant
  :engine [eval|vm]     — change le moteur d'exécution
  :time [on|off]        — chrono de compile/run
  :disasm [compact]     — désassemblage du dernier chunk
  :bytes                — taille binaire du dernier chunk
  :history              — chemin du fichier d'historique
  :set prompt <txt>     — change le prompt
"#;

/* ------------------------------- Utilitaires ------------------------------- */

fn banner(engine: &Engine) {
    let title = format!("Vitte REPL — moteur: {}", engine_label(engine));
    let bar = "─".repeat(title.len());
    eprintln!("{}", bar.paint(Color::Cyan));
    eprintln!("{}", title.paint(Color::Cyan).bold());
    eprintln!("{}", bar.paint(Color::Cyan));
    eprintln!("Tapez :help pour la liste des commandes.\n");
}

#[cfg(all(feature = "eval", feature = "vm"))]
fn engine_label(e: &Engine) -> &'static str {
    match e {
        Engine::Eval => "eval",
        Engine::Vm => "vm",
    }
}

#[cfg(all(feature = "eval", not(feature = "vm")))]
fn engine_label(_e: &Engine) -> &'static str { "eval" }

#[cfg(all(feature = "vm", not(feature = "eval")))]
fn engine_label(_e: &Engine) -> &'static str { "vm" }

#[cfg(all(not(feature = "eval"), not(feature = "vm")))]
fn engine_label(_e: &Engine) -> &'static str { "n/a" }

fn choose_default_engine(cli: Option<Engine>) -> Result<Engine> {
    if let Some(e) = cli { return Ok(e); }
    #[cfg(feature = "eval")]
    { return Ok(Engine::Eval); }
    #[cfg(all(not(feature="eval"), feature="vm"))]
    { return Ok(Engine::Vm); }
    #[allow(unreachable_code)]
    Err(anyhow!("Aucun moteur disponible : compile avec la feature `eval` ou `vm`."))
}

fn compile_repl_buffer(_src: &str) -> Result<VChunk> {
    Err(anyhow!("Compilation REPL indisponible dans cette build"))
}

/// Heuristique simple : une entrée est "complète" si
/// - le nombre de '(' et ')' est équilibré
/// - et si la ligne se termine par ';' (ou '}' — au cas où).
fn looks_complete(s: &str) -> bool {
    let mut depth = 0i32;
    let mut in_str = false;
    let mut esc = false;
    for ch in s.chars() {
        if in_str {
            if esc { esc = false; continue; }
            match ch {
                '\\' => esc = true,
                '"'  => in_str = false,
                _ => {}
            }
            continue;
        }
        match ch {
            '"' => in_str = true,
            '(' => depth += 1,
            ')' => depth -= 1,
            _ => {}
        }
    }
    if in_str || depth != 0 { return false; }
    let t = s.trim_end();
    t.ends_with(';') || t.ends_with('}')
}

fn human_millis(d: std::time::Duration) -> String {
    let ms = d.as_millis();
    if ms < 1_000 { return format!("{ms} ms"); }
    let s = ms as f64 / 1000.0;
    if s < 60.0 { return format!("{s:.3} s"); }
    let m = (s / 60.0).floor();
    let rest = s - m * 60.0;
    format!("{m:.0} min {rest:.1} s")
}
