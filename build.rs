// build.rs — messages post-build (install complétions), zéro dépendance.
use std::{env, fs, path::{Path, PathBuf}};

fn home_dir() -> Option<PathBuf> {
    if cfg!(windows) {
        env::var_os("USERPROFILE")
            .map(PathBuf::from)
            .or_else(|| {
                let drive = env::var_os("HOMEDRIVE")?;
                let path  = env::var_os("HOMEPATH")?;
                let mut p = PathBuf::from(drive);
                p.push(path);
                Some(p)
            })
    } else {
        env::var_os("HOME").map(PathBuf::from)
    }
}

fn xdg_data_home() -> PathBuf {
    if cfg!(windows) {
        let home = home_dir().unwrap_or_else(|| PathBuf::from("C:\\"));
        env::var_os("APPDATA")
            .map(PathBuf::from)
            .unwrap_or_else(|| home.join("AppData").join("Roaming"))
    } else {
        env::var_os("XDG_DATA_HOME")
            .map(PathBuf::from)
            .unwrap_or_else(|| home_dir().unwrap_or_else(|| PathBuf::from("/")).join(".local/share"))
    }
}

fn xdg_config_home() -> PathBuf {
    if cfg!(windows) {
        let home = home_dir().unwrap_or_else(|| PathBuf::from("C:\\"));
        env::var_os("APPDATA")
            .map(PathBuf::from)
            .unwrap_or_else(|| home.join("AppData").join("Roaming"))
    } else {
        env::var_os("XDG_CONFIG_HOME")
            .map(PathBuf::from)
            .unwrap_or_else(|| home_dir().unwrap_or_else(|| PathBuf::from("/")).join(".config"))
    }
}

fn infer_bin_name() -> String {
    // 1) Permettre override : BIN_NAME=vitte cargo build
    if let Some(v) = env::var_os("BIN_NAME") {
        if !v.is_empty() { return v.to_string_lossy().into_owned(); }
    }
    // 2) Sinon, on part du nom du package et on “nettoie”
    let pkg = env::var("CARGO_PKG_NAME").unwrap_or_else(|_| "app".into());
    if pkg.ends_with("-cli") { pkg.trim_end_matches("-cli").to_string() }
    else if pkg.starts_with("vitte-") { pkg.trim_start_matches("vitte-").to_string() }
    else { pkg }
}

fn repo_root_from_manifest(manifest_dir: &Path) -> PathBuf {
    // Heuristique simple : le repo racine est le parent de `crates/<crate>` ou identique pour mono-crate
    // crates/vitte-cli/Cargo.toml → repo root = ../..
    manifest_dir
        .parent().and_then(|p| p.parent())
        .unwrap_or(manifest_dir)
        .to_path_buf()
}

fn main() {
    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    let repo_root    = repo_root_from_manifest(&manifest_dir);

    // Où l’on pourrait déjà trouver des complétions prégénérées (ex: cli-todo)
    let todo_compl = repo_root.join("cli-todo").join("completions");
    let have_todo  = todo_compl.is_dir() && fs::read_dir(&todo_compl).is_ok();

    let bin = infer_bin_name();

    // Emplacements recommandés
    let data = xdg_data_home();
    let conf = xdg_config_home();

    // UNIX usuels
    let bash_usr  = data.join("bash-completion").join("completions").join(&bin);
    let zfunc_usr = data.join("zsh").join("site-functions").join(format!("_{}", bin));
    let zconf_usr = conf.join("zsh").join("completions").join(format!("_{}", bin)); // fallback
    let fish_usr  = conf.join("fish").join("completions").join(format!("{}.fish", bin));

    // Windows (approximations raisonnables pour shells installés)
    // - Git Bash peut lire /etc/bash_completion.d ou %APPDATA%\bash-completion\completions
    let bash_win  = data.join("bash-completion").join("completions").join(&bin);
    // - Zsh sous Windows (MSYS/Cygwin) : réutilisons site-functions
    let zfunc_win = data.join("zsh").join("site-functions").join(format!("_{}", bin));
    // - Fish Windows : %APPDATA%\fish\completions
    let fish_win  = conf.join("fish").join("completions").join(format!("{}.fish", bin));

    // Messages post-build
    println!("cargo:warning=✔ Build ok — infos d’installation des complétions pour `{}` :", bin);

    if cfg!(windows) {
        println!("cargo:warning=  • Bash :   {:?}", bash_win);
        println!("cargo:warning=  • Zsh  :   {:?}", zfunc_win);
        println!("cargo:warning=  • Fish :   {:?}", fish_win);
    } else {
        println!("cargo:warning=  • Bash :   {:?}", bash_usr);
        // Zsh : on propose site-functions, puis un fallback local dans ~/.config
        println!("cargo:warning=  • Zsh  :   {:?}  (ou {:?})", zfunc_usr, zconf_usr);
        println!("cargo:warning=  • Fish :   {:?}", fish_usr);
    }

    if have_todo {
        println!("cargo:warning=ℹ Des complétions prégénérées existent : {:?}", todo_compl);
        println!("cargo:warning=   Copiez-les vers les chemins ci-dessus si vous voulez les activer globalement.");
    }

    // Optionnel : exposer un dossier artefacts local (pour CI ou scripts)
    let out_dir = PathBuf::from(env::var("OUT_DIR").unwrap());
    let stash   = Path::new(&out_dir).join("completions-hints.txt");
    let _ = fs::write(
        &stash,
        format!(
"bin={bin}
bash={:?}
zsh={:?}
fish={:?}
", 
            if cfg!(windows) { &bash_win } else { &bash_usr },
            if cfg!(windows) { &zfunc_win } else { &zfunc_usr },
            if cfg!(windows) { &fish_win } else { &fish_usr },
        ),
    );
}
