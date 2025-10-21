use anyhow::{anyhow, bail, Context, Result};
use clap::{ArgAction, ColorChoice, Parser, Subcommand, ValueHint};
use env_logger;
use log::{debug, error, info};
use owo_colors::OwoColorize;
use serde::Deserialize;
use std::collections::BTreeMap;
use std::fs;
use std::io::{self, Read, Write};
use std::path::{Path, PathBuf};
use vitte_cli::{
    inspect::{render as render_inspection, InspectOptions},
    registry,
};

#[cfg(feature = "fmt")]
use vitte_fmt::{format_source, FormatterOptions};

#[derive(Debug, Parser)]
#[command(
    name = "vitte-bin",
    version,
    about = "Vitte CLI (binaire workspace)",
    color = ColorChoice::Auto
)]
struct Cli {
    /// Verbosité (-v, -vv, -vvv)
    #[arg(short, long, global = true, action = ArgAction::Count)]
    verbose: u8,
    /// Mode silencieux
    #[arg(short, long, global = true)]
    quiet: bool,
    /// Sous-commandes
    #[command(subcommand)]
    command: Command,
}

#[derive(Debug, Subcommand)]
enum Command {
    /// Exécute un fichier .vt, .vit, .vitte ou .vtbc
    Run {
        input: std::path::PathBuf,
        #[arg(trailing_var_arg = true)]
        args: Vec<String>,
    },
    /// Compile un fichier source
    Compile {
        input: std::path::PathBuf,
        #[arg(short, long)]
        out: Option<std::path::PathBuf>,
        #[arg(long, default_value = "bc")]
        emit: String,
    },
    /// Démarre un REPL interactif
    Repl,
    /// Formate un ou plusieurs fichiers source
    Fmt {
        #[arg(
            value_name = "PATH",
            value_hint = ValueHint::AnyPath,
            default_value = "."
        )]
        paths: Vec<PathBuf>,
        /// Mode vérification uniquement (aucune écriture)
        #[arg(long)]
        check: bool,
    },
    /// Inspecte un bytecode VITBC et affiche un résumé
    Inspect {
        #[arg(value_hint = ValueHint::FilePath)]
        input: Option<PathBuf>,
        /// Résumé succinct au lieu du rapport complet
        #[arg(long)]
        summary: bool,
        /// Table des symboles
        #[arg(long)]
        symbols: bool,
        /// Constantes globales
        #[arg(long = "consts")]
        consts_only: bool,
        /// Dump hexadécimal
        #[arg(long)]
        hexdump: bool,
        /// Sortie JSON
        #[arg(long)]
        json: bool,
        /// Informations cible
        #[arg(long = "target")]
        target_info: bool,
        /// Informations d'en-tête
        #[arg(long)]
        header: bool,
        /// Liste les sections présentes
        #[arg(long)]
        sections: bool,
        /// Chaînes ASCII embarquées
        #[arg(long)]
        strings: bool,
        /// Fonctions importées
        #[arg(long)]
        imports: bool,
        /// Fonctions exportées
        #[arg(long)]
        exports: bool,
        /// Désassemblage lisible
        #[arg(long)]
        disasm: bool,
        /// Informations de debug
        #[arg(long)]
        debug: bool,
        /// Métadonnées avancées
        #[arg(long)]
        meta: bool,
        /// Vérification d'intégrité
        #[arg(long)]
        verify: bool,
        /// Taille par section
        #[arg(long)]
        size: bool,
        /// Dépendances de modules
        #[arg(long)]
        deps: bool,
        /// Point d'entrée
        #[arg(long)]
        entry: bool,
        /// Active toutes les sections
        #[arg(long = "dump-all")]
        dump_all: bool,
    },
    /// Affiche la version détaillée
    Version,
    /// Gère les dépendances du projet
    Deps {
        #[command(subcommand)]
        action: Option<DepsAction>,
    },
}

#[derive(Debug, Subcommand, Clone, Copy)]
enum DepsAction {
    /// Liste les dépendances déclarées dans vitte.toml
    List,
    /// Synchronise les modules depuis vitte.org
    Sync,
}

fn init_logger(verbosity: u8, quiet: bool) {
    let level = if quiet {
        "off"
    } else {
        match verbosity {
            0 => "info",
            1 => "debug",
            _ => "trace",
        }
    };
    if std::env::var_os("RUST_LOG").is_none() {
        std::env::set_var("RUST_LOG", level);
    }
    let _ = env_logger::Builder::from_env(env_logger::Env::default().default_filter_or(level))
        .format_timestamp_millis()
        .try_init();
}

pub fn run_cli() -> Result<()> {
    let cli = Cli::parse();
    init_logger(cli.verbose, cli.quiet);
    if let Err(err) = real_main(cli) {
        error!("{err:#}");
        return Err(err);
    }
    Ok(())
}

fn real_main(cli: Cli) -> Result<()> {
    debug!("args = {:?}", std::env::args().collect::<Vec<_>>());
    let command = cli.command;
    let config = if matches!(command, Command::Version) { None } else { check_project_config()? };

    match command {
        Command::Version => println!("{}", crate::version_string()),
        Command::Run { input, args } => crate::engine_run(&input, &args)?,
        Command::Compile { input, out, emit } => {
            crate::engine_compile(&input, out.as_deref(), &emit)?
        }
        Command::Repl => crate::engine_repl()?,
        Command::Fmt { paths, check } => crate::command_fmt(&paths, check)?,
        Command::Inspect {
            input,
            summary,
            symbols,
            consts_only,
            hexdump,
            json,
            target_info,
            header,
            sections,
            strings,
            imports,
            exports,
            disasm,
            debug,
            meta,
            verify,
            size,
            deps,
            entry,
            dump_all,
        } => {
            let mut options = InspectOptions {
                summary,
                header,
                symbols,
                sections,
                consts: consts_only,
                strings,
                imports,
                exports,
                hexdump,
                disasm,
                json,
                target: target_info,
                debug,
                meta,
                verify,
                size,
                deps,
                entry,
                dump_all,
            };
            options.ensure_defaults();
            crate::command_inspect(input.as_ref(), options)?
        }
        Command::Deps { action } => {
            handle_deps(action.unwrap_or(DepsAction::List), config.as_ref())?
        }
    }
    Ok(())
}

pub fn version_string() -> String {
    format!(
        "Vitte CLI {}\ncommit: {}\nbuild: {}\nrustc: {}",
        env!("CARGO_PKG_VERSION"),
        option_env!("GIT_COMMIT").unwrap_or("unknown"),
        option_env!("VERGEN_BUILD_DATE").unwrap_or("unknown"),
        option_env!("VERGEN_RUSTC_SEMVER").unwrap_or("unknown"),
    )
}

pub fn engine_repl() -> Result<()> {
    repl_loop()
}

pub fn engine_run(input: &Path, args: &[String]) -> Result<()> {
    run_program(input, args)
}

pub fn engine_compile(input: &Path, out: Option<&Path>, emit: &str) -> Result<()> {
    let out_buf: Option<PathBuf> = out.map(|p| p.to_path_buf());
    compile_file(input, out_buf.as_ref(), emit)
}

pub fn resolve_output(input: &Path, emit: &str) -> PathBuf {
    match emit {
        "obj" => input.with_extension("o"),
        "exe" => {
            if cfg!(windows) {
                input.with_extension("exe")
            } else {
                input.with_extension("")
            }
        }
        _ => input.with_extension("vtbc"),
    }
}

pub fn command_fmt(paths: &[PathBuf], check: bool) -> Result<()> {
    #[cfg(feature = "fmt")]
    {
        command_fmt_impl(paths, check)
    }

    #[cfg(not(feature = "fmt"))]
    {
        let _ = (paths, check);
        bail!(
            "Formatage indisponible : recompilez avec `--features fmt` (ou `--features engine`)."
        );
    }
}

#[cfg(feature = "fmt")]
fn command_fmt_impl(paths: &[PathBuf], check: bool) -> Result<()> {
    let defaults: Vec<PathBuf> =
        if paths.is_empty() { vec![PathBuf::from(".")] } else { paths.to_vec() };

    let files = collect_fmt_targets(&defaults)?;

    if files.is_empty() {
        print_line("Aucun fichier Vitte (.vit, .vitte) trouvé.", MessageColor::Yellow);
        return Ok(());
    }

    let mut modified = Vec::new();
    let options = FormatterOptions::default();

    for path in &files {
        let original = fs::read_to_string(path)
            .with_context(|| format!("Impossible de lire {}", path.display()))?;
        let output = format_source(&original, options);
        let new_code = output.code;
        let changed = new_code != original;

        if check {
            if changed {
                modified.push(path.clone());
            }
        } else if changed {
            fs::write(path, new_code)
                .with_context(|| format!("Impossible d'écrire {}", path.display()))?;
            modified.push(path.clone());
        }
    }

    if check {
        if modified.is_empty() {
            let msg = format!("OK : code déjà formaté ({} fichier(s) vérifiés).", files.len());
            print_line(&msg, MessageColor::Green);
            Ok(())
        } else {
            print_line("Attention : des fichiers nécessitent un formatage :", MessageColor::Yellow);
            for path in &modified {
                let entry = format!("  - {}", path.display());
                print_line(&entry, MessageColor::Red);
            }
            bail!("{} fichier(s) non formaté(s)", modified.len())
        }
    } else {
        if modified.is_empty() {
            let msg = format!("Aucun changement ({} fichier(s) examinés).", files.len());
            print_line(&msg, MessageColor::Blue);
        } else {
            let msg = format!(
                "Formatage appliqué sur {} fichier(s) ({} au total).",
                modified.len(),
                files.len()
            );
            print_line(&msg, MessageColor::Green);
        }
        Ok(())
    }
}

#[cfg(feature = "fmt")]
fn collect_fmt_targets(inputs: &[PathBuf]) -> Result<Vec<PathBuf>> {
    let mut stack: Vec<PathBuf> = inputs
        .iter()
        .map(|p| if p.as_os_str().is_empty() { PathBuf::from(".") } else { p.clone() })
        .collect();
    let mut out = Vec::new();

    while let Some(path) = stack.pop() {
        let meta = match fs::metadata(&path) {
            Ok(m) => m,
            Err(err) => {
                print_diagnostic(
                    Diagnostic::warning(
                        "FMT001",
                        format!("Impossible de lire les métadonnées de {}", path.display()),
                    )
                    .with_help(err.to_string()),
                );
                continue;
            }
        };

        if meta.is_dir() {
            let entries = match fs::read_dir(&path) {
                Ok(e) => e,
                Err(err) => {
                    print_diagnostic(
                        Diagnostic::warning(
                            "FMT002",
                            format!("Lecture du dossier {} impossible", path.display()),
                        )
                        .with_help(err.to_string()),
                    );
                    continue;
                }
            };

            for entry in entries {
                let entry = match entry {
                    Ok(e) => e,
                    Err(err) => {
                        print_diagnostic(
                            Diagnostic::warning("FMT003", "Entrée illisible")
                                .with_help(err.to_string()),
                        );
                        continue;
                    }
                };
                let child = entry.path();
                let ftype = match entry.file_type() {
                    Ok(t) => t,
                    Err(err) => {
                        print_diagnostic(
                            Diagnostic::error(
                                "FMT004",
                                format!("Type inconnu pour {}", child.display()),
                            )
                            .with_help(err.to_string()),
                        );
                        continue;
                    }
                };
                if ftype.is_dir() {
                    if let Some(name) = child.file_name().and_then(|s| s.to_str()) {
                        if should_skip_directory(name) {
                            continue;
                        }
                    }
                    stack.push(child);
                } else if ftype.is_file() && is_vitte_source(&child) {
                    out.push(child);
                }
            }
        } else if meta.is_file() && is_vitte_source(&path) {
            out.push(path);
        }
    }

    out.sort();
    out.dedup();
    Ok(out)
}

#[cfg(feature = "fmt")]
fn is_vitte_source(path: &Path) -> bool {
    match path.extension().and_then(|ext| ext.to_str()) {
        Some(ext) => matches!(ext, "vit" | "vitte" | "vt"),
        None => false,
    }
}

#[cfg(feature = "fmt")]
fn should_skip_directory(name: &str) -> bool {
    matches!(name, "target" | ".git" | ".idea" | ".vscode" | "node_modules")
}

pub fn command_inspect(path: Option<&PathBuf>, mut options: InspectOptions) -> Result<()> {
    let mut bytes = Vec::new();
    match path {
        Some(p) if p.as_os_str() != "-" => {
            bytes = fs::read(p).with_context(|| format!("Impossible de lire {}", p.display()))?;
        }
        Some(_) | None => {
            io::stdin()
                .lock()
                .read_to_end(&mut bytes)
                .context("Impossible de lire le flux stdin")?;
        }
    }

    if bytes.is_empty() {
        emit_and_bail(
            Diagnostic::error("INS000", "Aucune donnée fournie à inspecter")
                .with_help("Passez un fichier .vitbc ou pipez le bytecode sur stdin."),
        )?;
    }

    options.ensure_defaults();
    let report = render_inspection(&bytes, &options);
    println!("{}", report);
    Ok(())
}

fn handle_deps(action: DepsAction, cfg: Option<&VitteConfig>) -> Result<()> {
    let Some(cfg) = cfg else {
        emit_and_bail(
            Diagnostic::error("CFG100", "Aucun fichier vitte.toml trouvé")
                .with_note("Les commandes de dépendances requièrent un projet initialisé.")
                .with_help("Créez vitte.toml à la racine ou exécutez `vitte init`."),
        )?;
        unreachable!()
    };

    match action {
        DepsAction::List => {
            if cfg.dependencies.entries.is_empty() {
                print_line("Aucune dépendance déclarée.", MessageColor::Yellow);
            } else {
                print_line("Dépendances déclarées :", MessageColor::Blue);
                for (name, spec) in &cfg.dependencies.entries {
                    let mut details = Vec::new();
                    if let Some(module) = &spec.module {
                        details.push(format!("module={module}"));
                    }
                    if let Some(version) = &spec.version {
                        details.push(format!("version={version}"));
                    }
                    if let Some(git) = &spec.git {
                        details.push(format!("git={git}"));
                    }
                    if let Some(path) = &spec.path {
                        details.push(format!("path={path}"));
                    }
                    let line = if details.is_empty() {
                        format!("  • {name}")
                    } else {
                        format!("  • {name} ({})", details.join(", "))
                    };
                    print_line(&line, MessageColor::Green);
                }
            }
        }
        DepsAction::Sync => {
            print_line("Synchronisation des modules Vitte…", MessageColor::Blue);
            let registry_modules = match registry::load_local_index(Path::new(".")) {
                Ok(mods) => mods,
                Err(err) => {
                    print_line(
                        &format!("warning[DEP200]: registre local introuvable ({err})"),
                        MessageColor::Yellow,
                    );
                    print_line(
                        "Créez `registry/modules/index.json` ou consultez https://vitte.org/modules.",
                        MessageColor::Help,
                    );
                    return Ok(());
                }
            };

            let mut matched = 0usize;
            for (name, spec) in &cfg.dependencies.entries {
                let module_name = spec.module.as_deref().unwrap_or(name);
                if let Some(entry) = registry::find(&registry_modules, module_name) {
                    let current = &entry.version;
                    if let Some(requested) = &spec.version {
                        if requested == current {
                            print_line(
                                &format!("  ✓ {module_name}@{current} (ok)"),
                                MessageColor::Green,
                            );
                            matched += 1;
                        } else {
                            print_line(
                                &format!(
                                    "  ⚠ {module_name} : demandé {requested}, disponible {current}"
                                ),
                                MessageColor::Yellow,
                            );
                        }
                    } else {
                        print_line(
                            &format!("  ✓ {module_name}@{current} (ok)"),
                            MessageColor::Green,
                        );
                        matched += 1;
                    }
                    if let Some(url) = &entry.url {
                        print_line(&format!("    url    : {url}"), MessageColor::Note);
                    }
                    if let Some(sum) = &entry.checksum {
                        print_line(&format!("    sha256 : {sum}"), MessageColor::Note);
                    }
                } else {
                    print_line(
                        &format!("  ✗ {module_name} introuvable dans le registre"),
                        MessageColor::Red,
                    );
                }
            }

            if matched == cfg.dependencies.entries.len() {
                print_line("Synchronisation terminée : tout est à jour.", MessageColor::Blue);
            } else {
                print_line(
                    "Synchronisation partielle : certains modules manquent ou diffèrent.",
                    MessageColor::Yellow,
                );
            }
        }
    }

    Ok(())
}

fn check_project_config() -> Result<Option<VitteConfig>> {
    let path = Path::new("vitte.toml");
    if !path.exists() {
        return Ok(None);
    }

    let content =
        fs::read_to_string(path).with_context(|| format!("Lecture de {}", path.display()))?;

    let cfg: VitteConfig = match toml::from_str(&content) {
        Ok(cfg) => cfg,
        Err(err) => {
            emit_and_bail(
                Diagnostic::error(
                    "CFG000",
                    format!("Impossible de parser {} : {}", path.display(), err),
                )
                .with_note("Le fichier doit respecter le format TOML 1.0.")
                .with_help("Vérifie la syntaxe TOML de vitte.toml."),
            )?;
            unreachable!()
        }
    };

    validate_config(&cfg)?;
    Ok(Some(cfg))
}

fn validate_config(cfg: &VitteConfig) -> Result<()> {
    if cfg.project.name.trim().is_empty() {
        emit_and_bail(
            Diagnostic::error("CFG001", "`[project].name` ne peut pas être vide")
                .with_help("Ajoute un nom de projet dans vitte.toml."),
        )?;
    }

    if let Some(license) = &cfg.project.license {
        if license.trim().is_empty() {
            emit_and_bail(
                Diagnostic::error("CFG002", "`[project].license` ne peut pas être vide")
                    .with_help("Spécifie une licence SPDX (ex: \"Apache-2.0\")."),
            )?;
        }
    }

    if let Some(description) = &cfg.project.description {
        if description.trim().is_empty() {
            emit_and_bail(
                Diagnostic::error("CFG003", "`[project].description` ne peut pas être vide")
                    .with_help("Ajoute une courte description du projet."),
            )?;
        }
    }

    if let Some(build) = &cfg.build {
        if let Some(rust) = &build.rust_version {
            if rust.trim().is_empty() {
                emit_and_bail(
                    Diagnostic::error("CFG010", "`[build].rust_version` ne peut pas être vide")
                        .with_help("Indique la version minimale de rustc attendue."),
                )?;
            }
        }

        if let Some(targets) = &build.targets {
            if targets.is_empty() {
                emit_and_bail(
                    Diagnostic::error(
                        "CFG011",
                        "`[build].targets` doit contenir au moins une cible",
                    )
                    .with_help("Liste les triples (ex: \"x86_64-pc-windows-msvc\")."),
                )?;
            }
            for target in targets {
                if target.trim().is_empty() {
                    emit_and_bail(Diagnostic::error(
                        "CFG012",
                        "`[build].targets` contient une entrée vide",
                    ))?;
                }
            }
        }

        if let Some(features) = &build.features {
            if features.iter().any(|f| f.trim().is_empty()) {
                emit_and_bail(Diagnostic::error(
                    "CFG013",
                    "`[build].features` contient une entrée vide",
                ))?;
            }
        }
    }

    Ok(())
}

#[derive(Copy, Clone)]
enum MessageColor {
    Red,
    Yellow,
    Green,
    Blue,
    Note,
    Help,
}

fn print_line(message: &str, color: MessageColor) {
    match color {
        MessageColor::Red => println!("{}", message.red().bold()),
        MessageColor::Yellow => println!("{}", message.yellow().bold()),
        MessageColor::Green => println!("{}", message.green().bold()),
        MessageColor::Blue => println!("{}", message.blue().bold()),
        MessageColor::Note => println!("{}", message.blue()),
        MessageColor::Help => println!("{}", message.green()),
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

    fn with_note(mut self, note: impl Into<String>) -> Self {
        self.notes.push(note.into());
        self
    }

    fn with_help(mut self, help: impl Into<String>) -> Self {
        self.help = Some(help.into());
        self
    }

    fn emit(&self) {
        let color = match self.severity {
            Severity::Error => MessageColor::Red,
            Severity::Warning => MessageColor::Yellow,
        };
        let label = match self.severity {
            Severity::Error => "error",
            Severity::Warning => "warning",
        };
        print_line(
            &format!("{label}[{code}]: {msg}", label = label, code = self.code, msg = self.message),
            color,
        );
        for note in &self.notes {
            print_line(&format!("  note: {note}"), MessageColor::Note);
        }
        if let Some(help) = &self.help {
            print_line(&format!("  help: {help}"), MessageColor::Help);
        }
    }
}

fn emit_and_bail(diag: Diagnostic) -> Result<()> {
    let message = diag.message.clone();
    diag.emit();
    match diag.severity {
        Severity::Error => bail!(message),
        Severity::Warning => Err(anyhow!(message)),
    }
}

fn print_diagnostic(diag: Diagnostic) {
    diag.emit();
}

#[derive(Debug, Deserialize)]
struct VitteConfig {
    project: ProjectSection,
    #[serde(default)]
    build: Option<BuildSection>,
    #[serde(default)]
    dependencies: DependencySet,
}

#[derive(Debug, Deserialize)]
struct ProjectSection {
    name: String,
    #[serde(default)]
    license: Option<String>,
    #[serde(default)]
    description: Option<String>,
}

#[derive(Debug, Deserialize)]
struct BuildSection {
    #[serde(default)]
    rust_version: Option<String>,
    #[serde(default)]
    targets: Option<Vec<String>>,
    #[serde(default)]
    features: Option<Vec<String>>,
}

#[derive(Debug, Default, Deserialize)]
struct DependencySet {
    #[serde(flatten)]
    entries: BTreeMap<String, DependencySpec>,
}

#[derive(Debug, Deserialize)]
struct DependencySpec {
    #[serde(default)]
    version: Option<String>,
    #[serde(default)]
    module: Option<String>,
    #[serde(default)]
    git: Option<String>,
    #[serde(default)]
    path: Option<String>,
}

fn run_program(input: &Path, args: &[String]) -> Result<()> {
    info!("▶️  run: {}", input.display());
    if is_bytecode(input) {
        engine::vm::run_bc(input, args)?;
    } else {
        engine::vm::run_source(input, args)?;
    }
    Ok(())
}

fn compile_file(input: &Path, out: Option<&PathBuf>, emit: &str) -> Result<()> {
    info!("🧩 Compilation {} (emit={})", input.display(), emit);
    engine::compiler::compile_file(input, out.map(|p| p.as_path()), 2, None, None, emit)
}

fn repl_loop() -> Result<()> {
    info!("💬 REPL démarré");
    let mut line = String::new();
    let mut stdout = io::stdout();
    loop {
        print!("vitte> ");
        stdout.flush().ok();
        line.clear();
        if io::stdin().read_line(&mut line)? == 0 {
            break;
        }
        let s = line.trim_end();
        if matches!(s, ":quit" | ":exit") {
            break;
        }
        match engine::vm::eval_line(s) {
            Ok(res) => println!("= {}", res),
            Err(e) => println!("! erreur: {e}"),
        }
    }
    Ok(())
}

pub fn is_bytecode(path: &Path) -> bool {
    matches!(path.extension().and_then(|e| e.to_str()), Some("vtbc") | Some("bc"))
}

#[cfg(feature = "engine")]
pub mod engine {
    use anyhow::{bail, Context, Result};
    use log::{debug, info};
    use std::fs;
    use std::path::{Path, PathBuf};
    use vitte_cli::{self, CompileOptions as CliCompileOptions, RunOptions as CliRunOptions};

    pub mod vm {
        use super::*;

        pub fn run_bc(input: &Path, args: &[String]) -> Result<()> {
            info!("[engine/vm] run_bc: {} {:?}", input.display(), args);
            let bytes = fs::read(input)
                .with_context(|| format!("impossible de lire {}", input.display()))?;
            let exit = vitte_cli::run_bytecode(
                &bytes,
                &CliRunOptions { args: args.to_vec(), optimize: false },
            )?;
            if exit != 0 {
                bail!("exécution terminée avec le code {}", exit);
            }
            Ok(())
        }

        pub fn run_source(input: &Path, args: &[String]) -> Result<()> {
            info!("[engine/vm] run_source: {} {:?}", input.display(), args);
            let source = fs::read_to_string(input)
                .with_context(|| format!("impossible de lire {}", input.display()))?;
            debug!("[engine] source size = {} bytes", source.len());

            let bytes = vitte_cli::compile_source_to_bytes(
                &source,
                &CliCompileOptions { optimize: true, emit_debug: false },
            )?;

            let exit = vitte_cli::run_bytecode(
                &bytes,
                &CliRunOptions { args: args.to_vec(), optimize: true },
            )?;
            if exit != 0 {
                bail!("exécution terminée avec le code {}", exit);
            }
            Ok(())
        }

        pub fn eval_line(s: &str) -> Result<String> {
            Ok(s.trim().to_string())
        }
    }

    pub mod compiler {
        use super::*;

        pub fn compile_file(
            input: &Path,
            out: Option<&Path>,
            opt_level: u8,
            _target: Option<&str>,
            _threads: Option<u32>,
            emit: &str,
        ) -> Result<()> {
            info!("[engine/compiler] compile: {} (emit={})", input.display(), emit);

            if !matches!(emit, "bc" | "vitbc") {
                bail!("format d'émission '{emit}' non supporté (bc uniquement pour l'instant)");
            }

            let out_path: PathBuf =
                out.map(PathBuf::from).unwrap_or_else(|| super::super::resolve_output(input, emit));

            if let Some(parent) = out_path.parent() {
                if !parent.exists() {
                    fs::create_dir_all(parent)
                        .with_context(|| format!("impossible de créer {}", parent.display()))?;
                }
            }

            let source = fs::read_to_string(input)
                .with_context(|| format!("impossible de lire {}", input.display()))?;

            let bytes = vitte_cli::compile_source_to_bytes(
                &source,
                &CliCompileOptions { optimize: opt_level > 0, emit_debug: false },
            )?;

            fs::write(&out_path, &bytes)
                .with_context(|| format!("impossible d'écrire {}", out_path.display()))?;
            println!("[engine] → {}", out_path.display());
            Ok(())
        }
    }
}

#[cfg(not(feature = "engine"))]
pub mod engine {
    use anyhow::{bail, Result};
    use std::path::Path;

    pub mod vm {
        use super::*;

        pub fn run_bc(_input: &Path, _args: &[String]) -> Result<()> {
            bail!("VM indisponible : recompilez avec `--features engine`");
        }

        pub fn run_source(_input: &Path, _args: &[String]) -> Result<()> {
            bail!("Compilation+VM indisponibles : recompilez avec `--features engine`");
        }

        pub fn eval_line(s: &str) -> Result<String> {
            Ok(s.trim().to_string())
        }
    }

    pub mod compiler {
        use super::*;

        pub fn compile_file(
            input: &Path,
            _out: Option<&Path>,
            _opt_level: u8,
            _target: Option<&str>,
            _threads: Option<u32>,
            _emit: &str,
        ) -> Result<()> {
            bail!(
                "Compilation indisponible pour {} : recompilez avec `--features engine`",
                input.display()
            );
        }
    }
}
