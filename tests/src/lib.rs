#![forbid(unsafe_code)]
//! Helpers communs aux tests d’intégration **Vitte**.
//!
//! - Par défaut : **aucune dépendance externe** (tout en `std`) → portable Linux/macOS/Windows.
//! - Feature `cli`      : runners pour la CLI via `cargo run -p vitte-tools --bin ...` (std::process).
//! - Feature `snapshots`: helpers `insta` (suffix CI/OS, normalisation légère).
//!
//! # Exemples
//! ```ignore
//! use vitte_tests::prelude::*;
//! init_logging();
//! let root = repo_root();
//! let s = read_fixture("projects/hello/expected.txt");
//! assert_contains!(&s, "hello");
//! ```

use std::{
    env,
    ffi::OsStr,
    fs,
    path::{Path, PathBuf},
    time::{SystemTime, UNIX_EPOCH},
};

/// Réexports pratiques pour les tests.
pub mod prelude {
    #[cfg(feature = "cli")]
    pub use super::cli::{pack_and_run, Cli, CliWhich};
    #[cfg(feature = "snapshots")]
    pub use super::snap::{assert_snapshot_str, set_snapshot_suffix_from_ci};
    pub use super::{
        assert_contains, assert_ends_with, assert_not_contains, assert_starts_with, ensure_dir,
        ensure_parent, fixtures_root, init_logging, list_files_with_ext, read_fixture, read_string,
        repo_root, sanitize_id, tmp::TempDir, write_file,
    };
}

/// Init de logs minimal, idempotent (sans dépendances).
pub fn init_logging() {
    use std::sync::Once;
    static ONCE: Once = Once::new();
    ONCE.call_once(|| {
        if env::var_os("RUST_LOG").is_none() {
            env::set_var("RUST_LOG", "info");
        }
        eprintln!(
            "[tests] logging enabled  os={} arch={} pid={}",
            env::consts::OS,
            env::consts::ARCH,
            std::process::id()
        );
    });
}

/// Remonte jusqu’au **workspace root** (fichier `Cargo.toml` contenant `[workspace]`).
pub fn repo_root() -> PathBuf {
    let mut p = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    for _ in 0..16 {
        let cand = p.join("Cargo.toml");
        if let Ok(s) = fs::read_to_string(&cand) {
            if s.contains("[workspace]") {
                return p;
            }
        }
        if !p.pop() {
            break;
        }
    }
    // Fallback : parent immédiat du crate de tests
    PathBuf::from(env!("CARGO_MANIFEST_DIR")).parent().unwrap_or(Path::new(".")).to_path_buf()
}

/// Racine des fixtures: `<repo>/tests/fixtures`
pub fn fixtures_root() -> PathBuf {
    repo_root().join("tests").join("fixtures")
}

/// Lecture UTF-8 avec message clair (panic si erreur).
pub fn read_string(path: impl AsRef<Path>) -> String {
    fs::read_to_string(path.as_ref())
        .unwrap_or_else(|e| panic!("read_string({}): {e}", path.as_ref().display()))
}

/// Lecture d’un fichier sous `tests/fixtures/...`.
pub fn read_fixture(rel: impl AsRef<Path>) -> String {
    read_string(fixtures_root().join(rel))
}

/// Écrit un fichier (UTF-8). Crée les dossiers parents si besoin.
pub fn write_file(path: impl AsRef<Path>, content: &str) {
    ensure_parent(path.as_ref());
    fs::write(path.as_ref(), content)
        .unwrap_or_else(|e| panic!("write_file({}): {e}", path.as_ref().display()));
}

/// Crée un dossier (avec parents).
pub fn ensure_dir(dir: impl AsRef<Path>) {
    fs::create_dir_all(dir.as_ref())
        .unwrap_or_else(|e| panic!("ensure_dir({}): {e}", dir.as_ref().display()));
}

/// Crée les parents d’un fichier.
pub fn ensure_parent(path: &Path) {
    if let Some(parent) = path.parent() {
        ensure_dir(parent);
    }
}

/// Parcours récursif : liste tous les fichiers d’extension `ext` sous `root`.
pub fn list_files_with_ext(root: &Path, ext: &str) -> Vec<PathBuf> {
    fn walk(out: &mut Vec<PathBuf>, dir: &Path, ext: &str) {
        if let Ok(rd) = fs::read_dir(dir) {
            for e in rd.flatten() {
                let p = e.path();
                if p.is_dir() {
                    walk(out, &p, ext);
                } else if p.extension().and_then(OsStr::to_str) == Some(ext) {
                    out.push(p);
                }
            }
        }
    }
    let mut v = Vec::new();
    walk(&mut v, root, ext);
    v.sort();
    v
}

/// Nettoie un chemin pour afficher/identifier (portable).
pub fn sanitize_id(path: &Path) -> String {
    path.to_string_lossy().replace('\\', "/").replace("../", "").replace("./", "").replace(':', "_")
}

/// `assert_contains!` : vérifie qu’une chaîne en contient une autre.
#[macro_export]
macro_rules! assert_contains {
    ($hay:expr, $needle:expr $(,)?) => {{
        let hay = &$hay;
        let needle = &$needle;
        if !hay.contains(needle) {
            panic!(
                "assert_contains failed\n  needle: {:?}\n  hay (first 240): {:.240}\n",
                needle, hay
            );
        }
    }};
}

/// `assert_not_contains!` : vérifie qu’une chaîne **ne** contient **pas** une autre.
#[macro_export]
macro_rules! assert_not_contains {
    ($hay:expr, $needle:expr $(,)?) => {{
        let hay = &$hay;
        let needle = &$needle;
        if hay.contains(needle) {
            panic!(
                "assert_not_contains failed\n  needle: {:?}\n  hay (first 240): {:.240}\n",
                needle, hay
            );
        }
    }};
}

/// `assert_starts_with!`
#[macro_export]
macro_rules! assert_starts_with {
    ($s:expr, $prefix:expr $(,)?) => {{
        let s = &$s;
        let prefix = &$prefix;
        if !s.starts_with(prefix) {
            panic!("assert_starts_with failed\n  prefix: {:?}\n  s: {:.240}\n", prefix, s);
        }
    }};
}

/// `assert_ends_with!`
#[macro_export]
macro_rules! assert_ends_with {
    ($s:expr, $suffix:expr $(,)?) => {{
        let s = &$s;
        let suffix = &$suffix;
        if !s.ends_with(suffix) {
            panic!("assert_ends_with failed\n  suffix: {:?}\n  s: {:.240}\n", suffix, s);
        }
    }};
}

/// Mini tempdir auto-nettoyé (std only).
pub mod tmp {
    use super::*;
    /// Dossier temporaire supprimé en `Drop` (best-effort).
    #[derive(Debug)]
    pub struct TempDir {
        path: PathBuf,
    }
    impl TempDir {
        /// Crée un dossier unique sous `%TMP%/vitte-tests-<salt>`.
        pub fn new() -> Self {
            let mut base = env::temp_dir();
            base.push(format!("vitte-tests-{:x}", randomish()));
            super::ensure_dir(&base);
            Self { path: base }
        }
        /// Chemin du dossier.
        pub fn path(&self) -> &Path {
            &self.path
        }
        /// Écrit un fichier relatif à ce tempdir et renvoie son chemin absolu.
        pub fn write(&self, rel: impl AsRef<Path>, content: &str) -> PathBuf {
            let p = self.path.join(rel);
            super::write_file(&p, content);
            p
        }
    }
    impl Drop for TempDir {
        fn drop(&mut self) {
            let _ = fs::remove_dir_all(&self.path);
        }
    }
    fn randomish() -> u64 {
        let t = SystemTime::now().duration_since(UNIX_EPOCH).unwrap_or_default().as_nanos() as u64;
        let pid = std::process::id() as u64;
        let mut x = t ^ pid ^ 0x9E37_79B9_7F4A_7C15;
        x ^= x << 7;
        x ^= x >> 9;
        x
    }
}

#[cfg(feature = "cli")]
pub mod cli {
    //! Runners CLI pour `vitte`, `vitte-pack`, `vitte-run` via `cargo run`.
    //!
    //! Ajoute à `tests/Cargo.toml` (si tu veux activer) :
    //! ```toml
    //! [features]
    //! cli = []
    //! ```
    use super::*;
    use std::ffi::OsString;
    use std::process::{Command, Output};

    /// Quel outil lancer.
    #[derive(Clone, Copy, Debug)]
    pub enum CliWhich {
        /// Binaire multi-commandes : `vitte <subcommand>`
        Vitte,
        /// Binaire dédié pack : `vitte-pack`
        VittePack,
        /// Binaire dédié run  : `vitte-run`
        VitteRun,
    }

    /// Config CLI (workspace/feature independent).
    #[derive(Clone, Debug)]
    pub struct Cli {
        /// Le **package** qui expose les bins (défaut: `vitte-tools`).
        pub package: String,
        /// Ajoute `-q` pour des sorties plus propres.
        pub quiet: bool,
    }

    impl Default for Cli {
        fn default() -> Self {
            Self { package: "vitte-tools".to_string(), quiet: true }
        }
    }

    impl Cli {
        /// Construit une `Command` pour `cargo run -p <pkg> --bin <bin> -- <args…>`.
        fn cargo_run(&self, bin: &str, args: &[OsString]) -> Command {
            let mut cmd = Command::new("cargo");
            cmd.arg("run");
            if self.quiet {
                cmd.arg("-q");
            }
            cmd.arg("-p").arg(&self.package);
            cmd.arg("--bin").arg(bin);
            cmd.arg("--");
            for a in args {
                cmd.arg(a);
            }
            cmd
        }

        /// Exécute `vitte-pack --input <dir> --output <bc>` (binaire dédié).
        pub fn pack_bin(&self, input_dir: &Path, out_bc: &Path) -> Output {
            let args = [
                OsString::from("--input"),
                input_dir.as_os_str().to_owned(),
                OsString::from("--output"),
                out_bc.as_os_str().to_owned(),
            ];
            self.cargo_run("vitte-pack", &args).output().expect("launch vitte-pack")
        }

        /// Exécute `vitte-run --input <bc>` (binaire dédié).
        pub fn run_bin(&self, in_bc: &Path) -> Output {
            let args = [OsString::from("--input"), in_bc.as_os_str().to_owned()];
            self.cargo_run("vitte-run", &args).output().expect("launch vitte-run")
        }

        /// Exécute `vitte <subcmd> ...` si tu préfères un **seul** binaire.
        pub fn vitte_subcommand(&self, subcmd: &str, extra: &[OsString]) -> Output {
            let mut args = Vec::with_capacity(1 + extra.len());
            args.push(OsString::from(subcmd));
            args.extend_from_slice(extra);
            self.cargo_run("vitte", &args).output().expect("launch vitte")
        }

        /// Helper : `pack` (cible `vitte-pack` par défaut).
        pub fn pack(&self, input_dir: &Path, out_bc: &Path) -> Output {
            self.pack_bin(input_dir, out_bc)
        }

        /// Helper : `run` (cible `vitte-run` par défaut).
        pub fn run(&self, in_bc: &Path) -> Output {
            self.run_bin(in_bc)
        }

        /// Convertit un `Output` en `(stdout_utf8, stderr_utf8)`. Panic si non-UTF8.
        pub fn output_to_strings(output: &Output) -> (String, String) {
            (
                String::from_utf8(output.stdout.clone()).expect("stdout utf8"),
                String::from_utf8(output.stderr.clone()).expect("stderr utf8"),
            )
        }

        /// Vérifie le `status` et renvoie `stdout` (panic sinon).
        pub fn expect_success_stdout(output: Output) -> String {
            if !output.status.success() {
                let (out, err) = Self::output_to_strings(&output);
                panic!(
                    "CLI exit code = {:?}\n--- stdout ---\n{}\n--- stderr ---\n{}",
                    output.status.code(),
                    out,
                    err
                );
            }
            Self::output_to_strings(&output).0
        }
    }

    /// Sugar : pack + run d’un projet (dossier avec `src/*.vit`) et renvoie `stdout`.
    pub fn pack_and_run(cli: &Cli, project_dir: &Path) -> String {
        let tmp = super::tmp::TempDir::new();
        let bc = tmp.path().join("out.vitbc");
        let pack_out = cli.pack(project_dir, &bc);
        let _ = Cli::expect_success_stdout(pack_out);
        let run_out = cli.run(&bc);
        Cli::expect_success_stdout(run_out)
    }
}

#[cfg(feature = "snapshots")]
pub mod snap {
    //! Helpers **insta** (snapshots) facultatifs.
    //!
    //! Ajoute à `tests/Cargo.toml` :
    //! ```toml
    //! [features]
    //! snapshots = []
    //!
    //! [dependencies]
    //! insta = { version = "1", features = ["yaml"] }
    //! ```
    use super::*;

    /// Ajoute un suffixe de snapshot basé sur le CI/plateforme
    /// (pratique si les sorties diffèrent entre OS).
    pub fn set_snapshot_suffix_from_ci() {
        let os = env::consts::OS;
        let arch = env::consts::ARCH;
        let ci = env::var("CI").ok().unwrap_or_else(|| "local".into());
        insta::with_settings!({ snapshot_suffix => format!("{}-{}-{}", os, arch, ci) }, {});
    }

    /// Assert snapshot sur une `&str` (avec normalisation simple).
    pub fn assert_snapshot_str(name: &str, content: &str) {
        let norm = normalize(content);
        insta::assert_snapshot!(name, norm);
    }

    fn normalize(s: &str) -> String {
        // Normalisations stables (chemins, timestamps ISO basiques)
        let mut out = String::with_capacity(s.len());
        for line in s.lines() {
            let line = line.replace('\\', "/");
            let line = line
                .split_whitespace()
                .map(|tok| if looks_like_iso8601(tok) { "<ts>" } else { tok })
                .collect::<Vec<_>>()
                .join(" ");
            out.push_str(&line);
            out.push('\n');
        }
        out
    }
    fn looks_like_iso8601(tok: &str) -> bool {
        tok.len() >= 19 && tok.as_bytes().get(4) == Some(&b'-') && tok.contains('T')
    }
}

#[cfg(test)]
mod _smoke {
    use super::*;
    #[test]
    fn repo_root_exists() {
        let root = repo_root();
        assert!(root.join("Cargo.toml").exists());
    }
}
