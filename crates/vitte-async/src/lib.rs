//! vitte-async — Façade async runtime-agnostic pour Vitte.
//!
//! Couvre : `spawn`, `block_on`, `sleep`, `timeout`, `interval`, canaux (`oneshot`, `mpsc`),
//! jointures (`join`, `try_join`), `select`, `CancellationToken`, et un alias `main` quand possible.
//!
//! Runtimes supportés (features) :
//! - `rt-tokio`      → Tokio
//! - `rt-async-std`  → async-std
//! - `rt-smol`       → smol
//!
//! Sans runtime sélectionné, fallback minimal basé sur `futures::executor`.
//!
//! Exemple simple :
//! ```no_run
//! use vitte_async::*;
//! async fn work() { sleep(std::time::Duration::from_millis(10)).await; }
//! fn main() {
//!     block_on(async {
//!         let j = spawn(work());
//!         j.await.unwrap();
//!     });
//! }
//! ```

#![forbid(unsafe_code)]
#![deny(missing_docs)]

use core::{fmt, future::Future, pin::Pin, task::{Context, Poll}};
use std::{sync::Arc, time::Duration};
use parking_lot::Mutex;
use thiserror::Error;

/* ============================== Sélection runtime ============================== */

/// Nom du runtime sélectionné à la compilation.
pub const RUNTIME: &str = {
    #[cfg(feature = "rt-tokio")]
    { "tokio" }
    #[cfg(all(not(feature = "rt-tokio"), feature = "rt-async-std"))]
    { "async-std" }
    #[cfg(all(not(feature = "rt-tokio"), not(feature = "rt-async-std"), feature = "rt-smol"))]
    { "smol" }
    #[cfg(all(not(feature = "rt-tokio"), not(feature = "rt-async-std"), not(feature = "rt-smol")))]
    { "fallback" }
};

/* ============================== Erreurs communes ============================== */

/// Erreurs async communes.
#[derive(Debug, Error)]
pub enum AsyncError {
    /// Le futur a expiré.
    #[error("timeout after {0:?}")]
    Timeout(Duration),
    /// La tâche a été annulée ou join a échoué.
    #[error("join error")]
    Join,
    /// Opération annulée par un `CancellationToken`.
    #[error("cancelled")]
    Cancelled,
}

/* ============================== Re-export utiles ============================== */

pub use futures::{
    future::{join, try_join, BoxFuture, FutureExt},
    stream::{Stream, StreamExt},
    select, pin_mut,
};

/* ============================== block_on ============================== */

/// Exécute un futur jusqu’à complétion sur le runtime actif ou un fallback.
pub fn block_on<F: Future>(fut: F) -> F::Output {
    #[cfg(feature = "rt-tokio")]
    {
        // Essaye d’utiliser un handle courant, sinon crée un runtime local.
        if let Ok(handle) = tokio::runtime::Handle::try_current() {
            return handle.block_on(fut);
        }
        let rt = tokio::runtime::Builder::new_current_thread().enable_all().build().expect("tokio rt");
        rt.block_on(fut)
    }
    #[cfg(all(not(feature = "rt-tokio"), feature = "rt-async-std"))]
    {
        async_std::task::block_on(fut)
    }
    #[cfg(all(not(feature = "rt-tokio"), not(feature = "rt-async-std"), feature = "rt-smol"))]
    {
        smol::block_on(fut)
    }
    #[cfg(all(not(feature = "rt-tokio"), not(feature = "rt-async-std"), not(feature = "rt-smol")))]
    {
        futures::executor::block_on(fut)
    }
}

/* ============================== JoinHandle ============================== */

/// Poignée de tâche portable.
pub struct JoinHandle<T>(JoinInner<T>);

enum JoinInner<T> {
    #[cfg(feature = "rt-tokio")]
    Tokio(tokio::task::JoinHandle<T>),
    #[cfg(all(not(feature = "rt-tokio"), feature = "rt-async-std"))]
    AsyncStd(async_std::task::JoinHandle<T>),
    #[cfg(all(not(feature = "rt-tokio"), not(feature = "rt-async-std"), feature = "rt-smol"))]
    Smol(smol::Task<T>),
    Fallback(FallbackJoin<T>),
}

impl<T> fmt::Debug for JoinHandle<T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result { write!(f, "JoinHandle<{:?}>", core::any::type_name::<T>()) }
}

impl<T> JoinHandle<T> {
    /// Annule la tâche si possible.
    pub fn abort(&self) {
        match &self.0 {
            #[cfg(feature = "rt-tokio")]
            JoinInner::Tokio(h) => h.abort(),
            #[cfg(all(not(feature = "rt-tokio"), feature = "rt-async-std"))]
            JoinInner::AsyncStd(h) => h.cancel(),
            #[cfg(all(not(feature = "rt-tokio"), not(feature = "rt-async-std"), feature = "rt-smol"))]
            JoinInner::Smol(h) => h.cancel(),
            JoinInner::Fallback(h) => h.abort(),
        }
    }
}

impl<T> Future for JoinHandle<T> {
    type Output = Result<T, AsyncError>;
    fn poll(self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Self::Output> {
        // safety: enum is Unpin by design
        let this = unsafe { self.get_unchecked_mut() };
        match &mut this.0 {
            #[cfg(feature = "rt-tokio")]
            JoinInner::Tokio(h) => match Pin::new(h).poll(cx) {
                Poll::Ready(Ok(v)) => Poll::Ready(Ok(v)),
                Poll::Ready(Err(_)) => Poll::Ready(Err(AsyncError::Join)),
                Poll::Pending => Poll::Pending,
            },
            #[cfg(all(not(feature = "rt-tokio"), feature = "rt-async-std"))]
            JoinInner::AsyncStd(h) => match Pin::new(h).poll(cx) {
                Poll::Ready(v) => Poll::Ready(Ok(v)),
                Poll::Pending => Poll::Pending,
            },
            #[cfg(all(not(feature = "rt-tokio"), not(feature = "rt-async-std"), feature = "rt-smol"))]
            JoinInner::Smol(h) => match Pin::new(h).poll(cx) {
                Poll::Ready(v) => Poll::Ready(Ok(v)),
                Poll::Pending => Poll::Pending,
            },
            JoinInner::Fallback(h) => h.poll(cx),
        }
    }
}

/* ============================== spawn ============================== */

/// Lance une tâche concurrente sur le runtime actif ou le fallback.
pub fn spawn<F, T>(fut: F) -> JoinHandle<T>
where
    F: Future<Output = T> + Send + 'static,
    T: Send + 'static,
{
    #[cfg(feature = "rt-tokio")]
    {
        return JoinHandle(JoinInner::Tokio(tokio::spawn(fut)));
    }
    #[cfg(all(not(feature = "rt-tokio"), feature = "rt-async-std"))]
    {
        return JoinHandle(JoinInner::AsyncStd(async_std::task::spawn(fut)));
    }
    #[cfg(all(not(feature = "rt-tokio"), not(feature = "rt-async-std"), feature = "rt-smol"))]
    {
        return JoinHandle(JoinInner::Smol(smol::spawn(fut)));
    }
    JoinHandle(JoinInner::Fallback(FallbackJoin::spawn(fut)))
}

/* ============================== sleep/interval/timeout ============================== */

/// Endort la tâche courante pour `dur`.
pub async fn sleep(dur: Duration) {
    #[cfg(feature = "rt-tokio")]
    { tokio::time::sleep(dur).await; }
    #[cfg(all(not(feature = "rt-tokio"), feature = "rt-async-std"))]
    { async_std::task::sleep(dur).await; }
    #[cfg(all(not(feature = "rt-tokio"), not(feature = "rt-async-std"), feature = "rt-smol"))]
    { smol::Timer::after(dur).await; }
    #[cfg(all(not(feature = "rt-tokio"), not(feature = "rt-async-std"), not(feature = "rt-smol")))]
    { futures_timer::Delay::new(dur).await; }
}

/// Répète des ticks espacés de `period`.
pub fn interval(period: Duration) -> impl Stream<Item = ()> {
    use futures::{stream, StreamExt as _};
    // Impl portable via unfold + sleep
    stream::unfold((), move |_| {
        let p = period;
        async move {
            sleep(p).await;
            Some(((), ()))
        }
    })
}

/// Exécute `fut` avec limite de temps.
pub async fn timeout<F: Future>(dur: Duration, fut: F) -> Result<F::Output, AsyncError> {
    #[cfg(feature = "rt-tokio")]
    {
        match tokio::time::timeout(dur, fut).await {
            Ok(v) => Ok(v),
            Err(_) => Err(AsyncError::Timeout(dur)),
        }
    }
    #[cfg(all(not(feature = "rt-tokio"), feature = "rt-async-std"))]
    {
        match async_std::future::timeout(dur, fut).await {
            Ok(v) => Ok(v),
            Err(_) => Err(AsyncError::Timeout(dur)),
        }
    }
    #[cfg(all(not(feature = "rt-tokio"), not(feature = "rt-async-std"), feature = "rt-smol"))]
    {
        match smol::future::or(smol::Timer::after(dur), fut).await {
            smol::future::Either::Right(v) => Ok(v),
            smol::future::Either::Left(_) => Err(AsyncError::Timeout(dur)),
        }
    }
    #[cfg(all(not(feature = "rt-tokio"), not(feature = "rt-async-std"), not(feature = "rt-smol")))]
    {
        use futures::{future::Either, FutureExt as _};
        match futures::future::select(futures_timer::Delay::new(dur), fut).await {
            Either::Right((v, _)) => Ok(v),
            Either::Left((_t, _)) => Err(AsyncError::Timeout(dur)),
        }
    }
}

/* ============================== Canaux ============================== */

pub mod channel {
    //! Canaux portables basés sur `futures::channel`.

    use super::*;

    /// oneshot (single-producer single-consumer).
    pub mod oneshot {
        pub use futures::channel::oneshot::{channel, Canceled, Receiver, Sender};
    }

    /// mpsc multi-producteur.
    pub mod mpsc {
        pub use futures::channel::mpsc::{channel, unbounded, Receiver, Sender};
    }
}

/* ============================== CancellationToken ============================== */

/// Jeton d’annulation clonable. Réveil asynchrone pour tous les waiters.
#[derive(Clone)]
pub struct CancellationToken {
    inner: Arc<CancellationInner>,
}

struct CancellationInner {
    cancelled: std::sync::atomic::AtomicBool,
    notify: Mutex<Vec<std::task::Waker>>,
}

impl Default for CancellationToken {
    fn default() -> Self { Self::new() }
}

impl CancellationToken {
    /// Nouveau jeton non annulé.
    pub fn new() -> Self {
        Self {
            inner: Arc::new(CancellationInner {
                cancelled: std::sync::atomic::AtomicBool::new(false),
                notify: Mutex::new(Vec::new()),
            }),
        }
    }

    /// Annule le jeton et réveille les tâches en attente.
    pub fn cancel(&self) {
        if !self.inner.cancelled.swap(true, std::sync::atomic::Ordering::SeqCst) {
            let mut not = self.inner.notify.lock();
            for w in not.drain(..) {
                w.wake();
            }
        }
    }

    /// État actuel.
    pub fn is_cancelled(&self) -> bool {
        self.inner.cancelled.load(std::sync::atomic::Ordering::SeqCst)
    }

    /// Futur qui se résout quand annulé.
    pub fn cancelled(&self) -> Cancelled<'_> { Cancelled { inner: &self.inner } }
}

/// Futur résolu à l’annulation.
pub struct Cancelled<'a> {
    inner: &'a CancellationInner,
}
impl<'a> Future for Cancelled<'a> {
    type Output = ();
    fn poll(self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<()> {
        if self.inner.cancelled.load(std::sync::atomic::Ordering::SeqCst) {
            Poll::Ready(())
        } else {
            let mut v = self.inner.notify.lock();
            v.push(cx.waker().clone());
            Poll::Pending
        }
    }
}

/* ============================== Utilitaires combinés ============================== */

/// Annule `fut` si `token` est déclenché. Rend `Cancelled` en cas d’annulation.
pub async fn cancel_on<F: Future>(
    token: CancellationToken,
    fut: F,
) -> Result<F::Output, AsyncError> {
    use futures::future::Either;
    futures::pin_mut!(fut);
    let race = futures::future::select(token.cancelled(), fut).await;
    match race {
        Either::Left((_unit, _fut)) => Err(AsyncError::Cancelled),
        Either::Right((v, _cancelled)) => Ok(v),
    }
}

/* ============================== Fallback executor ============================== */

use futures::{channel::oneshot as foneshot, FutureExt as _};

struct FallbackJoin<T> {
    rx: foneshot::Receiver<T>,
    abort_tx: Arc<std::sync::atomic::AtomicBool>,
}
impl<T> FallbackJoin<T> {
    fn spawn<F>(fut: F) -> Self
    where
        F: Future<Output = T> + Send + 'static,
        T: Send + 'static,
    {
        let (tx, rx) = foneshot::channel();
        let cancelled = Arc::new(std::sync::atomic::AtomicBool::new(false));
        let flag = cancelled.clone();
        std::thread::spawn(move || {
            let res = futures::executor::block_on(async {
                if flag.load(std::sync::atomic::Ordering::SeqCst) {
                    return None;
                }
                Some(fut.await)
            });
            if let Some(v) = res {
                let _ = tx.send(v);
            }
        });
        Self { rx, abort_tx: cancelled }
    }
    fn abort(&self) {
        self.abort_tx.store(true, std::sync::atomic::Ordering::SeqCst);
    }
}
impl<T> Future for FallbackJoin<T> {
    type Output = Result<T, AsyncError>;
    fn poll(self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Self::Output> {
        let this = unsafe { self.get_unchecked_mut() };
        match Pin::new(&mut this.rx).poll(cx) {
            Poll::Ready(Ok(v)) => Poll::Ready(Ok(v)),
            Poll::Ready(Err(_)) => Poll::Ready(Err(AsyncError::Join)),
            Poll::Pending => Poll::Pending,
        }
    }
}

/* ============================== Alias `main` ============================== */

/// Alias d’attribut `main` si le runtime le fournit.
/// - `rt-tokio`      → réexporte `tokio::main`
/// - `rt-async-std`  → réexporte `async_std::main`
/// - `rt-smol`/fallback → non disponible, utiliser `fn main() { block_on(async { ... }) }`
#[cfg(feature = "rt-tokio")]
pub use tokio::main;
#[cfg(all(not(feature = "rt-tokio"), feature = "rt-async-std"))]
pub use async_std::main;

/* ============================== Tests basiques ============================== */

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_block_on_sleep() {
        block_on(async {
            sleep(Duration::from_millis(5)).await;
        });
    }

    #[test]
    fn test_spawn_join() {
        let out = block_on(async {
            let j = spawn(async { 7u32 });
            j.await.unwrap()
        });
        assert_eq!(out, 7);
    }

    #[test]
    fn test_timeout_ok() {
        let res = block_on(async { timeout(Duration::from_millis(20), async { 1 }).await });
        assert!(matches!(res, Ok(1)));
    }

    #[test]
    fn test_timeout_expire() {
        let res = block_on(async {
            timeout(Duration::from_millis(5), async {
                sleep(Duration::from_millis(30)).await;
                1
            })
            .await
        });
        assert!(matches!(res, Err(AsyncError::Timeout(_))));
    }

    #[test]
    fn test_cancel_token() {
        let tok = CancellationToken::new();
        let t2 = tok.clone();
        let res = block_on(async {
            let f = async {
                cancel_on(t2, async {
                    sleep(Duration::from_millis(50)).await;
                    9u8
                })
                .await
            };
            let j = spawn(f);
            sleep(Duration::from_millis(5)).await;
            tok.cancel();
            j.await
        });
        assert!(matches!(res, Err(AsyncError::Cancelled)));
    }
}

/* ============================== Optional I/O surfacing ============================== */

/// Fonction utilitaire qui retourne un `std::io::Result<()>` quand un runtime I/O est requis.
/// Permet d’écrire des exemples communs sans lier à un runtime précis.
pub async fn yield_now() {
    #[cfg(feature = "rt-tokio")]
    { tokio::task::yield_now().await; }
    #[cfg(all(not(feature = "rt-tokio"), feature = "rt-async-std"))]
    { async_std::task::yield_now().await; }
    #[cfg(all(not(feature = "rt-tokio"), not(feature = "rt-async-std"), feature = "rt-smol"))]
    { smol::future::yield_now().await; }
    #[cfg(all(not(feature = "rt-tokio"), not(feature = "rt-async-std"), not(feature = "rt-smol")))]
    { futures::future::ready(()).await; }
}

/* ============================== Deps fallback timers ============================== */

// Le fallback de `sleep/timeout` s’appuie sur `futures-timer`.
#[cfg(all(not(feature = "rt-tokio"), not(feature = "rt-async-std"), not(feature = "rt-smol")))]
mod _dep {
    pub use futures_timer;
}