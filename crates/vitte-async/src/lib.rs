//! Async runtime façade for vitte-async
#![cfg_attr(not(feature = "std"), no_std)]

#[cfg(feature = "rt-tokio")]
pub use tokio::{
    spawn,
    time::{sleep, Duration, Instant, interval, timeout, yield_now},
    task::JoinHandle,
    runtime::{Builder, Runtime},
    sync::{mpsc, oneshot, Mutex, RwLock},
};

/// Cancellation token for cooperative task cancellation.
///
/// This is a re-export of [`tokio_util::sync::CancellationToken`].
#[cfg(feature = "rt-tokio")]
pub use tokio_util::sync::CancellationToken;

#[cfg(not(feature = "rt-tokio"))]
#[doc = "Enable feature `rt-tokio` to use the async runtime façade."]
pub mod _rt_disabled {}
