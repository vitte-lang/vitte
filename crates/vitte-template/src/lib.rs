//! vitte-template — moteur de templates pour Vitte
//!
//! Objectifs :
//! - API simple et stable pour rendre des templates depuis des chaînes, des fichiers, ou un registre
//! - Fallback portable sans dépendances: mini-moteur `{{name}}` + sections `{{#if x}}...{{/if}}`
//! - Intégration optionnelle avec `handlebars` si `feature = "handlebars"`
//! - Sérialisation optionnelle en JSON pour exporter/importer un registre
//!
//! Concepts :
//! - [`TemplateEngine`] façade polymorphe
//! - [`MiniEngine`] moteur intégré minimaliste (par défaut)
//! - [`HbEngine`] moteur handlebars si activé
//! - [`Registry`] stockage de templates nommés et partiels
//! - Helpers simples: upper, lower, len, json (mini), et helpers HB équivalents
//!
//! Sécurité :
//! - `#![forbid(unsafe_code)]`
//! - Aucun accès I/O implicite. Les fonctions `render_file*` lisent explicitement un chemin fourni.

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(
    clippy::doc_markdown,
    clippy::module_name_repetitions,
    clippy::too_many_lines,
    clippy::missing_errors_doc
)]

use anyhow::{Context, Result};
use std::collections::HashMap;
use std::fs;
use std::path::Path;

#[cfg(feature = "json")]
use serde::{Deserialize, Serialize};

#[cfg(feature = "handlebars")]
use handlebars::Handlebars;

/* =============================== Types de base =============================== */

/// Valeur de contexte minimale compréhensible par le mini-moteur.
/// Pour `handlebars`, toute valeur `serde_json::Value` est acceptée.
#[cfg_attr(feature = "json", derive(Serialize, Deserialize))]
#[derive(Clone, Debug, PartialEq)]
pub enum Value {
    Null,
    Bool(bool),
    Number(f64),
    String(String),
    Array(Vec<Value>),
    Object(HashMap<String, Value>),
}

impl Value {
    pub fn get(&self, key: &str) -> Option<&Value> {
        match self {
            Value::Object(m) => m.get(key),
            _ => None,
        }
    }
    fn truthy(&self) -> bool {
        match self {
            Value::Null => false,
            Value::Bool(b) => *b,
            Value::Number(n) => *n != 0.0,
            Value::String(s) => !s.is_empty(),
            Value::Array(v) => !v.is_empty(),
            Value::Object(m) => !m.is_empty(),
        }
    }
}

impl From<&str> for Value {
    fn from(s: &str) -> Self { Value::String(s.to_string()) }
}
impl From<String> for Value {
    fn from(s: String) -> Self { Value::String(s) }
}
impl From<bool> for Value {
    fn from(b: bool) -> Self { Value::Bool(b) }
}
impl From<f64> for Value {
    fn from(n: f64) -> Self { Value::Number(n) }
}
impl From<i64> for Value {
    fn from(n: i64) -> Self { Value::Number(n as f64) }
}
impl From<usize> for Value {
    fn from(n: usize) -> Self { Value::Number(n as f64) }
}
impl<K: Into<String>, V: Into<Value>> From<HashMap<K, V>> for Value {
    fn from(mut m: HashMap<K, V>) -> Self {
        let mut o = HashMap::<String, Value>::new();
        for (k, v) in m.drain() {
            o.insert(k.into(), v.into());
        }
        Value::Object(o)
    }
}

/* ================================ Registre =================================== */

/// Enregistrement d’un template nommé et de ses partiels.
#[cfg_attr(feature = "json", derive(Serialize, Deserialize))]
#[derive(Clone, Debug, Default, PartialEq)]
pub struct Registry {
    /// Templates nommés
    pub templates: HashMap<String, String>,
    /// Partiels utilisables via `{{> partial_name}}`
    pub partials: HashMap<String, String>,
}

impl Registry {
    pub fn new() -> Self { Self::default() }

    pub fn insert_template(&mut self, name: impl Into<String>, src: impl Into<String>) {
        self.templates.insert(name.into(), src.into());
    }
    pub fn insert_partial(&mut self, name: impl Into<String>, src: impl Into<String>) {
        self.partials.insert(name.into(), src.into());
    }
    pub fn get_template(&self, name: &str) -> Option<&str> {
        self.templates.get(name).map(String::as_str)
    }
    pub fn get_partial(&self, name: &str) -> Option<&str> {
        self.partials.get(name).map(String::as_str)
    }

    /// Importe tous les fichiers d’un dossier comme partiels, suffixe optionnel retiré.
    pub fn import_partials_from_dir(&mut self, dir: impl AsRef<Path>, strip_ext: bool) -> Result<usize> {
        let dir = dir.as_ref();
        let mut count = 0usize;
        for entry in fs::read_dir(dir).with_context(|| format!("read_dir {}", dir.display()))? {
            let entry = entry?;
            if !entry.file_type()?.is_file() {
                continue;
            }
            let path = entry.path();
            let name = if strip_ext {
                path.file_stem().and_then(|s| s.to_str()).unwrap_or_default().to_string()
            } else {
                path.file_name().and_then(|s| s.to_str()).unwrap_or_default().to_string()
            };
            let src = fs::read_to_string(&path).with_context(|| format!("read {}", path.display()))?;
            self.insert_partial(name, src);
            count += 1;
        }
        Ok(count)
    }

    /// Exporte le registre vers un fichier JSON (si `json`).
    #[cfg(feature = "json")]
    pub fn to_json_file(&self, path: impl AsRef<Path>) -> Result<()> {
        let s = serde_json::to_string_pretty(self)?;
        fs::write(path, s)?;
        Ok(())
    }

    /// Charge un registre depuis un fichier JSON (si `json`).
    #[cfg(feature = "json")]
    pub fn from_json_file(path: impl AsRef<Path>) -> Result<Self> {
        let s = fs::read_to_string(path)?;
        Ok(serde_json::from_str(&s)?)
    }
}

/* ============================== Interface moteur ============================= */

/// Interface standard d’un moteur de templates.
pub trait TemplateEngine {
    /// Enregistre des partiels dans le moteur.
    fn register_partials(&mut self, reg: &Registry) -> Result<()>;

    /// Rend une source de template contre un contexte.
    fn render_str(&self, src: &str, ctx: &Value) -> Result<String>;

    /// Rend un template nommé dans un [`Registry`].
    fn render_named(&self, reg: &Registry, name: &str, ctx: &Value) -> Result<String> {
        let src = reg
            .get_template(name)
            .ok_or_else(|| anyhow::anyhow!(format!("template '{name}' introuvable")))?;
        self.render_str(src, ctx)
    }

    /// Rend un fichier texte.
    fn render_file(&self, path: impl AsRef<Path>, ctx: &Value) -> Result<String> {
        let s = fs::read_to_string(&path).with_context(|| format!("read {}", path.as_ref().display()))?;
        self.render_str(&s, ctx)
    }
}

/* =============================== Mini moteur ================================ */

/// Moteur minimaliste avec :
/// - variables `{{key}}`
/// - appels helpers `{{upper key}}`, `{{lower key}}`, `{{len key}}`, `{{json key}}`
/// - partiels `{{> name}}`
/// - sections conditionnelles `{{#if key}}...{{/if}}`
/// - boucles sur tableaux `{{#each key}}...{{/each}}` avec `{{this}}`
/// Les chemins `a.b.c` sont supportés.
#[derive(Clone, Default)]
pub struct MiniEngine {
    partials: HashMap<String, String>,
}

impl MiniEngine {
    pub fn new() -> Self { Self::default() }

    fn lookup_path<'a>(&self, ctx: &'a Value, path: &str) -> Option<&'a Value> {
        let mut cur = ctx;
        for seg in path.split('.') {
            match cur {
                Value::Object(m) => {
                    cur = m.get(seg)?;
                }
                _ => return None,
            }
        }
        Some(cur)
    }

    fn render_inner(&self, src: &str, ctx: &Value) -> Result<String> {
        // Un parseur à base de pile pour sections.
        let mut out = String::with_capacity(src.len() + 16);
        let mut i = 0usize;
        let bytes = src.as_bytes();

        while i < bytes.len() {
            if i + 1 < bytes.len() && bytes[i] == b'{' && bytes[i + 1] == b'{' {
                // trouver }}
                if let Some(end) = find_delim(bytes, i + 2, b'}', b'}') {
                    let inside = src[i + 2..end].trim();
                    i = end + 2;

                    // section if
                    if inside.starts_with("#if ") {
                        let key = inside[4..].trim();
                        // trouver {{/if}}
                        if let Some((body, jump)) = find_section(src, i, "/if") {
                            let cond = if key == "this" { ctx.truthy() } else { self.lookup_path(ctx, key).map(|v| v.truthy()).unwrap_or(false) };
                            if cond {
                                out.push_str(&self.render_inner(body, ctx)?);
                            }
                            i = jump;
                            continue;
                        }
                    }
                    // each
                    if inside.starts_with("#each ") {
                        let key = inside[7..].trim();
                        if let Some((body, jump)) = find_section(src, i, "/each") {
                            if let Some(Value::Array(items)) = if key == "this" { Some(ctx) } else { self.lookup_path(ctx, key) }.cloned() {
                                for it in items {
                                    // rendre avec this = it
                                    let child = merge_this(ctx, it);
                                    out.push_str(&self.render_inner(body, &child)?);
                                }
                            }
                            i = jump;
                            continue;
                        }
                    }
                    // partiel
                    if inside.starts_with("> ") {
                        let name = inside[2..].trim();
                        if let Some(p) = self.partials.get(name) {
                            out.push_str(&self.render_inner(p, ctx)?);
                        } else {
                            return Err(anyhow::anyhow!(format!("partiel '{name}' introuvable")));
                        }
                        continue;
                    }
                    // variable ou helper
                    let mut parts = inside.split_whitespace();
                    if let Some(first) = parts.next() {
                        match first {
                            "upper" => {
                                let key = parts.next().ok_or_else(|| anyhow::anyhow!("upper: argument manquant"))?;
                                let v = val_to_string(if key == "this" { Some(ctx) } else { self.lookup_path(ctx, key) });
                                out.push_str(&v.to_uppercase());
                            }
                            "lower" => {
                                let key = parts.next().ok_or_else(|| anyhow::anyhow!("lower: argument manquant"))?;
                                let v = val_to_string(if key == "this" { Some(ctx) } else { self.lookup_path(ctx, key) });
                                out.push_str(&v.to_lowercase());
                            }
                            "len" => {
                                let key = parts.next().ok_or_else(|| anyhow::anyhow!("len: argument manquant"))?;
                                let n = match if key == "this" { Some(ctx) } else { self.lookup_path(ctx, key) } {
                                    Some(Value::Array(a)) => a.len(),
                                    Some(Value::Object(o)) => o.len(),
                                    Some(Value::String(s)) => s.chars().count(),
                                    Some(_) => 1,
                                    None => 0,
                                };
                                out.push_str(&n.to_string());
                            }
                            "json" => {
                                let key = parts.next().ok_or_else(|| anyhow::anyhow!("json: argument manquant"))?;
                                #[cfg(feature = "json")]
                                {
                                    let j = json_stringify(if key == "this" { Some(ctx) } else { self.lookup_path(ctx, key) });
                                    out.push_str(&j);
                                }
                                #[cfg(not(feature = "json"))]
                                {
                                    let v = val_to_string(if key == "this" { Some(ctx) } else { self.lookup_path(ctx, key) });
                                    out.push_str(&v);
                                }
                            }
                            _ => {
                                // variable simple
                                let key = inside;
                                if key == "this" {
                                    out.push_str(&val_to_string(Some(ctx)));
                                } else if let Some(v) = self.lookup_path(ctx, key) {
                                    out.push_str(&val_to_string(Some(v)));
                                } else {
                                    // inconnue → vide
                                }
                            }
                        }
                    }
                } else {
                    // pas de fin, écrire tel quel
                    out.push(src.as_bytes()[i] as char);
                    i += 1;
                }
            } else {
                out.push(src.as_bytes()[i] as char);
                i += 1;
            }
        }

        Ok(out)
    }
}

impl TemplateEngine for MiniEngine {
    fn register_partials(&mut self, reg: &Registry) -> Result<()> {
        self.partials = reg.partials.clone();
        Ok(())
    }

    fn render_str(&self, src: &str, ctx: &Value) -> Result<String> {
        self.render_inner(src, ctx)
    }
}

/* ============================= Handlebars (opt) ============================= */

/// Moteur handlebars si `feature = "handlebars"`.
#[cfg(feature = "handlebars")]
pub struct HbEngine {
    hb: Handlebars<'static>,
}

#[cfg(feature = "handlebars")]
impl HbEngine {
    pub fn new() -> Self {
        let mut hb = Handlebars::new();
        hb.register_escape_fn(handlebars::no_escape);
        // helpers équivalents
        hb.register_helper("upper", Box::new(h_upper));
        hb.register_helper("lower", Box::new(h_lower));
        hb.register_helper("len", Box::new(h_len));
        hb.register_helper("json", Box::new(h_json));
        Self { hb }
    }
}

#[cfg(feature = "handlebars")]
impl TemplateEngine for HbEngine {
    fn register_partials(&mut self, reg: &Registry) -> Result<()> {
        for (k, v) in &reg.partials {
            self.hb.register_partial(k, v)?;
        }
        Ok(())
    }

    fn render_str(&self, src: &str, ctx: &Value) -> Result<String> {
        // adapter Value -> serde_json::Value
        let j = to_json(ctx);
        Ok(self.hb.render_template(src, &j)?)
    }

    fn render_named(&self, reg: &Registry, name: &str, ctx: &Value) -> Result<String> {
        let src = reg.get_template(name).ok_or_else(|| anyhow::anyhow!("template introuvable"))?;
        self.render_str(src, ctx)
    }

    fn render_file(&self, path: impl AsRef<Path>, ctx: &Value) -> Result<String> {
        let s = fs::read_to_string(&path)?;
        self.render_str(&s, ctx)
    }
}

/* ---------------------- Helpers handlebars (opt) ---------------------- */

#[cfg(feature = "handlebars")]
fn h_upper(
    h: &handlebars::Helper<'_, '_>,
    _: &Handlebars<'_>,
    _: &handlebars::Context,
    _: &mut handlebars::RenderContext<'_, '_>,
    out: &mut dyn handlebars::Output,
) -> handlebars::HelperResult {
    let p = h
        .param(0)
        .ok_or(handlebars::RenderErrorReason::ParamNotFoundForIndex("upper", 0))?;
    out.write(&p.value().as_str().unwrap_or("").to_uppercase())?;
    Ok(())
}

#[cfg(feature = "handlebars")]
fn h_lower(
    h: &handlebars::Helper<'_, '_>,
    _: &Handlebars<'_>,
    _: &handlebars::Context,
    _: &mut handlebars::RenderContext<'_, '_>,
    out: &mut dyn handlebars::Output,
) -> handlebars::HelperResult {
    let p = h.param(0).ok_or(handlebars::RenderErrorReason::ParamNotFoundForIndex("lower", 0))?;
    out.write(&p.value().as_str().unwrap_or("").to_lowercase())?;
    Ok(())
}

#[cfg(feature = "handlebars")]
fn h_len(
    h: &handlebars::Helper<'_, '_>,
    _: &Handlebars<'_>,
    _: &handlebars::Context,
    _: &mut handlebars::RenderContext<'_, '_>,
    out: &mut dyn handlebars::Output,
) -> handlebars::HelperResult {
    let p = h.param(0).ok_or(handlebars::RenderErrorReason::ParamNotFoundForIndex("len", 0))?;
    let n = match p.value() {
        v if v.is_string() => v.as_str().unwrap_or("").chars().count(),
        v if v.is_array() => v.as_array().map(|a| a.len()).unwrap_or(0),
        v if v.is_object() => v.as_object().map(|o| o.len()).unwrap_or(0),
        v if v.is_null() => 0,
        _ => 1,
    };
    out.write(&n.to_string())?;
    Ok(())
}

#[cfg(feature = "handlebars")]
fn h_json(
    h: &handlebars::Helper<'_, '_>,
    _: &Handlebars<'_>,
    _: &handlebars::Context,
    _: &mut handlebars::RenderContext<'_, '_>,
    out: &mut dyn handlebars::Output,
) -> handlebars::HelperResult {
    let p = h.param(0).ok_or(handlebars::RenderErrorReason::ParamNotFoundForIndex("json", 0))?;
    out.write(&serde_json::to_string(p.value()).unwrap_or_else(|_| "null".into()))?;
    Ok(())
}

/* =============================== Façades utiles =============================== */

/// Rend avec le mini-moteur par défaut.
pub fn render_str(src: &str, ctx: &Value, reg: Option<&Registry>) -> Result<String> {
    #[cfg(feature = "handlebars")]
    {
        // Par défaut on reste sur MiniEngine pour éviter d'imposer HB quand non demandé.
    }
    let mut eng = MiniEngine::new();
    if let Some(r) = reg { eng.register_partials(r)?; }
    eng.render_str(src, ctx)
}

/// Rend un template nommé depuis un registre.
pub fn render_named(reg: &Registry, name: &str, ctx: &Value) -> Result<String> {
    let mut eng = MiniEngine::new();
    eng.register_partials(reg)?;
    eng.render_named(reg, name, ctx)
}

/// Rend un fichier en utilisant le mini-moteur.
pub fn render_file(path: impl AsRef<Path>, ctx: &Value, reg: Option<&Registry>) -> Result<String> {
    let s = fs::read_to_string(&path)?;
    render_str(&s, ctx, reg)
}

/// Rend avec handlebars si la feature est activée.
#[cfg(feature = "handlebars")]
pub fn render_str_hb(src: &str, ctx: &Value, reg: Option<&Registry>) -> Result<String> {
    let mut hb = HbEngine::new();
    if let Some(r) = reg { hb.register_partials(r)?; }
    hb.render_str(src, ctx)
}

/* ================================== Helpers ================================== */

fn find_delim(bytes: &[u8], mut i: usize, a: u8, b: u8) -> Option<usize> {
    while i + 1 < bytes.len() {
        if bytes[i] == a && bytes[i + 1] == b { return Some(i); }
        i += 1;
    }
    None
}

fn find_section<'a>(src: &'a str, start: usize, end_tag: &str) -> Option<(&'a str, usize)> {
    // Cherche `{{end_tag}}` au même niveau (non imbriqué pour la même balise).
    // Pour simplicité, on ne supporte pas l'imbrication du même type.
    let needle = format!("{{{{{end_tag}}}}}");
    let idx = src[start..].find(&needle)?;
    let body = &src[start..start + idx];
    let jump = start + idx + needle.len();
    Some((body, jump))
}

fn val_to_string(v: Option<&Value>) -> String {
    match v {
        Some(Value::Null) | None => "".into(),
        Some(Value::Bool(b)) => b.to_string(),
        Some(Value::Number(n)) => {
            if (n.fract() - 0.0).abs() < f64::EPSILON { (*n as i64).to_string() } else { n.to_string() }
        }
        Some(Value::String(s)) => s.clone(),
        Some(Value::Array(_)) | Some(Value::Object(_)) => "[object]".into(),
    }
}

#[cfg(feature = "json")]
fn json_stringify(v: Option<&Value>) -> String {
    let serde_v = to_json_opt(v);
    serde_json::to_string(&serde_v).unwrap_or_else(|_| "null".into())
}

#[cfg(feature = "json")]
fn to_json_opt(v: Option<&Value>) -> serde_json::Value {
    match v {
        None => serde_json::Value::Null,
        Some(Value::Null) => serde_json::Value::Null,
        Some(Value::Bool(b)) => serde_json::Value::Bool(*b),
        Some(Value::Number(n)) => serde_json::json!(n),
        Some(Value::String(s)) => serde_json::Value::String(s.clone()),
        Some(Value::Array(a)) => serde_json::Value::Array(a.iter().map(|x| to_json_opt(Some(x))).collect()),
        Some(Value::Object(m)) => {
            let mut map = serde_json::Map::new();
            for (k, v) in m {
                map.insert(k.clone(), to_json_opt(Some(v)));
            }
            serde_json::Value::Object(map)
        }
    }
}

#[cfg(feature = "handlebars")]
fn to_json(v: &Value) -> serde_json::Value {
    to_json_opt(Some(v))
}

fn merge_this(ctx: &Value, this: Value) -> Value {
    // Object with { this: this, ..ctx}
    let mut root = match ctx {
        Value::Object(m) => m.clone(),
        _ => HashMap::new(),
    };
    root.insert("this".into(), this);
    Value::Object(root)
}

/* ================================== Tests ================================== */

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn mini_vars_and_helpers() {
        let mut obj = HashMap::new();
        obj.insert("name".to_string(), Value::from("Vitte"));
        obj.insert("n".to_string(), Value::from(3usize));
        let ctx = Value::from(obj);

        let s = "Hello {{name}}! {{upper name}} {{len name}} {{json name}}";
        let out = render_str(s, &ctx, None).unwrap();
        assert!(out.contains("Hello Vitte! VITTE"));
    }

    #[test]
    fn mini_if_and_each() {
        let ctx = Value::Object(HashMap::from([
            ("ok".into(), Value::Bool(true)),
            ("xs".into(), Value::Array(vec![Value::from("a"), Value::from("b")])),
        ]));
        let s = "{{#if ok}}yes{{/if}} {{#each xs}}[{{this}}]{{/each}}";
        let out = render_str(s, &ctx, None).unwrap();
        assert_eq!(out, "yes [a][b]");
    }

    #[test]
    fn partials_and_registry() {
        let mut reg = Registry::new();
        reg.insert_partial("p", "P({{this}})");
        reg.insert_template("t", "X {{> p}} Y");

        let out = render_named(&reg, "t", &Value::from("k")).unwrap();
        assert_eq!(out, "X P(k) Y");
    }

    #[cfg(feature = "handlebars")]
    #[test]
    fn handlebars_path() {
        let mut reg = Registry::new();
        reg.insert_template("t", "hi {{upper name}}");
        let mut eng = HbEngine::new();
        eng.register_partials(&reg).unwrap();
        let ctx = Value::from(HashMap::from([("name", Value::from("vitte"))]));
        let out = eng.render_named(&reg, "t", &ctx).unwrap();
        assert!(out.contains("VITTE"));
    }
}