//! vitte-fuzz — outillage de fuzzing pour le langage Vitte.
//!
//! Ce crate est pour l’instant une façade minimale afin de laisser place aux
//! futures intégrations avec différents moteurs de fuzzing. Les vraies cibles
//! (lexer, parser, VM, etc.) seront branchées progressivement.

#![forbid(unsafe_code)]
#![deny(missing_docs)]

/// Configuration générique partagée par les stratégies de fuzzing.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct FuzzConfig {
    /// Taille maximale générée pour les corpus.
    pub max_corpus_bytes: usize,
    /// Nombre maximal d’itérations pour les campagnes simples.
    pub max_iterations: usize,
}

impl Default for FuzzConfig {
    fn default() -> Self {
        Self { max_corpus_bytes: 64 * 1024, max_iterations: 1_000 }
    }
}

/// Résultat symbolique d’une campagne de fuzzing.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum FuzzStatus {
    /// Aucun crash détecté dans les limites configurées.
    Clean,
    /// Un crash s’est produit et doit être repro.
    CrashFound,
    /// L’exécution a atteint une limite (temps ou itérations).
    Timeout,
}

/// Lance un fuzzing "stub" qui ne fait que retourner [`FuzzStatus::Clean`].
pub fn run_stub(_config: &FuzzConfig) -> FuzzStatus { FuzzStatus::Clean }

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn stub_is_clean() {
        let status = run_stub(&FuzzConfig::default());
        assert_eq!(status, FuzzStatus::Clean);
    }
}
