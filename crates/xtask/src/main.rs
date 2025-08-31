//! xtask — tâches utilitaires pour le workspace Vitte
//!
//! Usage :
//!   cargo run -p xtask -- <commande> [options]
//!
//! Exemples :
//!   cargo run -p xtask -- fmt --check
//!   cargo run -p xtask -- lint --fix
//!   cargo run -p xtask -- build -p vitte-compiler --release
//!   cargo run -p xtask -- test --release
//!   cargo run -p xtask -- cov
//!   cargo run -p xtask -- audit
//!   cargo run -p xtask -- wasm -p vitte-lexer
//!   cargo run -p xtask -- no-std -p vitte-core
//!   cargo run -p xtask -- release 0.1.0 --dry
//!   cargo run -p xtask -- graph

use std::env;
use std::path::Path;
use std::process::{exit, Command, Stdio};

use clap::{ArgAction, Parser, Subcommand};

/// Outil de tâches pour le workspace Vitte
#[derive(Parser, Debug)]
#[command(author, version, about, propagate_version = true)]
struct Cli {
    /// Plus de logs
    #[arg(long, global = true, action = ArgAction::Count)]
    verbose: u8,

    #[command(subcommand)]
    command: Commands,
}

#[derive(Subcommand, Debug)]
enum Commands {
    /// Formatter tout le workspace (rustfmt)
    Fmt {
        /// N'écrit rien, vérifie seulement
        #[arg(long)]
        check: bool,
    },

    /// Clippy sur tout le workspace
    Lint {
        /// Applique des fixes automatiques où possible
        #[arg(long)]
        fix: bool,
        /// Lancer avec --no-default-features
        #[arg(long)]
        no_default_features: bool,
        /// Features à activer (séparées par virgules)
        #[arg(long)]
        features: Option<String>,
    },

    /// Compiler le workspace (ou un crate)
    Build {
        /// Paquet (ex: vitte-compiler)
        #[arg(short, long)]
        package: Option<String>,
        /// --release
        #[arg(long)]
        release: bool,
        /// --target <triple>
        #[arg(long)]
        target: Option<String>,
        /// --no-default-features
        #[arg(long)]
        no_default_features: bool,
        /// --features "a,b,c"
        #[arg(long)]
        features: Option<String>,
    },

    /// Tests (workspace)
    Test {
        #[arg(long)]
        release: bool,
        /// Laisse s'afficher les prints
        #[arg(long)]
        nocapture: bool,
        /// Filtrer les tests (passé à cargo test)
        #[arg(long)]
        filter: Option<String>,
        /// Activer des features
        #[arg(long)]
        features: Option<String>,
    },

    /// Benchs (si présents)
    Bench {
        #[arg(long)]
        release: bool,
    },

    /// Générer la doc
    Doc {
        /// Ouvrir dans le navigateur
        #[arg(long)]
        open: bool,
    },

    /// Nettoyage (cargo clean)
    Clean,

    /// Audit de sécurité (cargo-audit)
    Audit,

    /// Vérification des policies (cargo-deny)
    Deny,

    /// Détection de deps inutilisées (cargo-udeps)
    Udeps {
        /// Paquet ciblé
        #[arg(short, long)]
        package: Option<String>,
    },

    /// Miri : tests interprétés pour UB
    Miri {
        /// Filtrer les tests
        #[arg(long)]
        filter: Option<String>,
    },

    /// Couverture avec cargo-llvm-cov
    Cov,

    /// Build wasm32-unknown-unknown pour un crate
    Wasm {
        /// Paquet ciblé (obligatoire)
        #[arg(short, long)]
        package: String,
        /// --release
        #[arg(long)]
        release: bool,
    },

    /// Build en no_std (sans std) pour un crate
    NoStd {
        /// Paquet ciblé (ex: vitte-core)
        #[arg(short, long)]
        package: String,
        /// --release
        #[arg(long)]
        release: bool,
        /// Target (par ex. thumbv7em-none-eabihf)
        #[arg(long)]
        target: Option<String>,
    },

    /// Graphe des dépendances (rapide)
    Graph,

    /// Release : tag + changelog (basique) + (optionnel) publish
    Release {
        /// Version à publier (ex: 0.1.0)
        version: String,
        /// Ne fait rien, affiche seulement les actions
        #[arg(long)]
        dry: bool,
        /// Signe la release (git tag -s)
        #[arg(long)]
        sign: bool,
        /// Publie sur crates.io (tous les crates du workspace)
        #[arg(long)]
        publish: bool,
    },
}

fn main() {
    let cli = Cli::parse();

    let verbose = cli.verbose > 0;
    let res = match cli.command {
        Commands::Fmt { check } => fmt(check, verbose),
        Commands::Lint {
            fix,
            no_default_features,
            features,
        } => lint(fix, no_default_features, features.as_deref(), verbose),
        Commands::Build {
            package,
            release,
            target,
            no_default_features,
            features,
        } => build(
            package.as_deref(),
            release,
            target.as_deref(),
            no_default_features,
            features.as_deref(),
            verbose,
        ),
        Commands::Test {
            release,
            nocapture,
            filter,
            features,
        } => test_cmd(
            release,
            nocapture,
            filter.as_deref(),
            features.as_deref(),
            verbose,
        ),
        Commands::Bench { release } => bench(release, verbose),
        Commands::Doc { open } => doc(open, verbose),
        Commands::Clean => run("cargo", &["clean"], verbose),
        Commands::Audit => audit(verbose),
        Commands::Deny => deny(verbose),
        Commands::Udeps { package } => udeps(package.as_deref(), verbose),
        Commands::Miri { filter } => miri(filter.as_deref(), verbose),
        Commands::Cov => cov(verbose),
        Commands::Wasm { package, release } => wasm(&package, release, verbose),
        Commands::NoStd {
            package,
            release,
            target,
        } => nostd(&package, release, target.as_deref(), verbose),
        Commands::Graph => graph(verbose),
        Commands::Release {
            version,
            dry,
            sign,
            publish,
        } => release(&version, dry, sign, publish, verbose),
    };

    if let Err(code) = res {
        exit(code);
    }
}

/* ─────────────────────────── Impl commandes ─────────────────────────── */

fn fmt(check: bool, v: bool) -> Result<(), i32> {
    if check {
        run("cargo", &["fmt", "--all", "--", "--check"], v)
    } else {
        run("cargo", &["fmt", "--all"], v)
    }
}

fn lint(fix: bool, ndf: bool, features: Option<&str>, v: bool) -> Result<(), i32> {
    let mut args = vec!["clippy", "--workspace", "--all-targets"];
    if !ndf {
        args.push("--all-features");
    } else {
        args.push("--no-default-features");
        if let Some(f) = features {
            args.push("--features");
            args.push(f);
        }
    }
    if fix {
        args.push("--fix");
        args.push("--allow-dirty");
    }
    args.extend_from_slice(&["--", "-D", "warnings"]);
    run("cargo", &args, v)
}

fn build(
    pkg: Option<&str>,
    rel: bool,
    target: Option<&str>,
    ndf: bool,
    features: Option<&str>,
    v: bool,
) -> Result<(), i32> {
    let mut args = vec!["build"];
    if let Some(p) = pkg {
        args.push("-p");
        args.push(p);
    }
    if rel {
        args.push("--release");
    }
    if let Some(t) = target {
        args.push("--target");
        args.push(t);
    }
    if ndf {
        args.push("--no-default-features");
        if let Some(f) = features {
            args.push("--features");
            args.push(f);
        }
    } else {
        args.push("--all-features");
    }
    run("cargo", &args, v)
}

fn test_cmd(
    rel: bool,
    nocapture: bool,
    filter: Option<&str>,
    features: Option<&str>,
    v: bool,
) -> Result<(), i32> {
    let mut args = vec!["test", "--workspace", "--all-targets"];
    if rel {
        args.push("--release");
    }
    // features
    if let Some(f) = features {
        args.push("--features");
        args.push(f);
    } else {
        args.push("--all-features");
    }
    if nocapture {
        args.push("--");
        args.push("--nocapture");
    }
    if let Some(f) = filter {
        if nocapture {
            // déjà un "--" plus haut
            args.push(f);
        } else {
            args.push("--");
            args.push(f);
        }
    }
    run("cargo", &args, v)
}

fn bench(rel: bool, v: bool) -> Result<(), i32> {
    let mut args = vec!["bench", "--workspace"];
    if rel {
        args.push("--release");
    }
    run("cargo", &args, v)
}

fn doc(open: bool, v: bool) -> Result<(), i32> {
    let mut args = vec!["doc", "--workspace", "--no-deps", "--all-features"];
    run("cargo", &args, v)?;
    if open {
        // open le fichier index de la racine du workspace
        let path = "target/doc/index.html";
        if Path::new(path).exists() {
            open_file(path, v);
        } else {
            eprintln!("(info) doc: {} introuvable (selon le layout local)", path);
        }
    }
    Ok(())
}

fn audit(v: bool) -> Result<(), i32> {
    ensure_tool("cargo-audit")?;
    run("cargo", &["audit"], v)
}

fn deny(v: bool) -> Result<(), i32> {
    ensure_tool("cargo-deny")?;
    run("cargo", &["deny", "check"], v)
}

fn udeps(pkg: Option<&str>, v: bool) -> Result<(), i32> {
    ensure_tool("cargo-udeps")?;
    let mut args = vec!["udeps", "--workspace"];
    if let Some(p) = pkg {
        args.push("-p");
        args.push(p);
    }
    run("cargo", &args, v)
}

fn miri(filter: Option<&str>, v: bool) -> Result<(), i32> {
    // Prépare l'outil
    run("rustup", &["+nightly", "component", "add", "miri"], v).ok();
    run("cargo", &["+nightly", "miri", "setup"], v)?;
    let mut args = vec!["+nightly", "miri", "test", "--workspace"];
    if let Some(f) = filter {
        args.push("--");
        args.push(f);
    }
    run("cargo", &args, v)
}

fn cov(v: bool) -> Result<(), i32> {
    ensure_tool("cargo-llvm-cov")?;
    run("cargo", &["llvm-cov", "clean", "--workspace"], v)?;
    run(
        "cargo",
        &[
            "llvm-cov",
            "test",
            "--workspace",
            "--all-features",
            "--lcov",
            "--output-path",
            "lcov.info",
        ],
        v,
    )
}

fn wasm(pkg: &str, rel: bool, v: bool) -> Result<(), i32> {
    let mut args = vec![
        "build",
        "-p",
        pkg,
        "--target",
        "wasm32-unknown-unknown",
        "--no-default-features",
    ];
    if rel {
        args.push("--release");
    }
    run("cargo", &args, v)
}

fn nostd(pkg: &str, rel: bool, target: Option<&str>, v: bool) -> Result<(), i32> {
    let mut args = vec!["build", "-p", pkg, "--no-default-features"];
    if rel {
        args.push("--release");
    }
    if let Some(t) = target {
        args.push("--target");
        args.push(t);
    }
    run("cargo", &args, v)
}

fn graph(v: bool) -> Result<(), i32> {
    // Vue rapide via cargo tree (pas besoin de dot)
    run("cargo", &["tree", "--edges", "normal,build,dev"], v)
}

fn release(version: &str, dry: bool, sign: bool, publish: bool, v: bool) -> Result<(), i32> {
    preflight_git(v)?;

    let tag = format!("v{}", version);

    // Tag
    if sign {
        run_preview(
            "git",
            &["tag", "-s", &tag, "-m", &format!("Release {}", version)],
            dry,
            v,
        )?;
    } else {
        run_preview("git", &["tag", &tag], dry, v)?;
    }
    run_preview("git", &["push", "origin", &tag], dry, v)?;

    // Changelog minimal
    println!("\n# Release {version}\n");
    let _ = print_changelog_since_last_tag();

    // Publish (optionnel) — crates ordonnés par dépendances, on peut juste tenter workspace
    if publish {
        run_preview(
            "cargo",
            &["publish", "--workspace", "--all-features"],
            dry,
            v,
        )?;
    }

    Ok(())
}

/* ─────────────────────────── Helpers ─────────────────────────── */

fn print_changelog_since_last_tag() -> Result<(), i32> {
    let out = Command::new("git")
        .args(["describe", "--tags", "--abbrev=0"])
        .output()
        .unwrap_or_else(|_| panic!("git describe failed"));
    if !out.status.success() {
        println!("(info) aucun tag précédent ; changelog depuis l'origine :\n");
        run(
            "git",
            &["log", "--pretty=format:* %h %s", "--no-merges"],
            true,
        )?;
        return Ok(());
    }
    let last = String::from_utf8_lossy(&out.stdout).trim().to_string();
    println!("(depuis {last})\n");
    run(
        "git",
        &[
            "log",
            &format!("{}..HEAD", last),
            "--pretty=format:* %h %s",
            "--no-merges",
        ],
        true,
    )
}

fn preflight_git(v: bool) -> Result<(), i32> {
    run("git", &["diff", "--exit-code"], v)?;
    run("git", &["diff", "--cached", "--exit-code"], v)?;
    run("git", &["fetch", "--tags"], v).ok();
    Ok(())
}

/// Exécute une commande et stream stdout/stderr.
fn run(cmd: &str, args: &[&str], verbose: bool) -> Result<(), i32> {
    if verbose {
        eprintln!("→ {} {}", cmd, args.join(" "));
    }
    let status = Command::new(cmd)
        .args(args)
        .stdin(Stdio::inherit())
        .stdout(Stdio::inherit())
        .stderr(Stdio::inherit())
        .status()
        .map_err(|_| {
            eprintln!("✗ impossible d'exécuter `{}` — est-il installé ?", cmd);
            127
        })?;
    if !status.success() {
        return Err(status.code().unwrap_or(1));
    }
    Ok(())
}

/// Exécute une commande… ou **simule** si `dry == true`.
fn run_preview(cmd: &str, args: &[&str], dry: bool, verbose: bool) -> Result<(), i32> {
    if dry {
        println!("[dry-run] {} {}", cmd, args.join(" "));
        Ok(())
    } else {
        run(cmd, args, verbose)
    }
}

/// Vérifie qu’un binaire est dispo dans le PATH, sinon affiche un tip.
fn ensure_tool(tool: &str) -> Result<(), i32> {
    let ok = which(tool);
    if !ok {
        eprintln!("✗ Outil requis introuvable: `{}`.\n  → installe-le (ex: `cargo install {}` ou `brew install {}`)", tool, tool, tool);
        return Err(127);
    }
    Ok(())
}

/// Pauvre équivalent de `which` (sans dépendance).
fn which(bin: &str) -> bool {
    env::var_os("PATH")
        .and_then(|paths| {
            env::split_paths(&paths).find_map(|p| {
                let full = p.join(bin);
                if cfg!(windows) {
                    // On tente .exe aussi
                    let full_exe = p.join(format!("{}.exe", bin));
                    if full.exists() || full_exe.exists() {
                        Some(())
                    } else {
                        None
                    }
                } else if full.exists() {
                    Some(())
                } else {
                    None
                }
            })
        })
        .is_some()
}

/// Ouvre un fichier/URL avec la commande du système.
fn open_file(path: &str, verbose: bool) {
    let (cmd, args): (&str, &[&str]) = if cfg!(target_os = "macos") {
        ("open", &[path])
    } else if cfg!(target_os = "windows") {
        ("cmd", &["/C", "start", "", path])
    } else {
        ("xdg-open", &[path])
    };
    let _ = run(cmd, args, verbose);
}
