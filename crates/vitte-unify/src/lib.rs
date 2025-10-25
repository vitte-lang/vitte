//! vitte-unify
//!
//! Unificateur de types simple, pur et testable.
//! Couvre : variables de type, constructeurs, fonctions, applications n-aires,
//! substitutions, occurs-check, composition.
//!
//! Exemple d’usage rapide :
//! let mut u = Unifier::default();
//! let a = u.fresh();
//! let b = u.fresh();
//! let t1 = Type::fun([Type::var(a)], Type::var(b));
//! let t2 = Type::fun([Type::ctor("i32")], Type::ctor("i32"));
//! let s = u.unify(&t1, &t2).unwrap();
//! assert_eq!(s.apply(&Type::var(a)), Type::ctor("i32"));

use std::collections::HashMap;
use std::fmt;

/// Identifiant de variable de type.
pub type TVar = u32;

/// Terme de type.
#[derive(Clone, PartialEq, Eq, Hash)]
pub enum Type {
    /// Variable de type (unifiée par substitution).
    Var(TVar),
    /// Constructeur nu, ex: "i32", "bool".
    Ctor(&'static str),
    /// Application n-aire: head(args...). Pour flèches, on fournit un helper `fun`.
    App(Box<Type>, Vec<Type>),
}

impl Type {
    #[inline]
    pub fn var(v: TVar) -> Self {
        Type::Var(v)
    }

    #[inline]
    pub fn ctor(name: &'static str) -> Self {
        Type::Ctor(name)
    }

    /// Type fonction: (args...) -> ret
    pub fn fun<I: IntoIterator<Item = Type>>(args: I, ret: Type) -> Self {
        Type::App(Box::new(Type::Ctor("->")), {
            let mut v: Vec<Type> = args.into_iter().collect();
            v.push(ret);
            v
        })
    }

    /// Décompose une flèche si c’en est une.
    pub fn as_fun(&self) -> Option<(&[Type], &Type)> {
        match self {
            Type::App(h, xs) if matches!(**h, Type::Ctor("->")) && !xs.is_empty() => {
                let (args, ret) = xs.split_at(xs.len() - 1);
                Some((args, &ret[0]))
            }
            _ => None,
        }
    }
}

impl fmt::Debug for Type {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Type::Var(v) => write!(f, "'t{}", v),
            Type::Ctor(c) => f.write_str(c),
            Type::App(head, args) => {
                if let Some((a, r)) = self.as_fun() {
                    write!(f, "(")?;
                    for (i, t) in a.iter().enumerate() {
                        if i > 0 {
                            write!(f, ", ")?;
                        }
                        write!(f, "{:?}", t)?;
                    }
                    write!(f, ") -> {:?}", r)
                } else {
                    write!(f, "{:?}(", head)?;
                    for (i, t) in args.iter().enumerate() {
                        if i > 0 {
                            write!(f, ", ")?;
                        }
                        write!(f, "{:?}", t)?;
                    }
                    write!(f, ")")
                }
            }
        }
    }
}

/// Substitution finie TVar → Type.
#[derive(Clone, Default, PartialEq, Eq)]
pub struct Subst {
    map: HashMap<TVar, Type>,
}

impl Subst {
    pub fn empty() -> Self {
        Self { map: HashMap::new() }
    }

    pub fn insert(&mut self, v: TVar, t: Type) {
        // normaliser pour éviter v := v
        if !matches!(t, Type::Var(x) if x == v) {
            self.map.insert(v, t);
        }
    }

    pub fn get(&self, v: TVar) -> Option<&Type> {
        self.map.get(&v)
    }

    /// Applique la substitution récursivement.
    pub fn apply(&self, t: &Type) -> Type {
        match t {
            Type::Var(v) => match self.map.get(v) {
                Some(tt) => self.apply(tt),
                None => t.clone(),
            },
            Type::Ctor(_) => t.clone(),
            Type::App(h, xs) => {
                let h2 = Box::new(self.apply(h));
                let xs2 = xs.iter().map(|x| self.apply(x)).collect();
                Type::App(h2, xs2)
            }
        }
    }

    /// Compose `self ∘ other` tel que (self ∘ other)(x) = self(other(x)).
    pub fn compose(&self, other: &Subst) -> Subst {
        let mut out = Subst::empty();
        // self after other: on applique self sur l’image de other
        for (v, t) in &other.map {
            out.insert(*v, self.apply(t));
        }
        // puis on ajoute les bindings de self qui ne sont pas masqués
        for (v, t) in &self.map {
            if !out.map.contains_key(v) {
                out.insert(*v, t.clone());
            }
        }
        out
    }

    /// Étend en place avec `v := t`, puis normalise l’existant.
    pub fn extend_and_normalize(&mut self, v: TVar, t: Type) {
        let rhs = self.apply(&t);
        let mut map = std::mem::take(&mut self.map);
        let original_subst = Subst { map: map.clone() };
        let rhs_subst = Subst { map: [(v, rhs.clone())].into_iter().collect() };

        for val in map.values_mut() {
            let normalized = original_subst.apply(val);
            *val = rhs_subst.apply(&normalized);
        }

        map.insert(v, rhs);
        self.map = map;
    }

    pub fn is_empty(&self) -> bool {
        self.map.is_empty()
    }
}

/// Erreurs d’unification.
#[derive(Debug, thiserror::Error, PartialEq, Eq)]
pub enum UnifyError {
    #[error("occurs check failed: 't{var} in {ty:?}")]
    Occurs { var: TVar, ty: Type },

    #[error("constructor mismatch: {left:?} vs {right:?}")]
    CtorMismatch { left: Type, right: Type },

    #[error("arity mismatch for {ctor}: {left} vs {right}")]
    ArityMismatch { ctor: &'static str, left: usize, right: usize },
}

/// Unificateur avec génération de variables fraîches.
#[derive(Default)]
pub struct Unifier {
    next: TVar,
}

impl Unifier {
    pub fn with_start(start: TVar) -> Self {
        Self { next: start }
    }
    pub fn fresh(&mut self) -> TVar {
        let v = self.next;
        self.next += 1;
        v
    }

    /// Unifie `a` et `b`, retourne la substitution la plus générale.
    pub fn unify(&mut self, a: &Type, b: &Type) -> Result<Subst, UnifyError> {
        self.unify_with(Subst::empty(), a, b)
    }

    /// Unifie sous substitution courante `s`.
    pub fn unify_with(&mut self, s: Subst, a: &Type, b: &Type) -> Result<Subst, UnifyError> {
        let a = s.apply(a);
        let b = s.apply(b);
        match (a, b) {
            (Type::Var(v), t) => bind_var(v, t, s),
            (t, Type::Var(v)) => bind_var(v, t, s),
            (Type::Ctor(c1), Type::Ctor(c2)) if c1 == c2 => Ok(s),
            (Type::App(h1, xs1), Type::App(h2, xs2)) => {
                // cas flèche ou constructeur appliqué
                match (&*h1, &*h2) {
                    (Type::Ctor(c1), Type::Ctor(c2)) if c1 == c2 => {
                        if xs1.len() != xs2.len() {
                            return Err(UnifyError::ArityMismatch {
                                ctor: c1,
                                left: xs1.len(),
                                right: xs2.len(),
                            });
                        }
                        let mut s_acc = s;
                        for (l, r) in xs1.iter().zip(xs2.iter()) {
                            s_acc = self.unify_with(s_acc, l, r)?;
                        }
                        Ok(s_acc)
                    }
                    _ => Err(UnifyError::CtorMismatch {
                        left: Type::App(h1, xs1),
                        right: Type::App(h2, xs2),
                    }),
                }
            }
            (l @ Type::Ctor(_), r @ Type::Ctor(_)) => {
                Err(UnifyError::CtorMismatch { left: l, right: r })
            }
            (l, r) => Err(UnifyError::CtorMismatch { left: l, right: r }),
        }
    }
}

/// Liaisons var := type avec occurs-check.
fn bind_var(v: TVar, t: Type, mut s: Subst) -> Result<Subst, UnifyError> {
    if let Type::Var(v2) = t {
        if v == v2 {
            return Ok(s);
        }
    }
    if occurs(v, &t, &s) {
        return Err(UnifyError::Occurs { var: v, ty: t });
    }
    s.extend_and_normalize(v, t);
    Ok(s)
}

/// Occurs-check sur t avec substitution s.
fn occurs(v: TVar, t: &Type, s: &Subst) -> bool {
    match s.apply(t) {
        Type::Var(v2) => v == v2,
        Type::Ctor(_) => false,
        Type::App(h, xs) => occurs(v, &h, s) || xs.iter().any(|x| occurs(v, x, s)),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn v(n: u32) -> Type {
        Type::var(n)
    }
    fn i32_() -> Type {
        Type::ctor("i32")
    }
    fn bool_() -> Type {
        Type::ctor("bool")
    }

    #[test]
    fn unify_var_with_ctor() {
        let mut u = Unifier::default();
        let a = u.fresh();
        let s = u.unify(&v(a), &i32_()).unwrap();
        assert_eq!(s.apply(&v(a)), i32_());
    }

    #[test]
    fn unify_fun_basic() {
        let mut u = Unifier::default();
        let a = u.fresh();
        let t1 = Type::fun([v(a)], v(a));
        let t2 = Type::fun([i32_()], i32_());
        let s = u.unify(&t1, &t2).unwrap();
        assert_eq!(s.apply(&v(a)), i32_());
    }

    #[test]
    fn occurs_check_fails() {
        let mut u = Unifier::default();
        let a = u.fresh();
        let t = Type::fun([v(a.clone())], v(a.clone()));
        let e = u.unify(&v(a), &t).unwrap_err();
        matches!(e, UnifyError::Occurs { .. });
    }

    #[test]
    fn ctor_mismatch() {
        let mut u = Unifier::default();
        let e = u.unify(&i32_(), &bool_()).unwrap_err();
        matches!(e, UnifyError::CtorMismatch { .. });
    }

    #[test]
    fn arity_mismatch() {
        let mut u = Unifier::default();
        let t1 = Type::App(Box::new(Type::ctor("Vec")), vec![i32_()]);
        let t2 = Type::App(Box::new(Type::ctor("Vec")), vec![i32_(), bool_()]);
        let e = u.unify(&t1, &t2).unwrap_err();
        matches!(e, UnifyError::ArityMismatch { .. });
    }

    #[test]
    fn compose_and_apply() {
        let mut u = Unifier::with_start(10);
        let a = 10;
        let b = 11;
        let s1 = u.unify(&v(a), &i32_()).unwrap();
        let s2 = u.unify(&v(b), &v(a)).unwrap();
        let sc = s2.compose(&s1); // s2 ∘ s1
        assert_eq!(sc.apply(&v(b)), i32_());
    }
}
