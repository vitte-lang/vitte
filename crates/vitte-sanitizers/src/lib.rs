

#![deny(missing_docs)]
//! vitte-sanitizers — helpers pour AddressSanitizer, LeakSanitizer, ThreadSanitizer, MemorySanitizer, UBSan
//!
//! Fournit :
//! - Détection des flags activés (via `cfg`).
//! - Helpers pour imprimer/forcer les RUSTFLAGS adaptés.
//! - Intégration CI (afficher état).
//!
//! Exemple :
//! ```
//! use vitte_sanitizers as san;
//! println!("ASan actif ? {}", san::asan_enabled());
//! ```

use thiserror::Error;

/// Erreurs liées aux sanitizers.
#[derive(Debug, Error)]
pub enum SanError {
    #[error("autre: {0}")]
    Other(String),
}

/// Résultat spécialisé.
pub type Result<T> = std::result::Result<T, SanError>;

/// Retourne vrai si AddressSanitizer activé.
pub fn asan_enabled() -> bool {
    cfg!(feature = "asan")
}

/// Retourne vrai si LeakSanitizer activé.
pub fn lsan_enabled() -> bool {
    cfg!(feature = "lsan")
}

/// Retourne vrai si ThreadSanitizer activé.
pub fn tsan_enabled() -> bool {
    cfg!(feature = "tsan")
}

/// Retourne vrai si MemorySanitizer activé.
pub fn msan_enabled() -> bool {
    cfg!(feature = "msan")
}

/// Retourne vrai si UndefinedBehaviorSanitizer activé.
pub fn ubsan_enabled() -> bool {
    cfg!(feature = "ubsan")
}

/// Retourne vrai si mode durci activé.
pub fn hardened_enabled() -> bool {
    cfg!(feature = "hardened")
}

/// Retourne vrai si mode fuzz activé.
pub fn fuzz_enabled() -> bool {
    cfg!(feature = "fuzz")
}

/// Affiche un résumé des sanitizers actifs.
pub fn summary() -> String {
    let mut v = Vec::new();
    if asan_enabled() { v.push("ASan"); }
    if lsan_enabled() { v.push("LSan"); }
    if tsan_enabled() { v.push("TSan"); }
    if msan_enabled() { v.push("MSan"); }
    if ubsan_enabled() { v.push("UBSan"); }
    if hardened_enabled() { v.push("Hardened"); }
    if fuzz_enabled() { v.push("Fuzz"); }
    if v.is_empty() { "none".into() } else { v.join("+") }
}

/// Retourne les RUSTFLAGS recommandés pour activer un sanitizer donné.
pub fn rustflags_for(s: &str) -> Option<&'static str> {
    match s {
        "asan" => Some("-Zsanitizer=address"),
        "lsan" => Some("-Zsanitizer=leak"),
        "tsan" => Some("-Zsanitizer=thread"),
        "msan" => Some("-Zsanitizer=memory"),
        "ubsan" => Some("-Zsanitizer=undefined"),
        _ => None,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn summary_none() {
        let s = summary();
        assert!(!s.is_empty());
    }

    #[test]
    fn rustflags_lookup() {
        assert_eq!(rustflags_for("asan"), Some("-Zsanitizer=address"));
        assert_eq!(rustflags_for("foo"), None);
    }
}