//! `vitte` — CLI principal de Vitte
//!
//! Bienvenue dans le binaire ! Ici on fait uniquement : parsing d'arguments,
//! initialisation (logger, couleur), et délégation à `vitte_cli` (lib).

#![forbid(unsafe_code)]

use std::{ffi::OsString, path::PathBuf, process::ExitCode};

use anyhow::{Context, Result};
use clap::{ArgAction, Parser, Subcommand, ValueEnum};

use vitte_cli as cli; // notre lib interne (src/lib.rs)

// ──────────────────────────── CLI (clap) ────────────────────────────

#[derive(Debug, Parser)]
#[command(name = "vitte", version, about = "Vitte CLI — compiler, exécuter, formater, inspecter VITBC", long_about = None)]
struct Opt {
    /// Augmente la verbosité (-v, -vv, -vvv)
    #[arg(short = 'v', long = "verbose", action = ArgAction::Count)]
    verbose: u8,

    /// Mode silencieux (casse la verbosité)
    #[arg(short = 'q', long = "quiet", action = ArgAction::SetTrue)]
    quiet: bool,

    /// Force la couleur (si la feature `color` est compilée)
    #[arg(long = "color", value_enum, default_value_t = ColorChoice::Auto)]
    color: ColorChoice,

    /// Sous-commandes
    #[command(subcommand)]
    cmd: Command,
}

#[derive(Debug, Clone, Copy, ValueEnum)]
enum ColorChoice {
    Auto,
    Always,
    Never,
}

#[derive(Debug, Subcommand)]
enum Command {
    /// Compiler une source vers un bytecode VITBC
    Compile {
        /// Fichier source (ou - pour stdin)
        input: Option<PathBuf>,
        /// Fichier de sortie (.vitbc) (stdout si omis et pas --auto)
        #[arg(short, long)]
        output: Option<PathBuf>,
        /// Optimisations (-O)
        #[arg(short = 'O', long)]
        optimize: bool,
        /// Émettre des infos de debug
        #[arg(long)]
        debug: bool,
        /// Créer les dossiers parents si nécessaire
        #[arg(long = "mkdir")]
        auto_mkdir: bool,
        /// Écraser le fichier de sortie
        #[arg(long)]
        overwrite: bool,
        /// Afficher le temps d'exécution
        #[arg(long)]
        time: bool,
        /// Sortie auto : même nom + .vitbc
        #[arg(long)]
        auto: bool,
    },

    /// Exécuter un bytecode (ou compiler puis exécuter avec --auto-compile)
    Run {
        /// Programme (bytecode .vitbc) ou source si --auto-compile
        program: Option<PathBuf>,
        /// Pass-through des arguments du programme (après --)
        #[arg(last = true)]
        args: Vec<OsString>,
        /// Compiler d'abord si `program` est une source (ou si omis => stdin)
        #[arg(long = "auto-compile")]
        auto_compile: bool,
        /// Optimiser la compilation auto
        #[arg(short = 'O', long)]
        optimize: bool,
        /// Afficher le temps d'exécution
        #[arg(long)]
        time: bool,
    },

    /// Lancer un REPL (si fourni par les hooks)
    Repl {
        /// Prompt du REPL
        #[arg(long, default_value = "vitte> ")]
        prompt: String,
    },

    /// Formater une source (lecture fichier ou stdin)
    Fmt {
        /// Fichier source (ou - pour stdin)
        input: Option<PathBuf>,
        /// Fichier de sortie (stdout si omis) ; --in-place pour réécrire
        #[arg(short, long)]
        output: Option<PathBuf>,
        /// Réécrire le fichier d'entrée
        #[arg(long = "in-place")]
        in_place: bool,
        /// Mode vérification uniquement (retourne erreur si diff)
        #[arg(long = "check")]
        check: bool,
    },

    /// Inspecter un artefact VITBC
    Inspect {
        /// Bytecode (.vitbc)
        input: Option<PathBuf>,
        /// Résumé compact
        #[arg(long)]
        summary: bool,
    },

    /// Désassembler un bytecode vers du texte
    Disasm {
        /// Bytecode
        input: Option<PathBuf>,
        /// Sortie texte (stdout si omis)
        #[arg(short, long)]
        output: Option<PathBuf>,
    },

    /// Lister les modules compilés depuis `vitte-modules`
    Modules {
        /// Sortie JSON (pretty-printed) au lieu du tableau texte
        #[arg(long)]
        json: bool,
    },
}

// ──────────────────────────── Entrée / Sortie ────────────────────────────

fn input_from_opt(p: &Option<PathBuf>) -> cli::Input {
    match p {
        Some(ref path) if path.as_os_str() == "-" => cli::Input::Stdin,
        Some(ref path) => cli::Input::Path(path.clone()),
        None => cli::Input::Stdin,
    }
}

fn input_kind_from_opt(p: &Option<PathBuf>, auto_compile: bool) -> cli::InputKind {
    match p {
        Some(ref path) if path.as_os_str() == "-" => {
            if auto_compile {
                cli::InputKind::SourceStdin
            } else {
                cli::InputKind::BytecodePath(path.clone())
            }
        },
        Some(ref path) => {
            // heuristique simple : .vitbc = bytecode, sinon source
            if path.extension().and_then(|e| e.to_str()) == Some("vitbc") || !auto_compile {
                cli::InputKind::BytecodePath(path.clone())
            } else {
                cli::InputKind::SourcePath(path.clone())
            }
        },
        None => {
            if auto_compile {
                cli::InputKind::SourceStdin
            } else {
                cli::InputKind::BytecodePath(PathBuf::from("-"))
            }
        },
    }
}

fn output_from_opt(
    output: &Option<PathBuf>,
    in_place: bool,
    auto: bool,
    for_compile: bool,
) -> cli::Output {
    if in_place {
        return cli::Output::InPlace;
    }
    if auto && for_compile {
        return cli::Output::Auto;
    }
    match output {
        Some(ref p) => cli::Output::Path(p.clone()),
        None => cli::Output::Stdout,
    }
}

// ──────────────────────────── Hooks (adapteurs) ────────────────────────────

fn make_hooks() -> cli::Hooks {
    let mut h = cli::Hooks::default();

    // Compilation — À RACCORDER à ton compilateur réel
    // Exemple d'API attendue : fn compile_to_vitbc(src: &str, opt: bool, debug: bool) -> Result<Vec<u8>>
    #[allow(unused_mut)]
    let mut compile_hook: Option<cli::CompileFn> = None;
    #[cfg(feature = "vm")]
    {
        // Si tu exposes une fonction depuis vitte-compiler, branche-la ici.
        // use vitte_compiler as vc;
        // compile_hook = Some(|src, opts| vc::compile_to_vitbc(src, opts.optimize, opts.emit_debug));
    }
    h.compile = compile_hook;

    // Exécution — À RACCORDER à ta VM
    #[allow(unused_mut)]
    let mut run_hook: Option<cli::RunFn> = None;
    #[cfg(feature = "vm")]
    {
        // use vitte_vm as vvm;
        // run_hook = Some(|bytecode, _opts| vvm::run_bytes(bytecode));
    }
    h.run_bc = run_hook;

    // REPL — À RACCORDER
    h.repl = None; // Some(|prompt| { /* ... */ Ok(0) });

    // Formatage — À RACCORDER (source -> String formatée)
    h.fmt = None; // Some(|source, check_only| Ok(if check_only { source.to_string() } else { format_code(source) }));

    // Désassemblage — À RACCORDER (bytes -> texte)
    h.disasm = None;

    // Inspection — À RACCORDER (bytes -> texte)
    h.inspect = None;

    h
}

// ──────────────────────────── Logger / Verbosité ────────────────────────────

fn init_telemetry(verbose: u8, quiet: bool) {
    #[cfg(feature = "trace")]
    {
        let level = if quiet {
            "error"
        } else {
            match verbose {
                0 => "warn",
                1 => "info",
                2 => "debug",
                _ => "trace",
            }
        };
        std::env::set_var(
            "RUST_LOG",
            std::env::var("RUST_LOG").unwrap_or_else(|_| level.to_string()),
        );
        cli::init_logger();
    }
}

fn init_color(choice: ColorChoice) {
    // Rien à faire côté code : `owo-colors` détecte tout seul TTY.
    // Tu peux forcer avec des env vars si tu veux : NO_COLOR / FORCE_COLOR etc.
    match choice {
        ColorChoice::Auto => { /* par défaut */ },
        ColorChoice::Always => {
            std::env::set_var("CLICOLOR_FORCE", "1");
            std::env::remove_var("NO_COLOR");
        },
        ColorChoice::Never => {
            std::env::set_var("NO_COLOR", "1");
            std::env::remove_var("CLICOLOR_FORCE");
        },
    }
}

// ──────────────────────────── main ────────────────────────────

fn main() -> ExitCode {
    if let Err(e) = real_main() {
        eprintln!("error: {:#}", e);
        return ExitCode::from(1);
    }
    ExitCode::from(0)
}

fn real_main() -> Result<()> {
    let opt = Opt::parse();

    init_color(opt.color);
    init_telemetry(opt.verbose, opt.quiet);

    let hooks = make_hooks();

    use cli::{
        Command as C, CompileTask, DisasmTask, FmtTask, InspectTask, ModulesFormat, ModulesTask,
        ReplTask, RunTask,
    };

    let command = match opt.cmd {
        Command::Compile { input, output, optimize, debug, auto_mkdir, overwrite, time, auto } => {
            let input = input_from_opt(&input);
            let out = output_from_opt(
                &output, /*in_place=*/ false, auto, /*for_compile=*/ true,
            );
            C::Compile(CompileTask {
                input,
                output: out,
                optimize,
                emit_debug: debug,
                auto_mkdir,
                overwrite,
                time,
            })
        },
        Command::Run { program, args, auto_compile, optimize, time } => {
            let program = input_kind_from_opt(&program, auto_compile);
            let args = args.into_iter().map(|s| s.to_string_lossy().to_string()).collect();
            C::Run(RunTask { program, args, auto_compile, optimize, time })
        },
        Command::Repl { prompt } => C::Repl(ReplTask { prompt }),
        Command::Fmt { input, output, in_place, check } => {
            let input = input_from_opt(&input);
            let out = output_from_opt(
                &output, in_place, /*auto=*/ false, /*for_compile=*/ false,
            );
            C::Fmt(FmtTask { input, output: out, check })
        },
        Command::Inspect { input, summary } => {
            let kind = match input {
                Some(p) if p.as_os_str() == "-" => cli::InputKind::BytecodePath(p),
                Some(p) => cli::InputKind::BytecodePath(p),
                None => cli::InputKind::BytecodePath(PathBuf::from("-")),
            };
            C::Inspect(InspectTask { input: kind, summary })
        },
        Command::Disasm { input, output } => {
            let kind = match input {
                Some(p) if p.as_os_str() == "-" => cli::InputKind::BytecodePath(p),
                Some(p) => cli::InputKind::BytecodePath(p),
                None => cli::InputKind::BytecodePath(PathBuf::from("-")),
            };
            let out = output_from_opt(
                &output, /*in_place=*/ false, /*auto=*/ false,
                /*for_compile=*/ false,
            );
            C::Disasm(DisasmTask { input: kind, output: out })
        },
        Command::Modules { json } => {
            let format = if json { ModulesFormat::Json } else { ModulesFormat::Table };
            C::Modules(ModulesTask { format })
        },
    };

    let code = cli::execute(command, &hooks).context("échec d'exécution de la commande")?;
    if code != 0 {
        std::process::exit(code);
    }
    Ok(())
}
