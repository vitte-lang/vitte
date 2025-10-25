// vitte-inc/src/snapshot.rs
//! Snapshot ultra complet du graphe d’incrémentalité.
//!
//! Contenu :
//! - Métadonnées nœud (fingerprint, last_built)
//! - Graphe orienté deps (u -> v si v dépend de u)
//! - Reverse automatique, vérifications d’invariants
//! - Outils de manipulation (add/remove/set deps), vues (succ/pred), portée (reachability)
//! - Détection de cycles et tri topologique restreint
//! - Versionnement sérialisable

use serde::{Deserialize, Serialize};
use std::collections::{HashMap, HashSet};

/// Empreinte légère (remplaçable par algo cryptographique si besoin).
pub type Fingerprint = u64;

/// Version de format sérialisé.
pub const SNAPSHOT_VERSION: u32 = 1;

/// Métadonnées d’un nœud.
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct NodeMeta {
    /// Identifiant logique stable (ex: chemin relatif).
    pub id: String,
    /// Empreinte connue au dernier build.
    pub fingerprint: Option<Fingerprint>,
    /// Horodatage de dernier build (secs).
    pub last_built: Option<u64>,
}

impl NodeMeta {
    #[inline]
    pub fn new(id: impl Into<String>) -> Self {
        Self { id: id.into(), fingerprint: None, last_built: None }
    }
}

/// Snapshot complet du graphe.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct IncSnapshot {
    /// Version de format. Défaut = `SNAPSHOT_VERSION`.
    #[serde(default = "default_snapshot_version")]
    pub version: u32,

    /// Nœuds connus (id -> meta).
    pub nodes: HashMap<String, NodeMeta>,

    /// Dépendances directes : `k` dépend de chaque élément de `deps[k]`.
    /// Arcs: dep -> k
    pub deps: HashMap<String, HashSet<String>>,

    /// Index inverse calculé (non sérialisé) : `x` est prédécesseur de tous `reverse[x]`.
    #[serde(skip)]
    pub reverse: HashMap<String, HashSet<String>>,

    /// Compteur d’incréments (stat facultative).
    #[serde(default)]
    pub build_epoch: u64,
}

#[inline]
fn default_snapshot_version() -> u32 { SNAPSHOT_VERSION }

impl Default for IncSnapshot {
    fn default() -> Self {
        Self {
            version: SNAPSHOT_VERSION,
            nodes: HashMap::new(),
            deps: HashMap::new(),
            reverse: HashMap::new(),
            build_epoch: 0,
        }
    }
}

impl IncSnapshot {
    // --- Construction / reset ---

    #[inline]
    pub fn new() -> Self { Self::default() }

    #[inline]
    pub fn clear(&mut self) {
        self.nodes.clear();
        self.deps.clear();
        self.reverse.clear();
        self.build_epoch = 0;
    }

    // --- Nœuds ---

    /// Garantit l’existence d’un nœud et retourne une référence mutable.
    pub fn ensure_node(&mut self, id: impl Into<String>) -> &mut NodeMeta {
        let id = id.into();
        self.nodes.entry(id.clone()).or_insert_with(|| NodeMeta::new(id))
    }

    #[inline]
    pub fn contains(&self, id: &str) -> bool { self.nodes.contains_key(id) }

    /// Supprime un nœud et toute arête incidente.
    pub fn remove_node(&mut self, id: &str) {
        self.nodes.remove(id);
        self.deps.remove(id);
        for set in self.deps.values_mut() { set.remove(id); }
        self.reverse.remove(id);
        for set in self.reverse.values_mut() { set.remove(id); }
    }

    // --- Dépendances ---

    /// Remplace l’ensemble des dépendances de `target` par `deps`.
    pub fn set_deps<I, S>(&mut self, target: impl Into<String>, deps: I)
    where
        I: IntoIterator<Item = S>,
        S: Into<String>,
    {
        let target = target.into();
        let set: HashSet<String> = deps.into_iter().map(|s| s.into()).collect();
        self.ensure_node(target.clone());
        for d in &set { self.ensure_node(d.clone()); }
        self.deps.insert(target, set);
        self.rebuild_reverse();
    }

    /// Ajoute une dépendance `dep -> target`.
    pub fn add_dep(&mut self, target: impl Into<String>, dep: impl Into<String>) {
        let target = target.into();
        let dep = dep.into();
        self.ensure_node(target.clone());
        self.ensure_node(dep.clone());
        self.deps.entry(target.clone()).or_default().insert(dep.clone());
        self.reverse.entry(dep).or_default().insert(target);
    }

    /// Retire une dépendance `dep -> target`.
    pub fn remove_dep(&mut self, target: &str, dep: &str) {
        if let Some(s) = self.deps.get_mut(target) { s.remove(dep); }
        if let Some(s) = self.reverse.get_mut(dep) { s.remove(target); }
    }

    /// Recalcule l’index inverse à partir de `deps`.
    pub fn rebuild_reverse(&mut self) {
        self.reverse.clear();
        for (t, ds) in &self.deps {
            for d in ds {
                self.reverse.entry(d.clone()).or_default().insert(t.clone());
            }
        }
    }

    // --- Requêtes graphe ---

    /// Successeurs immédiats (nœuds qui dépendent de `id`).
    #[inline]
    pub fn succ<'a>(&'a self, id: &str) -> impl Iterator<Item = &'a str> + 'a {
        self.reverse.get(id).into_iter().flat_map(|s| s.iter().map(String::as_str))
    }

    /// Prédécesseurs immédiats (les deps directes de `id`).
    #[inline]
    pub fn pred<'a>(&'a self, id: &str) -> impl Iterator<Item = &'a str> + 'a {
        self.deps.get(id).into_iter().flat_map(|s| s.iter().map(String::as_str))
    }

    #[inline]
    pub fn out_degree(&self, id: &str) -> usize { self.deps.get(id).map(|s| s.len()).unwrap_or(0) }

    #[inline]
    pub fn in_degree(&self, id: &str) -> usize { self.reverse.get(id).map(|s| s.len()).unwrap_or(0) }

    /// Fermeture ascendante depuis `roots` (tout ce qui dépend d’eux).
    pub fn affected_by<I, S>(&self, roots: I) -> HashSet<String>
    where
        I: IntoIterator<Item = S>,
        S: Into<String>,
    {
        let mut out = HashSet::new();
        let mut stack: Vec<String> = roots.into_iter().map(|s| s.into()).collect();
        while let Some(cur) = stack.pop() {
            if !out.insert(cur.clone()) { continue; }
            if let Some(succs) = self.reverse.get(&cur) {
                for n in succs {
                    if !out.contains(n) { stack.push(n.clone()); }
                }
            }
        }
        out
    }

    /// Fermeture descendante depuis `roots` (toutes leurs dépendances).
    pub fn reachable_deps<I, S>(&self, roots: I) -> HashSet<String>
    where
        I: IntoIterator<Item = S>,
        S: Into<String>,
    {
        let mut out = HashSet::new();
        let mut stack: Vec<String> = roots.into_iter().map(|s| s.into()).collect();
        while let Some(cur) = stack.pop() {
            if !out.insert(cur.clone()) { continue; }
            if let Some(preds) = self.deps.get(&cur) {
                for n in preds {
                    if !out.contains(n) { stack.push(n.clone()); }
                }
            }
        }
        out
    }

    /// Sous-graphe induit restreint à `keep`.
    pub fn induced(&self, keep: &HashSet<String>) -> (HashMap<String, HashSet<String>>, usize) {
        let mut out: HashMap<String, HashSet<String>> = HashMap::new();
        let mut edges = 0usize;
        for k in keep {
            if let Some(ds) = self.deps.get(k) {
                let filtered: HashSet<String> = ds.iter().filter(|d| keep.contains(&***d)).cloned().collect();
                edges += filtered.len();
                if !filtered.is_empty() { out.insert(k.clone(), filtered); }
            }
        }
        (out, edges)
        // note: noeuds isolés non insérés, ce qui est OK pour topo local.
    }

    // --- Cycles / topologie ---

    /// Détecte un cycle dans le sous-graphe `keep`. Retourne vrai si cycle.
    pub fn has_cycle_in(&self, keep: &HashSet<String>) -> bool {
        self.topo_order_in(keep).is_none()
    }

    /// Tri topologique dans `keep`. None si cycle.
    pub fn topo_order_in(&self, keep: &HashSet<String>) -> Option<Vec<String>> {
        let (adj, _) = self.induced(keep);
        // indegrees
        let mut indeg: HashMap<String, usize> = HashMap::new();
        for k in keep {
            indeg.entry(k.clone()).or_insert(0);
        }
        for (u, vs) in &adj {
            for v in vs { *indeg.entry(v.clone()).or_insert(0) += 1; }
        }

        let mut q: Vec<String> = indeg.iter().filter_map(|(k, &d)| if d == 0 { Some(k.clone()) } else { None }).collect();
        let mut out = Vec::with_capacity(keep.len());
        while let Some(n) = q.pop() {
            out.push(n.clone());
            if let Some(nexts) = adj.get(&n) {
                for m in nexts {
                    if let Some(d) = indeg.get_mut(m) {
                        *d -= 1;
                        if *d == 0 { q.push(m.clone()); }
                    }
                }
            }
        }
        if out.len() == keep.len() { Some(out) } else { None }
    }

    // --- Nettoyage / maintenance ---

    /// Supprime les nœuds orphelins sans fingerprint.
    pub fn gc_orphans(&mut self) {
        self.rebuild_reverse();
        let mut remove = Vec::new();
        for (id, meta) in &self.nodes {
            let has_in = self.reverse.get(id).map(|s| !s.is_empty()).unwrap_or(false);
            let has_out = self.deps.get(id).map(|s| !s.is_empty()).unwrap_or(false);
            if !has_in && !has_out && meta.fingerprint.is_none() {
                remove.push(id.clone());
            }
        }
        for id in remove {
            self.remove_node(&id);
        }
    }

    /// Vérifie quelques invariants légers. Retourne nombre d’arêtes.
    pub fn validate_light(&self) -> usize {
        // Tous les sommets référencés existent.
        for (t, ds) in &self.deps {
            assert!(self.nodes.contains_key(t), "unknown target {t}");
            for d in ds {
                assert!(self.nodes.contains_key(d), "unknown dependency {d}");
            }
        }
        // reverse cohérent (si rempli).
        if !self.reverse.is_empty() {
            for (d, ts) in &self.reverse {
                for t in ts {
                    assert!(self.deps.get(t).map_or(false, |s| s.contains(d)),
                        "reverse edge mismatch {d} -> {t}");
                }
            }
        }
        self.deps.values().map(|s| s.len()).sum()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn ensure_and_set_deps_builds_reverse() {
        let mut s = IncSnapshot::new();
        s.set_deps("top", ["mid"]);
        s.set_deps("mid", ["leaf"]);
        assert!(s.reverse.get("mid").unwrap().contains("top"));
        assert!(s.reverse.get("leaf").unwrap().contains("mid"));
    }

    #[test]
    fn add_remove_dep_ok() {
        let mut s = IncSnapshot::new();
        s.set_deps("a", []);
        s.add_dep("a", "b");
        assert!(s.deps.get("a").unwrap().contains("b"));
        assert!(s.reverse.get("b").unwrap().contains("a"));
        s.remove_dep("a", "b");
        assert!(!s.deps.get("a").unwrap().contains("b"));
    }

    #[test]
    fn closures_work() {
        let mut s = IncSnapshot::new();
        s.set_deps("top", ["mid"]);
        s.set_deps("mid", ["leaf"]);
        let up = s.affected_by(["leaf"]);
        assert!(up.contains("mid") && up.contains("top"));
        let down = s.reachable_deps(["top"]);
        assert!(down.contains("mid") && down.contains("leaf"));
    }

    #[test]
    fn topo_and_cycle() {
        let mut s = IncSnapshot::new();
        s.set_deps("c", ["b"]);
        s.set_deps("b", ["a"]);
        let keep: HashSet<String> = ["a","b","c"].into_iter().map(|x| x.to_string()).collect();
        let topo = s.topo_order_in(&keep).unwrap();
        assert_eq!(topo.len(), 3);

        s.set_deps("a", ["c"]); // crée un cycle
        assert!(s.has_cycle_in(&keep));
        assert!(s.topo_order_in(&keep).is_none());
    }

    #[test]
    fn gc_orphans_removes_isolated_without_fp() {
        let mut s = IncSnapshot::new();
        s.ensure_node("x");
        s.gc_orphans();
        assert!(!s.contains("x"));
    }

    #[test]
    fn version_defaults() {
        let s = IncSnapshot::default();
        assert_eq!(s.version, SNAPSHOT_VERSION);
    }
}
