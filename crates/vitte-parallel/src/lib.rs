#![deny(missing_docs)]
//! vitte-parallel — primitives de parallélisme pour Vitte
//!
//! Fournit :
//! - API de thread pool simple
//! - Iterateurs parallèles (rayon si activé)
//! - File de tâches avec crossbeam si activé
//! - Gestion des erreurs typées

use std::sync::mpsc;
use std::sync::{Arc, Mutex};
use std::thread;
use thiserror::Error;

/// Erreurs de parallélisme.
#[derive(Debug, Error)]
pub enum ParallelError {
    /// Erreur survenue lors du join d'un thread.
    #[error("join error: {0}")]
    Join(String),
    /// Erreur liée aux canaux de communication.
    #[error("channel error")]
    Channel,
}

/// Résultat spécialisé.
pub type Result<T> = std::result::Result<T, ParallelError>;

/// ThreadPool minimaliste.
pub struct ThreadPool {
    workers: Vec<thread::JoinHandle<()>>,
    sender: Option<mpsc::Sender<Box<dyn FnOnce() + Send + 'static>>>,
}

impl ThreadPool {
    /// Crée un pool de taille donnée.
    pub fn new(size: usize) -> Self {
        assert!(size > 0);
        let (tx, rx) = mpsc::channel::<Box<dyn FnOnce() + Send>>();
        let rx = Arc::new(Mutex::new(rx));
        let mut workers = Vec::with_capacity(size);
        for _ in 0..size {
            let rx = Arc::clone(&rx);
            let h = thread::spawn(move || {
                while let Ok(job) = rx.lock().unwrap().recv() {
                    job();
                }
            });
            workers.push(h);
        }
        Self { workers, sender: Some(tx) }
    }

    /// Exécute une tâche.
    pub fn execute<F>(&self, f: F)
    where
        F: FnOnce() + Send + 'static,
    {
        if let Some(s) = &self.sender {
            let _ = s.send(Box::new(f));
        }
    }
}

impl Drop for ThreadPool {
    fn drop(&mut self) {
        if let Some(s) = self.sender.take() {
            drop(s);
        }
        for h in self.workers.drain(..) {
            let _ = h.join();
        }
    }
}

/// API parallèle basée sur Rayon si activé.
#[cfg(feature = "rayon")]
pub mod rayon_api {
    pub use rayon::prelude::*;
}

/// API de file de tâches basée sur crossbeam si activé.
#[cfg(feature = "crossbeam")]
pub mod task {
    use super::*;
    use crossbeam::channel;

    /// Spawns plusieurs tâches et collecte résultats.
    pub fn spawn_collect<T, F>(jobs: Vec<F>) -> Vec<T>
    where
        F: FnOnce() -> T + Send + 'static,
        T: Send + 'static,
    {
        let (tx, rx) = channel::unbounded();
        for job in jobs {
            let tx = tx.clone();
            thread::spawn(move || {
                let res = job();
                let _ = tx.send(res);
            });
        }
        drop(tx);
        rx.iter().collect()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn pool_executes() {
        let pool = ThreadPool::new(2);
        let counter = Arc::new(Mutex::new(0));
        for _ in 0..10 {
            let c = Arc::clone(&counter);
            pool.execute(move || {
                *c.lock().unwrap() += 1;
            });
        }
        drop(pool);
        assert_eq!(*counter.lock().unwrap(), 10);
    }

    #[cfg(feature = "crossbeam")]
    #[test]
    fn spawn_collect_works() {
        let jobs: Vec<Box<dyn FnOnce() -> i32 + Send>> =
            (0..5).map(|i| Box::new(move || i * 2)).collect();
        let res = task::spawn_collect(jobs.into_iter().map(|f| f).collect::<Vec<_>>());
        assert_eq!(res.len(), 5);
    }
}
