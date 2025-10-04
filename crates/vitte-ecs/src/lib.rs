//! vitte-ecs — Entity Component System for Vitte
//!
//! Principes
//! - Données plates par composant (archetype-lite).
//! - Entities = u64 IDs.
//! - Components = stockés dans HashMaps ou Vecs par type.
//! - Systems = closures itérant sur ensembles de composants.
//!
//! Features
//! - `serde`: sérialisation des entités/composants.
//! - `parallel`: exécution des systèmes en parallèle via rayon.
//!
//! Exemple:
//! ```
//! use vitte_ecs::{World, Component};
//! #[derive(Debug)]
//! struct Pos(f32, f32);
//! impl Component for Pos {}
//! let mut w = World::new();
//! let e = w.spawn().with(Pos(1.0, 2.0)).id();
//! assert!(w.get::<Pos>(e).is_some());
//! ```

#![cfg_attr(not(feature = "std"), no_std)]
#![deny(missing_docs)]

extern crate alloc;
use alloc::{boxed::Box, vec::Vec};

use hashbrown::HashMap;

/// Identifiant d’entité.
pub type EntityId = u64;

/// Trait de composant.
pub trait Component: 'static + Send + Sync {}
impl<T: 'static + Send + Sync> Component for T {}

/// Structure stockant des composants pour un type donné.
struct ComponentStore {
    #[allow(dead_code)]
    // type_id unique
    type_id: core::any::TypeId,
    // box dyn HashMap<EntityId, Box<dyn Any>>
    data: Box<dyn AnyStore>,
}

/// Trait interne pour stocker dynamiquement.
trait AnyStore {
    fn remove(&mut self, id: EntityId);
    #[allow(dead_code)]
    fn clear(&mut self);
    #[allow(dead_code)]
    fn len(&self) -> usize;
    fn as_any(&self) -> &dyn core::any::Any;
    fn as_any_mut(&mut self) -> &mut dyn core::any::Any;
}

/// Implémentation de store concret pour un type C.
struct Store<C: Component> {
    map: HashMap<EntityId, C>,
}
impl<C: Component> Default for Store<C> {
    fn default() -> Self {
        Self { map: HashMap::new() }
    }
}
impl<C: Component> AnyStore for Store<C> {
    fn remove(&mut self, id: EntityId) { self.map.remove(&id); }
    fn clear(&mut self) { self.map.clear(); }
    fn len(&self) -> usize { self.map.len() }
    fn as_any(&self) -> &dyn core::any::Any { self }
    fn as_any_mut(&mut self) -> &mut dyn core::any::Any { self }
}

/// Entité builder.
pub struct EntityBuilder<'w> {
    id: EntityId,
    world: &'w mut World,
}
impl<'w> EntityBuilder<'w> {
    /// Ajoute un composant à l’entité.
    pub fn with<C: Component>(self, comp: C) -> Self {
        self.world.insert(self.id, comp);
        self
    }
    /// Retourne l’ID.
    pub fn id(self) -> EntityId { self.id }
}

/// Monde ECS.
pub struct World {
    next: EntityId,
    stores: HashMap<core::any::TypeId, ComponentStore>,
}
impl World {
    /// Crée un monde vide.
    pub fn new() -> Self {
        Self { next: 1, stores: HashMap::new() }
    }

    /// Crée une entité.
    pub fn spawn(&mut self) -> EntityBuilder<'_> {
        let id = self.next;
        self.next += 1;
        EntityBuilder { id, world: self }
    }

    /// Supprime une entité et ses composants.
    pub fn despawn(&mut self, id: EntityId) {
        for store in self.stores.values_mut() {
            store.data.remove(id);
        }
    }

    /// Insère un composant.
    pub fn insert<C: Component>(&mut self, id: EntityId, comp: C) {
        let tid = core::any::TypeId::of::<C>();
        self.stores.entry(tid).or_insert_with(|| ComponentStore {
            type_id: tid,
            data: Box::new(Store::<C>::default()),
        });
        let store = self.stores.get_mut(&tid).unwrap();
        let st = store.data.as_any_mut().downcast_mut::<Store<C>>().unwrap();
        st.map.insert(id, comp);
    }

    /// Récupère un composant.
    pub fn get<C: Component>(&self, id: EntityId) -> Option<&C> {
        let tid = core::any::TypeId::of::<C>();
        self.stores.get(&tid)
            .and_then(|store| store.data.as_any().downcast_ref::<Store<C>>())
            .and_then(|s| s.map.get(&id))
    }

    /// Récupère un composant mutable.
    pub fn get_mut<C: Component>(&mut self, id: EntityId) -> Option<&mut C> {
        let tid = core::any::TypeId::of::<C>();
        self.stores.get_mut(&tid)
            .and_then(|store| store.data.as_any_mut().downcast_mut::<Store<C>>())
            .and_then(|s| s.map.get_mut(&id))
    }

    /// Itère sur tous les (id, comp) d’un type.
    pub fn query<C: Component>(&self) -> impl Iterator<Item=(EntityId, &C)> {
        let tid = core::any::TypeId::of::<C>();
        self.stores.get(&tid)
            .and_then(|s| s.data.as_any().downcast_ref::<Store<C>>())
            .map(|st| st.map.iter().map(|(id, c)| (*id, c)))
            .into_iter()
            .flatten()
    }
}

/// Trait de système.
pub trait System {
    /// Exécute sur le monde.
    fn run(&mut self, world: &mut World);
}

impl<F: FnMut(&mut World)> System for F {
    fn run(&mut self, world: &mut World) { (self)(world) }
}

/// Ordonnanceur.
pub struct Scheduler {
    systems: Vec<Box<dyn System>>,
}
impl Scheduler {
    /// Crée un scheduler.
    pub fn new() -> Self { Self { systems: Vec::new() } }
    /// Ajoute un système.
    pub fn add<S: System + 'static>(mut self, s: S) -> Self {
        self.systems.push(Box::new(s));
        self
    }
    /// Exécute tous les systèmes.
    pub fn run(&mut self, world: &mut World) {
        #[cfg(feature = "parallel")]
        {
            use rayon::prelude::*;
            self.systems.par_iter_mut().for_each(|s| s.run(world));
        }
        #[cfg(not(feature = "parallel"))]
        {
            for s in &mut self.systems { s.run(world); }
        }
    }
}

/* -------------------------------- Tests --------------------------------- */

#[cfg(test)]
mod tests {
    use super::*;
    #[derive(Debug)]
    struct Pos(f32, f32);
    impl Component for Pos {}
    #[test]
    fn insert_and_query() {
        let mut w = World::new();
        let e = w.spawn().with(Pos(1.0, 2.0)).id();
        assert!(w.get::<Pos>(e).is_some());
        let mut sched = Scheduler::new().add(|world: &mut World| {
            for (_, p) in world.query::<Pos>() {
                assert_eq!(p.0, 1.0);
            }
        });
        sched.run(&mut w);
    }
}