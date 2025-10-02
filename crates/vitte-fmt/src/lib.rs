//! vitte-fmt — formateur de code source Vitte.
//!
//! Implémentation provisoire: renvoie l'entrée telle quelle. L'objectif est
//! surtout de fournir une surface API stable (`format_source`) pour le reste du
//! workspace tant que le formatter réel n'est pas développé.

#![forbid(unsafe_code)]
#![deny(missing_docs)]

/// Options minimales du formateur.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct FormatterOptions {
    /// Largeur cible (en colonnes) utilisée pour le wrapping heuristique.
    pub max_width: usize,
}

impl Default for FormatterOptions {
    fn default() -> Self {
        Self { max_width: 100 }
    }
}

/// Représente la sortie d’un passage de formateur.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct FormatOutput {
    /// Code formaté.
    pub code: String,
    /// Indique si des modifications ont été appliquées.
    pub changed: bool,
}

impl FormatOutput {
    /// Construit un résultat identique à l’entrée (aucun changement).
    pub fn identity(src: &str) -> Self {
        Self { code: src.to_string(), changed: false }
    }
}

/// Formate le code source Vitte fourni. Implémentation temporaire : renvoie
/// l’entrée sans modification.
pub fn format_source<S: AsRef<str>>(src: S, _options: FormatterOptions) -> FormatOutput {
    FormatOutput::identity(src.as_ref())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn identity_formatting() {
        let src = "fn main() { println!(\"hello\"); }";
        let out = format_source(src, FormatterOptions::default());
        assert_eq!(out.code, src);
        assert!(!out.changed);
    }
}
