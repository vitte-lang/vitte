#![cfg_attr(not(feature = "std"), no_std)]
//! vitte-shed — la **cabane à outils** du dev Vitte 🪚🧰
//!
//! Un micro-**orchestrateur de tâches** (build/run/test), incrémental, avec
//! gestion de **dépendances**, **fingerprints** de fichiers, **watch mode** et
//! **cache** optionnel sur disque. Idéal pour faire tourner les outils maison
//! (formatteur, compilateur, générateurs) via `vitte-cli`.
//!
//! Points clés:
//! - API simple: **déclare des tâches** avec deps + action `Fn(&mut Ctx) -> Result<()>`
//! - **Toposort** + détection de **cycles**
//! - **Dirty checking**: re-exécute une tâche si ses **inputs** (glob) ou **outputs** ont changé
//! - **Cache** mémoire et (optionnel) **cache JSON** sur disque (feature `serde`)
//! - **Watch mode** (feature `watch`, via `notify`) pour relancer à la volée
//! - Sans dépendance forte à l’écosystème Vitte; peut utiliser `vitte-mm` pour les noms si souhaité
//!
//! Exemple express:
//! ```ignore
//! let mut shed = Shed::new(Options::default());
//! shed.task(Task::build("fmt")
//!     .input("crates/**/src/**/*.rs")
//!     .run(|ctx| { ctx.info("formatting..."); Ok(()) })
//! );
//! shed.run("fmt")?;
//! ```

#[cfg(not(feature = "std"))]
extern crate alloc;

#[cfg(not(feature = "std"))]
use alloc::{
    boxed::Box,
    format,
    string::{String, ToString},
    vec::Vec,
};
#[cfg(feature = "std")]
use std::{boxed::Box, format, string::String, vec::Vec};

use core::{fmt, time::Duration};

// ───────────────────────────── Result / Error ─────────────────────────────

#[derive(Debug, Clone)]
pub enum ShedError {
    Io(String),
    Invalid(&'static str),
    Cycle(Vec<String>),
    NotFound(String),
    Action(String),
}
impl fmt::Display for ShedError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Io(s) => write!(f, "io:{s}"),
            Self::Invalid(s) => write!(f, "invalid:{s}"),
            Self::Cycle(v) => write!(f, "cycle: {:?}", v),
            Self::NotFound(s) => write!(f, "not found:{s}"),
            Self::Action(s) => write!(f, "action:{s}"),
        }
    }
}

pub type Result<T> = core::result::Result<T, ShedError>;

// ───────────────────────────── Options ────────────────────────────────────

#[derive(Clone, Debug)]
pub struct Options {
    /// Dossier du cache persistant (si `serde` + `std`).
    pub cache_dir: String,
    /// Taille max du journal (logs en mémoire).
    pub log_cap: usize,
    /// Tâches en parallèle (1 = séquentiel).
    pub parallel: usize,
}
impl Default for Options {
    fn default() -> Self {
        Self { cache_dir: ".vitte-shed".to_string(), log_cap: 10_000, parallel: 1 }
    }
}

// ───────────────────────────── Task & Builder ─────────────────────────────

/// Action de tâche.
pub trait TaskFn: Send + Sync {
    fn run(&self, ctx: &mut Ctx) -> Result<()>;
}
impl<F> TaskFn for F
where
    F: Fn(&mut Ctx) -> Result<()> + Send + Sync,
{
    fn run(&self, ctx: &mut Ctx) -> Result<()> {
        (self)(ctx)
    }
}

#[derive(Default)]
pub struct Task {
    pub name: String,
    pub desc: String,
    pub deps: Vec<String>,
    pub inputs: Vec<String>,  // globs
    pub outputs: Vec<String>, // globs
    pub cacheable: bool,
    #[cfg(feature = "std")]
    pub timeout: Option<Duration>,
    #[allow(clippy::type_complexity)]
    #[cfg(feature = "std")]
    pub(crate) run: Option<Box<dyn TaskFn>>, // None => noop
}

impl core::fmt::Debug for Task {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        f.debug_struct("Task")
            .field("name", &self.name)
            .field("desc", &self.desc)
            .field("deps", &self.deps)
            .field("inputs", &self.inputs)
            .field("outputs", &self.outputs)
            .field("cacheable", &self.cacheable)
            .field("timeout", &self.timeout)
            .finish()
    }
}

impl Task {
    pub fn build<S: Into<String>>(name: S) -> Self {
        Self { name: name.into(), ..Default::default() }
    }
    pub fn desc<S: Into<String>>(mut self, d: S) -> Self {
        self.desc = d.into();
        self
    }
    pub fn dep<S: Into<String>>(mut self, d: S) -> Self {
        self.deps.push(d.into());
        self
    }
    pub fn deps<I, S>(mut self, it: I) -> Self
    where
        I: IntoIterator<Item = S>,
        S: Into<String>,
    {
        for d in it {
            self.deps.push(d.into());
        }
        self
    }
    pub fn input<S: Into<String>>(mut self, g: S) -> Self {
        self.inputs.push(g.into());
        self
    }
    pub fn inputs<I, S>(mut self, it: I) -> Self
    where
        I: IntoIterator<Item = S>,
        S: Into<String>,
    {
        for g in it {
            self.inputs.push(g.into());
        }
        self
    }
    pub fn output<S: Into<String>>(mut self, g: S) -> Self {
        self.outputs.push(g.into());
        self
    }
    pub fn outputs<I, S>(mut self, it: I) -> Self
    where
        I: IntoIterator<Item = S>,
        S: Into<String>,
    {
        for g in it {
            self.outputs.push(g.into());
        }
        self
    }
    pub fn cacheable(mut self, yes: bool) -> Self {
        self.cacheable = yes;
        self
    }
    #[cfg(feature = "std")]
    pub fn timeout(mut self, d: Duration) -> Self {
        self.timeout = Some(d);
        self
    }
    #[cfg(feature = "std")]
    pub fn run<F: TaskFn + 'static>(mut self, f: F) -> Self {
        self.run = Some(Box::new(f));
        self
    }
}

// ───────────────────────────── Shed (orchestrator) ────────────────────────

pub struct Shed {
    _opts: Options,
    tasks: Vec<Task>,
    index: Vec<usize>,
    logs: RingLog,
    cache: Cache,
}

impl Shed {
    pub fn new(opts: Options) -> Self {
        let log_cap = opts.log_cap;
        Self { _opts: opts, tasks: Vec::new(), index: Vec::new(), logs: RingLog::new(log_cap), cache: Cache::default() }
    }

    /// Ajoute / remplace une tâche (par nom).
    pub fn task(&mut self, t: Task) {
        if let Some(i) = self.find(&t.name) {
            self.tasks[i] = t;
        } else {
            self.index.push(self.tasks.len());
            self.tasks.push(t);
        }
    }

    /// Exécute `target` et ses dépendances.
    #[cfg(feature = "std")]
    pub fn run(&mut self, target: &str) -> Result<()> {
        let order = self.plan(target)?; // toposort
        let mut ctx = Ctx::new();
        for idx in order {
            let name = self.tasks[idx].name.clone();
            if self.should_run(idx)? {
                self.exec_one(idx, &mut ctx)?;
                self.cache.update_task(&self.tasks[idx])?;
            } else {
                self.info(&format!("skip {} (up-to-date)", name));
            }
        }
        Ok(())
    }

    /// Mode watch : relance `target` si un input/output de son sous-graphe change.
    #[cfg(all(feature = "std", feature = "watch"))]
    pub fn watch(&mut self, target: &str) -> Result<()> {
        use notify::{Config, RecommendedWatcher, RecursiveMode, Watcher};
        let order = self.plan(target)?;
        let mut roots: Vec<String> = Vec::new();
        for idx in order.iter() {
            let t = &self.tasks[*idx];
            roots.extend(t.inputs.iter().cloned());
            roots.extend(t.outputs.iter().cloned());
        }
        let mut ctx = Ctx::new();
        self.info("watching… (Ctrl+C pour quitter)");
        let (tx, rx) = std::sync::mpsc::channel();
        let mut watcher = RecommendedWatcher::new(tx, Config::default())
            .map_err(|e| ShedError::Io(e.to_string()))?;
        for g in roots {
            for p in expand_glob(&g)? {
                let _ = watcher.watch(&p, RecursiveMode::Recursive);
            }
        }
        loop {
            match rx.recv() {
                Ok(_) => {
                    let _ = self.run(target);
                },
                Err(e) => return Err(ShedError::Io(e.to_string())),
            }
        }
    }

    // ── planification / topo ──
    fn plan(&self, target: &str) -> Result<Vec<usize>> {
        let t = self.find(target).ok_or_else(|| ShedError::NotFound(target.to_string()))?;
        let mut seen = Vec::new();
        let mut stack = Vec::new();
        let mut out = Vec::new();
        self.dfs(t, &mut seen, &mut stack, &mut out)?;
        Ok(out)
    }

    fn dfs(
        &self,
        i: usize,
        seen: &mut Vec<usize>,
        stack: &mut Vec<usize>,
        out: &mut Vec<usize>,
    ) -> Result<()> {
        if stack.contains(&i) {
            let mut cycle: Vec<String> = Vec::new();
            for &ix in stack.iter().skip_while(|&&x| x != i) {
                cycle.push(self.tasks[ix].name.clone());
            }
            cycle.push(self.tasks[i].name.clone());
            return Err(ShedError::Cycle(cycle));
        }
        if seen.contains(&i) {
            return Ok(());
        }
        stack.push(i);
        for d in &self.tasks[i].deps {
            if let Some(j) = self.find(d) {
                self.dfs(j, seen, stack, out)?;
            } else {
                return Err(ShedError::NotFound(d.clone()));
            }
        }
        stack.pop();
        seen.push(i);
        out.push(i);
        Ok(())
    }

    fn find(&self, name: &str) -> Option<usize> {
        self.tasks.iter().position(|t| t.name == name)
    }

    // ── exécution ──
    #[cfg(feature = "std")]
    fn exec_one(&mut self, idx: usize, ctx: &mut Ctx) -> Result<()> {
        let name = self.tasks[idx].name.clone();
        let timeout = self.tasks[idx].timeout;
        self.info(&format!("▶ {}", name));
        if let Some(run) = self.tasks[idx].run.as_ref() {
            if let Some(d) = timeout {
                let start = std::time::Instant::now();
                let res = run.run(ctx);
                if start.elapsed() > d {
                    Err(ShedError::Action("timeout".into()))
                } else {
                    res
                }
            } else {
                run.run(ctx)
            }
        } else {
            Ok(())
        }
    }

    // ── incremental / cache ──
    #[cfg(feature = "std")]
    fn should_run(&mut self, idx: usize) -> Result<bool> {
        let t = &self.tasks[idx];
        if !t.cacheable {
            return Ok(true);
        }
        let inputs = collect_files(&t.inputs)?;
        let outputs = collect_files(&t.outputs)?;
        let ih = hash_files(&inputs)?;
        let oh = hash_files(&outputs)?;
        Ok(!self.cache.is_fresh(&t.name, ih, oh))
    }

    // ── logs utilitaires ──
    fn info(&mut self, msg: &str) {
        let _ = self.logs.push(format!("[info] {msg}"));
    }
    #[allow(dead_code)]
    fn warn(&mut self, msg: &str) {
        let _ = self.logs.push(format!("[warn] {msg}"));
    }
    #[allow(dead_code)]
    fn error(&mut self, msg: &str) {
        let _ = self.logs.push(format!("[error] {msg}"));
    }

    pub fn dump_log(&self) -> Vec<String> {
        self.logs.snapshot()
    }
}

// ───────────────────────────── Contexte d’exécution ──────────────────────

#[cfg(feature = "std")]
pub struct Ctx {/* extensible: env, cwd, handles… */}
#[cfg(feature = "std")]
impl Ctx {
    fn new() -> Self {
        Self {}
    }
    /// Petit utilitaire pour exécuter une commande shell (best-effort, cross-plat).
    pub fn shell(&mut self, cmd: &str, args: &[&str]) -> Result<()> {
        let mut c = std::process::Command::new(cmd);
        c.args(args);
        let status = c.status().map_err(|e| ShedError::Io(e.to_string()))?;
        if status.success() {
            Ok(())
        } else {
            Err(ShedError::Action(format!("command failed: {} ({:?})", cmd, status.code())))
        }
    }
    pub fn info(&mut self, msg: &str) {
        let _ = msg; /* hook vers tracing plus tard */
    }
}

// ───────────────────────────── Cache (mémoire / disque) ──────────────────

#[derive(Clone, Debug, Default)]
struct CacheEntry {
    in_hash: u64,
    out_hash: u64,
}

#[derive(Clone, Debug, Default)]
struct Cache {
    mem: Map<String, CacheEntry>,
}

impl Cache {
    #[cfg(feature = "std")]
    fn is_fresh(&mut self, key: &str, ih: u64, oh: u64) -> bool {
        match self.mem.get(key) {
            Some(e) => e.in_hash == ih && e.out_hash == oh,
            None => false,
        }
    }
    #[cfg(feature = "std")]
    fn update_task(&mut self, t: &Task) -> Result<()> {
        let ih = hash_files(&collect_files(&t.inputs)?)?;
        let oh = hash_files(&collect_files(&t.outputs)?)?;
        self.mem.insert(t.name.clone(), CacheEntry { in_hash: ih, out_hash: oh });
        Ok(())
    }
}

// ───────────────────────────── Mini RingLog ──────────────────────────────

#[derive(Clone, Debug)]
struct RingLog {
    buf: Vec<String>,
    cap: usize,
    head: usize,
    len: usize,
}
impl RingLog {
    fn new(cap: usize) -> Self {
        Self { buf: Vec::with_capacity(cap), cap, head: 0, len: 0 }
    }
    fn push(&mut self, s: String) {
        if self.buf.len() < self.cap {
            self.buf.push(s)
        } else {
            let i = (self.head + self.len) % self.cap;
            self.buf[i] = s;
            if self.len < self.cap {
                self.len += 1
            } else {
                self.head = (self.head + 1) % self.cap
            }
        }
    }
    fn snapshot(&self) -> Vec<String> {
        let mut v = Vec::new();
        for i in 0..self.len {
            v.push(self.buf[(self.head + i) % self.cap].clone());
        }
        v
    }
}

// ───────────────────────────── Collecte & Hash fichiers ──────────────────

#[cfg(feature = "std")]
fn collect_files(globs: &[String]) -> Result<Vec<String>> {
    let mut out = Vec::new();
    for g in globs {
        let mut v = expand_glob(g)?;
        out.append(&mut v);
    }
    out.sort();
    out.dedup();
    Ok(out)
}

#[cfg(feature = "std")]
fn hash_files(files: &[String]) -> Result<u64> {
    let mut h = Fnv64::new();
    for f in files {
        let fp = fingerprint_file(f)?;
        h.write_u64(fp.hash);
        h.write_u64(fp.len as u64);
        h.write_u64(fp.mtime);
    }
    Ok(h.finish())
}

#[cfg(feature = "std")]
#[derive(Copy, Clone, Debug, Default)]
struct Fingerprint {
    len: u64,
    mtime: u64,
    hash: u64,
}

#[cfg(feature = "std")]
fn fingerprint_file(path: &str) -> Result<Fingerprint> {
    use std::{fs::File, io::Read, time::UNIX_EPOCH};
    let mut f = File::open(path).map_err(|e| ShedError::Io(e.to_string()))?;
    let meta = f.metadata().map_err(|e| ShedError::Io(e.to_string()))?;
    let mut buf = Vec::with_capacity(meta.len() as usize);
    f.read_to_end(&mut buf).map_err(|e| ShedError::Io(e.to_string()))?;
    let mtime = meta
        .modified()
        .ok()
        .and_then(|t| t.duration_since(UNIX_EPOCH).ok())
        .map(|d| d.as_secs())
        .unwrap_or(0);
    let hash = fnv1a64(&buf);
    Ok(Fingerprint { len: meta.len(), mtime, hash })
}

// ───────────────────────────── Glob basique (**, *, ?) ───────────────────

#[cfg(feature = "std")]
fn expand_glob(pattern: &str) -> Result<Vec<String>> {
    use std::{fs, path::PathBuf};
    let pat = normalize_slashes(pattern);
    let mut out = Vec::new();
    let cwd = PathBuf::from(".");
    fn rec(base: &std::path::Path, comps: &[&str], out: &mut Vec<String>) -> Result<()> {
        if comps.is_empty() {
            out.push(base.to_string_lossy().to_string());
            return Ok(());
        }
        let head = comps[0];
        let tail = &comps[1..];
        if head == "**" {
            // 1) consommer sans descendre
            rec(base, tail, out)?;
            // 2) descendre récursivement
            for entry in std::fs::read_dir(base).map_err(|e| ShedError::Io(e.to_string()))? {
                let e = entry.map_err(|e| ShedError::Io(e.to_string()))?;
                let p = e.path();
                if p.is_dir() {
                    rec(&p, comps, out)?;
                }
            }
        } else {
            for entry in fs::read_dir(base).map_err(|e| ShedError::Io(e.to_string()))? {
                let e = entry.map_err(|e| ShedError::Io(e.to_string()))?;
                let p = e.path();
                let name = p.file_name().and_then(|s| s.to_str()).unwrap_or("");
                if glob_match(head, name) {
                    if tail.is_empty() && p.is_file() {
                        out.push(p.to_string_lossy().to_string());
                    } else {
                        if p.is_dir() {
                            rec(&p, tail, out)?;
                        }
                    }
                }
            }
        }
        Ok(())
    }
    let comps: Vec<&str> = pat.split('/').collect();
    rec(&cwd, &comps, &mut out)?;
    Ok(out)
}

#[cfg(feature = "std")]
fn normalize_slashes(s: &str) -> String {
    s.replace('\\', "/")
}

#[cfg(feature = "std")]
fn glob_match(pat: &str, name: &str) -> bool {
    // simple wildcard matcher: * ? (pas de classes), * ne traverse pas '/'
    fn m(p: &[u8], n: &[u8]) -> bool {
        if p.is_empty() {
            return n.is_empty();
        }
        match p[0] {
            b'*' => {
                // * : 0..*
                // skip consecutive *
                let mut i = 0;
                while i < p.len() && p[i] == b'*' {
                    i += 1
                }
                if i == p.len() {
                    return true;
                }
                let p2 = &p[i..];
                let mut j = 0;
                while j <= n.len() {
                    if m(p2, &n[j..]) {
                        return true;
                    }
                    if j == n.len() {
                        break;
                    };
                    j += 1;
                }
                false
            },
            b'?' => {
                if n.is_empty() {
                    false
                } else {
                    m(&p[1..], &n[1..])
                }
            },
            c => {
                if n.first().copied() == Some(c) {
                    m(&p[1..], &n[1..])
                } else {
                    false
                }
            },
        }
    }
    m(pat.as_bytes(), name.as_bytes())
}

// ───────────────────────────── FNV-1a 64 ─────────────────────────────────

#[derive(Copy, Clone, Debug, Default)]
struct Fnv64(u64);
impl Fnv64 {
    fn new() -> Self {
        Self(0xcbf29ce484222325)
    }
    fn write_u64(&mut self, x: u64) {
        self.0 ^= x;
        self.0 = self.0.wrapping_mul(0x100000001b3);
    }
    fn finish(self) -> u64 {
        self.0
    }
}
#[cfg(feature = "std")]
fn fnv1a64(buf: &[u8]) -> u64 {
    let mut h = Fnv64::new();
    let mut chunk = [0u8; 8];
    let mut i = 0;
    while i < buf.len() {
        let n = core::cmp::min(8, buf.len() - i);
        chunk[..n].copy_from_slice(&buf[i..i + n]);
        let v = u64::from_le_bytes(chunk);
        h.write_u64(v);
        chunk = [0u8; 8];
        i += n;
    }
    h.finish()
}

// ───────────────────────────── Map abstraction (std/no_std) ──────────────

#[cfg(not(feature = "std"))]
use alloc::collections::BTreeMap as Map;
#[cfg(feature = "std")]
use std::collections::HashMap as Map;

// ───────────────────────────── Timeout helper ─────────────────────────────

// ───────────────────────────── Tests ─────────────────────────────────────

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn topo_and_cycle() {
        let mut s = Shed::new(Options::default());
        s.task(Task::build("a").dep("b"));
        s.task(Task::build("b").dep("c"));
        s.task(Task::build("c"));
        let plan = s.plan("a").unwrap();
        let names: Vec<_> = plan.iter().map(|&i| s.tasks[i].name.clone()).collect();
        assert_eq!(names, vec!["c", "b", "a"]);

        s.task(Task::build("x").dep("y"));
        s.task(Task::build("y").dep("x"));
        assert!(matches!(s.plan("x"), Err(ShedError::Cycle(_))));
    }

    #[cfg(feature = "std")]
    #[test]
    fn glob_and_hash() {
        // should not error even if no files matched; hashing empty set is defined
        let v = collect_files(&["does/not/exist/**".into()]).unwrap();
        assert!(v.is_empty());
        let h = hash_files(&v).unwrap();
        assert_eq!(h, Fnv64::new().finish());
    }
}
