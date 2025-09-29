#![forbid(unsafe_code)]
#![cfg_attr(docsrs, feature(doc_cfg))]
//! # vitte-modules — agrégateur de modules optionnels
//!
//! Ce crate regroupe **des modules activables par features** et expose :
//! - un **prélu** (`prelude`) qui ré-exporte les types usuels de chaque module activé,
//! - un **registre statique** des modules compilés (`modules()`),
//! - des aides minimales d’initialisation (hooks vides pour l’instant).
//!
//! 👉 Avantage : côté binaire/outil, tu n’importes qu’un seul crate et tu
//! branches/compiles ce dont tu as besoin via `--features`.
//!
//! ## Features reconnues (exemples conseillés)
//! - `ecs2d`, `gfx2d`         — gameplay & rendu 2D
//! - `resolver`, `typer`      — frontend sémantique
//! - `opt`, `fmt`             — passes IR, formateur
//! - `hal`, `mm`, `shed`, `sys` — couches basses & orchestrateur d’outils
//!
//! Tu peux bien sûr en ajouter d’autres : **ajoute la réexport + l’entrée de
//! registre** plus bas (c’est trivial).
//!
//! ## Exemple
//! ```ignore
//! use vitte_modules::{prelude::*, ModuleMeta};
//!
//! fn main() {
//!     for m in vitte_modules::modules() {
//!         eprintln!("module: {} — {}", m.name, m.description);
//!     }
//!     // Si ecs2d/gfx2d sont activés :
//!     #[cfg(all(feature = "ecs2d", feature = "gfx2d"))]
//!     {
//!         let mut runner = ecs2d::Runner::new(MyGame);
//!         // let mut renderer = gfx2d::wgpu_backend::Wgpu2D::new_windowed("Vitte 2D", 800, 450).unwrap();
//!         let _t = runner.tick();
//!     }
//! }
//! ```

// ———————————————————————————————————————————————
// Réexports par module (garde ça simple et lisible)
// ———————————————————————————————————————————————

#[cfg(feature = "ecs2d")]
#[cfg_attr(docsrs, doc(cfg(feature = "ecs2d")))]
pub use vitte_ecs2d as ecs2d;

#[cfg(feature = "gfx2d")]
#[cfg_attr(docsrs, doc(cfg(feature = "gfx2d")))]
pub use vitte_gfx2d as gfx2d;

#[cfg(feature = "resolver")]
#[cfg_attr(docsrs, doc(cfg(feature = "resolver")))]
pub use vitte_resolver as resolver;

#[cfg(feature = "typer")]
#[cfg_attr(docsrs, doc(cfg(feature = "typer")))]
pub use vitte_typer as typer;

#[cfg(feature = "opt")]
#[cfg_attr(docsrs, doc(cfg(feature = "opt")))]
pub use vitte_opt as opt;

#[cfg(feature = "fmt")]
#[cfg_attr(docsrs, doc(cfg(feature = "fmt")))]
pub use vitte_fmt as vfmt;

// Kernel / bas niveau
#[cfg(feature = "hal")]
#[cfg_attr(docsrs, doc(cfg(feature = "hal")))]
pub use vitte_hal as hal;

#[cfg(feature = "mm")]
#[cfg_attr(docsrs, doc(cfg(feature = "mm")))]
pub use vitte_mm as mm;

#[cfg(feature = "shed")]
#[cfg_attr(docsrs, doc(cfg(feature = "shed")))]
pub use vitte_shed as shed;

#[cfg(feature = "sys")]
#[cfg_attr(docsrs, doc(cfg(feature = "sys")))]
pub use vitte_sys as sys;

// ———————————————————————————————————————————————
// Métadonnées & registre statique
// ———————————————————————————————————————————————

/// Métadonnées minimalistes d’un module compilé.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
pub struct ModuleMeta {
    /// Nom lisible (recommandé : le nom du crate “public”).
    pub name: &'static str,
    /// Brève description/extrait marketing.
    pub description: &'static str,
    /// Tag(s) de catégorie (affichage/filtre UI éventuel).
    pub tags: &'static [&'static str],
}

/// Retourne la **liste des modules** compilés (en fonction des features).
///
/// Ordre stable, pratique pour afficher dans un `--about` / `version`.
pub fn modules() -> &'static [ModuleMeta] {
    static REGS: &[ModuleMeta] = &REGISTRY;
    REGS
}

// Construit l’array à la compile en empilant conditionnellement les infos.
// Astuce : on colle les éléments dans une constante et on la réexporte plus haut.
const REGISTRY: [ModuleMeta; REG_LEN] = {
    // Compte le nombre d’entrées activées pour dimensionner l’array.
    const fn count() -> usize {
        #[allow(unused_mut)]
        let mut n = 0usize;
        // 2D
        #[cfg(feature = "ecs2d")]
        {
            let _ = ();
            n += 1;
        }
        #[cfg(feature = "gfx2d")]
        {
            let _ = ();
            n += 1;
        }
        // Frontend
        #[cfg(feature = "resolver")]
        {
            let _ = ();
            n += 1;
        }
        #[cfg(feature = "typer")]
        {
            let _ = ();
            n += 1;
        }
        #[cfg(feature = "opt")]
        {
            let _ = ();
            n += 1;
        }
        #[cfg(feature = "fmt")]
        {
            let _ = ();
            n += 1;
        }
        // Kernel
        #[cfg(feature = "hal")]
        {
            let _ = ();
            n += 1;
        }
        #[cfg(feature = "mm")]
        {
            let _ = ();
            n += 1;
        }
        #[cfg(feature = "shed")]
        {
            let _ = ();
            n += 1;
        }
        #[cfg(feature = "sys")]
        {
            let _ = ();
            n += 1;
        }
        n
    }
    const REG_LEN: usize = count();
    #[allow(unused_mut)]
    let mut arr: [ModuleMeta; REG_LEN] = {
        // initialise avec des valeurs “zéro” lisibles (jamais renvoyées si REG_LEN == 0)
        const ZZ: ModuleMeta = ModuleMeta { name: "", description: "", tags: &[] };
        [ZZ; REG_LEN]
    };
    #[allow(unused_mut, unused_assignments, unused_variables)]
    let mut i = 0usize;

    // 2D
    #[cfg(feature = "ecs2d")]
    {
        arr[i] = ModuleMeta {
            name: "ecs2d",
            description: "ECS 2D minimal (Transform, Velocity, Sprite, Camera).",
            tags: &["2d", "ecs", "game"],
        };
        i += 1;
    }
    #[cfg(feature = "gfx2d")]
    {
        arr[i] = ModuleMeta {
            name: "gfx2d",
            description: "Rendu 2D (batch sprites) backends wgpu/sdl2.",
            tags: &["2d", "gfx", "renderer"],
        };
        i += 1;
    }

    // Frontend
    #[cfg(feature = "resolver")]
    {
        arr[i] = ModuleMeta {
            name: "resolver",
            description: "Graphe de modules, imports, table des symboles.",
            tags: &["frontend", "names"],
        };
        i += 1;
    }
    #[cfg(feature = "typer")]
    {
        arr[i] = ModuleMeta {
            name: "typer",
            description: "Inférence & vérification des types (HM-friendly).",
            tags: &["frontend", "types"],
        };
        i += 1;
    }
    #[cfg(feature = "opt")]
    {
        arr[i] = ModuleMeta {
            name: "opt",
            description: "Passes IR (CFG/SSA, ConstFold, DCE, SimplifyCFG).",
            tags: &["ir", "opt"],
        };
        i += 1;
    }
    #[cfg(feature = "fmt")]
    {
        arr[i] = ModuleMeta {
            name: "fmt",
            description: "Formateur code source stable, idempotent.",
            tags: &["devx", "style"],
        };
        i += 1;
    }

    // Kernel
    #[cfg(feature = "hal")]
    {
        arr[i] = ModuleMeta {
            name: "hal",
            description: "HAL : timers, IRQ, MMIO abstraits (x86_64/aarch64).",
            tags: &["kernel", "hal"],
        };
        i += 1;
    }
    #[cfg(feature = "mm")]
    {
        arr[i] = ModuleMeta {
            name: "mm",
            description: "Gestion mémoire (frames/pages, heap noyau).",
            tags: &["kernel", "memory"],
        };
        i += 1;
    }
    #[cfg(feature = "shed")]
    {
        arr[i] = ModuleMeta {
            name: "shed",
            description: "Orchestrateur de tâches (build/test/watch).",
            tags: &["tools", "automation"],
        };
        i += 1;
    }
    #[cfg(feature = "sys")]
    {
        arr[i] = ModuleMeta {
            name: "sys",
            description: "ABI noyau : syscalls & traps de base.",
            tags: &["kernel", "abi"],
        };
        i += 1;
    }

    arr
};

const REG_LEN: usize = {
    // miroir de count() pour usage externe (const générique possible si besoin)
    #[allow(unused_mut)]
    let mut n = 0usize;
    #[cfg(feature = "ecs2d")]
    {
        let _ = ();
        n += 1;
    }
    #[cfg(feature = "gfx2d")]
    {
        let _ = ();
        n += 1;
    }
    #[cfg(feature = "resolver")]
    {
        let _ = ();
        n += 1;
    }
    #[cfg(feature = "typer")]
    {
        let _ = ();
        n += 1;
    }
    #[cfg(feature = "opt")]
    {
        let _ = ();
        n += 1;
    }
    #[cfg(feature = "fmt")]
    {
        let _ = ();
        n += 1;
    }
    #[cfg(feature = "hal")]
    {
        let _ = ();
        n += 1;
    }
    #[cfg(feature = "mm")]
    {
        let _ = ();
        n += 1;
    }
    #[cfg(feature = "shed")]
    {
        let _ = ();
        n += 1;
    }
    #[cfg(feature = "sys")]
    {
        let _ = ();
        n += 1;
    }
    n
};

// ———————————————————————————————————————————————
// Prélu : reexports “haut débit” (facultatifs)
// ———————————————————————————————————————————————

/// `use vitte_modules::prelude::*;` pour importer les types usuels
/// des modules activés. Garde ça *léger* (pas tout l’arbre).
pub mod prelude {
    #[allow(unused_imports)]
    pub use super::ModuleMeta;

    // 2D
    #[cfg(feature = "ecs2d")]
    pub use super::ecs2d::{
        Camera2D, Input, Runner as EcsRunner, Sprite, Time, Transform2D, Velocity,
        World as EcsWorld,
    };

    #[cfg(feature = "gfx2d")]
    pub use super::gfx2d::{Cam2D, DrawSprite, Renderer2D};

    // Frontend
    #[cfg(feature = "resolver")]
    pub use super::resolver as sem_resolver;

    #[cfg(feature = "typer")]
    pub use super::typer as sem_typer;

    #[cfg(feature = "opt")]
    pub use super::opt as ir_opt;

    #[cfg(feature = "fmt")]
    pub use super::vfmt as fmt;

    // Kernel
    #[cfg(feature = "hal")]
    pub use super::hal;

    #[cfg(feature = "mm")]
    pub use super::mm;

    #[cfg(feature = "shed")]
    pub use super::shed;

    #[cfg(feature = "sys")]
    pub use super::sys;
}

// ———————————————————————————————————————————————
// Hooks d’initialisation (simples, extensibles plus tard)
// ———————————————————————————————————————————————

/// Initialise *éventuellement* certains sous-systèmes.
/// Aujourd’hui no-op (à étendre lorsque des modules exposeront des inits).
pub fn init_all() {
    // Exemple futur :
    // #[cfg(feature = "gfx2d")]
    // gfx2d::init_logger_or_whatever();
}

/// Reverse d’`init_all` si tu ajoutes des ressources globales.
pub fn shutdown_all() {
    // pour l’instant : rien
}

// ———————————————————————————————————————————————
// Tests smoke (compile/lie, registre non-panique)
// ———————————————————————————————————————————————
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn registry_is_consistent() {
        let list = modules();
        // Pas d’éléments "vides"
        for m in list {
            assert!(!m.name.is_empty());
        }
        // Pas de panique en init/shutdown
        init_all();
        shutdown_all();
    }
}
