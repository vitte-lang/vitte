//! vitte-dist-runner — exécution distribuée pour Vitte
//!
//! Capacités
//! - Exécution locale, SSH, Docker, Kubernetes (features).
//! - Description de tâche portable (cmd + args + env + files).
//! - Sandbox basique: cwd, timeout, mémoire/CPU indicatifs.
//! - API synchrone simple + points d’extension.
//!
//! Contraintes
//! - Requiert `std`.
//! - SSH/Docker/K8s sont optionnels (features `ssh`, `docker`, `k8s`).

#![cfg_attr(not(feature = "std"), no_std)]
#![deny(missing_docs)]
#![forbid(unsafe_op_in_unsafe_fn)]

#[cfg(not(feature = "std"))]
compile_error!("vitte-dist-runner nécessite std");

extern crate alloc;

use alloc::{string::String, vec, vec::Vec};
use camino::{Utf8Path, Utf8PathBuf};

#[cfg(feature = "std")]
use std::{collections::HashMap, time::Duration};

/// Résultat type.
pub type Result<T, E = Error> = core::result::Result<T, E>;

/// Erreurs.
#[derive(Debug, thiserror::Error)]
pub enum Error {
    /// Entrée invalide.
    #[error("invalid: {0}")]
    Invalid(String),
    /// I/O ou système.
    #[error("io: {0}")]
    Io(String),
    /// Processus distant a échoué.
    #[error("process failed: code={code:?}\n{stderr}")]
    Proc {
        /// Code retour.
        code: Option<i32>,
        /// Stderr (tronqué).
        stderr: String,
    },
    /// Fonctionnalité absente.
    #[error("unsupported: {0}")]
    Unsupported(&'static str),
    /// Kubernetes/SSH/Docker spécifiques.
    #[error("backend: {0}")]
    Backend(String),
}

/// Identité de cible d’exécution.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct Target {
    /// Nom logique de nœud ou contexte.
    pub name: String,
    /// Type de backend.
    pub kind: BackendKind,
}

/// Backend sélectionné.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum BackendKind {
    /// Machine locale.
    Local,
    /// SSH: hôte distant.
    #[cfg(feature = "ssh")]
    Ssh,
    /// Docker: conteneur.
    #[cfg(feature = "docker")]
    Docker,
    /// Kubernetes: Pod/Job.
    #[cfg(feature = "k8s")]
    K8s,
}

/// Limites d’exécution (best-effort).
#[derive(Debug, Clone, Copy, Default)]
pub struct Limits {
    /// Timeout total.
    pub timeout: Option<Duration>,
    /// Mémoire en MiB (hint).
    pub memory_mib: Option<u64>,
    /// CPU milli-cores (hint).
    pub cpu_millis: Option<u32>,
}

/// Fichiers à monter / transférer.
#[derive(Debug, Clone)]
pub struct FileSpec {
    /// Chemin local source.
    pub local: Utf8PathBuf,
    /// Chemin distant dans le sandbox.
    pub remote: Utf8PathBuf,
    /// Lecture seule.
    pub readonly: bool,
}

/// Spécification d’un job.
#[derive(Debug, Clone)]
pub struct JobSpec {
    /// Binaire/commande à exécuter.
    pub cmd: String,
    /// Arguments.
    pub args: Vec<String>,
    /// Variables d’environnement.
    pub env: Vec<(String, String)>,
    /// Répertoire de travail.
    pub cwd: Option<Utf8PathBuf>,
    /// Fichiers à attacher.
    pub files: Vec<FileSpec>,
    /// Limites.
    pub limits: Limits,
}

impl Default for JobSpec {
    fn default() -> Self {
        Self {
            cmd: "true".into(),
            args: vec![],
            env: vec![],
            cwd: None,
            files: vec![],
            limits: Limits::default(),
        }
    }
}

/// Résultat d’exécution.
#[derive(Debug, Clone)]
pub struct RunResult {
    /// Code de retour.
    pub code: i32,
    /// Stdout.
    pub stdout: Vec<u8>,
    /// Stderr.
    pub stderr: Vec<u8>,
    /// Durée réelle.
    pub elapsed: Option<Duration>,
}

/// Contrat d’un exécuteur.
pub trait Runner {
    /// Exécute la tâche et retourne son résultat.
    fn run(&self, job: &JobSpec) -> Result<RunResult>;
}

/* ----------------------------- Utilitaires ------------------------------ */

fn truncate(mut s: String) -> String {
    const MAX: usize = 24_000;
    if s.len() > MAX {
        s.truncate(MAX);
    }
    s
}

#[cfg(feature = "std")]
fn build_env(base: &[(String, String)]) -> HashMap<String, String> {
    let mut m = HashMap::new();
    for (k, v) in base {
        m.insert(k.clone(), v.clone());
    }
    m
}

/* -------------------------------- Local --------------------------------- */

/// Runner local (processus enfant).
#[cfg(feature = "std")]
#[derive(Debug, Default, Clone)]
pub struct LocalRunner;

#[cfg(feature = "std")]
impl Runner for LocalRunner {
    fn run(&self, job: &JobSpec) -> Result<RunResult> {
        use std::io::Read;
        use std::process::{Command, Stdio};
        let mut cmd = Command::new(&job.cmd);
        cmd.args(&job.args)
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped());

        if let Some(cwd) = &job.cwd {
            cmd.current_dir(cwd.as_std_path());
        }
        // env
        cmd.envs(build_env(&job.env));

        // files: local runner n’effectue pas de montage; l’appelant doit préparer cwd.
        // limits: best-effort timeout
        let start = std::time::Instant::now();
        let mut child = cmd.spawn().map_err(|e| Error::Io(e.to_string()))?;

        let timed_out;
        if let Some(t) = job.limits.timeout {
            timed_out = match child.wait_timeout::ChildExt::wait_timeout(&mut child, t) {
                Ok(Some(_status)) => false,
                Ok(None) => {
                    let _ = child.kill();
                    let _ = child.wait();
                    true
                }
                Err(e) => return Err(Error::Io(e.to_string())),
            };
        } else {
            timed_out = false;
        }

        let output = if !timed_out {
            child.wait_with_output().map_err(|e| Error::Io(e.to_string()))?
        } else {
            // synthèse sur timeout
            std::process::Output { status: std::process::ExitStatus::from_raw(137), stdout: vec![], stderr: b"timeout".to_vec() }
        };

        let code = output.status.code().unwrap_or(127);
        let elapsed = Some(start.elapsed());
        if output.status.success() {
            Ok(RunResult { code, stdout: output.stdout, stderr: output.stderr, elapsed })
        } else {
            Err(Error::Proc { code: Some(code), stderr: truncate(String::from_utf8_lossy(&output.stderr).into_owned()) })
        }
    }
}

// petite extension wait_timeout sans dépendance externe: cfg(unix) seulement
#[cfg(all(feature = "std", unix))]
mod wait_timeout {
    use std::os::unix::process::ExitStatusExt;
    use std::process::Child;
    use std::time::{Duration, Instant};
    pub trait ChildExt {
        fn wait_timeout(child: &mut Child, timeout: Duration) -> std::io::Result<Option<std::process::ExitStatus>>;
    }
    impl ChildExt for Child {
        fn wait_timeout(child: &mut Child, timeout: Duration) -> std::io::Result<Option<std::process::ExitStatus>> {
            let end = Instant::now() + timeout;
            loop {
                match child.try_wait()? {
                    Some(s) => return Ok(Some(s)),
                    None => {
                        if Instant::now() >= end {
                            return Ok(None);
                        }
                        std::thread::sleep(Duration::from_millis(10));
                    }
                }
            }
        }
    }
}
#[cfg(all(feature = "std", not(unix)))]
mod wait_timeout {
    use std::process::Child;
    use std::time::Duration;
    pub trait ChildExt {
        fn wait_timeout(_child: &mut Child, _timeout: Duration) -> std::io::Result<Option<std::process::ExitStatus>>;
    }
    impl ChildExt for Child {
        fn wait_timeout(_child: &mut Child, _timeout: Duration) -> std::io::Result<Option<std::process::ExitStatus>> {
            // Pas de try_wait stable avec timeout précis sur Windows sans poll
            Ok(None)
        }
    }
}

/* --------------------------------- SSH ---------------------------------- */

/// Runner SSH.
#[cfg(all(feature = "ssh", feature = "std"))]
#[derive(Debug, Clone)]
pub struct SshRunner {
    /// Cible SSH, ex: `user@host`.
    pub target: String,
    /// Répertoire distant de travail.
    pub workdir: Utf8PathBuf,
}

#[cfg(all(feature = "ssh", feature = "std"))]
impl Runner for SshRunner {
    fn run(&self, job: &JobSpec) -> Result<RunResult> {
        use openssh::{KnownHosts, Session};
        use std::time::Instant;

        let sess = Session::connect(&self.target, KnownHosts::Accept)
            .map_err(|e| Error::Backend(format!("ssh connect: {e}")))?;

        // Préparer workdir
        sess.command("mkdir").arg("-p").arg(self.workdir.as_str())
            .output()
            .map_err(|e| Error::Backend(format!("ssh mkdir: {e}")))?;

        // Upload des fichiers
        for f in &job.files {
            let remote = self.workdir.join(f.remote.as_str().trim_start_matches('/'));
            let parent = remote.parent().map(Utf8PathBuf::from).unwrap_or(self.workdir.clone());
            let _ = sess.command("mkdir").arg("-p").arg(parent.as_str()).output();
            sess.sftp().map_err(|e| Error::Backend(e.to_string()))?
                .write_to(&remote.as_str(), std::fs::read(f.local.as_std_path()).map_err(|e| Error::Io(e.to_string()))?)
                .map_err(|e| Error::Backend(e.to_string()))?;
        }

        // Construire la commande
        let mut cmd = String::new();
        // env
        for (k, v) in &job.env {
            cmd.push_str(&format!("{k}={v} "));
        }
        // cwd
        let cwd = job.cwd.clone().unwrap_or(self.workdir.clone());
        // timeout via `timeout` si dispo
        if let Some(t) = job.limits.timeout {
            let secs = t.as_secs().max(1);
            cmd.push_str(&format!("timeout {secs} "));
        }
        // commande
        cmd.push_str(&shell_escape(&job.cmd));
        for a in &job.args {
            cmd.push(' ');
            cmd.push_str(&shell_escape(a));
        }

        let start = Instant::now();
        let out = sess.command("bash").args(["-lc", &format!("cd {} && {}", cwd, cmd)])
            .output()
            .map_err(|e| Error::Backend(format!("ssh exec: {e}")))?;
        let elapsed = Some(start.elapsed());

        if out.status.success() {
            Ok(RunResult { code: out.status.code().unwrap_or(0), stdout: out.stdout, stderr: out.stderr, elapsed })
        } else {
            Err(Error::Proc { code: out.status.code(), stderr: truncate(String::from_utf8_lossy(&out.stderr).into_owned()) })
        }
    }
}

#[cfg(all(feature = "ssh", feature = "std"))]
fn shell_escape(s: &str) -> String {
    if s.bytes().all(|b| matches!(b, b'0'..=b'9' | b'a'..=b'z' | b'A'..=b'Z' | b'_' | b'-' | b'/' | b'.' | b':' | b'@')) {
        s.to_string()
    } else {
        format!("'{}'", s.replace('\'', r"'\''"))
    }
}

/* -------------------------------- Docker -------------------------------- */

/// Runner Docker.
#[cfg(all(feature = "docker", feature = "std"))]
#[derive(Debug, Clone)]
pub struct DockerRunner {
    /// Image docker.
    pub image: String,
    /// Volumes à monter (host->container).
    pub volumes: Vec<(Utf8PathBuf, Utf8PathBuf, bool)>, // (host, container, readonly)
    /// Entrypoint override optionnel.
    pub entrypoint: Option<String>,
    /// Args supplémentaires `docker run`.
    pub extra: Vec<String>,
}

#[cfg(all(feature = "docker", feature = "std"))]
impl Runner for DockerRunner {
    fn run(&self, job: &JobSpec) -> Result<RunResult> {
        use duct::cmd;

        // docker run --rm -w <cwd> -e K=V -v host:cont[:ro] image [cmd args...]
        let cwd = job.cwd.as_ref().map(|p| p.as_str()).unwrap_or("/");
        let mut args: Vec<String> = vec!["run".into(), "--rm".into(), "-w".into(), cwd.into()];

        for (k, v) in &job.env {
            args.push("-e".into());
            args.push(format!("{k}={v}"));
        }
        for (h, c, ro) in &self.volumes {
            let mut spec = format!("{}:{}", h, c);
            if *ro { spec.push_str(":ro"); }
            args.push("-v".into());
            args.push(spec);
        }
        if let Some(ep) = &self.entrypoint {
            args.push("--entrypoint".into());
            args.push(ep.clone());
        }
        args.extend(self.extra.clone());
        args.push(self.image.clone());
        args.push(job.cmd.clone());
        args.extend(job.args.clone());

        let out = cmd("docker", args).stderr_capture().stdout_capture().unchecked().run()
            .map_err(|e| Error::Io(e.to_string()))?;
        if out.status.success() {
            Ok(RunResult {
                code: out.status.code().unwrap_or(0),
                stdout: out.stdout,
                stderr: out.stderr,
                elapsed: None,
            })
        } else {
            Err(Error::Proc { code: out.status.code(), stderr: truncate(String::from_utf8_lossy(&out.stderr).into_owned()) })
        }
    }
}

/* ---------------------------------- K8s --------------------------------- */

/// Runner Kubernetes (Pod/Job éphémère minimal).
#[cfg(all(feature = "k8s", feature = "std"))]
#[derive(Debug, Clone)]
pub struct K8sRunner {
    /// Namespace.
    pub namespace: String,
    /// Image de base.
    pub image: String,
    /// ServiceAccount optionnel.
    pub sa: Option<String>,
}

#[cfg(all(feature = "k8s", feature = "std"))]
impl Runner for K8sRunner {
    fn run(&self, job: &JobSpec) -> Result<RunResult> {
        // Implémentation complète requiert manifestes + watch des logs.
        // Ici: stub explicite pour signaler la dépendance.
        Err(Error::Unsupported("k8s runner stub; brancher kube::Api<Job/Pod> et streamer les logs"))
    }
}

/* ------------------------------ Orchestrateur --------------------------- */

/// Politique de planification.
#[derive(Debug, Clone, Copy)]
pub enum Schedule {
    /// Tourniquet simple.
    RoundRobin,
    /// Toujours local.
    LocalOnly,
}

/// Cluster minimal.
#[derive(Debug)]
pub struct Cluster {
    nodes: Vec<(Target, Box<dyn Runner + Send + Sync>)>,
    sched: Schedule,
    rr: usize,
}

impl Cluster {
    /// Crée un cluster.
    pub fn new(sched: Schedule) -> Self {
        Self { nodes: vec![], sched, rr: 0 }
    }
    /// Ajoute un nœud.
    pub fn add_node<R>(&mut self, target: Target, runner: R)
    where
        R: Runner + Send + Sync + 'static,
    {
        self.nodes.push((target, Box::new(runner)));
    }
    /// Exécute un job selon la politique.
    pub fn run(&mut self, job: &JobSpec) -> Result<RunResult> {
        match self.sched {
            Schedule::LocalOnly => {
                let r = LocalRunner::default();
                r.run(job)
            }
            Schedule::RoundRobin => {
                if self.nodes.is_empty() {
                    return Err(Error::Invalid("cluster vide".into()));
                }
                let idx = self.rr % self.nodes.len();
                self.rr = self.rr.wrapping_add(1);
                self.nodes[idx].1.run(job)
            }
        }
    }
}

/* --------------------------------- Tests -------------------------------- */

#[cfg(all(test, feature = "std"))]
mod tests {
    use super::*;

    #[test]
    fn local_true() {
        let job = JobSpec {
            cmd: "printf".into(),
            args: vec!["ok".into()],
            ..Default::default()
        };
        let r = super::LocalRunner::default().run(&job).unwrap();
        assert_eq!(r.stdout, b"ok");
    }

    #[test]
    fn rr_scheduler() {
        let mut cl = Cluster::new(Schedule::RoundRobin);
        cl.add_node(
            Target { name: "local".into(), kind: BackendKind::Local },
            super::LocalRunner::default(),
        );
        let job = JobSpec { cmd: "true".into(), ..Default::default() };
        let _ = cl.run(&job).unwrap();
    }
}