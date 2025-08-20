// vitte-doc/src/main.rs
//
// Un binaire “tout-en-un” pour bosser sur la doc :
//  - construit le mdBook (via `mdbook build`)
//  - sert le dossier généré (axum + tower-http)
//  - watch des sources et rebuild auto
//  - routes de redirection pratiques (/docs, /std, /rfcs)
//  - open browser, healthz, version, CORS, compression
//
// Dépendances (Cargo.toml):
// [dependencies]
// anyhow = "1"
// axum = { version = "0.7", features = ["macros"] }
// clap = { version = "4", features = ["derive"] }
// notify = "6"
// tokio = { version = "1", features = ["full"] }
// tower = "0.5"
// tower-http = { version = "0.5", features = ["fs", "compression-br", "compression-gzip", "cors", "trace"] }
// tracing = "0.1"
// tracing-subscriber = { version = "0.3", features = ["env-filter", "fmt"] }
// webbrowser = "0.8"
//
// Optionnel (Windows ANSI):
// nu-ansi-term = "0.50"

use anyhow::{anyhow, Context, Result};
use axum::{
    body::Body,
    extract::State,
    http::{HeaderValue, StatusCode, Uri},
    response::{Html, IntoResponse, Redirect, Response},
    routing::get,
    Router,
};
use clap::{ArgAction, Parser, Subcommand};
use notify::{Config as NotifyConfig, Event, EventKind, RecommendedWatcher, RecursiveMode, Watcher};
use std::{
    net::{IpAddr, Ipv4Addr, SocketAddr},
    path::{Path, PathBuf},
    process::Command,
    sync::{atomic::{AtomicBool, Ordering}, Arc},
    time::{Duration, Instant},
};
use tokio::sync::mpsc;
use tower::ServiceBuilder;
use tower_http::{
    compression::{CompressionLayer, predicate::SizeAbove},
    cors::CorsLayer,
    services::{ServeDir, ServeFile},
    trace::TraceLayer,
};
use tracing::{error, info, warn, Level};
use tracing_subscriber::{fmt, EnvFilter};

#[tokio::main]
async fn main() -> Result<()> {
    init_tracing();

    let cli = Cli::parse();
    match cli.cmd {
        Cmd::Build { project, book_dir, strict } => {
            let project = project.unwrap_or_else(|| PathBuf::from("."));
            mdbook_build(&project, strict).context("build mdBook")?;
            // Affiche l’emplacement
            let book_dir = book_dir.unwrap_or_else(|| project.join("book"));
            eprintln!("✅ book généré → {}", book_dir.display());
            Ok(())
        }
        Cmd::Serve { project, book_dir, host, port, open, nowatch, strict } => {
            let project = project.unwrap_or_else(|| PathBuf::from("."));
            let book_dir = book_dir.unwrap_or_else(|| project.join("book"));

            // Build initial (sauf si l’utilisateur l’a déjà fait et préfère tenter sa chance)
            mdbook_build(&project, strict).context("build initial")?;

            // Watch (optionnel)
            let stop_flag = Arc::new(AtomicBool::new(false));
            let (tx, mut rx) = mpsc::unbounded_channel::<()>();

            if !nowatch {
                let stop = stop_flag.clone();
                let project_clone = project.clone();
                tokio::task::spawn_blocking(move || {
                    if let Err(e) = watch_and_rebuild(&project_clone, stop, strict, tx) {
                        eprintln!("watch error: {e:?}");
                    }
                });
            }

            // Démarre le serveur
            let addr = SocketAddr::from((host, port));
            let server = spawn_server(book_dir.clone(), addr).await?;

            // Ouvre le navigateur
            if open {
                let _ = webbrowser::open(&format!("http://{}", addr));
            }

            // Boucle principale: on attend Ctrl+C, ou rebuild notifications (simple log)
            tokio::select! {
                _ = tokio::signal::ctrl_c() => {
                    info!("🔚 Ctrl+C – arrêt…");
                }
                // Log soft: on ne redémarre pas le serveur, on sert juste les fichiers rebuild
                _ = async {
                    while let Some(_) = rx.recv().await {
                        info!("♻️  Build terminé.");
                    }
                } => {}
            }

            // Stop watcher
            stop_flag.store(true, Ordering::SeqCst);

            // Arrêt serveur (grâce au handle)
            server.graceful_shutdown();
            Ok(())
        }
    }
}

/* =========================
   CLI
   ========================= */

#[derive(Parser, Debug)]
#[command(name = "vitte-doc", version, about = "Outils docs Vitte (mdBook + serveur)")]
struct Cli {
    #[command(subcommand)]
    cmd: Cmd,
}

#[derive(Subcommand, Debug)]
enum Cmd {
    /// Construit le mdBook (book/…)
    Build {
        /// Dossier projet doc (contenant book.toml et src/)
        #[arg(long)]
        project: Option<PathBuf>,
        /// Dossier de sortie du mdBook (défaut: <project>/book)
        #[arg(long)]
        book_dir: Option<PathBuf>,
        /// Echec si mdbook n’est pas présent
        #[arg(long, action=ArgAction::SetTrue)]
        strict: bool,
    },
    /// Sert le mdBook et (par défaut) rebuild à chaud
    Serve {
        /// Dossier projet doc (contenant book.toml et src/)
        #[arg(long)]
        project: Option<PathBuf>,
        /// Dossier de sortie du mdBook (défaut: <project>/book)
        #[arg(long)]
        book_dir: Option<PathBuf>,
        /// Adresse d’écoute (host)
        #[arg(long, default_value_t=IpAddr::V4(Ipv4Addr::LOCALHOST))]
        host: IpAddr,
        /// Port d’écoute
        #[arg(long, default_value_t=3000)]
        port: u16,
        /// Ouvre automatiquement le navigateur
        #[arg(long, action=ArgAction::SetTrue)]
        open: bool,
        /// Désactive le watcher (pas de rebuild auto)
        #[arg(long, action=ArgAction::SetTrue)]
        nowatch: bool,
        /// Echec si mdbook n’est pas présent
        #[arg(long, action=ArgAction::SetTrue)]
        strict: bool,
    },
}

/* =========================
   BUILD mdBOOK
   ========================= */

fn mdbook_build(project_dir: &Path, strict: bool) -> Result<()> {
    // Précondition: book.toml doit exister
    let book_toml = project_dir.join("book.toml");
    if !book_toml.exists() {
        return Err(anyhow!("book.toml introuvable dans {}", project_dir.display()));
    }
    // Vérifie présence binaire mdbook
    let mdbook_bin = which_mdbook();
    if mdbook_bin.is_none() {
        let msg = "mdbook non trouvé dans le PATH. Installez-le: `cargo install mdbook`.";
        if strict {
            return Err(anyhow!(msg));
        } else {
            warn!("{msg} (strict=off → build sauté)");
            return Ok(());
        }
    }
    let mdbook_bin = mdbook_bin.unwrap();

    info!("📚 mdbook build …");
    let status = Command::new(&mdbook_bin)
        .arg("build")
        .current_dir(project_dir)
        .status()
        .with_context(|| format!("échec lancement `{}`", mdbook_bin.display()))?;

    if !status.success() {
        return Err(anyhow!("`mdbook build` a échoué (code {:?})", status.code()));
    }
    Ok(())
}

fn which_mdbook() -> Option<PathBuf> {
    // Simple heuristique PATH
    std::env::var_os("PATH").and_then(|paths| {
        std::env::split_paths(&paths)
            .map(|p| p.join(if cfg!(windows) { "mdbook.exe" } else { "mdbook" }))
            .find(|c| c.exists())
    })
}

/* =========================
   WATCH & REBUILD
   ========================= */

fn watch_and_rebuild(
    project_dir: &Path,
    stop: Arc<AtomicBool>,
    strict: bool,
    tx: mpsc::UnboundedSender<()>,
) -> Result<()> {
    let src_dir = project_dir.join("src");
    let book_toml = project_dir.join("book.toml");
    if !src_dir.exists() {
        return Err(anyhow!("dossier src/ introuvable dans {}", project_dir.display()));
    }

    let (watch_tx, watch_rx) = std::sync::mpsc::channel::<Result<Event, notify::Error>>();
    let mut watcher: RecommendedWatcher =
        Watcher::new(watch_tx, NotifyConfig::default()).context("création watcher")?;
    watcher.watch(&src_dir, RecursiveMode::Recursive)?;
    watcher.watch(&book_toml, RecursiveMode::NonRecursive).ok();

    info!("👀 Watch sur {} …", src_dir.display());

    let mut last = Instant::now();
    let debounce = Duration::from_millis(200);

    loop {
        if stop.load(Ordering::SeqCst) {
            info!("🛑 Arrêt watcher");
            break;
        }
        match watch_rx.recv_timeout(Duration::from_millis(250)) {
            Ok(Ok(event)) => {
                if !is_relevant(&event.kind) {
                    continue;
                }
                // Debounce simple
                if last.elapsed() < debounce {
                    continue;
                }
                last = Instant::now();
                info!("✏️  Changement détecté → rebuild…");
                if let Err(e) = mdbook_build(project_dir, strict) {
                    error!("build erreur: {e:#}");
                } else {
                    let _ = tx.send(());
                }
            }
            Ok(Err(e)) => {
                warn!("watch erreur: {e:?}");
            }
            Err(std::sync::mpsc::RecvTimeoutError::Timeout) => {
                // loop continue
            }
            Err(e) => {
                warn!("watch canal fermé: {e:?}");
                break;
            }
        }
    }

    Ok(())
}

fn is_relevant(kind: &EventKind) -> bool {
    use EventKind::*;
    matches!(
        kind,
        Create(_)
            | Modify(_)
            | Remove(_)
            | Any
            | Access(_)
    )
}

/* =========================
   SERVEUR HTTP (AXUM)
   ========================= */

#[derive(Clone)]
struct AppState {
    book_dir: PathBuf,
}

async fn spawn_server(book_dir: PathBuf, addr: SocketAddr) -> Result<ServerHandle> {
    let state = AppState { book_dir: book_dir.clone() };

    // Service de fichiers statiques (book/)
    let file_service = ServeDir::new(book_dir.clone())
        .append_index_html_on_directories(true)
        .precompressed_br()
        .precompressed_gzip();

    // Fallback SPA vers index.html si fichier introuvable
    let fallback = ServeFile::new(book_dir.join("index.html"));

    let router = Router::new()
        .route("/", get(root))
        .route("/healthz", get(|| async { "ok" }))
        .route("/version", get(version))
        // Redirections pratiques
        .route("/docs", get(|| async {
            Redirect::to("https://vitte-lang.github.io/vitte/")
        }))
        .route("/std", get(|| async {
            Redirect::to("https://vitte-lang.github.io/vitte/std/")
        }))
        .route("/rfcs", get(|| async {
            Redirect::to("https://vitte-lang.github.io/vitte/rfc/")
        }))
        // “Catch-all” : on sert tout le reste depuis book/
        .fallback_service(file_service.fallback(fallback))
        .with_state(state)
        .layer(
            ServiceBuilder::new()
                .layer(TraceLayer::new_for_http())
                .layer(
                    CompressionLayer::new()
                        // On compresse à partir d’une taille raisonnable
                        .compress_when(SizeAbove::new(256))
                )
                .layer(CorsLayer::permissive())
        );

    info!("🌐 Serveur sur http://{addr} (Ctrl+C pour quitter)");

    let (tx, rx) = tokio::sync::oneshot::channel::<()>();
    let app = router.into_make_service();
    let server = axum::Server::bind(&addr).serve(app);

    let graceful = server.with_graceful_shutdown(async {
        let _ = rx.await;
    });

    let join = tokio::spawn(async move {
        if let Err(e) = graceful.await {
            error!("server error: {e}");
        }
    });

    Ok(ServerHandle { stop: Some(tx), join: Some(join) })
}

async fn root(State(state): State<AppState>) -> Response {
    // Sert /index.html explicitement (utile si on veut ajouter des headers)
    let index = state.book_dir.join("index.html");
    match tokio::fs::read(&index).await {
        Ok(bytes) => {
            let mut resp = Response::new(Body::from(bytes));
            resp.headers_mut().insert(
                axum::http::header::CONTENT_TYPE,
                HeaderValue::from_static("text/html; charset=utf-8"),
            );
            resp
        }
        Err(_) => (StatusCode::NOT_FOUND, "index introuvable").into_response(),
    }
}

async fn version() -> Html<String> {
    Html(format!(
        "<pre>vitte-doc {}\nRustc: {}\n</pre>",
        env!("CARGO_PKG_VERSION"),
        rustc_version_runtime()
    ))
}

fn rustc_version_runtime() -> String {
    // Meilleur effort: essaye rustc --version (optionnel)
    if let Ok(out) = Command::new("rustc").arg("--version").output() {
        if out.status.success() {
            if let Ok(s) = String::from_utf8(out.stdout) {
                return s.trim().to_string();
            }
        }
    }
    "unknown".into()
}

/* =========================
   SERVER HANDLE
   ========================= */

struct ServerHandle {
    stop: Option<tokio::sync::oneshot::Sender<()>>,
    join: Option<tokio::task::JoinHandle<()>>,
}

impl ServerHandle {
    fn graceful_shutdown(mut self) {
        if let Some(tx) = self.stop.take() {
            let _ = tx.send(());
        }
    }
}

impl Drop for ServerHandle {
    fn drop(&mut self) {
        if let Some(tx) = self.stop.take() {
            let _ = tx.send(());
        }
        if let Some(j) = self.join.take() {
            // Laisse le task se terminer tranquillement
            // (pas de blocage sync ici)
            let _ = j.abort();
        }
    }
}

/* =========================
   TRACING
   ========================= */

fn init_tracing() {
    let env_filter = EnvFilter::try_from_default_env()
        .unwrap_or_else(|_| EnvFilter::new("info,tower_http=info,axum=info"));
    fmt()
        .with_env_filter(env_filter)
        .with_target(false)
        .with_level(true)
        .with_max_level(Level::INFO)
        .init();
}
