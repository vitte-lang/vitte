//! vitte-cross — outils de cross-compilation pour Vitte
//!
//! Objectifs:
//! - Lister/ajouter/supprimer des cibles (triplets)
//! - Construire pour une cible donnée
//! - Packager artefacts (archive/dmg/deb/rpm)
//! - Afficher l’environnement (toolchains, SDKs, PATH)
//!
//! Implémentation actuelle: squelette CLI sans dépendances externes.
//! Zéro `unsafe`. Parsing manuel des arguments.

use std::env;
use std::io::{self, Write};
use std::path::PathBuf;
use std::process::{self, Command};

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum Verbosity {
    Quiet,
    Normal,
    Verbose,
}

#[derive(Clone, Debug)]
struct GlobalOpts {
    verbosity: Verbosity,
    target: Option<String>,
    jobs: Option<usize>,
    out_dir: Option<PathBuf>,
}

impl Default for GlobalOpts {
    fn default() -> Self {
        Self { verbosity: Verbosity::Normal, target: None, jobs: None, out_dir: None }
    }
}

fn print_help() {
    println!(
        "Usage: vitte-cross <command> [OPTIONS]\n\n\
         Commands:\n\
           targets list                 Liste les cibles connues\n\
           targets add <triple>         Ajoute une cible (ex: x86_64-unknown-linux-gnu)\n\
           targets remove <triple>      Supprime une cible\n\
           build [--release]            Construit pour la cible\n\
           pkg <fmt>                    Package l’artefact (zip|tar|deb|rpm|dmg)\n\
           env                          Affiche l’environnement de cross\n\
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

fn print_version() {
    println!("vitte-cross {}", env!("CARGO_PKG_VERSION"));
}

fn parse_global_opts(args: &mut Vec<String>) -> GlobalOpts {
    let mut opts = GlobalOpts::default();
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
                let v = take_val(args, i);
                opts.target = v;
            },
            "-j" | "--jobs" => {
                let v = take_val(args, i).and_then(|s| s.parse().ok());
                opts.jobs = v;
            },
            "-o" | "--out-dir" => {
                let v = take_val(args, i).map(PathBuf::from);
                opts.out_dir = v;
            },
            _ => {
                i += 1;
            },
        }
    }
    opts
}

fn take_val(args: &mut Vec<String>, i_flag: usize) -> Option<String> {
    if i_flag + 1 >= args.len() {
        return None;
    }
    let _flag = args.remove(i_flag); // supprime le flag
    Some(args.remove(i_flag)) // la valeur prend la place
}

/* --------------------------- sous-commandes --------------------------- */

fn cmd_targets_list(_opts: &GlobalOpts) -> io::Result<()> {
    // Stub: dans une implémentation réelle, lire une config (~/.vitte/cross/targets)
    println!(
        "cibles connues:\n  - x86_64-unknown-linux-gnu\n  - aarch64-apple-darwin\n  - x86_64-pc-windows-msvc"
    );
    Ok(())
}

fn cmd_targets_add(_opts: &GlobalOpts, triple: &str) -> io::Result<()> {
    println!("ajout de la cible: {triple}");
    Ok(())
}

fn cmd_targets_remove(_opts: &GlobalOpts, triple: &str) -> io::Result<()> {
    println!("suppression de la cible: {triple}");
    Ok(())
}

fn cmd_build(opts: &GlobalOpts, release: bool) -> io::Result<()> {
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
    if opts.verbosity == Verbosity::Verbose {
        eprintln!("[run] {:?}", cmd);
    }
    run(&mut cmd)
}

fn cmd_pkg(opts: &GlobalOpts, fmt: &str) -> io::Result<()> {
    let target = opts.target.as_deref().unwrap_or(host_triple());
    println!("packaging format={fmt} target={target}");
    // Stub: empaqueter le binaire depuis target/<triple>/<profile>/vitte
    Ok(())
}

fn cmd_env(opts: &GlobalOpts) -> io::Result<()> {
    println!("hôte: {}", host_triple());
    println!("target: {}", opts.target.as_deref().unwrap_or("<none>"));
    println!("PATH: {}", env::var("PATH").unwrap_or_default());
    Ok(())
}

/* ----------------------------- utilitaires ---------------------------- */

fn run(cmd: &mut Command) -> io::Result<()> {
    let status = cmd.status()?;
    if !status.success() {
        return Err(io::Error::new(io::ErrorKind::Other, format!("commande échouée: {status}")));
    }
    Ok(())
}

fn host_triple() -> &'static str {
    // Approximation compile-time
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

/* --------------------------------- main -------------------------------- */

fn main() {
    let mut args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        print_help();
        process::exit(1);
    }

    // Prendre la commande par valeur avant de modifier `args`
    let cmd: String = args[1].clone();
    // Consommer la commande puis parser les options globales restantes
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
                "list" => cmd_targets_list(&opts),
                "add" => {
                    if args.len() < 3 {
                        eprintln!("error: missing <triple> for `targets add`");
                        process::exit(1);
                    }
                    cmd_targets_add(&opts, &args[2])
                },
                "remove" => {
                    if args.len() < 3 {
                        eprintln!("error: missing <triple> for `targets remove`");
                        process::exit(1);
                    }
                    cmd_targets_remove(&opts, &args[2])
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
