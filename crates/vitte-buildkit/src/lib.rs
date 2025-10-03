//! vitte-buildkit — toolkit de build (tâches, pipelines, packaging léger)
//!
//! Points clés :
//! - DAG de tâches avec dépendances, exécution parallèle par "vagues"
//! - Cache incrémental par empreinte (hash) d’entrées et paramètres
//! - Primitives prêtes à l’emploi : Shell, Copy, Mkdir, Clean, Hash, Touch
//! - Découverte de fichiers (glob simple), hachage fichiers, suivi d’artifacts
//! - Spécification TOML (recipes) → pipeline exécutable
//! - Logs via `log` + `env_logger`
//!
//! Ce crate n’utilise pas de dépendances système hors Rust std.

#![forbid(unsafe_code)]

use rayon::prelude::*;
use std::collections::{HashMap, HashSet, VecDeque};
use std::ffi::OsStr;
use std::fs::{self, File};
use std::hash::{Hash, Hasher};
use std::io::{Read, Write};
use std::path::{Path, PathBuf};
use std::process::Command;
use walkdir::WalkDir;

// ============================================================================
// Erreurs
// ============================================================================

#[derive(Debug)]
pub enum BuildError {
    /// I/O error
    Io(std::io::Error),
    /// TOML deserialize error
    TomlDe(toml::de::Error),
    /// TOML serialize error
    TomlSer(toml::ser::Error),
    /// JSON error
    Json(serde_json::Error),
    /// A command failed with an optional exit code
    CommandFailed { cmd: String, code: Option<i32> },
    /// Unknown task name
    UnknownTask(String),
    /// Cycle detected in task graph
    CyclicGraph,
    /// No tasks to run
    NoTasks,
    /// A task with the same name already exists
    TaskConflict(String),
    /// Aborted due to upstream error
    Aborted,
}

impl core::fmt::Display for BuildError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            BuildError::Io(e) => write!(f, "I/O: {e}"),
            BuildError::TomlDe(e) => write!(f, "TOML: {e}"),
            BuildError::TomlSer(e) => write!(f, "TOML: {e}"),
            BuildError::Json(e) => write!(f, "JSON: {e}"),
            BuildError::CommandFailed { cmd, code } => write!(f, "Commande échouée: {cmd} (code: {code:?})"),
            BuildError::UnknownTask(s) => write!(f, "Tâche inconnue: {s}"),
            BuildError::CyclicGraph => write!(f, "Cycle détecté dans le graphe de tâches"),
            BuildError::NoTasks => write!(f, "Aucune tâche à exécuter"),
            BuildError::TaskConflict(s) => write!(f, "Conflit de tâche: {s}"),
            BuildError::Aborted => write!(f, "Interrompu par une erreur en amont"),
        }
    }
}

impl std::error::Error for BuildError {}

// Convenience From impls to replace #[from]
impl From<std::io::Error> for BuildError {
    fn from(e: std::io::Error) -> Self { BuildError::Io(e) }
}
impl From<toml::de::Error> for BuildError {
    fn from(e: toml::de::Error) -> Self { BuildError::TomlDe(e) }
}
impl From<toml::ser::Error> for BuildError {
    fn from(e: toml::ser::Error) -> Self { BuildError::TomlSer(e) }
}
impl From<serde_json::Error> for BuildError {
    fn from(e: serde_json::Error) -> Self { BuildError::Json(e) }
}

pub type Result<T> = std::result::Result<T, BuildError>;

// ============================================================================
// Contexte, cache et hachage
// ============================================================================

#[derive(Debug, Clone)]
pub struct BuildContext {
    pub root: PathBuf,
    pub out_dir: PathBuf,
    pub cache_dir: PathBuf,
    pub env: HashMap<String, String>,
    pub parallel: usize,
}

impl BuildContext {
    pub fn new(root: impl Into<PathBuf>) -> Self {
        let root = root.into();
        let out_dir = root.join("target").join("build");
        let cache_dir = root.join(".vitte").join("buildkit");
        Self {
            root,
            out_dir,
            cache_dir,
            env: std::env::vars().collect(),
            parallel: std::thread::available_parallelism()
                .map(|n| n.get())
                .unwrap_or(4),
        }
    }

    pub fn ensure_dirs(&self) -> Result<()> {
        fs::create_dir_all(&self.out_dir)?;
        fs::create_dir_all(&self.cache_dir)?;
        Ok(())
    }
}

#[derive(Debug, Clone)]
struct TaskCacheEntry {
    hash: u64,
    outputs: Vec<PathBuf>,
    time: u64,
}

#[derive(Default, Debug, Clone)]
struct CacheDb {
    tasks: HashMap<String, TaskCacheEntry>,
}

impl CacheDb {
    fn load(p: &Path) -> Self {
        if let Ok(s) = fs::read_to_string(p) {
            if let Ok(v) = serde_json::from_str::<serde_json::Value>(&s) {
                let mut db = CacheDb { tasks: HashMap::new() };
                if let Some(obj) = v.get("tasks").and_then(|t| t.as_object()) {
                    for (k, vv) in obj {
                        let hash = vv.get("hash").and_then(|x| x.as_u64()).unwrap_or(0);
                        let time = vv.get("time").and_then(|x| x.as_u64()).unwrap_or(0);
                        let outputs = vv.get("outputs")
                            .and_then(|x| x.as_array())
                            .map(|arr| {
                                arr.iter().filter_map(|e| e.as_str().map(PathBuf::from)).collect::<Vec<_>>()
                            })
                            .unwrap_or_default();
                        db.tasks.insert(k.clone(), TaskCacheEntry { hash, outputs, time });
                    }
                }
                return db;
            }
        }
        Self::default()
    }
    fn save(&self, p: &Path) -> Result<()> {
        if let Some(parent) = p.parent() {
            fs::create_dir_all(parent)?;
        }
        let mut tasks = serde_json::Map::new();
        for (k, v) in &self.tasks {
            let outs = serde_json::Value::Array(
                v.outputs.iter().map(|o| serde_json::Value::String(o.to_string_lossy().into_owned())).collect()
            );
            let mut obj = serde_json::Map::new();
            obj.insert("hash".into(), serde_json::Value::Number((v.hash as u64).into()));
            obj.insert("time".into(), serde_json::Value::Number((v.time as u64).into()));
            obj.insert("outputs".into(), outs);
            tasks.insert(k.clone(), serde_json::Value::Object(obj));
        }
        let mut root = serde_json::Map::new();
        root.insert("tasks".into(), serde_json::Value::Object(tasks));
        let s = serde_json::Value::Object(root).to_string();
        fs::write(p, s)?;
        Ok(())
    }
}

fn now_unix_secs() -> u64 {
    use std::time::{SystemTime, UNIX_EPOCH};
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map(|d| d.as_secs())
        .unwrap_or(0)
}

fn file_fingerprint(path: &Path, hasher: &mut impl Hasher) {
    // Empreinte simple et déterministe. Non cryptographique.
    if let Ok(md) = fs::metadata(path) {
        md.len().hash(hasher);
        if let Ok(mtime) = md.modified() {
            if let Ok(d) = mtime.duration_since(std::time::UNIX_EPOCH) {
                d.as_secs().hash(hasher);
                d.subsec_nanos().hash(hasher);
            }
        }
        // On lit au plus 64 KiB pour limiter le coût
        if let Ok(mut f) = File::open(path) {
            let mut buf = [0u8; 65536];
            if let Ok(n) = f.read(&mut buf) {
                (&buf[..n]).hash(hasher);
            }
        }
    }
}

fn paths_fingerprint(paths: &[PathBuf]) -> u64 {
    let mut h = std::collections::hash_map::DefaultHasher::new();
    for p in paths {
        p.to_string_lossy().hash(&mut h);
        file_fingerprint(p, &mut h);
    }
    h.finish()
}

fn strings_fingerprint(ss: &[String]) -> u64 {
    let mut h = std::collections::hash_map::DefaultHasher::new();
    for s in ss {
        s.hash(&mut h);
    }
    h.finish()
}

// ============================================================================
// Tâches
// ============================================================================

pub trait Task: Send + Sync {
    fn name(&self) -> &str;
    fn deps(&self) -> &[String];
    fn inputs(&self) -> Vec<PathBuf> { vec![] }
    fn outputs(&self) -> Vec<PathBuf> { vec![] }
    fn params_fingerprint(&self) -> u64 { 0 }
    fn run(&self, ctx: &BuildContext) -> Result<()>;
    fn is_cacheable(&self) -> bool { true }
}

#[derive(Debug, Clone)]
pub struct ShellTaskSpec {
    pub name: String,
    pub deps: Vec<String>,
    pub cmd: String,
    pub dir: Option<PathBuf>,
    pub inputs: Vec<PathBuf>,
    pub outputs: Vec<PathBuf>,
    pub env: HashMap<String, String>,
}

pub struct ShellTask {
    spec: ShellTaskSpec,
}

impl ShellTask {
    pub fn new(spec: ShellTaskSpec) -> Self { Self { spec } }
}

impl Task for ShellTask {
    fn name(&self) -> &str { &self.spec.name }
    fn deps(&self) -> &[String] { &self.spec.deps }
    fn inputs(&self) -> Vec<PathBuf> { self.spec.inputs.clone() }
    fn outputs(&self) -> Vec<PathBuf> { self.spec.outputs.clone() }
    fn params_fingerprint(&self) -> u64 { strings_fingerprint(&[self.spec.cmd.clone()]) }
    fn run(&self, ctx: &BuildContext) -> Result<()> {
        let mut cmd = if cfg!(target_os = "windows") {
            let mut c = Command::new("cmd");
            c.arg("/C").arg(&self.spec.cmd);
            c
        } else {
            let mut c = Command::new("sh");
            c.arg("-c").arg(&self.spec.cmd);
            c
        };
        if let Some(dir) = &self.spec.dir {
            cmd.current_dir(dir);
        } else {
            cmd.current_dir(&ctx.root);
        }
        for (k, v) in &ctx.env { cmd.env(k, v); }
        for (k, v) in &self.spec.env { cmd.env(k, v); }

        log::info!("$ {}", &self.spec.cmd);
        let status = cmd.status()?;
        if !status.success() {
            return Err(BuildError::CommandFailed {
                cmd: self.spec.cmd.clone(),
                code: status.code(),
            });
        }
        Ok(())
    }
}

#[derive(Debug, Clone)]
pub struct CopyTaskSpec {
    pub name: String,
    pub deps: Vec<String>,
    pub from: PathBuf,
    pub to: PathBuf,
    pub recursive: bool,
}

pub struct CopyTask {
    spec: CopyTaskSpec,
}
impl CopyTask {
    pub fn new(spec: CopyTaskSpec) -> Self { Self { spec } }
}
impl Task for CopyTask {
    fn name(&self) -> &str { &self.spec.name }
    fn deps(&self) -> &[String] { &self.spec.deps }
    fn inputs(&self) -> Vec<PathBuf> {
        if self.spec.recursive {
            WalkDir::new(&self.spec.from)
                .into_iter()
                .filter_map(|e| e.ok())
                .filter(|e| e.path().is_file())
                .map(|e| e.path().to_path_buf())
                .collect()
        } else {
            vec![self.spec.from.clone()]
        }
    }
    fn outputs(&self) -> Vec<PathBuf> { vec![self.spec.to.clone()] }
    fn params_fingerprint(&self) -> u64 {
        strings_fingerprint(&[
            self.spec.from.to_string_lossy().into_owned(),
            self.spec.to.to_string_lossy().into_owned(),
            format!("rec={}", self.spec.recursive),
        ])
    }
    fn run(&self, _ctx: &BuildContext) -> Result<()> {
        let from = &self.spec.from;
        let to = &self.spec.to;
        if self.spec.recursive {
            for entry in WalkDir::new(from).into_iter().filter_map(|e| e.ok()) {
                let p = entry.path();
                if p.is_file() {
                    let rel = p.strip_prefix(from).unwrap();
                    let dest = to.join(rel);
                    if let Some(parent) = dest.parent() {
                        fs::create_dir_all(parent)?;
                    }
                    fs::copy(p, &dest)?;
                }
            }
        } else {
            if let Some(parent) = to.parent() { fs::create_dir_all(parent)?; }
            fs::copy(from, to)?;
        }
        Ok(())
    }
}

#[derive(Debug, Clone)]
pub struct MkdirTaskSpec {
    pub name: String,
    pub deps: Vec<String>,
    pub dir: PathBuf,
}
pub struct MkdirTask { spec: MkdirTaskSpec }
impl MkdirTask { pub fn new(spec: MkdirTaskSpec) -> Self { Self { spec } } }
impl Task for MkdirTask {
    fn name(&self) -> &str { &self.spec.name }
    fn deps(&self) -> &[String] { &self.spec.deps }
    fn outputs(&self) -> Vec<PathBuf> { vec![self.spec.dir.clone()] }
    fn params_fingerprint(&self) -> u64 {
        strings_fingerprint(&[self.spec.dir.to_string_lossy().into_owned()])
    }
    fn run(&self, _ctx: &BuildContext) -> Result<()> {
        fs::create_dir_all(&self.spec.dir)?;
        Ok(())
    }
}

#[derive(Debug, Clone)]
pub struct CleanTaskSpec {
    pub name: String,
    pub deps: Vec<String>,
    pub path: PathBuf,
}
pub struct CleanTask { spec: CleanTaskSpec }
impl CleanTask { pub fn new(spec: CleanTaskSpec) -> Self { Self { spec } } }
impl Task for CleanTask {
    fn name(&self) -> &str { &self.spec.name }
    fn deps(&self) -> &[String] { &self.spec.deps }
    fn outputs(&self) -> Vec<PathBuf> { vec![] }
    fn is_cacheable(&self) -> bool { false }
    fn run(&self, _ctx: &BuildContext) -> Result<()> {
        if self.spec.path.exists() {
            if self.spec.path.is_dir() { fs::remove_dir_all(&self.spec.path)?; }
            else { fs::remove_file(&self.spec.path)?; }
        }
        Ok(())
    }
}

#[derive(Debug, Clone)]
pub struct TouchTaskSpec {
    pub name: String,
    pub deps: Vec<String>,
    pub file: PathBuf,
    pub content: Option<String>,
}
pub struct TouchTask { spec: TouchTaskSpec }
impl TouchTask { pub fn new(spec: TouchTaskSpec) -> Self { Self { spec } } }
impl Task for TouchTask {
    fn name(&self) -> &str { &self.spec.name }
    fn deps(&self) -> &[String] { &self.spec.deps }
    fn outputs(&self) -> Vec<PathBuf> { vec![self.spec.file.clone()] }
    fn params_fingerprint(&self) -> u64 {
        strings_fingerprint(&[self.spec.file.to_string_lossy().into_owned(), self.spec.content.clone().unwrap_or_default()])
    }
    fn run(&self, _ctx: &BuildContext) -> Result<()> {
        if let Some(parent) = self.spec.file.parent() { fs::create_dir_all(parent)?; }
        let mut f = File::create(&self.spec.file)?;
        if let Some(s) = &self.spec.content { f.write_all(s.as_bytes())?; }
        Ok(())
    }
}

#[derive(Debug, Clone)]
pub struct HashTaskSpec {
    pub name: String,
    pub deps: Vec<String>,
    pub inputs: Vec<PathBuf>,
    pub out_file: PathBuf,
}
pub struct HashTask { spec: HashTaskSpec }
impl HashTask { pub fn new(spec: HashTaskSpec) -> Self { Self { spec } } }
impl Task for HashTask {
    fn name(&self) -> &str { &self.spec.name }
    fn deps(&self) -> &[String] { &self.spec.deps }
    fn inputs(&self) -> Vec<PathBuf> { self.spec.inputs.clone() }
    fn outputs(&self) -> Vec<PathBuf> { vec![self.spec.out_file.clone()] }
    fn params_fingerprint(&self) -> u64 { 0 }
    fn run(&self, _ctx: &BuildContext) -> Result<()> {
        let h = paths_fingerprint(&self.spec.inputs);
        if let Some(parent) = self.spec.out_file.parent() { fs::create_dir_all(parent)?; }
        fs::write(&self.spec.out_file, format!("{h:016x}"))?;
        Ok(())
    }
}

// ============================================================================
// Enregistrement des tâches
// ============================================================================

pub struct TaskRegistry {
    tasks: HashMap<String, Box<dyn Task>>,
}

impl TaskRegistry {
    pub fn new() -> Self { Self { tasks: HashMap::new() } }

    pub fn register<T: Task + 'static>(&mut self, task: T) -> Result<()> {
        let name = task.name().to_string();
        if self.tasks.contains_key(&name) {
            return Err(BuildError::TaskConflict(name));
        }
        self.tasks.insert(name, Box::new(task));
        Ok(())
    }

    pub fn get(&self, name: &str) -> Option<&Box<dyn Task>> { self.tasks.get(name) }

    pub fn all(&self) -> impl Iterator<Item = (&String, &Box<dyn Task>)> {
        self.tasks.iter()
    }
}

// ============================================================================
// Pipeline et exécution
// ============================================================================

#[derive(Debug, Clone, Default)]
pub struct Pipeline {
    pub targets: Vec<String>,
}

impl Pipeline {
    pub fn all_tasks(reg: &TaskRegistry) -> Self {
        Self {
            targets: reg.tasks.keys().cloned().collect(),
        }
    }
    pub fn from_targets(targets: impl IntoIterator<Item = String>) -> Self {
        Self { targets: targets.into_iter().collect() }
    }
}

pub struct Executor<'a> {
    ctx: &'a BuildContext,
    reg: &'a TaskRegistry,
    cache_path: PathBuf,
    cache: CacheDb,
}

impl<'a> Executor<'a> {
    pub fn new(ctx: &'a BuildContext, reg: &'a TaskRegistry) -> Self {
        let cache_path = ctx.cache_dir.join("cache.json");
        let cache = CacheDb::load(&cache_path);
        Self { ctx, reg, cache_path, cache }
    }

    fn persist_cache(&self) -> Result<()> { self.cache.save(&self.cache_path) }

    fn compute_task_hash(&self, task: &dyn Task) -> u64 {
        let mut inputs = task.inputs();
        // Les sorties participent au hash pour détecter le renommage
        let mut outputs = task.outputs();
        inputs.sort();
        outputs.sort();

        let mut total = 0u64;
        total ^= paths_fingerprint(&inputs);
        total ^= paths_fingerprint(&outputs);
        total ^= task.params_fingerprint();
        total
    }

    fn is_up_to_date(&self, tname: &str, task: &dyn Task) -> bool {
        if !task.is_cacheable() {
            return false;
        }
        let want = self.compute_task_hash(task);
        match self.cache.tasks.get(tname) {
            Some(entry) if entry.hash == want => {
                // Vérifie existence de toutes les sorties
                entry.outputs.iter().all(|o| o.exists())
            }
            _ => false,
        }
    }

    fn update_cache(&mut self, tname: &str, task: &dyn Task) {
        let entry = TaskCacheEntry {
            hash: self.compute_task_hash(task),
            outputs: task.outputs(),
            time: now_unix_secs(),
        };
        self.cache.tasks.insert(tname.to_string(), entry);
    }

    fn build_graph(&self, wanted: &HashSet<String>) -> Result<HashMap<String, Vec<String>>> {
        // Graphe: node -> deps
        let mut graph: HashMap<String, Vec<String>> = HashMap::new();
        let mut stack: Vec<String> = wanted.iter().cloned().collect();
        let mut seen: HashSet<String> = HashSet::new();

        while let Some(n) = stack.pop() {
            if !seen.insert(n.clone()) { continue; }
            let task = self
                .reg
                .get(&n)
                .ok_or_else(|| BuildError::UnknownTask(n.clone()))?;
            let deps = task.deps().to_vec();
            for d in &deps {
                stack.push(d.clone());
            }
            graph.insert(n, deps);
        }

        Ok(graph)
    }

    fn topo_waves(&self, graph: &HashMap<String, Vec<String>>) -> Result<Vec<Vec<String>>> {
        // Kahn: niveaux topologiques (dépendances d'abord)
        // indegree(n) = nombre de dépendances restantes pour n
        let mut indeg: HashMap<&str, usize> = HashMap::new();
        for (n, deps) in graph {
            indeg.insert(n.as_str(), deps.len());
            for d in deps {
                indeg.entry(d.as_str()).or_insert(0);
            }
        }

        // file initiale: nœuds sans dépendance
        let mut q: VecDeque<&str> = indeg
            .iter()
            .filter_map(|(k, &deg)| if deg == 0 { Some(*k) } else { None })
            .collect();

        let mut waves: Vec<Vec<String>> = vec![];
        let mut remaining = indeg.len();

        while !q.is_empty() {
            let mut wave = vec![];
            let mut next: Vec<&str> = vec![];
            let count = q.len();
            for _ in 0..count {
                let n = q.pop_front().unwrap();
                wave.push(n.to_string());
                remaining -= 1;
                // pour chaque m tel que n ∈ deps(m), décrémente indeg(m)
                for (m, deps) in graph {
                    if deps.iter().any(|d| d == n) {
                        let e = indeg.get_mut(m.as_str()).unwrap();
                        *e -= 1;
                        if *e == 0 { next.push(m); }
                    }
                }
            }
            waves.push(wave);
            q.extend(next);
        }

        if remaining != 0 { return Err(BuildError::CyclicGraph); }
        Ok(waves)
    }

    pub fn run(&mut self, pipeline: &Pipeline) -> Result<()> {
        self.ctx.ensure_dirs()?;

        // Étape 1: cibles voulues
        let wanted: HashSet<String> = if pipeline.targets.is_empty() {
            return Err(BuildError::NoTasks);
        } else {
            pipeline.targets.iter().cloned().collect()
        };

        // Étape 2: construire le graphe et vagues topologiques
        let graph = self.build_graph(&wanted)?;
        let waves = self.topo_waves(&graph)?;

        // Étape 3: exécution par vagues
        for wave in waves {
            // On filtre les tâches où cache OK
            let mut to_run: Vec<&Box<dyn Task>> = vec![];
            for name in &wave {
                let t = self.reg.get(name).ok_or_else(|| BuildError::UnknownTask(name.clone()))?;
                if self.is_up_to_date(name, t.as_ref()) {
                    log::info!("✓ {} (cache)", name);
                } else {
                    to_run.push(t);
                }
            }

            // Parallèle
            let errors: Vec<(String, BuildError)> = to_run
                .par_iter()
                .map(|t| {
                    let name = t.name().to_string();
                    let r = t.run(self.ctx);
                    if r.is_ok() {
                        log::info!("✔ {}", name);
                    }
                    (name, r)
                })
                .filter_map(|(name, r)| r.err().map(|e| (name, e)))
                .collect();

            if !errors.is_empty() {
                for (n, e) in &errors {
                    log::error!("✗ {n}: {e}");
                }
                return Err(BuildError::Aborted);
            }

            // Met à jour le cache
            for t in to_run {
                self.update_cache(t.name(), t.as_ref());
            }
            self.persist_cache()?;
        }

        Ok(())
    }
}

// ============================================================================
// Recettes TOML
// ============================================================================

#[derive(Debug, Clone)]
pub enum RecipeTask {
    Shell(ShellTaskSpec),
    Copy(CopyTaskSpec),
    Mkdir(MkdirTaskSpec),
    Clean(CleanTaskSpec),
    Touch(TouchTaskSpec),
    Hash(HashTaskSpec),
}

#[derive(Debug, Clone)]
pub struct Recipe {
    pub env: HashMap<String, String>,
    pub out_dir: Option<PathBuf>,
    pub targets: Vec<String>,
    pub tasks: Vec<RecipeTask>,
}

impl Recipe {
    pub fn load(path: &Path) -> Result<Self> {
        let s = fs::read_to_string(path)?;
        let v: toml::Value = toml::from_str(&s)?;

        // env
        let mut env = HashMap::new();
        if let Some(tbl) = v.get("env").and_then(|e| e.as_table()) {
            for (k, vv) in tbl {
                if let Some(s) = vv.as_str() {
                    env.insert(k.clone(), s.to_string());
                } else {
                    env.insert(k.clone(), vv.to_string());
                }
            }
        }
        // out_dir
        let out_dir = v.get("out_dir").and_then(|x| x.as_str()).map(PathBuf::from);
        // targets
        let targets = v
            .get("targets")
            .and_then(|x| x.as_array())
            .map(|arr| arr.iter().filter_map(|e| e.as_str().map(|s| s.to_string())).collect())
            .unwrap_or_else(|| Vec::new());

        // tasks
        let mut tasks: Vec<RecipeTask> = Vec::new();
        if let Some(arr) = v.get("tasks").and_then(|x| x.as_array()) {
            for t in arr {
                let tbl = match t.as_table() { Some(t) => t, None => continue };
                let typ = tbl.get("type").and_then(|x| x.as_str()).unwrap_or("");
                match typ {
                    "shell" => {
                        let mut spec = ShellTaskSpec {
                            name: String::new(),
                            deps: vec![],
                            cmd: String::new(),
                            dir: None,
                            inputs: vec![],
                            outputs: vec![],
                            env: HashMap::new(),
                        };
                        if let Some(s) = tbl.get("name").and_then(|x| x.as_str()) { spec.name = s.to_string(); }
                        if let Some(s) = tbl.get("cmd").and_then(|x| x.as_str()) { spec.cmd = s.to_string(); }
                        if let Some(s) = tbl.get("dir").and_then(|x| x.as_str()) { spec.dir = Some(PathBuf::from(s)); }
                        if let Some(vv) = tbl.get("deps").and_then(|x| x.as_array()) {
                            spec.deps = vv.iter().filter_map(|e| e.as_str().map(|s| s.to_string())).collect();
                        }
                        if let Some(vv) = tbl.get("inputs").and_then(|x| x.as_array()) {
                            spec.inputs = vv.iter().filter_map(|e| e.as_str().map(PathBuf::from)).collect();
                        }
                        if let Some(vv) = tbl.get("outputs").and_then(|x| x.as_array()) {
                            spec.outputs = vv.iter().filter_map(|e| e.as_str().map(PathBuf::from)).collect();
                        }
                        if let Some(envtbl) = tbl.get("env").and_then(|x| x.as_table()) {
                            for (k, vv) in envtbl {
                                if let Some(s) = vv.as_str() {
                                    spec.env.insert(k.clone(), s.to_string());
                                } else {
                                    spec.env.insert(k.clone(), vv.to_string());
                                }
                            }
                        }
                        tasks.push(RecipeTask::Shell(spec));
                    }
                    "copy" => {
                        let mut spec = CopyTaskSpec { name: String::new(), deps: vec![], from: PathBuf::new(), to: PathBuf::new(), recursive: false };
                        if let Some(s) = tbl.get("name").and_then(|x| x.as_str()) { spec.name = s.to_string(); }
                        if let Some(vv) = tbl.get("deps").and_then(|x| x.as_array()) {
                            spec.deps = vv.iter().filter_map(|e| e.as_str().map(|s| s.to_string())).collect();
                        }
                        if let Some(s) = tbl.get("from").and_then(|x| x.as_str()) { spec.from = PathBuf::from(s); }
                        if let Some(s) = tbl.get("to").and_then(|x| x.as_str()) { spec.to = PathBuf::from(s); }
                        if let Some(b) = tbl.get("recursive").and_then(|x| x.as_bool()) { spec.recursive = b; }
                        tasks.push(RecipeTask::Copy(spec));
                    }
                    "mkdir" => {
                        let mut spec = MkdirTaskSpec { name: String::new(), deps: vec![], dir: PathBuf::new() };
                        if let Some(s) = tbl.get("name").and_then(|x| x.as_str()) { spec.name = s.to_string(); }
                        if let Some(vv) = tbl.get("deps").and_then(|x| x.as_array()) {
                            spec.deps = vv.iter().filter_map(|e| e.as_str().map(|s| s.to_string())).collect();
                        }
                        if let Some(s) = tbl.get("dir").and_then(|x| x.as_str()) { spec.dir = PathBuf::from(s); }
                        tasks.push(RecipeTask::Mkdir(spec));
                    }
                    "clean" => {
                        let mut spec = CleanTaskSpec { name: String::new(), deps: vec![], path: PathBuf::new() };
                        if let Some(s) = tbl.get("name").and_then(|x| x.as_str()) { spec.name = s.to_string(); }
                        if let Some(vv) = tbl.get("deps").and_then(|x| x.as_array()) {
                            spec.deps = vv.iter().filter_map(|e| e.as_str().map(|s| s.to_string())).collect();
                        }
                        if let Some(s) = tbl.get("path").and_then(|x| x.as_str()) { spec.path = PathBuf::from(s); }
                        tasks.push(RecipeTask::Clean(spec));
                    }
                    "touch" => {
                        let mut spec = TouchTaskSpec { name: String::new(), deps: vec![], file: PathBuf::new(), content: None };
                        if let Some(s) = tbl.get("name").and_then(|x| x.as_str()) { spec.name = s.to_string(); }
                        if let Some(vv) = tbl.get("deps").and_then(|x| x.as_array()) {
                            spec.deps = vv.iter().filter_map(|e| e.as_str().map(|s| s.to_string())).collect();
                        }
                        if let Some(s) = tbl.get("file").and_then(|x| x.as_str()) { spec.file = PathBuf::from(s); }
                        if let Some(s) = tbl.get("content").and_then(|x| x.as_str()) { spec.content = Some(s.to_string()); }
                        tasks.push(RecipeTask::Touch(spec));
                    }
                    "hash" => {
                        let mut spec = HashTaskSpec { name: String::new(), deps: vec![], inputs: vec![], out_file: PathBuf::new() };
                        if let Some(s) = tbl.get("name").and_then(|x| x.as_str()) { spec.name = s.to_string(); }
                        if let Some(vv) = tbl.get("deps").and_then(|x| x.as_array()) {
                            spec.deps = vv.iter().filter_map(|e| e.as_str().map(|s| s.to_string())).collect();
                        }
                        if let Some(vv) = tbl.get("inputs").and_then(|x| x.as_array()) {
                            spec.inputs = vv.iter().filter_map(|e| e.as_str().map(PathBuf::from)).collect();
                        }
                        if let Some(s) = tbl.get("out_file").and_then(|x| x.as_str()) { spec.out_file = PathBuf::from(s); }
                        tasks.push(RecipeTask::Hash(spec));
                    }
                    _ => { /* inconnu: ignorer */ }
                }
            }
        }

        Ok(Recipe { env, out_dir, targets, tasks })
    }
}

pub fn registry_from_recipe(root: &Path, recipe: &Recipe) -> Result<(BuildContext, TaskRegistry, Pipeline)> {
    let mut ctx = BuildContext::new(root);
    if let Some(o) = &recipe.out_dir {
        ctx.out_dir = if o.is_absolute() { o.clone() } else { root.join(o) };
    }
    ctx.env.extend(recipe.env.clone());
    ctx.ensure_dirs()?;

    let mut reg = TaskRegistry::new();
    for t in &recipe.tasks {
        match t {
            RecipeTask::Shell(spec) => reg.register(ShellTask::new(spec.clone()))?,
            RecipeTask::Copy(spec) => reg.register(CopyTask::new(spec.clone()))?,
            RecipeTask::Mkdir(spec) => reg.register(MkdirTask::new(spec.clone()))?,
            RecipeTask::Clean(spec) => reg.register(CleanTask::new(spec.clone()))?,
            RecipeTask::Touch(spec) => reg.register(TouchTask::new(spec.clone()))?,
            RecipeTask::Hash(spec) => reg.register(HashTask::new(spec.clone()))?,
        };
    }

    // Si targets vide → toutes les tâches
    let pipe = if recipe.targets.is_empty() {
        Pipeline::all_tasks(&reg)
    } else {
        Pipeline::from_targets(recipe.targets.clone())
    };

    Ok((ctx, reg, pipe))
}

// ============================================================================
// Helpers de fichiers
// ============================================================================

pub fn glob_simple(root: &Path, pattern_ext: &str) -> Vec<PathBuf> {
    // Récupère récursivement tous les fichiers dont l'extension == pattern_ext
    WalkDir::new(root)
        .into_iter()
        .filter_map(|e| e.ok())
        .filter(|e| e.path().is_file())
        .map(|e| e.path().to_path_buf())
        .filter(|p| p.extension().and_then(OsStr::to_str) == Some(pattern_ext))
        .collect()
}

pub fn write_text(path: &Path, s: &str) -> Result<()> {
    if let Some(parent) = path.parent() { fs::create_dir_all(parent)?; }
    fs::write(path, s)?;
    Ok(())
}

// ============================================================================
// Exécution haut-niveau
// ============================================================================

pub fn run_recipe(path: impl AsRef<Path>) -> Result<()> {
    let path = path.as_ref();
    let root = path.parent().unwrap_or_else(|| Path::new("."));
    let recipe = Recipe::load(path)?;
    let (ctx, reg, pipe) = registry_from_recipe(root, &recipe)?;
    let mut ex = Executor::new(&ctx, &reg);
    ex.run(&pipe)
}

// ============================================================================
// Tests
// ============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn pipeline_minimal() {
        let temp = tempfile::tempdir().unwrap();
        let root = temp.path().to_path_buf();

        // Tâches: mkdir build/, touch a.txt, shell echo > b.txt, hash
        let mk = MkdirTaskSpec { name: "mk".into(), deps: vec![], dir: root.join("build") };
        let t1 = TouchTaskSpec { name: "a", deps: vec!["mk".into()], file: root.join("build/a.txt"), content: Some("hello".into()) };
        let s1 = ShellTaskSpec {
            name: "b".into(),
            deps: vec!["mk".into()],
            cmd: format!("echo 'world' > {}", root.join("build/b.txt").to_string_lossy()),
            dir: None, inputs: vec![], outputs: vec![root.join("build/b.txt")], env: HashMap::new()
        };
        let h1 = HashTaskSpec {
            name: "h".into(),
            deps: vec!["a".into(), "b".into()],
            inputs: vec![root.join("build/a.txt"), root.join("build/b.txt")],
            out_file: root.join("build/sum.txt"),
        };

        let mut reg = TaskRegistry::new();
        reg.register(MkdirTask::new(mk)).unwrap();
        reg.register(TouchTask::new(t1)).unwrap();
        reg.register(ShellTask::new(s1)).unwrap();
        reg.register(HashTask::new(h1)).unwrap();

        let ctx = BuildContext::new(&root);
        let mut ex = Executor::new(&ctx, &reg);
        let pipe = Pipeline::all_tasks(&reg);
        ex.run(&pipe).unwrap();

        assert!(root.join("build/a.txt").exists());
        assert!(root.join("build/b.txt").exists());
        assert!(root.join("build/sum.txt").exists());

        // Deuxième run → cache pour a et b et h peut régénérer si inchangé
        ex.run(&pipe).unwrap();
    }

    #[test]
    fn recipe_toml_round() {
        let temp = tempfile::tempdir().unwrap();
        let root = temp.path();

        // Crée une recette TOML
        let recipe = r#"
env = { LANG = "C" }
out_dir = "out"
targets = ["hash"]

[[tasks]]
type = "mkdir"
name = "mk"
dir = "out"

[[tasks]]
type = "touch"
name = "gen"
deps = ["mk"]
file = "out/data.txt"
content = "x"

[[tasks]]
type = "hash"
name = "hash"
deps = ["gen"]
inputs = ["out/data.txt"]
out_file = "out/hash.txt"
"#;
        let recipe_path = root.join("build.toml");
        fs::write(&recipe_path, recipe).unwrap();

        run_recipe(&recipe_path).unwrap();
        assert!(root.join("out/hash.txt").exists());
    }
}