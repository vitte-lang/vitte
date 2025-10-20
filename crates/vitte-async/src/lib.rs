//! Async runtime façade for vitte-async
#![cfg_attr(not(feature = "std"), no_std)]

#[cfg(feature = "rt-tokio")]
pub use tokio::{
    runtime::{Builder, Runtime},
    spawn,
    sync::{mpsc, oneshot, Mutex, RwLock},
    task::JoinHandle,
    time::{interval, sleep, timeout, yield_now, Duration, Instant},
};

/// Cancellation token for cooperative task cancellation.
///
/// This is a re-export of [`tokio_util::sync::CancellationToken`].
#[cfg(feature = "rt-tokio")]
pub use tokio_util::sync::CancellationToken;

#[cfg(not(feature = "rt-tokio"))]
#[doc = "Enable feature `rt-tokio` to use the async runtime façade."]
pub mod _rt_disabled {}
