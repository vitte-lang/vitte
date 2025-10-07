

//! vitte-bin — library surface
//!
//! Ce crate expose une petite API stable pour d'autres crates/binaries du workspace.
//! Il ne dépend d'aucun code du `main.rs` et reste optionnel.

use std::borrow::Cow;

/// Alias de résultat standard pour l'API publique
pub type Result<T> = anyhow::Result<T>;

/// Version du package (depuis Cargo)
pub const VERSION: &str = env!("CARGO_PKG_VERSION");

/// Retourne une chaîne de version enrichie (commit/date si disponibles)
pub fn version_string() -> String {
    let commit = option_env!("GIT_COMMIT").unwrap_or("unknown");
    let build_date = option_env!("VERGEN_BUILD_DATE").unwrap_or("unknown");
    let rustc = option_env!("VERGEN_RUSTC_SEMVER").unwrap_or(env!("CARGO_PKG_VERSION"));
    format!(
        "vitte {}\ncommit: {commit}\nbuild: {build_date}\nrustc: {rustc}",
        env!("CARGO_PKG_VERSION")
    )
}

/// Nom du binaire principal généré par ce crate
pub const BIN_NAME: &str = "vitte-bin";

/// Détecte si un fichier est un bytecode `.vtbc` (case-insensitive).
pub fn is_bytecode(path: impl AsRef<std::path::Path>) -> bool {
    path.as_ref()
        .extension()
        .and_then(|e| e.to_str())
        .map(|e| e.eq_ignore_ascii_case("vtbc"))
        .unwrap_or(false)
}

/// Résout le chemin de sortie en fonction de l'entrée et du type d'artefact.
/// `emit` peut valoir "bc" | "obj" | "exe".
pub fn resolve_output(input: &std::path::Path, emit: &str) -> std::path::PathBuf {
    match emit {
        "obj" => input.with_extension("o"),
        "exe" => {
            if cfg!(windows) { input.with_extension("exe") } else { input.with_extension("") }
        }
        _ => input.with_extension("vtbc"),
    }
}

/// Retourne un nom humain lisible pour une cible éventuelle.
pub fn pretty_target(triple: Option<&str>) -> Cow<'_, str> {
    triple.map(Cow::from).unwrap_or_else(|| Cow::from(std::env::consts::ARCH))
}

// ────────────────────────────────────────────────────────────────────
// Réexport optionnel des crates moteur quand la feature `engine` est active
// ────────────────────────────────────────────────────────────────────
#[cfg(feature = "engine")]
pub mod engine {
    pub use vitte_compiler as compiler;
    pub use vitte_fmt as fmt;
    pub use vitte_tools as tools;
    pub use vitte_vm as vm;
    pub use vitte_modules as modules;
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn version_not_empty() {
        assert!(!VERSION.is_empty());
    }

    #[test]
    fn bytecode_detection() {
        assert!(is_bytecode("a.vtbc"));
        assert!(is_bytecode("A.VTBC"));
        assert!(!is_bytecode("main.vt"));
    }

    #[test]
    fn resolve_out() {
        use std::path::Path;
        let p = Path::new("main.vt");
        assert!(resolve_output(p, "bc").to_string_lossy().contains(".vtbc"));
    }
}