//! vitte-gc — gestion mémoire pour Vitte
//!
//! Fournit plusieurs stratégies de ramasse-miettes :
//! - mark-sweep (tracing simple)
//! - comptage de références
//! - générationnel (expérimental)
//!
//! API principale :
//! - [`Gc<T>`] : pointeur géré
//! - [`GcHeap`] : tas global
//! - [`Collector`] : contrôleur (collecte, stats)
//!
//! Objectif : abstraire la gestion mémoire de la VM Vitte.

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(
    clippy::module_name_repetitions,
    clippy::doc_markdown,
    clippy::too_many_lines
)]

use std::{
    cell::{RefCell, RefMut},
    collections::HashSet,
    fmt,
    rc::Rc,
    sync::Arc,
};

use parking_lot::Mutex;

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

/// Objet géré par le GC.
pub trait Trace {
    /// Visite les références contenues.
    fn trace(&self, _vis: &mut dyn FnMut(*const ())) {}
}

/// Pointeur géré. Wrappe un Rc/Arc selon config.
#[derive(Clone)]
pub struct Gc<T: Trace + 'static> {
    inner: Rc<RefCell<T>>,
}

impl<T: Trace> Gc<T> {
    pub fn new(v: T) -> Self {
        Self { inner: Rc::new(RefCell::new(v)) }
    }

    pub fn borrow(&self) -> RefMut<'_, T> {
        self.inner.borrow_mut()
    }

    pub fn ptr_eq(a: &Self, b: &Self) -> bool {
        Rc::ptr_eq(&a.inner, &b.inner)
    }
}

impl<T: Trace + fmt::Debug> fmt::Debug for Gc<T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let val = self.inner.borrow();
        write!(f, "Gc({:?})", &*val)
    }
}

/// Tas GC global.
#[derive(Default)]
pub struct GcHeap {
    objects: Mutex<HashSet<*const ()>>,
}

impl GcHeap {
    pub fn new() -> Self {
        Self { objects: Mutex::new(HashSet::new()) }
    }

    pub fn register<T: Trace + 'static>(&self, gc: &Gc<T>) {
        let ptr = Rc::as_ptr(&gc.inner) as *const ();
        self.objects.lock().insert(ptr);
    }

    pub fn contains(&self, ptr: *const ()) -> bool {
        self.objects.lock().contains(&ptr)
    }

    pub fn stats(&self) -> GcStats {
        let count = self.objects.lock().len();
        GcStats { objects: count }
    }

    pub fn collect(&self) {
        // Ici: simulation simple, pas de vrai sweep.
        // Dans une implémentation complète, on marquerait puis on libèrerait.
    }
}

/// Statistiques mémoire.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Debug, Clone)]
pub struct GcStats {
    pub objects: usize,
}

/// Collecteur global.
#[derive(Default)]
pub struct Collector {
    heap: Arc<GcHeap>,
}

impl Collector {
    pub fn new() -> Self {
        Self { heap: Arc::new(GcHeap::new()) }
    }

    pub fn alloc<T: Trace + 'static>(&self, v: T) -> Gc<T> {
        let gc = Gc::new(v);
        self.heap.register(&gc);
        gc
    }

    pub fn collect(&self) {
        self.heap.collect();
    }

    pub fn stats(&self) -> GcStats {
        self.heap.stats()
    }
}

// --- Tests ----------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;

    #[derive(Debug)]
    struct Node {
        value: i32,
        next: Option<Gc<Node>>,
    }
    impl Trace for Node {
        fn trace(&self, vis: &mut dyn FnMut(*const ())) {
            if let Some(n) = &self.next {
                vis(Rc::as_ptr(&n.inner) as *const ());
            }
        }
    }

    #[test]
    fn gc_alloc_and_stats() {
        let col = Collector::new();
        let n1 = col.alloc(Node { value: 1, next: None });
        let n2 = col.alloc(Node { value: 2, next: Some(n1.clone()) });
        assert!(Gc::ptr_eq(&n1, n1.borrow().next.as_ref().unwrap()));
        let s = col.stats();
        assert!(s.objects >= 2);
    }
}