use anyhow::Result;
use clap::{value_parser, ArgAction, ColorChoice, Parser, Subcommand, ValueEnum};
use env_logger;
use log::{debug, error, info};
use std::path::{Path, PathBuf};

/// Format de sortie pour certaines commandes
#[derive(Copy, Clone, Debug, Eq, PartialEq, ValueEnum)]
pub enum Format { Text, Json }

/// Type d'artefact à émettre côté compile
#[derive(Copy, Clone, Debug, Eq, PartialEq, ValueEnum)]
pub enum Emit { Bc, Obj, Exe }

impl Emit {
    fn as_str(self) -> &'static str {
        match self { Emit::Bc => "bc", Emit::Obj => "obj", Emit::Exe => "exe" }
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

    /// Vérifie la configuration locale (trouve le moteur, droits d'écriture…)
    Doctor,
}

fn init_logger(verbosity: u8, quiet: bool, color: ColorChoice) {
    let level = if quiet {
        "off"
    } else {
        match verbosity { 0 => "info", 1 => "debug", _ => "trace" }
    };

    if std::env::var_os("RUST_LOG").is_none() {
        std::env::set_var("RUST_LOG", level);
    }
    // Respect de --color
    std::env::set_var("RUST_LOG_STYLE", match color { ColorChoice::Always => "always", ColorChoice::Never => "never", _ => "auto"});

    let _ = env_logger::Builder::from_env(
        env_logger::Env::default().default_filter_or(level),
    )
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
        Command::Compile { input, out, emit, target, opt, threads } => {
            cmd_compile(&input, out.as_deref(), emit, target.as_deref(), opt.as_deref(), threads, cli.format)?
        }
        Command::Repl => vitte_bin::engine_repl()?,
        Command::Info => cmd_info(cli.format)?,
        Command::Doctor => cmd_doctor(cli.format)?,
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
        println!("{{\"status\":\"ok\",\"action\":\"run\",\"input\":\"{}\"}}", escape_json(input.display()));
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

    if let Some(t) = target { debug!("target = {}", t); }
    if let Some(o) = opt { debug!("opt = {}", o); }
    if let Some(th) = threads { debug!("threads = {}", th); }

    // La lib gère la délégation moteur ou fallback (copie)
    vitte_bin::engine_compile(input, out, emit.as_str())?;

    let out_path = out.map(|p| p.to_path_buf()).unwrap_or_else(|| vitte_bin::resolve_output(input, emit.as_str()));

    match format {
        Format::Text => println!("→ {}", out_path.display()),
        Format::Json => println!(
            "{{\"status\":\"ok\",\"action\":\"compile\",\"input\":\"{}\",\"output\":\"{}\",\"emit\":\"{}\"}}",
            escape_json(input.display()), escape_json(out_path.display()), emit.as_str()
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
            if let Some(t) = target { println!("target: {}", t); }
            println!("rustc: {}", rustc);
        }
        Format::Json => {
            println!(
                "{{\"version\":\"{}\",\"engine\":\"{}\",\"target\":\"{}\",\"rustc\":\"{}\"}}",
                escape_json(version),
                escape_json(engine.unwrap_or_else(|| "(absent)".into())),
                escape_json(target.unwrap_or_else(|| "".into())),
                rustc
            );
        }
    }
    Ok(())
}

fn cmd_doctor(format: Format) -> Result<()> {
    let engine = which_engine();
    let cwd = std::env::current_dir().ok();
    let target_dir = cwd.clone().map(|p| p.join("target"));
    let can_write = target_dir
        .as_ref()
        .map(|p| std::fs::create_dir_all(p).map(|_| true).unwrap_or(false))
        .unwrap_or(false);

    match format {
        Format::Text => {
            println!("engine: {}", engine.clone().unwrap_or_else(|| "(absent)".into()));
            println!("cwd: {}", cwd.map(|p| p.display().to_string()).unwrap_or_default());
            println!("target: {}", target_dir.map(|p| p.display().to_string()).unwrap_or_default());
            println!("write: {}", if can_write { "ok" } else { "denied" });
        }
        Format::Json => {
            println!(
                "{{\"engine\":\"{}\",\"cwd\":\"{}\",\"target\":\"{}\",\"write\":{}}}",
                escape_json(engine.unwrap_or_else(|| "(absent)".into())),
                escape_json(cwd.map(|p| p.display().to_string()).unwrap_or_default()),
                escape_json(target_dir.map(|p| p.display().to_string()).unwrap_or_default()),
                if can_write { "true" } else { "false" }
            );
        }
    }
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
