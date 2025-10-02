#![deny(missing_docs)]
//! vitte-semantic — primitives NLP sémantiques légères pour Vitte
//!
//! Fournit :
//! - Tokenisation simple ou via HuggingFace `tokenizers` (feature `hf-tokenizers`).
//! - Détection de langue (feature `lang`).
//! - Embeddings légers par hashing trick (TF) et similarité cosinus.
//! - Sérialisation optionnelle des embeddings (feature `serde`).
//!
//! Exemple rapide :
//! ```
//! use vitte_semantic as vs;
//! let e1 = vs::embed("hello world", 256);
//! let e2 = vs::embed("hello there", 256);
//! let sim = vs::cosine(&e1.vector, &e2.vector);
//! assert!(sim >= 0.0 && sim <= 1.0);
//! ```

use std::collections::hash_map::DefaultHasher;
use std::hash::{Hash, Hasher};

use thiserror::Error;

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

#[cfg(feature = "whatlang")]
use whatlang::{detect, Lang};

/// Erreurs NLP.
#[derive(Debug, Error)]
pub enum NlpError {
    #[error("tokenization error: {0}")]
    Tok(String),
    #[error("other: {0}")]
    Other(String),
}

/// Résultat spécialisé.
pub type Result<T> = std::result::Result<T, NlpError>;

/// Tokenise une chaîne en jetons.
///
/// - Avec `hf-tokenizers`, s'appuie sur `tokenizers::tokenizer::Tokenizer` BPE basique (WordPiece-like) si fourni.
/// - Sinon, fallback par découpe unicode rudimentaire (espaces + ponctuation basique).
pub fn tokenize(text: &str) -> Vec<String> {
    #[cfg(feature = "hf-tokenizers")]
    {
        use tokenizers::{models::wordpiece::WordPiece, normalizers::bert::BertNormalizer, pre_tokenizers::whitespace::Whitespace, Tokenizer};
        // Construire un tokenizer minimal en RAM : WordPiece vide + fallback "[UNK]".
        // Ce n'est pas un vrai vocabulaire entraîné, mais permet une découpe stable pour tests.
        let wp = WordPiece::new(Default::default())
            .unk_token("[UNK]".into())
            .build()
            .unwrap();
        let mut tok = Tokenizer::new(wp);
        tok.with_normalizer(BertNormalizer::new(false, true, true, false));
        tok.with_pre_tokenizer(Whitespace);
        let out = tok.encode(text, false).unwrap();
        return out.get_tokens().iter().map(|s| s.to_string()).collect();
    }
    #[cfg(not(feature = "hf-tokenizers"))]
    {
        simple_tokens(text)
    }
}

/// Tokenisation très simple, indépendante des features.
fn simple_tokens(text: &str) -> Vec<String> {
    text.split(|c: char| c.is_whitespace() || is_punct(c))
        .filter(|s| !s.is_empty())
        .map(|s| s.to_lowercase())
        .collect()
}

fn is_punct(c: char) -> bool {
    matches!(c,
        '\u{0021}'..='\u{002F}'
        | '\u{003A}'..='\u{0040}'
        | '\u{005B}'..='\u{0060}'
        | '\u{007B}'..='\u{007E}')
}

/// Détection de langue. Retourne un code ISO si `lang` actif, sinon `None`.
#[cfg(feature = "lang")]
pub fn detect_lang(text: &str) -> Option<&'static str> {
    detect(text).map(|info| info.lang().code())
}

#[cfg(not(feature = "lang"))]
/// Détection de langue indisponible sans la feature `lang`.
pub fn detect_lang(_text: &str) -> Option<&'static str> { None }

/// Représentation d'un embedding léger.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Debug, Clone, PartialEq)]
pub struct Embedding {
    /// Vecteur dense de taille fixe.
    pub vector: Vec<f32>,
}

impl Embedding {
    /// Norme L2.
    pub fn norm2(&self) -> f32 { l2(&self.vector) }
}

/// Calcule un embedding par hashing trick TF.
///
/// - `dim` = taille du vecteur (ex. 256, 512, 1024).
/// - Hash de chaque token sur [0,dim[ et incrément TF.
/// - Normalisation L2 à la fin pour stabilité de la similarité.
pub fn embed(text: &str, dim: usize) -> Embedding {
    let toks = tokenize(text);
    let mut v = vec![0f32; dim.max(1)];
    for t in toks {
        let idx = (hash_u64(&t) as usize) % v.len();
        v[idx] += 1.0;
    }
    let n = l2(&v);
    if n > 0.0 { for x in &mut v { *x /= n; } }
    Embedding { vector: v }
}

/// Similarité cosinus entre deux vecteurs.
pub fn cosine(a: &[f32], b: &[f32]) -> f32 {
    if a.is_empty() || b.is_empty() || a.len()!=b.len() { return 0.0; }
    let mut dot = 0.0f32; let mut na = 0.0f32; let mut nb = 0.0f32;
    for i in 0..a.len() {
        dot += a[i]*b[i];
        na += a[i]*a[i];
        nb += b[i]*b[i];
    }
    if na==0.0 || nb==0.0 { 0.0 } else { dot / (na.sqrt()*nb.sqrt()) }
}

fn l2(v: &[f32]) -> f32 { v.iter().map(|x| x*x).sum::<f32>().sqrt() }

fn hash_u64<T: Hash>(x: &T) -> u64 {
    let mut h = DefaultHasher::new();
    x.hash(&mut h);
    h.finish()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn simple_tokenization() {
        let t = tokenize("Hello, world! This is Vitte.");
        assert!(!t.is_empty());
    }

    #[test]
    fn embedding_and_cosine() {
        let e1 = embed("hello world", 128);
        let e2 = embed("hello there", 128);
        let sim = cosine(&e1.vector, &e2.vector);
        assert!(sim >= 0.0 && sim <= 1.0);
    }

    #[cfg(feature = "lang")]
    #[test]
    fn language_detection() {
        let l = detect_lang("Bonjour, ceci est un test.");
        assert!(l.is_some());
    }
}
