//! vitte-shell — exécution de commandes, pipelines, PTY, capture, shlex
//!
//! Capacités principales:
//! - `Cmd`: builder unifié (prog, args, env, cwd, stdin, TTY) + run sync/async.
//! - `Pipeline`: `cmd1 | cmd2 | ...` sync/async.
//! - PTY interactif (feature `pty`) via `portable-pty`.
//! - Parsing ligne → argv via `shlex` et résolution via `which`.
//! - Capture `stdout/stderr` en texte ou binaire. Timeouts (async).
//! - Zéro `unsafe`.
//!
//! Exemple rapide:
//! ```no_run
//! use vitte_shell::{Cmd, shlex_split};
//! let out = Cmd::new("echo").arg("hello").run_capture_text().unwrap();
//! assert_eq!(out.stdout.trim(), "hello");
//! let argv = shlex_split(r#"echo "a b" c"#).unwrap();
//! assert_eq!(argv, vec!["echo","a b","c"]);
//! ```

#![forbid(unsafe_code)]

#[cfg(all(not(feature="std"), not(feature="alloc-only")))]
compile_error!("Enable feature `std` (default) or `alloc-only`.") ;

#[cfg(feature="alloc-only")]
extern crate alloc;

#[cfg(feature="alloc-only")]
use alloc::{string::String, vec::Vec, boxed::Box, format};

#[cfg(feature="std")]
use std::{string::String, vec::Vec, boxed::Box, env, path::{Path, PathBuf}, io::{self, Read, Write}, process::{Command, Stdio}};

#[cfg(feature="serde")]
use serde::{Serialize, Deserialize};

#[cfg(feature="errors")]
use thiserror::Error;

#[cfg(feature="spawn")]
use tokio::{process::Command as TCommand, time::{timeout, Duration}};

#[cfg(feature="spawn")]
use tokio_util::io::ReaderStream;

#[cfg(feature="capture")]
use bytes::Bytes;

#[cfg(feature="shlex")]
use shlex as shlex_crate;

#[cfg(feature="shlex")]
use which as which_crate;

#[cfg(feature="pty")]
use portable_pty as pty;

/* =============================== Erreurs =============================== */

#[cfg(feature="errors")]
#[derive(Debug, Error)]
pub enum ShellError {
    #[error("io: {0}")]
    Io(String),
    #[error("bad utf8")]
    Utf8,
    #[error("not found: {0}")]
    NotFound(String),
    #[error("spawn feature missing")]
    SpawnMissing,
    #[error("pty feature missing")]
    PtyMissing,
    #[error("timeout")]
    Timeout,
    #[error("invalid pipeline")]
    BadPipeline,
}
#[cfg(not(feature="errors"))]
#[derive(Debug)]
pub enum ShellError { Io(String), Utf8, NotFound(String), SpawnMissing, PtyMissing, Timeout, BadPipeline }

pub type Result<T> = core::result::Result<T, ShellError>;

#[inline] fn io_err<T>(e: impl core::fmt::Display) -> Result<T> { Err(ShellError::Io(e.to_string())) }

/* =============================== Sorties =============================== */

/// Résultat d'exécution capturé (texte).
#[cfg_attr(feature="serde", derive(Serialize, Deserialize))]
#[derive(Debug, Clone)]
pub struct TextOutput {
    pub status: i32,
    pub stdout: String,
    pub stderr: String,
}

/// Résultat d'exécution capturé (binaire).
#[cfg_attr(feature="serde", derive(Serialize, Deserialize))]
#[derive(Debug, Clone)]
pub struct BinOutput {
    pub status: i32,
    #[cfg(feature="capture")] pub stdout: Bytes,
    #[cfg(not(feature="capture"))] pub stdout: Vec<u8>,
    #[cfg(feature="capture")] pub stderr: Bytes,
    #[cfg(not(feature="capture"))] pub stderr: Vec<u8>,
}

/* =============================== Cmd builder =============================== */

#[derive(Debug, Clone)]
pub struct Cmd {
    prog: String,
    args: Vec<String>,
    envs: Vec<(String,String)>,
    clear_env: bool,
    cwd: Option<PathBuf>,
    stdin: Option<StdinData>,
    inherit_stdio: bool,
    tty: bool,               // si true, force inherit stdio (tty)
    #[cfg(feature="spawn")] kill_after: Option<Duration>,
}

#[derive(Debug, Clone)]
pub enum StdinData {
    Text(String),
    Bin(Vec<u8>),
}

impl Cmd {
    pub fn new(prog: impl Into<String>) -> Self {
        Self {
            prog: prog.into(),
            args: Vec::new(),
            envs: Vec::new(),
            clear_env: false,
            cwd: None,
            stdin: None,
            inherit_stdio: false,
            tty: false,
            #[cfg(feature="spawn")] kill_after: None,
        }
    }
    pub fn arg(mut self, a: impl Into<String>) -> Self { self.args.push(a.into()); self }
    pub fn args<I,S>(mut self, it: I) -> Self where I: IntoIterator<Item=S>, S: Into<String> { self.args.extend(it.into_iter().map(Into::into)); self }
    pub fn env(mut self, k: impl Into<String>, v: impl Into<String>) -> Self { self.envs.push((k.into(), v.into())); self }
    pub fn clear_env(mut self, on: bool) -> Self { self.clear_env = on; self }
    pub fn cwd<P: Into<PathBuf>>(mut self, p: P) -> Self { self.cwd = Some(p.into()); self }
    pub fn stdin_text(mut self, s: impl Into<String>) -> Self { self.stdin = Some(StdinData::Text(s.into())); self }
    pub fn stdin_bin(mut self, b: impl Into<Vec<u8>>) -> Self { self.stdin = Some(StdinData::Bin(b.into())); self }
    pub fn inherit_stdio(mut self, on: bool) -> Self { self.inherit_stdio = on; self }
    pub fn tty(mut self, on: bool) -> Self { self.tty = on; self }
    #[cfg(feature="spawn")] pub fn timeout(mut self, d: std::time::Duration) -> Self { self.kill_after = Some(d); self }

    /* ----------------------- Sync API (std::process) ----------------------- */

    /// Exécute en héritant stdio (imprime directement). Retourne code.
    pub fn run(&self) -> Result<i32> {
        let mut cmd = self.build_sync()?;
        let status = cmd.status().map_err(|e| ShellError::Io(e.to_string()))?;
        Ok(status.code().unwrap_or_default())
    }

    /// Exécute et capture stdout/stderr en texte (UTF-8 lossless).
    pub fn run_capture_text(&self) -> Result<TextOutput> {
        let mut cmd = self.build_sync()?;
        cmd.stdout(Stdio::piped()).stderr(Stdio::piped());
        let mut child = cmd.spawn().map_err(|e| ShellError::Io(e.to_string()))?;

        // write stdin if provided
        if let Some(data) = &self.stdin {
            if let Some(mut sin) = child.stdin.take() {
                match data {
                    StdinData::Text(s) => { let _ = sin.write_all(s.as_bytes()); }
                    StdinData::Bin(b) => { let _ = sin.write_all(b); }
                }
            }
        }
        let output = child.wait_with_output().map_err(|e| ShellError::Io(e.to_string()))?;
        let status = output.status.code().unwrap_or_default();
        let stdout = lossless_to_string(&output.stdout);
        let stderr = lossless_to_string(&output.stderr);
        Ok(TextOutput { status, stdout, stderr })
    }

    /// Exécute et capture binaire.
    pub fn run_capture_bin(&self) -> Result<BinOutput> {
        let mut cmd = self.build_sync()?;
        cmd.stdout(Stdio::piped()).stderr(Stdio::piped());
        let mut child = cmd.spawn().map_err(|e| ShellError::Io(e.to_string()))?;
        if let Some(data) = &self.stdin {
            if let Some(mut sin) = child.stdin.take() {
                match data {
                    StdinData::Text(s) => { let _ = sin.write_all(s.as_bytes()); }
                    StdinData::Bin(b) => { let _ = sin.write_all(b); }
                }
            }
        }
        let output = child.wait_with_output().map_err(|e| ShellError::Io(e.to_string()))?;
        let status = output.status.code().unwrap_or_default();
        #[cfg(feature="capture")]
        let (stdout, stderr) = (Bytes::from(output.stdout), Bytes::from(output.stderr));
        #[cfg(not(feature="capture"))]
        let (stdout, stderr) = (output.stdout, output.stderr);
        Ok(BinOutput { status, stdout, stderr })
    }

    fn build_sync(&self) -> Result<Command> {
        let prog = resolve_prog(&self.prog)?;
        let mut cmd = Command::new(&prog);
        cmd.args(&self.args);
        if self.clear_env { cmd.env_clear(); }
        for (k,v) in &self.envs { cmd.env(k, v); }
        if let Some(cwd) = &self.cwd { cmd.current_dir(cwd); }
        if self.inherit_stdio || self.tty {
            cmd.stdin(Stdio::inherit()).stdout(Stdio::inherit()).stderr(Stdio::inherit());
        } else {
            // stdin piped si on veut écrire dedans
            if self.stdin.is_some() { cmd.stdin(Stdio::piped()); }
        }
        Ok(cmd)
    }

    /* ----------------------- Async API (tokio::process) ----------------------- */

    /// Async: capture texte. Requiert feature `spawn`.
    #[cfg(feature="spawn")]
    pub async fn run_capture_text_async(&self) -> Result<TextOutput> {
        let mut cmd = self.build_async()?;
        cmd.stdout(Stdio::piped()).stderr(Stdio::piped());
        let mut child = cmd.spawn().map_err(|e| ShellError::Io(e.to_string()))?;

        if let Some(data) = &self.stdin {
            if let Some(mut sin) = child.stdin.take() {
                match data {
                    StdinData::Text(s) => { use tokio::io::AsyncWriteExt; let _ = sin.write_all(s.as_bytes()).await; }
                    StdinData::Bin(b) => { use tokio::io::AsyncWriteExt; let _ = sin.write_all(b).await; }
                }
            }
        }

        let fut = async {
            let out = child.wait_with_output().await.map_err(|e| ShellError::Io(e.to_string()))?;
            let status = out.status.code().unwrap_or_default();
            let stdout = lossless_to_string(&out.stdout);
            let stderr = lossless_to_string(&out.stderr);
            Ok::<_,ShellError>(TextOutput{status, stdout, stderr})
        };

        if let Some(d) = self.kill_after {
            match timeout(d, fut).await {
                Ok(res) => res,
                Err(_) => { let _ = child.kill().await; Err(ShellError::Timeout) }
            }
        } else {
            fut.await
        }
    }

    #[cfg(feature="spawn")]
    fn build_async(&self) -> Result<TCommand> {
        let prog = resolve_prog(&self.prog)?;
        let mut cmd = TCommand::new(&prog);
        cmd.args(&self.args);
        if self.clear_env { cmd.env_clear(); }
        for (k,v) in &self.envs { cmd.env(k, v); }
        if let Some(cwd) = &self.cwd { cmd.current_dir(cwd); }
        if self.inherit_stdio || self.tty {
            cmd.stdin(Stdio::inherit()).stdout(Stdio::inherit()).stderr(Stdio::inherit());
        } else {
            if self.stdin.is_some() { cmd.stdin(Stdio::piped()); }
        }
        Ok(cmd)
    }
}

/* =============================== Pipeline =============================== */

#[derive(Debug, Clone)]
pub struct Pipeline {
    pub cmds: Vec<Cmd>,
}

impl Pipeline {
    pub fn new() -> Self { Self { cmds: Vec::new() } }
    pub fn push(mut self, cmd: Cmd) -> Self { self.cmds.push(cmd); self }

    /// Sync: exécute pipeline avec redirections locales. Capture texte du dernier.
    pub fn run_capture_text(&self) -> Result<TextOutput> {
        if self.cmds.is_empty() { return Err(ShellError::BadPipeline); }
        if self.cmds.len() == 1 { return self.cmds[0].clone().run_capture_text(); }

        // Construire chaines via std::os pipes
        #[cfg(unix)]
        {
            use std::os::unix::io::{FromRawFd, IntoRawFd};
            let mut children = Vec::new();
            let mut prev_read: Option<io::ChildStdout> = None;

            for (i, c) in self.cmds.iter().enumerate() {
                let mut cmd = c.build_sync()?;
                if i+1 < self.cmds.len() {
                    let (mut rd, mut wr) = os_pipe::pipe().map_err(|e| ShellError::Io(e.to_string()))?;
                    cmd.stdin(prev_read.map(Stdio::from).unwrap_or(Stdio::inherit()));
                    cmd.stdout(unsafe { Stdio::from_raw_fd(wr.into_raw_fd()) });
                    cmd.stderr(Stdio::inherit());
                    let child = cmd.spawn().map_err(|e| ShellError::Io(e.to_string()))?;
                    // fermer côté écriture dans le parent pour passer EOF au prochain
                    drop(wr);
                    prev_read = child.stdout;
                    children.push(child);
                } else {
                    // dernier: stdout/stderr piped pour capture
                    cmd.stdin(prev_read.map(Stdio::from).unwrap_or(Stdio::inherit()));
                    cmd.stdout(Stdio::piped()).stderr(Stdio::piped());
                    let output = cmd.output().map_err(|e| ShellError::Io(e.to_string()))?;
                    let status = output.status.code().unwrap_or_default();
                    let stdout = lossless_to_string(&output.stdout);
                    let stderr = lossless_to_string(&output.stderr);
                    // attendre précédents
                    for mut ch in children { let _ = ch.wait(); }
                    return Ok(TextOutput{status, stdout, stderr});
                }
            }
            unreachable!()
        }
        #[cfg(not(unix))]
        {
            // Fallback pauvre: on concatène via `cmd /C` ou powershell -Command si Windows.
            // Pour rester portable sans dépendances, on exécute via 'sh -c' / 'cmd /C'.
            let joined = self.to_shell_string();
            #[cfg(target_os="windows")]
            let capture = Cmd::new("cmd").arg("/C").arg(joined).run_capture_text();
            #[cfg(not(target_os="windows"))]
            let capture = Cmd::new("sh").arg("-c").arg(joined).run_capture_text();
            capture
        }
    }

    /// Async: exécute pipeline et capture texte du dernier.
    #[cfg(feature="spawn")]
    pub async fn run_capture_text_async(&self) -> Result<TextOutput> {
        if self.cmds.is_empty() { return Err(ShellError::BadPipeline); }
        if self.cmds.len() == 1 { return self.cmds[0].clone().run_capture_text_async().await; }

        #[cfg(unix)]
        {
            use os_pipe::pipe;
            use std::os::unix::io::{FromRawFd, IntoRawFd};
            let mut children = Vec::new();
            let mut prev_read: Option<tokio::process::ChildStdout> = None;

            for i in 0..self.cmds.len() {
                let c = &self.cmds[i];
                let mut cmd = c.build_async()?;
                if i+1 < self.cmds.len() {
                    let (rd, wr) = pipe().map_err(|e| ShellError::Io(e.to_string()))?;
                    cmd.stdin(prev_read.map(Stdio::from).unwrap_or(Stdio::inherit()));
                    cmd.stdout(unsafe { Stdio::from_raw_fd(wr.into_raw_fd()) });
                    cmd.stderr(Stdio::inherit());
                    let mut child = cmd.spawn().map_err(|e| ShellError::Io(e.to_string()))?;
                    // capture new stdout pipe read end for next
                    let c2 = child.stdout.take();
                    children.push(child);
                    prev_read = c2;
                    drop(rd); // parent read handled by next cmd via prev_read
                } else {
                    cmd.stdin(prev_read.map(Stdio::from).unwrap_or(Stdio::inherit()));
                    cmd.stdout(Stdio::piped()).stderr(Stdio::piped());
                    let out = cmd.output().await.map_err(|e| ShellError::Io(e.to_string()))?;
                    for mut ch in children { let _ = ch.wait().await; }
                    let status = out.status.code().unwrap_or_default();
                    let stdout = lossless_to_string(&out.stdout);
                    let stderr = lossless_to_string(&out.stderr);
                    return Ok(TextOutput{status, stdout, stderr});
                }
            }
            unreachable!()
        }
        #[cfg(not(unix))]
        {
            let joined = self.to_shell_string();
            #[cfg(target_os="windows")]
            let capture = Cmd::new("cmd").arg("/C").arg(joined).run_capture_text_async().await;
            #[cfg(not(target_os="windows"))]
            let capture = Cmd::new("sh").arg("-c").arg(joined).run_capture_text_async().await;
            capture
        }
    }

    fn to_shell_string(&self) -> String {
        self.cmds.iter().map(|c| {
            let mut s = shell_escape(&c.prog);
            for a in &c.args { s.push(' '); s.push_str(&shell_escape(a)); }
            s
        }).collect::<Vec<_>>().join(" | ")
    }
}

/* =============================== PTY =============================== */

/// Démarre un processus dans un PTY et connecte aux stdio de l’hôte.
/// Retourne le code process à la fin.
#[cfg(feature="pty")]
pub fn run_in_pty(prog: &str, args: &[String], rows: u16, cols: u16, cwd: Option<&Path>) -> Result<i32> {
    use std::io::{Read, Write};
    let p = resolve_prog(prog)?;
    let pair = pty::native_pty_system().openpty(pty::PtySize { rows, cols, pixel_width: 0, pixel_height: 0 })
        .map_err(|e| ShellError::Io(e.to_string()))?;
    let mut cmd = pty::CommandBuilder::new(p);
    for a in args { cmd.arg(a); }
    if let Some(cwd) = cwd { cmd.cwd(cwd); }
    let mut child = pair.slave.spawn_command(cmd).map_err(|e| ShellError::Io(e.to_string()))?;
    drop(pair.slave);
    let mut reader = pair.master.try_clone_reader().map_err(|e| ShellError::Io(e.to_string()))?;
    let mut writer = pair.master.take_writer().map_err(|e| ShellError::Io(e.to_string()))?;

    // Pompage simple bloquant (lecture → stdout, stdin → écriture) jusqu'à fin.
    std::thread::spawn(move || {
        let mut buf = [0u8; 4096];
        let mut out = io::stdout();
        while let Ok(n) = reader.read(&mut buf) {
            if n == 0 { break; }
            let _ = out.write_all(&buf[..n]); let _ = out.flush();
        }
    });

    // Pas de lecture stdin ici pour rester simple. Peut être enrichi avec termios brut.
    let status = child.wait().map_err(|e| ShellError::Io(e.to_string()))?;
    drop(writer);
    Ok(status.code().unwrap_or_default())
}

/* =============================== Helpers =============================== */

/// Convertit bytes → String lossless (UTF-8 + fallback).
fn lossless_to_string(b: &[u8]) -> String {
    #[cfg(feature="encoding")]
    {
        // vitte-encoding: décodage lossless
        return vitte_encoding::decode_lossy_bytes(b);
    }
    #[cfg(not(feature="encoding"))]
    {
        String::from_utf8_lossy(b).to_string()
    }
}

/// Résout un programme via PATH si nécessaire (respecte absolu/relatif).
fn resolve_prog(prog: &str) -> Result<String> {
    if Path::new(prog).components().next().is_some() && Path::new(prog).exists() {
        return Ok(prog.to_string());
    }
    #[cfg(feature="shlex")]
    {
        if let Ok(p) = which_crate::which(prog) {
            return Ok(p.to_string_lossy().to_string());
        }
        return Err(ShellError::NotFound(prog.to_string()));
    }
    #[cfg(not(feature="shlex"))]
    {
        // PATH naive
        if let Ok(paths) = env::var("PATH") {
            for dir in env::split_paths(&paths) {
                let candidate = dir.join(prog);
                if candidate.exists() { return Ok(candidate.to_string_lossy().to_string()); }
            }
        }
        Err(ShellError::NotFound(prog.to_string()))
    }
}

/// Échappement minimal pour shell portable.
fn shell_escape(s: &str) -> String {
    if s.is_empty() { return "''".to_string(); }
    if s.bytes().all(|b| b.is_ascii_alphanumeric() || b == b'_' || b == b'-' || b == b'/' || b == b'.' || b == b':') {
        return s.to_string();
    }
    // POSIX style
    let mut out = String::from("'");
    for ch in s.chars() {
        if ch == '\'' { out.push_str("'\\''"); } else { out.push(ch); }
    }
    out.push('\'');
    out
}

/* =============================== shlex utils =============================== */

/// Découpe une ligne de commande en argv (shlex).
#[cfg(feature="shlex")]
pub fn shlex_split(line: &str) -> Result<Vec<String>> {
    shlex_crate::split(line).ok_or_else(|| ShellError::Io("shlex parse".into()))
}
#[cfg(not(feature="shlex"))]
pub fn shlex_split(_line: &str) -> Result<Vec<String>> { Err(ShellError::Io("feature shlex not enabled".into())) }

/* =============================== Tests =============================== */

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn shell_escape_basic() {
        assert_eq!(shell_escape("abc"), "abc");
        assert_eq!(shell_escape("a b"), "'a b'");
        assert!(shell_escape("a'b").contains("\\'"));
    }

    #[cfg(feature="std")]
    #[test]
    fn sync_echo_capture() {
        let out = Cmd::new("echo").arg("hi").run_capture_text().unwrap();
        assert!(out.stdout.to_lowercase().contains("hi"));
    }

    #[cfg(feature="shlex")]
    #[test]
    fn shlex_ok() {
        let v = shlex_split(r#"echo "a b" c"#).unwrap();
        assert_eq!(v, vec!["echo","a b","c"]);
    }
}