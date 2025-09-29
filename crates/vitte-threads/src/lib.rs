//! vitte-threads — abstractions de threads et parallélisme pour Vitte
//!
//! Couvre trois niveaux :
//! 1) **Threads standards**: spawn/join, join-all simple.
//! 2) **Pool de threads** *(feature `pool`)*: exécution parallèle, `parallel_map`/`parallel_for`.
//! 3) **Async Tokio** *(feature `async`)*: runtime multi-thread prêt à l’emploi.
//!
//! Fournit aussi des **canaux** typés (crossbeam) avec API compacte.
//!
//! Exemple rapide :
//! ```ignore
//! use vitte_threads::{spawn, join_all, chan, parallel_map};
//!
//! let h = spawn(|| 21 * 2);
//! let out = h.join().unwrap();
//!
//! let (tx, rx) = chan::unbounded::<u32>();
//! tx.send(1).unwrap(); assert_eq!(rx.recv().unwrap(), 1);
//!
//! let xs = vec![1,2,3,4];
//! let ys = parallel_map(&xs, |x| x * 2); // rayon si activé, sinon séquentiel
//! assert_eq!(ys, vec![2,4,6,8]);
//! ```

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(
    clippy::module_name_repetitions,
    clippy::doc_markdown,
    clippy::too_many_lines,
    clippy::missing_errors_doc
)]

use std::thread;

/// Raccourci pour `std::thread::spawn` avec contraintes Send + 'static.
pub fn spawn<F, T>(f: F) -> thread::JoinHandle<T>
where
    F: FnOnce() -> T + Send + 'static,
    T: Send + 'static,
{
    thread::spawn(f)
}

/// Joint une collection de `JoinHandle`.
pub fn join_all<T>(handles: Vec<thread::JoinHandle<T>>) -> Vec<std::thread::Result<T>> {
    handles.into_iter().map(|h| h.join()).collect()
}

/* ======================================================================================
   Canaux (crossbeam-channel)
   ====================================================================================== */

/// Canaux typés simplifiés.
pub mod chan {
    use crossbeam_channel as cb;

    /// Émetteur typé.
    pub type Sender<T> = cb::Sender<T>;
    /// Récepteur typé.
    pub type Receiver<T> = cb::Receiver<T>;

    /// Canal sans borne.
    #[must_use]
    pub fn unbounded<T>() -> (Sender<T>, Receiver<T>) {
        cb::unbounded()
    }

    /// Canal borné.
    #[must_use]
    pub fn bounded<T>(cap: usize) -> (Sender<T>, Receiver<T>) {
        cb::bounded(cap)
    }

    /// Proxy `select!` minimal (bloquant) qui tente `recv` sur plusieurs receveurs et
    /// renvoie le premier message disponible avec son index.
    ///
    /// Retourne `None` si tous les canaux sont déconnectés.
    pub fn recv_any<T: Clone>(rxs: &[Receiver<T>]) -> Option<(usize, T)> {
        if rxs.is_empty() {
            return None;
        }
        loop {
            for (i, r) in rxs.iter().enumerate() {
                if let Ok(v) = r.try_recv() {
                    return Some((i, v));
                }
            }
            // end condition: if all disconnected and empty
            if rxs.iter().all(|r| r.is_empty() && r.is_disconnected()) {
                return None;
            }
            // fallback to blocking on the first
            if let Ok(v) = rxs[0].recv() {
                return Some((0, v));
            }
        }
    }
}

/* ======================================================================================
   Parallélisme par pool (rayon)
   ====================================================================================== */

/// Primitives haut niveau basées sur **rayon** si `feature = "pool"`, sinon fallback séquentiel.
///
/// Fonctions libres:
/// - [`parallel_for`]
/// - [`parallel_map`]
/// - [`parallel_map_into`]
///
/// Avec `feature = "pool"`, un type [`ThreadPool`] est exposé pour contrôler la taille.
#[cfg(feature = "pool")]
pub mod pool {
    use rayon::{ThreadPool as RayonPool, ThreadPoolBuilder};
    use std::sync::Arc;

    /// Pool de threads configurable.
    #[derive(Clone)]
    pub struct ThreadPool {
        inner: Arc<RayonPool>,
    }

    impl ThreadPool {
        /// Crée un pool avec `num_threads`. Si `None`, rayon choisit automatiquement.
        pub fn new(num_threads: Option<usize>) -> anyhow::Result<Self> {
            let mut b = ThreadPoolBuilder::new();
            if let Some(n) = num_threads {
                b = b.num_threads(n);
            }
            let inner = b.build().map_err(|e| anyhow::anyhow!(e.to_string()))?;
            Ok(Self { inner: Arc::new(inner) })
        }

        /// Exécute une fermeture dans le pool.
        pub fn spawn<F: FnOnce() + Send + 'static>(&self, f: F) {
            self.inner.spawn(f);
        }

        /// Exécute `f(i)` pour `i in 0..len` en parallèle.
        pub fn for_range<F: Fn(usize) + Sync + Send>(&self, len: usize, f: F) {
            self.inner.install(|| {
                (0..len).into_par_iter().for_each(|i| f(i));
            });
        }

        /// Map parallèle avec allocation de sortie.
        pub fn map<I, O, F>(&self, input: &[I], f: F) -> Vec<O>
        where
            I: Sync,
            O: Send,
            F: Fn(&I) -> O + Sync,
        {
            self.inner.install(|| input.par_iter().map(|x| f(x)).collect())
        }
    }

    // Re-export ergonomique du trait ParIter.
    use rayon::prelude::{IntoParallelIterator, ParallelIterator, ParallelSlice};

    /// Exécute `f(i)` pour `i in 0..len` en parallèle.
    pub fn parallel_for<F: Fn(usize) + Sync + Send>(len: usize, f: F) {
        (0..len).into_par_iter().for_each(f);
    }

    /// Applique `f` à chaque élément, renvoie une `Vec` des résultats.
    pub fn parallel_map<I: Sync, O: Send, F: Fn(&I) -> O + Sync>(input: &[I], f: F) -> Vec<O> {
        input.par_iter().map(|x| f(x)).collect()
    }

    /// Variante consommant un itérateur en le collectant en parallèle. Pratique pour gros flux.
    pub fn parallel_map_into<T, O, It, F>(it: It, f: F) -> Vec<O>
    where
        It: IntoParallelIterator<Item = T>,
        O: Send,
        F: Fn(T) -> O + Sync + Send,
    {
        it.into_par_iter().map(f).collect()
    }
}

#[cfg(not(feature = "pool"))]
mod fallback_seq {
    /// Fallback séquentiel si `pool` n'est pas activée.
    pub fn parallel_for<F: Fn(usize) + Sync + Send>(len: usize, f: F) {
        for i in 0..len {
            f(i);
        }
    }
    pub fn parallel_map<I, O, F: Fn(&I) -> O + Sync>(input: &[I], f: F) -> Vec<O> {
        input.iter().map(|x| f(x)).collect()
    }
    pub fn parallel_map_into<T, O, It, F>(it: It, f: F) -> Vec<O>
    where
        It: IntoIterator<Item = T>,
        F: Fn(T) -> O,
    {
        it.into_iter().map(f).collect()
    }
}

#[cfg(feature = "pool")]
pub use pool::{parallel_for, parallel_map, parallel_map_into};
#[cfg(not(feature = "pool"))]
pub use fallback_seq::{parallel_for, parallel_map, parallel_map_into};

/// Re-export optionnel du type `ThreadPool` si `pool` activée.
#[cfg(feature = "pool")]
pub use pool::ThreadPool;

/* ======================================================================================
   Async (Tokio)
   ====================================================================================== */

/// Aides pour lancer un runtime Tokio multi-thread très simplement.
#[cfg(feature = "async")]
pub mod rt {
    use tokio::runtime::{Builder, Handle, Runtime};

    /// Wrapper léger autour d’un `tokio::Runtime`.
    pub struct AsyncRuntime {
        rt: Runtime,
    }

    impl AsyncRuntime {
        /// Crée un runtime multi-thread. Si `worker_threads` est `None`, Tokio choisit.
        pub fn new(worker_threads: Option<usize>) -> anyhow::Result<Self> {
            let mut b = Builder::new_multi_thread().enable_all();
            if let Some(n) = worker_threads {
                b.worker_threads(n);
            }
            Ok(Self { rt: b.build()? })
        }

        /// Exécute un futur jusqu’à complétion.
        pub fn block_on<F: std::future::Future>(&self, fut: F) -> F::Output {
            self.rt.block_on(fut)
        }

        /// `Handle` sur le runtime pour `spawn`.
        pub fn handle(&self) -> Handle {
            self.rt.handle().clone()
        }
    }

    /// Raccourci qui exécute un futur avec un runtime ad-hoc de courte durée.
    pub fn block_on<F: std::future::Future>(fut: F) -> F::Output {
        let rt = AsyncRuntime::new(None).expect("tokio runtime");
        rt.block_on(fut)
    }
}

/* ======================================================================================
   Utilitaires de haut niveau
   ====================================================================================== */

/// Transforme un slice en résultats via `parallel_map`.
pub fn map_parallel<I: Sync, O: Send, F: Fn(&I) -> O + Sync>(input: &[I], f: F) -> Vec<O> {
    parallel_map(input, f)
}

/// Exécute `f(i)` pour `i in 0..len` via `parallel_for`.
pub fn for_parallel<F: Fn(usize) + Sync + Send>(len: usize, f: F) {
    parallel_for(len, f);
}

/* ======================================================================================
   Tests
   ====================================================================================== */

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn join_all_works() {
        let hs = (0..4).map(|i| spawn(move || i + 1)).collect();
        let out = join_all(hs);
        assert_eq!(out.into_iter().map(Result::unwrap).sum::<i32>(), 10);
    }

    #[test]
    fn channels_basic() {
        let (tx, rx) = crate::chan::unbounded::<i32>();
        tx.send(7).unwrap();
        assert_eq!(rx.recv().unwrap(), 7);
    }

    #[test]
    fn parallel_map_fallback_or_pool() {
        let xs: Vec<i32> = (0..1000).collect();
        let ys = parallel_map(&xs, |x| x + 1);
        assert_eq!(ys[0], 1);
        assert_eq!(ys[999], 1000);
    }

    #[cfg(feature = "pool")]
    #[test]
    fn threadpool_custom_size() {
        let tp = ThreadPool::new(Some(2)).unwrap();
        let v = tp.map(&(0..1000).collect::<Vec<_>>(), |x| x * 2);
        assert_eq!(v[10], 20);
    }

    #[cfg(feature = "async")]
    #[test]
    fn tokio_runtime_runs() {
        use crate::rt::AsyncRuntime;
        let rt = AsyncRuntime::new(Some(2)).unwrap();
        let x = rt.block_on(async { 1 + 2 });
        assert_eq!(x, 3);
    }
}