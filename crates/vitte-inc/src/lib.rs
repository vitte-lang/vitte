//! vitte-inc
//!
//! Gestion simple et fiable de l'incrémentalité de build pour Vitte.
//! Fournit :
//! - suivi des fichiers / artefacts et de leurs empreintes (fingerprints)
//! - graphe de dépendances léger
//! - calcul des nœuds "dirty" (à recompiler)
//! - snapshot (sérialisation) pour persistance entre runs
//!
//! Conception : pas d'IO bloquant caché. L'API expose les opérations pures
//! que l'appelant peut persister / exécuter dans son runtime habituel.

use serde::{Deserialize, Serialize};
use std::collections::{HashMap, HashSet};
use std::path::PathBuf;
use std::time::{SystemTime, UNIX_EPOCH};

/// Empreinte d'un fichier / artefact.
/// 64 bits suffisants pour checks rapides ; remplacer par algo cryptographique si besoin.
pub type Fingerprint = u64;

/// Erreurs simplifiées pour le crate.
#[derive(thiserror::Error, Debug)]
pub enum IncError {
    #[error("IO error: {0}")]
    Io(#[from] std::io::Error),

    #[error("serialization error: {0}")]
    Serde(#[from] bincode::Error),

    #[error("entity not found: {0}")]
    NotFound(String),
}

/// Représente un nœud du graphe (source ou artefact).
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct NodeMeta {
    /// chemin relatif ou identifiant logique
    pub id: String,

    /// empreinte connue lors du dernier snapshot
    pub fingerprint: Option<Fingerprint>,

    /// horodatage du dernier calcul (optionnel)
    pub last_built: Option<u64>,
}

impl NodeMeta {
    pub fn new(id: impl Into<String>) -> Self {
        Self {
            id: id.into(),
            fingerprint: None,
            last_built: None,
        }
    }
}

/// Snapshot complet du moteur d'incrémentalité.
/// Sérialisable pour persistance.
#[derive(Debug, Default, Serialize, Deserialize)]
pub struct IncSnapshot {
    pub nodes: HashMap<String, NodeMeta>,
    /// dépendances: key -> set of nodes it depends on (key needs those)
    pub deps: HashMap<String, HashSet<String>>,
    /// reverse deps: node -> set of nodes that depend on it (computed)
    #[serde(skip)]
    pub reverse_deps: HashMap<String, HashSet<String>>,
}

impl IncSnapshot {
    /// reconstruit les reverse_deps à partir de deps
    pub fn rebuild_reverse_deps(&mut self) {
        self.reverse_deps.clear();
        for (k, set) in &self.deps {
            for dep in set {
                self.reverse_deps
                    .entry(dep.clone())
                    .or_insert_with(HashSet::new)
                    .insert(k.clone());
            }
        }
    }

    /// ensure node exists in snapshot
    pub fn ensure_node(&mut self, id: impl Into<String>) -> &mut NodeMeta {
        let id = id.into();
        self.nodes
            .entry(id.clone())
            .or_insert_with(|| NodeMeta::new(id))
    }
}

/// Moteur d'incrémentalité basique.
pub struct IncManager {
    pub snapshot: IncSnapshot,
}

impl IncManager {
    /// Crée un manager vide.
    pub fn new() -> Self {
        Self {
            snapshot: IncSnapshot::default(),
        }
    }

    /// Charge un snapshot depuis des bytes (bincode).
    pub fn from_bytes(bytes: &[u8]) -> Result<Self, IncError> {
        let mut s: IncSnapshot = bincode::deserialize(bytes)?;
        s.rebuild_reverse_deps();
        Ok(Self { snapshot: s })
    }

    /// Sérialise le snapshot en bytes (bincode).
    pub fn to_bytes(&self) -> Result<Vec<u8>, IncError> {
        let mut serializable = self.snapshot.clone();
        // ne sérialise pas reverse_deps ; sera reconstruit au chargement
        serializable.reverse_deps = HashMap::new();
        Ok(bincode::serialize(&serializable)?)
    }

    /// Met à jour la fingerprint d'un nœud.
    pub fn set_fingerprint(&mut self, id: impl Into<String>, fp: Fingerprint) {
        let id = id.into();
        let node = self.snapshot.ensure_node(id.clone());
        node.fingerprint = Some(fp);
        node.last_built = Some(now_secs());
    }

    /// Ajoute/écrase les dépendances connues d'un nœud.
    pub fn set_deps<I, S>(&mut self, id: impl Into<String>, deps: I)
    where
        I: IntoIterator<Item = S>,
        S: Into<String>,
    {
        let id = id.into();
        let deps_set: HashSet<String> = deps.into_iter().map(|s| s.into()).collect();
        self.snapshot.deps.insert(id.clone(), deps_set);
        // garder nodes présents
        self.snapshot.ensure_node(id);
        for d in self.snapshot.deps.get(&id).unwrap().iter() {
            self.snapshot.ensure_node(d.clone());
        }
        self.snapshot.rebuild_reverse_deps();
    }

    /// Marque un nœud comme modifié : retourne l'ensemble initial des nœuds dirty.
    /// is_source indique si c'est un fichier source édité par l'utilisateur.
    pub fn mark_modified(&mut self, id: impl Into<String>) -> HashSet<String> {
        let id = id.into();
        self.snapshot.ensure_node(id.clone());
        // enlever fingerprint pour forcer rebuild
        if let Some(n) = self.snapshot.nodes.get_mut(&id) {
            n.fingerprint = None;
        }
        // recalcule dirty closure ascendante (reverse deps)
        self.dirty_closure(&[id])
    }

    /// calcule l'ensemble de nœuds 'dirty' à partir d'une liste de racines modifiées.
    /// Inclut les racines elles-mêmes.
    pub fn dirty_closure<I, S>(&self, roots: I) -> HashSet<String>
    where
        I: IntoIterator<Item = S>,
        S: Into<String>,
    {
        let mut out = HashSet::new();
        let mut stack: Vec<String> = roots.into_iter().map(|s| s.into()).collect();
        while let Some(cur) = stack.pop() {
            if !out.insert(cur.clone()) {
                continue;
            }
            if let Some(dependees) = self.snapshot.reverse_deps.get(&cur) {
                for p in dependees {
                    if !out.contains(p) {
                        stack.push(p.clone());
                    }
                }
            }
        }
        out
    }

    /// Calcule les nœuds nécessitant rebuild comparé à un map de fingerprints externes
    /// (par ex résultats d'un hash de fichier sur disque).
    /// Retourne (dirty_set, unchanged_set)
    pub fn compare_fingerprints(
        &self,
        external: &HashMap<String, Fingerprint>,
    ) -> (HashSet<String>, HashSet<String>) {
        let mut dirty = HashSet::new();
        let mut unchanged = HashSet::new();

        for (id, meta) in &self.snapshot.nodes {
            match (meta.fingerprint, external.get(id)) {
                (Some(old), Some(&new)) if old == new => {
                    unchanged.insert(id.clone());
                }
                _ => {
                    dirty.insert(id.clone());
                }
            }
        }

        (dirty, unchanged)
    }

    /// Donne un ordre topologique simple pour compilation des nœuds fournis.
    /// Si cycle détecté, retourne Err(IncError::NotFound("cycle")) pour signaler problème.
    pub fn topo_order(&self, targets: &HashSet<String>) -> Result<Vec<String>, IncError> {
        // Kahn's algorithm restreint au sous-graphe des targets.
        // construisons indegrees pour les nodes du sous-graphe
        let mut indeg: HashMap<String, usize> = HashMap::new();
        let mut adj: HashMap<String, HashSet<String>> = HashMap::new();

        // initialisation
        for t in targets {
            indeg.entry(t.clone()).or_insert(0);
            adj.entry(t.clone()).or_insert_with(HashSet::new);
        }

        // remplir arcs dépendance -> target
        for t in targets {
            if let Some(deps) = self.snapshot.deps.get(t) {
                for dep in deps {
                    if targets.contains(dep) {
                        // edge dep -> t
                        adj.entry(dep.clone()).or_insert_with(HashSet::new).insert(t.clone());
                        *indeg.entry(t.clone()).or_insert(0) += 1;
                        indeg.entry(dep.clone()).or_insert(0);
                    }
                }
            }
        }

        let mut queue: Vec<String> = indeg
            .iter()
            .filter_map(|(k, &v)| if v == 0 { Some(k.clone()) } else { None })
            .collect();

        let mut order = Vec::with_capacity(targets.len());

        while let Some(n) = queue.pop() {
            order.push(n.clone());
            if let Some(neis) = adj.get(&n) {
                for m in neis {
                    if let Some(e) = indeg.get_mut(m) {
                        *e -= 1;
                        if *e == 0 {
                            queue.push(m.clone());
                        }
                    }
                }
            }
        }

        if order.len() != targets.len() {
            return Err(IncError::NotFound("cycle detected in dependency subgraph".into()));
        }

        Ok(order)
    }
}

/// helper : timestamp seconds
fn now_secs() -> u64 {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map(|d| d.as_secs())
        .unwrap_or(0)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn snapshot_roundtrip() {
        let mut m = IncManager::new();
        m.set_fingerprint("a", 1);
        m.set_fingerprint("b", 2);
        m.set_deps("app", vec!["a", "b"]);
        let bytes = m.to_bytes().expect("serialize");
        let m2 = IncManager::from_bytes(&bytes).expect("deserialize");
        assert!(m2.snapshot.deps.contains_key("app"));
        assert!(m2.snapshot.nodes.contains_key("a"));
    }

    #[test]
    fn dirty_closure_propagates() {
        let mut m = IncManager::new();
        m.set_deps("mid", vec!["leaf"]);
        m.set_deps("top", vec!["mid"]);
        m.snapshot.rebuild_reverse_deps();
        let dirty = m.mark_modified("leaf");
        assert!(dirty.contains("leaf"));
        assert!(dirty.contains("mid"));
        assert!(dirty.contains("top"));
    }

    #[test]
    fn topo_order_ok() {
        let mut m = IncManager::new();
        m.set_deps("b", vec!["a"]);
        m.set_deps("c", vec!["b"]);
        m.snapshot.rebuild_reverse_deps();
        let targets: HashSet<String> = ["a", "b", "c"].iter().map(|s| s.to_string()).collect();
        let order = m.topo_order(&targets).expect("topo");
        // order must be a -> b -> c (or any valid topo)
        assert_eq!(order.len(), 3);
        assert!(order.iter().position(|x| x == "a").unwrap() < order.iter().position(|x| x == "b").unwrap());
    }
}
