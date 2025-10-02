//! vitte-i18n — Internationalisation pour Vitte
//!
//! - Catalogues par locale (`fr`, `en-GB`, …) via [fluent](https://projectfluent.org).
//! - Chargement FTL (Fluent) et JSON (facultatif, feature `json`).
//! - Fallbacks hiérarchiques (ex: `fr-FR` → `fr` → `en`).
//! - API thread-safe, cache de bundles, formatage avec variables/pluriels.
//!
//! ## Exemple rapide
//! ```no_run
//! use vitte_i18n::{I18n, Lang, args};
//! let mut i18n = I18n::new(Lang::parse("fr").unwrap()).with_fallbacks(&[Lang::parse("en").unwrap()]);
//! i18n.add_ftl("fr", r#"
//! hello-user = Bonjour { $name }!
//! items = { $n ->
//!     [one] 1 élément
//!    *[other] { $n } éléments
//! }"#).unwrap();
//! assert_eq!(i18n.t("hello-user", args!(name => "Vincent")), "Bonjour Vincent!");
//! assert_eq!(i18n.t("items", args!(n => 1)), "1 élément");
//! assert_eq!(i18n.t("items", args!(n => 3)), "3 éléments");
//! ```

#![forbid(unsafe_code)]
#![deny(missing_docs)]

use std::collections::{BTreeMap, HashMap};
use std::path::Path;
use std::sync::{Arc, RwLock};

use fluent_bundle::{FluentArgs, FluentBundle, FluentError, FluentResource, FluentValue};
use fluent_syntax::parser::errors::ParserError;
use thiserror::Error;
use unic_langid::LanguageIdentifier;

/// Résultat standard.
pub type I18nResult<T> = Result<T, I18nError>;

/// Erreurs I18n.
#[derive(Error, Debug)]
pub enum I18nError {
    /// Locale invalide.
    #[error("invalid locale: {0}")]
    InvalidLocale(String),

    /// Erreur de parsing FTL.
    #[error("ftl parse error: {0:?}")]
    Ftl(Vec<ParserError>),

    /// Erreur de formatage Fluent.
    #[error("fluent format error: {0:?}")]
    Fluent(Vec<FluentError>),

    /// I/O local.
    #[error("io: {0}")]
    Io(#[from] std::io::Error),

    /// JSON indisponible car feature désactivée.
    #[error("json feature not enabled")]
    JsonFeatureDisabled,

    /// JSON (quand feature active).
    #[cfg(feature = "json")]
    #[error("json: {0}")]
    Json(#[from] serde_json::Error),

    /// Clé introuvable.
    #[error("missing message: {0}")]
    MissingKey(String),
}

/// Type alias pour les locales.
pub type Lang = LanguageIdentifier;

/// Construit un `Lang` depuis &str ou retourne une erreur explicite.
fn lang<S: AsRef<str>>(s: S) -> I18nResult<Lang> {
    s.as_ref()
        .parse::<LanguageIdentifier>()
        .map_err(|_| I18nError::InvalidLocale(s.as_ref().to_string()))
}

/// Valeurs d’arguments pour formatage.
#[derive(Debug, Clone)]
pub enum ArgValue<'a> {
    /// Texte.
    Str(&'a str),
    /// Entier.
    Int(i64),
    /// Nombre flottant.
    Num(f64),
    /// Booléen.
    Bool(bool),
}

impl<'a> From<ArgValue<'a>> for FluentValue<'a> {
    fn from(v: ArgValue<'a>) -> Self {
        match v {
            ArgValue::Str(s) => FluentValue::String(s.into()),
            ArgValue::Int(i) => FluentValue::from(i),
            ArgValue::Num(n) => FluentValue::from(n),
            ArgValue::Bool(b) => FluentValue::from(b),
        }
    }
}

/// Macro pratique pour construire des `FluentArgs`.
/// ```
/// use vitte_i18n::args;
/// let a = args!(name => "Alice", n => 3);
/// ```
#[macro_export]
macro_rules! args {
    ($($k:ident => $v:expr),* $(,)?) => {{
        let mut a = ::fluent_bundle::FluentArgs::new();
        $( a.set(stringify!($k), ($crate::ArgValue::from($v))); )*
        a
    }};
}

impl<'a> From<&'a str> for ArgValue<'a> {
    fn from(s: &'a str) -> Self { ArgValue::Str(s) }
}
impl<'a> From<i64> for ArgValue<'a> {
    fn from(i: i64) -> Self { ArgValue::Int(i) }
}
impl<'a> From<i32> for ArgValue<'a> {
    fn from(i: i32) -> Self { ArgValue::Int(i as i64) }
}
impl<'a> From<usize> for ArgValue<'a> {
    fn from(i: usize) -> Self { ArgValue::Int(i as i64) }
}
impl<'a> From<f64> for ArgValue<'a> {
    fn from(n: f64) -> Self { ArgValue::Num(n) }
}
impl<'a> From<bool> for ArgValue<'a> {
    fn from(b: bool) -> Self { ArgValue::Bool(b) }
}

/// Enregistrement immuable des ressources FTL par langue.
#[derive(Default, Clone)]
struct Catalog {
    /// Map langue → fragments FTL.
    ftl_sources: HashMap<Lang, Vec<String>>,
}

impl Catalog {
    fn add_ftl(&mut self, lang: &Lang, src: &str) {
        self.ftl_sources.entry(lang.clone()).or_default().push(src.to_string());
    }

    fn langs(&self) -> impl Iterator<Item = &Lang> {
        self.ftl_sources.keys()
    }
}

/// Cache de bundles compilés pour accélérer le formatage.
#[derive(Default)]
struct BundleCache {
    /// Map (lang) → FluentBundle compilé.
    bundles: HashMap<Lang, FluentBundle<FluentResource>>,
}

impl BundleCache {
    fn clear(&mut self) {
        self.bundles.clear();
    }
}

/// Moteur I18n thread-safe.
#[derive(Clone)]
pub struct I18n {
    inner: Arc<RwLock<Inner>>,
}

struct Inner {
    base: Lang,
    fallbacks: Vec<Lang>,
    catalog: Catalog,
    cache: BundleCache,
}

impl I18n {
    /// Crée un moteur avec la locale de base.
    pub fn new(base: Lang) -> Self {
        Self {
            inner: Arc::new(RwLock::new(Inner {
                base,
                fallbacks: Vec::new(),
                catalog: Catalog::default(),
                cache: BundleCache::default(),
            })),
        }
    }

    /// Définit une chaîne de fallbacks, ordre de priorité décroissant.
    pub fn with_fallbacks(mut self, langs: &[Lang]) -> Self {
        if let Ok(mut g) = self.inner.write() {
            g.fallbacks = langs.to_vec();
            g.cache.clear();
        }
        self
    }

    /// Locale actuelle.
    pub fn current(&self) -> Lang {
        self.inner.read().unwrap().base.clone()
    }

    /// Change la locale active.
    pub fn set_locale<S: AsRef<str>>(&self, locale: S) -> I18nResult<()> {
        let lang = lang(locale)?;
        let mut g = self.inner.write().unwrap();
        g.base = lang;
        g.cache.clear();
        Ok(())
    }

    /// Ajoute des ressources FTL pour une langue.
    pub fn add_ftl<S: AsRef<str>>(&mut self, locale: S, ftl_source: &str) -> I18nResult<()> {
        let lg = lang(locale)?;
        let mut g = self.inner.write().unwrap();
        g.catalog.add_ftl(&lg, ftl_source);
        g.cache.clear();
        Ok(())
    }

    /// Charge un fichier FTL sur disque.
    pub fn add_ftl_file<S: AsRef<str>>(&mut self, locale: S, path: &Path) -> I18nResult<()> {
        let s = std::fs::read_to_string(path)?;
        self.add_ftl(locale, &s)
    }

    /// Charge tous les fichiers `.ftl` d'un dossier (non récursif).
    pub fn add_ftl_dir<S: AsRef<str>>(&mut self, locale: S, dir: &Path) -> I18nResult<usize> {
        let mut n = 0;
        for e in std::fs::read_dir(dir)? {
            let e = e?;
            if e.file_type()?.is_file() && e.path().extension().and_then(|x| x.to_str()) == Some("ftl") {
                self.add_ftl(&locale, &std::fs::read_to_string(e.path())?)?;
                n += 1;
            }
        }
        Ok(n)
    }

    /// Ajout depuis JSON (feature `json`): `{ "key": "Bonjour", "title = …": "…"}`
    #[cfg(feature = "json")]
    pub fn add_json<S: AsRef<str>>(&mut self, locale: S, json_str: &str) -> I18nResult<()> {
        use serde::Deserialize;
        #[derive(Deserialize)]
        struct MapWrap(#[serde(with = "serde_json::value::map::Map")] serde_json::Map<String, serde_json::Value>);
        // On accepte un objet JSON string→string. On convertit en FTL trivial.
        let map: BTreeMap<String, serde_json::Value> = serde_json::from_str(json_str)?;
        let mut ftl = String::new();
        for (k, v) in map {
            match v {
                serde_json::Value::String(s) => {
                    // échappe lines multi pour FTL basique
                    if s.contains('\n') {
                        ftl.push_str(&format!("{} =\n  {}\n", k, s.replace('\n', "\n  ")));
                    } else {
                        ftl.push_str(&format!("{} = {}\n", k, s));
                    }
                }
                _ => {
                    // sérialise JSON et place en texte
                    let s = serde_json::to_string(&v)?;
                    ftl.push_str(&format!("{} = {}\n", k, s));
                }
            }
        }
        self.add_ftl(locale, &ftl)
    }

    /// Variante sans feature json.
    #[cfg(not(feature = "json"))]
    pub fn add_json<S: AsRef<str>>(&mut self, _locale: S, _json_str: &str) -> I18nResult<()> {
        Err(I18nError::JsonFeatureDisabled)
    }

    /// Formate la clé dans la locale active avec arguments.
    pub fn t<'a>(&self, key: &str, args: FluentArgs<'a>) -> String {
        self.t_opt(key, args).unwrap_or_else(|| format!("{{{key}}}"))
    }

    /// Idem `t` mais retourne `None` si la clé n’existe pas dans la chaîne de fallback.
    pub fn t_opt<'a>(&self, key: &str, args: FluentArgs<'a>) -> Option<String> {
        let (langs, bundle_for) = {
            // petite capture pour limiter la tenue du lock
            let g = self.inner.read().unwrap();
            let mut chain = Vec::with_capacity(2 + g.fallbacks.len());
            chain.push(g.base.clone());
            chain.extend(g.base.language.clone().map(|_| lang_root(&g.base)));
            for l in &g.fallbacks {
                chain.push(l.clone());
            }
            // dé-dup
            chain.sort();
            chain.dedup();
            let cache_ptr = &g.cache as *const BundleCache as usize;
            (chain, cache_ptr)
        };

        for lg in langs {
            if let Some(v) = self.try_format_with(&lg, key, &args, bundle_for) {
                return Some(v);
            }
        }
        None
    }

    /// Renvoie une erreur si la clé est introuvable.
    pub fn t_strict<'a>(&self, key: &str, args: FluentArgs<'a>) -> I18nResult<String> {
        self.t_opt(key, args)
            .ok_or_else(|| I18nError::MissingKey(key.to_string()))
    }

    // ---------------- internals ----------------

    /// Compile ou récupère du cache un bundle pour `lang`.
    fn get_or_build_bundle(&self, lang: &Lang, cache_id: usize) -> Option<FluentBundle<FluentResource>> {
        // accède au cache via lock en écriture au besoin
        let mut need_build = false;

        {
            let g = self.inner.read().ok()?;
            if let Some(b) = g.cache.bundles.get(lang) {
                return Some(clone_bundle(b));
            }
            // pas trouvé → build si ressources présentes
            need_build = g.catalog.ftl_sources.contains_key(lang);
        }

        if !need_build {
            return None;
        }

        let built = {
            let g = self.inner.read().ok()?;
            let Some(srcs) = g.catalog.ftl_sources.get(lang) else { return None; };
            let mut bundle = FluentBundle::new_concurrent(vec![lang.clone()]);
            for s in srcs {
                let (res, errs) = parse_ftl(s);
                if !errs.is_empty() {
                    // on ignore silencieusement les entrées corrompues mais on tente le reste
                }
                if let Err(_e) = bundle.add_resource(res) {
                    // conflits de clés → continue; le dernier gagne par défaut si on empilait différemment
                }
            }
            bundle
        };

        // insère dans le cache
        if let Ok(mut g) = self.inner.write() {
            g.cache.bundles.insert(lang.clone(), clone_bundle(&built));
        }
        Some(built)
    }

    /// Essaie de formater une clé dans un bundle pour `lang`.
    fn try_format_with<'a>(
        &self,
        lang: &Lang,
        key: &str,
        args: &FluentArgs<'a>,
        cache_id: usize,
    ) -> Option<String> {
        let bundle = self.get_or_build_bundle(lang, cache_id)?;
        let msg = bundle.get_message(key)?;
        let pat = msg.value()?;
        let mut errs = Vec::new();
        let v = bundle.format_pattern(pat, Some(args), &mut errs).to_string();
        if errs.is_empty() {
            Some(v)
        } else {
            // on retourne quand même le rendu mais on pourrait logguer
            Some(v)
        }
    }
}

/// Parse un bloc FTL en `FluentResource`.
fn parse_ftl(s: &str) -> (FluentResource, Vec<ParserError>) {
    match FluentResource::try_new(s.to_string()) {
        Ok(r) => (r, vec![]),
        Err((res, errs)) => (res, errs),
    }
}

/// Clone “cheap” d’un bundle via reconstitution.
/// (FluentBundle ne dérive pas Clone; on ré-attache les mêmes locales et ressources déjà fusionnées.)
fn clone_bundle(src: &FluentBundle<FluentResource>) -> FluentBundle<FluentResource> {
    let mut out = FluentBundle::new_concurrent(src.locales().to_vec());
    for res in src.resources() {
        let _ = out.add_resource(res.clone());
    }
    out
}

/// Retourne la langue racine (`fr-FR` → `fr`) si possible, sinon identique.
fn lang_root(l: &Lang) -> Lang {
    if l.region.is_some() || l.script.is_some() {
        let mut r = l.clone();
        r.region = None;
        r.script = None;
        r
    } else {
        l.clone()
    }
}

/// Détecte une locale depuis l’environnement (LANG, LC_ALL, …). Fallback `en`.
pub fn detect_locale() -> Lang {
    let candidates = ["LC_ALL", "LC_MESSAGES", "LANG"];
    for k in candidates {
        if let Ok(v) = std::env::var(k) {
            // “fr_FR.UTF-8” → “fr-FR”
            let norm = v
                .split('.')
                .next()
                .unwrap_or("en")
                .replace('_', "-");
            if let Ok(l) = norm.parse::<LanguageIdentifier>() {
                return l;
            }
        }
    }
    "en".parse().unwrap()
}

// ===================== Tests =====================

#[cfg(test)]
mod tests {
    use super::*;
    use fluent_bundle::FluentArgs;

    #[test]
    fn ftl_basic() {
        let mut i = I18n::new(lang("fr").unwrap());
        i.add_ftl("fr", "hello = bonjour").unwrap();
        assert_eq!(i.t("hello", FluentArgs::new()), "bonjour");
        assert_eq!(i.t("missing", FluentArgs::new()), "{missing}");
    }

    #[test]
    fn args_and_plural() {
        let mut i = I18n::new(lang("fr").unwrap());
        i.add_ftl("fr", r#"
hello-user = Bonjour { $name }!
items = { $n ->
  [one] 1 élément
 *[other] { $n } éléments
}"#).unwrap();
        let mut a = FluentArgs::new();
        a.set("name", ArgValue::from("Alice"));
        assert_eq!(i.t("hello-user", a), "Bonjour Alice!");
        assert_eq!(i.t("items", args!(n => 1)), "1 élément");
        assert_eq!(i.t("items", args!(n => 3)), "3 éléments");
    }

    #[cfg(feature = "json")]
    #[test]
    fn json_loader() {
        let mut i = I18n::new(lang("en").unwrap());
        i.add_json("en", r#"{ "a": "hi", "b": "line\nbreak" }"#).unwrap();
        assert_eq!(i.t("a", FluentArgs::new()), "hi");
        assert_eq!(i.t("b", FluentArgs::new()), "line\nbreak");
    }

    #[test]
    fn fallbacks() {
        let mut i = I18n::new(lang("fr-FR").unwrap()).with_fallbacks(&[lang("en").unwrap()]);
        i.add_ftl("fr", "x = racine fr").unwrap();
        i.add_ftl("en", "x = root en").unwrap();
        assert_eq!(i.t("x", FluentArgs::new()), "racine fr"); // fr-FR → fr
    }
}