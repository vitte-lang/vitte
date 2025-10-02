#![deny(missing_docs)]
//! vitte-semver — utilitaires de gestion de versions SemVer pour Vitte
//!
//! Fournit :
//! - Parsing et validation SemVer (via crate `semver`).
//! - Comparaison de versions, contraintes.
//! - Incréments major/minor/patch.
//! - Compatibilité entre deux versions.
//! - Sérialisation optionnelle (`serde`).
//!
//! Exemple rapide :
//! ```
//! use vitte_semver as vs;
//! let v = vs::parse("1.2.3").unwrap();
//! assert_eq!(vs::inc_patch(&v).to_string(), "1.2.4");
//! ```

use thiserror::Error;
use semver::{Version, VersionReq};

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

/// Erreurs SemVer.
#[derive(Debug, Error)]
pub enum SemverError {
    #[error("parse error: {0}")]
    Parse(String),
    #[error("requirement error: {0}")]
    Req(String),
    #[error("other: {0}")]
    Other(String),
}

/// Résultat spécialisé.
pub type Result<T> = std::result::Result<T, SemverError>;

/// Parse une chaîne en version.
pub fn parse(s: &str) -> Result<Version> {
    Version::parse(s).map_err(|e| SemverError::Parse(e.to_string()))
}

/// Parse une contrainte de version.
pub fn parse_req(s: &str) -> Result<VersionReq> {
    VersionReq::parse(s).map_err(|e| SemverError::Req(e.to_string()))
}

/// Incrémente la version majeure.
pub fn inc_major(v: &Version) -> Version {
    Version {
        major: v.major + 1,
        minor: 0,
        patch: 0,
        pre: v.pre.clone(),
        build: v.build.clone(),
    }
}

/// Incrémente la version mineure.
pub fn inc_minor(v: &Version) -> Version {
    Version {
        major: v.major,
        minor: v.minor + 1,
        patch: 0,
        pre: v.pre.clone(),
        build: v.build.clone(),
    }
}

/// Incrémente le patch.
pub fn inc_patch(v: &Version) -> Version {
    Version {
        major: v.major,
        minor: v.minor,
        patch: v.patch + 1,
        pre: v.pre.clone(),
        build: v.build.clone(),
    }
}

/// Retourne vrai si `a` satisfait la contrainte `req`.
pub fn matches(a: &Version, req: &VersionReq) -> bool {
    req.matches(a)
}

/// Vérifie compatibilité (par défaut, mêmes major).
pub fn compatible(a: &Version, b: &Version) -> bool {
    a.major == b.major
}

/// Sérialisation wrapper optionnel.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct VersionString(pub String);

impl VersionString {
    /// Parse la string en Version.
    pub fn as_version(&self) -> Result<Version> {
        parse(&self.0)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parse_and_inc() {
        let v = parse("1.2.3").unwrap();
        assert_eq!(inc_patch(&v).to_string(), "1.2.4");
        assert_eq!(inc_minor(&v).to_string(), "1.3.0");
        assert_eq!(inc_major(&v).to_string(), "2.0.0");
    }

    #[test]
    fn req_match() {
        let v = parse("1.2.3").unwrap();
        let r = parse_req(">=1.0,<2.0").unwrap();
        assert!(matches(&v, &r));
    }

    #[test]
    fn compat_check() {
        let v1 = parse("1.2.3").unwrap();
        let v2 = parse("1.9.9").unwrap();
        assert!(compatible(&v1, &v2));
    }
}
