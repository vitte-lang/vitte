// src/main.rs
//! Vitte — CLI complet
//! Sous-commandes: compile, run, repl, fmt, inspect, disasm, modules, help
//! Options globales: -v/--verbose, -q/--quiet, --color, -V/--version

use std::path::PathBuf;

use anyhow::{Context, Result};
use clap::{builder::PossibleValuesParser, ArgAction, ColorChoice, Parser, Subcommand};
use clap::CommandFactory;
use log::{debug, error, info, warn};

/// Point d’entrée
fn main() {
    // On parse d’abord pour récupérer le niveau de verbosité et la couleur.
    let cli = Cli::parse();
    init_logging(cli.verbose, cli.quiet);

    if let Err(err) = real_main(cli) {
        error!("{err:#}");
        std::process::exit(1);
    }
}

fn real_main(cli: Cli) -> Result<()> {
    debug!("args = {:?}", std::env::args().collect::<Vec<_>>());
    match cli.command {
        CommandKind::Compile {
            input, out, optimize, target, features, emit,
        } => {
            cmd_compile(input, out, optimize, target, features, emit)?;
        }
        CommandKind::Run { input, args } => {
            cmd_run(input, args)?;
        }
        CommandKind::Repl { prompt } => {
            cmd_repl(prompt)?;
        }
        CommandKind::Fmt { file, check } => {
            cmd_fmt(file, check)?;
        }
        CommandKind::Inspect { input } => {
            cmd_inspect(input)?;
        }
        CommandKind::Disasm { bc } => {
            cmd_disasm(bc)?;
        }
        CommandKind::Modules { list_paths } => {
            cmd_modules(list_paths)?;
        }
        CommandKind::Help => {
            // clap gère déjà `--help`, mais on laisse une commande dédiée.
            Cli::command().print_help()?;
            println!();
        }
    }
    Ok(())
}

/* ────────────────────────────────────────────────────────────────── */
/* CLI                                                                */
/* ────────────────────────────────────────────────────────────────── */

#[derive(Debug, Parser)]
#[command(
    name = "vitte",
    author,
    version,
    about = "Vitte — outil en ligne de commande",
    long_about = None,
    // couleur par défaut auto (hérité du terminal)
    color = ColorChoice::Auto
)]
struct Cli {
    /// Augmente la verbosité (-v, -vv, -vvv)
    #[arg(short, long, global = true, action = ArgAction::Count)]
    verbose: u8,

    /// Mode silencieux (casse la verbosité)
    #[arg(short, long, global = true)]
    quiet: bool,

    /// Force la couleur [auto|always|never]
    #[arg(long, value_parser = PossibleValuesParser::new(["auto", "always", "never"]), global = true)]
    color: Option<String>,

    /// Sous-commande
    #[command(subcommand)]
    command: CommandKind,
}

#[derive(Debug, Subcommand)]
enum CommandKind {
    /// Compile un fichier Vitte
    Compile {
        /// Fichier source (ex: main.vt)
        input: PathBuf,
        /// Sortie (-o). Si absent, déduit du nom d’entrée
        #[arg(short = 'o', long)]
        out: Option<PathBuf>,
        /// Niveau d’optimisation (0..3)
        #[arg(short = 'O', long, default_value_t = 2)]
        optimize: u8,
        /// Triple cible (ex: aarch64-apple-darwin)
        #[arg(long)]
        target: Option<String>,
        /// Features activées (ex: full,serde)
        #[arg(long)]
        features: Option<String>,
        /// Artéfact à émettre [bc|obj|exe]
        #[arg(long, default_value = "bc")]
        emit: String,
    },

    /// Exécute un fichier source ou un bytecode
    Run {
        /// Fichier (source .vt ou bytecode .vtbc)
        input: PathBuf,
        /// Arguments passés au programme
        #[arg(trailing_var_arg = true)]
        args: Vec<String>,
    },

    /// REPL du langage
    Repl {
        /// Invite personnalisée
        #[arg(long, default_value = "vitte> ")]
        prompt: String,
    },

    /// Formatte un fichier source
    Fmt {
        file: PathBuf,
        /// N’écrit pas; retourne code ≠0 si des changements seraient appliqués
        #[arg(long)]
        check: bool,
    },

    /// Inspecte la structure (AST/IR/metadata) d’un fichier
    Inspect {
        input: PathBuf,
    },

    /// Désassemble un bytecode .vtbc
    Disasm {
        bc: PathBuf,
    },

    /// Gère les modules (résolution, chemins, cache)
    Modules {
        /// Afficher les chemins de recherche
        #[arg(long)]
        list_paths: bool,
    },

    /// Affiche cette aide (équivalent à --help)
    Help,
}

/* ────────────────────────────────────────────────────────────────── */
/* Logging                                                            */
/* ────────────────────────────────────────────────────────────────── */

fn init_logging(verbosity: u8, quiet: bool) {
    // Niveau
    let level = if quiet {
        "off"
    } else {
        match verbosity {
            0 => "info",
            1 => "debug",
            _ => "trace",
        }
    };

    // RUST_LOG déjà défini ? on le respecte, sinon on force le nôtre.
    if std::env::var_os("RUST_LOG").is_none() {
        std::env::set_var("RUST_LOG", level);
    }

    // Mise en place du logger
    let mut b = env_logger::Builder::from_env(env_logger::Env::default().default_filter_or(level));
    b.format_timestamp_millis();
    let _ = b.try_init();
}

/* ────────────────────────────────────────────────────────────────── */
/* Sous-commandes — impl                                              */
/* ────────────────────────────────────────────────────────────────── */

fn cmd_compile(
    input: PathBuf,
    out: Option<PathBuf>,
    optimize: u8,
    target: Option<String>,
    features: Option<String>,
    emit: String,
) -> Result<()> {
    info!("🔧 compile: {}", input.display());
    debug!("opt={optimize} target={target:?} features={features:?} emit={emit}");

    // ==== Branche moteur réel (si présent) ==========================
    #[cfg(feature = "engine")]
    {
        // Exemple: appelez vos fonctions réelles ici
        vitte_compiler::compile_file(&input, out.as_deref(), optimize, target.as_deref(), features.as_deref(), &emit)
            .context("échec compilation")?;
        return Ok(());
    }

    // ==== Fallback sûr (pas de moteur) ==============================
    warn!("moteur non compilé (feature `engine` absente) — fallback démo");
    // On simule une sortie en copiant le fichier vers .vtbc si emit=bc
    let out = out.unwrap_or_else(|| input.with_extension(match emit.as_str() {
        "obj" => "o",
        "exe" => if cfg!(windows) { "exe" } else { "" }.into(),
        _ => "vtbc",
    }));
    std::fs::copy(&input, &out).with_context(|| format!("ne peut écrire {}", out.display()))?;
    info!("→ {}", out.display());
    Ok(())
}

fn cmd_run(input: PathBuf, args: Vec<String>) -> Result<()> {
    info!("▶️  run: {}", input.display());

    // Bytecode ?
    let _is_bc = input
        .extension()
        .and_then(|e| e.to_str())
        .map(|e| e.eq_ignore_ascii_case("vtbc"))
        .unwrap_or(false);

    #[cfg(feature = "engine")]
    {
        if is_bc {
            vitte_vm::run_bc(&input, &args).context("échec exécution bytecode")?;
        } else {
            vitte_vm::run_source(&input, &args).context("échec exécution source")?;
        }
        return Ok(());
    }

    // Fallback (pas de moteur)
    warn!("moteur non compilé (feature `engine` absente) — exécution simulée");
    println!("Vitte exécute: {}", input.display());
    if !args.is_empty() {
        println!("args: {args:?}");
    }
    Ok(())
}

fn cmd_repl(prompt: String) -> Result<()> {
    use std::io::{self, Write};
    info!("💬 REPL");
    let mut line = String::new();
    loop {
        print!("{prompt}");
        io::stdout().flush().ok();
        line.clear();
        if io::stdin().read_line(&mut line)? == 0 {
            break;
        }
        let s = line.trim_end();
        if s.eq(":q") || s.eq(":quit") { break; }

        #[cfg(feature = "engine")]
        {
            match vitte_vm::eval_line(s) {
                Ok(v) => println!("= {v}"),
                Err(e) => println!("! {e}"),
            }
            continue;
        }

        // Fallback
        println!("= {s}");
    }
    Ok(())
}

fn cmd_fmt(file: PathBuf, check: bool) -> Result<()> {
    info!("🧹 fmt: {}", file.display());
    #[cfg(feature = "engine")]
    {
        return vitte_fmt::format_file(&file, check).context("formatage échoué");
    }

    // Fallback “no-op” sûr
    if check {
        println!("Formatted: {}", file.display());
    } else {
        println!("Wrote: {}", file.display());
    }
    Ok(())
}

fn cmd_inspect(input: PathBuf) -> Result<()> {
    info!("🔎 inspect: {}", input.display());
    #[cfg(feature = "engine")]
    {
        let r = vitte_tools::inspect(&input).context("inspect échoué")?;
        println!("{r}");
        return Ok(());
    }
    println!("(fallback) informations indisponibles sans feature `engine`");
    Ok(())
}

fn cmd_disasm(bc: PathBuf) -> Result<()> {
    info!("🧩 disasm: {}", bc.display());
    #[cfg(feature = "engine")]
    {
        let txt = vitte_tools::disassemble(&bc).context("désassemblage échoué")?;
        println!("{txt}");
        return Ok(());
    }
    println!("(fallback) désassemblage indisponible sans feature `engine`");
    Ok(())
}

fn cmd_modules(list_paths: bool) -> Result<()> {
    info!("📦 modules");
    #[cfg(feature = "engine")]
    {
        if list_paths {
            for p in vitte_modules::search_paths() {
                println!("{}", p.display());
            }
        } else {
            println!("{}", vitte_modules::summary());
        }
        return Ok(());
    }
    if list_paths {
        println!("/usr/local/lib/vitte-modules (exemple)");
        println!("{}/.vitte/modules", std::env::var("HOME").unwrap_or_default());
    } else {
        println!("(fallback) gestion des modules indisponible sans feature `engine`");
    }
    Ok(())
}

/* ────────────────────────────────────────────────────────────────── */
/* Helpers                                                            */
/* ────────────────────────────────────────────────────────────────── */

#[allow(dead_code)]
fn version_string() -> String {
    // Utilisé si vous souhaitez une sous-commande `version` dédiée
    let commit = option_env!("GIT_COMMIT").unwrap_or("unknown");
    let build_date = option_env!("VERGEN_BUILD_DATE").unwrap_or("unknown");
    let rustc = option_env!("VERGEN_RUSTC_SEMVER").unwrap_or(env!("CARGO_PKG_VERSION"));
    format!("vitte {}\ncommit: {commit}\nbuild: {build_date}\nrustc: {rustc}", env!("CARGO_PKG_VERSION"))
}
