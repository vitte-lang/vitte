// vitte-inc/src/engine.rs
//! Moteur d’incrémentalité : calcul des deltas, ordre de build, et persistance.

use crate::{IncError, IncSnapshot, Fingerprint};
use std::collections::{HashMap, HashSet};
use std::time::{SystemTime, UNIX_EPOCH};

/// Plan de compilation minimal.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct BuildPlan {
    /// Ensemble des nœuds à (re)construire.
    pub dirty: HashSet<String>,
    /// Ordre topologique restreint à `dirty`.
    pub order: Vec<String>,
}

impl BuildPlan {
    pub fn is_empty(&self) -> bool { self.dirty.is_empty() }
}

/// Moteur principal d’incrémentalité.
pub struct IncManager {
    pub snapshot: IncSnapshot,
}

impl IncManager {
    /// Crée un manager vide.
    pub fn new() -> Self {
        Self { snapshot: IncSnapshot::default() }
    }

    /// Charge un snapshot (bincode).
    pub fn from_bytes(bytes: &[u8]) -> Result<Self, IncError> {
        let mut s: IncSnapshot = bincode::deserialize(bytes)?;
        s.rebuild_reverse();
        Ok(Self { snapshot: s })
    }

    /// Sauvegarde le snapshot (bincode).
    pub fn to_bytes(&self) -> Result<Vec<u8>, IncError> {
        // reverse est reconstruit au chargement, on l’efface avant sérialisation
        let mut s = self.snapshot.clone();
        s.reverse = HashMap::new();
        Ok(bincode::serialize(&s)?)
    }

    /// Met à jour la fingerprint d’un nœud et son timestamp.
    pub fn set_fingerprint(&mut self, id: impl Into<String>, fp: Fingerprint) {
        let id = id.into();
        let node = self.snapshot.ensure_node(id);
        node.fingerprint = Some(fp);
        node.last_built = Some(now_secs());
    }

    /// Définit les dépendances directes d’un nœud.
    pub fn set_deps<I, S>(&mut self, id: impl Into<String>, deps: I)
    where
        I: IntoIterator<Item = S>,
        S: Into<String>,
    {
        let id = id.into();
        let set: HashSet<String> = deps.into_iter().map(|s| s.into()).collect();
        self.snapshot.deps.insert(id.clone(), set);
        self.snapshot.ensure_node(id);
        // s’assure que tous les deps existent
        if let Some(ds) = self.snapshot.deps.get(id.as_str()) {
            for d in ds { self.snapshot.ensure_node(d.clone()); }
        }
        self.snapshot.rebuild_reverse();
    }

    /// Marque un nœud comme modifié par l’utilisateur.
    /// Enlève sa fingerprint et retourne la fermeture dirty ascendante.
    pub fn mark_modified(&mut self, id: impl Into<String>) -> HashSet<String> {
        let id = id.into();
        self.snapshot.ensure_node(id.clone());
        if let Some(n) = self.snapshot.nodes.get_mut(&id) {
            n.fingerprint = None;
        }
        self.dirty_closure([id])
    }

    /// Calcule la fermeture dirty ascendante depuis un ensemble de racines.
    pub fn dirty_closure<I, S>(&self, roots: I) -> HashSet<String>
    where
        I: IntoIterator<Item = S>,
        S: Into<String>,
    {
        let mut out = HashSet::new();
        let mut stack: Vec<String> = roots.into_iter().map(|s| s.into()).collect();
        while let Some(cur) = stack.pop() {
            if !out.insert(cur.clone()) { continue; }
            if let Some(dependees) = self.snapshot.reverse.get(&cur) {
                for p in dependees {
                    if !out.contains(p) {
                        stack.push(p.clone());
                    }
                }
            }
        }
        out
    }

    /// Compare fingerprints internes vs externes (hash disque).
    /// Retourne (dirty, unchanged).
    pub fn compare_fingerprints(
        &self,
        external: &HashMap<String, Fingerprint>,
    ) -> (HashSet<String>, HashSet<String>) {
        let mut dirty = HashSet::new();
        let mut ok = HashSet::new();
        for (id, meta) in &self.snapshot.nodes {
            match (meta.fingerprint, external.get(id)) {
                (Some(a), Some(&b)) if a == b => { ok.insert(id.clone()); }
                _ => { dirty.insert(id.clone()); }
            }
        }
        (dirty, ok)
    }

    /// Ordre topologique restreint à `targets`. Erreur si cycle.
    pub fn topo_order(&self, targets: &HashSet<String>) -> Result<Vec<String>, IncError> {
        // indeg et adj du sous-graphe
        let mut indeg: HashMap<String, usize> = HashMap::new();
        let mut adj: HashMap<String, HashSet<String>> = HashMap::new();

        for t in targets {
            indeg.entry(t.clone()).or_insert(0);
            if let Some(deps) = self.snapshot.deps.get(t) {
                for d in deps {
                    if targets.contains(d) {
                        *indeg.entry(t.clone()).or_insert(0) += 1;
                        adj.entry(d.clone()).or_default().insert(t.clone());
                    }
                }
            }
        }

        let mut q: Vec<String> = indeg
            .iter()
            .filter_map(|(k, &v)| if v == 0 { Some(k.clone()) } else { None })
            .collect();
        let mut out = Vec::with_capacity(targets.len());

        while let Some(n) = q.pop() {
            out.push(n.clone());
            if let Some(nexts) = adj.get(&n) {
                for m in nexts {
                    if let Some(e) = indeg.get_mut(m) {
                        *e -= 1;
                        if *e == 0 { q.push(m.clone()); }
                    }
                }
            }
        }

        if out.len() != targets.len() {
            return Err(IncError::NotFound("cycle detected in dependency subgraph".into()));
        }
        Ok(out)
    }

    /// Construit un plan à partir d’un ensemble de racines modifiées et d’empreintes externes.
    /// 1) calc dirty par fingerprints, 2) ajoute fermeture ascendante des racines,
    /// 3) topological sort.
    pub fn plan_from(
        &self,
        modified_roots: impl IntoIterator<Item = String>,
        external: &HashMap<String, Fingerprint>,
    ) -> Result<BuildPlan, IncError> {
        let (by_fp_dirty, _ok) = self.compare_fingerprints(external);
        let roots_dirty = self.dirty_closure(modified_roots);
        let dirty: HashSet<String> = by_fp_dirty.union(&roots_dirty).cloned().collect();
        let order = if dirty.is_empty() { Vec::new() } else { self.topo_order(&dirty)? };
        Ok(BuildPlan { dirty, order })
    }

    /// Applique les résultats de build : met à jour fingerprints et timestamps.
    pub fn apply_build_results<I>(&mut self, results: I)
    where
        I: IntoIterator<Item = (String, Fingerprint)>,
    {
        for (id, fp) in results {
            self.set_fingerprint(id, fp);
        }
    }

    /// Nettoie les nœuds orphelins qui n’ont plus d’arcs et pas de fingerprint.
    pub fn gc_orphans(&mut self) {
        // recompute reverse
        self.snapshot.rebuild_reverse();
        let mut to_remove = Vec::new();
        for (id, meta) in &self.snapshot.nodes {
            let has_in = self.snapshot.reverse.get(id).map(|s| !s.is_empty()).unwrap_or(false);
            let has_out = self.snapshot.deps.get(id).map(|s| !s.is_empty()).unwrap_or(false);
            if !has_in && !has_out && meta.fingerprint.is_none() {
                to_remove.push(id.clone());
            }
        }
        for id in to_remove {
            self.snapshot.nodes.remove(&id);
            self.snapshot.deps.remove(&id);
            self.snapshot.reverse.remove(&id);
        }
    }
}

/// Timestamp seconds.
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
    fn plan_with_roots_and_fingerprints() {
        let mut m = IncManager::new();
        m.set_deps("mid", ["leaf"]);
        m.set_deps("top", ["mid"]);
        // aucune fingerprint -> tout dirty
        let ext: HashMap<String, Fingerprint> = HashMap::new();
        let plan = m.plan_from(vec!["leaf".into()], &ext).unwrap();
        assert_eq!(plan.order.len(), 3);
        assert!(plan.dirty.contains("top"));
    }

    #[test]
    fn topo_cycle_error() {
        let mut m = IncManager::new();
        m.set_deps("a", ["b"]);
        m.set_deps("b", ["a"]);
        let targets: HashSet<String> = ["a","b"].into_iter().map(|s| s.to_string()).collect();
        let err = m.topo_order(&targets).unwrap_err();
        matches!(err, IncError::NotFound(_));
    }
}
