//! vitte-vm/stack.rs
//!
//! Petite pile générique avec garde-fous : limites optionnelles, erreurs claires,
//! opérations utilitaires (dup/swap/peek_n). Pensée pour l’interpréteur Vitte.
//!
//! Usage rapide :
//!     let mut s = Stack::with_limit(1 << 20); // 1M éléments max
//!     s.push(42)?; let top = s.peek()?; let v = s.pop()?;
//!
//! Spécialisation `impl` pour `Value` (de l’interpréteur) incluse en bas.

use std::fmt;

/// Erreurs de pile.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum StackError {
    /// Pop/peek alors que la pile est vide.
    Underflow(&'static str),
    /// Dépassement de la limite configurée.
    Overflow,
    /// Index hors limites pour `peek_n`, `swap_n`, etc.
    IndexOutOfBounds,
}

impl fmt::Display for StackError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            StackError::Underflow(op) => write!(f, "stack underflow (op: {op})"),
            StackError::Overflow => write!(f, "stack overflow"),
            StackError::IndexOutOfBounds => write!(f, "stack index out of bounds"),
        }
    }
}

impl std::error::Error for StackError {}

/// Pile générique avec limite optionnelle.
#[derive(Debug, Clone)]
pub struct Stack<T> {
    data: Vec<T>,
    limit: Option<usize>,
}

impl<T> Default for Stack<T> {
    fn default() -> Self {
        Self { data: Vec::with_capacity(64), limit: None }
    }
}

impl<T> Stack<T> {
    /// Crée une pile sans limite explicite.
    pub fn new() -> Self { Self::default() }

    /// Crée une pile avec une limite dure (nb max d’éléments).
    pub fn with_limit(limit: usize) -> Self {
        Self { data: Vec::with_capacity(64), limit: Some(limit) }
    }

    /// Définit/retire la limite de capacité.
    pub fn set_limit(&mut self, limit: Option<usize>) {
        self.limit = limit;
    }

    /// Nombre d’éléments.
    #[inline] pub fn len(&self) -> usize { self.data.len() }

    /// Vide ?
    #[inline] pub fn is_empty(&self) -> bool { self.data.is_empty() }

    /// Vide complètement la pile.
    pub fn clear(&mut self) { self.data.clear(); }

    /// Empile un élément (respecte la limite).
    pub fn push(&mut self, v: T) -> Result<(), StackError> {
        if let Some(limit) = self.limit {
            if self.data.len() >= limit {
                return Err(StackError::Overflow);
            }
        }
        self.data.push(v);
        Ok(())
    }

    /// Dépile l’élément du haut.
    pub fn pop(&mut self, op_name: &'static str) -> Result<T, StackError> {
        self.data.pop().ok_or(StackError::Underflow(op_name))
    }

    /// Regarde l’élément du haut (emprunt immuable).
    pub fn peek(&self) -> Result<&T, StackError> {
        self.data.last().ok_or(StackError::Underflow("peek"))
    }

    /// Regarde l’élément du haut (emprunt mutable).
    pub fn peek_mut(&mut self) -> Result<&mut T, StackError> {
        self.data.last_mut().ok_or(StackError::Underflow("peek_mut"))
    }

    /// Regarde l’élément à N depuis le haut (0 = top).
    pub fn peek_n(&self, n_from_top: usize) -> Result<&T, StackError> {
        if n_from_top >= self.data.len() { return Err(StackError::IndexOutOfBounds); }
        let idx = self.data.len() - 1 - n_from_top;
        Ok(&self.data[idx])
    }

    /// Idem, version mutable.
    pub fn peek_n_mut(&mut self, n_from_top: usize) -> Result<&mut T, StackError> {
        if n_from_top >= self.data.len() { return Err(StackError::IndexOutOfBounds); }
        let idx = self.data.len() - 1 - n_from_top;
        Ok(&mut self.data[idx])
    }

    /// Duplique le top.
    pub fn dup(&mut self) -> Result<(), StackError>
    where
        T: Clone,
    {
        let v = self.peek()?.clone();
        self.push(v)
    }

    /// Échange top et top-1.
    pub fn swap(&mut self) -> Result<(), StackError> {
        if self.data.len() < 2 { return Err(StackError::Underflow("swap")); }
        let len = self.data.len();
        self.data.swap(len - 1, len - 2);
        Ok(())
    }

    /// Échange top et l’élément à N depuis le haut (N>=1).
    pub fn swap_n(&mut self, n_from_top: usize) -> Result<(), StackError> {
        if n_from_top == 0 { return Ok(()); }
        if self.data.len() <= n_from_top { return Err(StackError::IndexOutOfBounds); }
        let len = self.data.len();
        let a = len - 1;
        let b = len - 1 - n_from_top;
        self.data.swap(a, b);
        Ok(())
    }

    /// Enlève N éléments d’un coup (si possible).
    pub fn popn(&mut self, n: usize, op_name: &'static str) -> Result<(), StackError> {
        if self.data.len() < n { return Err(StackError::Underflow(op_name)); }
        let new_len = self.data.len() - n;
        self.data.truncate(new_len);
        Ok(())
    }

    /// Expose interne (lecture seule) — utile pour debug pretty.
    pub fn as_slice(&self) -> &[T] { &self.data }
}

/* ---------- Intégration douce avec la VM (Value) ---------- */

/// Si tu utilises la `Value` de l’interpréteur, quelques helpers pratiques.
#[cfg(feature = "vm-value-helpers")]
impl Stack<crate::interpreter::Value> {
    pub fn pop_i64(&mut self) -> Result<i64, StackError> {
        use crate::interpreter::Value::*;
        match self.pop("pop_i64")? {
            I64(x) => Ok(x),
            F64(x) => Ok(x as i64),
            v => Err(StackError::Underflow(match v {
                _ => "type error (expected number)",
            })),
        }
    }
    // Ajoute ici d’autres helpers si besoin (pop_bool, pop_f64, etc.)
}

/* ------------------------------- Tests ------------------------------- */
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn basics() {
        let mut s = Stack::with_limit(3);
        assert!(s.is_empty());
        s.push(1).unwrap();
        s.push(2).unwrap();
        assert_eq!(*s.peek().unwrap(), 2);
        s.swap().unwrap();
        assert_eq!(*s.peek().unwrap(), 1);
        s.dup().unwrap();
        assert_eq!(s.len(), 3);
        assert!(matches!(s.push(4), Err(StackError::Overflow)));
        assert_eq!(s.pop("test").unwrap(), 1);
        assert_eq!(s.pop("test").unwrap(), 2);
        assert_eq!(s.pop("test").unwrap(), 1);
        assert!(matches!(s.pop("test"), Err(StackError::Underflow(_))));
    }

    #[test]
    fn peek_n_and_swap_n() {
        let mut s = Stack::new();
        for i in 0..5 { s.push(i).unwrap(); } // [0,1,2,3,4] ; 4 = top
        assert_eq!(*s.peek_n(0).unwrap(), 4);
        assert_eq!(*s.peek_n(4).unwrap(), 0);
        s.swap_n(3).unwrap(); // swap top (4) with element 3-from-top (1)
        // stack now: [0,4,2,3,1]
        assert_eq!(*s.peek().unwrap(), 1);
        assert_eq!(*s.peek_n(3).unwrap(), 4);
    }

    #[test]
    fn popn_truncate() {
        let mut s = Stack::new();
        for i in 0..10 { s.push(i).unwrap(); }
        s.popn(7, "popn").unwrap();
        assert_eq!(s.len(), 3);
        assert_eq!(*s.peek().unwrap(), 2);
    }
}
