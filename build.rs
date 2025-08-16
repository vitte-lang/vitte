// build.rs — messages post-build (install complétions) sans dépendances externes
use std::{env, path::PathBuf};

fn home_dir() -> Option<PathBuf> {
    // Évite dirs_next en build-deps : on reste léger et compatible.
    if cfg!(windows) {
        env::var_os("USERPROFILE")
            .map(PathBuf::from)
            .or_else(|| {
                let drive = env::var_os("HOMEDRIVE")?;
                let path = env::var_os("HOMEPATH")?;
                let mut p = PathBuf::from(drive);
                p.push(path);
                Some(p)
            })
    } else {
        env::var_os("HOME").map(PathBuf::from)
    }
}

fn main() {
    // Ne relance pas le script à chaque build inutilement
    println!("cargo:rerun-if-changed=build.rs");

    // Construit des chemins « best effort » ; l’install auto fait foi.
    let (bash_dir, zsh_dir, fish_dir, pwsh_dir, elv_dir) = if let Some(h) = home_dir() {
        let bash = h.join(".local/share/bash-completion/completions"); // Linux user
        let zsh  = h.join(".zsh/completions");                         // nécessite fpath+=
        let fish = h.join(".config/fish/completions");
        let pwsh = if cfg!(windows) {
            // Windows: scripts utilisateur PowerShell
            h.join("Documents/PowerShell/Scripts")
        } else {
            // PowerShell Core (Unix) : dossier scripts utilisateur
            h.join(".config/powershell/Scripts")
        };
        let elv  = h.join(".config/elvish/lib");
        (bash, zsh, fish, pwsh, elv)
    } else {
        (
            PathBuf::from("<home>/.local/share/bash-completion/completions"),
            PathBuf::from("<home>/.zsh/completions"),
            PathBuf::from("<home>/.config/fish/completions"),
            if cfg!(windows) {
                PathBuf::from("<home>/Documents/PowerShell/Scripts")
            } else {
                PathBuf::from("<home>/.config/powershell/Scripts")
            },
            PathBuf::from("<home>/.config/elvish/lib"),
        )
    };

    // Message convivial (affiché à la fin de la build)
    println!("cargo:warning=────────────────────────────────────────────────────────");
    println!("cargo:warning=  ✅ vitte installé ! Ajoute l’auto-complétion en 1 commande :");
    println!("cargo:warning=    vitte completions --install");
    println!("cargo:warning=  Ou génère pour un shell précis :");
    println!("cargo:warning=    vitte completions --shell bash --dir {}", bash_dir.display());
    println!("cargo:warning=    vitte completions --shell zsh  --dir {}", zsh_dir.display());
    println!("cargo:warning=    vitte completions --shell fish --dir {}", fish_dir.display());
    println!("cargo:warning=    vitte completions --shell powershell --dir {}", pwsh_dir.display());
    println!("cargo:warning=    vitte completions --shell elvish --dir {}", elv_dir.display());
    println!("cargo:warning=────────────────────────────────────────────────────────");
}

