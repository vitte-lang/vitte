//! vitte-debugger — Bibliothèque de débogage pour le langage Vitte
//!
//! Fournit :
//! - Structures DAP (api.rs)
//! - Gestion des breakpoints (breakpoint.rs)
//! - Évaluations simples (eval.rs)
//! - Système de hooks VM → source (hook.rs)
//! - État centralisé du débogueur (state.rs)
//! - Contrôleur de stepping (step.rs)
//!
//! Destiné à être utilisé par :
//! - `vitte-dap` (adaptateur Debug Adapter Protocol)
//! - `vitte-cli --debug`
//! - tout outil externe voulant piloter la VM Vitte.

pub mod api;
pub mod breakpoint;
pub mod eval;
pub mod hook;
pub mod state;
pub mod step;

/// Prelude : importer d’un coup les traits et types utiles
pub mod prelude {
    pub use crate::api::*;
    pub use crate::breakpoint::*;
    pub use crate::eval::*;
    pub use crate::hook::*;
    pub use crate::state::*;
    pub use crate::step::*;
}

#[cfg(test)]
mod tests {
    use super::prelude::*;

    #[test]
    fn sanity_compile() {
        // Juste vérifier que tout compile et que les modules sont accessibles.
        let _ = StepMode::Into;
        let _ = Breakpoint { id: 1, source: "file.vitte".into(), line: 42, verified: true };
    }
}
