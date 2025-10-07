use anyhow::{anyhow, Result};
use clap::{value_parser, ArgAction, ColorChoice, Parser, Subcommand, ValueEnum};
use colored::Colorize;
use env_logger;
use log::{debug, error, info};
use notify::{Config, Event, EventKind, RecommendedWatcher, RecursiveMode, Watcher};
use serde::Serialize;
use serde_json;
use std::path::{Component, Path, PathBuf};
use std::process::Command as ProcCommand;
use std::sync::mpsc::channel;
use std::time::{Duration, Instant};

/// Format de sortie pour certaines commandes
#[derive(Copy, Clone, Debug, Eq, PartialEq, ValueEnum)]
pub enum Format {
    Text,
    Json,
}

/// Type d'artefact à émettre côté compile
#[derive(Copy, Clone, Debug, Eq, PartialEq, ValueEnum)]
pub enum Emit {
    Bc,
    Obj,
    Exe,
}

impl Emit {
    fn as_str(self) -> &'static str {
        match self {
            Emit::Bc => "bc",
            Emit::Obj => "obj",
            Emit::Exe => "exe",
        }
    }
}

#[derive(Debug, Parser)]
#[command(
    name = "vitte-bin",
    version,
    about = "Vitte CLI (binaire workspace) — run | compile | repl | version",
    long_about = "Binaire d'orchestration Vitte. Délègue au moteur quand disponible, sinon fallback simulé.",
    color = ColorChoice::Auto
)]
struct Cli {
    /// Verbosité (-v, -vv, -vvv)
    #[arg(short, long, global = true, action = ArgAction::Count)]
    verbose: u8,

    /// Mode silencieux (équivaut à RUST_LOG=off)
    #[arg(short, long, global = true)]
    quiet: bool,

    /// Format de sortie pour certaines commandes
    #[arg(long, global = true, value_enum, default_value = "text")]
    format: Format,

    /// Force la détection de couleur (auto|always|never)
    #[arg(long, value_enum, default_value = "auto")]
    color: ColorChoice,

    #[command(subcommand)]
    command: Command,
}

#[derive(Debug, Subcommand)]
enum Command {
    /// Exécute un fichier .vt, .vit, .vitte ou .vtbc
    Run {
        /// Fichier d'entrée
        input: PathBuf,
        /// Arguments passés au programme
        #[arg(trailing_var_arg = true)]
        args: Vec<String>,
    },

    /// Compile un fichier source
    Compile {
        /// Fichier source
        input: PathBuf,
        /// Fichier de sortie (-o)
        #[arg(short, long)]
        out: Option<PathBuf>,
        /// Type d'artefact à émettre
        #[arg(long, value_enum, default_value_t = Emit::Bc)]
        emit: Emit,
        /// Triple de compilation cible (ex: x86_64-unknown-linux-gnu)
        #[arg(long)]
        target: Option<String>,
        /// Niveau d'optimisation (0-3, s, z)
        #[arg(short = 'O', long)]
        opt: Option<String>,
        /// Threads utilisés par le compilateur
        #[arg(long, value_parser = value_parser!(u32).range(1..=256))]
        threads: Option<u32>,
    },

    /// Démarre un REPL interactif
    Repl,

    /// Affiche la version détaillée
    Version,

    /// Affiche des infos d'environnement utiles (cibles, moteur, build)
    Info,

    /// Vérifie la configuration locale (fmt, lint, check, etc.)
    Doctor {
        /// Installe un hook Git pre-commit qui lance `vitte-bin doctor`
        #[arg(long = "install-hook")]
        install_hook: bool,
        /// Active le mode veille : surveille les fichiers et relance en continu
        #[arg(long)]
        watch: bool,
        /// Corrige automatiquement formatage et lint simples
        #[arg(long)]
        fix: bool,
    },
}

fn init_logger(verbosity: u8, quiet: bool, color: ColorChoice) {
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
    // Respect de --color
    std::env::set_var(
        "RUST_LOG_STYLE",
        match color {
            ColorChoice::Always => "always",
            ColorChoice::Never => "never",
            _ => "auto",
        },
    );

    let _ = env_logger::Builder::from_env(env_logger::Env::default().default_filter_or(level))
        .format_timestamp_millis()
        .try_init();
}

fn main() {
    let cli = Cli::parse();
    init_logger(cli.verbose, cli.quiet, cli.color);
    if let Err(e) = real_main(cli) {
        error!("{e:#}");
        std::process::exit(1);
    }
}

fn real_main(cli: Cli) -> Result<()> {
    debug!("args = {:?}", std::env::args().collect::<Vec<_>>());
    match cli.command {
        Command::Version => println!("{}", vitte_bin::version_string()),
        Command::Run { input, args } => cmd_run(&input, &args, cli.format)?,
        Command::Compile { input, out, emit, target, opt, threads } => cmd_compile(
            &input,
            out.as_deref(),
            emit,
            target.as_deref(),
            opt.as_deref(),
            threads,
            cli.format,
        )?,
        Command::Repl => vitte_bin::engine_repl()?,
        Command::Info => cmd_info(cli.format)?,
        Command::Doctor { install_hook, watch, fix } => {
            cmd_doctor(cli.format, install_hook, watch, fix)?
        },
    }
    Ok(())
}

/* -------------------------------------------------------------------------- */
/* Implémentations des commandes                                              */
/* -------------------------------------------------------------------------- */

fn ensure_exists(path: &Path) -> Result<()> {
    anyhow::ensure!(path.exists(), "fichier introuvable: {}", path.display());
    Ok(())
}

fn cmd_run(input: &Path, args: &[String], format: Format) -> Result<()> {
    ensure_exists(input)?;
    info!("▶️  run: {}", input.display());

    // Détection (utile pour logs uniquement, la lib gère fallback/engine)
    let kind = if vitte_bin::is_bytecode(input) { "bytecode" } else { "source" };
    debug!("kind = {}", kind);

    vitte_bin::engine_run(input, args)?;

    if matches!(format, Format::Json) {
        println!(
            "{{\"status\":\"ok\",\"action\":\"run\",\"input\":\"{}\"}}",
            escape_json(input.display())
        );
    }
    Ok(())
}

fn cmd_compile(
    input: &Path,
    out: Option<&Path>,
    emit: Emit,
    target: Option<&str>,
    opt: Option<&str>,
    threads: Option<u32>,
    format: Format,
) -> Result<()> {
    ensure_exists(input)?;
    info!("🛠️  compile: {}", input.display());

    if let Some(t) = target {
        debug!("target = {}", t);
    }
    if let Some(o) = opt {
        debug!("opt = {}", o);
    }
    if let Some(th) = threads {
        debug!("threads = {}", th);
    }

    // La lib gère la délégation moteur ou fallback (copie)
    vitte_bin::engine_compile(input, out, emit.as_str())?;

    let out_path = out
        .map(|p| p.to_path_buf())
        .unwrap_or_else(|| vitte_bin::resolve_output(input, emit.as_str()));

    match format {
        Format::Text => println!("→ {}", out_path.display()),
        Format::Json => println!(
            "{{\"status\":\"ok\",\"action\":\"compile\",\"input\":\"{}\",\"output\":\"{}\",\"emit\":\"{}\"}}",
            escape_json(input.display()),
            escape_json(out_path.display()),
            emit.as_str()
        ),
    }
    Ok(())
}

fn cmd_info(format: Format) -> Result<()> {
    let version = vitte_bin::version_string();
    let target = std::env::var("TARGET").ok();
    let rustc = option_env!("RUSTC_WRAPPER").unwrap_or("rustc");
    let engine = which_engine();

    match format {
        Format::Text => {
            println!("{}", version);
            println!("engine: {}", engine.unwrap_or_else(|| "(absent)".into()));
            if let Some(t) = target {
                println!("target: {}", t);
            }
            println!("rustc: {}", rustc);
        },
        Format::Json => {
            println!(
                "{{\"version\":\"{}\",\"engine\":\"{}\",\"target\":\"{}\",\"rustc\":\"{}\"}}",
                escape_json(version),
                escape_json(engine.unwrap_or_else(|| "(absent)".into())),
                escape_json(target.unwrap_or_else(|| "".into())),
                rustc
            );
        },
    }
    Ok(())
}

fn cmd_doctor(format: Format, install_hook: bool, watch: bool, fix: bool) -> Result<()> {
    if watch && matches!(format, Format::Json) {
        anyhow::bail!("`--watch` ne peut pas être combiné avec `--format json`");
    }

    if watch {
        if install_hook {
            install_pre_commit_hook()?;
        }
        run_doctor_watch()?;
        return Ok(());
    }

    let mut plan = DoctorPlan::default();
    let mut report = collect_doctor_report(&mut plan);

    if fix {
        apply_fixes(&plan)?;
        report = collect_doctor_report(&mut plan);
    }

    if install_hook {
        install_pre_commit_hook()?;
        report.suggestions.push("Hook pré-commit installé : `.git/hooks/pre-commit`".to_string());
    }

    emit_report(&report, format)?;

    if doctor_failed(&report) {
        Err(anyhow!("vitte doctor: des vérifications ont échoué"))
    } else {
        Ok(())
    }
}

#[derive(Debug, Serialize)]
struct CheckResult {
    name: String,
    status: CheckStatus,
    duration_ms: u128,
    suggestion: Option<String>,
    details: String,
}

#[derive(Debug, Serialize, Clone, Copy, PartialEq, Eq)]
enum CheckStatus {
    Success,
    Failure,
}

impl CheckStatus {
    fn colored_label(self) -> colored::ColoredString {
        match self {
            CheckStatus::Success => "OK".green().bold(),
            CheckStatus::Failure => "FAIL".red().bold(),
        }
    }
}

#[derive(Debug, Serialize)]
struct DoctorReport {
    results: Vec<CheckResult>,
    suggestions: Vec<String>,
}

#[derive(Default)]
struct DoctorPlan {
    fmt_failed: bool,
    lint_failed: bool,
}

fn collect_doctor_report(plan: &mut DoctorPlan) -> DoctorReport {
    let mut results = Vec::new();

    let fmt_res = run_check(
        "Format",
        "cargo",
        &["fmt", "--", "--check"],
        Some("Exécute `vitte fmt` pour corriger automatiquement."),
    );
    plan.fmt_failed = fmt_res.status == CheckStatus::Failure;
    results.push(fmt_res);

    let lint_res = run_check(
        "Lint",
        "cargo",
        &["clippy", "--workspace", "--all-targets", "--", "-D", "warnings"],
        Some("Corrige les warnings puis relance `cargo clippy --all-targets`."),
    );
    plan.lint_failed = lint_res.status == CheckStatus::Failure;
    results.push(lint_res);

    results.push(run_check(
        "Diagnostics",
        "cargo",
        &["check", "--workspace", "--all-targets"],
        Some("Corrige les erreurs listées puis relance `cargo check`."),
    ));

    let suggestions = results
        .iter()
        .filter_map(|res| (res.status == CheckStatus::Failure).then(|| res.suggestion.clone()))
        .flatten()
        .collect();

    DoctorReport { results, suggestions }
}

fn apply_fixes(plan: &DoctorPlan) -> Result<()> {
    if plan.fmt_failed {
        println!("{} Formatage…", "↺".blue());
        let status = ProcCommand::new("cargo").args(["fmt"]).status()?;
        if !status.success() {
            return Err(anyhow!("`cargo fmt` a échoué"));
        }
    }

    if plan.lint_failed {
        println!("{} Clippy (quick fix)…", "↺".blue());
        let status = ProcCommand::new("cargo")
            .args(["clippy", "--workspace", "--all-targets", "--", "--fix", "-Zunstable-options", "-A", "warnings"])
            .status()?;
        if !status.success() {
            println!("{} Clippy fix partiel : revois les warnings restants.", "!".yellow());
        }
    }

    Ok(())
}

fn run_check(name: &str, program: &str, args: &[&str], suggestion: Option<&str>) -> CheckResult {
    let start = Instant::now();
    let output = ProcCommand::new(program)
        .args(args)
        .output()
        .map_err(|e| e.to_string());

    match output {
        Ok(out) => {
            let duration_ms = start.elapsed().as_millis();
            let success = out.status.success();
            let stdout = String::from_utf8_lossy(&out.stdout);
            let stderr = String::from_utf8_lossy(&out.stderr);
            let mut details = String::new();
            if !stdout.trim().is_empty() {
                details.push_str(stdout.trim());
                details.push('\n');
            }
            if !stderr.trim().is_empty() {
                details.push_str(stderr.trim());
            }
            let status = if success {
                CheckStatus::Success
            } else {
                CheckStatus::Failure
            };
            CheckResult {
                name: name.to_string(),
                status,
                duration_ms,
                suggestion: suggestion.map(|s| s.to_string()),
                details: details.trim().to_string(),
            }
        },
        Err(err) => CheckResult {
            name: name.to_string(),
            status: CheckStatus::Failure,
            duration_ms: start.elapsed().as_millis(),
            suggestion: Some(format!(
                "Vérifie que `{}` est disponible dans le PATH avant de relancer.",
                program
            )),
            details: err,
        },
    }
}

fn emit_report(report: &DoctorReport, format: Format) -> Result<()> {
    match format {
        Format::Text => print_human_report(report),
        Format::Json => {
            let json = serde_json::to_string_pretty(report)?;
            println!("{}", json);
        },
    }
    Ok(())
}

fn doctor_failed(report: &DoctorReport) -> bool {
    report
        .results
        .iter()
        .any(|result| result.status == CheckStatus::Failure)
}

fn print_human_report(report: &DoctorReport) {
    println!("==== Pipeline Vitte Doctor ====");
    for result in &report.results {
        println!(
            "{} {:<12} ({} ms)",
            result.status.colored_label(),
            result.name,
            result.duration_ms
        );
        if !result.details.is_empty() {
            println!("{}", indent(&result.details, 4));
        }
        if let Some(suggestion) = &result.suggestion {
            if result.status == CheckStatus::Failure {
                println!("    → {}", suggestion.blue());
            }
        }
    }

    let has_failures = report.results.iter().any(|r| r.status == CheckStatus::Failure);
    if has_failures {
        if report.suggestions.is_empty() {
            println!(
                "{} Des corrections sont nécessaires avant le build.",
                "!".yellow()
            );
        } else {
            println!("{} Suggestions:", "•".yellow());
            for s in &report.suggestions {
                println!("    - {}", s);
            }
        }
    } else if report.suggestions.is_empty() {
        println!("{} Toutes les vérifications sont passées.", "✔".green());
    } else {
        println!("{} Suggestions:", "•".yellow());
        for s in &report.suggestions {
            println!("    - {}", s);
        }
    }
}

fn run_doctor_watch() -> Result<()> {
    println!("{}", "👀 Mode watch activé — Ctrl+C pour quitter.".cyan());

    let mut plan = DoctorPlan::default();
    let mut report = collect_doctor_report(&mut plan);
    emit_report(&report, Format::Text)?;

    let (tx, rx) = channel();
    let mut watcher = RecommendedWatcher::new(move |res: Result<Event, notify::Error>| {
        let _ = tx.send(res);
    }, Config::default())?;
    watcher.watch(Path::new("."), RecursiveMode::Recursive)?;

    let debounce = Duration::from_millis(250);
    let mut last_run = Instant::now();

    while let Ok(event) = rx.recv() {
        match event {
            Ok(ev) => {
                if !matches!(
                    ev.kind,
                    EventKind::Modify(_) | EventKind::Create(_) | EventKind::Remove(_)
                ) {
                    continue;
                }
                if !ev.paths.is_empty() && ev.paths.iter().all(|p| should_ignore_path(p)) {
                    continue;
                }
                if last_run.elapsed() < debounce {
                    continue;
                }
                println!();
                println!("{} {:?}", "↻".green(), ev.kind);
                report = collect_doctor_report(&mut plan);
                emit_report(&report, Format::Text)?;
                last_run = Instant::now();
            },
            Err(err) => {
                eprintln!("{} watch error: {err}", "!".red());
            },
        }
    }

    Ok(())
}

fn indent(text: &str, pad: usize) -> String {
    let prefix = " ".repeat(pad);
    text.lines()
        .map(|line| format!("{}{}", prefix, line))
        .collect::<Vec<_>>()
        .join("\n")
}

fn should_ignore_path(path: &Path) -> bool {
    path.components().any(|comp| match comp {
        Component::Normal(name) => {
            if let Some(seg) = name.to_str() {
                matches!(
                    seg,
                    "target" | ".git" | ".idea" | ".vscode" | "node_modules"
                )
            } else {
                false
            }
        },
        _ => false,
    })
}

fn install_pre_commit_hook() -> Result<()> {
    let hook_path = Path::new(".git/hooks/pre-commit");
    let script = "#!/usr/bin/env sh\n\n# Auto-generated by vitte doctor\necho '[vitte] pre-commit: pipeline doctor'\nvitte-bin doctor\n";

    if let Some(parent) = hook_path.parent() {
        std::fs::create_dir_all(parent)?;
    }

    std::fs::write(hook_path, script)?;
    #[cfg(unix)]
    {
        use std::os::unix::fs::PermissionsExt;
        let perm = std::fs::Permissions::from_mode(0o755);
        std::fs::set_permissions(hook_path, perm)?;
    }
    println!(
        "{} Hook pre-commit installé: {}",
        "✔".green(),
        hook_path.display()
    );
    Ok(())
}

/* -------------------------------------------------------------------------- */
/* Utilitaires                                                                */
/* -------------------------------------------------------------------------- */

fn which_engine() -> Option<String> {
    match which::which("vitte") {
        Ok(p) => Some(p.display().to_string()),
        Err(_) => None,
    }
}

fn escape_json<D: std::fmt::Display>(d: D) -> String {
    let s = d.to_string();
    s.replace('\\', "\\\\").replace('"', "\\\"")
}
