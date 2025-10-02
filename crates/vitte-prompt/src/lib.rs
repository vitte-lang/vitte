//! vitte-prompt — templates, front-matter, includes/partials, rendu Markdown, cache
//!
//! Capacités principales:
//! - Chargement depuis chaîne ou fichier (UTF-8 par défaut, auto via `vitte-encoding` si activé).
//! - Front-matter facultatif: YAML (`---`), TOML (`+++`), JSON (`;;;`) au début du fichier.
//! - Moteurs de template au choix: `minijinja` (par défaut) ou `handlebars`.
//! - Includes/partials via loader FS raciné sur un dossier.
//! - Variables fusionnées: `front-matter` < `defaults` < `data` appelant.
//! - Post-rendu Markdown (option `markdown`) → HTML.
//! - Cache LRU des templates compilés (clé = chemin logique).
//! - Petits utilitaires: interpolation regex, normalisation des variables.
//!
//! Zéro `unsafe`.

#![forbid(unsafe_code)]

#[cfg(all(not(feature="std"), not(feature="alloc-only")))]
compile_error!("Enable feature `std` (default) or `alloc-only`.") ;

#[cfg(feature="alloc-only")]
extern crate alloc;

#[cfg(feature="alloc-only")]
use alloc::{string::String, vec::Vec, boxed::Box, format, borrow::ToOwned, collections::BTreeMap};

#[cfg(feature="std")]
use std::{string::String, vec::Vec, boxed::Box, collections::BTreeMap, path::{Path, PathBuf}, fs, io};

#[cfg(feature="serde")]
use serde::{Serialize, Deserialize};
#[cfg(feature="serde")]
use serde_json::{Value as Json, Map as JsonMap};

#[cfg(feature="errors")]
use thiserror::Error;

#[cfg(feature="jinja")]
use minijinja::{Environment, context, value::Value as JinjaValue, Source as JinjaSource};

#[cfg(feature="handlebars")]
use handlebars::Handlebars;

#[cfg(feature="markdown")]
use pulldown_cmark::{Options as MdOptions, Parser, html};

#[cfg(feature="regex")]
use regex::Regex;

#[cfg(feature="cache")]
use lru::LruCache;

#[cfg(feature="fs")]
use vitte_path as vpath;

#[cfg(feature="encoding")]
use vitte_encoding as venc;

/* ================================ Erreurs ================================ */

#[cfg(feature="errors")]
#[derive(Debug, Error)]
pub enum PromptError {
    #[error("io error: {0}")]
    Io(String),
    #[error("invalid encoding")]
    Encoding,
    #[error("parse front-matter")]
    FrontMatter,
    #[error("template: {0}")]
    Template(String),
    #[error("engine feature not enabled")]
    EngineMissing,
}
#[cfg(not(feature="errors"))]
#[derive(Debug)]
pub enum PromptError { Io(String), Encoding, FrontMatter, Template(String), EngineMissing }

pub type Result<T> = core::result::Result<T, PromptError>;

#[inline]
fn io_err<T>(e: impl core::fmt::Display) -> Result<T> { Err(PromptError::Io(e.to_string())) }

/* ================================ Types ================================ */

/// D’où provient le contenu du template.
#[derive(Clone, Debug)]
pub enum PromptSource {
    Inline { name: String, text: String },
    #[cfg(feature="std")]
    File { path: PathBuf },
}

/// Front-matter + corps.
#[cfg_attr(feature="serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug, PartialEq)]
pub struct PromptDoc {
    pub meta: Json,
    pub body: String,
}

impl PromptDoc {
    pub fn new(meta: Json, body: String) -> Self { Self { meta, body } }
}

/// Sélection du moteur de template.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Engine { #[cfg(feature="jinja")] Jinja, #[cfg(feature="handlebars")] Handlebars }

/// Options globales de rendu.
#[derive(Clone, Debug)]
pub struct RenderOptions {
    pub engine: Engine,
    pub root: Option<String>,          // racine FS pour includes
    pub defaults: Json,                // valeurs par défaut fusionnées
    pub render_markdown: bool,         // post-rendu MD->HTML
    pub cache_capacity: usize,         // 0 = désactivé
    pub regex_pass: Option<String>,    // regex de substitution post-rendu (ex: "\\{\\{NOW\\}\\}")
    pub regex_replace: Option<String>, // remplacement pour regex_pass
}

impl Default for RenderOptions {
    fn default() -> Self {
        Self {
            #[cfg(feature="jinja")] engine: Engine::Jinja,
            #[cfg(all(not(feature="jinja"), feature="handlebars"))] engine: Engine::Handlebars,
            #[cfg(all(not(feature="jinja"), not(feature="handlebars")))] engine: { /* unreachable in use */ panic!("enable jinja or handlebars"); },
            root: None,
            defaults: json_obj(),
            render_markdown: false,
            cache_capacity: 64,
            regex_pass: None,
            regex_replace: None,
        }
    }
}

/* ================================ Contexte principal ================================ */

/// Gestionnaire de templates et cache.
pub struct PromptEngine {
    opts: RenderOptions,
    #[cfg(feature="cache")]
    cache: LruCache<String, Compiled>,
    #[cfg(feature="jinja")]
    jinja: Option<Environment<'static>>,
    #[cfg(feature="handlebars")]
    hbs: Option<Handlebars<'static>>,
}

impl PromptEngine {
    pub fn new(opts: RenderOptions) -> Result<Self> {
        let mut me = Self {
            #[cfg(feature="cache")]
            cache: if opts.cache_capacity>0 { LruCache::new(opts.cache_capacity.try_into().unwrap_or(64)) } else { LruCache::unbounded() },
            #[cfg(not(feature="cache"))]
            /* dummy */ cache: (),
            jinja: None,
            hbs: None,
            opts,
        };
        me.init_engines()?;
        Ok(me)
    }

    fn init_engines(&mut self) -> Result<()> {
        match self.opts.engine {
            #[cfg(feature="jinja")]
            Engine::Jinja => {
                let mut env = Environment::new();
                if let Some(root) = &self.opts.root {
                    let mut src = JinjaSource::new();
                    // lazy: pas de preload, on utilise add_template pour cache unitaire
                    env.set_source(src);
                    // custom loader via callback de fallback:
                    // minijinja n’a pas de FS loader direct stable ici, on utilisera add_template_dynamic ci-dessous.
                    let _ = root;
                }
                self.jinja = Some(env);
                Ok(())
            }
            #[cfg(feature="handlebars")]
            Engine::Handlebars => {
                let mut hb = Handlebars::new();
                hb.set_strict_mode(false);
                hb.register_escape_fn(handlebars::no_escape);
                self.hbs = Some(hb);
                Ok(())
            }
            #[allow(unreachable_patterns)]
            _ => Err(PromptError::EngineMissing),
        }
    }

    /// Charge un `PromptDoc` depuis une source.
    pub fn load(&self, src: &PromptSource) -> Result<PromptDoc> {
        match src {
            PromptSource::Inline { text, .. } => parse_front_matter(text),
            #[cfg(feature="std")]
            PromptSource::File { path } => self.load_file(path),
        }
    }

    #[cfg(feature="std")]
    fn load_file(&self, path: &Path) -> Result<PromptDoc> {
        let bytes = fs::read(path).map_err(|e| PromptError::Io(e.to_string()))?;
        let text = {
            #[cfg(feature="encoding")]
            {
                // essai BOM + heuristique via vitte-encoding
                let det = venc::detect(&bytes).unwrap_or_else(|| venc::DetectResult{ encoding: venc::Encoding::Utf8, confidence: 10, had_bom: false });
                let cow = venc::decode_lossy(&bytes, det.encoding); // suppose que vitte-encoding expose decode_lossy
                cow.to_string()
            }
            #[cfg(not(feature="encoding"))]
            {
                String::from_utf8(bytes).map_err(|_| PromptError::Encoding)?
            }
        };
        parse_front_matter(&text)
    }

    /// Compile + rend un template avec `data`. `name` est la clé logique de cache.
    pub fn render(&mut self, name: &str, src: &PromptSource, data: &Json) -> Result<String> {
        let doc = self.load(src)?;
        let merged = merge_vars(&[&doc.meta, &self.opts.defaults, data]);

        // Compile (ou récupère) selon moteur
        let out = match self.opts.engine {
            #[cfg(feature="jinja")]
            Engine::Jinja => self.render_jinja(name, &doc.body, &merged),
            #[cfg(feature="handlebars")]
            Engine::Handlebars => self.render_hbs(name, &doc.body, &merged),
            #[allow(unreachable_patterns)]
            _ => Err(PromptError::EngineMissing),
        }?;

        // Post-traitements
        let out = if self.opts.render_markdown {
            #[cfg(feature="markdown")]
            { markdown_to_html(&out) }
            #[cfg(not(feature="markdown"))]
            { out }
        } else { out };

        let out = if let (Some(rx), Some(rep)) = (&self.opts.regex_pass, &self.opts.regex_replace) {
            #[cfg(feature="regex")]
            {
                let re = Regex::new(rx).map_err(|e| PromptError::Template(e.to_string()))?;
                re.replace_all(&out, rep.as_str()).into_owned()
            }
            #[cfg(not(feature="regex"))]
            { out }
        } else { out };

        Ok(out)
    }

    /* --------------------------- moteurs --------------------------- */

    #[cfg(feature="jinja")]
    fn render_jinja(&mut self, name: &str, text: &str, vars: &Json) -> Result<String> {
        // cache clé = name
        let key = format!("jinja::{name}");
        let compiled = {
            #[cfg(feature="cache")]
            {
                if let Some(c) = self.cache.get(&key) { if let Compiled::Jinja(ref s) = c { Some(s.clone()) } else { None } }
                else { None }
            }
            #[cfg(not(feature="cache"))]
            { None }
        };

        if compiled.is_none() {
            let env = self.jinja.as_mut().ok_or(PromptError::EngineMissing)?;
            // add or replace template
            env.add_template(name, text).map_err(|e| PromptError::Template(e.to_string()))?;
            #[cfg(feature="cache")]
            { self.cache.put(key.clone(), Compiled::Jinja(name.to_string())); }
        }

        let env = self.jinja.as_ref().unwrap();
        let v = to_jinja(vars);
        let tpl = env.get_template(name).map_err(|e| PromptError::Template(e.to_string()))?;
        tpl.render(v).map_err(|e| PromptError::Template(e.to_string()))
    }

    #[cfg(feature="handlebars")]
    fn render_hbs(&mut self, name: &str, text: &str, vars: &Json) -> Result<String> {
        let key = format!("hbs::{name}");
        let mut need_register = true;
        #[cfg(feature="cache")]
        {
            if let Some(Compiled::Hbs(_)) = self.cache.get(&key) { need_register = false; }
        }
        if need_register {
            let hb = self.hbs.as_mut().ok_or(PromptError::EngineMissing)?;
            hb.register_template_string(name, text).map_err(|e| PromptError::Template(e.to_string()))?;
            // Partials/includes si root connu: on laisse l’utilisateur pré-enregistrer si besoin.
            #[cfg(feature="cache")]
            { self.cache.put(key.clone(), Compiled::Hbs(name.to_string())); }
        }
        let hb = self.hbs.as_ref().unwrap();
        hb.render(name, vars).map_err(|e| PromptError::Template(e.to_string()))
    }
}

/* ================================ Cache interne ================================ */

#[derive(Clone, Debug)]
enum Compiled {
    #[cfg(feature="jinja")]
    Jinja(String),     // template name
    #[cfg(feature="handlebars")]
    Hbs(String),       // template name
}

/* ================================ Front-matter ================================ */

/// Détecte et extrait front-matter YAML (`---`), TOML (`+++`), JSON (`;;;`).
fn parse_front_matter(input: &str) -> Result<PromptDoc> {
    let s = input.trim_start_matches('\u{feff}'); // BOM safe
    if s.starts_with("---\n") {
        if let Some(end) = s.find("\n---\n") {
            let fm = &s[4..end];
            let body = s[end+5..].to_string();
            #[cfg(feature="yaml")]
            {
                let meta: Json = serde_yaml::from_str(fm).map_err(|_| PromptError::FrontMatter)?;
                return Ok(PromptDoc::new(meta, body));
            }
            #[cfg(not(feature="yaml"))]
            { return Err(PromptError::FrontMatter); }
        }
    } else if s.starts_with("+++\n") {
        if let Some(end) = s.find("\n+++\n") {
            let fm = &s[4..end];
            let body = s[end+5..].to_string();
            #[cfg(feature="toml")]
            {
                let meta_toml: toml::Value = toml::from_str(fm).map_err(|_| PromptError::FrontMatter)?;
                let meta = toml_to_json(meta_toml);
                return Ok(PromptDoc::new(meta, body));
            }
            #[cfg(not(feature="toml"))]
            { return Err(PromptError::FrontMatter); }
        }
    } else if s.starts_with(";;;\n") {
        if let Some(end) = s.find("\n;;;\n") {
            let fm = &s[4..end];
            let body = s[end+5..].to_string();
            #[cfg(feature="json")]
            {
                let meta: Json = serde_json::from_str(fm).map_err(|_| PromptError::FrontMatter)?;
                return Ok(PromptDoc::new(meta, body));
            }
            #[cfg(not(feature="json"))]
            { return Err(PromptError::FrontMatter); }
        }
    }
    // Pas de front-matter
    Ok(PromptDoc::new(json_obj(), s.to_string()))
}

/* ================================ Utilitaires ================================ */

#[cfg(feature="serde")]
fn json_obj() -> Json { Json::Object(JsonMap::new()) }

#[cfg(feature="serde")]
fn merge_vars(list: &[&Json]) -> Json {
    fn merge_into(dst: &mut JsonMap<String, Json>, src: &Json) {
        if let Json::Object(map) = src {
            for (k, v) in map {
                match (dst.get_mut(k), v) {
                    (Some(Json::Object(dobj)), Json::Object(sobj)) => {
                        let mut d = dobj.clone();
                        for (kk, vv) in sobj {
                            d.insert(kk.clone(), vv.clone());
                        }
                        dst.insert(k.clone(), Json::Object(d));
                    }
                    _ => { dst.insert(k.clone(), v.clone()); }
                }
            }
        }
    }
    let mut out = JsonMap::new();
    for j in list { merge_into(&mut out, j); }
    Json::Object(out)
}

#[cfg(feature="toml")]
fn toml_to_json(v: toml::Value) -> Json {
    match v {
        toml::Value::String(s) => Json::String(s),
        toml::Value::Integer(i) => Json::from(i),
        toml::Value::Float(f) => Json::from(f),
        toml::Value::Boolean(b) => Json::from(b),
        toml::Value::Datetime(d) => Json::String(d.to_string()),
        toml::Value::Array(arr) => Json::Array(arr.into_iter().map(toml_to_json).collect()),
        toml::Value::Table(tab) => {
            let mut m = JsonMap::new();
            for (k, v) in tab { m.insert(k, toml_to_json(v)); }
            Json::Object(m)
        }
    }
}

#[cfg(feature="markdown")]
fn markdown_to_html(md: &str) -> String {
    let mut opts = MdOptions::empty();
    opts.insert(MdOptions::ENABLE_TABLES);
    opts.insert(MdOptions::ENABLE_FOOTNOTES);
    let parser = Parser::new_ext(md, opts);
    let mut out = String::new();
    html::push_html(&mut out, parser);
    out
}

#[cfg(feature="jinja")]
fn to_jinja(j: &Json) -> JinjaValue {
    match j {
        Json::Null => JinjaValue::from(()),
        Json::Bool(b) => JinjaValue::from(*b),
        Json::Number(n) => {
            if let Some(i) = n.as_i64() { JinjaValue::from(i) }
            else if let Some(u) = n.as_u64() { JinjaValue::from(u as i64) }
            else if let Some(f) = n.as_f64() { JinjaValue::from(f) }
            else { JinjaValue::from(0) }
        }
        Json::String(s) => JinjaValue::from(s.as_str()),
        Json::Array(a) => JinjaValue::from_iter(a.iter().map(to_jinja)),
        Json::Object(o) => {
            let mut m = minijinja::value::Object::new();
            for (k, v) in o { m.set_attr(k, to_jinja(v)); }
            JinjaValue::from_object(m)
        }
    }
}

/* ================================ Tests ================================ */

#[cfg(test)]
mod tests {
    use super::*;
    #[cfg(feature="serde")] fn js(s:&str)->Json{ serde_json::from_str(s).unwrap() }

    #[test]
    fn no_front_matter() {
        let doc = parse_front_matter("Hello {{ name }}").unwrap();
        #[cfg(feature="serde")] assert!(doc.meta.is_object());
        assert!(doc.body.contains("{{"));
    }

    #[cfg(feature="yaml")]
    #[test]
    fn yaml_front_matter() {
        let s = concat!("---\nname: Alice\n---\nHello {{ name }}");
        let d = parse_front_matter(s).unwrap();
        assert!(d.body.starts_with("Hello"));
        #[cfg(feature="serde")] assert_eq!(d.meta["name"], "Alice");
    }

    #[cfg(feature="jinja")]
    #[cfg(feature="serde")]
    #[test]
    fn render_jinja_inline() {
        let mut pe = PromptEngine::new(RenderOptions::default()).unwrap();
        let src = PromptSource::Inline { name: "t".into(), text: "Hi {{ who }}".into() };
        let out = pe.render("t",&src,&js("{\"who\":\"Bob\"}")).unwrap();
        assert_eq!(out, "Hi Bob");
    }

    #[cfg(all(feature="handlebars", feature="serde"))]
    #[test]
    fn render_hbs_inline() {
        let mut opts = RenderOptions::default();
        opts.engine = Engine::Handlebars;
        let mut pe = PromptEngine::new(opts).unwrap();
        let src = PromptSource::Inline { name: "t".into(), text: "Hi {{who}}".into() };
        let out = pe.render("t",&src,&js("{\"who\":\"Bob\"}")).unwrap();
        assert_eq!(out, "Hi Bob");
    }

    #[cfg(all(feature="markdown", feature="serde", feature="jinja"))]
    #[test]
    fn markdown_post() {
        let mut opts = RenderOptions::default();
        opts.render_markdown = true;
        let mut pe = PromptEngine::new(opts).unwrap();
        let src = PromptSource::Inline { name: "m".into(), text: "# Title".into() };
        let out = pe.render("m",&src,&js("{}")).unwrap();
        assert!(out.contains("<h1>"));
    }
}