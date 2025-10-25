//! vitte-resolve — résolution de dépendances simple et déterministe
//!
//! Objectif: sélectionner une version unique par paquet qui satisfait toutes
//! les contraintes de version, avec stratégie "plus récent compatible".
//!
//! Conçu pour être indépendant du transport: on définit un `Provider` abstrait
//! qui expose les manifests et les versions disponibles.

#![forbid(unsafe_code)]
#![deny(rust_2018_idioms, unused_must_use, missing_docs)]

use core::fmt;
use std::collections::{BTreeMap, BTreeSet, HashMap, VecDeque};

use semver::{Version, VersionReq};
use thiserror::Error;

/// Identité d'un paquet (nom + version concrète)
#[derive(Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Debug)]
pub struct PackageId {
    /// Nom canonique
    pub name: String,
    /// Version concrète
    pub version: Version,
}

impl PackageId {
    /// Crée un `PackageId`.
    pub fn new<N: Into<String>>(name: N, version: Version) -> Self {
        Self { name: name.into(), version }
    }
}

impl fmt::Display for PackageId {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}@{}", self.name, self.version)
    }
}

/// Dépendance déclarée dans un manifest
#[derive(Clone, Debug)]
pub struct Dependency {
    /// Nom du paquet requis
    pub name: String,
    /// Contrainte de version (semver)
    pub req: VersionReq,
    /// Indique si la dépendance est optionnelle
    pub optional: bool,
    /// Nom des features demandées (opaque pour le résolveur)
    pub features: BTreeSet<String>,
}

impl Dependency {
    /// Dépendance obligatoire avec contrainte
    pub fn required<N: Into<String>>(name: N, req: VersionReq) -> Self {
        Self { name: name.into(), req, optional: false, features: BTreeSet::new() }
    }
}

/// Manifest minimal d'un paquet
#[derive(Clone, Debug)]
pub struct Manifest {
    /// Identité du paquet
    pub id: PackageId,
    /// Dépendances directes
    pub dependencies: Vec<Dependency>,
}

impl Manifest {
    /// Crée un manifest.
    pub fn new(id: PackageId, dependencies: Vec<Dependency>) -> Self { Self { id, dependencies } }
}

/// Interface d'accès aux paquets et versions
pub trait Provider {
    /// Retourne toutes les versions publiées pour `name` (désordonnées ou non).
    fn available_versions(&self, name: &str) -> Result<Vec<Version>, ResolveError>;
    /// Charge le manifest pour une version précise.
    fn manifest(&self, name: &str, version: &Version) -> Result<Manifest, ResolveError>;
}

/// Options de résolution
#[derive(Clone, Copy, Debug, Default)]
pub struct ResolveOptions {
    /// Autoriser les pré‐versions si la contrainte les accepte explicitement
    pub allow_prerelease: bool,
    /// Si plusieurs versions satisfont, choisir la plus récente
    pub prefer_latest: bool,
}

/// Erreurs possibles lors de la résolution
#[derive(Error, Debug)]
pub enum ResolveError {
    /// Aucun paquet au nom donné
    #[error("package not found: {0}")]
    NotFound(String),
    /// Aucune version ne satisfait la contrainte
    #[error("no matching version for {name} satisfying {req}")]
    NoMatchingVersion { name: String, req: String },
    /// Conflit de contraintes entre dépendances
    #[error("conflict on {name}: cannot find a single version that satisfies all constraints: {constraints}")]
    Conflict { name: String, constraints: String },
    /// Erreur d'accès au provider
    #[error(transparent)]
    Provider(#[from] Box<dyn std::error::Error + Send + Sync>),
}

/// Résultat gelé de la résolution
#[derive(Clone, Debug)]
pub struct Lockfile {
    /// Paquets résolus par nom
    pub packages: BTreeMap<String, PackageId>,
    /// Arêtes du graphe: nom -> dépendances concrètes
    pub edges: BTreeMap<String, BTreeSet<String>>, // clés = name@version pour stabilité
    /// Ordre topologique au besoin (racine d'abord)
    pub topo: Vec<PackageId>,
}

impl Lockfile {
    /// Récupère l'identité par nom
    pub fn get(&self, name: &str) -> Option<&PackageId> { self.packages.get(name) }
}

/// Résolveur principal
pub struct Resolver<'p, P: Provider> {
    provider: &'p P,
    opts: ResolveOptions,
}

impl<'p, P: Provider> Resolver<'p, P> {
    /// Construit un résolveur
    pub fn new(provider: &'p P) -> Self { Self { provider, opts: ResolveOptions::default() } }
    /// Avec options
    pub fn with_options(mut self, opts: ResolveOptions) -> Self { self.opts = opts; self }

    /// Résout à partir d'un manifest racine
    pub fn resolve(&self, root: &Manifest) -> Result<Lockfile, ResolveError> {
        // Contraintes cumulées par nom
        let mut constraints: HashMap<String, Vec<VersionReq>> = HashMap::new();
        // Sélection courante par nom
        let mut selected: BTreeMap<String, Version> = BTreeMap::new();
        // Graphe: clé lisible "name@version" -> ses dépendances clés
        let mut edges: BTreeMap<String, BTreeSet<String>> = BTreeMap::new();
        // Manifests cache
        let mut manifest_cache: HashMap<(String, Version), Manifest> = HashMap::new();

        // Initialiser avec les dépendances racine
        let mut queue: VecDeque<(String, VersionReq)> = VecDeque::new();
        for d in root.dependencies.iter().filter(|d| !d.optional) {
            constraints.entry(d.name.clone()).or_default().push(d.req.clone());
            queue.push_back((d.name.clone(), d.req.clone()));
        }

        // La racine figure implicitement dans le lock
        selected.insert(root.id.name.clone(), root.id.version.clone());
        edges.entry(format!("{}@{}", root.id.name, root.id.version)).or_default();

        while let Some((name, _req)) = queue.pop_front() {
            // Calculer la meilleure version qui satisfait toutes les contraintes du paquet `name`
            let wants = constraints.get(&name).cloned().unwrap_or_default();
            let pick = self.pick_version(&name, &wants, &selected)?;

            let already = selected.get(&name);
            if let Some(v) = already {
                if v != &pick {
                    // Si la version déjà choisie ne convient plus, c'est un vrai conflit
                    // car `pick_version` tient compte de toutes les contraintes connues.
                    return Err(ResolveError::Conflict {
                        name: name.clone(),
                        constraints: wants.iter().map(|r| r.to_string()).collect::<Vec<_>>().join(", "),
                    });
                }
            } else {
                selected.insert(name.clone(), pick.clone());
            }

            // Charger le manifest concret
            let key = (name.clone(), pick.clone());
            let man = if let Some(m) = manifest_cache.get(&key) { m.clone() } else {
                let m = self.provider.manifest(&name, &pick)?;
                manifest_cache.insert(key.clone(), m.clone());
                m
            };

            // Enregistrer les arêtes du graphe pour ce noeud
            let from_key = format!("{}@{}", man.id.name, man.id.version);
            let e = edges.entry(from_key.clone()).or_default();

            for dep in man.dependencies.iter().filter(|d| !d.optional) {
                constraints.entry(dep.name.clone()).or_default().push(dep.req.clone());
                queue.push_back((dep.name.clone(), dep.req.clone()));

                // La cible sera résolue avec sa version finale; on n'a pas encore la version ici.
                // On remplira la clé finale quand le paquet cible aura une version choisie.
                // Pour garder la structure stable, on stocke pour l'instant un placeholder basé sur le nom;
                // on mettra à jour après le tour principal.
                e.insert(dep.name.clone());
            }
        }

        // Remplacer les placeholders par des clés name@version maintenant que `selected` est complet.
        let mut resolved_edges: BTreeMap<String, BTreeSet<String>> = BTreeMap::new();
        for (from, tos) in edges.into_iter() {
            let mut finals = BTreeSet::new();
            for to_name in tos {
                if let Some(v) = selected.get(&to_name) {
                    finals.insert(format!("{}@{}", to_name, v));
                } else {
                    // Dépendance déclarée mais non résolue: impossible dans l'algo actuel
                    return Err(ResolveError::NotFound(to_name));
                }
            }
            resolved_edges.insert(from, finals);
        }

        // Construire l'ordre topologique simple par DFS
        let mut topo = topo_sort(&resolved_edges)?;
        // Garder uniquement les `PackageId` pour les noeuds présents
        let mut packages: BTreeMap<String, PackageId> = BTreeMap::new();
        for (name, ver) in &selected {
            packages.insert(name.clone(), PackageId::new(name.clone(), ver.clone()));
        }
        // Filtrer topo vers PackageId connus
        topo.retain(|pid| packages.contains_key(&pid.name));

        Ok(Lockfile { packages, edges: resolved_edges, topo })
    }

    fn pick_version(
        &self,
        name: &str,
        reqs: &[VersionReq],
        _selected: &BTreeMap<String, Version>,
    ) -> Result<Version, ResolveError> {
        let mut versions = self.provider.available_versions(name)?;
        if versions.is_empty() {
            return Err(ResolveError::NotFound(name.to_string()));
        }
        // Trier décroissant pour "plus récent compatible"
        versions.sort();
        versions.reverse();

        // Filtrer par prérelease et contraintes
        let allow_pr = self.opts.allow_prerelease || reqs.iter().any(req_allows_prerelease);
        let matches = versions.into_iter().filter(|v| {
            if !allow_pr && v.is_prerelease() { return false; }
            reqs.iter().all(|r| r.matches(v))
        });

        // Choix
        for v in matches {
            return Ok(v);
        }

        Err(ResolveError::NoMatchingVersion { name: name.to_string(), req: reqs_to_string(reqs) })
    }
}

fn req_allows_prerelease(r: &VersionReq) -> bool {
    // Heuristique: si une quelconque prédicat contient une version prérelease
    // on considère que le demandeur accepte les pré‐versions correspondantes.
    // `semver` ne fournit pas d'API directe pour ça.
    r.to_string().contains('-')
}

fn reqs_to_string(reqs: &[VersionReq]) -> String {
    if reqs.is_empty() { return "*".into(); }
    reqs.iter().map(|r| r.to_string()).collect::<Vec<_>>().join(" && ")
}

/// Topo tri simple sur des clés `name@version`
fn topo_sort(edges: &BTreeMap<String, BTreeSet<String>>) -> Result<Vec<PackageId>, ResolveError> {
    // Calcul des degrés entrants
    let mut indeg: HashMap<&str, usize> = HashMap::new();
    for (from, tos) in edges.iter() {
        indeg.entry(from.as_str()).or_insert(0);
        for to in tos { *indeg.entry(to.as_str()).or_insert(0) += 1; }
    }

    // Kahn
    let mut q: VecDeque<&str> = indeg.iter().filter_map(|(k, &d)| if d == 0 { Some(*k) } else { None }).collect();
    let mut out: Vec<PackageId> = Vec::new();

    while let Some(n) = q.pop_front() {
        let pid = parse_key(n);
        out.push(pid);
        if let Some(tos) = edges.get(n) {
            for to in tos {
                if let Some(d) = indeg.get_mut(to.as_str()) {
                    *d -= 1;
                    if *d == 0 { q.push_back(to); }
                }
            }
        }
    }

    if out.len() != indeg.len() {
        // Cycle détecté; produire un message concis
        return Err(ResolveError::Provider("cyclic dependency detected".into()));
    }

    Ok(out)
}

fn parse_key(key: &str) -> PackageId {
    // key = name@version
    if let Some((name, ver)) = key.rsplit_once('@') {
        let v = Version::parse(ver).unwrap_or_else(|_| Version::new(0, 0, 0));
        return PackageId::new(name.to_string(), v);
    }
    PackageId::new(key.to_string(), Version::new(0, 0, 0))
}

// ------------------------------ Tests ---------------------------------------
#[cfg(test)]
mod tests {
    use super::*;
    use std::sync::Arc;

    #[derive(Default)]
    struct MemoryProvider {
        // name -> versions disponibles
        versions: HashMap<String, BTreeSet<Version>>, // tri croissant
        // (name, version) -> manifest
        manifests: HashMap<(String, Version), Manifest>,
    }

    impl MemoryProvider {
        fn insert_pkg(&mut self, name: &str, version: &str, deps: &[(&str, &str)]) {
            let v = Version::parse(version).unwrap();
            self.versions.entry(name.to_string()).or_default().insert(v.clone());
            let m = Manifest::new(
                PackageId::new(name.to_string(), v.clone()),
                deps.iter()
                    .map(|(n, r)| Dependency::required((*n).to_string(), VersionReq::parse(r).unwrap()))
                    .collect(),
            );
            self.manifests.insert((name.to_string(), v), m);
        }
    }

    impl Provider for MemoryProvider {
        fn available_versions(&self, name: &str) -> Result<Vec<Version>, ResolveError> {
            self.versions
                .get(name)
                .map(|s| s.iter().cloned().collect())
                .ok_or_else(|| ResolveError::NotFound(name.to_string()))
        }
        fn manifest(&self, name: &str, version: &Version) -> Result<Manifest, ResolveError> {
            self.manifests
                .get(&(name.to_string(), version.clone()))
                .cloned()
                .ok_or_else(|| ResolveError::NotFound(format!("{}@{}", name, version)))
        }
    }

    #[test]
    fn resolves_simple_tree() {
        let mut p = MemoryProvider::default();
        p.insert_pkg("root", "1.0.0", &[ ("a", ">=1.0, <2.0"), ("b", "^1.2") ]);
        p.insert_pkg("a", "1.0.0", &[]);
        p.insert_pkg("a", "1.1.0", &[]);
        p.insert_pkg("b", "1.2.0", &[("a", ">=1.1, <2.0")]);
        p.insert_pkg("b", "1.3.0", &[("a", ">=1.1, <2.0")]);

        let root = p.manifest("root", &Version::parse("1.0.0").unwrap()).unwrap();
        let r = Resolver::new(&p).resolve(&root).unwrap();

        assert_eq!(r.get("a").unwrap().version, Version::parse("1.1.0").unwrap());
        assert_eq!(r.get("b").unwrap().version, Version::parse("1.3.0").unwrap());
    }

    #[test]
    fn detects_conflict() {
        let mut p = MemoryProvider::default();
        p.insert_pkg("root", "1.0.0", &[ ("a", ">=1.0, <2.0"), ("b", "^2.0") ]);
        p.insert_pkg("a", "1.5.0", &[("x", "^1.0")]);
        p.insert_pkg("b", "2.0.0", &[("x", "^2.0")]);
        p.insert_pkg("x", "1.1.0", &[]);
        p.insert_pkg("x", "2.0.0", &[]);

        let root = p.manifest("root", &Version::parse("1.0.0").unwrap()).unwrap();
        let err = Resolver::new(&p).resolve(&root).unwrap_err();
        match err {
            ResolveError::Conflict { name, .. } => assert_eq!(name, "x"),
            _ => panic!("unexpected error: {err}")
        }
    }

    #[test]
    fn forbids_prerelease_by_default() {
        let mut p = MemoryProvider::default();
        p.insert_pkg("root", "1.0.0", &[("a", ">=1.0, <2.0")]);
        p.insert_pkg("a", "1.1.0-alpha.1", &[]);
        p.insert_pkg("a", "1.0.1", &[]);

        let root = p.manifest("root", &Version::parse("1.0.0").unwrap()).unwrap();
        let r = Resolver::new(&p).resolve(&root).unwrap();
        assert_eq!(r.get("a").unwrap().version, Version::parse("1.0.1").unwrap());

        let r2 = Resolver::new(&p).with_options(ResolveOptions { allow_prerelease: true, prefer_latest: true }).resolve(&root).unwrap();
        assert_eq!(r2.get("a").unwrap().version, Version::parse("1.1.0-alpha.1").unwrap());
    }
}
