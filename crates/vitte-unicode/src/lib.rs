#![deny(missing_docs)]
//! vitte-unicode — utilitaires Unicode pour Vitte
//!
//! Fournit :
//! - Normalisation NFC/NFD/NFKC/NFKD, casefold.
//! - Segmentation graphemes / mots.
//! - Algorithme bidirectionnel (BiDi).
//! - Largeur d’affichage.
//! - ICU4X optionnel.
//!
//! Exemple :
//! ```
//! use vitte_unicode::{is_nfc, normalize_nfkc};
//! assert!(is_nfc("é"));
//! assert_eq!(normalize_nfkc("Å"), "Å");
//! ```

use thiserror::Error;

#[cfg(feature = "serde")]
use serde::{Serialize, Deserialize};

/// Erreurs Unicode
#[derive(Debug, Error)]
pub enum UnicodeError {
    /// Entrée invalide (données non conformes, séquence UTF-8 incomplète, etc.).
    #[error("invalid input: {0}")]
    Invalid(String),
    /// Erreur générique non classée.
    #[error("other: {0}")]
    Other(String),
}

/// Résultat spécialisé
pub type Result<T> = std::result::Result<T, UnicodeError>;

/// Normalise en NFC
#[cfg(feature = "normalization")]
pub fn normalize_nfc(s: &str) -> String {
    use unicode_normalization::UnicodeNormalization;
    s.nfc().collect()
}

/// Normalise en NFD
#[cfg(feature = "normalization")]
pub fn normalize_nfd(s: &str) -> String {
    use unicode_normalization::UnicodeNormalization;
    s.nfd().collect()
}

/// Normalise en NFKC
#[cfg(feature = "normalization")]
pub fn normalize_nfkc(s: &str) -> String {
    use unicode_normalization::UnicodeNormalization;
    s.nfkc().collect()
}

/// Normalise en NFKD
#[cfg(feature = "normalization")]
pub fn normalize_nfkd(s: &str) -> String {
    use unicode_normalization::UnicodeNormalization;
    s.nfkd().collect()
}

/// Casefold simple
#[cfg(feature = "normalization")]
pub fn casefold(s: &str) -> String {
    s.chars().flat_map(|c| c.to_lowercase()).collect()
}

/// Vérifie NFC
#[cfg(feature = "normalization")]
pub fn is_nfc(s: &str) -> bool {
    use unicode_normalization::is_nfc;
    is_nfc(s)
}

/// Segmente en graphemes
#[cfg(feature = "segmentation")]
pub fn graphemes(s: &str) -> Vec<&str> {
    use unicode_segmentation::UnicodeSegmentation;
    s.graphemes(true).collect()
}

/// Segmente en mots
#[cfg(feature = "segmentation")]
pub fn words(s: &str) -> Vec<&str> {
    use unicode_segmentation::UnicodeSegmentation;
    s.unicode_words().collect()
}

/// Applique l’algo bidirectionnel et retourne les niveaux
#[cfg(feature = "bidi")]
pub fn bidi_levels(s: &str) -> Vec<usize> {
    use unicode_bidi::{BidiInfo, Level};
    let info = BidiInfo::new(s, None);
    info.paragraphs.iter().flat_map(|p| {
        info.levels[p.range.clone()].iter().map(|l| l.number() as usize).collect::<Vec<_>>()
    }).collect()
}

/// Largeur d’affichage (East Asian Width)
#[cfg(feature = "width")]
pub fn display_width(s: &str) -> usize {
    use unicode_width::UnicodeWidthStr;
    s.width()
}

/// Retourne les catégories Unicode simples d'un caractère
pub fn char_categories(c: char) -> Vec<&'static str> {
    let mut cats = Vec::new();
    if c.is_alphabetic() { cats.push("Letter"); }
    if c.is_numeric() { cats.push("Number"); }
    if c.is_whitespace() { cats.push("Whitespace"); }
    if c.is_control() { cats.push("Control"); }
    if c.is_ascii_punctuation() { cats.push("Punctuation"); }
    if c.is_ascii_graphic() && !c.is_alphanumeric() { cats.push("Symbol"); }
    cats
}

#[cfg(feature="normalization")]
/// Supprime les diacritiques (accents)
pub fn strip_marks(s: &str) -> String {
    use unicode_normalization::UnicodeNormalization;
    use unicode_normalization::char::is_combining_mark;
    s.nfd().filter(|c| !is_combining_mark(*c)).collect()
}

#[cfg(feature="bidi")]
/// Applique bidi et retourne texte réordonné
pub fn reverse_bidi(s: &str) -> String {
    use unicode_bidi::BidiInfo;
    let info = BidiInfo::new(s, None);
    info.reorder_line(0, s)
}

#[cfg(feature="segmentation")]
/// Compte graphemes
pub fn grapheme_count(s: &str) -> usize {
    use unicode_segmentation::UnicodeSegmentation;
    s.graphemes(true).count()
}

#[cfg(feature="segmentation")]
/// Compte mots
pub fn word_count(s: &str) -> usize {
    use unicode_segmentation::UnicodeSegmentation;
    s.unicode_words().count()
}

#[cfg(feature="width")]
/// True si tous les caractères sont plein-chasse
pub fn is_fullwidth(s: &str) -> bool {
    use unicode_width::UnicodeWidthChar;
    s.chars().all(|c| c.width().unwrap_or(1) > 1)
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn test_nfkc() {
        #[cfg(feature="normalization")]
        assert_eq!(normalize_nfkc("Å"), "Å");
    }

    #[test]
    fn test_graphemes() {
        #[cfg(feature="segmentation")]
        {
            let g = graphemes("a🇫🇷b");
            assert!(g.contains(&"🇫🇷"));
        }
    }

    #[test]
    fn test_width() {
        #[cfg(feature="width")]
        assert_eq!(display_width("コン"), 4);
    }

    #[test]
    fn categories_and_counts() {
        let cats = char_categories('é');
        assert!(cats.contains(&"Letter"));
        #[cfg(feature="segmentation")]
        {
            assert_eq!(grapheme_count("a🇫🇷b"), 3);
            assert_eq!(word_count("salut le monde"), 3);
        }
    }

    #[test]
    fn strip_and_bidi() {
        #[cfg(feature="normalization")]
        assert_eq!(strip_marks("é"), "e");
        #[cfg(feature="bidi")]
        {
            let txt = "abc";
            let rev = reverse_bidi(txt);
            assert!(rev.len() >= 3);
        }
    }

    #[test]
    fn fullwidth() {
        #[cfg(feature="width")]
        assert!(is_fullwidth("コン"));
    }
}