//! vitte-project — gestion de projets Vitte (manifestes, workspaces, build)
//!
//! Capacités:
//! - Détecter la racine d’un projet (fichier `vitte.toml` ou `vitte.json`)
//! - Charger un manifeste (TOML ou JSON selon l’extension et les features)
//! - Résoudre un workspace (liste de membres explicites)
//! - Construire un ordre de build topologique selon les dépendances internes
//!
//! API rapide:
//! - [`find_project_root`], [`load_manifest`]
//! - [`resolve_workspace`], [`build_plan`]
//!
//! Format TOML minimal attendu (exemple):
//! ```toml
//! [package]
//! name = "foo"
//! version = "0.1.0"
//!
//! [workspace]
//! members = ["crates/a", "crates/b"]
//!
//! [dependencies]
//! a = "0.1"
//! ```
//!
//! Format JSON équivalent si `vitte.json` (feature `json`).

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(clippy::module_name_repetitions, clippy::doc_markdown, clippy::too_many_lines)]

use anyhow::{bail, Context, Result};
use std::collections::{HashMap, HashSet, VecDeque};
use std::fs;
use std::path::{Path, PathBuf};

#[cfg(any(feature = "json", feature = "toml"))]
use serde::{Deserialize, Serialize};

/// Nom des fichiers de manifeste supportés.
pub const MANIFEST_TOML: &str = "vitte.toml";
pub const MANIFEST_JSON: &str = "vitte.json";

/* ============================ Modèles =================================== */

/// Section `[package]`.
#[cfg_attr(any(feature = "json", feature = "toml"), derive(Serialize, Deserialize))]
#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct Package {
    pub name: String,
    pub version: String,
}

/// Dépendance simple (nom -> req semver arbitraire).
#[cfg_attr(any(feature = "json", feature = "toml"), derive(Serialize, Deserialize))]
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Dependency {
    pub name: String,
    pub version: String,
}
impl Dependency {
    pub fn new(name: impl Into<String>, version: impl Into<String>) -> Self {
        Self { name: name.into(), version: version.into() }
    }
}

/// Section `[workspace]`.
#[cfg_attr(any(feature = "json", feature = "toml"), derive(Serialize, Deserialize))]
#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct Workspace {
    /// Chemins relatifs explicites vers les membres.
    pub members: Vec<String>,
}

/// Manifeste complet.
#[cfg_attr(any(feature = "json", feature = "toml"), derive(Serialize, Deserialize))]
#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct Manifest {
    pub package: Package,
    #[cfg_attr(any(feature = "json", feature = "toml"), serde(default))]
    pub workspace: Option<Workspace>,
    /// Map TOML `[dependencies]` convertie en liste pour stabilité.
    #[cfg_attr(any(feature = "json", feature = "toml"), serde(default))]
    pub dependencies: HashMap<String, String>,
}

impl Manifest {
    /// Dépendances sous forme normalisée.
    pub fn deps(&self) -> Vec<Dependency> {
        let mut v: Vec<_> = self
            .dependencies
            .iter()
            .map(|(k, ver)| Dependency::new(k.clone(), ver.clone()))
            .collect();
        v.sort_by(|a, b| a.name.cmp(&b.name));
        v
    }
}

/* ============================ Chargement ================================= */

/// Détecte la racine de projet en remontant les dossiers.
pub fn find_project_root(start: impl AsRef<Path>) -> Option<PathBuf> {
    let mut cur = Some(start.as_ref().to_path_buf());
    while let Some(dir) = cur {
        let toml = dir.join(MANIFEST_TOML);
        let json = dir.join(MANIFEST_JSON);
        if toml.is_file() || json.is_file() {
            return Some(dir);
        }
        cur = dir.parent().map(|p| p.to_path_buf());
    }
    None
}

/// Charge un manifeste depuis un chemin fichier.
pub fn load_manifest(path: impl AsRef<Path>) -> Result<Manifest> {
    let p = path.as_ref();
    let data = fs::read_to_string(p).with_context(|| format!("read {}", p.display()))?;
    match p.extension().and_then(|e| e.to_str()) {
        Some("toml") => load_manifest_toml_str(&data),
        Some("json") => load_manifest_json_str(&data),
        _ => bail!("extension inconnue pour manifeste: {}", p.display()),
    }
}

/// Charge `vitte.toml` ou `vitte.json` situé à `root`.
pub fn load_manifest_at_root(root: impl AsRef<Path>) -> Result<Manifest> {
    let root = root.as_ref();
    let p = root.join(MANIFEST_TOML);
    if p.is_file() {
        return load_manifest(&p);
    }
    let j = root.join(MANIFEST_JSON);
    if j.is_file() {
        return load_manifest(&j);
    }
    bail!("manifeste introuvable sous {}", root.display());
}

#[cfg(feature = "toml")]
fn load_manifest_toml_str(s: &str) -> Result<Manifest> {
    use toml::Value;
    let v: Value = s.parse::<Value>().context("parse TOML")?;
    // Deserialize direct si possible
    let mut m: Manifest = v.clone().try_into().context("deserialize Manifest")?;
    // Normaliser [dependencies] si présente sous table
    if let Some(Value::Table(deps)) = v.get("dependencies") {
        let map: HashMap<String, String> = deps
            .iter()
            .map(|(k, v)| (k.clone(), match v {
                Value::String(s) => s.clone(),
                _ => "*".to_string(),
            }))
            .collect();
        m.dependencies = map;
    }
    Ok(m)
}
#[cfg(not(feature = "toml"))]
fn load_manifest_toml_str(_s: &str) -> Result<Manifest> {
    bail!("feature `toml` désactivée")
}

#[cfg(feature = "json")]
fn load_manifest_json_str(s: &str) -> Result<Manifest> {
    Ok(serde_json::from_str(s).context("parse JSON")?)
}
#[cfg(not(feature = "json"))]
fn load_manifest_json_str(_s: &str) -> Result<Manifest> {
    bail!("feature `json` désactivée")
}

/* ============================ Workspace ================================== */

/// Représentation d’un paquet concret dans un workspace.
#[derive(Debug, Clone)]
pub struct Project {
    pub root: PathBuf,
    pub manifest: Manifest,
}

impl Project {
    pub fn name(&self) -> &str { &self.manifest.package.name }
    pub fn deps(&self) -> Vec<Dependency> { self.manifest.deps() }
}

/// Résout les membres du workspace à partir de `root`.
/// Les chemins doivent être explicites et contenir un manifeste.
pub fn resolve_workspace(root: impl AsRef<Path>) -> Result<Vec<Project>> {
    let root = root.as_ref();
    let top = load_manifest_at_root(root)?;
    let members = top.workspace.as_ref().map(|w| w.members.clone()).unwrap_or_default();

    // Si pas de workspace: renvoyer le projet seul.
    if members.is_empty() {
        return Ok(vec![Project { root: root.to_path_buf(), manifest: top }]);
    }

    let mut out = Vec::new();
    for rel in members {
        let p = root.join(rel);
        let r = find_project_root(&p).unwrap_or(p.clone());
        let m = load_manifest_at_root(&r)
            .with_context(|| format!("membre de workspace invalide: {}", r.display()))?;
        out.push(Project { root: r, manifest: m });
    }
    Ok(out)
}

/* ============================ Build plan ================================= */

/// Ordre de build topologique sur les membres internes.
/// Les dépendances externes (non présentes parmi `projects`) sont ignorées.
pub fn build_plan(projects: &[Project]) -> Result<Vec<String>> {
    // Map nom -> index
    let mut index = HashMap::<&str, usize>::new();
    for (i, p) in projects.iter().enumerate() {
        if index.insert(p.name(), i).is_some() {
            bail!("nom du paquet dupliqué dans le workspace: {}", p.name());
        }
    }

    // Graph: edges dep -> pkg
    let n = projects.len();
    let mut indeg = vec![0usize; n];
    let mut adj = vec![Vec::<usize>::new(); n];

    for (i, p) in projects.iter().enumerate() {
        for d in p.deps() {
            if let Some(&j) = index.get(d.name.as_str()) {
                // j construit avant i
                adj[j].push(i);
                indeg[i] += 1;
            }
        }
    }

    // Kahn
    let mut q = VecDeque::new();
    for i in 0..n {
        if indeg[i] == 0 {
            q.push_back(i);
        }
    }
    let mut order = Vec::<usize>::new();
    while let Some(u) = q.pop_front() {
        order.push(u);
        for &v in &adj[u] {
            indeg[v] -= 1;
            if indeg[v] == 0 {
                q.push_back(v);
            }
        }
    }
    if order.len() != n {
        // cycle entre membres internes
        bail!("cycle de dépendances détecté dans le workspace");
    }

    Ok(order.into_iter().map(|i| projects[i].name().to_string()).collect())
}

/* ============================ Utilitaires ================================ */

/// Cherche la dépendance nommée dans un projet.
pub fn has_internal_dep(projects: &[Project], pkg: &str, dep: &str) -> bool {
    let set: HashSet<&str> = projects.iter().map(|p| p.name()).collect();
    if !set.contains(pkg) || !set.contains(dep) {
        return false;
    }
    projects
        .iter()
        .find(|p| p.name() == pkg)
        .map(|p| p.deps().iter().any(|d| d.name == dep))
        .unwrap_or(false)
}

/* ============================== Tests =================================== */

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::Write;
    use tempfile::tempdir;

    #[cfg(feature = "toml")]
    fn write_pkg(dir: &Path, name: &str, deps: &[(&str, &str)]) -> PathBuf {
        let p = dir.join(name);
        fs::create_dir_all(&p).unwrap();
        let mut toml = String::new();
        toml.push_str("[package]\n");
        toml.push_str(&format!("name = \"{}\"\nversion = \"0.1.0\"\n", name));
        if !deps.is_empty() {
            toml.push_str("\n[dependencies]\n");
            for (n, v) in deps {
                toml.push_str(&format!("{n} = \"{v}\"\n"));
            }
        }
        fs::write(p.join(MANIFEST_TOML), toml).unwrap();
        p
    }

    #[cfg(feature = "toml")]
    #[test]
    fn detect_root_and_load() {
        let tmp = tempdir().unwrap();
        let root = tmp.path();
        let a = write_pkg(root, "a", &[]);
        let sub = a.join("sub");
        fs::create_dir_all(&sub).unwrap();
        // find upward
        let found = find_project_root(&sub).unwrap();
        assert_eq!(found, a);
        let m = load_manifest_at_root(&a).unwrap();
        assert_eq!(m.package.name, "a");
    }

    #[cfg(feature = "toml")]
    #[test]
    fn workspace_and_plan() {
        let tmp = tempdir().unwrap();
        let root = tmp.path();

        // workspace root
        let ws = root.join("ws");
        fs::create_dir_all(&ws).unwrap();
        fs::write(
            ws.join(MANIFEST_TOML),
            r#"[package]
name = "workspace"
version = "0.0.0"

[workspace]
members = ["a", "b"]
"#,
        )
        .unwrap();

        // members
        write_pkg(&ws, "a", &[]);
        write_pkg(&ws, "b", &[("a", "0.1")]);

        let projects = resolve_workspace(&ws).unwrap();
        assert_eq!(projects.len(), 2);
        let plan = build_plan(&projects).unwrap();
        // a doit précéder b
        let pos_a = plan.iter().position(|n| n == "a").unwrap();
        let pos_b = plan.iter().position(|n| n == "b").unwrap();
        assert!(pos_a < pos_b);
        assert!(has_internal_dep(&projects, "b", "a"));
    }

    #[cfg(all(feature = "json", not(feature = "toml")))]
    #[test]
    fn json_manifest_works() {
        let tmp = tempdir().unwrap();
        let root = tmp.path();
        fs::write(
            root.join(MANIFEST_JSON),
            r#"{"package":{"name":"p","version":"0.1.0"},"dependencies":{"x":"*"} }"#,
        )
        .unwrap();
        let m = load_manifest_at_root(root).unwrap();
        assert_eq!(m.package.name, "p");
        assert_eq!(m.deps()[0].name, "x");
    }
}