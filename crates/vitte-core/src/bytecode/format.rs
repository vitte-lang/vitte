// crates/vitte-core/src/bytecode/format.rs
//! Outils de formatage/aperçu pour le pool de constantes du bytecode.
//!
//! Fournit :
//! - [`ConstTypeStats`] : stats par type (null/bool/i64/f64/str/bytes)
//! - [`count_consts`]    : comptage générique sur n'importe quel itérateur de &ConstValue
//! - [`const_preview`]   : aperçu lisible d'une constante (type + rendu raccourci)
//! - [`escape_for_preview`] : escape sûr pour affichage
//!
//! ⚠️ Ce module vit *dans le crate `vitte-core`* : on utilise `crate::...`,
//!    pas `vitte_core::...`.

use std::fmt::Write;

use crate::bytecode::chunk::ConstValue;

/// Statistiques de répartition des types de constantes.
#[derive(Debug, Default, Clone, Copy, PartialEq, Eq)]
pub struct ConstTypeStats {
    pub null:  usize,
    pub bools: usize,
    pub i64s:  usize,
    pub f64s:  usize,
    pub strs:  usize,
    pub bytes: usize,
}

impl ConstTypeStats {
    #[inline]
    pub fn total(&self) -> usize {
        self.null + self.bools + self.i64s + self.f64s + self.strs + self.bytes
    }
}

/// Compte les types de constantes depuis n'importe quel itérateur de `&ConstValue`.
///
/// Exemple :
/// ```ignore
/// let stats = count_consts(chunk.consts.iter().map(|(_, c)| c));
/// ```
pub fn count_consts<'a, I>(iter: I) -> ConstTypeStats
where
    I: IntoIterator<Item = &'a ConstValue>,
{
    let mut s = ConstTypeStats::default();
    for c in iter {
        match c {
            ConstValue::Null       => s.null  += 1,
            ConstValue::Bool(_)    => s.bools += 1,
            ConstValue::I64(_)     => s.i64s  += 1,
            ConstValue::F64(_)     => s.f64s  += 1,
            ConstValue::Str(_)     => s.strs  += 1,
            ConstValue::Bytes(_)   => s.bytes += 1,
        }
    }
    s
}

/// Retourne `(type, aperçu)` pour une constante, avec troncature contrôlée.
///
/// `max` s'applique à la longueur **échappée** (pour éviter d'afficher des
/// codes de contrôle crus).
pub fn const_preview(c: &ConstValue, max: usize) -> (&'static str, String) {
    match c {
        ConstValue::Null      => ("null", "null".into()),
        ConstValue::Bool(b)   => ("bool", format!("{b}")),
        ConstValue::I64(i)    => ("i64",  format!("{i}")),
        ConstValue::F64(x)    => ("f64",  format!("{x}")),
        ConstValue::Str(s)    => ("str",  preview_str(s, max)),
        ConstValue::Bytes(b)  => ("bytes", format!("len={}", b.len())),
    }
}

/// Escape une chaîne pour un aperçu “humain-safe”.
///
/// Gère guillemets, antislash et caractères de contrôle.
pub fn escape_for_preview(s: &str) -> String {
    let mut out = String::with_capacity(s.len());
    for ch in s.chars() {
        match ch {
            '\n' => out.push_str("\\n"),
            '\t' => out.push_str("\\t"),
            '\r' => out.push_str("\\r"),
            '"'  => out.push_str("\\\""),
            '\\' => out.push_str("\\\\"),
            c if c.is_control() => {
                let _ = write!(&mut out, "\\x{:02X}", c as u32);
            }
            c => out.push(c),
        }
    }
    out
}

/// Effectue un aperçu de `s` (échappé) tronqué à `max` caractères.
pub fn preview_str(s: &str, max: usize) -> String {
    let esc = escape_for_preview(s);
    if esc.len() <= max {
        format!("\"{esc}\"")
    } else {
        // Évite panique si max == 0
        let cut = esc.char_indices().nth(max).map(|(i, _)| i).unwrap_or(esc.len().min(max));
        format!("\"{}…\"", &esc[..cut])
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::bytecode::chunk::ConstValue::*;

    #[test]
    fn stats_counting() {
        let v = [Null, Bool(true), Bool(false), I64(1), F64(2.0), Str("ok".into()), Bytes(vec![1,2,3])];
        let stats = count_consts(v.iter());
        assert_eq!(stats.null, 1);
        assert_eq!(stats.bools, 2);
        assert_eq!(stats.i64s, 1);
        assert_eq!(stats.f64s, 1);
        assert_eq!(stats.strs, 1);
        assert_eq!(stats.bytes, 1);
        assert_eq!(stats.total(), v.len());
    }

    #[test]
    fn preview_string_trunc() {
        let s = "hello\nworld";
        let p = preview_str(s, 7);
        assert!(p.starts_with("\"hello\\n"));
        assert!(p.ends_with('…') || p.ends_with('"'));
    }

    #[test]
    fn const_preview_shapes() {
        let (t, v) = const_preview(&Null, 10);
        assert_eq!(t, "null");
        assert_eq!(v, "null");
        let (t, v) = const_preview(&Bool(true), 10);
        assert_eq!(t, "bool");
        assert_eq!(v, "true");
    }
}
