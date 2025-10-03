//! vitte-math — mathematical utilities stub.
//!
//! A minimal placeholder so that crates depending on `vitte-math` continue to
//! compile. Only a handful of simple helpers are implemented; everything else
//! returns the identity or an error as appropriate.

#![cfg_attr(not(feature = "std"), no_std)]

extern crate alloc;

use alloc::vec::Vec;

/// Sums the elements of a slice.
pub fn sum<T>(data: &[T]) -> T
where
    T: Copy + core::ops::Add<Output = T> + Default,
{
    let mut acc = T::default();
    for &v in data {
        acc = acc + v;
    }
    acc
}

/// Returns the mean of a slice (stubbed to zero for empty data).
pub fn mean<T>(data: &[T]) -> f64
where
    T: Into<f64> + Copy,
{
    if data.is_empty() {
        0.0
    } else {
        let total: f64 = data.iter().copied().map(Into::into).sum();
        total / (data.len() as f64)
    }
}

/// Simple linear algebra vector placeholder.
#[derive(Debug, Clone, PartialEq)]
pub struct Vector<T> {
    data: Vec<T>,
}

impl<T> Vector<T> {
    /// Creates a vector from a `Vec`.
    pub fn new(data: Vec<T>) -> Self {
        Self { data }
    }

    /// Returns the number of elements.
    pub fn len(&self) -> usize {
        self.data.len()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn sum_stub() {
        assert_eq!(sum(&[1, 2, 3]), 6);
    }
}
