// build.rs — affiche des instructions post-build pour installer les complétions
// Ultra-complet : XDG/Windows/macOS(Homebrew), bash/zsh/fish/pwsh/elvish/nu, multi-bin,
// support DESTDIR/PREFIX pour packagers, respect des features Cargo.

use std::{env, path::PathBuf};

fn env_path_or(key: &str, fallback: PathBuf) -> PathBuf {
    env::var_os(key).map(PathBuf::from).unwrap_or(fallback)
}

fn home_dir() -> PathBuf {
    // nécessite dirs-next en build-dependency
    dirs_next::home_dir()
        .or_else(|| env::var_os("HOME").map(PathBuf::from))
        .or_else(|| env::var_os("USERPROFILE").map(PathBuf::from))
        .unwrap_or_else(|| PathBuf::from("$HOME"))
}

fn xdg_data_home() -> PathBuf {
    if cfg!(windows) {
        let home = home_dir();
        env_path_or("APPDATA", home.join("AppData").join("Roaming"))
    } else {
        env_path_or("XDG_DATA_HOME", home_dir().join(".local/share"))
    }
}

fn xdg_config_home() -> PathBuf {
    if cfg!(windows) {
        let home = home_dir();
        env_path_or("APPDATA", home.join("AppData").join("Roaming"))
    } else {
        env_path_or("XDG_CONFIG_HOME", home_dir().join(".config"))
    }
}

fn documents_dir() -> PathBuf {
    dirs_next::document_dir().unwrap_or_else(|| home_dir().join("Documents"))
}

fn homebrew_prefix() -> Option<PathBuf> {
    if let Some(p) = env::var_os("HOMEBREW_PREFIX") {
        return Some(PathBuf::from(p));
    }
    if cfg!(target_os = "macos") {
        if cfg!(target_arch = "aarch64") {
            Some(PathBuf::from("/opt/homebrew"))
        } else {
            Some(PathBuf::from("/usr/local"))
        }
    } else {
        None
    }
}

fn msys_git_prefix() -> Option<PathBuf> {
    // Heuristique Git Bash/MinGW sur Windows (utile pour bash-completion système)
    if cfg!(windows) {
        if let Some(root) = env::var_os("ProgramFiles") {
            let p = PathBuf::from(root).join("Git");
            return Some(p);
        }
    }
    None
}

fn cargo_feature_enabled(name: &str) -> bool {
    let key = format!("CARGO_FEATURE_{}", name.to_uppercase().replace('-', "_"));
    env::var_os(&key).is_some()
}

fn comma_split_env(key: &str) -> Vec<String> {
    env::var(key)
        .ok()
        .map(|s| s.split(',').map(|v| v.trim().to_string()).filter(|v| !v.is_empty()).collect())
        .unwrap_or_default()
}

fn default_bins() -> Vec<String> {
    // par défaut, on utilise le nom du package (converti _ → -)
    let pkg = env::var("CARGO_PKG_NAME").unwrap_or_else(|_| "vitte".into());
    let mut v = vec![pkg.replace('_', "-")];

    // override monobinaire
    if let Ok(bin) = env::var("VITTE_CLI_BIN") {
        if !bin.is_empty() {
            v = vec![bin];
        }
    }

    // override multi-bins
    let extra = comma_split_env("VITTE_CLI_BINS");
    if !extra.is_empty() {
        v = extra;
    }

    v.sort();
    v.dedup();
    v
}

fn main() {
    // éviter le bruit sur docs.rs
    if env::var("DOCS_RS").is_ok() {
        return;
    }
    // afficher seulement pour le package primaire
    if env::var("CARGO_PRIMARY_PACKAGE").is_err() {
        return;
    }
    // opt-out global
    if env::var("VITTE_NO_COMPLETION_HINT").is_ok() {
        return;
    }

    // Reruns conditionnels
    for k in [
        "HOME",
        "USERPROFILE",
        "APPDATA",
        "XDG_DATA_HOME",
        "XDG_CONFIG_HOME",
        "HOMEBREW_PREFIX",
        "ProgramFiles",
        "SHELL",
        "PREFIX",
        "DESTDIR",
        "VITTE_CLI_BIN",
        "VITTE_CLI_BINS",
        "VITTE_NO_COMPLETION_HINT",
        // features (ex: CARGO_FEATURE_CLI)
        "CARGO_FEATURE_CLI",
    ] {
        println!("cargo:rerun-if-env-changed={k}");
    }

    // Si le crate n'expose pas de CLI, message minimal
    let has_cli = cargo_feature_enabled("cli")
        || cargo_feature_enabled("run-cli")
        || cargo_feature_enabled("fmt-cli")
        || cargo_feature_enabled("check-cli")
        || cargo_feature_enabled("dump-cli")
        || cargo_feature_enabled("pack-cli")
        || cargo_feature_enabled("graph-cli");

    if !has_cli {
        println!("cargo:warning=ℹ️  {} construit (lib). Pas de CLI détectée → pas d'auto-complétion à installer.",
            env::var("CARGO_PKG_NAME").unwrap_or_else(|_| "crate".into()));
        return;
    }

    let bins = default_bins();
    let home = home_dir();
    let data = xdg_data_home();
    let cfg = xdg_config_home();

    // per-user (XDG) — chemins de complétions
    let bash_user = data.join("bash-completion/completions");
    let zsh_user = data.join("zsh/site-functions"); // recommandé (dans $fpath si configuré)
    let fish_user = cfg.join("fish/completions");
    let pwsh_user =
        if cfg!(windows) { cfg.join("PowerShell/Modules") } else { cfg.join("powershell/Modules") };
    let pwsh_docs = if cfg!(windows) {
        documents_dir().join("PowerShell/Modules")
    } else {
        home.join(".local/share/powershell/Modules")
    };
    let elv_user = cfg.join("elvish/lib");
    let nu_user = cfg.join("nushell/completions");

    // alternatives système
    let (bash_alt1, bash_alt2, zsh_alt1, zsh_alt2) = if cfg!(target_os = "macos") {
        let hb = homebrew_prefix().unwrap_or_else(|| PathBuf::from("/opt/homebrew"));
        (
            hb.join("etc/bash_completion.d"),
            PathBuf::from("/etc/bash_completion.d"),
            hb.join("share/zsh/site-functions"),
            PathBuf::from("/usr/local/share/zsh/site-functions"),
        )
    } else {
        (
            PathBuf::from("/usr/share/bash-completion/completions"),
            PathBuf::from("/etc/bash_completion.d"),
            PathBuf::from("/usr/share/zsh/site-functions"),
            PathBuf::from("/usr/local/share/zsh/site-functions"),
        )
    };

    // Git Bash (Windows) — emplacements utiles
    let git_bash = msys_git_prefix().map(|p| {
        (p.join("usr/share/bash-completion/completions"), p.join("etc/bash_completion.d"))
    });

    // DESTDIR/PREFIX pour packaging
    let prefix = env::var_os("PREFIX").map(PathBuf::from).unwrap_or_else(|| {
        if cfg!(target_os = "macos") {
            homebrew_prefix().unwrap_or_else(|| PathBuf::from("/usr/local"))
        } else {
            PathBuf::from("/usr")
        }
    });
    let destdir = env::var_os("DESTDIR").map(PathBuf::from).unwrap_or_else(|| PathBuf::from(""));

    let pkg = env::var("CARGO_PKG_NAME").unwrap_or_else(|_| "vitte".into());

    println!("cargo:warning=━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    println!("cargo:warning=  ✅ Build OK : {pkg}");
    println!("cargo:warning=  ➜ Auto-complétion : exécute une des commandes ci-dessous.");
    println!("cargo:warning=");

    // pour chaque binaire déclaré
    for bin in bins {
        println!("cargo:warning=  ▸ Binaire : {bin}");
        println!("cargo:warning=    • Installation automatique (si supportée par le binaire) :");
        println!("cargo:warning=        {bin} completions --install");
        println!("cargo:warning=");

        // BASH
        println!("cargo:warning=    • bash");
        println!("cargo:warning=        user  : {}", bash_user.display());
        println!("cargo:warning=        alt 1 : {}", bash_alt1.display());
        println!("cargo:warning=        alt 2 : {}", bash_alt2.display());
        if let Some((gb1, gb2)) = &git_bash {
            println!("cargo:warning=        Git Bash : {}  (ou  {})", gb1.display(), gb2.display());
        }
        println!(
            "cargo:warning=        cmd   : {bin} completions --shell bash --dir {}",
            bash_user.display()
        );

        // ZSH
        println!("cargo:warning=    • zsh");
        println!("cargo:warning=        user  : {}", zsh_user.display());
        println!("cargo:warning=        alt 1 : {}", zsh_alt1.display());
        println!("cargo:warning=        alt 2 : {}", zsh_alt2.display());
        println!(
            "cargo:warning=        cmd   : {bin} completions --shell zsh --dir {}",
            zsh_user.display()
        );
        println!("cargo:warning=        note  : ajoute ce dossier au $fpath puis `autoload -Uz compinit && compinit`");

        // FISH
        println!("cargo:warning=    • fish");
        println!("cargo:warning=        user  : {}", fish_user.display());
        println!(
            "cargo:warning=        cmd   : {bin} completions --shell fish --dir {}",
            fish_user.display()
        );

        // POWERSHELL
        if cfg!(windows) {
            println!("cargo:warning=    • PowerShell (Windows)");
            println!("cargo:warning=        user  : {}", pwsh_user.display());
            println!("cargo:warning=        docs  : {}", pwsh_docs.display());
            println!(
                "cargo:warning=        cmd   : {bin} completions --shell powershell --dir {}",
                pwsh_user.display()
            );
        } else {
            println!("cargo:warning=    • PowerShell (pwsh 7+)");
            println!("cargo:warning=        user  : {}", pwsh_user.display());
            println!(
                "cargo:warning=        cmd   : {bin} completions --shell powershell --dir {}",
                pwsh_user.display()
            );
        }

        // ELVISH
        println!("cargo:warning=    • elvish");
        println!("cargo:warning=        user  : {}", elv_user.display());
        println!(
            "cargo:warning=        cmd   : {bin} completions --shell elvish --dir {}",
            elv_user.display()
        );

        // NUSHELL
        println!("cargo:warning=    • nushell");
        println!("cargo:warning=        user  : {}", nu_user.display());
        println!(
            "cargo:warning=        cmd   : {bin} completions --shell nu --dir {}",
            nu_user.display()
        );

        println!("cargo:warning=");
    }

    // Packagers: chemins orientés FHS (PREFIX/DESTDIR)
    let bash_sys = prefix.join("share/bash-completion/completions");
    let zsh_sys = prefix.join("share/zsh/site-functions");
    let fish_sys = prefix.join("share/fish/vendor_completions.d");
    let pwsh_sys = if cfg!(windows) {
        // généralement géré par l’installeur (module PowerShell)
        PathBuf::from("%ProgramFiles%/PowerShell/7/Modules")
    } else {
        prefix.join("share/powershell/Modules")
    };
    let nu_sys = prefix.join("share/nushell/completions");

    let d = if destdir.as_os_str().is_empty() { PathBuf::from("") } else { destdir.clone() };
    println!("cargo:warning=  Packagers (PREFIX/DESTDIR) :");
    println!("cargo:warning=    bash : {}/{}/", d.display(), bash_sys.display());
    println!(
        "cargo:warning=    zsh  : {}/{}/  (fichier `_{}')",
        d.display(),
        zsh_sys.display(),
        pkg
    );
    println!("cargo:warning=    fish : {}/{}/", d.display(), fish_sys.display());
    println!("cargo:warning=    pwsh : {}/{}/", d.display(), pwsh_sys.display());
    println!("cargo:warning=    nu   : {}/{}/", d.display(), nu_sys.display());

    println!("cargo:warning=");
    println!("cargo:warning=  Astuces :");
    println!("cargo:warning=    - Crée le dossier si besoin (mkdir -p …).");
    println!("cargo:warning=    - Ouvre un nouveau terminal ou recharge le shell :");
    println!("cargo:warning=        bash : `source ~/.bashrc`  | zsh : `exec zsh`");
    println!("cargo:warning=        fish : `exec fish`         | pwsh : `.$PROFILE`");
    println!("cargo:warning=        nu   : `exec nu`           | elvish : `exec elvish`");
    println!("cargo:warning=━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
}
