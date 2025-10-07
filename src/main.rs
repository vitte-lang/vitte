/*! 
 * vitte-bin — binaire principal du workspace Vitte
 * CLI complète, sans dépendance directe à vitte-cli.
 * Utilise les helpers exposés par la lib du même crate (src/lib.rs).
 */

use std::path::PathBuf;

use anyhow::{Context, Result};
use clap::{Parser, Subcommand, ArgAction, ColorChoice};
use log::{debug, error, info, warn};

fn main() {
    let cli = Cli::parse();
    init_logger(cli.verbose, cli.quiet);

    if let Err(err) = real_main(cli) {
        error!("{err:#}");
        std::process::exit(1);
    }
}

fn real_main(cli: Cli) -> Result<()> {
    debug!("args = {:?}", std::env::args().collect::<Vec<_>>());
    match cli.command {
        Command::Run { input, args } => cmd_run(input, args)?,
        Command::Compile { input, out, emit } => cmd_compile(input, out, emit)?,
        Command::Repl => cmd_repl()?,
        Command::Version => println!("{}", vitte_bin::version_string()),
    }
    Ok(())
}

/* -------------------------------------------------------------------------- */
/* CLI                                                                       */
/* -------------------------------------------------------------------------- */
#[derive(Debug, Parser)]
#[command(
    name = "vitte-bin",
    author,
    version,
    about = "Vitte CLI (workspace bin)",
    long_about = None,
    color = ColorChoice::Auto
)]
struct Cli {
    /// Augmente la verbosité (-v, -vv, -vvv)
    #[arg(short, long, global = true, action = ArgAction::Count)]
    verbose: u8,

    /// Mode silencieux
    #[arg(short, long, global = true)]
    quiet: bool,

    /// Sous-commande
    #[command(subcommand)]
    command: Command,
}

#[derive(Debug, Subcommand)]
enum Command {
    /// Exécute un fichier .vt ou .vtbc
    Run {
        input: PathBuf,
        #[arg(trailing_var_arg = true)]
        args: Vec<String>,
    },

    /// Compile un fichier source
    Compile {
        input: PathBuf,
        #[arg(short, long)]
        out: Option<PathBuf>,
        #[arg(long, default_value = "bc")]
        emit: String,
    },

    /// Démarre un REPL interactif
    Repl,

    /// Affiche la version détaillée
    Version,
}

/* -------------------------------------------------------------------------- */
/* Logging                                                                    */
/* -------------------------------------------------------------------------- */
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

    let mut b = env_logger::Builder::from_env(env_logger::Env::default().default_filter_or(level));
    b.format_timestamp_millis();
    let _ = b.try_init();
}

/* -------------------------------------------------------------------------- */
/* Commandes principales                                                      */
/* -------------------------------------------------------------------------- */
fn cmd_run(input: PathBuf, args: Vec<String>) -> Result<()> {
    info!("▶️  run: {}", input.display());
    let _is_bc = vitte_bin::is_bytecode(&input);

    #[cfg(feature = "engine")]
    {
        if is_bc {
            vitte_bin::engine::vm::run_bc(&input, &args).context("échec exécution bytecode")?;
        } else {
            vitte_bin::engine::vm::run_source(&input, &args).context("échec exécution source")?;
        }
        return Ok(());
    }

    // Fallback (pas de moteur)
    warn!("feature `engine` désactivée — exécution simulée");
    println!("Simulation : {} {:?}", input.display(), args);
    Ok(())
}

fn cmd_compile(input: PathBuf, out: Option<PathBuf>, emit: String) -> Result<()> {
    info!("🛠️ compile: {}", input.display());

    #[cfg(feature = "engine")]
    {
        vitte_bin::engine::compiler::compile_file(&input, out.as_deref(), 2, None, None, &emit)
            .context("échec compilation")?;
        return Ok(());
    }

    // Fallback : on simule en copiant vers un artefact dérivé
    warn!("feature `engine` désactivée — copie simulée");
    let out_path = out.unwrap_or_else(|| vitte_bin::resolve_output(&input, &emit));
    std::fs::copy(&input, &out_path)
        .with_context(|| format!("impossible d’écrire {}", out_path.display()))?;
    println!("→ {}", out_path.display());
    Ok(())
}

fn cmd_repl() -> Result<()> {
    use std::io::{self, Write};
    info!("💬 REPL");
    let mut line = String::new();
    let mut stdout = io::stdout();
    loop {
        print!("vitte> ");
        stdout.flush().ok();
        line.clear();
        if io::stdin().read_line(&mut line)? == 0 { break; }
        let s = line.trim_end();

        #[cfg(feature = "engine")]
        {
            match vitte_bin::engine::vm::eval_line(s) {
                Ok(v) => println!("= {v}"),
                Err(e) => println!("! {e}"),
            }
            continue;
        }

        println!("= {}", s);
    }
    Ok(())
}
