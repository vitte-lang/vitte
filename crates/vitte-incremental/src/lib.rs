//! vitte-incremental — moteur d’incrémentalité et de cache pour Vitte
//!
//! Fournit un graphe de build dirigé acyclique (DAG) avec empreintes,
//! invalidation fine par dépendances, exécution topologique et cache.
//!
//! - Nœuds : sources (fichiers ou constantes), tâches (fonctions pures), phony.
//! - Empreintes : xxh3 rapides sur contenu + métadonnées (chemin, nom tâche, etc.).
//! - Dirty tracking : un nœud est reconstruit si son empreinte a changé ou si
//!   l’une de ses dépendances a changé.
//! - Cache : mémoire par défaut, sérialisable en option (`feature = "serde"`).
//!
//! API haut-niveau :
//! - `Engine::new()`
//! - `add_source_file`, `add_source_bytes`, `add_phony`, `add_task`
//! - `build(&[NodeId])` → construit les cibles demandées + dépendances
//! - `artifact(node)` pour lire la sortie d’un nœud (bytes)
//!
//! Hypothèses : les tâches sont **pures** vis-à-vis de leurs entrées. Les effets
//! (I/O disque) doivent être réalisés à partir de l’`Artifact` retourné et non
//! l’inverse, pour permettre l’incrémentalité.

#![forbid(unsafe_code)]
#![deny(missing_docs)]

use indexmap::{IndexMap, IndexSet};
use sha2::{Digest, Sha256};
use std::fmt;
use std::fs;
use std::path::{Path, PathBuf};
use std::time::SystemTime;
use thiserror::Error;
use xxhash_rust::xxh3::xxh3_64;

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

/// Erreurs possibles.
#[derive(Error, Debug)]
pub enum IncError {
    /// Nœud introuvable.
    #[error("node not found: {0}")]
    NotFound(String),
    /// Graphe cyclique détecté.
    #[error("cycle detected involving: {0}")]
    Cycle(String),
    /// Erreur d’I/O locale.
    #[error("io: {0}")]
    Io(#[from] std::io::Error),
    /// Erreur de tâche.
    #[error("task `{task}` failed: {error}")]
    TaskFailed { task: String, error: String },
    /// Entrées invalides.
    #[error("invalid: {0}")]
    Invalid(&'static str),
}

/// Résultat standard.
pub type IncResult<T> = Result<T, IncError>;

/// Identifiant stable d’un nœud dans l’engine.
#[derive(Copy, Clone, Debug, PartialEq, Eq, Hash)]
pub struct NodeId(usize);

impl fmt::Display for NodeId {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "#{}", self.0)
    }
}

/// Sortie produite par un nœud.
#[derive(Clone, Debug)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Artifact {
    /// Octets de sortie.
    pub bytes: Vec<u8>,
}

impl Artifact {
    /// Crée à partir de bytes.
    pub fn from_bytes(b: impl AsRef<[u8]>) -> Self {
        Self { bytes: b.as_ref().to_vec() }
    }

    /// Écrit atomiquement vers `path`.
    pub fn write_to(&self, path: impl AsRef<Path>) -> IncResult<()> {
        let path = path.as_ref();
        if let Some(p) = path.parent() {
            fs::create_dir_all(p)?;
        }
        let tmp = path.with_extension("tmp.vitte");
        fs::write(&tmp, &self.bytes)?;
        fs::rename(tmp, path)?;
        Ok(())
    }
}

/// Empreinte d’un nœud. Combine xxh3 (rapide) et sha256 (debug).
#[derive(Clone, Debug, PartialEq, Eq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Fingerprint {
    /// Hash rapide 64 bits (xxh3).
    pub fast: u64,
    /// SHA-256 pour vérif/diagnostic.
    pub sha256: [u8; 32],
}

impl Fingerprint {
    /// Empreinte vide.
    pub fn empty() -> Self {
        Self::of_bytes(&[])
    }
    /// Sur bytes.
    pub fn of_bytes(b: &[u8]) -> Self {
        let fast = xxh3_64(b);
        let sha = Sha256::digest(b);
        let mut sha256 = [0u8; 32];
        sha256.copy_from_slice(&sha);
        Self { fast, sha256 }
    }
    /// Sur concat de plusieurs empreintes.
    pub fn of_chain(parts: &[&Fingerprint]) -> Self {
        let mut fast = 0u64;
        let mut sha = Sha256::new();
        for p in parts {
            fast ^= p.fast.rotate_left(13).wrapping_add(0x9E37_79B9_7F4A_7C15);
            sha.update(&p.sha256);
        }
        let mut sha256 = [0u8; 32];
        sha256.copy_from_slice(&sha.finalize());
        Self { fast, sha256 }
    }
}

/// Signature d’une tâche pure. Entrées → sortie.
type TaskFn = dyn Send + Sync + Fn(&[Artifact]) -> IncResult<Artifact>;

/// Métadonnées d’un nœud.
#[derive(Clone, Debug)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum NodeKind {
    /// Source fichier lu sur disque.
    SourceFile { path: PathBuf, /// mtime dernière lecture
                 mtime: Option<SystemTime> },
    /// Source bytes constante gérée en mémoire.
    SourceBytes { tag: String, data: Vec<u8> },
    /// Tâche pure avec nom humain.
    Task { name: String },
    /// Nœud phony (agrégateur).
    Phony { name: String },
}

impl NodeKind {
    fn label(&self) -> String {
        match self {
            NodeKind::SourceFile { path, .. } => format!("file:{}", path.display()),
            NodeKind::SourceBytes { tag, .. } => format!("bytes:{tag}"),
            NodeKind::Task { name } => format!("task:{name}"),
            NodeKind::Phony { name } => format!("phony:{name}"),
        }
    }
}

/// Entrées d’un nœud (dépendances directes).
#[derive(Clone, Debug, Default)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Deps {
    /// Liste ordonnée des dépendances.
    pub inputs: Vec<NodeId>,
}

/// État runtime d’un nœud.
#[derive(Clone, Debug)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct NodeState {
    /// Empreinte de la dernière exécution/lecture.
    pub fp: Fingerprint,
    /// Sortie mise en cache (si applicable).
    pub artifact: Option<Artifact>,
    /// Marqueur dirty calculé lors d’un build.
    #[cfg_attr(feature = "serde", serde(skip))]
    pub dirty: bool,
}

impl Default for NodeState {
    fn default() -> Self {
        Self { fp: Fingerprint::empty(), artifact: None, dirty: true }
    }
}

/// Représentation d’un nœud.
pub struct Node {
    kind: NodeKind,
    deps: Deps,
    task: Option<Box<TaskFn>>,
    state: NodeState,
}

impl Node {
    fn new(kind: NodeKind) -> Self {
        Self { kind, deps: Deps::default(), task: None, state: NodeState::default() }
    }
}

/// Snapshot sérialisable du cache et des empreintes (feature `serde`).
#[cfg(feature = "serde")]
#[derive(Serialize, Deserialize, Default)]
pub struct Persist {
    nodes: Vec<(NodeKind, Deps, NodeState)>,
}

/// Moteur incrémental.
pub struct Engine {
    nodes: IndexMap<NodeId, Node>,
    name_to_id: IndexMap<String, NodeId>,
}

impl Default for Engine {
    fn default() -> Self {
        Self { nodes: IndexMap::new(), name_to_id: IndexMap::new() }
    }
}

impl Engine {
    /// Nouveau moteur.
    pub fn new() -> Self {
        Self::default()
    }

    // ------------------------
    // Création de nœuds
    // ------------------------

    /// Ajoute une source fichier. Relecture du contenu à chaque build si mtime change.
    pub fn add_source_file(&mut self, path: impl AsRef<Path>) -> NodeId {
        let path = path.as_ref().to_path_buf();
        let id = self.alloc(NodeKind::SourceFile { path: path.clone(), mtime: None });
        self.name_to_id.insert(format!("file:{}", path.display()), id);
        id
    }

    /// Ajoute une source constante en mémoire.
    pub fn add_source_bytes(&mut self, tag: impl Into<String>, data: Vec<u8>) -> NodeId {
        let tag = tag.into();
        let id = self.alloc(NodeKind::SourceBytes { tag: tag.clone(), data });
        self.name_to_id.insert(format!("bytes:{tag}"), id);
        id
    }

    /// Ajoute un nœud phony (agrégation de plusieurs dépendances).
    pub fn add_phony(&mut self, name: impl Into<String>, inputs: &[NodeId]) -> NodeId {
        let name = name.into();
        let mut n = Node::new(NodeKind::Phony { name: name.clone() });
        n.deps.inputs = inputs.to_vec();
        let id = self.insert(n);
        self.name_to_id.insert(format!("phony:{name}"), id);
        id
    }

    /// Ajoute une tâche pure. La fonction reçoit les artifacts des dépendances.
    pub fn add_task<F>(&mut self, name: impl Into<String>, inputs: &[NodeId], f: F) -> NodeId
    where
        F: Send + Sync + 'static + Fn(&[Artifact]) -> IncResult<Artifact>,
    {
        let name = name.into();
        let mut n = Node::new(NodeKind::Task { name: name.clone() });
        n.deps.inputs = inputs.to_vec();
        n.task = Some(Box::new(f));
        let id = self.insert(n);
        self.name_to_id.insert(format!("task:{name}"), id);
        id
    }

    /// Récupère un NodeId par label (`file:/...`, `task:name`, `phony:name`, `bytes:tag`).
    pub fn resolve(&self, label: &str) -> Option<NodeId> {
        self.name_to_id.get(label).copied()
    }

    /// Retourne l’Artifact d’un nœud si présent en cache.
    pub fn artifact(&self, id: NodeId) -> Option<&Artifact> {
        self.nodes.get(&id)?.state.artifact.as_ref()
    }

    /// Empreinte actuelle d’un nœud si connue.
    pub fn fingerprint(&self, id: NodeId) -> Option<&Fingerprint> {
        self.nodes.get(&id).map(|n| &n.state.fp)
    }

    // ------------------------
    // Build
    // ------------------------

    /// Construit les cibles demandées et leurs dépendances. Retourne la liste des nœuds
    /// reconstruits dans l’ordre d’exécution.
    pub fn build(&mut self, targets: &[NodeId]) -> IncResult<Vec<NodeId>> {
        // 1) tri topologique couvrant les cibles
        let order = self.toposort(targets)?;
        // 2) phase de lecture sources et calcul dirty
        let mut rebuilt = Vec::new();
        for id in order {
            let dirty = self.refresh_dirty(id)?;
            if dirty {
                self.execute(id)?;
                rebuilt.push(id);
            }
        }
        Ok(rebuilt)
    }

    /// Toposort des sous-graphes atteignables depuis `targets`.
    fn toposort(&self, targets: &[NodeId]) -> IncResult<Vec<NodeId>> {
        let mut visited = IndexSet::new();
        let mut temp = IndexSet::new();
        let mut out = Vec::new();

        fn dfs(
            g: &Engine,
            id: NodeId,
            visited: &mut IndexSet<NodeId>,
            temp: &mut IndexSet<NodeId>,
            out: &mut Vec<NodeId>,
        ) -> IncResult<()> {
            if visited.contains(&id) {
                return Ok(());
            }
            if !temp.insert(id) {
                return Err(IncError::Cycle(format!("{id}")));
            }
            let n = g.nodes.get(&id).ok_or_else(|| IncError::NotFound(id.to_string()))?;
            for &d in &n.deps.inputs {
                dfs(g, d, visited, temp, out)?;
            }
            temp.swap_remove(&id);
            visited.insert(id);
            out.push(id);
            Ok(())
        }

        for &t in targets {
            dfs(self, t, &mut visited, &mut temp, &mut out)?;
        }
        Ok(out)
    }

    /// Recalcule l’empreinte d’un nœud et décide s’il est dirty.
    fn refresh_dirty(&mut self, id: NodeId) -> IncResult<bool> {
        let (new_fp, label) = {
            let n = self.nodes.get(&id).ok_or_else(|| IncError::NotFound(id.to_string()))?;
            (self.compute_fingerprint(n)?, n.kind.label())
        };
        let n = self.nodes.get_mut(&id).unwrap();
        let deps_fp: Vec<&Fingerprint> = n
            .deps
            .inputs
            .iter()
            .map(|d| &self.nodes.get(d).unwrap().state.fp)
            .collect();
        let combined = Fingerprint::of_chain(std::iter::once(&new_fp).chain(deps_fp.into_iter()).collect::<Vec<_>>().as_slice());
        let was = n.state.fp.clone();
        let dirty = was != combined;
        n.state.fp = combined;
        n.state.dirty = dirty;
        log::debug!("fp[{label}] dirty={dirty}");
        Ok(dirty)
    }

    /// Exécute un nœud dirty.
    fn execute(&mut self, id: NodeId) -> IncResult<()> {
        let (kind, inputs, task_name) = {
            let n = self.nodes.get(&id).ok_or_else(|| IncError::NotFound(id.to_string()))?;
            let name = match &n.kind {
                NodeKind::Task { name } => Some(name.clone()),
                _ => None,
            };
            (n.kind.clone(), n.deps.inputs.clone(), name)
        };

        match kind {
            NodeKind::SourceFile { path, .. } => {
                let (bytes, mtime) = read_file_with_mtime(&path)?;
                let n = self.nodes.get_mut(&id).unwrap();
                n.kind = NodeKind::SourceFile { path, mtime: Some(mtime) };
                n.state.artifact = Some(Artifact::from_bytes(bytes));
            }
            NodeKind::SourceBytes { .. } => {
                // déjà en nœud, rien à recalculer : l’empreinte vient du contenu stocké
                // mais on réécrit l’artifact depuis le stockage
                let bytes = match &self.nodes.get(&id).unwrap().kind {
                    NodeKind::SourceBytes { data, .. } => data.clone(),
                    _ => unreachable!(),
                };
                self.nodes.get_mut(&id).unwrap().state.artifact = Some(Artifact::from_bytes(bytes));
            }
            NodeKind::Phony { .. } => {
                // phony ne produit rien
                self.nodes.get_mut(&id).unwrap().state.artifact = Some(Artifact::from_bytes(&[]));
            }
            NodeKind::Task { name } => {
                let func = self.nodes.get(&id).unwrap().task.as_ref().ok_or(IncError::Invalid("task without function"))?;
                let input_artifacts = inputs
                    .iter()
                    .map(|i| {
                        self.nodes
                            .get(i)
                            .and_then(|n| n.state.artifact.clone())
                            .ok_or_else(|| IncError::Invalid("missing input artifact"))
                    })
                    .collect::<IncResult<Vec<_>>>()?;
                let art = (func)(&input_artifacts).map_err(|e| IncError::TaskFailed {
                    task: name.clone(),
                    error: e.to_string(),
                })?;
                self.nodes.get_mut(&id).unwrap().state.artifact = Some(art);
            }
        }
        Ok(())
    }

    /// Calcule l’empreinte intrinsèque d’un nœud (sans dépendances).
    fn compute_fingerprint(&self, n: &Node) -> IncResult<Fingerprint> {
        Ok(match &n.kind {
            NodeKind::SourceFile { path, mtime } => {
                // combine chemin + mtime + contenu hash rapide de tête
                let meta = fs::metadata(path)?;
                let mt = meta.modified().ok().or(*mtime);
                let mut buf = Vec::new();
                buf.extend_from_slice(path.as_os_str().to_string_lossy().as_bytes());
                if let Some(t) = mt {
                    if let Ok(d) = t.duration_since(SystemTime::UNIX_EPOCH) {
                        buf.extend_from_slice(&d.as_secs().to_le_bytes());
                        buf.extend_from_slice(&d.subsec_nanos().to_le_bytes());
                    }
                } else {
                    buf.extend_from_slice(&0u64.to_le_bytes());
                }
                // contenu pièce jointe: pour éviter un read complet ici, on peut hasher taille+mtime.
                buf.extend_from_slice(&meta.len().to_le_bytes());
                Fingerprint::of_bytes(&buf)
            }
            NodeKind::SourceBytes { tag, data } => {
                let mut buf = Vec::new();
                buf.extend_from_slice(tag.as_bytes());
                buf.extend_from_slice(&xxh3_64(data).to_le_bytes());
                Fingerprint::of_bytes(&buf)
            }
            NodeKind::Task { name } => {
                // la pureté impose que seule la fonction + son nom fassent partie de l’empreinte
                // Ici on n’a pas de hash de code, on utilise le nom + arité d’entrée
                Fingerprint::of_bytes(name.as_bytes())
            }
            NodeKind::Phony { name } => Fingerprint::of_bytes(name.as_bytes()),
        })
    }

    // ------------------------
    // Persistance
    // ------------------------

    /// Exporte l’état (empreintes + outputs) au format JSON (feature `serde`).
    #[cfg(feature = "serde")]
    pub fn save_json(&self, path: impl AsRef<Path>) -> IncResult<()> {
        let mut v = Vec::new();
        for (_id, n) in &self.nodes {
            v.push((n.kind.clone(), n.deps.clone(), n.state.clone()));
        }
        let p = Persist { nodes: v };
        let s = serde_json::to_string_pretty(&p).unwrap();
        let tmp = path.as_ref().with_extension("tmp.vitte.json");
        fs::write(&tmp, s)?;
        fs::rename(tmp, path)?;
        Ok(())
    }

    /// Importe un état précédemment exporté. Les nœuds doivent être recréés
    /// dans le même ordre et avec les mêmes labels pour que les empreintes réappliquées
    /// aient du sens. Les outputs sont restaurés en mémoire.
    #[cfg(feature = "serde")]
    pub fn load_json(&mut self, path: impl AsRef<Path>) -> IncResult<()> {
        let s = fs::read_to_string(path)?;
        let p: Persist = serde_json::from_str(&s).unwrap();
        // Stratégie simple : réconcilier par label.
        let mut label_to_id = IndexMap::new();
        for (id, n) in &self.nodes {
            label_to_id.insert(n.kind.label(), *id);
        }
        for (kind, _deps, state) in p.nodes {
            if let Some(&id) = label_to_id.get(&kind.label()) {
                let n = self.nodes.get_mut(&id).unwrap();
                n.state = state;
            }
        }
        Ok(())
    }

    // ------------------------
    // Internes
    // ------------------------

    fn alloc(&mut self, kind: NodeKind) -> NodeId {
        let n = Node::new(kind);
        self.insert(n)
    }

    fn insert(&mut self, n: Node) -> NodeId {
        let id = NodeId(self.nodes.len());
        self.nodes.insert(id, n);
        id
    }
}

// ------------------------ Utilitaires ------------------------

fn read_file_with_mtime(path: &Path) -> IncResult<(Vec<u8>, SystemTime)> {
    let data = fs::read(path)?;
    let mt = fs::metadata(path)?.modified().unwrap_or(SystemTime::UNIX_EPOCH);
    Ok((data, mt))
}

// ------------------------ Tests ------------------------

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::Write;
    use tempfile::tempdir;

    #[test]
    fn source_bytes_and_task_incremental() {
        let mut e = Engine::new();
        let a = e.add_source_bytes("A", b"hello".to_vec());
        let b = e.add_task("uppercase", &[a], |ins| {
            let mut s = String::from_utf8(ins[0].bytes.clone()).unwrap();
            s.make_ascii_uppercase();
            Ok(Artifact::from_bytes(s.into_bytes()))
        });
        // first build
        let rebuilt = e.build(&[b]).unwrap();
        assert_eq!(rebuilt, vec![a, b]);
        assert_eq!(std::str::from_utf8(&e.artifact(b).unwrap().bytes).unwrap(), "HELLO");
        // second build (no change)
        let rebuilt = e.build(&[b]).unwrap();
        assert!(rebuilt.is_empty());
        // change source
        if let NodeKind::SourceBytes { data, .. } = &mut e.nodes.get_mut(&a).unwrap().kind {
            *data = b"world".to_vec();
        }
        let rebuilt = e.build(&[b]).unwrap();
        assert_eq!(rebuilt, vec![a, b]);
        assert_eq!(std::str::from_utf8(&e.artifact(b).unwrap().bytes).unwrap(), "WORLD");
    }

    #[test]
    fn file_source_dirty_on_write() {
        let dir = tempdir().unwrap();
        let p = dir.path().join("x.txt");
        fs::write(&p, "foo").unwrap();

        let mut e = Engine::new();
        let s = e.add_source_file(&p);
        let t = e.add_task("dup", &[s], |ins| {
            let mut v = ins[0].bytes.clone();
            v.extend_from_slice(&ins[0].bytes);
            Ok(Artifact::from_bytes(v))
        });

        let first = e.build(&[t]).unwrap();
        assert_eq!(first, vec![s, t]);
        assert_eq!(e.artifact(t).unwrap().bytes, b"foofoo");

        // no change
        let second = e.build(&[t]).unwrap();
        assert!(second.is_empty());

        // modify file
        {
            let mut f = fs::OpenOptions::new().write(true).open(&p).unwrap();
            f.set_len(0).unwrap();
            write!(f, "bar").unwrap();
        }

        let third = e.build(&[t]).unwrap();
        assert_eq!(third, vec![s, t]);
        assert_eq!(e.artifact(t).unwrap().bytes, b"barbar");
    }

    #[test]
    fn phony_aggregates() {
        let mut e = Engine::new();
        let a = e.add_source_bytes("A", b"a".to_vec());
        let b = e.add_source_bytes("B", b"b".to_vec());
        let p = e.add_phony("all", &[a, b]);
        let built = e.build(&[p]).unwrap();
        // phony has no artifact content, but is considered rebuilt once
        assert_eq!(built, vec![a, b, p]);
    }

    #[test]
    fn cycle_detected() {
        // Build a -> b -> a
        let mut e = Engine::new();
        let a = e.add_source_bytes("A", b"a".to_vec());
        let b = e.add_task("id", &[a], |ins| Ok(ins[0].clone()));
        // create fake cycle by editing deps directly (for test)
        e.nodes.get_mut(&NodeId(b.0)).unwrap().deps.inputs = vec![NodeId(a.0)];
        // now inject back-cycle
        e.nodes.get_mut(&NodeId(a.0)).unwrap().deps.inputs.push(NodeId(b.0));
        let err = e.build(&[NodeId(b.0)]).unwrap_err();
        match err {
            IncError::Cycle(_) => {}
            _ => panic!("expected cycle"),
        }
    }
}