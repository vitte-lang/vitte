//! vitte-cli — bibliothèque interne du binaire `vitte`
//!
//! But : fournir une API **propre, testable et réutilisable** pour le CLI
//! sans mélanger la logique d'E/S et le parsing d'arguments (laisse ça à `main.rs`).
//!
//! Points clés :
//! - Pipeline générique **compile → (optionnel) compresse → (optionnel) exécute**
//! - Callbacks/hook pour brancher ton compilateur, ton désassembleur, ton formateur, ta VM
//! - Utilitaires d'E/S (stdin/stdout, écriture atomique, création dossiers, chrono)
//! - Traces (`feature = "trace"`) et couleurs (`feature = "color"`) optionnelles
//! - Zéro dépendance forte sur l'impl interne des crates Vitte (pas d'API fantôme)

#![deny(unused_must_use)]
#![forbid(unsafe_code)]

use similar::{ChangeTag, TextDiff};
use std::{
    fs,
    fs::File,
    io::{self, BufReader, BufWriter, Read, Write},
    path::{Path, PathBuf},
    time::Instant,
};

use anyhow::{anyhow, Context, Result};
use vitte_core::SourceId;
use vitte_lexer::{Lexer, LineMap, TokenKind};
use vitte_parser::Parser;

#[cfg(feature = "trace")]
use env_logger;

#[cfg(feature = "color")]
use owo_colors::OwoColorize;

// ───────────────────────────── Types publics ─────────────────────────────

/// Représente une commande haut-niveau (sans parsing CLI — réservé à main.rs).
#[derive(Clone, Debug)]
pub enum Command {
    /// Compile un fichier source vers bytecode.
    Compile(CompileTask),
    /// Lance un bytecode via la VM (si disponible) — accepte aussi le chemin d'une source si `auto_compile = true`.
    Run(RunTask),
    /// Lancer un REPL (si fourni par les hooks).
    Repl(ReplTask),
    /// Formater un fichier source (ou stdin → stdout).
    Fmt(FmtTask),
    /// Inspecter un artefact (headers, tailles, sections, etc.).
    Inspect(InspectTask),
    /// Désassembler un bytecode.
    Disasm(DisasmTask),
    /// Lister les modules compilés (selon les features du méta-crate).
    Modules(ModulesTask),
}

#[derive(Clone, Debug, Default)]
pub struct CompileTask {
    pub input: Input,     // chemin ou stdin
    pub output: Output,   // chemin, stdout ou auto (même dossier, .vitbc)
    pub optimize: bool,   // -O
    pub emit_debug: bool, // info debug
    pub auto_mkdir: bool, // crée les dossiers parents si besoin
    pub overwrite: bool,  // autorise l'écrasement
    pub time: bool,       // afficher le timing
}

#[derive(Clone, Debug, Default)]
pub struct RunTask {
    pub program: InputKind, // bytecode (fichier ou bytes), ou Source si auto_compile
    pub args: Vec<String>,
    pub auto_compile: bool, // si program = Source(path/stdin), compiler d'abord
    pub optimize: bool,
    pub time: bool,
}

#[derive(Clone, Debug, Default)]
pub struct ReplTask {
    pub prompt: String,
}

#[derive(Clone, Debug, Default)]
pub struct FmtTask {
    pub input: Input,   // chemin source ou stdin
    pub output: Output, // si Output::InPlace => réécrit le fichier
    pub check: bool,    // mode --check (retourne erreur si diff)
    pub diff: bool,     // affiche un diff (--diff)
}

#[derive(Clone, Debug, Default)]
pub struct InspectTask {
    pub input: InputKind,
    pub options: inspect::InspectOptions,
}

#[derive(Clone, Debug, Default)]
pub struct DisasmTask {
    pub input: InputKind,
    pub output: Output, // fichier ou stdout
}

#[derive(Clone, Debug)]
pub enum ModulesFormat {
    Table,
    Json,
}

impl Default for ModulesFormat {
    fn default() -> Self {
        ModulesFormat::Table
    }
}

#[derive(Clone, Debug, Default)]
pub struct ModulesTask {
    pub format: ModulesFormat,
}

/// Entrée texte (source) : fichier ou `-` (=stdin).
#[derive(Clone, Debug)]
pub enum Input {
    Path(PathBuf),
    Stdin,
}
impl Default for Input {
    fn default() -> Self {
        Self::Stdin
    }
}

/// Entrée binaire/typée :
#[derive(Clone, Debug)]
pub enum InputKind {
    BytecodePath(PathBuf),
    BytecodeBytes(Vec<u8>),
    SourcePath(PathBuf),
    SourceStdin,
}
impl Default for InputKind {
    fn default() -> Self {
        Self::SourceStdin
    }
}

/// Sortie générique.
#[derive(Clone, Debug)]
pub enum Output {
    Path(PathBuf),
    Stdout,
    InPlace, // pour fmt
    Auto,    // pour compile : même nom + .vitbc
}
impl Default for Output {
    fn default() -> Self {
        Self::Stdout
    }
}

/// Hooks pour brancher tes implémentations.
#[derive(Clone)]
pub struct Hooks {
    pub compile: Option<CompileFn>,
    pub run_bc: Option<RunFn>,
    pub repl: Option<ReplFn>,
    pub fmt: Option<FormatFn>,
    pub disasm: Option<DisasmFn>,
    pub inspect: Option<InspectFn>,
}
impl Default for Hooks {
    fn default() -> Self {
        Self { compile: None, run_bc: None, repl: None, fmt: None, disasm: None, inspect: None }
    }
}

/// Signature d'un compilateur : source → bytecode.
pub type CompileFn = fn(source: &str, opts: &CompileOptions) -> Result<Vec<u8>>;
/// VM : bytecode → exit code.
pub type RunFn = fn(bytecode: &[u8], opts: &RunOptions) -> Result<i32>;
/// REPL : boucle, retourne exit code.
pub type ReplFn = fn(prompt: &str) -> Result<i32>;
/// Formatage : source → code formaté.
pub type FormatFn = fn(source: &str, check_only: bool) -> Result<String>;
/// Désassemblage : bytecode → texte.
pub type DisasmFn = fn(bytecode: &[u8]) -> Result<String>;
/// Inspection : bytecode → texte (ou JSON), au choix de ton impl.
pub type InspectFn = fn(bytes: &[u8], options: &inspect::InspectOptions) -> Result<String>;

#[derive(Clone, Debug, Default)]
pub struct CompileOptions {
    pub optimize: bool,
    pub emit_debug: bool,
}

#[derive(Clone, Debug, Default)]
pub struct RunOptions {
    pub args: Vec<String>,
    pub optimize: bool,
}

// ───────────────────────────── Initialisation ─────────────────────────────

/// Initialise le logger selon la feature `trace`.
pub fn init_logger() {
    #[cfg(feature = "trace")]
    {
        let _ = env_logger::Builder::from_env(env_logger::Env::default().default_filter_or("info"))
            .format_timestamp_secs()
            .try_init();
    }
}

// ───────────────────────────── Exécution ─────────────────────────────

/// Exécute une commande avec les hooks fournis. Retourne un code de sortie.
pub fn execute(cmd: Command, hooks: &Hooks) -> Result<i32> {
    match cmd {
        Command::Compile(t) => {
            let code = compile_entry(t, hooks)?;
            Ok(code)
        }
        Command::Run(t) => run_entry(t, hooks),
        Command::Repl(t) => {
            let f = hooks.repl.ok_or_else(|| anyhow!("REPL non disponible (hook manquant)"))?;
            let exit = f(&t.prompt)?;
            Ok(exit)
        }
        Command::Fmt(t) => {
            fmt_entry(t, hooks)?;
            Ok(0)
        }
        Command::Inspect(t) => {
            inspect_entry(t, hooks)?;
            Ok(0)
        }
        Command::Disasm(t) => {
            disasm_entry(t, hooks)?;
            Ok(0)
        }
        Command::Modules(t) => {
            modules_entry(t)?;
            Ok(0)
        }
    }
}

fn compile_entry(task: CompileTask, hooks: &Hooks) -> Result<i32> {
    let CompileTask { input, output, optimize, emit_debug, auto_mkdir, overwrite, time } = task;
    let compiler = match hooks.compile {
        Some(c) => c,
        None => {
            return bail_diagnostic(
                Diagnostic::new(
                    "HOOK001",
                    "Compilation indisponible : hook `compile` absent",
                )
                .with_note("Activez la feature `vm` (`cargo run -p vitte-cli --features vm`) ou fournissez votre propre compilateur via Hooks::compile.")
                .with_help("Recompilez vitte-cli avec la feature `engine` ou fournissez un compilateur dans Hooks::compile."),
            )
        },
    };

    let src = read_source(&input).context("lecture de la source")?;
    perform_frontend_checks(&src, &input)?;

    let start = Instant::now();
    let compile_opts = CompileOptions { optimize, emit_debug };
    let bc = match compiler(&src, &compile_opts) {
        Ok(bytes) => bytes,
        Err(err) => {
            return bail_diagnostic(
                Diagnostic::new("COMPILE100", "Le hook de compilation a retourné une erreur.")
                    .with_note(format!("détail: {err}"))
                    .with_help("Activez les logs (RUST_LOG=debug) ou recompilez avec `--features vm` pour utiliser le compilateur de référence."),
            );
        }
    };
    let elapsed = start.elapsed();

    let out_path = match (&output, &input) {
        (Output::Auto, Input::Path(p)) => default_bytecode_path(p),
        (Output::Auto, Input::Stdin) => PathBuf::from("out.vitbc"),
        (Output::Path(p), _) => p.clone(),
        (Output::Stdout, _) => PathBuf::new(), // indicateur stdout
        (Output::InPlace, _) => anyhow::bail!("Output::InPlace n'a pas de sens pour compile"),
    };

    if let Output::Stdout = output {
        let mut w = BufWriter::new(io::stdout().lock());
        w.write_all(&bc)?;
        w.flush()?;
    } else {
        if auto_mkdir {
            if let Some(parent) = out_path.parent() {
                fs::create_dir_all(parent)?;
            }
        }
        if out_path.exists() && !overwrite {
            return Err(anyhow!("fichier de sortie existe déjà: {}", display(&out_path)));
        }
        write_bytes_atomic(&out_path, &bc)
            .with_context(|| format!("écriture de {}", display(&out_path)))?;
        status_ok("COMPILE", &format!("{} ({} octets)", display(&out_path), bc.len()));
    }

    if time {
        status_info("TIME", &format!("compile: {} ms", elapsed.as_millis()));
    }
    Ok(0)
}

fn run_entry(task: RunTask, hooks: &Hooks) -> Result<i32> {
    let runner = match hooks.run_bc {
        Some(r) => r,
        None => {
            return bail_diagnostic(
                Diagnostic::new("HOOK002", "Exécution indisponible : hook `run_bc` absent")
                    .with_note("Activez la feature `vm` (`cargo run -p vitte-cli --features vm`) ou fournissez une implémentation de VM via Hooks::run_bc.")
                    .with_help("Activez la feature `vm` ou fournissez une VM via Hooks::run_bc."),
            );
        }
    };

    let bytes = match task.program {
        InputKind::BytecodePath(p) => {
            fs::read(&p).with_context(|| format!("lecture bytecode: {}", display(&p)))?
        }
        InputKind::BytecodeBytes(b) => b,
        InputKind::SourcePath(p) if task.auto_compile => {
            let src = read_source(&Input::Path(p.clone()))?;
            perform_frontend_checks(&src, &Input::Path(p.clone()))?;
            let compiler = match hooks.compile {
                Some(c) => c,
                None => {
                    return bail_diagnostic(
                        Diagnostic::new(
                            "HOOK001",
                            "auto-compile demandé mais hook `compile` absent",
                        )
                        .with_note("Activez la feature `vm` (`cargo run -p vitte-cli --features vm`) ou fournissez `Hooks::compile`.")
                        .with_help("Activez la feature `engine` ou fournissez un compilateur."),
                    );
                }
            };
            let compile_opts = CompileOptions { optimize: task.optimize, emit_debug: false };
            match compiler(&src, &compile_opts) {
                Ok(bytes) => bytes,
                Err(err) => {
                    return bail_diagnostic(
                        Diagnostic::new("COMPILE101", "La compilation automatique a échoué.")
                            .with_note(format!("détail: {err}"))
                            .with_help("Passez un bytecode `.vitbc` déjà compilé ou activez les logs (RUST_LOG=debug)."),
                    );
                }
            }
        }
        InputKind::SourceStdin if task.auto_compile => {
            let src = read_source(&Input::Stdin)?;
            perform_frontend_checks(&src, &Input::Stdin)?;
            let compiler = match hooks.compile {
                Some(c) => c,
                None => {
                    return bail_diagnostic(
                        Diagnostic::new(
                            "HOOK001",
                            "auto-compile demandé mais hook `compile` absent",
                        )
                        .with_note("Activez la feature `vm` (`cargo run -p vitte-cli --features vm`) ou fournissez `Hooks::compile`.")
                        .with_help("Activez la feature `engine` ou fournissez un compilateur."),
                    );
                }
            };
            let compile_opts = CompileOptions { optimize: task.optimize, emit_debug: false };
            match compiler(&src, &compile_opts) {
                Ok(bytes) => bytes,
                Err(err) => {
                    return bail_diagnostic(
                        Diagnostic::new("COMPILE101", "La compilation automatique a échoué.")
                            .with_note(format!("détail: {err}"))
                            .with_help("Passez un bytecode `.vitbc` déjà compilé ou activez les logs (RUST_LOG=debug)."),
                    );
                }
            }
        }
        InputKind::SourcePath(p) => {
            return bail_diagnostic(
                Diagnostic::new(
                    "RUN100",
                    format!(
                        "'run' attend un bytecode (ou activez --auto-compile) : {}",
                        display(&p)
                    ),
                )
                .with_note("Astuce : utilisez `vitte run --auto-compile <source>` ou fournissez un bytecode `.vitbc`.")
                .with_help("Fournissez un .vitbc ou ajoutez --auto-compile."),
            );
        }
        InputKind::SourceStdin => {
            return bail_diagnostic(
                Diagnostic::new(
                    "RUN101",
                    "'run' attend un bytecode (ou --auto-compile) depuis stdin",
                )
                .with_note(
                    "Redirigez un bytecode (`cat programme.vitbc | vitte run -`) ou utilisez --auto-compile pour compiler une source.",
                ),
            );
        }
    };

    let start = Instant::now();
    let code = match runner(&bytes, &RunOptions { args: task.args, optimize: task.optimize }) {
        Ok(code) => code,
        Err(err) => {
            return bail_diagnostic(
                Diagnostic::new("RUN200", "La machine virtuelle a signalé une erreur.")
                    .with_note(format!("détail: {err}"))
                    .with_help(
                        "Vérifiez votre bytecode ou activez les journaux (`RUST_LOG=debug`).",
                    ),
            );
        }
    };
    let elapsed = start.elapsed();

    if task.time {
        status_info("TIME", &format!("run: {} ms", elapsed.as_millis()));
    }
    if code == 0 {
        status_ok("RUN", "exit=0");
    } else {
        status_warn("RUN", &format!("exit={code}"));
    }
    Ok(code)
}

fn perform_frontend_checks(src: &str, input: &Input) -> Result<()> {
    let label = input_label(input);
    let source = SourceId(0);

    let mut lexer = Lexer::new(src, source);
    let lines = lexer.lines.clone();
    loop {
        match lexer.next() {
            Ok(Some(tok)) => {
                if matches!(tok.value, TokenKind::Eof) {
                    break;
                }
            }
            Ok(None) => break,
            Err(err) => {
                let ((l1, c1), (l2, c2)) = lines.span_lines(err.span);
                return bail_diagnostic(
                    Diagnostic::new("LEX100", format!("Erreur lexicale: {err}"))
                        .with_note(format!("{label}:L{}C{}-L{}C{}", l1, c1, l2, c2))
                        .with_help(
                            "Corrigez le lexème identifié avant de relancer la compilation.",
                        ),
                );
            }
        }
    }

    if is_simple_module_declaration(src) {
        return Ok(());
    }

    let mut parser = Parser::new(src, source);
    if let Err(err) = parser.parse_program() {
        let span = err.span;
        let msg = err.message;
        let lines = LineMap::new(src);
        let ((l1, c1), (l2, c2)) = lines.span_lines(span);
        return bail_diagnostic(
            Diagnostic::new("PARSE100", format!("Erreur de syntaxe: {msg}"))
                .with_note(format!("{label}:L{}C{}-L{}C{}", l1, c1, l2, c2))
                .with_help("Corrigez la structure du code avant compilation."),
        );
    }

    Ok(())
}

fn is_simple_module_declaration(src: &str) -> bool {
    let trimmed = src.trim();
    if !trimmed.starts_with("module") {
        return false;
    }
    let mut rest = trimmed.trim_start_matches("module").trim_start();
    if rest.is_empty() {
        return false;
    }
    let ident_len =
        rest.chars().take_while(|c| c.is_ascii_alphanumeric() || *c == '_' || *c == '-').count();
    if ident_len == 0 {
        return false;
    }
    rest = rest[ident_len..].trim_start();
    rest == ";" || rest.starts_with('{')
}

fn input_label(input: &Input) -> String {
    match input {
        Input::Path(p) => display(p),
        Input::Stdin => "stdin".to_string(),
    }
}

fn print_text_diff(title: &str, before: &str, after: &str) {
    let diff = TextDiff::from_lines(before, after);
    eprintln!("--- {} (avant)", title);
    eprintln!("+++ {} (après)", title);
    for change in diff.iter_all_changes() {
        let mut text = change.to_string();
        if text.ends_with('\n') {
            text.pop();
            if text.ends_with('\r') {
                text.pop();
            }
        }
        match change.tag() {
            ChangeTag::Delete => {
                #[cfg(feature = "color")]
                eprintln!("{}", format!("-{}", text).red());
                #[cfg(not(feature = "color"))]
                eprintln!("-{}", text);
            }
            ChangeTag::Insert => {
                #[cfg(feature = "color")]
                eprintln!("{}", format!("+{}", text).green());
                #[cfg(not(feature = "color"))]
                eprintln!("+{}", text);
            }
            ChangeTag::Equal => {
                #[cfg(feature = "color")]
                eprintln!(" {}", text.dimmed());
                #[cfg(not(feature = "color"))]
                eprintln!(" {}", text);
            }
        }
    }
}

fn fmt_entry(task: FmtTask, hooks: &Hooks) -> Result<()> {
    let formatter = match hooks.fmt {
        Some(f) => f,
        None => {
            return bail_diagnostic(
                Diagnostic::new("HOOK003", "Formatage indisponible : hook `fmt` absent")
                    .with_note("Activez la feature `fmt` (`cargo run -p vitte-cli --features fmt`) ou fournissez `Hooks::fmt`.")
                    .with_help("Recompilez vitte-cli avec la feature `fmt` ou fournissez un formateur."),
            );
        }
    };

    let src = read_source(&task.input)?;
    let formatted = match formatter(&src, task.check) {
        Ok(out) => out,
        Err(err) => {
            return bail_diagnostic(
                Diagnostic::new("FMT200", "Le formateur a retourné une erreur.")
                    .with_note(format!("détail: {err}"))
                    .with_help("Activez les journaux (RUST_LOG=debug) ou inspectez le fichier pour repérer les constructions non supportées."),
            );
        }
    };

    let label = input_label(&task.input);
    let changed = src != formatted;

    if task.check {
        if task.diff && changed {
            print_text_diff(&label, &src, &formatted);
        }
        if changed {
            return bail_diagnostic(
                Diagnostic::new("FMT201", "Le fichier n'est pas formaté.")
                    .with_note(format!("fichier : {label}"))
                    .with_help("Exécutez `vitte fmt --in-place <fichier>` ou retirez --check pour appliquer le formatage."),
            );
        }
        status_ok("FMT", &format!("check OK ({label})"));
        return Ok(());
    }

    if task.diff && changed {
        print_text_diff(&label, &src, &formatted);
    }

    match task.output {
        Output::Stdout => {
            let mut w = BufWriter::new(io::stdout().lock());
            w.write_all(formatted.as_bytes())?;
            w.flush()?;
        }
        Output::InPlace => match task.input {
            Input::Path(ref p) => write_text_atomic(p, &formatted)?,
            Input::Stdin => anyhow::bail!("--in-place nécessite un fichier d'entrée"),
        },
        Output::Path(ref p) => write_text_atomic(p, &formatted)?,
        Output::Auto => anyhow::bail!("Output::Auto n'est pas valide pour fmt"),
    }

    let tag_msg = match task.output {
        Output::Stdout => "stdout".to_string(),
        Output::Path(ref p) => display(p),
        Output::InPlace => match task.input {
            Input::Path(ref p) => display(p),
            Input::Stdin => "stdin".to_string(),
        },
        Output::Auto => "auto".to_string(),
    };

    status_ok("FMT", &format!("écrit -> {tag_msg}"));
    Ok(())
}

fn inspect_entry(task: InspectTask, hooks: &Hooks) -> Result<()> {
    let f = match hooks.inspect {
        Some(f) => f,
        None => return bail_diagnostic(
            Diagnostic::new("HOOK004", "Inspection indisponible : hook `inspect` absent")
                .with_note("Activez la feature `engine`/`vm` ou renseignez `Hooks::inspect` pour analyser vos bytecodes.")
                .with_help(
                    "Recompilez vitte-cli avec la feature `engine` ou fournissez un inspecteur.",
                ),
        ),
    };
    let bytes = match task.input {
        InputKind::BytecodePath(p) => {
            if p.as_os_str() == "-" {
                read_stdin_bytes()?
            } else {
                fs::read(&p).with_context(|| format!("lecture bytecode: {}", display(&p)))?
            }
        }
        InputKind::BytecodeBytes(b) => b,
        InputKind::SourcePath(p) => {
            return bail_diagnostic(
                Diagnostic::new(
                    "INS100",
                    format!("'inspect' attend un bytecode, pas une source : {}", display(&p)),
                )
                .with_note(
                    "Utilisez `vitte compile <fichier>` pour générer un `.vitbc` avant inspection.",
                )
                .with_help("Compilez d'abord votre source en .vitbc."),
            );
        }
        InputKind::SourceStdin => read_stdin_bytes()?,
    };
    let options = task.options.clone();
    let text = match f(&bytes, &options) {
        Ok(out) => out,
        Err(err) => {
            return bail_diagnostic(
                Diagnostic::new("INS200", "L'inspecteur a signalé une erreur.")
                    .with_note(format!("détail: {err}"))
                    .with_help("Activez les journaux (RUST_LOG=debug) pour diagnostiquer le contenu du bytecode."),
            );
        }
    };
    let mut w = BufWriter::new(io::stdout().lock());
    w.write_all(text.as_bytes())?;
    w.flush()?;
    Ok(())
}

fn read_stdin_bytes() -> Result<Vec<u8>> {
    let mut buf = Vec::new();
    io::stdin().lock().read_to_end(&mut buf).context("impossible de lire stdin")?;
    if buf.is_empty() {
        return bail_diagnostic(
            Diagnostic::new("INS102", "stdin ne contient aucun octet")
                .with_note("Exemple : `cat out.vitbc | vitte inspect -`")
                .with_help("Redirigez un bytecode .vitbc vers l'entrée standard."),
        );
    }
    Ok(buf)
}

pub mod inspect {
    use serde::Serialize;

    #[derive(Debug, Clone, Default)]
    pub struct InspectOptions {
        pub summary: bool,
        pub header: bool,
        pub symbols: bool,
        pub sections: bool,
        pub consts: bool,
        pub strings: bool,
        pub imports: bool,
        pub exports: bool,
        pub hexdump: bool,
        pub disasm: bool,
        pub json: bool,
        pub target: bool,
        pub debug: bool,
        pub meta: bool,
        pub verify: bool,
        pub size: bool,
        pub deps: bool,
        pub entry: bool,
        pub dump_all: bool,
    }

    impl InspectOptions {
        pub fn ensure_defaults(&mut self) {
            if self.dump_all {
                self.summary = true;
                self.header = true;
                self.symbols = true;
                self.sections = true;
                self.consts = true;
                self.strings = true;
                self.imports = true;
                self.exports = true;
                self.hexdump = true;
                self.disasm = true;
                self.target = true;
                self.debug = true;
                self.meta = true;
                self.verify = true;
                self.size = true;
                self.deps = true;
                self.entry = true;
            }

            if !self.summary
                && !self.header
                && !self.symbols
                && !self.sections
                && !self.consts
                && !self.strings
                && !self.imports
                && !self.exports
                && !self.hexdump
                && !self.disasm
                && !self.target
                && !self.debug
                && !self.meta
                && !self.verify
                && !self.size
                && !self.deps
                && !self.entry
                && !self.json
            {
                self.summary = true;
            }
        }
    }

    #[derive(Debug)]
    struct InspectionData {
        format: InspectFormat,
        size: usize,
        declared_len: Option<usize>,
        snippet: Option<String>,
        sections: Vec<SectionInfo>,
        strings: Vec<String>,
        notes: Vec<String>,
        hash: Option<String>,
        verify_ok: Option<bool>,
        payload_offset: usize,
        payload: Vec<u8>,
    }

    #[derive(Debug)]
    struct SectionInfo {
        name: &'static str,
        offset: usize,
        size: usize,
    }

    #[derive(Debug, Serialize)]
    struct InspectionReport {
        format: String,
        size: usize,
        declared_len: Option<usize>,
        target: Option<String>,
        snippet_preview: Option<String>,
        sections: Vec<ReportSection>,
        strings: Vec<String>,
        symbols: Vec<String>,
        consts: Vec<String>,
        imports: Vec<String>,
        exports: Vec<String>,
        dependencies: Vec<String>,
        entry: Option<String>,
        hash_blake3: Option<String>,
        verified: Option<bool>,
        notes: Vec<String>,
    }

    #[derive(Debug, Serialize)]
    struct ReportSection {
        name: String,
        offset: usize,
        size: usize,
    }

    #[derive(Debug)]
    enum InspectFormat {
        Vbc0,
        Unknown,
    }

    impl InspectFormat {
        fn as_str(&self) -> &'static str {
            match self {
                InspectFormat::Vbc0 => "VBC0",
                InspectFormat::Unknown => "unknown",
            }
        }
    }

    pub fn render(bytes: &[u8], opts: &InspectOptions) -> String {
        let mut options = opts.clone();
        options.ensure_defaults();
        let data = analyze(bytes);

        if options.json {
            return render_json(&data);
        }

        let mut sections = Vec::new();

        if options.summary {
            sections.push(render_summary(&data));
        }
        if options.header {
            sections.push(render_header(&data));
        }
        if options.sections {
            sections.push(render_sections(&data));
        }
        if options.size {
            sections.push(render_size(&data));
        }
        if options.symbols {
            sections.push(render_symbols(&data));
        }
        if options.consts {
            sections.push(render_consts(&data));
        }
        if options.strings {
            sections.push(render_strings(&data));
        }
        if options.imports {
            sections.push(render_imports(&data));
        }
        if options.exports {
            sections.push(render_exports(&data));
        }
        if options.deps {
            sections.push(render_deps(&data));
        }
        if options.entry {
            sections.push(render_entry(&data));
        }
        if options.target {
            sections.push(render_target(&data));
        }
        if options.meta {
            sections.push(render_meta(&data));
        }
        if options.verify {
            sections.push(render_verify(&data));
        }
        if options.debug {
            sections.push(render_debug(&data));
        }
        if options.disasm {
            sections.push(render_disasm(&data));
        }
        if options.hexdump {
            sections.push(render_hexdump(bytes));
        }

        if sections.is_empty() {
            sections.push(render_summary(&data));
        }

        sections.join("\n\n")
    }

    fn analyze(bytes: &[u8]) -> InspectionData {
        let hash = Some(blake3::hash(bytes).to_hex().to_string());
        if bytes.len() >= 8 && &bytes[..4] == b"VBC0" {
            let declared = u32::from_le_bytes([bytes[4], bytes[5], bytes[6], bytes[7]]) as usize;
            let payload = bytes.get(8..).unwrap_or_default();
            let snippet_len = payload.len().min(declared).min(160);
            let snippet = if snippet_len == 0 {
                None
            } else {
                Some(String::from_utf8_lossy(&payload[..snippet_len]).to_string())
            };

            let mut sections = vec![SectionInfo { name: ".header", offset: 0, size: 8 }];
            if !payload.is_empty() {
                sections.push(SectionInfo {
                    name: ".text",
                    offset: 8,
                    size: payload.len().min(declared),
                });
            }
            if payload.len() > declared {
                sections.push(SectionInfo {
                    name: ".extra",
                    offset: 8 + declared,
                    size: payload.len() - declared,
                });
            }

            let strings = extract_strings(payload);
            let verify_ok = Some(payload.len() == declared);
            let mut notes = vec!["Inspection VBC0 simplifiée (format expérimental).".to_string()];
            if verify_ok == Some(false) {
                notes.push("Payload plus long que la longueur déclarée (stub)".to_string());
            }

            InspectionData {
                format: InspectFormat::Vbc0,
                size: bytes.len(),
                declared_len: Some(declared),
                snippet,
                sections,
                strings,
                notes,
                hash,
                verify_ok,
                payload_offset: 8,
                payload: payload.to_vec(),
            }
        } else {
            InspectionData {
                format: InspectFormat::Unknown,
                size: bytes.len(),
                declared_len: None,
                snippet: None,
                sections: Vec::new(),
                strings: extract_strings(bytes),
                notes: vec!["Format non reconnu — inspection limitée.".to_string()],
                hash,
                verify_ok: None,
                payload_offset: 0,
                payload: bytes.to_vec(),
            }
        }
    }

    fn extract_strings(bytes: &[u8]) -> Vec<String> {
        let mut acc = Vec::new();
        let mut current = Vec::new();
        for &b in bytes {
            let ch = b as char;
            if ch.is_ascii_graphic() || ch == ' ' {
                current.push(b);
            } else {
                if current.len() >= 4 {
                    if let Ok(s) = String::from_utf8(current.clone()) {
                        acc.push(s);
                    }
                }
                current.clear();
            }
        }
        if current.len() >= 4 {
            if let Ok(s) = String::from_utf8(current) {
                acc.push(s);
            }
        }
        if acc.len() > 16 {
            acc.truncate(16);
        }
        acc
    }

    fn render_summary(data: &InspectionData) -> String {
        match data.format {
            InspectFormat::Vbc0 => {
                let declared = data
                    .declared_len
                    .map(|len| format!("\nlongueur déclarée : {len} octets"))
                    .unwrap_or_default();
                let snippet = data
                    .snippet
                    .as_ref()
                    .map(|s| format!("\naperçu source :\n{}", s))
                    .unwrap_or_default();
                format!("Format : VBC0\nsize : {} octets{declared}{snippet}", data.size)
            }
            InspectFormat::Unknown => format!("Format : inconnu\nsize : {} octets", data.size),
        }
    }

    fn render_header(data: &InspectionData) -> String {
        let mut out = format!(
            "En-tête:\n  format : {}\n  taille totale : {} octets",
            data.format.as_str(),
            data.size
        );
        if let Some(declared) = data.declared_len {
            out.push_str(&format!("\n  longueur déclarée : {} octets", declared));
        }
        if let Some(hash) = &data.hash {
            out.push_str(&format!("\n  BLAKE3 : {hash}"));
        }
        out
    }

    fn render_sections(data: &InspectionData) -> String {
        if data.sections.is_empty() {
            return "Sections :\n  (non disponibles pour ce format)".to_string();
        }
        let mut out = String::from("Sections :");
        for section in &data.sections {
            out.push_str(&format!(
                "\n  {name:<10} offset={offset:#06x} size={size}",
                name = section.name,
                offset = section.offset,
                size = section.size
            ));
        }
        out
    }

    fn render_size(data: &InspectionData) -> String {
        if data.sections.is_empty() {
            return format!("Taille totale : {} octets", data.size);
        }
        let mut out = format!("Taille totale : {} octets", data.size);
        let mut accum = 0usize;
        for section in &data.sections {
            accum += section.size;
        }
        out.push_str(&format!("\nSomme sections : {} octets", accum));
        out
    }

    fn render_symbols(data: &InspectionData) -> String {
        match data.format {
            InspectFormat::Vbc0 => {
                "Symboles :\n  (table des symboles non encodée dans ce format)".to_string()
            }
            InspectFormat::Unknown => {
                "Symboles :\n  Impossible de lister les symboles — format non reconnu.".to_string()
            }
        }
    }

    fn render_consts(data: &InspectionData) -> String {
        match data.format {
            InspectFormat::Vbc0 => "Constantes :\n  (non disponibles dans ce build)".to_string(),
            InspectFormat::Unknown => {
                "Constantes :\n  Impossible d'extraire les constantes.".to_string()
            }
        }
    }

    fn render_strings(data: &InspectionData) -> String {
        if data.strings.is_empty() {
            return "Chaînes embarquées :\n  aucune chaîne ASCII détectée".to_string();
        }
        let mut out = String::from("Chaînes embarquées :");
        for s in data.strings.iter().take(10) {
            out.push_str(&format!("\n  \"{}\"", s.replace('\n', "\\n")));
        }
        if data.strings.len() > 10 {
            out.push_str(&format!("\n  … ({}) supplémentaires", data.strings.len() - 10));
        }
        out
    }

    fn render_imports(_data: &InspectionData) -> String {
        "Imports :\n  Informations non disponibles dans cette préversion.".to_string()
    }

    fn render_exports(_data: &InspectionData) -> String {
        "Exports :\n  Informations non disponibles dans cette préversion.".to_string()
    }

    fn render_deps(_data: &InspectionData) -> String {
        "Dépendances :\n  Aucune dépendance module encodée dans ce fichier.".to_string()
    }

    fn render_entry(_data: &InspectionData) -> String {
        "Point d'entrée : non spécifié (format VBC0 stub).".to_string()
    }

    fn render_target(data: &InspectionData) -> String {
        let target = match data.format {
            InspectFormat::Vbc0 => "non spécifiée (VBC0 stub)".to_string(),
            InspectFormat::Unknown => "inconnue".to_string(),
        };
        format!("Cible supposée : {target}")
    }

    fn render_meta(data: &InspectionData) -> String {
        if let Some(hash) = &data.hash {
            format!(
                "Métadonnées :\n  Build-id (BLAKE3) : {hash}\n  Auteur : inconnu\n  Timestamp : n/a"
            )
        } else {
            "Métadonnées indisponibles.".to_string()
        }
    }

    fn render_verify(data: &InspectionData) -> String {
        match data.verify_ok {
            Some(true) => {
                format!("Intégrité : OK (BLAKE3 = {})", data.hash.as_deref().unwrap_or("n/a"))
            }
            Some(false) => "Intégrité : échec (payload > longueur déclarée)".to_string(),
            None => "Intégrité : impossible de vérifier.".to_string(),
        }
    }

    fn render_debug(_data: &InspectionData) -> String {
        "Debug info :\n  Table des lignes / scopes non disponible.".to_string()
    }

    fn render_disasm(data: &InspectionData) -> String {
        if data.payload.is_empty() {
            return "Désassemblage :\n  Aucun bytecode à analyser.".to_string();
        }
        let mut out = String::from("Désassemblage (stub) :");
        for (idx, chunk) in data.payload.chunks(4).enumerate().take(32) {
            let offset = data.payload_offset + idx * 4;
            let op = chunk.get(0).copied().unwrap_or(0);
            let operands =
                chunk.iter().skip(1).map(|b| format!("{:02X}", b)).collect::<Vec<_>>().join(" ");
            out.push_str(&format!("\n  {offset:08X}: OP_{op:02X} {operands}", operands = operands));
        }
        if data.payload.len() / 4 > 32 {
            out.push_str("\n  …");
        }
        out
    }

    fn render_hexdump(bytes: &[u8]) -> String {
        let mut out = String::new();
        for (offset, chunk) in bytes.chunks(16).enumerate() {
            let off = offset * 16;
            let hex = chunk.iter().map(|b| format!("{:02X}", b)).collect::<Vec<_>>().join(" ");
            let ascii = chunk
                .iter()
                .map(|b| {
                    let c = *b as char;
                    if c.is_ascii_graphic() || c.is_ascii_whitespace() {
                        c
                    } else {
                        '.'
                    }
                })
                .collect::<String>();
            out.push_str(&format!("{off:08X}: {:<47} |{}|\n", hex, ascii));
        }
        if out.is_empty() {
            out.push_str("(flux vide)");
        }
        out
    }

    fn render_json(data: &InspectionData) -> String {
        let report = InspectionReport {
            format: data.format.as_str().to_string(),
            size: data.size,
            declared_len: data.declared_len,
            target: Some(match data.format {
                InspectFormat::Vbc0 => "vitte-vm (stub)".to_string(),
                InspectFormat::Unknown => "unknown".to_string(),
            }),
            snippet_preview: data.snippet.clone(),
            sections: data
                .sections
                .iter()
                .map(|s| ReportSection { name: s.name.to_string(), offset: s.offset, size: s.size })
                .collect(),
            strings: data.strings.clone(),
            symbols: Vec::new(),
            consts: Vec::new(),
            imports: Vec::new(),
            exports: Vec::new(),
            dependencies: Vec::new(),
            entry: None,
            hash_blake3: data.hash.clone(),
            verified: data.verify_ok,
            notes: data.notes.clone(),
        };
        serde_json::to_string_pretty(&report).unwrap_or_else(|_| "{}".to_string())
    }
}

pub mod registry {
    use anyhow::{Context, Result};
    use serde::Deserialize;
    use std::{fs, path::Path};

    #[derive(Debug, Deserialize, Clone)]
    pub struct Module {
        pub name: String,
        pub version: String,
        #[serde(default)]
        pub description: String,
        #[serde(default)]
        pub url: Option<String>,
        #[serde(default)]
        pub checksum: Option<String>,
        #[serde(default)]
        pub tags: Vec<String>,
    }

    #[derive(Debug, Deserialize)]
    struct Index {
        modules: Vec<Module>,
    }

    pub fn load_local_index(root: &Path) -> Result<Vec<Module>> {
        let path = root.join("registry/modules/index.json");
        let data = fs::read_to_string(&path)
            .with_context(|| format!("lecture registre {}", path.display()))?;
        let index: Index = serde_json::from_str(&data)
            .with_context(|| format!("parse registre {}", path.display()))?;
        Ok(index.modules)
    }

    pub fn find<'a>(modules: &'a [Module], name: &str) -> Option<&'a Module> {
        modules.iter().find(|m| m.name == name)
    }
}

fn disasm_entry(task: DisasmTask, hooks: &Hooks) -> Result<()> {
    let f = match hooks.disasm {
        Some(f) => f,
        None => {
            return bail_diagnostic(
                Diagnostic::new("HOOK005", "Désassemblage indisponible : hook `disasm` absent")
                    .with_note("Activez la feature `vm` ou implémentez Hooks::disasm pour produire un rendu lisible.")
                    .with_help("Recompilez vitte-cli avec la feature `disasm/modules` ou fournissez un désassembleur."),
            )
        },
    };
    let bytes = match task.input {
        InputKind::BytecodePath(p) => {
            fs::read(&p).with_context(|| format!("lecture bytecode: {}", display(&p)))?
        }
        InputKind::BytecodeBytes(b) => b,
        InputKind::SourcePath(p) => {
            return bail_diagnostic(
                Diagnostic::new("DIS100", format!("'disasm' attend un bytecode, pas une source : {}", display(&p)))
                    .with_note("Compilez votre fichier (`vitte compile <src>`), puis désassemblez le `.vitbc`."),
            );
        }
        InputKind::SourceStdin => {
            return bail_diagnostic(
                Diagnostic::new("DIS101", "'disasm' attend un bytecode sur stdin")
                    .with_note("Redirigez un bytecode (`cat app.vitbc | vitte disasm -`)."),
            );
        }
    };
    let text = match f(&bytes) {
        Ok(out) => out,
        Err(err) => {
            return bail_diagnostic(
                Diagnostic::new("DIS200", "Le désassembleur a signalé une erreur.")
                    .with_note(format!("détail: {err}"))
                    .with_help("Assurez-vous que le bytecode provient de `vitte compile` ou activez les journaux détaillés."),
            );
        }
    };
    match task.output {
        Output::Stdout => {
            let mut w = BufWriter::new(io::stdout().lock());
            w.write_all(text.as_bytes())?;
            w.flush()?;
        }
        Output::Path(ref p) => write_text_atomic(p, &text)?,
        Output::InPlace => anyhow::bail!("Output::InPlace n'a pas de sens pour disasm"),
        Output::Auto => anyhow::bail!("Output::Auto n'est pas valide pour disasm"),
    }
    status_ok("DISASM", "ok");
    Ok(())
}

fn modules_entry(task: ModulesTask) -> Result<()> {
    #[cfg(not(feature = "modules"))]
    {
        let _ = task;
        return bail_diagnostic(
            Diagnostic::new(
                "HOOK006",
                "Commande `modules` indisponible (feature `modules` désactivée)",
            )
            .with_note("Activez la feature `modules` (`cargo run -p vitte-cli --features modules`) pour inclure la liste intégrée.")
            .with_help("Recompilez vitte-cli avec `--features modules`."),
        );
    }

    #[cfg(feature = "modules")]
    {
        use ModulesFormat::*;

        let mods = vitte_modules::modules();
        match task.format {
            Table => {
                if mods.is_empty() {
                    println!(
                        "(aucun module activé — compile `vitte-modules` avec les features désirées)"
                    );
                } else {
                    for module in mods {
                        println!("{:<12} {}", module.name, module.description);
                        if !module.tags.is_empty() {
                            println!("    tags: {}", module.tags.join(", "));
                        }
                    }
                }
                Ok(())
            }
            Json => {
                let payload: Vec<_> = mods
                    .iter()
                    .map(|m| {
                        serde_json::json!({
                            "name": m.name,
                            "description": m.description,
                            "tags": m.tags,
                        })
                    })
                    .collect();
                println!("{}", serde_json::to_string_pretty(&payload)?);
                Ok(())
            }
        }
    }
}

// ───────────────────────────── Utilitaires E/S ─────────────────────────────

fn read_source(input: &Input) -> Result<String> {
    match input {
        Input::Stdin => {
            let mut s = String::new();
            io::stdin().read_to_string(&mut s)?;
            Ok(s)
        }
        Input::Path(p) => {
            let f = File::open(&p).with_context(|| format!("ouverture: {}", display(p)))?;
            let mut r = BufReader::new(f);
            let mut s = String::new();
            r.read_to_string(&mut s)?;
            Ok(s)
        }
    }
}

fn write_bytes_atomic(path: &Path, bytes: &[u8]) -> Result<()> {
    let parent =
        path.parent().ok_or_else(|| anyhow!("chemin de sortie sans parent: {}", display(path)))?;
    let tmp = unique_tmp_path(parent, path.file_name().unwrap_or_default());
    {
        let mut w = BufWriter::new(File::create(&tmp)?);
        w.write_all(bytes)?;
        w.flush()?;
    }
    if path.exists() {
        // Windows : Rename sur cible existante peut échouer
        let _ = fs::remove_file(path);
    }
    fs::rename(&tmp, path).or_else(|_| {
        // fallback : copie puis suppr tmp
        fs::copy(&tmp, path).map(|_| ()).and_then(|_| fs::remove_file(&tmp).map(|_| ()))
    })?;
    Ok(())
}

fn write_text_atomic(path: &Path, text: &str) -> Result<()> {
    write_bytes_atomic(path, text.as_bytes())
}

fn unique_tmp_path(dir: &Path, base: &std::ffi::OsStr) -> PathBuf {
    let mut i = 0u32;
    loop {
        let candidate = dir.join(format!("{}.tmp{}", base.to_string_lossy(), i));
        if !candidate.exists() {
            return candidate;
        }
        i = i.wrapping_add(1);
    }
}

fn default_bytecode_path(src: &Path) -> PathBuf {
    let stem = src.file_stem().and_then(|s| s.to_str()).unwrap_or("out");
    let mut p = src.with_file_name(stem);
    p.set_extension("vitbc");
    p
}

fn display(p: &Path) -> String {
    p.to_string_lossy().to_string()
}

// ───────────────────────────── Sorties jolies ─────────────────────────────

fn status_ok(tag: &str, msg: &str) {
    #[cfg(feature = "color")]
    {
        eprintln!("{} {}", tag.green().bold(), msg);
    }
    #[cfg(not(feature = "color"))]
    {
        eprintln!("{} {}", tag, msg);
    }
}

fn status_info(tag: &str, msg: &str) {
    #[cfg(feature = "color")]
    {
        eprintln!("{} {}", tag.blue().bold(), msg);
    }
    #[cfg(not(feature = "color"))]
    {
        eprintln!("{} {}", tag, msg);
    }
}

fn status_warn(tag: &str, msg: &str) {
    #[cfg(feature = "color")]
    {
        eprintln!("{} {}", tag.yellow().bold(), msg);
    }
    #[cfg(not(feature = "color"))]
    {
        eprintln!("{} {}", tag, msg);
    }
}

#[allow(dead_code)]
enum Severity {
    Error,
    Warning,
}

struct Diagnostic {
    severity: Severity,
    code: &'static str,
    message: String,
    notes: Vec<String>,
    help: Option<String>,
}

impl Diagnostic {
    fn new(code: &'static str, message: impl Into<String>) -> Self {
        Self {
            severity: Severity::Error,
            code,
            message: message.into(),
            notes: Vec::new(),
            help: None,
        }
    }

    #[allow(dead_code)]
    fn error(code: &'static str, message: impl Into<String>) -> Self {
        Self {
            severity: Severity::Error,
            code,
            message: message.into(),
            notes: Vec::new(),
            help: None,
        }
    }

    #[allow(dead_code)]
    fn warning(code: &'static str, message: impl Into<String>) -> Self {
        Self {
            severity: Severity::Warning,
            code,
            message: message.into(),
            notes: Vec::new(),
            help: None,
        }
    }

    #[allow(dead_code)]
    fn with_note(mut self, note: impl Into<String>) -> Self {
        self.notes.push(note.into());
        self
    }

    fn with_help(mut self, help: impl Into<String>) -> Self {
        self.help = Some(help.into());
        self
    }
}

fn emit_diagnostic(diag: &Diagnostic) {
    #[cfg(feature = "color")]
    {
        match diag.severity {
            Severity::Error => {
                eprintln!("{} {}", format!("error[{}]", diag.code).red().bold(), diag.message.red())
            }
            Severity::Warning => eprintln!(
                "{} {}",
                format!("warning[{}]", diag.code).yellow().bold(),
                diag.message.yellow()
            ),
        }
        for note in &diag.notes {
            eprintln!("{} {}", "note".blue().bold(), note);
        }
        if let Some(help) = &diag.help {
            eprintln!("{} {}", "help".green().bold(), help);
        }
    }
    #[cfg(not(feature = "color"))]
    {
        let label = match diag.severity {
            Severity::Error => format!("error[{}]", diag.code),
            Severity::Warning => format!("warning[{}]", diag.code),
        };
        eprintln!("{}: {}", label, diag.message);
        for note in &diag.notes {
            eprintln!("  note: {}", note);
        }
        if let Some(help) = &diag.help {
            eprintln!("  help: {}", help);
        }
    }
}

fn bail_diagnostic<T>(diag: Diagnostic) -> Result<T> {
    let message = format!("error[{}]: {}", diag.code, diag.message);
    emit_diagnostic(&diag);
    Err(anyhow!(message))
}

#[allow(dead_code)]
fn print_diagnostic(diag: Diagnostic) {
    emit_diagnostic(&diag);
}

pub mod repl {
    use anyhow::{anyhow, Context, Result};
    use rustyline::{
        completion::{Completer, Pair},
        config::Configurer,
        error::ReadlineError,
        highlight::Highlighter,
        hint::{Hinter, HistoryHinter},
        history::{FileHistory, History},
        validate::{ValidationContext, ValidationResult, Validator},
        Context as LineContext, Editor, Helper,
    };
    use std::{
        borrow::Cow,
        fs,
        io::{self, Write},
        path::Path,
    };

    #[cfg(feature = "color")]
    use owo_colors::OwoColorize;

    pub fn fallback(prompt: &str) -> Result<i32> {
        match advanced_repl(prompt) {
            Ok(code) => Ok(code),
            Err(err) => {
                eprintln!("[repl] initialisation interactive échouée: {err}");
                eprintln!("[repl] retour au mode basique (stdin)");
                basic_stdio(prompt)
            }
        }
    }

    fn advanced_repl(prompt: &str) -> Result<i32> {
        let mut rl = Editor::<ReplHelper, FileHistory>::new()
            .context("initialisation de l'éditeur interactif")?;
        rl.set_auto_add_history(false);
        if let Err(err) = rl.set_history_ignore_dups(true) {
            eprintln!("[repl] impossible d'activer history_ignore_dups: {err}");
        }
        rl.set_history_ignore_space(true);
        rl.set_helper(Some(ReplHelper::default()));
        let mut session = ReplSession::new(prompt);
        session.print_banner();

        loop {
            let prompt = session.prompt_for_next_line();
            match rl.readline(&prompt) {
                Ok(line) => match session.handle_line(line, &mut rl)? {
                    FlowControl::Continue => continue,
                    FlowControl::Exit => break,
                },
                Err(ReadlineError::Interrupted) => {
                    session.on_interrupt();
                    continue;
                }
                Err(ReadlineError::Eof) => {
                    session.on_eof();
                    break;
                }
                Err(err) => return Err(err.into()),
            }
        }
        Ok(0)
    }

    fn basic_stdio(prompt: &str) -> Result<i32> {
        let stdin = io::stdin();
        let mut stdout = io::stdout();
        #[cfg(feature = "color")]
        println!("{}", "Vitte REPL (mode basique) — :help pour l'aide, :quit pour quitter".cyan());
        #[cfg(not(feature = "color"))]
        println!("Vitte REPL (mode basique) — :help pour l'aide, :quit pour quitter");

        let mut line = String::new();
        loop {
            print!("{prompt}");
            stdout.flush()?;
            line.clear();
            if stdin.read_line(&mut line)? == 0 {
                println!("(EOF) sortie du REPL");
                break;
            }
            let trimmed = line.trim();
            if trimmed.is_empty() {
                continue;
            }
            if trimmed.starts_with(':') {
                match trimmed {
                    ":quit" | ":q" | ":exit" | "exit" => break,
                    ":help" | ":h" => ReplSession::print_help(),
                    _ => println!("Commande inconnue ({trimmed}) — tape :help"),
                }
                continue;
            }
            println!("→ {}", trimmed);
        }
        Ok(0)
    }

    enum FlowControl {
        Continue,
        Exit,
    }

    type ReplEditor = Editor<ReplHelper, FileHistory>;

    #[derive(Copy, Clone)]
    struct CommandInfo {
        name: &'static str,
        adds_space: bool,
    }

    const REPL_COMMANDS: &[CommandInfo] = &[
        CommandInfo { name: "help", adds_space: false },
        CommandInfo { name: "h", adds_space: false },
        CommandInfo { name: "quit", adds_space: false },
        CommandInfo { name: "exit", adds_space: false },
        CommandInfo { name: "q", adds_space: false },
        CommandInfo { name: "history", adds_space: true },
        CommandInfo { name: "clear-history", adds_space: false },
        CommandInfo { name: "multi", adds_space: true },
        CommandInfo { name: "show", adds_space: false },
        CommandInfo { name: "buffer", adds_space: false },
        CommandInfo { name: "clear", adds_space: false },
        CommandInfo { name: "run", adds_space: false },
        CommandInfo { name: "flush", adds_space: false },
        CommandInfo { name: "save", adds_space: true },
        CommandInfo { name: "load", adds_space: true },
        CommandInfo { name: "pop", adds_space: false },
        CommandInfo { name: "prompt", adds_space: true },
        CommandInfo { name: "status", adds_space: false },
    ];

    #[derive(Default)]
    struct ReplHelper {
        hinter: HistoryHinter,
    }

    impl Helper for ReplHelper {}

    impl Hinter for ReplHelper {
        type Hint = String;

        fn hint(&self, line: &str, pos: usize, ctx: &LineContext<'_>) -> Option<String> {
            self.hinter.hint(line, pos, ctx)
        }
    }

    impl Highlighter for ReplHelper {
        fn highlight_prompt<'b, 's: 'b, 'p: 'b>(
            &'s self,
            prompt: &'p str,
            _: bool,
        ) -> Cow<'p, str> {
            Cow::Borrowed(prompt)
        }
    }

    impl Completer for ReplHelper {
        type Candidate = Pair;

        fn complete(
            &self,
            line: &str,
            pos: usize,
            _ctx: &LineContext<'_>,
        ) -> Result<(usize, Vec<Pair>), ReadlineError> {
            let slice = &line[..pos];
            let Some(colon_idx) = slice.rfind(':') else {
                return Ok((pos, Vec::new()));
            };
            if colon_idx > 0 {
                if let Some(prev) = slice[..colon_idx].chars().last() {
                    if !prev.is_whitespace() {
                        return Ok((pos, Vec::new()));
                    }
                }
            }
            let token = &slice[colon_idx + 1..];
            if token.chars().any(|c| c.is_whitespace()) {
                return Ok((pos, Vec::new()));
            }
            let token_lower = token.to_ascii_lowercase();
            let mut pairs = Vec::new();
            for info in REPL_COMMANDS {
                if info.name.starts_with(&token_lower) {
                    let mut replacement = format!(":{}", info.name);
                    if info.adds_space {
                        replacement.push(' ');
                    }
                    let display = format!(":{}", info.name);
                    pairs.push(Pair { display, replacement });
                }
            }
            if pairs.is_empty() {
                return Ok((pos, pairs));
            }
            let start = colon_idx;
            Ok((start, pairs))
        }
    }

    impl Validator for ReplHelper {
        fn validate(
            &self,
            _: &mut ValidationContext<'_>,
        ) -> Result<ValidationResult, ReadlineError> {
            Ok(ValidationResult::Valid(None))
        }
    }

    struct ReplSession {
        base_prompt: String,
        prompt_override: Option<String>,
        multiline: bool,
        buffer: Vec<String>,
    }

    impl ReplSession {
        fn new(prompt: &str) -> Self {
            Self {
                base_prompt: ensure_trailing_space(prompt),
                prompt_override: None,
                multiline: false,
                buffer: Vec::new(),
            }
        }

        fn prompt_for_next_line(&self) -> String {
            let base = self.prompt_override.as_deref().unwrap_or(&self.base_prompt);
            if self.multiline {
                format!("{}{}| ", base, self.buffer.len() + 1)
            } else {
                base.to_string()
            }
        }

        fn handle_line(&mut self, line: String, rl: &mut ReplEditor) -> Result<FlowControl> {
            let trimmed = line.trim();
            if trimmed.is_empty() {
                if self.multiline {
                    self.buffer.push(String::new());
                }
                return Ok(FlowControl::Continue);
            }

            if trimmed.starts_with(':') {
                return self.handle_command(trimmed, rl);
            }

            if self.multiline {
                self.buffer.push(line);
                println!("(buffer {})", self.buffer.len());
            } else {
                self.echo_single(trimmed);
                if let Err(err) = rl.add_history_entry(trimmed) {
                    eprintln!("[repl] historique indisponible: {err}");
                }
            }
            Ok(FlowControl::Continue)
        }

        fn handle_command(&mut self, raw: &str, rl: &mut ReplEditor) -> Result<FlowControl> {
            let command = raw.trim_start_matches(':').trim();
            if command.is_empty() {
                Self::print_help();
                return Ok(FlowControl::Continue);
            }

            let mut parts = command.splitn(2, char::is_whitespace);
            let cmd = parts.next().unwrap_or("").to_ascii_lowercase();
            let arg = parts.next().map(str::trim).filter(|s| !s.is_empty());

            match cmd.as_str() {
                "help" | "h" => Self::print_help(),
                "quit" | "exit" | "q" => return Ok(FlowControl::Exit),
                "history" => self.print_history(rl, arg)?,
                "clear-history" => {
                    if let Err(err) = rl.history_mut().clear() {
                        eprintln!("[repl] impossible d'effacer l'historique: {err}");
                    } else {
                        println!("(historique effacé)");
                    }
                }
                "multi" => self.toggle_multiline(arg),
                "show" | "buffer" => self.show_buffer(),
                "clear" => {
                    self.buffer.clear();
                    println!("(buffer vidé)");
                }
                "run" | "flush" => self.flush_buffer(rl),
                "save" => {
                    let path = arg.ok_or_else(|| anyhow!("utilisation : :save <fichier>"))?;
                    self.save_buffer(path)?;
                }
                "load" => {
                    let path = arg.ok_or_else(|| anyhow!("utilisation : :load <fichier>"))?;
                    self.load_buffer(path)?;
                }
                "pop" => {
                    if let Some(line) = self.buffer.pop() {
                        println!("(retiré) {}", line);
                    } else {
                        println!("(buffer vide)");
                    }
                }
                "prompt" => self.update_prompt(arg),
                "status" => self.print_status(),
                _ => println!("Commande inconnue : {raw} — tape :help"),
            }
            Ok(FlowControl::Continue)
        }

        fn print_banner(&self) {
            #[cfg(feature = "color")]
            println!(
                "{}",
                "Vitte REPL — commandes : :help, :multi, :run, :save, :load, :history, :quit"
                    .cyan()
            );
            #[cfg(not(feature = "color"))]
            println!("Vitte REPL — commandes : :help, :multi, :run, :save, :load, :history, :quit");
        }

        fn print_help() {
            #[cfg(feature = "color")]
            let header = "Commandes REPL".bold();
            #[cfg(not(feature = "color"))]
            let header = "Commandes REPL".to_string();
            println!(
                "{header}\n  :help            — afficher cette aide\n  :quit / :exit    — quitter le REPL\n  :history [n]     — afficher l'historique (optionnellement limité)\n  :clear-history   — vider l'historique\n  :multi [on|off]  — basculer le mode multi-ligne\n  :show            — afficher le buffer courant\n  :clear           — vider le buffer\n  :run             — valider le buffer (affiche et vide)\n  :save <file>     — enregistrer le buffer dans un fichier\n  :load <file>     — charger un fichier dans le buffer (active multi-ligne)\n  :pop             — retirer la dernière ligne du buffer\n  :prompt <txt>    — définir une invite personnalisée (:prompt reset pour revenir par défaut)\n  :status          — afficher l'état actuel"
            );
        }

        fn print_history(&self, rl: &ReplEditor, arg: Option<&str>) -> Result<()> {
            let history = rl.history();
            if history.is_empty() {
                println!("(historique vide)");
                return Ok(());
            }
            let limit = arg.and_then(|s| s.parse::<usize>().ok());
            let len = history.len();
            let start = limit.map(|n| len.saturating_sub(n)).unwrap_or(0);
            for (idx, entry) in history.iter().enumerate().skip(start) {
                println!("{:>4} {}", idx + 1, entry);
            }
            Ok(())
        }

        fn toggle_multiline(&mut self, arg: Option<&str>) {
            let target = arg.map(|s| s.to_ascii_lowercase());
            let desired = match target.as_deref() {
                Some("on") | Some("true") => Some(true),
                Some("off") | Some("false") => Some(false),
                Some("toggle") | None => None,
                Some(other) => {
                    println!(
                        "Valeur inconnue pour :multi — utilisez on/off/toggle (reçu: {other})"
                    );
                    return;
                }
            };
            if let Some(value) = desired {
                if value {
                    self.enable_multiline();
                } else {
                    self.disable_multiline();
                }
            } else if self.multiline {
                self.disable_multiline();
            } else {
                self.enable_multiline();
            }
        }

        fn enable_multiline(&mut self) {
            if !self.multiline {
                self.multiline = true;
                println!("(mode multi-ligne activé — terminer avec :run)");
            } else {
                println!("(mode multi-ligne déjà actif)");
            }
        }

        fn disable_multiline(&mut self) {
            if self.multiline {
                self.multiline = false;
                self.buffer.clear();
                println!("(mode multi-ligne désactivé, buffer vidé)");
            } else {
                println!("(mode multi-ligne déjà inactif)");
            }
        }

        fn show_buffer(&self) {
            if self.buffer.is_empty() {
                println!("(buffer vide)");
                return;
            }
            for (idx, line) in self.buffer.iter().enumerate() {
                println!("{:>4} {}", idx + 1, line);
            }
        }

        fn flush_buffer(&mut self, rl: &mut ReplEditor) {
            if self.buffer.is_empty() {
                println!("(buffer vide)");
                return;
            }
            let block = self.buffer.join("\n");
            println!("→ {}", block);
            if let Err(err) = rl.add_history_entry(block.as_str()) {
                eprintln!("[repl] historique indisponible: {err}");
            }
            self.buffer.clear();
        }

        fn save_buffer(&self, path: &str) -> Result<()> {
            if self.buffer.is_empty() {
                println!("(buffer vide — rien à sauvegarder)");
                return Ok(());
            }
            let content = self.buffer.join("\n");
            fs::write(Path::new(path), content).with_context(|| format!("écriture de {path}"))?;
            println!("(buffer enregistré dans {path})");
            Ok(())
        }

        fn load_buffer(&mut self, path: &str) -> Result<()> {
            let data = fs::read_to_string(Path::new(path))
                .with_context(|| format!("lecture de {path}"))?;
            self.buffer = data.lines().map(|s| s.to_string()).collect();
            self.multiline = true;
            println!("(buffer chargé depuis {path}, {} lignes)", self.buffer.len());
            Ok(())
        }

        fn update_prompt(&mut self, arg: Option<&str>) {
            match arg {
                None => println!(
                    "Invite actuelle : {}",
                    self.prompt_override.as_deref().unwrap_or(&self.base_prompt).trim_end()
                ),
                Some(value) if value.eq_ignore_ascii_case("reset") => {
                    self.prompt_override = None;
                    println!("(invite réinitialisée)");
                }
                Some(value) => {
                    let mut prompt = value.to_string();
                    if !prompt.ends_with(' ') {
                        prompt.push(' ');
                    }
                    self.prompt_override = Some(prompt);
                    println!("(nouvelle invite enregistrée)");
                }
            }
        }

        fn print_status(&self) {
            println!(
                "Statut : multi-ligne={}, buffer={} lignes, invite=\"{}\"",
                if self.multiline { "on" } else { "off" },
                self.buffer.len(),
                self.prompt_override.as_deref().unwrap_or(&self.base_prompt).trim_end()
            );
        }

        fn echo_single(&self, line: &str) {
            println!("→ {}", line.trim_end());
        }

        fn on_interrupt(&mut self) {
            if self.multiline && !self.buffer.is_empty() {
                self.buffer.clear();
                println!("(Ctrl-C) buffer multi-ligne vidé");
            } else {
                println!("(Ctrl-C) ligne ignorée");
            }
        }

        fn on_eof(&self) {
            println!("(EOF) sortie du REPL");
        }
    }

    fn ensure_trailing_space(input: &str) -> String {
        if input.ends_with(' ') {
            input.to_string()
        } else {
            format!("{input} ")
        }
    }
}

// ───────────────────────────── Tests ─────────────────────────────

#[cfg(test)]
mod tests {
    use super::*;

    fn fake_compile(src: &str, _opts: &CompileOptions) -> Result<Vec<u8>> {
        // jouet : encode la taille + contenu
        let mut out = Vec::new();
        out.extend_from_slice(&(src.len() as u32).to_le_bytes());
        out.extend_from_slice(src.as_bytes());
        Ok(out)
    }

    fn fake_run(bc: &[u8], _opts: &RunOptions) -> Result<i32> {
        if bc.len() >= 4 {
            Ok(0)
        } else {
            Ok(1)
        }
    }

    fn fake_fmt(src: &str, _check: bool) -> Result<String> {
        Ok(src.trim().to_string())
    }
    fn fake_disasm(bc: &[u8]) -> Result<String> {
        Ok(format!("len={}", bc.len()))
    }
    fn fake_inspect(b: &[u8], _opts: &inspect::InspectOptions) -> Result<String> {
        Ok(format!("bytes:{}", b.len()))
    }

    #[test]
    fn pipeline_compile_stdout() {
        let hooks = Hooks {
            compile: Some(fake_compile),
            run_bc: Some(fake_run),
            repl: None,
            fmt: Some(fake_fmt),
            disasm: Some(fake_disasm),
            inspect: Some(fake_inspect),
        };
        let t = CompileTask {
            input: Input::Stdin,
            output: Output::Stdout,
            optimize: false,
            emit_debug: false,
            auto_mkdir: false,
            overwrite: true,
            time: false,
        };
        // juste vérifier que ça ne panique pas
        let _ = compile_entry(t, &hooks).unwrap();
    }

    #[test]
    fn default_paths() {
        let p = PathBuf::from("src/main.vt");
        let out = super::default_bytecode_path(&p);
        assert_eq!(out.file_name().unwrap().to_string_lossy(), "main.vitbc");
    }
}
