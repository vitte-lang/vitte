

//! vitte-gui — petite API stable pour l’UI de Vitte.
//!
//! Ce crate expose une surface minimale (sans dépendances externes)
//! afin d’éviter les warnings et erreurs de build dans le workspace.
//! Il pourra être étendu plus tard avec une vraie implémentation GUI.

#![deny(missing_docs, rust_2018_idioms)]
#![cfg_attr(not(feature = "std"), no_std)]

/// Alias de résultat local.
pub type Result<T, E = GuiError> = core::result::Result<T, E>;

/// Erreurs possibles du sous-système GUI.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum GuiError {
    /// Fonctionnalité non encore disponible.
    Unsupported,
}

/// Configuration minimale de l’UI.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct GuiConfig {
    /// Mode sans affichage (utile pour les tests/CI).
    pub headless: bool,
}

impl Default for GuiConfig {
    fn default() -> Self { Self { headless: false } }
}

/// Initialise l’UI (stub sans effet pour l’instant).
#[inline]
pub fn init(_cfg: GuiConfig) -> Result<()> { Ok(()) }

/// Retourne la version du crate.
#[cfg(feature = "std")]
#[inline]
pub fn version() -> &'static str { env!("CARGO_PKG_VERSION") }

/// État d’application minimal.
#[derive(Debug, Default)]
pub struct AppState {
    /// Nombre de frames « rendues » (compteur fictif).
    frames: u64,
}

impl AppState {
    /// Avance d’une « frame ».
    #[inline]
    pub fn tick(&mut self) { self.frames = self.frames.saturating_add(1); }

    /// Lit le compteur de frames.
    #[inline]
    pub fn frames(&self) -> u64 { self.frames }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn api_works() {
        let _ = init(GuiConfig::default()).unwrap();
        let mut st = AppState::default();
        st.tick();
        assert_eq!(st.frames(), 1);
    }
}