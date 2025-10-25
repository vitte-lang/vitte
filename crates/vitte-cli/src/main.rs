//! `vitte` — CLI principal de Vitte
//!
//! Bienvenue dans le binaire ! Ici on fait uniquement : parsing d'arguments,
//! initialisation (logger, couleur), et délégation à `vitte_cli` (lib).

#![forbid(unsafe_code)]

#[cfg(feature = "server")]
use std::net::SocketAddr;
use std::{
    ffi::OsString,
    path::{Path, PathBuf},
    process::ExitCode,
};

use anyhow::{Context, Result};
use clap::{ArgAction, Parser, Subcommand, ValueEnum};

use cli::inspect::InspectOptions;
use vitte_cli as cli; // notre lib interne (src/lib.rs)

#[cfg(feature = "fmt")]
use vitte_fmt::{format_source, FormatterOptions};

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

    /// Profil d'environnement (dev ou release)
    #[arg(long = "profile", value_enum, default_value_t = ProfileChoice::Dev, global = true)]
    profile: ProfileChoice,

    /// Langue pour les messages utilisateur
    #[arg(long = "lang", value_enum, global = true)]
    lang: Option<LangChoice>,

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

#[derive(Debug, Clone, Copy, ValueEnum)]
enum ProfileChoice {
    Dev,
    Release,
}

#[derive(Debug, Clone, Copy, ValueEnum)]
enum LangChoice {
    En,
    Fr,
}

impl From<LangChoice> for cli::i18n::Lang {
    fn from(value: LangChoice) -> Self {
        match value {
            LangChoice::En => cli::i18n::Lang::En,
            LangChoice::Fr => cli::i18n::Lang::Fr,
        }
    }
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

    /// Lancer le serveur JSON-RPC
    #[cfg(feature = "server")]
    Server {
        /// Adresse d’écoute (ex: 127.0.0.1:7411)
        #[arg(long = "listen", default_value = "127.0.0.1:7411")]
        addr: String,
        /// Jeton d’authentification à fournir via `auth`
        #[arg(long = "token")]
        token: Option<String>,
        /// Racine du workspace (défaut: répertoire courant)
        #[arg(long = "root")]
        root: Option<PathBuf>,
    },

    /// Lancer un REPL (si fourni par les hooks)
    Repl {
        /// Prompt du REPL
        #[arg(long, default_value = "vitte> ")]
        prompt: String,
    },

    /// Diagnostiquer l'environnement (toolchain, cache, modules)
    Doctor {
        /// Afficher le rapport au format JSON
        #[arg(long = "json")]
        json: bool,
        /// Proposer une purge du cache `target`
        #[arg(long = "fix-cache")]
        fix_cache: bool,
        /// Ne pas demander de confirmation pour les actions correctives
        #[arg(long = "yes")]
        yes: bool,
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
        /// Afficher le diff des changements (stdout)
        #[arg(long = "diff")]
        diff: bool,
    },

    /// Inspecter un artefact VITBC
    Inspect {
        /// Bytecode (.vitbc)
        input: Option<PathBuf>,
        /// Résumé compact
        #[arg(long)]
        summary: bool,
        /// Table des symboles uniquement
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
        /// Liste des sections
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
        /// Désassemblage textuel
        #[arg(long)]
        disasm: bool,
        /// Informations de debug
        #[arg(long)]
        debug: bool,
        /// Métadonnées additionnelles
        #[arg(long)]
        meta: bool,
        /// Vérification d'intégrité
        #[arg(long)]
        verify: bool,
        /// Taille par section
        #[arg(long)]
        size: bool,
        /// Dépendances embarquées
        #[arg(long)]
        deps: bool,
        /// Point d'entrée
        #[arg(long)]
        entry: bool,
        /// Active toutes les sections
        #[arg(long = "dump-all")]
        dump_all: bool,
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
        /// Trace le registre local (chemin, taille, timing)
        #[arg(long = "trace-registry")]
        trace_registry: bool,
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
        }
        Some(ref path) => {
            // heuristique simple : .vitbc = bytecode, sinon source
            if path.extension().and_then(|e| e.to_str()) == Some("vitbc") || !auto_compile {
                cli::InputKind::BytecodePath(path.clone())
            } else {
                cli::InputKind::SourcePath(path.clone())
            }
        }
        None => {
            if auto_compile {
                cli::InputKind::SourceStdin
            } else {
                cli::InputKind::BytecodePath(PathBuf::from("-"))
            }
        }
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
        Some(ref p) if p.as_os_str() == "-" => cli::Output::Stdout,
        Some(ref p) => cli::Output::Path(p.clone()),
        None => {
            if for_compile {
                cli::Output::Auto
            } else {
                cli::Output::Stdout
            }
        }
    }
}

// ──────────────────────────── Hooks (adapteurs) ────────────────────────────

fn make_hooks() -> cli::Hooks {
    let mut h = cli::Hooks::default();

    #[cfg(feature = "engine")]
    {
        h.compile = Some(|src, opts| cli::compile_source_to_bytes(src, opts));
        h.run_bc = Some(|bytecode, opts| cli::run_bytecode(bytecode, opts));
        h.disasm = Some(|bytes| cli::disassemble_bytecode(bytes));
    }

    // REPL — fallback intégré pour dialoguer avec l’utilisateur
    h.repl = Some(cli::repl::fallback);

    // Formatage — fallback via vitte-fmt si disponible
    #[cfg(feature = "fmt")]
    {
        h.fmt = Some(fmt_hook);
    }

    // Inspection — fallback simple sur le format VBC0 synthétique
    h.inspect = Some(inspect_hook);

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
        ColorChoice::Auto => { /* par défaut */ }
        ColorChoice::Always => {
            std::env::set_var("CLICOLOR_FORCE", "1");
            std::env::remove_var("NO_COLOR");
        }
        ColorChoice::Never => {
            std::env::set_var("NO_COLOR", "1");
            std::env::remove_var("CLICOLOR_FORCE");
        }
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

fn infer_default_command(arg: &OsString) -> Option<OsString> {
    let raw = arg.to_str()?;
    if raw.starts_with('-') {
        return None;
    }

    let path = Path::new(raw);
    let ext = path.extension().and_then(|e| e.to_str()).map(|e| e.to_ascii_lowercase());
    let Some(ext) = ext else {
        return None;
    };
    if matches!(ext.as_str(), "vitbc" | "vitb") {
        return Some(OsString::from("run"));
    }
    if matches!(ext.as_str(), "vitte" | "vit" | "vt") {
        return Some(OsString::from("compile"));
    }
    None
}

fn alias_for(arg: &OsString) -> Option<OsString> {
    match arg.to_str()? {
        "build" => Some(OsString::from("compile")),
        "dsasm" => Some(OsString::from("disasm")),
        _ => None,
    }
}

fn real_main() -> Result<()> {
    let mut argv: Vec<OsString> = std::env::args_os().collect();
    if argv.len() == 2 {
        if let Some(cmd) = infer_default_command(&argv[1]) {
            argv.insert(1, cmd);
        }
    }
    if argv.len() > 1 {
        if let Some(alias) = alias_for(&argv[1]) {
            argv[1] = alias;
        }
    }

    let opt = Opt::parse_from(argv);

    let env_lang =
        std::env::var("VITTE_LANG").ok().and_then(|value| cli::i18n::Lang::from_code(&value));
    let cli_lang = opt.lang.map(|choice| cli::i18n::Lang::from(choice));
    let selected_lang = cli_lang.or(env_lang).unwrap_or(cli::i18n::Lang::En);
    cli::i18n::init(selected_lang);

    init_color(opt.color);
    init_telemetry(opt.verbose, opt.quiet);

    let profile = match opt.profile {
        ProfileChoice::Dev => cli::context::Profile::Dev,
        ProfileChoice::Release => cli::context::Profile::Release,
    };
    let cwd = std::env::current_dir().ok();
    let profile_config = cli::context::load_profile_config(profile, cwd.as_deref())
        .with_context(|| format!("chargement du profil {}", profile.as_str()))?;

    let hooks = make_hooks();

    use cli::{
        Command as C, CompileTask, DisasmTask, DoctorTask, FmtTask, InspectTask, ModulesFormat,
        ModulesTask, ReplTask, RunTask,
    };

    let command = match opt.cmd {
        Command::Compile { input, output, optimize, debug, auto_mkdir, overwrite, time, auto } => {
            let input = input_from_opt(&input);
            let out = output_from_opt(
                &output, /*in_place=*/ false, auto, /*for_compile=*/ true,
            );
            let profile_cfg = profile_config.clone();
            let final_optimize = optimize || profile_cfg.optimize;
            let final_debug = debug || profile_cfg.emit_debug;
            C::Compile(CompileTask {
                input,
                output: out,
                optimize: final_optimize,
                emit_debug: final_debug,
                auto_mkdir,
                overwrite,
                time,
                profile: profile_cfg,
            })
        }
        Command::Run { program, args, auto_compile, optimize, time } => {
            let program = input_kind_from_opt(&program, auto_compile);
            let args = args.into_iter().map(|s| s.to_string_lossy().to_string()).collect();
            let profile_cfg = profile_config.clone();
            let final_optimize = optimize || profile_cfg.optimize;
            C::Run(RunTask {
                program,
                args,
                auto_compile,
                optimize: final_optimize,
                time,
                profile: profile_cfg,
            })
        }
        #[cfg(feature = "server")]
        Command::Server { addr, token, root } => {
            let addr: SocketAddr =
                addr.parse().with_context(|| format!("adresse d'écoute invalide: {addr}"))?;
            let server_hooks = hooks.clone();
            let server_profile = profile_config.clone();
            let workspace_root = root.or_else(|| cwd.clone());
            let options = cli::server::ServerOptions {
                addr,
                auth_token: token,
                workspace_root,
                default_profile: server_profile,
            };
            let runtime =
                tokio::runtime::Runtime::new().context("initialisation runtime server")?;
            runtime.block_on(cli::server::run(options, server_hooks))?;
            return Ok(());
        }
        Command::Doctor { json, fix_cache, yes } => {
            C::Doctor(DoctorTask { output_json: json, fix_cache, assume_yes: yes })
        }
        Command::Repl { prompt } => C::Repl(ReplTask { prompt }),
        Command::Fmt { input, output, in_place, check, diff } => {
            let input = input_from_opt(&input);
            let out = output_from_opt(
                &output, in_place, /*auto=*/ false, /*for_compile=*/ false,
            );
            C::Fmt(FmtTask { input, output: out, check, diff })
        }
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
            let kind = match input {
                Some(p) if p.as_os_str() == "-" => cli::InputKind::BytecodePath(p),
                Some(p) => cli::InputKind::BytecodePath(p),
                None => cli::InputKind::BytecodePath(PathBuf::from("-")),
            };
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
            C::Inspect(InspectTask { input: kind, options })
        }
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
        }
        Command::Modules { json, trace_registry } => {
            let format = if json { ModulesFormat::Json } else { ModulesFormat::Table };
            C::Modules(ModulesTask { format, trace_registry })
        }
    };

    let code = cli::execute(command, &hooks).context("échec d'exécution de la commande")?;
    if code != 0 {
        std::process::exit(code);
    }
    Ok(())
}

#[cfg(feature = "fmt")]
fn fmt_hook(source: &str, _check_only: bool) -> anyhow::Result<String> {
    let options = FormatterOptions::default();
    let output = format_source(source, options);
    Ok(output.code)
}

fn inspect_hook(bytes: &[u8], opts: &InspectOptions) -> anyhow::Result<String> {
    Ok(cli::inspect::render(bytes, opts))
}
