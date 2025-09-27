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

use std::{
    fs,
    fs::File,
    io::{self, BufReader, BufWriter, Read, Write},
    path::{Path, PathBuf},
    time::Instant,
};

use anyhow::{anyhow, Context, Result};

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
    pub input: Input,
    pub output: Output, // si Output::InPlace => réécrit le fichier
    pub check: bool,    // mode --check (retourne erreur si diff)
}

#[derive(Clone, Debug, Default)]
pub struct InspectTask {
    pub input: InputKind,
    pub summary: bool,
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
pub type InspectFn = fn(bytes: &[u8], summary: bool) -> Result<String>;

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
        },
        Command::Run(t) => run_entry(t, hooks),
        Command::Repl(t) => {
            let f = hooks.repl.ok_or_else(|| anyhow!("REPL non disponible (hook manquant)"))?;
            let exit = f(&t.prompt)?;
            Ok(exit)
        },
        Command::Fmt(t) => {
            fmt_entry(t, hooks)?;
            Ok(0)
        },
        Command::Inspect(t) => {
            inspect_entry(t, hooks)?;
            Ok(0)
        },
        Command::Disasm(t) => {
            disasm_entry(t, hooks)?;
            Ok(0)
        },
        Command::Modules(t) => {
            modules_entry(t)?;
            Ok(0)
        },
    }
}

fn compile_entry(task: CompileTask, hooks: &Hooks) -> Result<i32> {
    let CompileTask { input, output, optimize, emit_debug, auto_mkdir, overwrite, time } = task;
    let compiler = hooks
        .compile
        .ok_or_else(|| anyhow!("Compilation indisponible (hook `compile` manquant)"))?;

    let src = read_source(&input).context("lecture de la source")?;

    let start = Instant::now();
    let bc =
        compiler(&src, &CompileOptions { optimize, emit_debug }).context("échec de compilation")?;
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
        status_ok("COMPILE", &format!("{}", display(&out_path)));
    }

    if time {
        status_info("TIME", &format!("compile: {} ms", elapsed.as_millis()));
    }
    Ok(0)
}

fn run_entry(task: RunTask, hooks: &Hooks) -> Result<i32> {
    let runner = hooks.run_bc.ok_or_else(|| {
        anyhow!("Exécution indisponible (hook `run_bc` manquant — activez la VM)")
    })?;

    let bytes = match task.program {
        InputKind::BytecodePath(p) => {
            fs::read(&p).with_context(|| format!("lecture bytecode: {}", display(&p)))?
        },
        InputKind::BytecodeBytes(b) => b,
        InputKind::SourcePath(p) if task.auto_compile => {
            let src = read_source(&Input::Path(p.clone()))?;
            let compiler = hooks
                .compile
                .ok_or_else(|| anyhow!("auto-compile demandé mais hook `compile` manquant"))?;
            compiler(&src, &CompileOptions { optimize: task.optimize, emit_debug: false })?
        },
        InputKind::SourceStdin if task.auto_compile => {
            let src = read_source(&Input::Stdin)?;
            let compiler = hooks
                .compile
                .ok_or_else(|| anyhow!("auto-compile demandé mais hook `compile` manquant"))?;
            compiler(&src, &CompileOptions { optimize: task.optimize, emit_debug: false })?
        },
        InputKind::SourcePath(p) => {
            anyhow::bail!("'run' attend un bytecode (ou activez --auto-compile) : {}", display(&p))
        },
        InputKind::SourceStdin => {
            anyhow::bail!("'run' attend un bytecode (ou activez --auto-compile) depuis stdin")
        },
    };

    let start = Instant::now();
    let code = runner(&bytes, &RunOptions { args: task.args, optimize: task.optimize })?;
    let elapsed = start.elapsed();

    if task.time {
        status_info("TIME", &format!("run: {} ms", elapsed.as_millis()));
    }
    Ok(code)
}

fn fmt_entry(task: FmtTask, hooks: &Hooks) -> Result<()> {
    let formatter = hooks
        .fmt
        .ok_or_else(|| anyhow!("Formatage indisponible (hook `fmt` manquant)"))?;

    let src = read_source(&task.input)?;
    let formatted = formatter(&src, task.check)?;

    match task.output {
        Output::Stdout => {
            let mut w = BufWriter::new(io::stdout().lock());
            w.write_all(formatted.as_bytes())?;
            w.flush()?;
        },
        Output::InPlace => match task.input {
            Input::Path(ref p) => write_text_atomic(p, &formatted)?,
            Input::Stdin => anyhow::bail!("--in-place nécessite un fichier d'entrée"),
        },
        Output::Path(ref p) => write_text_atomic(p, &formatted)?,
        Output::Auto => anyhow::bail!("Output::Auto n'est pas valide pour fmt"),
    }

    if task.check && src != formatted {
        anyhow::bail!("formatting diff: fichier non conforme (mode --check)");
    }

    status_ok("FMT", "ok");
    Ok(())
}

fn inspect_entry(task: InspectTask, hooks: &Hooks) -> Result<()> {
    let f = hooks
        .inspect
        .ok_or_else(|| anyhow!("Inspection indisponible (hook `inspect` manquant)"))?;
    let bytes = match task.input {
        InputKind::BytecodePath(p) => {
            fs::read(&p).with_context(|| format!("lecture bytecode: {}", display(&p)))?
        },
        InputKind::BytecodeBytes(b) => b,
        InputKind::SourcePath(p) => {
            anyhow::bail!("'inspect' attend un bytecode, pas une source : {}", display(&p))
        },
        InputKind::SourceStdin => anyhow::bail!("'inspect' attend un bytecode sur stdin"),
    };
    let text = f(&bytes, task.summary)?;
    let mut w = BufWriter::new(io::stdout().lock());
    w.write_all(text.as_bytes())?;
    w.flush()?;
    Ok(())
}

fn disasm_entry(task: DisasmTask, hooks: &Hooks) -> Result<()> {
    let f = hooks
        .disasm
        .ok_or_else(|| anyhow!("Désassemblage indisponible (hook `disasm` manquant)"))?;
    let bytes = match task.input {
        InputKind::BytecodePath(p) => {
            fs::read(&p).with_context(|| format!("lecture bytecode: {}", display(&p)))?
        },
        InputKind::BytecodeBytes(b) => b,
        InputKind::SourcePath(p) => {
            anyhow::bail!("'disasm' attend un bytecode, pas une source : {}", display(&p))
        },
        InputKind::SourceStdin => anyhow::bail!("'disasm' attend un bytecode sur stdin"),
    };
    let text = f(&bytes)?;
    match task.output {
        Output::Stdout => {
            let mut w = BufWriter::new(io::stdout().lock());
            w.write_all(text.as_bytes())?;
            w.flush()?;
        },
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
        Err(anyhow!(
            "Commande `modules` indisponible : recompile `vitte-cli` avec la feature `modules`."
        ))
    }

    #[cfg(feature = "modules")]
    {
        use ModulesFormat::*;

        let mods = vitte_modules::modules();
        match task.format {
            Table => {
                if mods.is_empty() {
                    println!("(aucun module activé — compile `vitte-modules` avec les features désirées)");
                } else {
                    for module in mods {
                        println!("{:<12} {}", module.name, module.description);
                        if !module.tags.is_empty() {
                            println!("    tags: {}", module.tags.join(", "));
                        }
                    }
                }
                Ok(())
            },
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
            },
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
        },
        Input::Path(p) => {
            let f = File::open(&p).with_context(|| format!("ouverture: {}", display(p)))?;
            let mut r = BufReader::new(f);
            let mut s = String::new();
            r.read_to_string(&mut s)?;
            Ok(s)
        },
    }
}

fn write_bytes_atomic(path: &Path, bytes: &[u8]) -> Result<()> {
    let parent = path
        .parent()
        .ok_or_else(|| anyhow!("chemin de sortie sans parent: {}", display(path)))?;
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
    fn fake_inspect(b: &[u8], _s: bool) -> Result<String> {
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
