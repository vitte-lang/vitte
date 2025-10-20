#![deny(missing_docs)]
//! vitte-autocompletion — moteur d'autocomplétion portable pour CLI/REPL/éditeurs.
//!
//! Objectifs: latence faible, aucune allocation superflue, API stable.
//! Fournit:
//! - `Engine` pour agréger des `Source` et classer des candidats.
//! - `MemorySource` basée sur un trie immuable compact.
//! - `MatchAlgo` avec `Prefix`, `Substring`, et `Fuzzy` (feature `fuzzy`).
//! - Types `Completion`, `Context`, `CompletionList`.
//! - Aides: déduplication, coalescence de métadonnées, indices de surlignage.
//!
//! Exemple:
//! ```rust
//! use vitte_autocompletion as ac;
//! let mut eng = ac::Engine::builder().algo(ac::MatchAlgo::Prefix).build();
//! eng.add_source(ac::MemorySource::from_pairs([
//!     ("help", ac::CompletionKind::Command),
//!     ("hello", ac::CompletionKind::Keyword),
//!     ("halt", ac::CompletionKind::Command),
//! ]));
//! let out = eng.complete("he", &ac::Context::default(), 8).unwrap();
//! assert!(out.items.iter().any(|c| c.label == "hello"));
//! ```

/// Réexporte l'algorithme et les utilitaires de classement/mise en évidence.
pub use matcher::{
    dedup_in_place, highlight_indices, match_query, rank_in_place, FuzzyConfig, MatchAlgo,
};
/// Réexporte les sources de complétions (in-mémoire) et le trait `Source`.
pub use source::{MemorySource, Source};
/// Réexporte le trie immuable compact et son builder.
pub use trie::{Trie, TrieBuilder};
/// Réexporte les types de données publics de l'API d'autocomplétion.
pub use types::{Completion, CompletionKind, CompletionList, CompletionMeta, Context};

/// Erreurs de l'engine.
#[derive(Debug)]
pub enum AutoError {
    /// Aucune source enregistrée.
    NoSource,
}

impl core::fmt::Display for AutoError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            AutoError::NoSource => write!(f, "no source"),
        }
    }
}

impl std::error::Error for AutoError {}

/// Builder d'`Engine`.
pub struct EngineBuilder {
    algo: MatchAlgo,
    namespace_filtering: bool,
}

impl Default for EngineBuilder {
    fn default() -> Self {
        Self { algo: MatchAlgo::Prefix, namespace_filtering: true }
    }
}

impl EngineBuilder {
    /// Définit l'algorithme.
    pub fn algo(mut self, algo: MatchAlgo) -> Self {
        self.algo = algo;
        self
    }
    /// Active/désactive la filtration légère par namespace depuis `Context`.
    pub fn namespace_filtering(mut self, on: bool) -> Self {
        self.namespace_filtering = on;
        self
    }
    /// Construit l'`Engine`.
    pub fn build(self) -> Engine {
        Engine {
            sources: Vec::new(),
            algo: self.algo,
            namespace_filtering: self.namespace_filtering,
        }
    }
}

/// Engine d'autocomplétion. Agrège des `Source` et applique un algo de matching.
pub struct Engine {
    sources: Vec<Box<dyn Source + Send + Sync>>,
    algo: MatchAlgo,
    namespace_filtering: bool,
}

impl Default for Engine {
    fn default() -> Self {
        Engine { sources: Vec::new(), algo: MatchAlgo::Prefix, namespace_filtering: true }
    }
}

impl Engine {
    /// Nouveau builder.
    pub fn builder() -> EngineBuilder {
        EngineBuilder::default()
    }

    /// Crée un engine avec `algo`.
    pub fn new(algo: MatchAlgo) -> Self {
        EngineBuilder::default().algo(algo).build()
    }

    /// Change l'algo à la volée.
    pub fn set_algo(&mut self, algo: MatchAlgo) {
        self.algo = algo;
    }

    /// Ajoute une source thread-safe.
    pub fn add_source<S: Source + Send + Sync + 'static>(&mut self, src: S) {
        self.sources.push(Box::new(src));
    }

    /// Nombre de sources.
    pub fn sources_len(&self) -> usize {
        self.sources.len()
    }

    /// Propose des complétions pour `query` dans `context`. Tronque à `limit`.
    /// Déduplique et trie de façon déterministe.
    pub fn complete(
        &self,
        query: &str,
        context: &Context,
        limit: usize,
    ) -> Result<CompletionList, AutoError> {
        if self.sources.is_empty() {
            return Err(AutoError::NoSource);
        }
        let mut buf: Vec<Completion> = Vec::with_capacity(128);
        for s in &self.sources {
            s.collect(query, context, &mut buf);
        }
        if self.namespace_filtering {
            util::light_namespace_filter(context, &mut buf);
        }
        matcher::rank_in_place(&mut buf, query, self.algo);
        matcher::dedup_in_place(&mut buf);
        if buf.len() > limit {
            buf.truncate(limit);
        }
        Ok(CompletionList { items: buf })
    }
}

// ============================================================================
// util.rs
// ============================================================================
mod util {
    use super::types::{Completion, CompletionKind, Context};

    /// Retourne vrai si `c` débute un mot après `prev`.
    #[inline]
    pub fn is_word_boundary(prev: Option<char>, c: char) -> bool {
        match prev {
            None => true,
            Some(p) => !p.is_alphanumeric() && c.is_alphanumeric(),
        }
    }

    /// Retourne vrai s'il y a une frontière camelCase entre `prev` et `c`.
    #[inline]
    pub fn is_camel_boundary(prev: Option<char>, c: char) -> bool {
        match prev {
            Some(p) if p.is_ascii_lowercase() && c.is_ascii_uppercase() => true,
            _ => false,
        }
    }

    /// Filtrage léger par namespace côté client.
    /// - "path" garde Path, "command" garde Command, "keyword" garde Keyword, sinon aucun filtrage.
    pub fn light_namespace_filter(ctx: &Context, items: &mut Vec<Completion>) {
        let Some(ns) = ctx.namespace.as_deref() else {
            return;
        };
        let keep = |k: CompletionKind| -> bool {
            match ns {
                "path" => matches!(k, CompletionKind::Path),
                "command" => matches!(k, CompletionKind::Command),
                "keyword" => matches!(k, CompletionKind::Keyword),
                "symbol" => matches!(k, CompletionKind::Symbol),
                "option" => matches!(k, CompletionKind::Option),
                _ => true,
            }
        };
        items.retain(|c| keep(c.kind));
    }
}

// ============================================================================
// types.rs
// ============================================================================
mod types {

    /// Type de complétion.
    #[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
    #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
    pub enum CompletionKind {
        /// Symbole (ident, fonction…)
        Symbol,
        /// Mot-clé.
        Keyword,
        /// Chemin de fichier.
        Path,
        /// Commande.
        Command,
        /// Snippet.
        Snippet,
        /// Option (ex: `--help`).
        Option,
        /// Valeur (ex: `true`, `42`).
        Value,
        /// Autre.
        Other,
    }

    /// Métadonnées facultatives.
    #[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
    #[derive(Debug, Clone, Default)]
    pub struct CompletionMeta {
        /// Détail court.
        pub detail: Option<String>,
        /// Documentation courte.
        pub doc: Option<String>,
    }

    /// Entrée de complétion.
    #[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
    #[derive(Debug, Clone)]
    pub struct Completion {
        /// Texte à insérer.
        pub insert: String,
        /// Texte affiché.
        pub label: String,
        /// Score calculé. Plus élevé = mieux.
        pub score: i32,
        /// Catégorie.
        pub kind: CompletionKind,
        /// Métadonnées.
        pub meta: CompletionMeta,
    }

    impl Completion {
        /// Crée une complétion basique avec `label` = `insert`.
        pub fn new<S: Into<String>>(s: S, kind: CompletionKind) -> Self {
            let x = s.into();
            Self { insert: x.clone(), label: x, score: 0, kind, meta: CompletionMeta::default() }
        }

        /// Ajoute un détail.
        pub fn with_detail(mut self, detail: impl Into<String>) -> Self {
            self.meta.detail = Some(detail.into());
            self
        }

        /// Ajoute une doc.
        pub fn with_doc(mut self, doc: impl Into<String>) -> Self {
            self.meta.doc = Some(doc.into());
            self
        }
    }

    /// Contexte d'édition.
    #[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
    #[derive(Debug, Clone, Default)]
    pub struct Context {
        /// Index du curseur.
        pub cursor: usize,
        /// Contenu précédant le curseur (ligne ou buffer partiel).
        pub preceding: String,
        /// Namespace logique (ex: "path", "keyword", "symbol").
        pub namespace: Option<String>,
    }

    /// Liste de complétions.
    #[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
    #[derive(Debug, Clone, Default)]
    pub struct CompletionList {
        /// Éléments.
        pub items: Vec<Completion>,
    }
}

// ============================================================================
// trie.rs
// ============================================================================
mod trie {
    use std::collections::BTreeMap;

    /// Trie immuable compact pour recherche par préfixe.
    #[derive(Clone)]
    pub struct Trie<T: Copy> {
        root: Node<T>,
    }

    #[derive(Clone)]
    struct Node<T: Copy> {
        term: Option<T>,
        edges: BTreeMap<char, Node<T>>,
    }

    impl<T: Copy> Default for Node<T> {
        fn default() -> Self {
            Self { term: None, edges: BTreeMap::new() }
        }
    }

    /// Builder de `Trie`.
    pub struct TrieBuilder<T: Copy> {
        root: Node<T>,
    }

    impl<T: Copy> Default for Trie<T> {
        fn default() -> Self {
            Self { root: Node::default() }
        }
    }

    impl<T: Copy> Default for TrieBuilder<T> {
        fn default() -> Self {
            Self { root: Node::default() }
        }
    }

    impl<T: Copy> TrieBuilder<T> {
        /// Insère `s` avec la valeur `val`.
        pub fn insert(&mut self, s: &str, val: T) {
            let mut n = &mut self.root;
            for ch in s.chars() {
                n = n.edges.entry(ch).or_default();
            }
            n.term = Some(val);
        }

        /// Construit un `Trie` immuable.
        pub fn build(self) -> Trie<T> {
            Trie { root: self.root }
        }
    }

    impl<T: Copy> Trie<T> {
        /// Itère sur les labels ayant le préfixe `p`.
        pub fn iter_prefix<'a>(&'a self, p: &str) -> impl Iterator<Item = (String, &'a T)> + 'a {
            let mut node = &self.root;
            for ch in p.chars() {
                if let Some(n) = node.edges.get(&ch) {
                    node = n;
                } else {
                    return Box::new(std::iter::empty()) as Box<dyn Iterator<Item = (String, &T)>>;
                }
            }
            let mut acc: Vec<(String, &T)> = Vec::new();
            fn dfs<'a, T: Copy>(pref: &mut String, n: &'a Node<T>, out: &mut Vec<(String, &'a T)>) {
                if let Some(ref v) = n.term {
                    out.push((pref.clone(), v));
                }
                for (ch, child) in n.edges.iter() {
                    pref.push(*ch);
                    dfs(pref, child, out);
                    pref.pop();
                }
            }
            let mut pref = String::from(p);
            dfs(&mut pref, node, &mut acc);
            Box::new(acc.into_iter())
        }
    }
}

// ============================================================================
// matcher.rs
// ============================================================================
mod matcher {
    use crate::types::{Completion, CompletionKind};
    use crate::util::{is_camel_boundary, is_word_boundary};
    use smallvec::SmallVec;
    use std::collections::HashSet;

    /// Algorithme de matching à utiliser.
    #[derive(Debug, Clone, Copy)]
    pub enum MatchAlgo {
        /// Préfixe exact.
        Prefix,
        /// Sous-chaîne.
        Substring,
        /// Fuzzy (activable via feature).
        #[cfg(feature = "fuzzy")]
        Fuzzy(FuzzyConfig),
    }

    /// Config fuzzy.
    #[cfg(feature = "fuzzy")]
    #[derive(Debug, Clone, Copy)]
    pub struct FuzzyConfig {
        /// Pénalité d'écart.
        pub gap_penalty: i32,
        /// Bonus limite de mot.
        pub bonus_boundary: i32,
        /// Bonus séquences consécutives.
        pub bonus_consecutive: i32,
    }

    /// Tri en place selon le score décroissant, puis tiebreak: longueur croissante, ordre lexical.
    pub fn rank_in_place(items: &mut Vec<Completion>, query: &str, algo: MatchAlgo) {
        for it in items.iter_mut() {
            it.score = score(&it.label, query, algo);
        }
        items.sort_by(|a, b| {
            b.score
                .cmp(&a.score)
                .then_with(|| a.label.len().cmp(&b.label.len()))
                .then_with(|| a.label.cmp(&b.label))
        });
    }

    /// Déduplique en place par (label, kind), garde la première occurrence (score max après tri).
    pub fn dedup_in_place(items: &mut Vec<Completion>) {
        let mut seen: HashSet<(String, CompletionKind)> = HashSet::with_capacity(items.len());
        items.retain(|c| seen.insert((c.label.clone(), c.kind)));
    }

    /// Score brut avec bonus de frontières de mot et camelCase.
    pub fn score(label: &str, query: &str, algo: MatchAlgo) -> i32 {
        if query.is_empty() {
            return 0;
        }
        match algo {
            MatchAlgo::Prefix => {
                if label.starts_with(query) {
                    base_prefix_score(label, query)
                } else {
                    -1
                }
            }
            MatchAlgo::Substring => {
                if let Some(pos) = label.find(query) {
                    base_substring_score(label, pos, query.len())
                } else {
                    -1
                }
            }
            #[cfg(feature = "fuzzy")]
            MatchAlgo::Fuzzy(cfg) => fuzzy_score(label, query, cfg),
        }
    }

    #[inline]
    fn base_prefix_score(label: &str, query: &str) -> i32 {
        let mut sc = 1000i32.saturating_sub((label.len() - query.len()) as i32);
        // Bonus frontière/camel
        let mut prev = None;
        for (i, ch) in label.chars().enumerate().take(query.chars().count()) {
            if is_word_boundary(prev, ch) || is_camel_boundary(prev, ch) {
                sc += 2;
            }
            prev = Some(ch);
            if i > 16 {
                break;
            }
        }
        sc
    }

    #[inline]
    fn base_substring_score(label: &str, start: usize, qlen: usize) -> i32 {
        let mut sc = 800 - start as i32;
        let prev = label[..start].chars().rev().next();
        let first = label[start..].chars().next().unwrap_or_default();
        if is_word_boundary(prev, first) || is_camel_boundary(prev, first) {
            sc += 10;
        }
        // bonus compacité
        sc -= (label.len() - qlen) as i32 / 16;
        sc
    }

    /// Indices à surligner pour l'affichage. Pour `Prefix` et `Substring` c'est exact.
    /// Pour `Fuzzy`, renvoie une approximation monotone.
    pub fn highlight_indices(label: &str, query: &str, algo: MatchAlgo) -> SmallVec<[usize; 32]> {
        let mut out: SmallVec<[usize; 32]> = SmallVec::new();
        if query.is_empty() {
            return out;
        }
        match algo {
            MatchAlgo::Prefix => {
                for i in 0..query.len().min(label.len()) {
                    out.push(i);
                }
            }
            MatchAlgo::Substring => {
                if let Some(pos) = label.find(query) {
                    for i in pos..pos + query.len() {
                        out.push(i);
                    }
                }
            }
            #[cfg(feature = "fuzzy")]
            MatchAlgo::Fuzzy(_cfg) => {
                // Greedy: avance dans label et marque chaque match case-insensitive.
                let mut qi = 0usize;
                for (i, lc) in label.chars().enumerate() {
                    if qi < query.len() {
                        let qc = query.chars().nth(qi).unwrap();
                        if lc.eq_ignore_ascii_case(&qc) {
                            out.push(i);
                            qi += 1;
                        }
                    }
                }
            }
        }
        out
    }

    /// Appariement simple pour observation externe.
    pub fn match_query(
        candidates: &[String],
        query: &str,
        algo: MatchAlgo,
        limit: usize,
    ) -> SmallVec<[Completion; 32]> {
        let mut out: SmallVec<[Completion; 32]> = SmallVec::new();
        for s in candidates {
            let mut c =
                crate::types::Completion::new(s.clone(), crate::types::CompletionKind::Other);
            c.score = score(&c.label, query, algo);
            if c.score >= 0 {
                out.push(c);
            }
        }
        out.sort_by(|a, b| {
            b.score
                .cmp(&a.score)
                .then_with(|| a.label.len().cmp(&b.label.len()))
                .then_with(|| a.label.cmp(&b.label))
        });
        if out.len() > limit {
            out.truncate(limit);
        }
        out
    }

    #[cfg(feature = "fuzzy")]
    fn fuzzy_score(label: &str, query: &str, cfg: FuzzyConfig) -> i32 {
        // Variante compacte Smith-Waterman adaptée type fzy. O(n*m) borné.
        let nl = label.len();
        let nq = query.len();
        if nq == 0 || nl == 0 || nq > 128 || nl > 4096 {
            return -1;
        }

        let lchars: Vec<char> = label.chars().collect();
        let qchars: Vec<char> = query.chars().collect();

        let mut prev = vec![0i32; nq + 1];
        let mut best = -1;

        for (i, &lc) in lchars.iter().enumerate() {
            let mut cur = vec![0i32; nq + 1];
            for (j, &qc) in qchars.iter().enumerate() {
                let mut s = if lc.eq_ignore_ascii_case(&qc) { 10 } else { -5 };
                // Bonus limites de mots/camel
                let prevc = if i == 0 { None } else { Some(lchars[i - 1]) };
                if super::util::is_word_boundary(prevc, lc)
                    || super::util::is_camel_boundary(prevc, lc)
                {
                    s += cfg.bonus_boundary;
                }
                if j > 0 && i > 0 {
                    s += cfg.bonus_consecutive;
                }
                let diag = prev[j] + s;
                let up = prev[j + 1] + cfg.gap_penalty;
                let left = cur[j] + cfg.gap_penalty;
                let v = 0.max(diag).max(up).max(left);
                cur[j + 1] = v;
                if v > best {
                    best = v;
                }
            }
            prev = cur;
        }
        best
    }
}

// ============================================================================
// source.rs
// ============================================================================
mod source {
    use crate::{
        trie::Trie,
        types::{Completion, CompletionKind, Context},
    };

    /// Source de données de complétion.
    pub trait Source {
        /// Collecte des candidats pour `query` dans `ctx` et les pousse dans `out`.
        fn collect(&self, query: &str, ctx: &Context, out: &mut Vec<Completion>);
    }

    /// Source mémoire simple basée sur un Trie.
    pub struct MemorySource {
        trie: Trie<CompletionKind>,
    }

    impl MemorySource {
        /// Construit depuis des paires (label, kind).
        pub fn from_pairs<I, S>(pairs: I) -> Self
        where
            I: IntoIterator<Item = (S, CompletionKind)>,
            S: AsRef<str>,
        {
            let mut b = crate::trie::TrieBuilder::default();
            for (s, k) in pairs {
                b.insert(s.as_ref(), k);
            }
            Self { trie: b.build() }
        }
    }

    impl Source for MemorySource {
        fn collect(&self, query: &str, _ctx: &Context, out: &mut Vec<Completion>) {
            for (label, &kind) in self.trie.iter_prefix(query) {
                out.push(Completion::new(label, kind));
            }
        }
    }
}

// ============================================================================
// Tests
// ============================================================================
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn prefix_basic() {
        let mut eng = Engine::builder().algo(MatchAlgo::Prefix).build();
        eng.add_source(MemorySource::from_pairs([
            ("help", CompletionKind::Command),
            ("hello", CompletionKind::Keyword),
            ("halt", CompletionKind::Command),
            ("heap", CompletionKind::Keyword),
        ]));
        let out = eng.complete("he", &Context::default(), 16).unwrap();
        assert!(out.items.iter().any(|c| c.label == "help"));
        assert!(out.items.iter().any(|c| c.label == "hello"));
    }

    #[test]
    fn substring_score() {
        use matcher::score;
        assert!(score("helloWorld", "World", MatchAlgo::Substring) > 0);
        assert!(score("hello", "xyz", MatchAlgo::Substring) < 0);
    }

    #[test]
    fn dedup_keeps_best() {
        let mut v = vec![
            Completion::new("help", CompletionKind::Command),
            Completion::new("help", CompletionKind::Command),
            Completion::new("help", CompletionKind::Keyword),
        ];
        v[0].score = 10;
        v[1].score = 20;
        v[2].score = 5;
        rank_in_place(&mut v, "he", MatchAlgo::Prefix);
        dedup_in_place(&mut v);
        // On garde 2 éléments: (help, Command) et (help, Keyword)
        assert_eq!(v.iter().filter(|c| c.label == "help").count(), 2);
    }

    #[test]
    fn highlight_prefix_and_substring() {
        let idx = highlight_indices("hello", "he", MatchAlgo::Prefix);
        assert_eq!(idx.as_slice(), &[0, 1]);
        let idx2 = highlight_indices("path/to/file", "to", MatchAlgo::Substring);
        assert_eq!(idx2.as_slice(), &[5, 6]);
    }

    #[cfg(feature = "fuzzy")]
    #[test]
    fn fuzzy_ok() {
        let cfg = FuzzyConfig { gap_penalty: -3, bonus_boundary: 5, bonus_consecutive: 2 };
        use matcher::score;
        assert!(score("concatenate", "cct", MatchAlgo::Fuzzy(cfg)) >= 0);
    }
}
