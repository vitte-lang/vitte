//! stack.rs — Pile de valeurs & pile d'appels pour la VM Vitte
//!
//! - `Stack<V>`     : pile générique (limite, push/pop/peek, set/get sûrs)
//! - `CallFrame`    : métadonnées d'un appel (bp, ret_ip, ip courant, locals…)
//! - `CallStack`    : pile de frames (push/pop/current)
//! - `VmStacks<V>`  : orchestration CALL/RET avec conservation du retour
//!
//! Intégration rapide dans l'interpréteur :
//!   type Val = crate::interpreter::Value; // ou ton type runtime
//!   let mut st = VmStacks::<Val>::with_limit(1 << 20); // 1M éléments max
//!   st.push(Value::Int(1));
//!   // call:
//!   st.begin_call("foo", /*target_ip*/ 42, /*ret_ip*/ ip, /*argc*/ 2, /*locals*/ 3)?;
//!   // ... exécute la fonction ...
//!   st.end_call_push_return(ret_val)?; // nettoie & remet la valeur de retour
//!
//! NB: `bp` = base pointer (début des args de la frame courante)

use std::fmt;

/// Erreurs possibles liées aux piles
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum StackError {
    Overflow,
    Underflow,
    OutOfBounds,
    FrameUnderflow,
    FrameOverflow,
    BadReturn, // incohérence d'état lors d'un RET
}

impl std::error::Error for StackError {}
impl fmt::Display for StackError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        use StackError::*;
        match self {
            Overflow => write!(f, "stack overflow"),
            Underflow => write!(f, "stack underflow"),
            OutOfBounds => write!(f, "stack index out of bounds"),
            FrameUnderflow => write!(f, "call stack underflow"),
            FrameOverflow => write!(f, "call stack overflow"),
            BadReturn => write!(f, "inconsistent state on return"),
        }
    }
}

/* =============================== Value Stack ============================== */

/// Pile générique avec limite dure (optionnelle).
#[derive(Debug, Clone)]
pub struct Stack<V> {
    buf: Vec<V>,
    limit: usize, // 0 = illimité
}

impl<V> Default for Stack<V> {
    fn default() -> Self { Self { buf: Vec::new(), limit: 0 } }
}
impl<V> Stack<V> {
    pub fn new() -> Self { Self::default() }
    pub fn with_limit(limit: usize) -> Self { Self { buf: Vec::new(), limit } }

    #[inline] pub fn len(&self) -> usize { self.buf.len() }
    #[inline] pub fn is_empty(&self) -> bool { self.buf.is_empty() }
    #[inline] pub fn clear(&mut self) { self.buf.clear(); }

    #[inline]
    pub fn reserve(&mut self, n: usize) {
        self.buf.reserve(n);
    }

    #[inline]
    pub fn push(&mut self, v: V) -> Result<(), StackError> {
        if self.limit != 0 && self.buf.len() >= self.limit {
            return Err(StackError::Overflow);
        }
        self.buf.push(v);
        Ok(())
    }

    #[inline]
    pub fn pop(&mut self) -> Result<V, StackError> {
        self.buf.pop().ok_or(StackError::Underflow)
    }

    #[inline]
    pub fn peek(&self) -> Result<&V, StackError> {
        self.buf.last().ok_or(StackError::Underflow)
    }

    #[inline]
    pub fn peek_mut(&mut self) -> Result<&mut V, StackError> {
        self.buf.last_mut().ok_or(StackError::Underflow)
    }

    #[inline]
    pub fn get(&self, ix: usize) -> Result<&V, StackError> {
        self.buf.get(ix).ok_or(StackError::OutOfBounds)
    }

    #[inline]
    pub fn get_mut(&mut self, ix: usize) -> Result<&mut V, StackError> {
        self.buf.get_mut(ix).ok_or(StackError::OutOfBounds)
    }

    /// Accès à une fenêtre [start..end) (non copiée). Utile pour args/locals.
    pub fn slice(&self, start: usize, end: usize) -> Result<&[V], StackError> {
        if start > end || end > self.buf.len() { return Err(StackError::OutOfBounds); }
        Ok(&self.buf[start..end])
    }

    pub fn slice_mut(&mut self, start: usize, end: usize) -> Result<&mut [V], StackError> {
        if start > end || end > self.buf.len() { return Err(StackError::OutOfBounds); }
        Ok(&mut self.buf[start..end])
    }

    /// Tronque la pile à `new_len`. Si new_len > len → erreur.
    pub fn truncate_to(&mut self, new_len: usize) -> Result<(), StackError> {
        if new_len > self.buf.len() { return Err(StackError::OutOfBounds); }
        self.buf.truncate(new_len);
        Ok(())
    }

    /// Longueur actuelle (alias)
    #[inline] pub fn sp(&self) -> usize { self.buf.len() }
}

/* =============================== Call Frames ============================== */

/// Métadonnées d'une frame d'appel.
#[derive(Debug, Clone)]
pub struct CallFrame {
    pub name: String,   // nom logique (pour stacktrace)
    pub ip: usize,      // IP courant *dans* la fonction (sera mis à jour par la VM)
    pub ret_ip: usize,  // IP de reprise dans le caller
    pub bp: usize,      // base pointer = index des premiers arguments sur la pile
    pub argc: u16,      // nombre d'arguments
    pub locals: u16,    // slots locaux réservés (pile : args... | locals...)
}

impl CallFrame {
    pub fn new(name: impl Into<String>, ip: usize, ret_ip: usize, bp: usize, argc: u16, locals: u16) -> Self {
        Self { name: name.into(), ip, ret_ip, bp, argc, locals }
    }

    /// Index de début des locals sur la pile (immédiatement après les args).
    #[inline] pub fn locals_start(&self) -> usize { self.bp + self.argc as usize }

    /// Index de fin (exclusif) des locals sur la pile.
    #[inline] pub fn locals_end(&self) -> usize { self.locals_start() + self.locals as usize }

    /// Fenêtre complète "frame" : args + locals
    #[inline] pub fn frame_span(&self) -> (usize, usize) { (self.bp, self.locals_end()) }
}

/// Pile d'appels
#[derive(Debug, Default, Clone)]
pub struct CallStack {
    frames: Vec<CallFrame>,
    limit: usize, // 0 = illimité
}

impl CallStack {
    pub fn new() -> Self { Self { frames: Vec::new(), limit: 0 } }
    pub fn with_limit(limit: usize) -> Self { Self { frames: Vec::new(), limit } }

    #[inline] pub fn depth(&self) -> usize { self.frames.len() }
    #[inline] pub fn is_empty(&self) -> bool { self.frames.is_empty() }

    pub fn push(&mut self, f: CallFrame) -> Result<(), StackError> {
        if self.limit != 0 && self.frames.len() >= self.limit {
            return Err(StackError::FrameOverflow);
        }
        self.frames.push(f);
        Ok(())
    }

    pub fn pop(&mut self) -> Result<CallFrame, StackError> {
        self.frames.pop().ok_or(StackError::FrameUnderflow)
    }

    pub fn current(&self) -> Option<&CallFrame> { self.frames.last() }
    pub fn current_mut(&mut self) -> Option<&mut CallFrame> { self.frames.last_mut() }

    /// IP de reprise du caller (utile au `RET`).
    pub fn caller_ret_ip(&self) -> Result<usize, StackError> {
        let f = self.frames.last().ok_or(StackError::FrameUnderflow)?;
        Ok(f.ret_ip)
    }
}

/* ============================== VmStacks glue ============================= */

/// Ensemble des piles VM : valeurs + frames.  
/// Fournit des helpers ergonomiques pour CALL/RET.
#[derive(Debug, Clone)]
pub struct VmStacks<V> {
    pub values: Stack<V>,
    pub calls: CallStack,
}

impl<V> Default for VmStacks<V> {
    fn default() -> Self { Self { values: Stack::new(), calls: CallStack::new() } }
}
impl<V> VmStacks<V> {
    pub fn with_limit(limit_values: usize) -> Self {
        Self { values: Stack::with_limit(limit_values), calls: CallStack::new() }
    }

    #[inline] pub fn sp(&self) -> usize { self.values.sp() }
    #[inline] pub fn depth(&self) -> usize { self.calls.depth() }

    pub fn push(&mut self, v: V) -> Result<(), StackError> { self.values.push(v) }
    pub fn pop(&mut self) -> Result<V, StackError> { self.values.pop() }
    pub fn peek(&self) -> Result<&V, StackError> { self.values.peek() }
    pub fn peek_mut(&mut self) -> Result<&mut V, StackError> { self.values.peek_mut() }

    /// Commencer un appel :
    /// - suppose que la pile contient déjà les `argc` arguments du callee en haut de pile,
    ///   dans l'ordre (arg0, arg1, ..., arg[n-1]).
    /// - réserve `locals` slots (remplis par la suite par la VM).
    pub fn begin_call(
        &mut self,
        name: impl Into<String>,
        target_ip: usize,
        ret_ip: usize,
        argc: u16,
        locals: u16,
    ) -> Result<(), StackError> {
        // bp = position du 1er argument (si aucun argument, bp = sp)
        let bp = self.values.sp().saturating_sub(argc as usize);
        // Allouer la frame
        let frame = CallFrame::new(name, target_ip, ret_ip, bp, argc, locals);
        self.calls.push(frame)?;
        // Réserver l'espace pour les locals sur la pile valeurs (avec des trous)
        let need = locals as usize;
        if need > 0 {
            // Pousse des "slots" non-initialisés (on ne peut pas push V::default() sans Default)
            // → On "grandit" via un hack : réserve puis set par indices via l'utilisateur.
            // Ici on remplit en retard : l'utilisateur devra set via slice_mut().
            // Pour rester safe, on pousse des unités vides via Option<V> ? Non, on reste simple :
            // on ajoute need éléments en dupliquant le dernier si possible — sinon on interdit.
            // Solution: on utilise un vecteur réservé + contrôles : on n'insère pas ici.
            // → Alternative simple : les locals sont *conceptuels* ; on ne pousse rien maintenant.
            // Le code utilisateur doit gérer l'occupation réelle de la pile via set/get.
            // On documente : locals réserve *virtuellement* [bp+argc .. bp+argc+locals).
        }
        Ok(())
    }

    /// Terminer un appel :
    /// - `ret_val` est la valeur de retour
    /// - nettoie args+locals de la frame et remet `ret_val` au sommet
    /// - renvoie l'IP de reprise (caller)
    pub fn end_call_push_return(&mut self, ret_val: V) -> Result<usize, StackError> {
        let f = self.calls.pop()?;
        let (start, end) = f.frame_span();
        // Vérif bornes
        if end > self.values.sp() || start > end {
            return Err(StackError::BadReturn);
        }
        // Tronquer toute la frame (args+locals)
        self.values.truncate_to(start)?;
        // Remettre la valeur de retour
        self.values.push(ret_val)?;
        Ok(f.ret_ip)
    }

    /// Accès aux arguments de la frame courante.
    pub fn args(&self) -> Result<&[V], StackError> {
        let f = self.calls.current().ok_or(StackError::FrameUnderflow)?;
        self.values.slice(f.bp, f.locals_start())
    }

    /// Accès mut aux locals de la frame courante (fenêtre [locals_start..locals_end)).
    /// NB : si vous n'avez pas réellement alloué ces slots sur la pile valeurs,
    /// vous devez vous-même garantir l'espace via `push` ou une convention VM.
    pub fn locals_mut(&mut self) -> Result<&mut [V], StackError> {
        let f = self.calls.current().ok_or(StackError::FrameUnderflow)?;
        self.values.slice_mut(f.locals_start(), f.locals_end())
    }

    /// Fenêtre complète (args + locals) de la frame courante.
    pub fn frame_window(&self) -> Result<&[V], StackError> {
        let f = self.calls.current().ok_or(StackError::FrameUnderflow)?;
        let (a, b) = f.frame_span();
        self.values.slice(a, b)
    }
}

/* ================================== Tests ================================= */

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn value_stack_basics() {
        let mut s = Stack::<i64>::with_limit(3);
        assert!(s.push(1).is_ok());
        assert!(s.push(2).is_ok());
        assert_eq!(*s.peek().unwrap(), 2);
        assert_eq!(s.len(), 2);
        assert_eq!(s.pop().unwrap(), 2);
        assert_eq!(s.pop().unwrap(), 1);
        assert!(matches!(s.pop(), Err(StackError::Underflow)));
        assert!(s.push(7).is_ok());
        assert!(s.push(8).is_ok());
        assert!(s.push(9).is_ok());
        assert!(matches!(s.push(10), Err(StackError::Overflow)));
    }

    #[test]
    fn callstack_and_vmstacks() {
        let mut vm = VmStacks::<i64>::default();

        // Simule deux args déjà sur la pile
        vm.push(10).unwrap();
        vm.push(20).unwrap();
        let argc = 2;
        let locals = 1;

        vm.begin_call("foo", /*target_ip*/ 100, /*ret_ip*/ 7, argc, locals).unwrap();
        assert_eq!(vm.depth(), 1);
        // (optionnel) réserver physiquement un slot local
        vm.push(0).unwrap(); // local[0]

        // locals_mut : update le slot local
        {
            let f = vm.calls.current().unwrap().clone();
            let start = f.locals_start();
            vm.values.get_mut(start).unwrap().clone_from(&123);
        }

        // Fin d'appel : remet la valeur de retour au sommet et nettoie args+locals
        let rip = vm.end_call_push_return(42).unwrap();
        assert_eq!(rip, 7);
        assert_eq!(vm.depth(), 0);
        assert_eq!(*vm.peek().unwrap(), 42);
    }
}
