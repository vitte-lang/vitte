// build.rs — affiche des instructions post-build pour installer les complétions
use std::{env, path::{PathBuf}};

fn path_or(env_key: &str, fallback: PathBuf) -> PathBuf {
    env::var_os(env_key).map(PathBuf::from).unwrap_or(fallback)
}

fn home_dir() -> PathBuf {
    dirs_next::home_dir()
        .or_else(|| env::var_os("HOME").map(PathBuf::from))
        .or_else(|| env::var_os("USERPROFILE").map(PathBuf::from))
        .unwrap_or_else(|| PathBuf::from("$HOME")) // n’expanse pas, mais lisible
}

fn xdg_data_home() -> PathBuf {
    if cfg!(windows) {
        // Sur Windows, %APPDATA% (~\AppData\Roaming) est l’équivalent le plus proche
        let home = home_dir();
        path_or("APPDATA", home.join("AppData").join("Roaming"))
    } else {
        path_or("XDG_DATA_HOME", home_dir().join(".local/share"))
    }
}

fn xdg_config_home() -> PathBuf {
    if cfg!(windows) {
        // Powershell/Fish/Elvish stockent souvent dans %APPDATA%\<tool>
        let home = home_dir();
        path_or("APPDATA", home.join("AppData").join("Roaming"))
    } else {
        path_or("XDG_CONFIG_HOME", home_dir().join(".config"))
    }
}

fn main() {
    // Afficher seulement quand on build le package principal (évite le spam si dépendance)
    if env::var("CARGO_PRIMARY_PACKAGE").is_err() {
        return;
    }

    // Aide Cargo à ne relancer que si ces variables changent
    println!("cargo:rerun-if-env-changed=HOME");
    println!("cargo:rerun-if-env-changed=USERPROFILE");
    println!("cargo:rerun-if-env-changed=APPDATA");
    println!("cargo:rerun-if-env-changed=XDG_DATA_HOME");
    println!("cargo:rerun-if-env-changed=XDG_CONFIG_HOME");

    let home = home_dir();
    let data = xdg_data_home();
    let cfg  = xdg_config_home();

    // Chemins “par défaut” (exemples) par shell
    let bash_user = data.join("bash-completion/completions"); // Linux, Git Bash
    let zsh_user  = home.join(".zsh/completions");            // per-user
    let fish_user = cfg.join("fish/completions");
    let pwsh_user = if cfg!(windows) {
        // Deux emplacements courants sous Windows (PowerShell 5/7)
        // 1) %APPDATA%\PowerShell\Modules
        // 2) ~/Documents/PowerShell/Modules
        // On affiche les deux plus bas.
        cfg.join("PowerShell/Modules")
    } else {
        // Linux/macOS PowerShell 7+ suit XDG
        cfg.join("powershell/Modules")
    };
    let pwsh_docs = if cfg!(windows) {
        dirs_next::document_dir()
            .unwrap_or_else(|| home.join("Documents"))
            .join("PowerShell/Modules")
    } else {
        // Pas pertinent hors Windows; on met un placeholder lisible.
        PathBuf::from("$HOME/Documents/PowerShell/Modules")
    };
    let elv_user  = cfg.join("elvish/lib");

    // Alternatives utiles selon plateforme
    let (bash_alt1, bash_alt2, zsh_alt1, zsh_alt2) = if cfg!(target_os = "macos") {
        // Homebrew Intel vs Apple Silicon
        (
            PathBuf::from("/usr/local/etc/bash_completion.d"),
            PathBuf::from("/opt/homebrew/etc/bash_completion.d"),
            PathBuf::from("/usr/local/share/zsh/site-functions"),
            PathBuf::from("/opt/homebrew/share/zsh/site-functions"),
        )
    } else {
        (
            PathBuf::from("/usr/share/bash-completion/completions"),
            PathBuf::from("/etc/bash_completion.d"),
            PathBuf::from("/usr/share/zsh/site-functions"),
            PathBuf::from("/usr/local/share/zsh/site-functions"),
        )
    };

    println!("cargo:warning=────────────────────────────────────────────────────────");
    println!("cargo:warning=  ✅ vitte installé !");
    println!("cargo:warning=  → Ajoute l’auto-complétion automatiquement :");
    println!("cargo:warning=      vitte completions --install");
    println!("cargo:warning=");
    println!("cargo:warning=  Exemples d’installation manuelle par shell (chemins usuels) :");
    // BASH
    println!("cargo:warning=    • bash");
    println!("cargo:warning=        user  : {}", bash_user.display());
    println!("cargo:warning=        alt 1 : {}", bash_alt1.display());
    println!("cargo:warning=        alt 2 : {}", bash_alt2.display());
    println!("cargo:warning=        cmd   : vitte completions --shell bash --dir {}", bash_user.display());
    // ZSH
    println!("cargo:warning=    • zsh");
    println!("cargo:warning=        user  : {}", zsh_user.display());
    println!("cargo:warning=        alt 1 : {}", zsh_alt1.display());
    println!("cargo:warning=        alt 2 : {}", zsh_alt2.display());
    println!("cargo:warning=        cmd   : vitte completions --shell zsh --dir {}", zsh_user.display());
    // FISH
    println!("cargo:warning=    • fish");
    println!("cargo:warning=        user  : {}", fish_user.display());
    println!("cargo:warning=        cmd   : vitte completions --shell fish --dir {}", fish_user.display());
    // POWERSHELL
    if cfg!(windows) {
        println!("cargo:warning=    • PowerShell (Windows)");
        println!("cargo:warning=        user  : {}", pwsh_user.display());
        println!("cargo:warning=        docs  : {}", pwsh_docs.display());
        println!("cargo:warning=        cmd   : vitte completions --shell powershell --dir {}", pwsh_user.display());
    } else {
        println!("cargo:warning=    • PowerShell (pwsh 7+)");
        println!("cargo:warning=        user  : {}", pwsh_user.display());
        println!("cargo:warning=        cmd   : vitte completions --shell powershell --dir {}", pwsh_user.display());
    }
    // ELVISH
    println!("cargo:warning=    • elvish");
    println!("cargo:warning=        user  : {}", elv_user.display());
    println!("cargo:warning=        cmd   : vitte completions --shell elvish --dir {}", elv_user.display());
    println!("cargo:warning=");
    println!("cargo:warning=  Astuce : après copie, ouvre un nouveau shell ou recharge ta config.");
    println!("cargo:warning=────────────────────────────────────────────────────────");
}
