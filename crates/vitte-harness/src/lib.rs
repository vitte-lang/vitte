//! vitte-harness — infrastructure de test et benchmarks partagée.
//!
//! Le crate fournit pour l’instant des stubs permettant aux autres composants
//! de compiler. Les helpers concrets (runner test, bench CLI) seront ajoutés au
//! fur et à mesure.

#![forbid(unsafe_code)]
#![deny(missing_docs)]

/// Configuration minimale d’un scénario de test.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct TestConfig {
    /// Libellé humain affiché lors de l’exécution.
    pub label: String,
    /// Nombre maximal d’itérations.
    pub iterations: usize,
}

impl Default for TestConfig {
    fn default() -> Self {
        Self { label: "default".into(), iterations: 1 }
    }
}

/// Résultat simplifié d’un test.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TestResult {
    /// Test réussi.
    Passed,
    /// Test échoué.
    Failed,
}

/// Stub qui exécute un test en retournant systématiquement `Passed`.
pub fn run_stub(_config: &TestConfig) -> TestResult {
    TestResult::Passed
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn stub_passes() {
        let res = run_stub(&TestConfig::default());
        assert_eq!(res, TestResult::Passed);
    }
}
