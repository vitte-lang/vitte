//! `vitte` — CLI principal de Vitte
//!
//! Bienvenue dans le binaire ! Ici on fait uniquement : parsing d'arguments,
//! initialisation (logger, couleur), et délégation à `vitte_cli` (lib).

#![forbid(unsafe_code)]

use std::{
    ffi::OsString,
    path::{Path, PathBuf},
    process::ExitCode,
};

use anyhow::{Context, Result, bail};
use clap::{ArgAction, Parser, Subcommand, ValueEnum};

use cli::inspect::InspectOptions;
use vitte_cli as cli; // notre lib interne (src/lib.rs)

#[cfg(feature = "fmt")]
use vitte_fmt::{FormatterOptions, format_source};

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
        Some(ref p) if p.as_os_str() == "-" => cli::Output::Stdout,
        Some(ref p) => cli::Output::Path(p.clone()),
        None => {
            if for_compile {
                cli::Output::Auto
            } else {
                cli::Output::Stdout
            }
        },
    }
}

// ──────────────────────────── Hooks (adapteurs) ────────────────────────────

fn make_hooks() -> cli::Hooks {
    let mut h = cli::Hooks::default();

    // Compilation — À RACCORDER à ton compilateur réel
    // Exemple d'API attendue : fn compile_to_vitbc(src: &str, opt: bool, debug: bool) -> Result<Vec<u8>>
    #[cfg(feature = "vm")]
    {
        // Fallback: encode la source pour offrir `--auto-compile` avec la VM stub.
        h.compile = Some(|src, _opts| {
            let mut out = Vec::with_capacity(8 + src.len());
            out.extend_from_slice(b"VBC0");
            out.extend_from_slice(&(src.len() as u32).to_le_bytes());
            out.extend_from_slice(src.as_bytes());
            Ok(out)
        });
    }

    // Exécution — À RACCORDER à ta VM
    #[cfg(feature = "vm")]
    {
        use vitte_vm::Vm;

        h.run_bc = Some(|bytecode, _opts| {
            // VM minimale : instancie et exécute le bytecode reçu.
            // Tant que la VM ne gère pas args/opts, on les ignore.
            let mut vm = Vm::new();
            Ok(vm.run_bytecode(bytecode))
        });
    }

    // Désassemblage — stub pour le format VBC0 synthétique
    #[cfg(feature = "vm")]
    {
        h.disasm = Some(|bytes| {
            if bytes.len() >= 8 && &bytes[..4] == b"VBC0" {
                let len = u32::from_le_bytes([bytes[4], bytes[5], bytes[6], bytes[7]]) as usize;
                let payload = bytes.get(8..).unwrap_or_default();
                let snippet = String::from_utf8_lossy(&payload[..payload.len().min(len)]);
                Ok(format!(
                    "; Vitte stub bytecode\nmagic: VBC0\nlen: {} bytes\n--- source snippet ---\n{}",
                    len, snippet
                ))
            } else {
                Ok(format!("; Bytecode inconnu ({} octets)", bytes.len()))
            }
        });
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

    validate_cli_syntax(&argv)?;

    let opt = Opt::parse_from(argv);

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
            if in_place && output.is_some() {
                bail!(
                    "les options `--in-place` et `--output` ne peuvent pas être utilisées ensemble"
                );
            }
            if in_place && check {
                bail!("`--in-place` est incompatible avec `--check`");
            }

            let input = input_from_opt(&input);
            if in_place && matches!(input, cli::Input::Stdin) {
                bail!("`--in-place` nécessite un fichier en entrée (pas stdin)");
            }

            let out = output_from_opt(
                &output, in_place, /*auto=*/ false, /*for_compile=*/ false,
            );
            C::Fmt(FmtTask { input, output: out, check })
        },
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

#[cfg(feature = "fmt")]
fn fmt_hook(source: &str, _check_only: bool) -> anyhow::Result<String> {
    let options = FormatterOptions::default();
    let output = format_source(source, options);
    Ok(output.code)
}

fn inspect_hook(bytes: &[u8], opts: &InspectOptions) -> anyhow::Result<String> {
    Ok(cli::inspect::render(bytes, opts))
}

fn validate_cli_syntax(argv: &[OsString]) -> Result<()> {
    let mut idx = 1; // skip binary name
    if argv.len() <= 1 {
        return Ok(());
    }

    consume_global_options(argv, &mut idx)?;

    if idx >= argv.len() {
        return Ok(());
    }

    let command = argv[idx].to_str().unwrap_or_default().to_string();
    idx += 1;

    let tail = &argv[idx..];
    match command.as_str() {
        "compile" => validate_compile_args(tail),
        "run" => validate_run_args(tail),
        _ => Ok(()),
    }
}

fn consume_global_options(argv: &[OsString], idx: &mut usize) -> Result<()> {
    while *idx < argv.len() {
        let Some(current) = argv[*idx].to_str() else { break };
        match current {
            "-v" | "-vv" | "-vvv" | "--verbose" | "-q" | "--quiet" => {
                *idx += 1;
            },
            s if s.starts_with("--color=") => {
                let value = &s["--color=".len()..];
                validate_color_value_str(value)?;
                *idx += 1;
            },
            "--color" => {
                *idx += 1;
                if *idx >= argv.len() {
                    anyhow::bail!(
                        "l'option `--color` requiert une valeur (`auto`, `always` ou `never`)"
                    );
                }
                validate_color_value_os(&argv[*idx])?;
                *idx += 1;
            },
            _ => break,
        }
    }
    Ok(())
}

fn validate_compile_args(args: &[OsString]) -> Result<()> {
    let mut idx = 0;
    let mut seen_option = false;

    if idx < args.len() && !is_compile_option_token(&args[idx]) {
        if !is_path_token(&args[idx]) {
            let token = args[idx].to_string_lossy();
            anyhow::bail!("argument `{token}` non reconnu pour `vitte compile`");
        }
        idx += 1; // consume source operand
    }

    while idx < args.len() {
        if try_consume_compile_option(args, &mut idx, &mut seen_option)? {
            continue;
        }
        let token = args[idx].to_string_lossy();
        if is_global_option_token(&args[idx]) {
            anyhow::bail!("l'option `{token}` doit être placée avant la commande `compile`");
        }
        if seen_option && is_path_token(&args[idx]) {
            anyhow::bail!(
                "la source `{token}` doit être fournie avant les options de `vitte compile`"
            );
        }
        anyhow::bail!("argument `{token}` non reconnu pour `vitte compile`");
    }
    Ok(())
}

fn validate_run_args(args: &[OsString]) -> Result<()> {
    let mut idx = 0;
    let mut seen_option = false;

    if idx < args.len() && !is_run_option_token(&args[idx]) && args[idx].to_str() != Some("--") {
        if !is_path_token(&args[idx]) {
            let token = args[idx].to_string_lossy();
            anyhow::bail!("argument `{token}` non reconnu pour `vitte run`");
        }
        idx += 1; // consume program operand
    }

    while idx < args.len() {
        if let Some("--") = args[idx].to_str() {
            idx += 1;
            if idx >= args.len() {
                anyhow::bail!("`vitte run --` doit être suivi d'au moins un argument de programme");
            }
            // Remaining tokens are program arguments; always accepted
            return Ok(());
        }

        if try_consume_run_option(args, &mut idx, &mut seen_option)? {
            continue;
        }

        let token = args[idx].to_string_lossy();
        if is_global_option_token(&args[idx]) {
            anyhow::bail!("l'option `{token}` doit être placée avant la commande `run`");
        }
        if seen_option && is_path_token(&args[idx]) {
            anyhow::bail!(
                "le programme `{token}` doit être fourni avant les options de `vitte run`"
            );
        }
        anyhow::bail!("argument `{token}` non reconnu pour `vitte run`");
    }
    Ok(())
}

fn try_consume_compile_option(
    args: &[OsString],
    idx: &mut usize,
    seen_option: &mut bool,
) -> Result<bool> {
    if *idx >= args.len() {
        return Ok(false);
    }
    let Some(current) = args[*idx].to_str() else {
        return Ok(false);
    };
    let flag_consumed = match current {
        "-O" | "--optimize" | "--debug" | "--mkdir" | "--overwrite" | "--time" | "--auto" => {
            *idx += 1;
            true
        },
        "-o" | "--output" => {
            *idx += 1;
            if *idx >= args.len() {
                anyhow::bail!("l'option `{current}` requiert une valeur");
            }
            *idx += 1;
            true
        },
        _ if current.starts_with("--output=") => {
            if current.len() == "--output=".len() {
                anyhow::bail!("l'option `--output` requiert une valeur");
            }
            *idx += 1;
            true
        },
        _ => false,
    };
    if flag_consumed {
        *seen_option = true;
    }
    Ok(flag_consumed)
}

fn try_consume_run_option(
    args: &[OsString],
    idx: &mut usize,
    seen_option: &mut bool,
) -> Result<bool> {
    if *idx >= args.len() {
        return Ok(false);
    }
    let Some(current) = args[*idx].to_str() else {
        return Ok(false);
    };
    let flag_consumed = matches!(current, "-O" | "--optimize" | "--auto-compile" | "--time");
    if flag_consumed {
        *idx += 1;
        *seen_option = true;
    }
    Ok(flag_consumed)
}

fn is_compile_option_token(arg: &OsString) -> bool {
    if let Some(s) = arg.to_str() {
        matches!(
            s,
            "-O" | "--optimize"
                | "--debug"
                | "--mkdir"
                | "--overwrite"
                | "--time"
                | "--auto"
                | "-o"
                | "--output"
        ) || s.starts_with("--output=")
    } else {
        false
    }
}

fn is_run_option_token(arg: &OsString) -> bool {
    if let Some(s) = arg.to_str() {
        matches!(s, "-O" | "--optimize" | "--auto-compile" | "--time")
    } else {
        false
    }
}

fn is_global_option_token(arg: &OsString) -> bool {
    if let Some(s) = arg.to_str() {
        matches!(s, "-v" | "-vv" | "-vvv" | "--verbose" | "-q" | "--quiet" | "--color")
            || s.starts_with("--color=")
    } else {
        false
    }
}

fn is_path_token(arg: &OsString) -> bool {
    if arg.to_str() == Some("-") {
        return true;
    }
    !arg.to_string_lossy().starts_with("--") && !arg.to_string_lossy().starts_with("-")
}

fn validate_color_value_os(value: &OsString) -> Result<()> {
    match value.to_str() {
        Some(v) => validate_color_value_str(v),
        None => anyhow::bail!("valeur `--color` invalide (non UTF-8)"),
    }
}

fn validate_color_value_str(value: &str) -> Result<()> {
    if matches!(value, "auto" | "always" | "never") {
        Ok(())
    } else {
        anyhow::bail!("valeur `--color` invalide `{value}` (attendu: auto, always ou never)")
    }
}
