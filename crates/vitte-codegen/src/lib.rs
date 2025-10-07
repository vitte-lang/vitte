#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(
    clippy::module_name_repetitions,
    clippy::doc_markdown,
    clippy::too_many_lines
)]
//! vitte-codegen — moteur de génération de code pour Vitte
//!
//! Objectifs :
//! - Compiler un module IR vers différents backends (ASM, LLVM, Cranelift…)
//! - Gérer les cibles (x86_64, aarch64, wasm…)
//! - Exporter les artefacts (obj, exe, lib…)
//! - Fournir une interface CLI minimale pour la compilation croisée
//!
//! Implémentation actuelle : squelette de moteur + tests de fumée.

#[cfg(feature = "cli")]
use std::env;
#[cfg(feature = "cli")]
use std::io::{self, Write};
#[cfg(feature = "cli")]
use std::process::{self, Command};

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Verbosity {
    Quiet,
    Normal,
    Verbose,
}

impl Default for Verbosity {
    fn default() -> Self {
        Verbosity::Normal
    }
}

#[derive(Clone, Debug, Default)]
pub struct GlobalOpts {
    pub verbosity: Verbosity,
    pub target: Option<String>,
    pub jobs: Option<usize>,
    pub out_dir: Option<std::path::PathBuf>,
}

#[cfg(feature = "cli")]
pub fn print_help() {
    println!(
        "Usage: vitte-codegen <command> [OPTIONS]\n\n\
         Commands:\n\
           targets list                 Liste les cibles connues\n\
           targets add <triple>         Ajoute une cible (ex: x86_64-unknown-linux-gnu)\n\
           targets remove <triple>      Supprime une cible\n\
           build [--release]            Construit pour la cible\n\
           pkg <fmt>                    Package l’artefact (zip|tar|deb|rpm|dmg)\n\
           env                          Affiche l’environnement de build\n\
           version                      Affiche la version\n\
           help                         Affiche cette aide\n\n\
         Options globales:\n\
           -t, --target <triple>        Triplet cible\n\
           -j, --jobs <N>               Nombre de jobs de build\n\
           -o, --out-dir <dir>          Dossier de sortie\n\
           -q, --quiet                  Silencieux\n\
           -v, --verbose                Verbeux\n        "
    );
}

#[cfg(feature = "cli")]
pub fn print_version() {
    println!("vitte-codegen {}", env!("CARGO_PKG_VERSION"));
}

#[cfg(feature = "cli")]
pub fn parse_global_opts(args: &mut Vec<String>) -> GlobalOpts {
    let mut opts = GlobalOpts { verbosity: Verbosity::Normal, ..Default::default() };
    let mut i = 1; // après la commande
    while i < args.len() {
        match args[i].as_str() {
            "-q" | "--quiet" => {
                opts.verbosity = Verbosity::Quiet;
                args.remove(i);
            },
            "-v" | "--verbose" => {
                opts.verbosity = Verbosity::Verbose;
                args.remove(i);
            },
            "-t" | "--target" => {
                if i + 1 < args.len() {
                    let _ = args.remove(i);
                    opts.target = Some(args.remove(i));
                }
            },
            "-j" | "--jobs" => {
                if i + 1 < args.len() {
                    let _ = args.remove(i);
                    opts.jobs = args.remove(i).parse().ok();
                }
            },
            "-o" | "--out-dir" => {
                if i + 1 < args.len() {
                    let _ = args.remove(i);
                    opts.out_dir = Some(std::path::PathBuf::from(args.remove(i)));
                }
            },
            _ => {
                i += 1;
            },
        }
    }
    opts
}

#[cfg(feature = "cli")]
pub fn cmd_targets_list() -> io::Result<()> {
    println!("Cibles connues :");
    println!("  - x86_64-unknown-linux-gnu");
    println!("  - aarch64-apple-darwin");
    println!("  - x86_64-pc-windows-msvc");
    Ok(())
}

#[cfg(feature = "cli")]
pub fn cmd_targets_add(triple: &str) -> io::Result<()> {
    println!("Ajout de la cible : {triple}");
    Ok(())
}

#[cfg(feature = "cli")]
pub fn cmd_targets_remove(triple: &str) -> io::Result<()> {
    println!("Suppression de la cible : {triple}");
    Ok(())
}

#[cfg(feature = "cli")]
pub fn cmd_build(opts: &GlobalOpts, release: bool) -> io::Result<()> {
    let target = opts.target.as_deref().unwrap_or(host_triple());
    let mut cmd = Command::new("cargo");
    cmd.arg("build");
    if release {
        cmd.arg("--release");
    }
    if let Some(j) = opts.jobs {
        cmd.arg("-j").arg(j.to_string());
    }
    if let Some(dir) = &opts.out_dir {
        cmd.arg("--target-dir").arg(dir);
    }
    cmd.env("CARGO_BUILD_TARGET", target);
    run(&mut cmd)
}

#[cfg(feature = "cli")]
pub fn cmd_pkg(opts: &GlobalOpts, fmt: &str) -> io::Result<()> {
    let target = opts.target.as_deref().unwrap_or(host_triple());
    println!("Packaging format={fmt} target={target}");
    Ok(())
}

#[cfg(feature = "cli")]
pub fn cmd_env(opts: &GlobalOpts) -> io::Result<()> {
    println!("Hôte: {}", host_triple());
    println!("Target: {}", opts.target.as_deref().unwrap_or("<none>"));
    println!("PATH: {}", env::var("PATH").unwrap_or_default());
    Ok(())
}

#[cfg(feature = "cli")]
pub fn run(cmd: &mut Command) -> io::Result<()> {
    let status = cmd.status()?;
    if !status.success() {
        return Err(io::Error::new(io::ErrorKind::Other, format!("commande échouée: {status}")));
    }
    Ok(())
}

#[cfg(feature = "cli")]
pub fn host_triple() -> &'static str {
    #[cfg(all(target_os = "linux", target_arch = "x86_64"))]
    {
        "x86_64-unknown-linux-gnu"
    }
    #[cfg(all(target_os = "linux", target_arch = "aarch64"))]
    {
        "aarch64-unknown-linux-gnu"
    }
    #[cfg(all(target_os = "macos", target_arch = "x86_64"))]
    {
        "x86_64-apple-darwin"
    }
    #[cfg(all(target_os = "macos", target_arch = "aarch64"))]
    {
        "aarch64-apple-darwin"
    }
    #[cfg(all(target_os = "windows", target_env = "msvc", target_arch = "x86_64"))]
    {
        "x86_64-pc-windows-msvc"
    }
    #[cfg(not(any(
        all(target_os = "linux", target_arch = "x86_64"),
        all(target_os = "linux", target_arch = "aarch64"),
        all(target_os = "macos", target_arch = "x86_64"),
        all(target_os = "macos", target_arch = "aarch64"),
        all(target_os = "windows", target_env = "msvc", target_arch = "x86_64"),
    )))]
    {
        "unknown-unknown-unknown"
    }
}

#[cfg(feature = "cli")]
pub fn main() {
    let mut args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        print_help();
        process::exit(1);
    }

    let cmd: String = args.get(1).cloned().unwrap();
    args.remove(1);
    let opts = parse_global_opts(&mut args);

    let res = match cmd.as_str() {
        "help" | "-h" | "--help" => {
            print_help();
            Ok(())
        },
        "version" | "-V" | "--version" => {
            print_version();
            Ok(())
        },
        "targets" => {
            if args.len() < 2 {
                eprintln!("error: missing subcommand for `targets`");
                print_help();
                process::exit(1);
            }
            match args[1].as_str() {
                "list" => cmd_targets_list(),
                "add" => {
                    if args.len() < 3 {
                        eprintln!("error: missing <triple> for `targets add`");
                        process::exit(1);
                    }
                    cmd_targets_add(&args[2])
                },
                "remove" => {
                    if args.len() < 3 {
                        eprintln!("error: missing <triple> for `targets remove`");
                        process::exit(1);
                    }
                    cmd_targets_remove(&args[2])
                },
                other => {
                    eprintln!("error: unknown targets subcommand `{other}`");
                    process::exit(1);
                },
            }
        },
        "build" => {
            let release = args.iter().any(|a| a == "--release");
            cmd_build(&opts, release)
        },
        "pkg" => {
            if args.len() < 2 {
                eprintln!("error: missing <fmt> for `pkg`");
                process::exit(1);
            }
            cmd_pkg(&opts, &args[1])
        },
        "env" => cmd_env(&opts),
        other => {
            eprintln!("error: unknown command `{other}`");
            print_help();
            process::exit(1);
        },
    };

    if let Err(e) = res {
        let _ = writeln!(io::stderr(), "error: {e}");
        process::exit(1);
    }
}

#[cfg(all(test, any(feature = "asm", feature = "cranelift", feature = "llvm")))]
mod tests {
    use super::*;
    use vitte_ir::builder::ModuleBuilder;

    #[test]
    fn smoke_codegen_empty() {
        let mut builder = ModuleBuilder::new("test");
        let module = builder.finish();
        let cfg = CodegenConfig::default_target(CodegenTarget::X86_64);
        let res = compile_module(&module, &cfg);
        assert!(res.is_ok());
    }
}
