//! vitte-autodiff — primitives de différenciation automatique pour Vitte.
//!
//! Ce crate est actuellement une ébauche. Il expose une façade minimale afin
//! de permettre l’intégration progressive de l’autodiff dans les autres
//! composants du workspace. Les API seront enrichies dans de prochains
//! changements.

#![forbid(unsafe_code)]
#![deny(missing_docs)]

/// Représente un moteur de différenciation automatique abstrait.
#[derive(Debug, Default, Clone, Copy)]
pub struct Engine;

impl Engine {
    /// Calcule la dérivée d’une fonction scalaire à une dimension via une
    /// approximation simple. Cette implémentation est volontairement naïve et
    /// sert uniquement de « stub ».
    pub fn derive<F>(f: F, x: f64) -> f64
    where
        F: Fn(f64) -> f64,
    {
        const H: f64 = 1e-6;
        let fx1 = f(x + H);
        let fx0 = f(x - H);
        (fx1 - fx0) / (2.0 * H)
    }
}

#[cfg(test)]
mod tests {
    use super::Engine;

    #[test]
    fn derive_polynomial() {
        let f = |x: f64| x * x * x; // x^3
        let deriv = Engine::derive(f, 2.0);
        // f'(x) = 3x^2, so at x = 2 -> 12
        assert!((deriv - 12.0).abs() < 1e-3);
    }
}
