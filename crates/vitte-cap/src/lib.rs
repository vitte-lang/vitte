//! vitte-cap — gestion des capacités, permissions et sandboxing pour Vitte
//!
//! Buts:
//! - Modèle à capacités par *capabilities flags* + portée (ressource ciblée)
//! - Politique déclarative (allow/deny) sérialisable (TOML/JSON via `serde`)
//! - Vérificateur déterministe: `require(cap, resource)`
//! - Jetons signables (hors-scope cryptographie) pour délégation locale
//! - Journalisation optionnelle via un `AuditSink`
//!
//! Design notes:
//! - Par défaut, *deny-all*. On autorise si une règle ALLOW matche et aucune
//!   règle DENY plus spécifique ne s’y oppose.
//! - Spécificité: plus le motif est “long” (après normalisation), plus il est
//!   spécifique. En cas d’égalité, DENY > ALLOW.
//! - `no_std` support: activer la feature `no_std` (utilise `alloc`).

#![cfg_attr(feature = "no_std", no_std)]
#![forbid(unsafe_code)]

#[cfg(feature = "no_std")]
extern crate alloc;

use bitflags::bitflags;
use core::cmp::Ordering;

#[cfg(feature = "no_std")]
use alloc::{collections::BTreeMap, string::String, vec, vec::Vec};

#[cfg(not(feature = "no_std"))]
use std::{collections::BTreeMap, string::String, vec::Vec};

use serde::{Deserialize, Serialize};

#[cfg(feature = "std")]
use thiserror::Error;

// ---------------------------------------------------------------------------
// Erreurs
// ---------------------------------------------------------------------------

#[cfg_attr(feature = "std", derive(Error))]
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub enum CapError {
    #[cfg_attr(feature = "std", error("capability manquante: {0:?}"))]
    Missing(Capability),
    #[cfg_attr(feature = "std", error("accès refusé: {cap:?} → {res:?} ({reason})"))]
    Denied { cap: Capability, res: Resource, reason: String },
    #[cfg_attr(feature = "std", error("jeton expiré"))]
    TokenExpired,
    #[cfg_attr(feature = "std", error("politique vide: deny-all"))]
    EmptyPolicy,
}

#[cfg(not(feature = "std"))]
impl core::fmt::Display for CapError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            CapError::Missing(c) => write!(f, "missing {:?}", c),
            CapError::Denied { cap, res, reason } => {
                write!(f, "denied {:?} -> {:?}: {}", cap, res, reason)
            }
            CapError::TokenExpired => write!(f, "token expired"),
            CapError::EmptyPolicy => write!(f, "empty policy"),
        }
    }
}

// ---------------------------------------------------------------------------
// Capabilities
// ---------------------------------------------------------------------------

bitflags! {
    #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
    pub struct Caps: u64 {
        const FS_READ       = 1 << 0;
        const FS_WRITE      = 1 << 1;
        const NET_CONNECT   = 1 << 2;
        const PROC_SPAWN    = 1 << 3;
        const EXEC          = 1 << 4;
        const RAND          = 1 << 5;
        const TIME          = 1 << 6;
        const ENV_READ      = 1 << 7;
        const ENV_WRITE     = 1 << 8;
        const IPC           = 1 << 9;
        const FFI           = 1 << 10;
        const DEVICE        = 1 << 11;
        const UI            = 1 << 12;
        const CUSTOM_0      = 1 << 13;
        const CUSTOM_1      = 1 << 14;
        const CUSTOM_2      = 1 << 15;
        const ALL           = u64::MAX;
    }
}

impl serde::Serialize for Caps {
    fn serialize<S>(&self, serializer: S) -> core::result::Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        serializer.serialize_u64(self.bits())
    }
}

impl<'de> serde::Deserialize<'de> for Caps {
    fn deserialize<D>(deserializer: D) -> core::result::Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        let bits = u64::deserialize(deserializer)?;
        Caps::from_bits(bits).ok_or_else(|| serde::de::Error::custom("invalid caps bits"))
    }
}

/// Enum pratique pour messages d’erreur humains.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum Capability {
    FsRead,
    FsWrite,
    NetConnect,
    ProcSpawn,
    Exec,
    Rand,
    Time,
    EnvRead,
    EnvWrite,
    Ipc,
    Ffi,
    Device,
    Ui,
    Custom(u8),
}

impl Capability {
    pub fn to_flag(self) -> Caps {
        match self {
            Capability::FsRead => Caps::FS_READ,
            Capability::FsWrite => Caps::FS_WRITE,
            Capability::NetConnect => Caps::NET_CONNECT,
            Capability::ProcSpawn => Caps::PROC_SPAWN,
            Capability::Exec => Caps::EXEC,
            Capability::Rand => Caps::RAND,
            Capability::Time => Caps::TIME,
            Capability::EnvRead => Caps::ENV_READ,
            Capability::EnvWrite => Caps::ENV_WRITE,
            Capability::Ipc => Caps::IPC,
            Capability::Ffi => Caps::FFI,
            Capability::Device => Caps::DEVICE,
            Capability::Ui => Caps::UI,
            Capability::Custom(0) => Caps::CUSTOM_0,
            Capability::Custom(1) => Caps::CUSTOM_1,
            Capability::Custom(2) => Caps::CUSTOM_2,
            Capability::Custom(_) => Caps::CUSTOM_2, // coalesced
        }
    }
}

// ---------------------------------------------------------------------------
// Ressources et motifs
// ---------------------------------------------------------------------------

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub enum Resource {
    FilePath(String),                            // ex: "/var/data/x.txt"
    NetHost { host: String, port: Option<u16> }, // "example.com", 443
    EnvVar(String),                              // "HOME"
    Process(String),                             // "convert"
    Custom { kind: String, id: String },
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub enum Pattern {
    /// Préfixe normalisé. Pour fichiers: chemin absolu ou canonique
    Prefix(String),
    /// Glob simple avec * unique (optionnel). Ex: "example.*"
    Glob(String),
    /// Égalité stricte
    Exact(String),
    /// Match toujours vrai pour ce type
    Any,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct ResourceSelector {
    pub kind: SelectorKind,
    pub pat: Pattern,
    /// Contrainte additionnelle (ex: port)
    #[serde(default)]
    pub meta: BTreeMap<String, String>,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum SelectorKind {
    File,
    NetHost,
    Env,
    Proc,
    Custom,
}

impl ResourceSelector {
    pub fn matches(&self, res: &Resource) -> bool {
        match (self.kind, res) {
            (SelectorKind::File, Resource::FilePath(p)) => pat_match(&self.pat, p),
            (SelectorKind::Env, Resource::EnvVar(k)) => pat_match(&self.pat, k),
            (SelectorKind::Proc, Resource::Process(n)) => pat_match(&self.pat, n),
            (SelectorKind::NetHost, Resource::NetHost { host, port }) => {
                let host_ok = pat_match(&self.pat, host);
                let port_ok = if let Some(expect) = self.meta.get("port") {
                    if let Ok(ep) = expect.parse::<u16>() {
                        Some(ep) == *port
                    } else {
                        false
                    }
                } else {
                    true
                };
                host_ok && port_ok
            }
            (SelectorKind::Custom, Resource::Custom { kind, id }) => {
                let k_ok = self.meta.get("kind").map(|k| k == kind).unwrap_or(true);
                let id_ok = pat_match(&self.pat, id);
                k_ok && id_ok
            }
            _ => false,
        }
    }

    /// Mesure de “spécificité” naïve: longueur de la chaîne interrogée.
    pub fn specificity(&self) -> usize {
        match &self.pat {
            Pattern::Any => 0,
            Pattern::Exact(s) | Pattern::Prefix(s) | Pattern::Glob(s) => s.len(),
        }
    }
}

fn pat_match(p: &Pattern, s: &str) -> bool {
    match p {
        Pattern::Any => true,
        Pattern::Exact(t) => s == t,
        Pattern::Prefix(t) => s.starts_with(t),
        Pattern::Glob(t) => {
            // Supporte au plus un '*'
            if let Some(pos) = t.find('*') {
                let (a, b) = t.split_at(pos);
                let b = &b[1..]; // remove '*'
                s.len() >= a.len() + b.len() && s.starts_with(a) && s.ends_with(b)
            } else {
                s == t
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Règles et politique
// ---------------------------------------------------------------------------

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum Effect {
    Allow,
    Deny,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct Rule {
    pub effect: Effect,
    /// Flags autorisés/interdits par cette règle
    pub caps: Caps,
    /// Sélecteur de ressource
    pub selector: ResourceSelector,
    /// Commentaire optionnel
    #[serde(default)]
    pub note: String,
}

impl Rule {
    pub fn applies(&self, cap: Capability, res: &Resource) -> bool {
        self.caps.contains(cap.to_flag()) && self.selector.matches(res)
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Policy {
    /// Règles ordonnées. La résolution tient compte de la spécificité.
    pub rules: Vec<Rule>,
    /// Si aucune règle ne matche: deny
    #[serde(default = "Policy::default_deny")]
    pub deny_by_default: bool,
}

impl Default for Policy {
    fn default() -> Self {
        Self { rules: Vec::new(), deny_by_default: true }
    }
}

impl Policy {
    fn default_deny() -> bool {
        true
    }

    /// Résout une décision.
    pub fn decide(&self, cap: Capability, res: &Resource) -> Decision {
        // On sélectionne les règles qui s’appliquent.
        let mut hits: Vec<(&Rule, usize)> = self
            .rules
            .iter()
            .filter(|r| r.applies(cap, res))
            .map(|r| (r, r.selector.specificity()))
            .collect();

        if hits.is_empty() {
            return if self.deny_by_default {
                Decision::Denied { reason: "default-deny".into(), matched: None }
            } else {
                Decision::Allowed { matched: None }
            };
        }

        // Trie par spécificité desc. En cas d’égalité: DENY > ALLOW.
        hits.sort_by(|(a, sa), (b, sb)| {
            match sb.cmp(sa) {
                Ordering::Less => Ordering::Less, // sa > sb → a avant
                Ordering::Greater => Ordering::Greater,
                Ordering::Equal => match (a.effect, b.effect) {
                    (Effect::Deny, Effect::Allow) => Ordering::Less,
                    (Effect::Allow, Effect::Deny) => Ordering::Greater,
                    _ => Ordering::Equal,
                },
            }
        });

        let (top, _) = hits[0];
        match top.effect {
            Effect::Allow => Decision::Allowed { matched: Some(top.clone()) },
            Effect::Deny => {
                Decision::Denied { reason: top.note.clone(), matched: Some(top.clone()) }
            }
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Decision {
    Allowed { matched: Option<Rule> },
    Denied { reason: String, matched: Option<Rule> },
}

// ---------------------------------------------------------------------------
// Jetons (délégation locale)
// ---------------------------------------------------------------------------

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Grant {
    pub caps: Caps,
    pub selector: ResourceSelector,
    /// Epoch seconds pour expiration optionnelle
    #[serde(default)]
    pub not_after_epoch: Option<u64>,
}

impl Grant {
    pub fn expired(&self, now_epoch: u64) -> bool {
        self.not_after_epoch.map(|t| now_epoch > t).unwrap_or(false)
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Token {
    pub grants: Vec<Grant>,
    /// Métadonnées libres
    #[serde(default)]
    pub meta: BTreeMap<String, String>,
}

impl Token {
    pub fn allows(&self, now_epoch: u64, cap: Capability, res: &Resource) -> bool {
        self.grants.iter().any(|g| {
            !g.expired(now_epoch) && g.caps.contains(cap.to_flag()) && g.selector.matches(res)
        })
    }
}

// ---------------------------------------------------------------------------
// Audit
// ---------------------------------------------------------------------------

/// Destination de journalisation.
pub trait AuditSink {
    fn on_allow(&self, cap: Capability, res: &Resource, rule: Option<&Rule>);
    fn on_deny(&self, cap: Capability, res: &Resource, rule: Option<&Rule>, reason: &str);
}

/// Audit factice.
pub struct NoopAudit;
impl AuditSink for NoopAudit {
    fn on_allow(&self, _cap: Capability, _res: &Resource, _rule: Option<&Rule>) {}
    fn on_deny(&self, _cap: Capability, _res: &Resource, _rule: Option<&Rule>, _reason: &str) {}
}

// ---------------------------------------------------------------------------
// Vérificateur
// ---------------------------------------------------------------------------

#[derive(Debug, Clone)]
pub struct Checker<A: AuditSink> {
    pub policy: Policy,
    pub token: Option<Token>,
    pub audit: A,
    /// Horloge externe en secondes Unix
    pub now_epoch: u64,
}

impl<A: AuditSink> Checker<A> {
    pub fn new(policy: Policy, audit: A) -> Self {
        Self { policy, token: None, audit, now_epoch: 0 }
    }

    pub fn with_time(mut self, now_epoch: u64) -> Self {
        self.now_epoch = now_epoch;
        self
    }

    pub fn with_token(mut self, tok: Token) -> Self {
        self.token = Some(tok);
        self
    }

    /// Vérifie et retourne Ok(()) si autorisé.
    pub fn require(&self, cap: Capability, res: Resource) -> Result<(), CapError> {
        // 1) Jeton prioritaire: il peut surclasser la politique de base si ALLOW.
        if let Some(tok) = &self.token {
            if tok.allows(self.now_epoch, cap, &res) {
                self.audit.on_allow(cap, &res, None);
                return Ok(());
            }
            // Si le jeton existe mais est expiré et aucun grant valide, signale.
            if tok.grants.iter().any(|g| g.not_after_epoch.is_some())
                && tok.grants.iter().all(|g| g.expired(self.now_epoch))
            {
                self.audit.on_deny(cap, &res, None, "token-expired");
                return Err(CapError::TokenExpired);
            }
        }

        // 2) Politique
        if self.policy.rules.is_empty() && self.policy.deny_by_default {
            self.audit.on_deny(cap, &res, None, "empty-policy");
            return Err(CapError::EmptyPolicy);
        }

        match self.policy.decide(cap, &res) {
            Decision::Allowed { matched } => {
                self.audit.on_allow(cap, &res, matched.as_ref());
                Ok(())
            }
            Decision::Denied { reason, matched } => {
                self.audit.on_deny(cap, &res, matched.as_ref(), &reason);
                Err(CapError::Denied { cap, res, reason })
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Construire des règles rapidement (helpers)
// ---------------------------------------------------------------------------

pub mod prelude {
    pub use super::{
        AuditSink, CapError, Capability, Caps, Checker, Decision, Effect, Grant, NoopAudit,
        Pattern, Policy, Resource, ResourceSelector, SelectorKind, Token,
    };

    /// Raccourcis:
    use super::{BTreeMap, Pattern as P, ResourceSelector as S, SelectorKind as K};
    pub fn file_prefix(prefix: &str) -> S {
        S { kind: K::File, pat: P::Prefix(prefix.into()), meta: Default::default() }
    }
    pub fn file_exact(path: &str) -> S {
        S { kind: K::File, pat: P::Exact(path.into()), meta: Default::default() }
    }
    pub fn host(host: &str, port: Option<u16>) -> S {
        let mut m: BTreeMap<String, String> = BTreeMap::new();
        if let Some(p) = port {
            m.insert("port".into(), p.to_string());
        }
        S { kind: K::NetHost, pat: P::Exact(host.into()), meta: m }
    }
    pub fn env_glob(glob: &str) -> S {
        S { kind: K::Env, pat: P::Glob(glob.into()), meta: Default::default() }
    }
    pub fn proc_name(name: &str) -> S {
        S { kind: K::Proc, pat: P::Exact(name.into()), meta: Default::default() }
    }
    pub fn custom(kind: &str, id_pat: P) -> S {
        let mut m: BTreeMap<String, String> = BTreeMap::new();
        m.insert("kind".into(), kind.into());
        S { kind: K::Custom, pat: id_pat, meta: m }
    }
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;
    use prelude::*;

    fn policy_example() -> Policy {
        Policy {
            rules: vec![
                Rule {
                    effect: Effect::Allow,
                    caps: Caps::FS_READ,
                    selector: file_prefix("/opt/data"),
                    note: "read-only data".into(),
                },
                Rule {
                    effect: Effect::Deny,
                    caps: Caps::FS_WRITE,
                    selector: file_prefix("/opt/data"),
                    note: "no writes in data".into(),
                },
                Rule {
                    effect: Effect::Allow,
                    caps: Caps::NET_CONNECT,
                    selector: host("example.com", Some(443)),
                    note: "api".into(),
                },
                Rule {
                    effect: Effect::Deny,
                    caps: Caps::NET_CONNECT,
                    selector: host("example.com", Some(80)),
                    note: "no http".into(),
                },
                Rule {
                    effect: Effect::Allow,
                    caps: Caps::ENV_READ,
                    selector: env_glob("RUST_*"),
                    note: "rust env".into(),
                },
            ],
            deny_by_default: true,
        }
    }

    #[test]
    fn allow_read_in_prefix() {
        let chk = Checker { policy: policy_example(), token: None, audit: NoopAudit, now_epoch: 0 };
        chk.require(Capability::FsRead, Resource::FilePath("/opt/data/file.txt".into())).unwrap();
    }

    #[test]
    fn deny_write_in_prefix() {
        let chk = Checker { policy: policy_example(), token: None, audit: NoopAudit, now_epoch: 0 };
        let err = chk
            .require(Capability::FsWrite, Resource::FilePath("/opt/data/file.txt".into()))
            .unwrap_err();
        match err {
            CapError::Denied { .. } => {}
            _ => panic!("expected deny"),
        }
    }

    #[test]
    fn net_tls_ok_http_denied() {
        let chk = Checker { policy: policy_example(), token: None, audit: NoopAudit, now_epoch: 0 };
        chk.require(
            Capability::NetConnect,
            Resource::NetHost { host: "example.com".into(), port: Some(443) },
        )
        .unwrap();
        assert!(matches!(
            chk.require(
                Capability::NetConnect,
                Resource::NetHost { host: "example.com".into(), port: Some(80) }
            ),
            Err(CapError::Denied { .. })
        ));
    }

    #[test]
    fn token_overrides_policy_if_valid() {
        let pol = Policy { rules: vec![], deny_by_default: true };
        let grant = Grant {
            caps: Caps::FS_READ,
            selector: prelude::file_exact("/secret.txt"),
            not_after_epoch: Some(1000),
        };
        let tok = Token { grants: vec![grant], meta: Default::default() };
        let chk = Checker { policy: pol, token: Some(tok), audit: NoopAudit, now_epoch: 999 };
        chk.require(Capability::FsRead, Resource::FilePath("/secret.txt".into())).unwrap();
    }

    #[test]
    fn token_expired() {
        let pol = Policy { rules: vec![], deny_by_default: true };
        let grant = Grant {
            caps: Caps::FS_READ,
            selector: prelude::file_exact("/secret.txt"),
            not_after_epoch: Some(1000),
        };
        let tok = Token { grants: vec![grant], meta: Default::default() };
        let chk = Checker { policy: pol, token: Some(tok), audit: NoopAudit, now_epoch: 2000 };
        let err =
            chk.require(Capability::FsRead, Resource::FilePath("/secret.txt".into())).unwrap_err();
        assert!(matches!(err, CapError::TokenExpired));
    }

    #[test]
    fn serde_roundtrip_policy() {
        let pol = policy_example();
        let s = serde_json::to_string_pretty(&pol).unwrap();
        let de: Policy = serde_json::from_str(&s).unwrap();
        assert_eq!(pol.rules.len(), de.rules.len());
    }
}
