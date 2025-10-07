use anyhow::Result;
use clap::{Parser, Subcommand, ArgAction, ColorChoice};
use env_logger;
use log::{debug, error};

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
    /// Affiche la version détaillée
    Version,
}

fn init_logger(verbosity: u8, quiet: bool) {
    let level = if quiet { "off" } else { match verbosity { 0 => "info", 1 => "debug", _ => "trace" } };
    if std::env::var_os("RUST_LOG").is_none() { std::env::set_var("RUST_LOG", level); }
    let _ = env_logger::Builder::from_env(env_logger::Env::default().default_filter_or(level))
        .format_timestamp_millis()
        .try_init();
}

fn main() {
    let cli = Cli::parse();
    init_logger(cli.verbose, cli.quiet);
    if let Err(e) = real_main(cli) {
        error!("{e:#}");
        std::process::exit(1);
    }
}

fn real_main(cli: Cli) -> Result<()> {
    debug!("args = {:?}", std::env::args().collect::<Vec<_>>());
    match cli.command {
        Command::Version => println!("{}", crate::version_string()),
        Command::Run { input, args } => crate::engine_run(&input, &args)?,
        Command::Compile { input, out, emit } => crate::engine_compile(&input, out.as_deref(), &emit)?,
        Command::Repl => crate::engine_repl()?,
    }
    Ok(())
}