//! vitte-package — primitives liées au packaging et à la distribution.
//!
//! Ce crate est pour l’instant un stub afin que les autres composants puissent
//! évoluer en attendant l’implémentation réelle du gestionnaire de paquets.

#![forbid(unsafe_code)]
#![deny(missing_docs)]

/// Identifiant d’un paquet Vitte.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct PackageId(pub String);

/// Métadonnées minimales d’un paquet.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct PackageMeta {
    /// Nom affiché.
    pub name: String,
    /// Version semver.
    pub version: String,
}

impl PackageMeta {
    /// Construit une métadonnée triviale.
    pub fn new(name: impl Into<String>, version: impl Into<String>) -> Self {
        Self { name: name.into(), version: version.into() }
    }
}

/// Stub : résout un paquet en retournant des métadonnées artificielles.
pub fn resolve_stub(id: &PackageId) -> PackageMeta {
    PackageMeta::new(&id.0, "0.0.0-stub")
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn resolve_stub_returns_version() {
        let meta = resolve_stub(&PackageId("vitte-core".into()));
        assert_eq!(meta.version, "0.0.0-stub");
    }
}
