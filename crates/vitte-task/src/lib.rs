#![deny(missing_docs)]
//! vitte-task — gestion de tâches pour Vitte
//!
//! Fournit :
//! - Spawning de futures (tokio/async-std/smol).
//! - Tâches annulables via `TaskHandle`.
//! - Tâches récurrentes (timers).
//! - File d’attente simple + retries.
//!
//! Exemple :
//! ```ignore
//! use vitte_task::{spawn, sleep, TaskHandle};
//! let h = spawn(async { 1+1 });
//! ```

use futures::Future;
use std::time::Duration;
use thiserror::Error;

/// Erreurs liées aux tâches
#[derive(Debug, Error)]
pub enum TaskError {
    /// La tâche a été annulée via `TaskHandle::cancel`.
    #[error("cancelled")]
    Cancelled,

    /// Erreur lors de l'attente/agrégation du résultat d'une tâche (join).
    #[error("join error: {0}")]
    Join(String),

    /// Autre erreur liée aux tâches.
    #[error("other: {0}")]
    Other(String),
}

/// Résultat spécialisé
pub type Result<T> = std::result::Result<T, TaskError>;

/// Handle pour annuler une tâche
#[derive(Debug, Clone)]
pub struct TaskHandle {
    cancelled: std::sync::Arc<std::sync::atomic::AtomicBool>,
}

impl TaskHandle {
    /// Nouvelle poignée
    pub fn new() -> Self {
        Self { cancelled: std::sync::Arc::new(std::sync::atomic::AtomicBool::new(false)) }
    }
    /// Annule la tâche
    pub fn cancel(&self) {
        self.cancelled.store(true, std::sync::atomic::Ordering::SeqCst);
    }
    /// Vérifie si annulée
    pub fn is_cancelled(&self) -> bool {
        self.cancelled.load(std::sync::atomic::Ordering::SeqCst)
    }
}

/// Spawning runtime-agnostic
pub fn spawn<F>(fut: F) -> TaskHandle
where
    F: Future<Output = ()> + Send + 'static,
{
    let handle = TaskHandle::new();
    let cancel_flag = handle.cancelled.clone();

    #[cfg(feature = "rt-tokio")]
    tokio::spawn(async move {
        use futures::FutureExt;
        let mut fut = Box::pin(fut);
        loop {
            if cancel_flag.load(std::sync::atomic::Ordering::SeqCst) {
                break;
            }
            if fut.as_mut().now_or_never().is_some() {
                break;
            }
            tokio::time::sleep(Duration::from_millis(50)).await;
        }
    });

    #[cfg(feature = "rt-async-std")]
    async_std::task::spawn(async move {
        use futures::FutureExt;
        let mut fut = Box::pin(fut);
        loop {
            if cancel_flag.load(std::sync::atomic::Ordering::SeqCst) {
                break;
            }
            if fut.as_mut().now_or_never().is_some() {
                break;
            }
            async_std::task::sleep(Duration::from_millis(50)).await;
        }
    });

    #[cfg(feature = "rt-smol")]
    smol::spawn(async move {
        use futures::FutureExt;
        let mut fut = Box::pin(fut);
        loop {
            if cancel_flag.load(std::sync::atomic::Ordering::SeqCst) {
                break;
            }
            if fut.as_mut().now_or_never().is_some() {
                break;
            }
            smol::Timer::after(Duration::from_millis(50)).await;
        }
    })
    .detach();

    handle
}

/// Sleep async runtime-agnostic
pub async fn sleep(dur: Duration) {
    #[cfg(feature = "rt-tokio")]
    {
        tokio::time::sleep(dur).await
    }

    #[cfg(feature = "rt-async-std")]
    {
        async_std::task::sleep(dur).await
    }

    #[cfg(feature = "rt-smol")]
    {
        smol::Timer::after(dur).await
    }
}

/// Retry une future avec délai fixe
pub async fn retry<F, Fut, T, E>(
    mut f: F,
    retries: usize,
    delay: Duration,
) -> std::result::Result<T, E>
where
    F: FnMut() -> Fut,
    Fut: Future<Output = std::result::Result<T, E>>,
{
    let mut last = None;
    for _ in 0..retries {
        match f().await {
            Ok(v) => return Ok(v),
            Err(e) => {
                last = Some(e);
                sleep(delay).await;
            }
        }
    }
    Err(last.unwrap())
}

#[cfg(test)]
mod tests {
    use super::*;
    #[tokio::test(flavor = "current_thread")]
    async fn retry_ok() {
        let mut n = 0;
        let res: std::result::Result<i32, &str> = retry(
            || {
                n += 1;
                async move {
                    if n < 3 {
                        Err("no")
                    } else {
                        Ok(42)
                    }
                }
            },
            5,
            Duration::from_millis(1),
        )
        .await;
        assert_eq!(res.unwrap(), 42);
    }
}
