//! vitte-cli — bibliothèque interne du binaire `vitte`
//!
//! But : fournir une API **propre, testable et réutilisable** pour le CLI
//! sans mélanger la logique d'E/S et le parsing d'arguments (laisse ça à `main.rs`).
//!
//! Points clés :
//! - Pipeline générique **compile → (optionnel) compresse → (optionnel) exécute**
//! - Callbacks/hook pour brancher ton compilateur, ton désassembleur, ton formateur, ta VM
//! - Utilitaires d'E/S (stdin/stdout, écriture atomique, création dossiers, chrono)
//! - Traces (`feature = "trace"`) et couleurs (`feature = "color"`) optionnelles
//! - Zéro dépendance forte sur l'impl interne des crates Vitte (pas d'API fantôme)

#![deny(unused_must_use)]
#![forbid(unsafe_code)]

use similar::{ChangeTag, TextDiff};
use std::{
    fs,
    fs::File,
    io::{self, BufReader, BufWriter, Read, Write},
    path::{Path, PathBuf},
    time::Instant,
};
use tempfile::NamedTempFile;

use anyhow::{anyhow, Context, Result};
use vitte_core::SourceId;
use vitte_lexer::{Lexer, LineMap, TokenKind};
use vitte_parser::Parser;

pub mod context {
    use anyhow::{Context, Result};
    use serde::Deserialize;
    use std::{
        collections::BTreeMap,
        fs,
        path::{Path, PathBuf},
    };
    use toml::Value;

    #[derive(Debug, Clone, Copy, PartialEq, Eq)]
    pub enum Profile {
        Dev,
        Release,
    }

    impl Profile {
        pub fn as_str(&self) -> &'static str {
            match self {
                Profile::Dev => "dev",
                Profile::Release => "release",
            }
        }
    }

    impl Default for Profile {
        fn default() -> Self {
            Profile::Dev
        }
    }

    #[derive(Debug, Clone)]
    pub struct ProfileConfig {
        pub profile: Profile,
        pub optimize: bool,
        pub emit_debug: bool,
        pub diagnostics: bool,
        pub cache: bool,
    }

    impl ProfileConfig {
        pub fn defaults(profile: Profile) -> Self {
            match profile {
                Profile::Dev => Self {
                    profile,
                    optimize: false,
                    emit_debug: true,
                    diagnostics: true,
                    cache: false,
                },
                Profile::Release => Self {
                    profile,
                    optimize: true,
                    emit_debug: false,
                    diagnostics: false,
                    cache: true,
                },
            }
        }

        pub fn apply_overrides(&mut self, overrides: &ProfileOverrides) {
            if let Some(optimize) = overrides.optimize {
                self.optimize = optimize;
            }
            if let Some(emit_debug) = overrides.emit_debug {
                self.emit_debug = emit_debug;
            }
            if let Some(diagnostics) = overrides.diagnostics {
                self.diagnostics = diagnostics;
            }
            if let Some(cache) = overrides.cache {
                self.cache = cache;
            }
        }
    }

    impl Default for ProfileConfig {
        fn default() -> Self {
            Self::defaults(Profile::Dev)
        }
    }

    #[derive(Debug, Clone, Default, Deserialize)]
    pub struct ProfileOverrides {
        #[serde(default)]
        pub optimize: Option<bool>,
        #[serde(default)]
        pub emit_debug: Option<bool>,
        #[serde(default)]
        pub diagnostics: Option<bool>,
        #[serde(default)]
        pub cache: Option<bool>,
    }

    #[derive(Debug, Default, Deserialize)]
    struct RawProfileConfig {
        #[serde(default)]
        profiles: ProfilesSection,
        #[serde(flatten)]
        _rest: BTreeMap<String, Value>,
    }

    #[derive(Debug, Default, Deserialize)]
    struct ProfilesSection {
        #[serde(default)]
        dev: Option<ProfileOverrides>,
        #[serde(default)]
        release: Option<ProfileOverrides>,
        #[serde(flatten)]
        _custom: BTreeMap<String, Value>,
    }

    fn config_path(root: Option<&Path>) -> PathBuf {
        match root {
            Some(dir) => dir.join("vitte.toml"),
            None => PathBuf::from("vitte.toml"),
        }
    }

    fn read_overrides(profile: Profile, root: Option<&Path>) -> Result<Option<ProfileOverrides>> {
        let path = config_path(root);
        let contents = match fs::read_to_string(&path) {
            Ok(data) => data,
            Err(err) if err.kind() == std::io::ErrorKind::NotFound => return Ok(None),
            Err(err) => return Err(err).with_context(|| format!("lecture de {}", path.display())),
        };
        let raw: RawProfileConfig =
            toml::from_str(&contents).with_context(|| format!("parse de {}", path.display()))?;
        let overrides = match profile {
            Profile::Dev => raw.profiles.dev,
            Profile::Release => raw.profiles.release,
        };
        Ok(overrides)
    }

    pub fn load_profile_config(profile: Profile, root: Option<&Path>) -> Result<ProfileConfig> {
        let mut config = ProfileConfig::defaults(profile);
        if let Some(overrides) = read_overrides(profile, root)? {
            config.apply_overrides(&overrides);
        }
        Ok(config)
    }
}

#[cfg(feature = "server")]
pub mod server {
    use super::{context, execute, Command, CompileTask, Hooks, Input, InputKind, Output, RunTask};
    use anyhow::{anyhow, Context, Result};
    use serde::{Deserialize, Serialize};
    use serde_json::{json, Value};
    use std::{
        net::SocketAddr,
        path::{Path, PathBuf},
        sync::Arc,
    };
    use tokio::io::{AsyncBufReadExt, AsyncWriteExt, BufReader};
    use tokio::net::{tcp::OwnedWriteHalf, TcpListener, TcpStream};

    pub struct ServerOptions {
        pub addr: SocketAddr,
        pub auth_token: Option<String>,
        pub workspace_root: Option<PathBuf>,
        pub default_profile: context::ProfileConfig,
    }

    pub async fn run(options: ServerOptions, hooks: Hooks) -> Result<()> {
        let state = Arc::new(ServerState {
            hooks,
            auth_token: options.auth_token,
            workspace_root: options.workspace_root,
            default_profile: options.default_profile,
        });

        let listener = TcpListener::bind(options.addr)
            .await
            .with_context(|| format!("impossible d'écouter sur {}", options.addr))?;

        loop {
            let (stream, addr) = listener.accept().await?;
            let state = Arc::clone(&state);
            tokio::spawn(async move {
                if let Err(err) = handle_client(state, stream).await {
                    eprintln!("[rpc] connexion {} fermée: {:#}", addr, err);
                }
            });
        }
    }

    struct ServerState {
        hooks: Hooks,
        auth_token: Option<String>,
        workspace_root: Option<PathBuf>,
        default_profile: context::ProfileConfig,
    }

    #[derive(Deserialize)]
    struct RpcRequest {
        jsonrpc: String,
        method: String,
        #[serde(default)]
        params: Value,
        #[serde(default)]
        id: Option<Value>,
    }

    #[derive(Serialize)]
    struct RpcResponse {
        jsonrpc: &'static str,
        #[serde(skip_serializing_if = "Option::is_none")]
        result: Option<Value>,
        #[serde(skip_serializing_if = "Option::is_none")]
        error: Option<RpcError>,
        #[serde(skip_serializing_if = "Option::is_none")]
        id: Option<Value>,
    }

    #[derive(Serialize)]
    struct RpcError {
        code: i32,
        message: String,
        #[serde(skip_serializing_if = "Option::is_none")]
        data: Option<Value>,
    }

    async fn handle_client(state: Arc<ServerState>, stream: TcpStream) -> Result<()> {
        let (read_half, mut write_half) = stream.into_split();
        let mut reader = BufReader::new(read_half);
        let mut line = String::new();
        let mut authenticated = state.auth_token.is_none();

        send_notification(
            &mut write_half,
            "server/ready",
            json!({ "authenticated": authenticated }),
        )
        .await?;

        loop {
            line.clear();
            let read = reader.read_line(&mut line).await?;
            if read == 0 {
                break;
            }
            let payload = line.trim();
            if payload.is_empty() {
                continue;
            }

            let request: RpcRequest = match serde_json::from_str(payload) {
                Ok(req) => req,
                Err(err) => {
                    send_error(&mut write_half, None, -32700, format!("JSON invalide: {err}"))
                        .await?;
                    continue;
                }
            };

            if request.jsonrpc != "2.0" {
                send_error(
                    &mut write_half,
                    request.id.clone(),
                    -32600,
                    "version JSON-RPC invalide".into(),
                )
                .await?;
                continue;
            }

            if request.method == "auth" {
                if state.auth_token.is_none() {
                    send_error(&mut write_half, request.id.clone(), -32000, "auth inutile".into())
                        .await?;
                    continue;
                }
                match request.params.get("token").and_then(Value::as_str) {
                    Some(token) if Some(token) == state.auth_token.as_deref() => {
                        authenticated = true;
                        send_response(&mut write_half, request.id.clone(), json!({ "ok": true }))
                            .await?;
                    }
                    _ => {
                        send_error(
                            &mut write_half,
                            request.id.clone(),
                            -32000,
                            "token invalide".into(),
                        )
                        .await?;
                    }
                }
                continue;
            }

            if !authenticated {
                send_error(&mut write_half, request.id.clone(), -32001, "auth requise".into())
                    .await?;
                continue;
            }

            let result = match request.method.as_str() {
                "ping" => Ok(json!({ "pong": true })),
                "compile" => {
                    let params: CompileParams = deserialize_params(&request.params)?;
                    handle_compile(&state, &mut write_half, params).await
                }
                "run" => {
                    let params: RunParams = deserialize_params(&request.params)?;
                    handle_run(&state, &mut write_half, params).await
                }
                "fmt" => {
                    let params: FmtParams = deserialize_params(&request.params)?;
                    handle_fmt(&state, params).await
                }
                "shutdown" => {
                    send_response(&mut write_half, request.id.clone(), json!({ "ok": true }))
                        .await?;
                    break;
                }
                other => Err(anyhow!("méthode inconnue: {other}")),
            };

            if let Some(id) = request.id {
                match result {
                    Ok(value) => send_response(&mut write_half, Some(id), value).await?,
                    Err(err) => {
                        send_error(&mut write_half, Some(id), -32002, err.to_string()).await?;
                    }
                }
            }
        }

        Ok(())
    }

    async fn handle_compile(
        state: &ServerState,
        writer: &mut OwnedWriteHalf,
        params: CompileParams,
    ) -> Result<Value> {
        let input_path = resolve_path(state.workspace_root.as_deref(), &params.input)?;
        let input_repr = input_path.display().to_string();
        let output_path = params
            .output
            .as_ref()
            .map(|p| resolve_path(state.workspace_root.as_deref(), p))
            .transpose()?;

        let mut profile_cfg = resolve_profile(state, params.profile.as_deref())?;
        let optimize = params.optimize.unwrap_or(profile_cfg.optimize);
        let emit_debug = params.emit_debug.unwrap_or(profile_cfg.emit_debug);
        profile_cfg.optimize = optimize;
        profile_cfg.emit_debug = emit_debug;

        let task = CompileTask {
            input: Input::Path(input_path.clone()),
            output: match output_path {
                Some(path) => Output::Path(path),
                None => Output::Auto,
            },
            optimize,
            emit_debug,
            auto_mkdir: params.auto_mkdir.unwrap_or(false),
            overwrite: params.overwrite.unwrap_or(false),
            time: params.time.unwrap_or(false),
            profile: profile_cfg.clone(),
        };

        send_notification(
            writer,
            "build/progress",
            json!({ "command": "compile", "stage": "started", "path": input_repr }),
        )
        .await?;

        let exit_code = execute_command(Command::Compile(task), state.hooks.clone()).await?;

        send_notification(
            writer,
            "build/progress",
            json!({ "command": "compile", "stage": "finished", "exit_code": exit_code }),
        )
        .await?;

        Ok(json!({ "exit_code": exit_code }))
    }

    async fn handle_run(
        state: &ServerState,
        writer: &mut OwnedWriteHalf,
        params: RunParams,
    ) -> Result<Value> {
        let program_path = resolve_path(state.workspace_root.as_deref(), &params.program)?;
        let program_repr = program_path.display().to_string();
        let input_kind = if params.auto_compile.unwrap_or(false) {
            InputKind::SourcePath(program_path.clone())
        } else {
            InputKind::BytecodePath(program_path.clone())
        };

        let mut profile_cfg = resolve_profile(state, params.profile.as_deref())?;
        let optimize = params.optimize.unwrap_or(profile_cfg.optimize);
        profile_cfg.optimize = optimize;

        let task = RunTask {
            program: input_kind,
            args: params.args.unwrap_or_default(),
            auto_compile: params.auto_compile.unwrap_or(false),
            optimize,
            time: params.time.unwrap_or(false),
            profile: profile_cfg.clone(),
        };

        send_notification(
            writer,
            "build/progress",
            json!({ "command": "run", "stage": "started", "program": program_repr }),
        )
        .await?;

        let exit_code = execute_command(Command::Run(task), state.hooks.clone()).await?;

        send_notification(
            writer,
            "build/progress",
            json!({ "command": "run", "stage": "finished", "exit_code": exit_code }),
        )
        .await?;

        Ok(json!({ "exit_code": exit_code }))
    }

    async fn handle_fmt(state: &ServerState, params: FmtParams) -> Result<Value> {
        let input_path = resolve_path(state.workspace_root.as_deref(), &params.input)?;
        let hooks = state.hooks.clone();
        let content = tokio::fs::read_to_string(&input_path)
            .await
            .with_context(|| format!("lecture de {}", input_path.display()))?;

        let formatted = tokio::task::spawn_blocking(move || {
            let fmt_fn = hooks.fmt.ok_or_else(|| {
                anyhow!("hook de formatage indisponible — activez la feature `fmt`")
            })?;
            fmt_fn(&content, false)
        })
        .await??;

        Ok(json!({ "formatted": formatted }))
    }

    async fn execute_command(cmd: Command, hooks: Hooks) -> Result<i32> {
        let result = tokio::task::spawn_blocking(move || execute(cmd, &hooks))
            .await
            .context("commande server bloquée")?;
        result
    }

    fn resolve_path(root: Option<&Path>, raw: &str) -> Result<PathBuf> {
        let path = PathBuf::from(raw);
        if path.is_absolute() || root.is_none() {
            Ok(path)
        } else {
            Ok(root.unwrap().join(path))
        }
    }

    fn resolve_profile(
        state: &ServerState,
        profile: Option<&str>,
    ) -> Result<context::ProfileConfig> {
        if let Some(name) = profile {
            let profile = match name {
                "dev" => context::Profile::Dev,
                "release" => context::Profile::Release,
                other => return Err(anyhow!(format!("profil inconnu: {other}"))),
            };
            context::load_profile_config(profile, state.workspace_root.as_deref())
        } else {
            Ok(state.default_profile.clone())
        }
    }

    async fn send_response(
        writer: &mut OwnedWriteHalf,
        id: Option<Value>,
        result: Value,
    ) -> Result<()> {
        let resp = RpcResponse { jsonrpc: "2.0", result: Some(result), error: None, id };
        let payload = serde_json::to_vec(&resp)?;
        writer.write_all(&payload).await?;
        writer.write_all(b"\n").await?;
        writer.flush().await?;
        Ok(())
    }

    async fn send_error(
        writer: &mut OwnedWriteHalf,
        id: Option<Value>,
        code: i32,
        message: String,
    ) -> Result<()> {
        let resp = RpcResponse {
            jsonrpc: "2.0",
            result: None,
            error: Some(RpcError { code, message, data: None }),
            id,
        };
        let payload = serde_json::to_vec(&resp)?;
        writer.write_all(&payload).await?;
        writer.write_all(b"\n").await?;
        writer.flush().await?;
        Ok(())
    }

    async fn send_notification(
        writer: &mut OwnedWriteHalf,
        method: &str,
        params: Value,
    ) -> Result<()> {
        let notif = json!({
            "jsonrpc": "2.0",
            "method": method,
            "params": params,
        });
        let payload = serde_json::to_vec(&notif)?;
        writer.write_all(&payload).await?;
        writer.write_all(b"\n").await?;
        writer.flush().await?;
        Ok(())
    }

    fn deserialize_params<T: for<'de> Deserialize<'de>>(value: &Value) -> Result<T> {
        serde_json::from_value(value.clone()).map_err(|err| anyhow!(err))
    }

    #[derive(Deserialize)]
    struct CompileParams {
        input: String,
        #[serde(default)]
        output: Option<String>,
        #[serde(default)]
        optimize: Option<bool>,
        #[serde(default)]
        emit_debug: Option<bool>,
        #[serde(default)]
        profile: Option<String>,
        #[serde(default)]
        auto_mkdir: Option<bool>,
        #[serde(default)]
        overwrite: Option<bool>,
        #[serde(default)]
        time: Option<bool>,
    }

    #[derive(Deserialize)]
    struct RunParams {
        program: String,
        #[serde(default)]
        args: Option<Vec<String>>,
        #[serde(default)]
        auto_compile: Option<bool>,
        #[serde(default)]
        optimize: Option<bool>,
        #[serde(default)]
        profile: Option<String>,
        #[serde(default)]
        time: Option<bool>,
    }

    #[derive(Deserialize)]
    struct FmtParams {
        input: String,
    }
}

pub mod i18n {
    use std::collections::HashMap;
    use std::sync::{OnceLock, RwLock};

    #[derive(Debug, Clone, Copy, PartialEq, Eq)]
    pub enum Lang {
        En,
        Fr,
    }

    impl Lang {
        pub fn from_code(code: &str) -> Option<Self> {
            match code.to_ascii_lowercase().as_str() {
                "en" | "en-us" | "en_gb" => Some(Lang::En),
                "fr" | "fr-fr" | "fr_ca" => Some(Lang::Fr),
                _ => None,
            }
        }

        pub fn code(self) -> &'static str {
            match self {
                Lang::En => "en",
                Lang::Fr => "fr",
            }
        }
    }

    fn lang_lock() -> &'static RwLock<Lang> {
        static LANG: OnceLock<RwLock<Lang>> = OnceLock::new();
        LANG.get_or_init(|| RwLock::new(Lang::En))
    }

    pub fn init(lang: Lang) {
        if let Ok(mut guard) = lang_lock().write() {
            *guard = lang;
        }
    }

    pub fn current_lang() -> Lang {
        *lang_lock().read().expect("lang lock poisoned")
    }

    fn parse_catalog(input: &str) -> HashMap<String, String> {
        let mut map = HashMap::new();
        for line in input.lines() {
            let line = line.trim();
            if line.is_empty() || line.starts_with('#') {
                continue;
            }
            if let Some(idx) = line.find('=') {
                let key = line[..idx].trim();
                let value = line[idx + 1..].trim();
                map.insert(key.to_string(), value.to_string());
            }
        }
        map
    }

    fn catalog(lang: Lang) -> &'static HashMap<String, String> {
        match lang {
            Lang::En => static_catalog_en(),
            Lang::Fr => static_catalog_fr(),
        }
    }

    fn static_catalog_en() -> &'static HashMap<String, String> {
        static EN: OnceLock<HashMap<String, String>> = OnceLock::new();
        EN.get_or_init(|| parse_catalog(include_str!("i18n/en.ftl")))
    }

    fn static_catalog_fr() -> &'static HashMap<String, String> {
        static FR: OnceLock<HashMap<String, String>> = OnceLock::new();
        FR.get_or_init(|| parse_catalog(include_str!("i18n/fr.ftl")))
    }

    fn lookup<'a>(lang: Lang, key: &str) -> Option<&'a str> {
        catalog(lang).get(key).map(|s| s.as_str())
    }

    fn render(template: &str, args: &[(&str, &str)]) -> String {
        let mut out = template.to_string();
        for (key, value) in args {
            let placeholder = format!("{{{key}}}");
            out = out.replace(&placeholder, value);
        }
        out
    }

    fn translate(lang: Lang, key: &str, args: &[(&str, &str)]) -> String {
        if let Some(text) = lookup(lang, key) {
            return render(text, args);
        }
        if let Some(text) = lookup(Lang::En, key) {
            return render(text, args);
        }
        render(key, args)
    }

    pub fn tr(key: &str, args: &[(&str, &str)]) -> String {
        translate(current_lang(), key, args)
    }

    pub fn plural(base_key: &str, count: usize, extra: &[(&str, &str)]) -> String {
        let lang = current_lang();
        let key = match count {
            0 => format!("{base_key}.zero"),
            1 => format!("{base_key}.one"),
            _ => format!("{base_key}.other"),
        };
        let count_str = count.to_string();
        let mut vars: Vec<(&str, &str)> = Vec::with_capacity(extra.len() + 1);
        vars.push(("count", &count_str));
        for (k, v) in extra {
            vars.push((k, v));
        }
        translate(lang, &key, &vars)
    }
}

pub mod doctor {
    use super::i18n;
    use anyhow::{Context, Result};
    use semver::Version;
    use serde::Serialize;
    use std::fs::{self, OpenOptions};
    use std::io::{self, Write};
    use std::path::Path;
    use std::process::Command as ProcessCommand;

    const MIN_RUSTC_VERSION: &str = "1.70.0";

    #[derive(Debug, Clone, Serialize)]
    pub struct DoctorReport {
        pub workspace_root: String,
        pub summary: DoctorSummary,
        pub checks: Vec<CheckResult>,
        pub suggestions: Vec<String>,
    }

    impl DoctorReport {
        pub fn has_errors(&self) -> bool {
            self.summary.errors > 0
        }
    }

    #[derive(Debug, Clone, Serialize)]
    pub struct DoctorSummary {
        pub errors: usize,
        pub warnings: usize,
    }

    #[derive(Debug, Clone, Serialize)]
    pub struct CheckResult {
        pub name: String,
        pub status: CheckStatus,
        pub details: String,
        #[serde(skip_serializing_if = "Option::is_none")]
        pub recommendation: Option<String>,
    }

    impl CheckResult {
        fn ok(name: impl Into<String>, details: impl Into<String>) -> Self {
            Self {
                name: name.into(),
                status: CheckStatus::Ok,
                details: details.into(),
                recommendation: None,
            }
        }

        fn warn(
            name: impl Into<String>,
            details: impl Into<String>,
            recommendation: impl Into<String>,
        ) -> Self {
            Self {
                name: name.into(),
                status: CheckStatus::Warn,
                details: details.into(),
                recommendation: Some(recommendation.into()),
            }
        }

        fn error(
            name: impl Into<String>,
            details: impl Into<String>,
            recommendation: impl Into<String>,
        ) -> Self {
            Self {
                name: name.into(),
                status: CheckStatus::Error,
                details: details.into(),
                recommendation: Some(recommendation.into()),
            }
        }
    }

    #[derive(Debug, Clone, Copy, Serialize, PartialEq, Eq)]
    #[serde(rename_all = "lowercase")]
    pub enum CheckStatus {
        Ok,
        Warn,
        Error,
    }

    impl std::fmt::Display for CheckStatus {
        fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
            match self {
                CheckStatus::Ok => write!(f, "OK"),
                CheckStatus::Warn => write!(f, "WARN"),
                CheckStatus::Error => write!(f, "ERR"),
            }
        }
    }

    pub fn run(root: &Path) -> DoctorReport {
        let mut checks = Vec::new();
        let mut suggestions = Vec::new();

        let rustc = check_rustc();
        if let Some(rec) = rustc.recommendation.clone() {
            if rustc.status != CheckStatus::Ok {
                suggestions.push(rec.clone());
            }
        }
        checks.push(rustc);

        let cargo = check_cargo();
        if let Some(rec) = cargo.recommendation.clone() {
            if cargo.status != CheckStatus::Ok {
                suggestions.push(rec.clone());
            }
        }
        checks.push(cargo);

        let cache = check_cache_dir(root);
        if let Some(rec) = cache.recommendation.clone() {
            if cache.status != CheckStatus::Ok {
                suggestions.push(rec.clone());
            }
        }
        checks.push(cache);

        let modules = check_modules_dir(root);
        if let Some(rec) = modules.recommendation.clone() {
            if modules.status != CheckStatus::Ok {
                suggestions.push(rec.clone());
            }
        }
        checks.push(modules);

        let errors = checks.iter().filter(|c| matches!(c.status, CheckStatus::Error)).count();
        let warnings = checks.iter().filter(|c| matches!(c.status, CheckStatus::Warn)).count();

        DoctorReport {
            workspace_root: root.display().to_string(),
            summary: DoctorSummary { errors, warnings },
            checks,
            suggestions,
        }
    }

    fn check_rustc() -> CheckResult {
        let name = i18n::tr("doctor.check.rustc.name", &[]);
        let output = match ProcessCommand::new("rustc").arg("--version").output() {
            Ok(out) => out,
            Err(err) => {
                let err_text = err.to_string();
                let details = i18n::tr(
                    "doctor.check.rustc.not_found",
                    [("error", err_text.as_str())].as_ref(),
                );
                let recommendation = i18n::tr("doctor.check.rustc.recommend.install", &[]);
                return CheckResult::error(name, details, recommendation);
            }
        };

        if !output.status.success() {
            let code = output.status.code().map(|c| c.to_string()).unwrap_or_else(|| "?".into());
            return CheckResult::error(
                name,
                i18n::tr("doctor.check.rustc.exec_failed", [("code", code.as_str())].as_ref()),
                i18n::tr("doctor.check.rustc.recommend.verify", &[]),
            );
        }

        let version_line = String::from_utf8_lossy(&output.stdout);
        let detected = version_line.trim().to_string();
        let detected_version = detected.split_whitespace().nth(1).unwrap_or_default().to_string();

        match Version::parse(&detected_version) {
            Ok(parsed) => {
                let min = Version::parse(MIN_RUSTC_VERSION).expect("valid min version");
                if parsed < min {
                    CheckResult::warn(
                        name,
                        i18n::tr(
                            "doctor.check.rustc.outdated",
                            [
                                ("version", detected_version.as_str()),
                                ("minimum", MIN_RUSTC_VERSION),
                            ]
                            .as_ref(),
                        ),
                        i18n::tr("doctor.check.rustc.recommend.update", &[]),
                    )
                } else {
                    CheckResult::ok(
                        name,
                        i18n::tr(
                            "doctor.check.rustc.ok",
                            [("version", detected_version.as_str())].as_ref(),
                        ),
                    )
                }
            }
            Err(_) => CheckResult::warn(
                name,
                i18n::tr(
                    "doctor.check.rustc.parse_error",
                    [("output", detected.as_str())].as_ref(),
                ),
                i18n::tr("doctor.check.rustc.recommend.verify", &[]),
            ),
        }
    }

    fn check_cargo() -> CheckResult {
        let name = i18n::tr("doctor.check.cargo.name", &[]);
        match ProcessCommand::new("cargo").arg("--version").output() {
            Ok(out) if out.status.success() => {
                let version = String::from_utf8_lossy(&out.stdout).trim().to_string();
                CheckResult::ok(
                    name,
                    i18n::tr("doctor.check.cargo.ok", [("version", version.as_str())].as_ref()),
                )
            }
            Ok(out) => CheckResult::error(
                name,
                {
                    let code_str =
                        out.status.code().map(|c| c.to_string()).unwrap_or_else(|| "?".into());
                    i18n::tr(
                        "doctor.check.cargo.exec_failed",
                        [("code", code_str.as_str())].as_ref(),
                    )
                },
                i18n::tr("doctor.check.cargo.recommend.verify", &[]),
            ),
            Err(err) => CheckResult::error(
                name,
                {
                    let err_text = err.to_string();
                    i18n::tr(
                        "doctor.check.cargo.not_found",
                        [("error", err_text.as_str())].as_ref(),
                    )
                },
                i18n::tr("doctor.check.cargo.recommend.install", &[]),
            ),
        }
    }

    fn check_cache_dir(root: &Path) -> CheckResult {
        let name = i18n::tr("doctor.check.cache.name", &[]);
        let target_dir = root.join("target");
        if !target_dir.exists() {
            let path_text = target_dir.display().to_string();
            return CheckResult::warn(
                name,
                i18n::tr("doctor.check.cache.missing", [("path", path_text.as_str())].as_ref()),
                i18n::tr("doctor.check.cache.recommend.build", &[]),
            );
        }

        let probe = target_dir.join(".doctor_probe");
        match OpenOptions::new().create(true).write(true).truncate(true).open(&probe) {
            Ok(mut file) => {
                let path_text = target_dir.display().to_string();
                if let Err(err) = file.write_all(b"ok") {
                    let _ = fs::remove_file(&probe);
                    let err_text = err.to_string();
                    return CheckResult::warn(
                        name,
                        i18n::tr(
                            "doctor.check.cache.write_fail",
                            [("path", path_text.as_str()), ("error", err_text.as_str())].as_ref(),
                        ),
                        i18n::tr("doctor.check.cache.recommend.perms", &[]),
                    );
                }
                let _ = fs::remove_file(&probe);
                CheckResult::ok(
                    name,
                    i18n::tr("doctor.check.cache.ok", [("path", path_text.as_str())].as_ref()),
                )
            }
            Err(err) => CheckResult::warn(
                name,
                {
                    let path_text = target_dir.display().to_string();
                    let err_text = err.to_string();
                    i18n::tr(
                        "doctor.check.cache.open_fail",
                        [("path", path_text.as_str()), ("error", err_text.as_str())].as_ref(),
                    )
                },
                i18n::tr("doctor.check.cache.recommend.perms", &[]),
            ),
        }
    }

    fn check_modules_dir(root: &Path) -> CheckResult {
        let name = i18n::tr("doctor.check.modules.name", &[]);
        let modules_dir = root.join("modules").join("vitte-modules");
        if modules_dir.exists() {
            let path_text = modules_dir.display().to_string();
            CheckResult::ok(
                name,
                i18n::tr("doctor.check.modules.ok", [("path", path_text.as_str())].as_ref()),
            )
        } else {
            let path_text = modules_dir.display().to_string();
            CheckResult::warn(
                name,
                i18n::tr("doctor.check.modules.missing", [("path", path_text.as_str())].as_ref()),
                i18n::tr("doctor.check.modules.recommend.init", &[]),
            )
        }
    }

    pub fn print_report(report: &DoctorReport) {
        let errors_label = i18n::plural("doctor.count.errors", report.summary.errors, &[]);
        let warnings_label = i18n::plural("doctor.count.warnings", report.summary.warnings, &[]);
        let header = i18n::tr(
            "doctor.report.header",
            &[
                ("workspace", report.workspace_root.as_str()),
                ("errors_label", errors_label.as_str()),
                ("warnings_label", warnings_label.as_str()),
            ],
        );
        println!("{}", header);
        for check in &report.checks {
            let status_tag = match check.status {
                CheckStatus::Ok => i18n::tr("doctor.status.ok", &[]),
                CheckStatus::Warn => i18n::tr("doctor.status.warn", &[]),
                CheckStatus::Error => i18n::tr("doctor.status.error", &[]),
            };
            println!("  {} {}", status_tag, check.name);
            println!("        {}", check.details);
            if let Some(rec) = &check.recommendation {
                println!(
                    "        {}",
                    i18n::tr("doctor.suggestion.inline", [("text", rec.as_str())].as_ref())
                );
            }
        }
        if !report.suggestions.is_empty() {
            println!("\n{}", i18n::tr("doctor.suggestions.title", &[]));
            for (idx, suggestion) in report.suggestions.iter().enumerate() {
                let idx_str = (idx + 1).to_string();
                println!(
                    "  {}",
                    i18n::tr(
                        "doctor.suggestions.item",
                        [("index", idx_str.as_str()), ("text", suggestion.as_str())].as_ref()
                    )
                );
            }
        }
    }

    pub fn purge_cache(root: &Path) -> Result<()> {
        let target = root.join("target");
        if !target.exists() {
            return Ok(());
        }
        fs::remove_dir_all(&target)
            .with_context(|| format!("suppression de {}", target.display()))?;
        Ok(())
    }

    pub fn ask_confirmation(prompt: &str) -> Result<bool> {
        let suffix = match i18n::current_lang() {
            i18n::Lang::Fr => " [o/N] ",
            i18n::Lang::En => " [y/N] ",
        };
        print!("{}{suffix}", prompt);
        io::stdout().flush()?;
        let mut line = String::new();
        io::stdin().read_line(&mut line)?;
        let response = line.trim().to_ascii_lowercase();
        let positive = matches!(response.as_str(), "y" | "yes" | "o" | "oui");
        Ok(positive)
    }

    #[cfg(test)]
    mod tests {
        use super::*;
        use tempfile::tempdir;

        #[test]
        fn run_report_on_empty_workspace() {
            let dir = tempdir().unwrap();
            let report = run(dir.path());
            assert_eq!(report.workspace_root, dir.path().display().to_string());
            assert!(report.summary.errors == 0 || report.summary.errors == 1);
            assert!(!report.checks.is_empty());
        }

        #[test]
        fn purge_cache_removes_folder() {
            let dir = tempdir().unwrap();
            let target = dir.path().join("target");
            fs::create_dir_all(&target).unwrap();
            fs::write(target.join("dummy"), b"data").unwrap();
            purge_cache(dir.path()).unwrap();
            assert!(!target.exists());
        }
    }
}

#[cfg(feature = "trace")]
use env_logger;

#[cfg(feature = "color")]
use owo_colors::OwoColorize;

// ───────────────────────────── Types publics ─────────────────────────────

/// Représente une commande haut-niveau (sans parsing CLI — réservé à main.rs).
#[derive(Clone, Debug)]
pub enum Command {
    /// Compile un fichier source vers bytecode.
    Compile(CompileTask),
    /// Lance un bytecode via la VM (si disponible) — accepte aussi le chemin d'une source si `auto_compile = true`.
    Run(RunTask),
    /// Lancer un REPL (si fourni par les hooks).
    Repl(ReplTask),
    /// Formater un fichier source (ou stdin → stdout).
    Fmt(FmtTask),
    /// Inspecter un artefact (headers, tailles, sections, etc.).
    Inspect(InspectTask),
    /// Désassembler un bytecode.
    Disasm(DisasmTask),
    /// Lister les modules compilés (selon les features du méta-crate).
    Modules(ModulesTask),
    /// Diagnostiquer l'environnement CLI.
    Doctor(DoctorTask),
}

#[derive(Clone, Debug, Default)]
pub struct CompileTask {
    pub input: Input,     // chemin ou stdin
    pub output: Output,   // chemin, stdout ou auto (même dossier, .vitbc)
    pub optimize: bool,   // -O
    pub emit_debug: bool, // info debug
    pub auto_mkdir: bool, // crée les dossiers parents si besoin
    pub overwrite: bool,  // autorise l'écrasement
    pub time: bool,       // afficher le timing
    pub profile: context::ProfileConfig,
}

#[derive(Clone, Debug, Default)]
pub struct RunTask {
    pub program: InputKind, // bytecode (fichier ou bytes), ou Source si auto_compile
    pub args: Vec<String>,
    pub auto_compile: bool, // si program = Source(path/stdin), compiler d'abord
    pub optimize: bool,
    pub time: bool,
    pub profile: context::ProfileConfig,
}

#[derive(Clone, Debug, Default)]
pub struct ReplTask {
    pub prompt: String,
}

#[derive(Clone, Debug, Default)]
pub struct FmtTask {
    pub input: Input,   // chemin source ou stdin
    pub output: Output, // si Output::InPlace => réécrit le fichier
    pub check: bool,    // mode --check (retourne erreur si diff)
    pub diff: bool,     // affiche un diff (--diff)
}

#[derive(Clone, Debug, Default)]
pub struct InspectTask {
    pub input: InputKind,
    pub options: inspect::InspectOptions,
}

#[derive(Clone, Debug, Default)]
pub struct DisasmTask {
    pub input: InputKind,
    pub output: Output, // fichier ou stdout
}

#[derive(Clone, Debug)]
pub enum ModulesFormat {
    Table,
    Json,
}

impl Default for ModulesFormat {
    fn default() -> Self {
        ModulesFormat::Table
    }
}

#[derive(Clone, Debug, Default)]
pub struct ModulesTask {
    pub format: ModulesFormat,
    pub trace_registry: bool,
}

#[derive(Clone, Debug, Default)]
pub struct DoctorTask {
    pub output_json: bool,
    pub fix_cache: bool,
    pub assume_yes: bool,
}

/// Entrée texte (source) : fichier ou `-` (=stdin).
#[derive(Clone, Debug)]
pub enum Input {
    Path(PathBuf),
    Stdin,
}
impl Default for Input {
    fn default() -> Self {
        Self::Stdin
    }
}

/// Entrée binaire/typée :
#[derive(Clone, Debug)]
pub enum InputKind {
    BytecodePath(PathBuf),
    BytecodeBytes(Vec<u8>),
    SourcePath(PathBuf),
    SourceStdin,
}
impl Default for InputKind {
    fn default() -> Self {
        Self::SourceStdin
    }
}

/// Sortie générique.
#[derive(Clone, Debug)]
pub enum Output {
    Path(PathBuf),
    Stdout,
    InPlace, // pour fmt
    Auto,    // pour compile : même nom + .vitbc
}
impl Default for Output {
    fn default() -> Self {
        Self::Stdout
    }
}

/// Hooks pour brancher tes implémentations.
#[derive(Clone)]
pub struct Hooks {
    pub compile: Option<CompileFn>,
    pub run_bc: Option<RunFn>,
    pub repl: Option<ReplFn>,
    pub fmt: Option<FormatFn>,
    pub disasm: Option<DisasmFn>,
    pub inspect: Option<InspectFn>,
}
impl Default for Hooks {
    fn default() -> Self {
        Self { compile: None, run_bc: None, repl: None, fmt: None, disasm: None, inspect: None }
    }
}

/// Signature d'un compilateur : source → bytecode.
pub type CompileFn = fn(source: &str, opts: &CompileOptions) -> Result<Vec<u8>>;
/// VM : bytecode → exit code.
pub type RunFn = fn(bytecode: &[u8], opts: &RunOptions) -> Result<i32>;
/// REPL : boucle, retourne exit code.
pub type ReplFn = fn(prompt: &str) -> Result<i32>;
/// Formatage : source → code formaté.
pub type FormatFn = fn(source: &str, check_only: bool) -> Result<String>;
/// Désassemblage : bytecode → texte.
pub type DisasmFn = fn(bytecode: &[u8]) -> Result<String>;
/// Inspection : bytecode → texte (ou JSON), au choix de ton impl.
pub type InspectFn = fn(bytes: &[u8], options: &inspect::InspectOptions) -> Result<String>;

#[derive(Clone, Debug, Default)]
pub struct CompileOptions {
    pub optimize: bool,
    pub emit_debug: bool,
}

#[derive(Clone, Debug, Default)]
pub struct RunOptions {
    pub args: Vec<String>,
    pub optimize: bool,
}

// ───────────────────────────── Initialisation ─────────────────────────────

/// Initialise le logger selon la feature `trace`.
pub fn init_logger() {
    #[cfg(feature = "trace")]
    {
        let _ = env_logger::Builder::from_env(env_logger::Env::default().default_filter_or("info"))
            .format_timestamp_secs()
            .try_init();
    }
}

// ───────────────────────────── Exécution ─────────────────────────────

/// Exécute une commande avec les hooks fournis. Retourne un code de sortie.
pub fn execute(cmd: Command, hooks: &Hooks) -> Result<i32> {
    match cmd {
        Command::Compile(t) => {
            let code = compile_entry(t, hooks)?;
            Ok(code)
        }
        Command::Run(t) => run_entry(t, hooks),
        Command::Repl(t) => {
            let f = hooks.repl.ok_or_else(|| anyhow!("REPL non disponible (hook manquant)"))?;
            let exit = f(&t.prompt)?;
            Ok(exit)
        }
        Command::Fmt(t) => {
            fmt_entry(t, hooks)?;
            Ok(0)
        }
        Command::Inspect(t) => {
            inspect_entry(t, hooks)?;
            Ok(0)
        }
        Command::Disasm(t) => {
            disasm_entry(t, hooks)?;
            Ok(0)
        }
        Command::Modules(t) => {
            modules_entry(t)?;
            Ok(0)
        }
        Command::Doctor(t) => doctor_entry(t),
    }
}

fn compile_entry(task: CompileTask, hooks: &Hooks) -> Result<i32> {
    let CompileTask {
        input,
        output,
        optimize,
        emit_debug,
        auto_mkdir,
        overwrite,
        time,
        profile: profile_cfg,
    } = task;
    let compiler = match hooks.compile {
        Some(c) => c,
        None => {
            return bail_diagnostic(
                Diagnostic::new(
                    "HOOK001",
                    "Compilation indisponible : hook `compile` absent",
                )
                .with_note("Activez la feature `vm` (`cargo run -p vitte-cli --features vm`) ou fournissez votre propre compilateur via Hooks::compile.")
                .with_help("Recompilez vitte-cli avec la feature `engine` ou fournissez un compilateur dans Hooks::compile."),
            )
        },
    };

    let src = read_source(&input).context("lecture de la source")?;
    if profile_cfg.diagnostics {
        perform_frontend_checks(&src, &input)?;
    }

    let start = Instant::now();
    let compile_opts = CompileOptions { optimize, emit_debug };
    let bc = match compiler(&src, &compile_opts) {
        Ok(bytes) => bytes,
        Err(err) => {
            return bail_diagnostic(
                Diagnostic::new("COMPILE100", "Le hook de compilation a retourné une erreur.")
                    .with_note(format!("détail: {err}"))
                    .with_help("Activez les logs (RUST_LOG=debug) ou recompilez avec `--features vm` pour utiliser le compilateur de référence."),
            );
        }
    };
    let elapsed = start.elapsed();

    let out_path = match (&output, &input) {
        (Output::Auto, Input::Path(p)) => default_bytecode_path(p),
        (Output::Auto, Input::Stdin) => PathBuf::from("out.vitbc"),
        (Output::Path(p), _) => p.clone(),
        (Output::Stdout, _) => PathBuf::new(), // indicateur stdout
        (Output::InPlace, _) => anyhow::bail!("Output::InPlace n'a pas de sens pour compile"),
    };

    if let Output::Stdout = output {
        let mut w = BufWriter::new(io::stdout().lock());
        w.write_all(&bc)?;
        w.flush()?;
    } else {
        if auto_mkdir {
            if let Some(parent) = out_path.parent() {
                fs::create_dir_all(parent)?;
            }
        }
        if out_path.exists() && !overwrite {
            return Err(anyhow!("fichier de sortie existe déjà: {}", display(&out_path)));
        }
        write_bytes_atomic(&out_path, &bc)
            .with_context(|| format!("écriture de {}", display(&out_path)))?;
        status_ok("COMPILE", &format!("{} ({} octets)", display(&out_path), bc.len()));
    }

    if time {
        status_info("TIME", &format!("compile: {} ms", elapsed.as_millis()));
    }
    Ok(0)
}

fn run_entry(task: RunTask, hooks: &Hooks) -> Result<i32> {
    let runner = match hooks.run_bc {
        Some(r) => r,
        None => {
            return bail_diagnostic(
                Diagnostic::new("HOOK002", "Exécution indisponible : hook `run_bc` absent")
                    .with_note("Activez la feature `vm` (`cargo run -p vitte-cli --features vm`) ou fournissez une implémentation de VM via Hooks::run_bc.")
                    .with_help("Activez la feature `vm` ou fournissez une VM via Hooks::run_bc."),
            );
        }
    };

    let bytes = match task.program {
        InputKind::BytecodePath(p) => {
            fs::read(&p).with_context(|| format!("lecture bytecode: {}", display(&p)))?
        }
        InputKind::BytecodeBytes(b) => b,
        InputKind::SourcePath(p) if task.auto_compile => {
            let src = read_source(&Input::Path(p.clone()))?;
            perform_frontend_checks(&src, &Input::Path(p.clone()))?;
            let compiler = match hooks.compile {
                Some(c) => c,
                None => {
                    return bail_diagnostic(
                        Diagnostic::new(
                            "HOOK001",
                            "auto-compile demandé mais hook `compile` absent",
                        )
                        .with_note("Activez la feature `vm` (`cargo run -p vitte-cli --features vm`) ou fournissez `Hooks::compile`.")
                        .with_help("Activez la feature `engine` ou fournissez un compilateur."),
                    );
                }
            };
            let compile_opts = CompileOptions { optimize: task.optimize, emit_debug: false };
            match compiler(&src, &compile_opts) {
                Ok(bytes) => bytes,
                Err(err) => {
                    return bail_diagnostic(
                        Diagnostic::new("COMPILE101", "La compilation automatique a échoué.")
                            .with_note(format!("détail: {err}"))
                            .with_help("Passez un bytecode `.vitbc` déjà compilé ou activez les logs (RUST_LOG=debug)."),
                    );
                }
            }
        }
        InputKind::SourceStdin if task.auto_compile => {
            let src = read_source(&Input::Stdin)?;
            perform_frontend_checks(&src, &Input::Stdin)?;
            let compiler = match hooks.compile {
                Some(c) => c,
                None => {
                    return bail_diagnostic(
                        Diagnostic::new(
                            "HOOK001",
                            "auto-compile demandé mais hook `compile` absent",
                        )
                        .with_note("Activez la feature `vm` (`cargo run -p vitte-cli --features vm`) ou fournissez `Hooks::compile`.")
                        .with_help("Activez la feature `engine` ou fournissez un compilateur."),
                    );
                }
            };
            let compile_opts = CompileOptions { optimize: task.optimize, emit_debug: false };
            match compiler(&src, &compile_opts) {
                Ok(bytes) => bytes,
                Err(err) => {
                    return bail_diagnostic(
                        Diagnostic::new("COMPILE101", "La compilation automatique a échoué.")
                            .with_note(format!("détail: {err}"))
                            .with_help("Passez un bytecode `.vitbc` déjà compilé ou activez les logs (RUST_LOG=debug)."),
                    );
                }
            }
        }
        InputKind::SourcePath(p) => {
            return bail_diagnostic(
                Diagnostic::new(
                    "RUN100",
                    format!(
                        "'run' attend un bytecode (ou activez --auto-compile) : {}",
                        display(&p)
                    ),
                )
                .with_note("Astuce : utilisez `vitte run --auto-compile <source>` ou fournissez un bytecode `.vitbc`.")
                .with_help("Fournissez un .vitbc ou ajoutez --auto-compile."),
            );
        }
        InputKind::SourceStdin => {
            return bail_diagnostic(
                Diagnostic::new(
                    "RUN101",
                    "'run' attend un bytecode (ou --auto-compile) depuis stdin",
                )
                .with_note(
                    "Redirigez un bytecode (`cat programme.vitbc | vitte run -`) ou utilisez --auto-compile pour compiler une source.",
                ),
            );
        }
    };

    let start = Instant::now();
    let code = match runner(&bytes, &RunOptions { args: task.args, optimize: task.optimize }) {
        Ok(code) => code,
        Err(err) => {
            return bail_diagnostic(
                Diagnostic::new("RUN200", "La machine virtuelle a signalé une erreur.")
                    .with_note(format!("détail: {err}"))
                    .with_help(
                        "Vérifiez votre bytecode ou activez les journaux (`RUST_LOG=debug`).",
                    ),
            );
        }
    };
    let elapsed = start.elapsed();

    if task.time {
        status_info("TIME", &format!("run: {} ms", elapsed.as_millis()));
    }
    if code == 0 {
        status_ok("RUN", "exit=0");
    } else {
        status_warn("RUN", &format!("exit={code}"));
    }
    Ok(code)
}

fn perform_frontend_checks(src: &str, input: &Input) -> Result<()> {
    let label = input_label(input);
    let source = SourceId(0);

    let mut lexer = Lexer::new(src, source);
    let lines = lexer.lines.clone();
    loop {
        match lexer.next() {
            Ok(Some(tok)) => {
                if matches!(tok.value, TokenKind::Eof) {
                    break;
                }
            }
            Ok(None) => break,
            Err(err) => {
                let ((l1, c1), (l2, c2)) = lines.span_lines(err.span);
                return bail_diagnostic(
                    Diagnostic::new("LEX100", format!("Erreur lexicale: {err}"))
                        .with_note(format!("{label}:L{}C{}-L{}C{}", l1, c1, l2, c2))
                        .with_help(
                            "Corrigez le lexème identifié avant de relancer la compilation.",
                        ),
                );
            }
        }
    }

    if is_simple_module_declaration(src) {
        return Ok(());
    }

    let mut parser = Parser::new(src, source);
    if let Err(err) = parser.parse_program() {
        let span = err.span;
        let msg = err.message;
        let lines = LineMap::new(src);
        let ((l1, c1), (l2, c2)) = lines.span_lines(span);
        return bail_diagnostic(
            Diagnostic::new("PARSE100", format!("Erreur de syntaxe: {msg}"))
                .with_note(format!("{label}:L{}C{}-L{}C{}", l1, c1, l2, c2))
                .with_help("Corrigez la structure du code avant compilation."),
        );
    }

    Ok(())
}

fn is_simple_module_declaration(src: &str) -> bool {
    let trimmed = src.trim();
    if !trimmed.starts_with("module") {
        return false;
    }
    let mut rest = trimmed.trim_start_matches("module").trim_start();
    if rest.is_empty() {
        return false;
    }
    let ident_len =
        rest.chars().take_while(|c| c.is_ascii_alphanumeric() || *c == '_' || *c == '-').count();
    if ident_len == 0 {
        return false;
    }
    rest = rest[ident_len..].trim_start();
    rest == ";" || rest.starts_with('{')
}

fn input_label(input: &Input) -> String {
    match input {
        Input::Path(p) => display(p),
        Input::Stdin => "stdin".to_string(),
    }
}

fn print_text_diff(title: &str, before: &str, after: &str) {
    let diff = TextDiff::from_lines(before, after);
    eprintln!("--- {} (avant)", title);
    eprintln!("+++ {} (après)", title);
    for change in diff.iter_all_changes() {
        let mut text = change.to_string();
        if text.ends_with('\n') {
            text.pop();
            if text.ends_with('\r') {
                text.pop();
            }
        }
        match change.tag() {
            ChangeTag::Delete => {
                #[cfg(feature = "color")]
                eprintln!("{}", format!("-{}", text).red());
                #[cfg(not(feature = "color"))]
                eprintln!("-{}", text);
            }
            ChangeTag::Insert => {
                #[cfg(feature = "color")]
                eprintln!("{}", format!("+{}", text).green());
                #[cfg(not(feature = "color"))]
                eprintln!("+{}", text);
            }
            ChangeTag::Equal => {
                #[cfg(feature = "color")]
                eprintln!(" {}", text.dimmed());
                #[cfg(not(feature = "color"))]
                eprintln!(" {}", text);
            }
        }
    }
}

fn fmt_entry(task: FmtTask, hooks: &Hooks) -> Result<()> {
    let formatter = match hooks.fmt {
        Some(f) => f,
        None => {
            return bail_diagnostic(
                Diagnostic::new("HOOK003", "Formatage indisponible : hook `fmt` absent")
                    .with_note("Activez la feature `fmt` (`cargo run -p vitte-cli --features fmt`) ou fournissez `Hooks::fmt`.")
                    .with_help("Recompilez vitte-cli avec la feature `fmt` ou fournissez un formateur."),
            );
        }
    };

    let src = read_source(&task.input)?;
    let formatted = match formatter(&src, task.check) {
        Ok(out) => out,
        Err(err) => {
            return bail_diagnostic(
                Diagnostic::new("FMT200", "Le formateur a retourné une erreur.")
                    .with_note(format!("détail: {err}"))
                    .with_help("Activez les journaux (RUST_LOG=debug) ou inspectez le fichier pour repérer les constructions non supportées."),
            );
        }
    };

    let label = input_label(&task.input);
    let changed = src != formatted;

    if task.check {
        if task.diff && changed {
            print_text_diff(&label, &src, &formatted);
        }
        if changed {
            return bail_diagnostic(
                Diagnostic::new("FMT201", "Le fichier n'est pas formaté.")
                    .with_note(format!("fichier : {label}"))
                    .with_help("Exécutez `vitte fmt --in-place <fichier>` ou retirez --check pour appliquer le formatage."),
            );
        }
        status_ok("FMT", &format!("check OK ({label})"));
        return Ok(());
    }

    if task.diff && changed {
        print_text_diff(&label, &src, &formatted);
    }

    match task.output {
        Output::Stdout => {
            let mut w = BufWriter::new(io::stdout().lock());
            w.write_all(formatted.as_bytes())?;
            w.flush()?;
        }
        Output::InPlace => match task.input {
            Input::Path(ref p) => write_text_atomic(p, &formatted)?,
            Input::Stdin => anyhow::bail!("--in-place nécessite un fichier d'entrée"),
        },
        Output::Path(ref p) => write_text_atomic(p, &formatted)?,
        Output::Auto => anyhow::bail!("Output::Auto n'est pas valide pour fmt"),
    }

    let tag_msg = match task.output {
        Output::Stdout => "stdout".to_string(),
        Output::Path(ref p) => display(p),
        Output::InPlace => match task.input {
            Input::Path(ref p) => display(p),
            Input::Stdin => "stdin".to_string(),
        },
        Output::Auto => "auto".to_string(),
    };

    status_ok("FMT", &format!("écrit -> {tag_msg}"));
    Ok(())
}

fn inspect_entry(task: InspectTask, hooks: &Hooks) -> Result<()> {
    let f = match hooks.inspect {
        Some(f) => f,
        None => return bail_diagnostic(
            Diagnostic::new("HOOK004", "Inspection indisponible : hook `inspect` absent")
                .with_note("Activez la feature `engine`/`vm` ou renseignez `Hooks::inspect` pour analyser vos bytecodes.")
                .with_help(
                    "Recompilez vitte-cli avec la feature `engine` ou fournissez un inspecteur.",
                ),
        ),
    };
    let bytes = match task.input {
        InputKind::BytecodePath(p) => {
            if p.as_os_str() == "-" {
                read_stdin_bytes()?
            } else {
                fs::read(&p).with_context(|| format!("lecture bytecode: {}", display(&p)))?
            }
        }
        InputKind::BytecodeBytes(b) => b,
        InputKind::SourcePath(p) => {
            return bail_diagnostic(
                Diagnostic::new(
                    "INS100",
                    format!("'inspect' attend un bytecode, pas une source : {}", display(&p)),
                )
                .with_note(
                    "Utilisez `vitte compile <fichier>` pour générer un `.vitbc` avant inspection.",
                )
                .with_help("Compilez d'abord votre source en .vitbc."),
            );
        }
        InputKind::SourceStdin => read_stdin_bytes()?,
    };
    let options = task.options.clone();
    let text = match f(&bytes, &options) {
        Ok(out) => out,
        Err(err) => {
            return bail_diagnostic(
                Diagnostic::new("INS200", "L'inspecteur a signalé une erreur.")
                    .with_note(format!("détail: {err}"))
                    .with_help("Activez les journaux (RUST_LOG=debug) pour diagnostiquer le contenu du bytecode."),
            );
        }
    };
    let mut w = BufWriter::new(io::stdout().lock());
    w.write_all(text.as_bytes())?;
    w.flush()?;
    Ok(())
}

fn read_stdin_bytes() -> Result<Vec<u8>> {
    let mut buf = Vec::new();
    io::stdin().lock().read_to_end(&mut buf).context("impossible de lire stdin")?;
    if buf.is_empty() {
        return bail_diagnostic(
            Diagnostic::new("INS102", "stdin ne contient aucun octet")
                .with_note("Exemple : `cat out.vitbc | vitte inspect -`")
                .with_help("Redirigez un bytecode .vitbc vers l'entrée standard."),
        );
    }
    Ok(buf)
}

pub mod inspect {
    use serde::Serialize;

    #[derive(Debug, Clone, Default)]
    pub struct InspectOptions {
        pub summary: bool,
        pub header: bool,
        pub symbols: bool,
        pub sections: bool,
        pub consts: bool,
        pub strings: bool,
        pub imports: bool,
        pub exports: bool,
        pub hexdump: bool,
        pub disasm: bool,
        pub json: bool,
        pub target: bool,
        pub debug: bool,
        pub meta: bool,
        pub verify: bool,
        pub size: bool,
        pub deps: bool,
        pub entry: bool,
        pub dump_all: bool,
    }

    impl InspectOptions {
        pub fn ensure_defaults(&mut self) {
            if self.dump_all {
                self.summary = true;
                self.header = true;
                self.symbols = true;
                self.sections = true;
                self.consts = true;
                self.strings = true;
                self.imports = true;
                self.exports = true;
                self.hexdump = true;
                self.disasm = true;
                self.target = true;
                self.debug = true;
                self.meta = true;
                self.verify = true;
                self.size = true;
                self.deps = true;
                self.entry = true;
            }

            if !self.summary
                && !self.header
                && !self.symbols
                && !self.sections
                && !self.consts
                && !self.strings
                && !self.imports
                && !self.exports
                && !self.hexdump
                && !self.disasm
                && !self.target
                && !self.debug
                && !self.meta
                && !self.verify
                && !self.size
                && !self.deps
                && !self.entry
                && !self.json
            {
                self.summary = true;
            }
        }
    }

    #[derive(Debug)]
    struct InspectionData {
        format: InspectFormat,
        size: usize,
        declared_len: Option<usize>,
        snippet: Option<String>,
        sections: Vec<SectionInfo>,
        strings: Vec<String>,
        notes: Vec<String>,
        hash: Option<String>,
        verify_ok: Option<bool>,
        payload_offset: usize,
        payload: Vec<u8>,
    }

    #[derive(Debug)]
    struct SectionInfo {
        name: &'static str,
        offset: usize,
        size: usize,
    }

    #[derive(Debug, Serialize)]
    struct InspectionReport {
        format: String,
        size: usize,
        declared_len: Option<usize>,
        target: Option<String>,
        snippet_preview: Option<String>,
        sections: Vec<ReportSection>,
        strings: Vec<String>,
        symbols: Vec<String>,
        consts: Vec<String>,
        imports: Vec<String>,
        exports: Vec<String>,
        dependencies: Vec<String>,
        entry: Option<String>,
        hash_blake3: Option<String>,
        verified: Option<bool>,
        notes: Vec<String>,
    }

    #[derive(Debug, Serialize)]
    struct ReportSection {
        name: String,
        offset: usize,
        size: usize,
    }

    #[derive(Debug)]
    enum InspectFormat {
        Vbc0,
        Unknown,
    }

    impl InspectFormat {
        fn as_str(&self) -> &'static str {
            match self {
                InspectFormat::Vbc0 => "VBC0",
                InspectFormat::Unknown => "unknown",
            }
        }
    }

    pub fn render(bytes: &[u8], opts: &InspectOptions) -> String {
        let mut options = opts.clone();
        options.ensure_defaults();
        let data = analyze(bytes);

        if options.json {
            return render_json(&data);
        }

        let mut sections = Vec::new();

        if options.summary {
            sections.push(render_summary(&data));
        }
        if options.header {
            sections.push(render_header(&data));
        }
        if options.sections {
            sections.push(render_sections(&data));
        }
        if options.size {
            sections.push(render_size(&data));
        }
        if options.symbols {
            sections.push(render_symbols(&data));
        }
        if options.consts {
            sections.push(render_consts(&data));
        }
        if options.strings {
            sections.push(render_strings(&data));
        }
        if options.imports {
            sections.push(render_imports(&data));
        }
        if options.exports {
            sections.push(render_exports(&data));
        }
        if options.deps {
            sections.push(render_deps(&data));
        }
        if options.entry {
            sections.push(render_entry(&data));
        }
        if options.target {
            sections.push(render_target(&data));
        }
        if options.meta {
            sections.push(render_meta(&data));
        }
        if options.verify {
            sections.push(render_verify(&data));
        }
        if options.debug {
            sections.push(render_debug(&data));
        }
        if options.disasm {
            sections.push(render_disasm(&data));
        }
        if options.hexdump {
            sections.push(render_hexdump(bytes));
        }

        if sections.is_empty() {
            sections.push(render_summary(&data));
        }

        sections.join("\n\n")
    }

    fn analyze(bytes: &[u8]) -> InspectionData {
        let hash = Some(blake3::hash(bytes).to_hex().to_string());
        if bytes.len() >= 8 && &bytes[..4] == b"VBC0" {
            let declared = u32::from_le_bytes([bytes[4], bytes[5], bytes[6], bytes[7]]) as usize;
            let payload = bytes.get(8..).unwrap_or_default();
            let snippet_len = payload.len().min(declared).min(160);
            let snippet = if snippet_len == 0 {
                None
            } else {
                Some(String::from_utf8_lossy(&payload[..snippet_len]).to_string())
            };

            let mut sections = vec![SectionInfo { name: ".header", offset: 0, size: 8 }];
            if !payload.is_empty() {
                sections.push(SectionInfo {
                    name: ".text",
                    offset: 8,
                    size: payload.len().min(declared),
                });
            }
            if payload.len() > declared {
                sections.push(SectionInfo {
                    name: ".extra",
                    offset: 8 + declared,
                    size: payload.len() - declared,
                });
            }

            let strings = extract_strings(payload);
            let verify_ok = Some(payload.len() == declared);
            let mut notes = vec!["Inspection VBC0 simplifiée (format expérimental).".to_string()];
            if verify_ok == Some(false) {
                notes.push("Payload plus long que la longueur déclarée (stub)".to_string());
            }

            InspectionData {
                format: InspectFormat::Vbc0,
                size: bytes.len(),
                declared_len: Some(declared),
                snippet,
                sections,
                strings,
                notes,
                hash,
                verify_ok,
                payload_offset: 8,
                payload: payload.to_vec(),
            }
        } else {
            InspectionData {
                format: InspectFormat::Unknown,
                size: bytes.len(),
                declared_len: None,
                snippet: None,
                sections: Vec::new(),
                strings: extract_strings(bytes),
                notes: vec!["Format non reconnu — inspection limitée.".to_string()],
                hash,
                verify_ok: None,
                payload_offset: 0,
                payload: bytes.to_vec(),
            }
        }
    }

    fn extract_strings(bytes: &[u8]) -> Vec<String> {
        let mut acc = Vec::new();
        let mut current = Vec::new();
        for &b in bytes {
            let ch = b as char;
            if ch.is_ascii_graphic() || ch == ' ' {
                current.push(b);
            } else {
                if current.len() >= 4 {
                    if let Ok(s) = String::from_utf8(current.clone()) {
                        acc.push(s);
                    }
                }
                current.clear();
            }
        }
        if current.len() >= 4 {
            if let Ok(s) = String::from_utf8(current) {
                acc.push(s);
            }
        }
        if acc.len() > 16 {
            acc.truncate(16);
        }
        acc
    }

    fn render_summary(data: &InspectionData) -> String {
        match data.format {
            InspectFormat::Vbc0 => {
                let declared = data
                    .declared_len
                    .map(|len| format!("\nlongueur déclarée : {len} octets"))
                    .unwrap_or_default();
                let snippet = data
                    .snippet
                    .as_ref()
                    .map(|s| format!("\naperçu source :\n{}", s))
                    .unwrap_or_default();
                format!("Format : VBC0\nsize : {} octets{declared}{snippet}", data.size)
            }
            InspectFormat::Unknown => format!("Format : inconnu\nsize : {} octets", data.size),
        }
    }

    fn render_header(data: &InspectionData) -> String {
        let mut out = format!(
            "En-tête:\n  format : {}\n  taille totale : {} octets",
            data.format.as_str(),
            data.size
        );
        if let Some(declared) = data.declared_len {
            out.push_str(&format!("\n  longueur déclarée : {} octets", declared));
        }
        if let Some(hash) = &data.hash {
            out.push_str(&format!("\n  BLAKE3 : {hash}"));
        }
        out
    }

    fn render_sections(data: &InspectionData) -> String {
        if data.sections.is_empty() {
            return "Sections :\n  (non disponibles pour ce format)".to_string();
        }
        let mut out = String::from("Sections :");
        for section in &data.sections {
            out.push_str(&format!(
                "\n  {name:<10} offset={offset:#06x} size={size}",
                name = section.name,
                offset = section.offset,
                size = section.size
            ));
        }
        out
    }

    fn render_size(data: &InspectionData) -> String {
        if data.sections.is_empty() {
            return format!("Taille totale : {} octets", data.size);
        }
        let mut out = format!("Taille totale : {} octets", data.size);
        let mut accum = 0usize;
        for section in &data.sections {
            accum += section.size;
        }
        out.push_str(&format!("\nSomme sections : {} octets", accum));
        out
    }

    fn render_symbols(data: &InspectionData) -> String {
        match data.format {
            InspectFormat::Vbc0 => {
                "Symboles :\n  (table des symboles non encodée dans ce format)".to_string()
            }
            InspectFormat::Unknown => {
                "Symboles :\n  Impossible de lister les symboles — format non reconnu.".to_string()
            }
        }
    }

    fn render_consts(data: &InspectionData) -> String {
        match data.format {
            InspectFormat::Vbc0 => "Constantes :\n  (non disponibles dans ce build)".to_string(),
            InspectFormat::Unknown => {
                "Constantes :\n  Impossible d'extraire les constantes.".to_string()
            }
        }
    }

    fn render_strings(data: &InspectionData) -> String {
        if data.strings.is_empty() {
            return "Chaînes embarquées :\n  aucune chaîne ASCII détectée".to_string();
        }
        let mut out = String::from("Chaînes embarquées :");
        for s in data.strings.iter().take(10) {
            out.push_str(&format!("\n  \"{}\"", s.replace('\n', "\\n")));
        }
        if data.strings.len() > 10 {
            out.push_str(&format!("\n  … ({}) supplémentaires", data.strings.len() - 10));
        }
        out
    }

    fn render_imports(_data: &InspectionData) -> String {
        "Imports :\n  Informations non disponibles dans cette préversion.".to_string()
    }

    fn render_exports(_data: &InspectionData) -> String {
        "Exports :\n  Informations non disponibles dans cette préversion.".to_string()
    }

    fn render_deps(_data: &InspectionData) -> String {
        "Dépendances :\n  Aucune dépendance module encodée dans ce fichier.".to_string()
    }

    fn render_entry(_data: &InspectionData) -> String {
        "Point d'entrée : non spécifié (format VBC0 stub).".to_string()
    }

    fn render_target(data: &InspectionData) -> String {
        let target = match data.format {
            InspectFormat::Vbc0 => "non spécifiée (VBC0 stub)".to_string(),
            InspectFormat::Unknown => "inconnue".to_string(),
        };
        format!("Cible supposée : {target}")
    }

    fn render_meta(data: &InspectionData) -> String {
        if let Some(hash) = &data.hash {
            format!(
                "Métadonnées :\n  Build-id (BLAKE3) : {hash}\n  Auteur : inconnu\n  Timestamp : n/a"
            )
        } else {
            "Métadonnées indisponibles.".to_string()
        }
    }

    fn render_verify(data: &InspectionData) -> String {
        match data.verify_ok {
            Some(true) => {
                format!("Intégrité : OK (BLAKE3 = {})", data.hash.as_deref().unwrap_or("n/a"))
            }
            Some(false) => "Intégrité : échec (payload > longueur déclarée)".to_string(),
            None => "Intégrité : impossible de vérifier.".to_string(),
        }
    }

    fn render_debug(_data: &InspectionData) -> String {
        "Debug info :\n  Table des lignes / scopes non disponible.".to_string()
    }

    fn render_disasm(data: &InspectionData) -> String {
        if data.payload.is_empty() {
            return "Désassemblage :\n  Aucun bytecode à analyser.".to_string();
        }
        let mut out = String::from("Désassemblage (stub) :");
        for (idx, chunk) in data.payload.chunks(4).enumerate().take(32) {
            let offset = data.payload_offset + idx * 4;
            let op = chunk.get(0).copied().unwrap_or(0);
            let operands =
                chunk.iter().skip(1).map(|b| format!("{:02X}", b)).collect::<Vec<_>>().join(" ");
            out.push_str(&format!("\n  {offset:08X}: OP_{op:02X} {operands}", operands = operands));
        }
        if data.payload.len() / 4 > 32 {
            out.push_str("\n  …");
        }
        out
    }

    fn render_hexdump(bytes: &[u8]) -> String {
        let mut out = String::new();
        for (offset, chunk) in bytes.chunks(16).enumerate() {
            let off = offset * 16;
            let hex = chunk.iter().map(|b| format!("{:02X}", b)).collect::<Vec<_>>().join(" ");
            let ascii = chunk
                .iter()
                .map(|b| {
                    let c = *b as char;
                    if c.is_ascii_graphic() || c.is_ascii_whitespace() {
                        c
                    } else {
                        '.'
                    }
                })
                .collect::<String>();
            out.push_str(&format!("{off:08X}: {:<47} |{}|\n", hex, ascii));
        }
        if out.is_empty() {
            out.push_str("(flux vide)");
        }
        out
    }

    fn render_json(data: &InspectionData) -> String {
        let report = InspectionReport {
            format: data.format.as_str().to_string(),
            size: data.size,
            declared_len: data.declared_len,
            target: Some(match data.format {
                InspectFormat::Vbc0 => "vitte-vm (stub)".to_string(),
                InspectFormat::Unknown => "unknown".to_string(),
            }),
            snippet_preview: data.snippet.clone(),
            sections: data
                .sections
                .iter()
                .map(|s| ReportSection { name: s.name.to_string(), offset: s.offset, size: s.size })
                .collect(),
            strings: data.strings.clone(),
            symbols: Vec::new(),
            consts: Vec::new(),
            imports: Vec::new(),
            exports: Vec::new(),
            dependencies: Vec::new(),
            entry: None,
            hash_blake3: data.hash.clone(),
            verified: data.verify_ok,
            notes: data.notes.clone(),
        };
        serde_json::to_string_pretty(&report).unwrap_or_else(|_| "{}".to_string())
    }
}

pub mod registry {
    use anyhow::{anyhow, Context, Result};
    use serde::Deserialize;
    use std::{
        fs::File,
        io::BufReader,
        path::{Path, PathBuf},
        time::{Duration, Instant},
    };

    #[derive(Debug, Deserialize, Clone)]
    pub struct Module {
        pub name: String,
        pub version: String,
        #[serde(default)]
        pub description: String,
        #[serde(default)]
        pub url: Option<String>,
        #[serde(default)]
        pub checksum: Option<String>,
        #[serde(default)]
        pub tags: Vec<String>,
    }

    #[derive(Debug, Deserialize)]
    struct Index {
        modules: Vec<Module>,
    }

    pub struct LoadedIndex {
        pub modules: Vec<Module>,
        pub source_path: PathBuf,
        pub compressed: bool,
        pub file_size: u64,
        pub decode_duration: Duration,
    }

    pub fn load_local_index_with_trace(root: &Path) -> Result<LoadedIndex> {
        let json_path = root.join("registry/modules/index.json");
        let zst_path = root.join("registry/modules/index.json.zst");
        let (source_path, compressed) = if json_path.exists() {
            (json_path, false)
        } else if zst_path.exists() {
            (zst_path, true)
        } else {
            return Err(anyhow!(
                "registre introuvable (recherché {} et {})",
                json_path.display(),
                zst_path.display()
            ));
        };
        let file = File::open(&source_path)
            .with_context(|| format!("lecture registre {}", source_path.display()))?;
        let file_size = file
            .metadata()
            .map(|m| m.len())
            .with_context(|| format!("metadata registre {}", source_path.display()))?;
        let start = Instant::now();
        let modules = if compressed {
            let decoder = zstd::stream::read::Decoder::new(file)
                .with_context(|| format!("décompression registre {}", source_path.display()))?;
            let reader = BufReader::new(decoder);
            let index: Index = serde_json::from_reader(reader)
                .with_context(|| format!("parse registre {}", source_path.display()))?;
            index.modules
        } else {
            let reader = BufReader::new(file);
            let index: Index = serde_json::from_reader(reader)
                .with_context(|| format!("parse registre {}", source_path.display()))?;
            index.modules
        };
        let decode_duration = start.elapsed();
        Ok(LoadedIndex { modules, source_path, compressed, file_size, decode_duration })
    }

    pub fn load_local_index(root: &Path) -> Result<Vec<Module>> {
        load_local_index_with_trace(root).map(|info| info.modules)
    }

    pub fn find<'a>(modules: &'a [Module], name: &str) -> Option<&'a Module> {
        modules.iter().find(|m| m.name == name)
    }
}

fn disasm_entry(task: DisasmTask, hooks: &Hooks) -> Result<()> {
    let f = match hooks.disasm {
        Some(f) => f,
        None => {
            return bail_diagnostic(
                Diagnostic::new("HOOK005", "Désassemblage indisponible : hook `disasm` absent")
                    .with_note("Activez la feature `vm` ou implémentez Hooks::disasm pour produire un rendu lisible.")
                    .with_help("Recompilez vitte-cli avec la feature `disasm/modules` ou fournissez un désassembleur."),
            )
        },
    };
    let bytes = match task.input {
        InputKind::BytecodePath(p) => {
            fs::read(&p).with_context(|| format!("lecture bytecode: {}", display(&p)))?
        }
        InputKind::BytecodeBytes(b) => b,
        InputKind::SourcePath(p) => {
            return bail_diagnostic(
                Diagnostic::new("DIS100", format!("'disasm' attend un bytecode, pas une source : {}", display(&p)))
                    .with_note("Compilez votre fichier (`vitte compile <src>`), puis désassemblez le `.vitbc`."),
            );
        }
        InputKind::SourceStdin => {
            return bail_diagnostic(
                Diagnostic::new("DIS101", "'disasm' attend un bytecode sur stdin")
                    .with_note("Redirigez un bytecode (`cat app.vitbc | vitte disasm -`)."),
            );
        }
    };
    let text = match f(&bytes) {
        Ok(out) => out,
        Err(err) => {
            return bail_diagnostic(
                Diagnostic::new("DIS200", "Le désassembleur a signalé une erreur.")
                    .with_note(format!("détail: {err}"))
                    .with_help("Assurez-vous que le bytecode provient de `vitte compile` ou activez les journaux détaillés."),
            );
        }
    };
    match task.output {
        Output::Stdout => {
            let mut w = BufWriter::new(io::stdout().lock());
            w.write_all(text.as_bytes())?;
            w.flush()?;
        }
        Output::Path(ref p) => write_text_atomic(p, &text)?,
        Output::InPlace => anyhow::bail!("Output::InPlace n'a pas de sens pour disasm"),
        Output::Auto => anyhow::bail!("Output::Auto n'est pas valide pour disasm"),
    }
    status_ok("DISASM", "ok");
    Ok(())
}

fn modules_entry(task: ModulesTask) -> Result<()> {
    if task.trace_registry {
        match registry::load_local_index_with_trace(std::path::Path::new(".")) {
            Ok(info) => {
                let mode = if info.compressed { "json.zst" } else { "json" };
                let millis = info.decode_duration.as_millis();
                status_info(
                    "REGISTRY",
                    &format!(
                        "{} ({mode}, {} modules, size={} bytes, load={} ms)",
                        info.source_path.display(),
                        info.modules.len(),
                        info.file_size,
                        millis
                    ),
                );
            }
            Err(err) => {
                status_warn("REGISTRY", &format!("impossible de charger le registre: {err}"));
            }
        }
    }

    #[cfg(not(feature = "modules"))]
    {
        let _ = task;
        return bail_diagnostic(
            Diagnostic::new(
                "HOOK006",
                "Commande `modules` indisponible (feature `modules` désactivée)",
            )
            .with_note("Activez la feature `modules` (`cargo run -p vitte-cli --features modules`) pour inclure la liste intégrée.")
            .with_help("Recompilez vitte-cli avec `--features modules`."),
        );
    }

    #[cfg(feature = "modules")]
    {
        use ModulesFormat::*;

        let mods = vitte_modules::modules();
        match task.format {
            Table => {
                if mods.is_empty() {
                    println!(
                        "(aucun module activé — compile `vitte-modules` avec les features désirées)"
                    );
                } else {
                    for module in mods {
                        println!("{:<12} {}", module.name, module.description);
                        if !module.tags.is_empty() {
                            println!("    tags: {}", module.tags.join(", "));
                        }
                    }
                }
                Ok(())
            }
            Json => {
                let payload: Vec<_> = mods
                    .iter()
                    .map(|m| {
                        serde_json::json!({
                            "name": m.name,
                            "description": m.description,
                            "tags": m.tags,
                        })
                    })
                    .collect();
                println!("{}", serde_json::to_string_pretty(&payload)?);
                Ok(())
            }
        }
    }
}

fn doctor_entry(task: DoctorTask) -> Result<i32> {
    let root = std::env::current_dir().context("détection du workspace")?;
    let report = doctor::run(&root);

    if task.output_json {
        let json = serde_json::to_string_pretty(&report)?;
        println!("{}", json);
    } else {
        doctor::print_report(&report);
    }

    if task.fix_cache {
        let target_dir = root.join("target");
        if target_dir.exists() {
            let proceed = if task.assume_yes {
                true
            } else {
                let prompt = i18n::tr(
                    "doctor.prompt.purge_cache",
                    [("path", target_dir.display().to_string().as_str())].as_ref(),
                );
                doctor::ask_confirmation(&prompt)?
            };
            if proceed {
                doctor::purge_cache(&root)?;
                println!("{}", i18n::tr("doctor.action.cache_purged", &[]));
            } else {
                println!("{}", i18n::tr("doctor.action.cache_kept", &[]));
            }
        } else {
            println!("{}", i18n::tr("doctor.action.cache_missing", &[]));
        }
    }

    let exit_code = if report.summary.errors > 0 {
        2
    } else if report.summary.warnings > 0 {
        1
    } else {
        0
    };
    Ok(exit_code)
}

// ───────────────────────────── Utilitaires E/S ─────────────────────────────

fn read_source(input: &Input) -> Result<String> {
    match input {
        Input::Stdin => {
            let mut s = String::new();
            io::stdin().read_to_string(&mut s)?;
            Ok(s)
        }
        Input::Path(p) => {
            let f = File::open(&p).with_context(|| format!("ouverture: {}", display(p)))?;
            let mut r = BufReader::new(f);
            let mut s = String::new();
            r.read_to_string(&mut s)?;
            Ok(s)
        }
    }
}

fn write_bytes_atomic(path: &Path, bytes: &[u8]) -> Result<()> {
    let parent =
        path.parent().ok_or_else(|| anyhow!("chemin de sortie sans parent: {}", display(path)))?;
    let mut tmp = NamedTempFile::new_in(parent)?;
    {
        let file = tmp.as_file_mut();
        file.write_all(bytes)?;
        file.flush()?;
    }
    #[cfg(unix)]
    {
        use std::os::unix::fs::PermissionsExt;

        let mode = fs::metadata(path).map(|m| m.permissions().mode()).unwrap_or(0o644);
        let perms = fs::Permissions::from_mode(mode);
        fs::set_permissions(tmp.path(), perms)?;
    }
    if path.exists() {
        // Windows : Rename sur cible existante peut échouer
        let _ = fs::remove_file(path);
    }
    match tmp.persist(path) {
        Ok(_) => Ok(()),
        Err(err) => {
            let tmp_file = err.file;
            let tmp_path = tmp_file.path().to_path_buf();
            fs::copy(&tmp_path, path)?;
            drop(tmp_file);
            Ok(())
        }
    }
}

fn write_text_atomic(path: &Path, text: &str) -> Result<()> {
    write_bytes_atomic(path, text.as_bytes())
}

fn default_bytecode_path(src: &Path) -> PathBuf {
    let stem = src.file_stem().and_then(|s| s.to_str()).unwrap_or("out");
    let mut p = src.with_file_name(stem);
    p.set_extension("vitbc");
    p
}

fn display(p: &Path) -> String {
    p.to_string_lossy().to_string()
}

// ───────────────────────────── Sorties jolies ─────────────────────────────

fn status_ok(tag: &str, msg: &str) {
    #[cfg(feature = "color")]
    {
        eprintln!("{} {}", tag.green().bold(), msg);
    }
    #[cfg(not(feature = "color"))]
    {
        eprintln!("{} {}", tag, msg);
    }
}

fn status_info(tag: &str, msg: &str) {
    #[cfg(feature = "color")]
    {
        eprintln!("{} {}", tag.blue().bold(), msg);
    }
    #[cfg(not(feature = "color"))]
    {
        eprintln!("{} {}", tag, msg);
    }
}

fn status_warn(tag: &str, msg: &str) {
    #[cfg(feature = "color")]
    {
        eprintln!("{} {}", tag.yellow().bold(), msg);
    }
    #[cfg(not(feature = "color"))]
    {
        eprintln!("{} {}", tag, msg);
    }
}

#[allow(dead_code)]
enum Severity {
    Error,
    Warning,
}

struct Diagnostic {
    severity: Severity,
    code: &'static str,
    message: String,
    notes: Vec<String>,
    help: Option<String>,
}

impl Diagnostic {
    fn new(code: &'static str, message: impl Into<String>) -> Self {
        Self {
            severity: Severity::Error,
            code,
            message: message.into(),
            notes: Vec::new(),
            help: None,
        }
    }

    #[allow(dead_code)]
    fn error(code: &'static str, message: impl Into<String>) -> Self {
        Self {
            severity: Severity::Error,
            code,
            message: message.into(),
            notes: Vec::new(),
            help: None,
        }
    }

    #[allow(dead_code)]
    fn warning(code: &'static str, message: impl Into<String>) -> Self {
        Self {
            severity: Severity::Warning,
            code,
            message: message.into(),
            notes: Vec::new(),
            help: None,
        }
    }

    #[allow(dead_code)]
    fn with_note(mut self, note: impl Into<String>) -> Self {
        self.notes.push(note.into());
        self
    }

    fn with_help(mut self, help: impl Into<String>) -> Self {
        self.help = Some(help.into());
        self
    }
}

fn emit_diagnostic(diag: &Diagnostic) {
    #[cfg(feature = "color")]
    {
        match diag.severity {
            Severity::Error => {
                eprintln!("{} {}", format!("error[{}]", diag.code).red().bold(), diag.message.red())
            }
            Severity::Warning => eprintln!(
                "{} {}",
                format!("warning[{}]", diag.code).yellow().bold(),
                diag.message.yellow()
            ),
        }
        for note in &diag.notes {
            eprintln!("{} {}", "note".blue().bold(), note);
        }
        if let Some(help) = &diag.help {
            eprintln!("{} {}", "help".green().bold(), help);
        }
    }
    #[cfg(not(feature = "color"))]
    {
        let label = match diag.severity {
            Severity::Error => format!("error[{}]", diag.code),
            Severity::Warning => format!("warning[{}]", diag.code),
        };
        eprintln!("{}: {}", label, diag.message);
        for note in &diag.notes {
            eprintln!("  note: {}", note);
        }
        if let Some(help) = &diag.help {
            eprintln!("  help: {}", help);
        }
    }
}

fn bail_diagnostic<T>(diag: Diagnostic) -> Result<T> {
    let message = format!("error[{}]: {}", diag.code, diag.message);
    emit_diagnostic(&diag);
    Err(anyhow!(message))
}

#[allow(dead_code)]
fn print_diagnostic(diag: Diagnostic) {
    emit_diagnostic(&diag);
}

pub mod repl {
    use anyhow::{anyhow, Context, Result};
    use dirs_next::config_dir;
    use rustyline::{
        completion::{Completer, Pair},
        config::Configurer,
        error::ReadlineError,
        highlight::Highlighter,
        hint::{Hinter, HistoryHinter},
        history::{FileHistory, History},
        validate::{ValidationContext, ValidationResult, Validator},
        Context as LineContext, Editor, Helper,
    };
    use std::{
        borrow::Cow,
        fs,
        io::{self, Write},
        path::{Path, PathBuf},
        sync::{Arc, Mutex},
    };

    #[cfg(feature = "color")]
    use owo_colors::OwoColorize;

    const HISTORY_FILE_NAME: &str = "repl_history";

    // Analyse et parsing pour autocomplétion
    use vitte_analyzer::{AnalysisInput, Analyzer};
    use vitte_ast as ast;
    use vitte_core::SourceId;
    use vitte_parser::Parser;

    pub fn fallback(prompt: &str) -> Result<i32> {
        match advanced_repl(prompt) {
            Ok(code) => Ok(code),
            Err(err) => {
                eprintln!("[repl] initialisation interactive échouée: {err}");
                eprintln!("[repl] retour au mode basique (stdin)");
                basic_stdio(prompt)
            }
        }
    }

    fn advanced_repl(prompt: &str) -> Result<i32> {
        let mut rl = Editor::<ReplHelper, FileHistory>::new()
            .context("initialisation de l'éditeur interactif")?;
        rl.set_auto_add_history(false);
        if let Err(err) = rl.set_history_ignore_dups(true) {
            eprintln!("[repl] impossible d'activer history_ignore_dups: {err}");
        }
        rl.set_history_ignore_space(true);
        let shared = SharedAnalysis::new();
        rl.set_helper(Some(ReplHelper {
            hinter: HistoryHinter {},
            shared: Some(shared.clone()),
            snippets: DEFAULT_SNIPPETS,
        }));

        let history_path = history_file_path();
        if let Some(path) = history_path.as_ref() {
            if path.exists() {
                if let Err(err) = rl.load_history(path) {
                    eprintln!(
                        "[repl] impossible de charger l'historique ({}): {err}",
                        path.display()
                    );
                }
            }
        }

        let mut session = ReplSession::new(prompt, history_path.clone(), shared);
        session.print_banner();

        loop {
            let prompt = session.prompt_for_next_line();
            match rl.readline(&prompt) {
                Ok(line) => match session.handle_line(line, &mut rl)? {
                    FlowControl::Continue => continue,
                    FlowControl::Exit => break,
                },
                Err(ReadlineError::Interrupted) => {
                    session.on_interrupt();
                    continue;
                }
                Err(ReadlineError::Eof) => {
                    session.on_eof();
                    break;
                }
                Err(err) => return Err(err.into()),
            }
        }
        if let Some(path) = history_path {
            if let Some(parent) = path.parent() {
                if let Err(err) = fs::create_dir_all(parent) {
                    eprintln!(
                        "[repl] impossible de créer le dossier d'historique ({}): {err}",
                        parent.display()
                    );
                    return Ok(0);
                }
            }
            if !path.exists() {
                if let Err(err) = fs::File::create(&path) {
                    eprintln!(
                        "[repl] impossible de créer le fichier d'historique ({}): {err}",
                        path.display()
                    );
                    return Ok(0);
                }
            }
            if let Err(err) = rl.append_history(&path) {
                eprintln!(
                    "[repl] impossible de sauvegarder l'historique ({}): {err}",
                    path.display()
                );
            }
        }
        Ok(0)
    }

    fn basic_stdio(prompt: &str) -> Result<i32> {
        let stdin = io::stdin();
        let mut stdout = io::stdout();
        #[cfg(feature = "color")]
        println!("{}", "Vitte REPL (mode basique) — :help pour l'aide, :quit pour quitter".cyan());
        #[cfg(not(feature = "color"))]
        println!("Vitte REPL (mode basique) — :help pour l'aide, :quit pour quitter");

        let mut line = String::new();
        loop {
            print!("{prompt}");
            stdout.flush()?;
            line.clear();
            if stdin.read_line(&mut line)? == 0 {
                println!("(EOF) sortie du REPL");
                break;
            }
            let trimmed = line.trim();
            if trimmed.is_empty() {
                continue;
            }
            if trimmed.starts_with(':') {
                match trimmed {
                    ":quit" | ":q" | ":exit" | "exit" => break,
                    ":help" | ":h" => ReplSession::print_help(),
                    _ => println!("Commande inconnue ({trimmed}) — tape :help"),
                }
                continue;
            }
            println!("→ {}", trimmed);
        }
        Ok(0)
    }

    enum FlowControl {
        Continue,
        Exit,
    }

    type ReplEditor = Editor<ReplHelper, FileHistory>;

    #[derive(Copy, Clone)]
    struct CommandInfo {
        name: &'static str,
        adds_space: bool,
    }

    const REPL_COMMANDS: &[CommandInfo] = &[
        CommandInfo { name: "help", adds_space: false },
        CommandInfo { name: "h", adds_space: false },
        CommandInfo { name: "quit", adds_space: false },
        CommandInfo { name: "exit", adds_space: false },
        CommandInfo { name: "q", adds_space: false },
        CommandInfo { name: "history", adds_space: true },
        CommandInfo { name: "clear-history", adds_space: false },
        CommandInfo { name: "multi", adds_space: true },
        CommandInfo { name: "show", adds_space: false },
        CommandInfo { name: "buffer", adds_space: false },
        CommandInfo { name: "clear", adds_space: false },
        CommandInfo { name: "run", adds_space: false },
        CommandInfo { name: "flush", adds_space: false },
        CommandInfo { name: "save", adds_space: true },
        CommandInfo { name: "load", adds_space: true },
        CommandInfo { name: "pop", adds_space: false },
        CommandInfo { name: "prompt", adds_space: true },
        CommandInfo { name: "status", adds_space: false },
    ];

    #[derive(Default)]
    struct ReplHelper {
        hinter: HistoryHinter,
        shared: Option<SharedAnalysis>,
        snippets: &'static [&'static str],
    }

    impl Helper for ReplHelper {}

    impl Hinter for ReplHelper {
        type Hint = String;

        fn hint(&self, line: &str, pos: usize, ctx: &LineContext<'_>) -> Option<String> {
            self.hinter.hint(line, pos, ctx)
        }
    }

    impl Highlighter for ReplHelper {
        fn highlight_prompt<'b, 's: 'b, 'p: 'b>(
            &'s self,
            prompt: &'p str,
            _: bool,
        ) -> Cow<'p, str> {
            Cow::Borrowed(prompt)
        }
    }

    impl Completer for ReplHelper {
        type Candidate = Pair;

        fn complete(
            &self,
            line: &str,
            pos: usize,
            _ctx: &LineContext<'_>,
        ) -> Result<(usize, Vec<Pair>), ReadlineError> {
            let slice = &line[..pos];

            // 1) Complétion des commandes :...
            if let Some(colon_idx) = slice.rfind(':') {
                if colon_idx == 0 || slice[..colon_idx].ends_with(char::is_whitespace) {
                    let token = &slice[colon_idx + 1..];
                    if !token.chars().any(|c| c.is_whitespace()) {
                        let token_lower = token.to_ascii_lowercase();
                        let mut pairs = Vec::new();
                        for info in REPL_COMMANDS {
                            if info.name.starts_with(&token_lower) {
                                let mut replacement = format!(":{}", info.name);
                                if info.adds_space {
                                    replacement.push(' ');
                                }
                                let display = format!(":{}", info.name);
                                pairs.push(Pair { display, replacement });
                            }
                        }
                        return Ok((colon_idx, pairs));
                    }
                }
            }

            // 2) Complétion symbolique/snippets
            let start_idx = slice
                .rfind(|c: char| !(c.is_alphanumeric() || c == '_'))
                .map(|i| i + 1)
                .unwrap_or(0);
            let prefix = &slice[start_idx..];
            if prefix.is_empty() {
                return Ok((pos, Vec::new()));
            }
            let mut pairs = Vec::new();
            if let Some(shared) = &self.shared {
                for s in shared.symbols() {
                    if s.starts_with(prefix) {
                        pairs.push(Pair { display: s.clone(), replacement: s });
                    }
                }
            }
            for &snip in self.snippets.iter() {
                if snip.starts_with(prefix) {
                    pairs.push(Pair { display: snip.to_string(), replacement: snip.to_string() });
                }
            }
            Ok((start_idx, pairs))
        }
    }

    impl Validator for ReplHelper {
        fn validate(
            &self,
            _: &mut ValidationContext<'_>,
        ) -> Result<ValidationResult, ReadlineError> {
            Ok(ValidationResult::Valid(None))
        }
    }

    struct ReplSession {
        base_prompt: String,
        prompt_override: Option<String>,
        multiline: bool,
        buffer: Vec<String>,
        history_path: Option<PathBuf>,
        analysis: SharedAnalysis,
        accepted_source: String,
    }

    impl ReplSession {
        fn new(prompt: &str, history_path: Option<PathBuf>, analysis: SharedAnalysis) -> Self {
            Self {
                base_prompt: ensure_trailing_space(prompt),
                prompt_override: None,
                multiline: false,
                buffer: Vec::new(),
                history_path,
                analysis,
                accepted_source: String::new(),
            }
        }

        fn prompt_for_next_line(&self) -> String {
            let base = self.prompt_override.as_deref().unwrap_or(&self.base_prompt);
            if self.multiline {
                format!("{}{}| ", base, self.buffer.len() + 1)
            } else {
                base.to_string()
            }
        }

        fn handle_line(&mut self, line: String, rl: &mut ReplEditor) -> Result<FlowControl> {
            let trimmed = line.trim_end();
            if trimmed.is_empty() {
                if self.multiline {
                    self.buffer.push(String::new());
                }
                return Ok(FlowControl::Continue);
            }

            if trimmed.starts_with(':') {
                return self.handle_command(trimmed, rl);
            }

            if self.multiline {
                self.buffer.push(trimmed.to_string());
                println!("(buffer {})", self.buffer.len());
            } else {
                // Mode simple: tenter un item complet
                match parse_program_from_text(trimmed) {
                    Ok(_) => {
                        if let Err(err) = self.analysis.append_and_analyze(trimmed) {
                            eprintln!("[repl] analyse échouée: {err}");
                        } else {
                            self.accepted_source.push_str(trimmed);
                            self.accepted_source.push('\n');
                            self.echo_single(trimmed);
                            if let Err(err) = rl.add_history_entry(trimmed) {
                                eprintln!("[repl] historique indisponible: {err}");
                            }
                        }
                    }
                    Err(_) => {
                        println!(
                            "(entrée incomplète — passage en mode multi-ligne; terminez avec :run)"
                        );
                        self.multiline = true;
                        self.buffer.push(trimmed.to_string());
                    }
                }
            }
            Ok(FlowControl::Continue)
        }

        fn handle_command(&mut self, raw: &str, rl: &mut ReplEditor) -> Result<FlowControl> {
            let command = raw.trim_start_matches(':').trim();
            if command.is_empty() {
                Self::print_help();
                return Ok(FlowControl::Continue);
            }

            let mut parts = command.splitn(2, char::is_whitespace);
            let cmd = parts.next().unwrap_or("").to_ascii_lowercase();
            let arg = parts.next().map(str::trim).filter(|s| !s.is_empty());

            match cmd.as_str() {
                "help" | "h" => Self::print_help(),
                "quit" | "exit" | "q" => return Ok(FlowControl::Exit),
                "history" => self.print_history(rl, arg)?,
                "clear-history" => match rl.history_mut().clear() {
                    Ok(_) => {
                        if let Some(path) = &self.history_path {
                            if let Err(err) = fs::remove_file(path) {
                                if err.kind() != io::ErrorKind::NotFound {
                                    eprintln!(
                                        "[repl] impossible de supprimer l'historique ({}): {err}",
                                        path.display()
                                    );
                                }
                            }
                        }
                        println!("(historique effacé)");
                    }
                    Err(err) => {
                        eprintln!("[repl] impossible d'effacer l'historique: {err}");
                    }
                },
                "multi" => self.toggle_multiline(arg),
                "show" | "buffer" => self.show_buffer(),
                "clear" => {
                    self.buffer.clear();
                    println!("(buffer vidé)");
                }
                "run" | "flush" => self.flush_buffer(rl),
                "save" => {
                    let path = arg.ok_or_else(|| anyhow!("utilisation : :save <fichier>"))?;
                    self.save_buffer(path)?;
                }
                "load" => {
                    let path = arg.ok_or_else(|| anyhow!("utilisation : :load <fichier>"))?;
                    self.load_buffer(path)?;
                }
                "pop" => {
                    if let Some(line) = self.buffer.pop() {
                        println!("(retiré) {}", line);
                    } else {
                        println!("(buffer vide)");
                    }
                }
                "prompt" => self.update_prompt(arg),
                "status" => self.print_status(),
                _ => println!("Commande inconnue : {raw} — tape :help"),
            }
            Ok(FlowControl::Continue)
        }

        fn print_banner(&self) {
            #[cfg(feature = "color")]
            println!(
                "{}",
                "Vitte REPL — commandes : :help, :multi, :run, :save, :load, :history, :quit"
                    .cyan()
            );
            #[cfg(not(feature = "color"))]
            println!("Vitte REPL — commandes : :help, :multi, :run, :save, :load, :history, :quit");
        }

        fn print_help() {
            #[cfg(feature = "color")]
            let header = "Commandes REPL".bold();
            #[cfg(not(feature = "color"))]
            let header = "Commandes REPL".to_string();
            println!(
                "{header}\n  :help            — afficher cette aide\n  :quit / :exit    — quitter le REPL\n  :history [n]     — afficher l'historique (optionnellement limité)\n  :clear-history   — vider l'historique\n  :multi [on|off]  — basculer le mode multi-ligne\n  :show            — afficher le buffer courant\n  :clear           — vider le buffer\n  :run             — valider le buffer (affiche et vide)\n  :save <file>     — enregistrer le buffer dans un fichier\n  :load <file>     — charger un fichier dans le buffer (active multi-ligne)\n  :pop             — retirer la dernière ligne du buffer\n  :prompt <txt>    — définir une invite personnalisée (:prompt reset pour revenir par défaut)\n  :status          — afficher l'état actuel"
            );
        }

        fn print_history(&self, rl: &ReplEditor, arg: Option<&str>) -> Result<()> {
            let history = rl.history();
            if history.is_empty() {
                println!("(historique vide)");
                return Ok(());
            }
            let limit = arg.and_then(|s| s.parse::<usize>().ok());
            let len = history.len();
            let start = limit.map(|n| len.saturating_sub(n)).unwrap_or(0);
            for (idx, entry) in history.iter().enumerate().skip(start) {
                println!("{:>4} {}", idx + 1, entry);
            }
            Ok(())
        }

        fn toggle_multiline(&mut self, arg: Option<&str>) {
            let target = arg.map(|s| s.to_ascii_lowercase());
            let desired = match target.as_deref() {
                Some("on") | Some("true") => Some(true),
                Some("off") | Some("false") => Some(false),
                Some("toggle") | None => None,
                Some(other) => {
                    println!(
                        "Valeur inconnue pour :multi — utilisez on/off/toggle (reçu: {other})"
                    );
                    return;
                }
            };
            if let Some(value) = desired {
                if value {
                    self.enable_multiline();
                } else {
                    self.disable_multiline();
                }
            } else if self.multiline {
                self.disable_multiline();
            } else {
                self.enable_multiline();
            }
        }

        fn enable_multiline(&mut self) {
            if !self.multiline {
                self.multiline = true;
                println!("(mode multi-ligne activé — terminer avec :run)");
            } else {
                println!("(mode multi-ligne déjà actif)");
            }
        }

        fn disable_multiline(&mut self) {
            if self.multiline {
                self.multiline = false;
                self.buffer.clear();
                println!("(mode multi-ligne désactivé, buffer vidé)");
            } else {
                println!("(mode multi-ligne déjà inactif)");
            }
        }

        fn show_buffer(&self) {
            if self.buffer.is_empty() {
                println!("(buffer vide)");
                return;
            }
            for (idx, line) in self.buffer.iter().enumerate() {
                println!("{:>4} {}", idx + 1, line);
            }
        }

        fn flush_buffer(&mut self, rl: &mut ReplEditor) {
            if self.buffer.is_empty() {
                println!("(buffer vide)");
                return;
            }
            let block = self.buffer.join("\n");
            match parse_program_from_text(&block) {
                Ok(_) => {
                    if let Err(err) = self.analysis.append_and_analyze(&block) {
                        eprintln!("[repl] analyse échouée: {err}");
                    } else {
                        println!("→ {}", block);
                        self.accepted_source.push_str(&block);
                        self.accepted_source.push('\n');
                        if let Err(err) = rl.add_history_entry(block.as_str()) {
                            eprintln!("[repl] historique indisponible: {err}");
                        }
                        self.buffer.clear();
                        self.multiline = false;
                    }
                }
                Err(err) => {
                    println!("(erreur de parsing) {err}");
                    println!("(modifiez/complétez puis :run, ou :clear pour annuler)");
                }
            }
        }

        fn save_buffer(&self, path: &str) -> Result<()> {
            if self.buffer.is_empty() {
                println!("(buffer vide — rien à sauvegarder)");
                return Ok(());
            }
            let content = self.buffer.join("\n");
            fs::write(Path::new(path), content).with_context(|| format!("écriture de {path}"))?;
            println!("(buffer enregistré dans {path})");
            Ok(())
        }

        fn load_buffer(&mut self, path: &str) -> Result<()> {
            let data = fs::read_to_string(Path::new(path))
                .with_context(|| format!("lecture de {path}"))?;
            self.buffer = data.lines().map(|s| s.to_string()).collect();
            self.multiline = true;
            println!("(buffer chargé depuis {path}, {} lignes)", self.buffer.len());
            Ok(())
        }

        fn update_prompt(&mut self, arg: Option<&str>) {
            match arg {
                None => println!(
                    "Invite actuelle : {}",
                    self.prompt_override.as_deref().unwrap_or(&self.base_prompt).trim_end()
                ),
                Some(value) if value.eq_ignore_ascii_case("reset") => {
                    self.prompt_override = None;
                    println!("(invite réinitialisée)");
                }
                Some(value) => {
                    let mut prompt = value.to_string();
                    if !prompt.ends_with(' ') {
                        prompt.push(' ');
                    }
                    self.prompt_override = Some(prompt);
                    println!("(nouvelle invite enregistrée)");
                }
            }
        }

        fn print_status(&self) {
            println!(
                "Statut : multi-ligne={}, buffer={} lignes, invite=\"{}\"",
                if self.multiline { "on" } else { "off" },
                self.buffer.len(),
                self.prompt_override.as_deref().unwrap_or(&self.base_prompt).trim_end()
            );
        }

        fn echo_single(&self, line: &str) {
            println!("→ {}", line.trim_end());
        }

        fn on_interrupt(&mut self) {
            if self.multiline && !self.buffer.is_empty() {
                self.buffer.clear();
                println!("(Ctrl-C) buffer multi-ligne vidé");
            } else {
                println!("(Ctrl-C) ligne ignorée");
            }
        }

        fn on_eof(&self) {
            println!("(EOF) sortie du REPL");
        }
    }

    fn ensure_trailing_space(input: &str) -> String {
        if input.ends_with(' ') {
            input.to_string()
        } else {
            format!("{input} ")
        }
    }

    // ───────────────────────────── Analyse partagée pour complétions ─────────────────────────────
    #[derive(Clone)]
    struct SharedAnalysis(Arc<Mutex<AnalysisEngine>>);

    impl SharedAnalysis {
        fn new() -> Self {
            #[allow(unused_mut)]
            let mut analyzer = Analyzer::new();
            // Analyse incrémentale si activée côté CLI
            #[cfg(feature = "analyzer-incremental")]
            {
                analyzer.enable_incremental();
            }
            Self(Arc::new(Mutex::new(AnalysisEngine {
                analyzer,
                source: String::new(),
                symbols: Vec::new(),
            })))
        }

        fn append_and_analyze(&self, new_block: &str) -> Result<()> {
            let mut guard = self.0.lock().expect("analysis lock");
            if !guard.source.is_empty() {
                guard.source.push_str("\n");
            }
            guard.source.push_str(new_block);
            let program = parse_program_from_text(&guard.source)?;
            let _report = guard.analyzer.analyze_with(AnalysisInput::new(&program))?;
            let mut syms = guard.analyzer.symbol_names();
            syms.sort();
            guard.symbols = syms;
            Ok(())
        }

        fn symbols(&self) -> Vec<String> {
            self.0.lock().expect("analysis lock").symbols.clone()
        }
    }

    struct AnalysisEngine {
        analyzer: Analyzer,
        source: String,
        symbols: Vec<String>,
    }

    fn parse_program_from_text(src: &str) -> Result<ast::Program> {
        let mut parser = Parser::new(src, SourceId(0));
        parser.parse_program().map_err(|e| anyhow!(e.to_string()))
    }

    const DEFAULT_SNIPPETS: &[&str] = &[
        "fn name() -> void { }",
        "const NAME: i64 = 0;",
        "struct Name { field: i64; }",
        "enum Name { Variant, }",
    ];

    fn history_file_path() -> Option<PathBuf> {
        let mut base = config_dir()?;
        base.push("vitte");
        if let Err(err) = fs::create_dir_all(&base) {
            eprintln!(
                "[repl] impossible de créer le dossier d'historique ({}): {err}",
                base.display()
            );
            return None;
        }
        base.push(HISTORY_FILE_NAME);
        Some(base)
    }
}

#[cfg(feature = "engine")]
pub fn compile_source_to_bytes(source: &str, opts: &CompileOptions) -> Result<Vec<u8>> {
    use vitte_compiler::{Compiler, CompilerOptions, Severity};
    use vitte_core::SourceId;
    use vitte_parser::Parser;

    let mut parser = Parser::new(source, SourceId(0));
    let program = parser.parse_program().map_err(|err| anyhow!("Erreur de parsing: {err}"))?;

    let mut compiler = Compiler::new(CompilerOptions {
        deny_warnings: false,
        vitbc_version: 2,
        compress_code: opts.optimize,
        embed_names: opts.emit_debug,
    });

    let bytecode = match compiler.compile(&program) {
        Ok(bc) => bc,
        Err(err) => return Err(format_compile_error(err)),
    };

    for diag in compiler.take_diagnostics() {
        if diag.severity == Severity::Warning {
            eprintln!("warning: {}", diag.message);
        }
    }

    Ok(bytecode.to_bytes(compiler.options.compress_code))
}

#[cfg(feature = "engine")]
fn format_compile_error(err: vitte_compiler::CompileError) -> anyhow::Error {
    use vitte_compiler::Severity;

    let mut msg = String::from("échec de compilation:\n");
    if err.diagnostics.is_empty() {
        msg.push_str("  (aucun diagnostic)");
        return anyhow!(msg);
    }
    for diag in err.diagnostics {
        let level = match diag.severity {
            Severity::Error => "error",
            Severity::Warning => "warn",
            Severity::Info => "info",
        };
        let span = diag
            .span
            .map(|sp| format!("@{}:{} ({})", sp.line, sp.column, sp.offset))
            .unwrap_or_else(|| "@?".into());
        msg.push_str(&format!("  {level} {span} {}\n", diag.message));
    }
    anyhow!(msg)
}

#[cfg(feature = "engine")]
pub fn run_bytecode(bytes: &[u8], _opts: &RunOptions) -> Result<i32> {
    use vitte_vm::Vm;

    let mut vm = Vm::new();
    vm.run_bytecode(bytes).map_err(|err| anyhow!(err.to_string()))
}

#[cfg(feature = "engine")]
pub fn disassemble_bytecode(bytes: &[u8]) -> Result<String> {
    let mut options = inspect::InspectOptions::default();
    options.header = true;
    options.sections = true;
    options.disasm = true;
    options.ensure_defaults();
    Ok(inspect::render(bytes, &options))
}

// ───────────────────────────── Tests ─────────────────────────────

#[cfg(test)]
mod tests {
    use super::*;

    fn fake_compile(src: &str, _opts: &CompileOptions) -> Result<Vec<u8>> {
        // jouet : encode la taille + contenu
        let mut out = Vec::new();
        out.extend_from_slice(&(src.len() as u32).to_le_bytes());
        out.extend_from_slice(src.as_bytes());
        Ok(out)
    }

    fn fake_run(bc: &[u8], _opts: &RunOptions) -> Result<i32> {
        if bc.len() >= 4 {
            Ok(0)
        } else {
            Ok(1)
        }
    }

    fn fake_fmt(src: &str, _check: bool) -> Result<String> {
        Ok(src.trim().to_string())
    }
    fn fake_disasm(bc: &[u8]) -> Result<String> {
        Ok(format!("len={}", bc.len()))
    }
    fn fake_inspect(b: &[u8], _opts: &inspect::InspectOptions) -> Result<String> {
        Ok(format!("bytes:{}", b.len()))
    }

    #[test]
    fn pipeline_compile_stdout() {
        let hooks = Hooks {
            compile: Some(fake_compile),
            run_bc: Some(fake_run),
            repl: None,
            fmt: Some(fake_fmt),
            disasm: Some(fake_disasm),
            inspect: Some(fake_inspect),
        };
        let t = CompileTask {
            input: Input::Stdin,
            output: Output::Stdout,
            optimize: false,
            emit_debug: false,
            auto_mkdir: false,
            overwrite: true,
            time: false,
            profile: context::ProfileConfig::default(),
        };
        // juste vérifier que ça ne panique pas
        let _ = compile_entry(t, &hooks).unwrap();
    }

    #[test]
    fn default_paths() {
        let p = PathBuf::from("src/main.vt");
        let out = super::default_bytecode_path(&p);
        assert_eq!(out.file_name().unwrap().to_string_lossy(), "main.vitbc");
    }

    #[cfg(unix)]
    #[test]
    fn atomic_write_preserves_permissions() {
        use std::os::unix::fs::PermissionsExt;

        let dir = tempfile::tempdir().unwrap();
        let path = dir.path().join("script.sh");
        std::fs::write(&path, b"#!/bin/sh\necho old\n").unwrap();
        let mut perms = std::fs::metadata(&path).unwrap().permissions();
        perms.set_mode(0o755);
        std::fs::set_permissions(&path, perms).unwrap();

        super::write_bytes_atomic(&path, b"#!/bin/sh\necho new\n").unwrap();

        let mode = std::fs::metadata(&path).unwrap().permissions().mode();
        assert_eq!(mode & 0o777, 0o755);
    }
}
