//! vitte-channel — canaux concurrents (MPSC, SPSC, broadcast, async optionnel)
//!
//! Couvre :
//! - MPSC: basé sur `crossbeam-channel` (bounded/unbounded).
//! - SPSC: ring buffer lock-free (SPMC interdit).
//! - Broadcast: N abonnés, chaque abonné reçoit tout (fan-out), best-effort (drop si lent).
//! - Async (feature `async`): wrappers tokio (`mpsc`, `broadcast`) + adaptateurs `Stream`.
//!
//! API commune minimale: `Sender<T>`, `Receiver<T>`.

#![forbid(unsafe_code)]

use std::fmt;
use std::time::Duration;

use crossbeam_channel as xch;
use parking_lot::{Mutex, RwLock};

// ===========================================================================
// Traits communs
// ===========================================================================

pub trait Sender<T>: Send + Sync + 'static {
    fn try_send(&self, msg: T) -> Result<(), SendError<T>>;
    fn send(&self, msg: T) -> Result<(), SendError<T>> {
        self.try_send(msg)
    }
    fn is_closed(&self) -> bool;
}

pub trait Receiver<T>: Send + Sync + 'static {
    fn try_recv(&self) -> Result<T, RecvError>;
    fn recv(&self) -> Result<T, RecvError>;
    fn is_closed(&self) -> bool;
    fn len(&self) -> usize { 0 }
}

#[derive(Debug)]
pub struct SendError<T>(pub T);

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RecvError {
    Empty,
    Closed,
}

impl<T> fmt::Display for SendError<T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result { write!(f, "send failed") }
}
impl fmt::Display for RecvError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self { RecvError::Empty => write!(f, "empty"), RecvError::Closed => write!(f, "closed") }
    }
}
impl std::error::Error for RecvError {}

// ===========================================================================
// MPSC (crossbeam-channel)
// ===========================================================================

pub mod mpsc {
    use super::*;

    #[derive(Clone)]
    pub struct Sender<T>(xch::Sender<T>);
    pub struct Receiver<T>(xch::Receiver<T>);

    pub fn unbounded<T>() -> (Sender<T>, Receiver<T>) {
        let (s, r) = xch::unbounded();
        (Sender(s), Receiver(r))
    }

    pub fn bounded<T>(cap: usize) -> (Sender<T>, Receiver<T>) {
        let (s, r) = xch::bounded(cap);
        (Sender(s), Receiver(r))
    }

    impl<T> super::Sender<T> for Sender<T> {
        fn try_send(&self, msg: T) -> Result<(), super::SendError<T>> {
            self.0.try_send(msg).map_err(|e| match e {
                xch::TrySendError::Full(m) | xch::TrySendError::Disconnected(m) => super::SendError(m),
            })
        }
        fn send(&self, msg: T) -> Result<(), super::SendError<T>> {
            self.0.send(msg).map_err(super::SendError)
        }
        fn is_closed(&self) -> bool { self.0.is_empty() && self.0.is_disconnected() }
    }

    impl<T> super::Receiver<T> for Receiver<T> {
        fn try_recv(&self) -> Result<T, super::RecvError> {
            self.0.try_recv().map_err(|e| match e {
                xch::TryRecvError::Empty => super::RecvError::Empty,
                xch::TryRecvError::Disconnected => super::RecvError::Closed,
            })
        }
        fn recv(&self) -> Result<T, super::RecvError> {
            self.0.recv().map_err(|_| super::RecvError::Closed)
        }
        fn is_closed(&self) -> bool { self.0.is_disconnected() }
        fn len(&self) -> usize { self.0.len() }
    }
}

// ===========================================================================
// SPSC lock-free (anneau monoprod/monocons)
// ===========================================================================

pub mod spsc {
    use super::*;
    use std::cell::UnsafeCell;
    use std::sync::atomic::{AtomicUsize, Ordering};

    // Anneau sans alloc après création. Non redimensionnable. T: Send.
    pub struct Ring<T> {
        buf: Vec<UnsafeCell<Option<T>>>,
        cap: usize,
        head: AtomicUsize, // next write
        tail: AtomicUsize, // next read
        closed: AtomicUsize, // 0=open,1=closed
    }

    unsafe impl<T: Send> Send for Ring<T> {}
    unsafe impl<T: Send> Sync for Ring<T> {}

    pub struct Sender<T> {
        inner: std::sync::Arc<Ring<T>>,
    }
    pub struct Receiver<T> {
        inner: std::sync::Arc<Ring<T>>,
    }

    pub fn bounded<T>(cap: usize) -> (Sender<T>, Receiver<T>) {
        assert!(cap.is_power_of_two(), "cap must be power of two");
        let mut v = Vec::with_capacity(cap);
        for _ in 0..cap { v.push(UnsafeCell::new(None)); }
        let r = Ring {
            buf: v,
            cap,
            head: AtomicUsize::new(0),
            tail: AtomicUsize::new(0),
            closed: AtomicUsize::new(0),
        };
        let a = std::sync::Arc::new(r);
        (Sender { inner: a.clone() }, Receiver { inner: a })
    }

    impl<T> Drop for Sender<T> {
        fn drop(&mut self) { self.inner.closed.store(1, Ordering::Release); }
    }
    impl<T> Drop for Receiver<T> {
        fn drop(&mut self) { self.inner.closed.store(1, Ordering::Release); }
    }

    impl<T> super::Sender<T> for Sender<T> {
        fn try_send(&self, msg: T) -> Result<(), super::SendError<T>> {
            if self.inner.closed.load(Ordering::Acquire) == 1 {
                return Err(super::SendError(msg));
            }
            let cap = self.inner.cap;
            let head = self.inner.head.load(Ordering::Relaxed);
            let tail = self.inner.tail.load(Ordering::Acquire);
            if head - tail == cap { return Err(super::SendError(msg)); } // plein
            let idx = head & (cap - 1);
            unsafe { *self.inner.buf[idx].get() = Some(msg); }
            self.inner.head.store(head + 1, Ordering::Release);
            Ok(())
        }
        fn is_closed(&self) -> bool { self.inner.closed.load(Ordering::Acquire) == 1 }
    }

    impl<T> super::Receiver<T> for Receiver<T> {
        fn try_recv(&self) -> Result<T, super::RecvError> {
            let cap = self.inner.cap;
            let tail = self.inner.tail.load(Ordering::Relaxed);
            let head = self.inner.head.load(Ordering::Acquire);
            if tail == head {
                return if self.is_closed() { Err(super::RecvError::Closed) } else { Err(super::RecvError::Empty) };
            }
            let idx = tail & (cap - 1);
            let v = unsafe { (*self.inner.buf[idx].get()).take().unwrap() };
            self.inner.tail.store(tail + 1, Ordering::Release);
            Ok(v)
        }
        fn recv(&self) -> Result<T, super::RecvError> {
            loop {
                match self.try_recv() {
                    Ok(v) => return Ok(v),
                    Err(super::RecvError::Empty) => std::thread::yield_now(),
                    Err(e @ super::RecvError::Closed) => return Err(e),
                }
            }
        }
        fn is_closed(&self) -> bool { self.inner.closed.load(Ordering::Acquire) == 1 }
        fn len(&self) -> usize {
            let head = self.inner.head.load(Ordering::Acquire);
            let tail = self.inner.tail.load(Ordering::Acquire);
            head.saturating_sub(tail).min(self.inner.cap)
        }
    }
}

// ===========================================================================
// Broadcast (fan-out best-effort)
// ===========================================================================

pub mod broadcast {
    use super::*;

    #[derive(Clone)]
    pub struct Sender<T: Clone + Send + 'static> {
        inner: std::sync::Arc<Inner<T>>,
    }
    pub struct Receiver<T: Clone + Send + 'static> {
        id: usize,
        inner: std::sync::Arc<Inner<T>>,
    }

    struct Inner<T: Clone + Send + 'static> {
        subs: RwLock<Vec<xch::Sender<T>>>,
        cap: usize,
        closed: Mutex<bool>,
    }

    pub fn bounded<T: Clone + Send + 'static>(cap: usize) -> (Sender<T>, Receiver<T>) {
        let inner = std::sync::Arc::new(Inner { subs: RwLock::new(Vec::new()), cap, closed: Mutex::new(false) });
        let s = Sender { inner: inner.clone() };
        let r = s.subscribe();
        (s, r)
    }

    impl<T: Clone + Send + 'static> Sender<T> {
        pub fn subscribe(&self) -> Receiver<T> {
            let (tx, rx) = xch::bounded::<T>(self.inner.cap);
            let mut subs = self.inner.subs.write();
            let id = subs.len();
            subs.push(tx);
            Receiver { id, inner: self.inner.clone() }.attach(rx)
        }
    }

    impl<T: Clone + Send + 'static> super::Sender<T> for Sender<T> {
        fn try_send(&self, msg: T) -> Result<(), super::SendError<T>> {
            if *self.inner.closed.lock() { return Err(super::SendError(msg)); }
            let subs = self.inner.subs.read();
            // best-effort: on tente, on droppe si plein
            for tx in subs.iter() {
                let _ = tx.try_send(msg.clone());
            }
            Ok(())
        }
        fn is_closed(&self) -> bool { *self.inner.closed.lock() }
    }

    impl<T: Clone + Send + 'static> Drop for Sender<T> {
        fn drop(&mut self) { *self.inner.closed.lock() = true; }
    }

    impl<T: Clone + Send + 'static> Receiver<T> {
        fn attach(mut self, rx: xch::Receiver<T>) -> Self {
            // stocker dans un Arc<ReceiverState> si besoin; ici direct
            self.rx = Some(rx);
            self
        }
        fn rx(&self) -> &xch::Receiver<T> { self.rx.as_ref().unwrap() }
        fn rx_mut(&mut self) -> &mut xch::Receiver<T> { self.rx.as_mut().unwrap() }
    }

    impl<T: Clone + Send + 'static> super::Receiver<T> for Receiver<T> {
        fn try_recv(&self) -> Result<T, super::RecvError> {
            self.rx().try_recv().map_err(|e| match e {
                xch::TryRecvError::Empty => super::RecvError::Empty,
                xch::TryRecvError::Disconnected => super::RecvError::Closed,
            })
        }
        fn recv(&self) -> Result<T, super::RecvError> {
            self.rx().recv().map_err(|_| super::RecvError::Closed)
        }
        fn is_closed(&self) -> bool { false }
        fn len(&self) -> usize { self.rx().len() }
    }

    impl<T: Clone + Send + 'static> Drop for Receiver<T> {
        fn drop(&mut self) {
            let mut subs = self.inner.subs.write();
            if self.id < subs.len() {
                subs.remove(self.id);
            }
        }
    }

    // stock rx dans l'objet (field optionnel)
    impl<T: Clone + Send + 'static> Receiver<T> {
        // field
        rx_field!();
    }

    // petite astuce macro pour stocker rx sans répéter
    macro_rules! rx_field {
        () => {
            rx: Option<xch::Receiver<T>>
        };
    }
    use rx_field;
}

// ===========================================================================
// Async (Tokio) — feature `async`
// ===========================================================================

#[cfg(feature = "async")]
pub mod r#async {
    use super::*;
    use futures::{stream::Stream, task::{Context, Poll}};
    use std::pin::Pin;
    use tokio::sync::{broadcast as tbcast, mpsc as tmpsc};

    // mpsc
    #[derive(Clone)]
    pub struct MpscSender<T>(tmpsc::Sender<T>);
    pub struct MpscReceiver<T>(tmpsc::Receiver<T>);

    pub fn mpsc_bounded<T>(cap: usize) -> (MpscSender<T>, MpscReceiver<T>) {
        let (s, r) = tmpsc::channel(cap);
        (MpscSender(s), MpscReceiver(r))
    }

    impl<T: Send + 'static> super::Sender<T> for MpscSender<T> {
        fn try_send(&self, msg: T) -> Result<(), super::SendError<T>> {
            self.0.try_send(msg).map_err(|e| match e {
                tmpsc::error::TrySendError::Full(m) | tmpsc::error::TrySendError::Closed(m) => super::SendError(m),
            })
        }
        fn is_closed(&self) -> bool { self.0.is_closed() }
    }

    impl<T: Send + 'static> MpscReceiver<T> {
        pub async fn recv_async(&mut self) -> Option<T> { self.0.recv().await }
    }

    impl<T: Send + 'static> Stream for MpscReceiver<T> {
        type Item = T;
        fn poll_next(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Option<Self::Item>> {
            Pin::new(&mut self.0).poll_recv(cx)
        }
    }

    // broadcast
    #[derive(Clone)]
    pub struct BroadcastSender<T: Clone + Send + 'static>(tbcast::Sender<T>);
    pub struct BroadcastReceiver<T: Clone + Send + 'static>(tbcast::Receiver<T>);

    pub fn broadcast<T: Clone + Send + 'static>(cap: usize) -> (BroadcastSender<T>, BroadcastReceiver<T>) {
        let (s, r) = tbcast::channel(cap);
        (BroadcastSender(s), BroadcastReceiver(r))
    }

    impl<T: Clone + Send + 'static> BroadcastSender<T> {
        pub fn subscribe(&self) -> BroadcastReceiver<T> { BroadcastReceiver(self.0.subscribe()) }
    }

    impl<T: Clone + Send + 'static> super::Sender<T> for BroadcastSender<T> {
        fn try_send(&self, msg: T) -> Result<(), super::SendError<T>> {
            self.0.send(msg).map(|_| ()).map_err(|e| super::SendError(e.0))
        }
        fn is_closed(&self) -> bool { false }
    }

    impl<T: Clone + Send + 'static> BroadcastReceiver<T> {
        pub async fn recv_async(&mut self) -> Option<T> {
            match self.0.recv().await {
                Ok(v) => Some(v),
                Err(tbcast::error::RecvError::Lagged(_)) => self.recv_async().await,
                Err(tbcast::error::RecvError::Closed) => None,
            }
        }
    }

    impl<T: Clone + Send + 'static> Stream for BroadcastReceiver<T> {
        type Item = T;
        fn poll_next(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Option<Self::Item>> {
            match Pin::new(&mut self.0).poll_recv(cx) {
                Poll::Ready(Ok(v)) => Poll::Ready(Some(v)),
                Poll::Ready(Err(tbcast::error::RecvError::Lagged(_))) => Poll::Pending,
                Poll::Ready(Err(tbcast::error::RecvError::Closed)) => Poll::Ready(None),
                Poll::Pending => Poll::Pending,
            }
        }
    }
}

// ===========================================================================
// Tests
// ===========================================================================

#[cfg(test)]
mod tests {
    use super::*;
    use std::thread;

    #[test]
    fn mpsc_basic() {
        let (s, r) = mpsc::unbounded();
        s.send(1).unwrap();
        assert_eq!(r.try_recv().ok(), Some(1));
    }

    #[test]
    fn spsc_ring() {
        let (s, r) = spsc::bounded::<u32>(8);
        for i in 0..8 { s.try_send(i).unwrap(); }
        assert!(s.try_send(9).is_err());
        let mut sum = 0;
        for _ in 0..8 { sum += r.try_recv().unwrap(); }
        assert_eq!(sum, 28);
    }

    #[test]
    fn broadcast_fanout() {
        let (tx, rx1) = broadcast::bounded::<u32>(16);
        let rx2 = tx.subscribe();
        tx.try_send(7).unwrap();
        assert_eq!(rx1.try_recv().ok(), Some(7));
        assert_eq!(rx2.try_recv().ok(), Some(7));
    }

    #[test]
    fn mpsc_threads() {
        let (s, r) = mpsc::bounded(64);
        let t = thread::spawn(move || {
            for i in 0..1_000 {
                s.send(i).unwrap();
            }
        });
        let mut c = 0;
        while c < 1_000 {
            if let Ok(_v) = r.try_recv() { c += 1; }
            else { std::thread::yield_now(); }
        }
        t.join().unwrap();
    }

    #[cfg(feature = "async")]
    #[tokio::test(flavor = "multi_thread")]
    async fn tokio_mpsc_stream() {
        use futures::StreamExt;
        let (tx, mut rx) = r#async::mpsc_bounded(8);
        tx.try_send(1).unwrap();
        tx.try_send(2).unwrap();
        let v = rx.next().await.unwrap();
        let w = rx.next().await.unwrap();
        assert_eq!((v, w), (1, 2));
    }
}