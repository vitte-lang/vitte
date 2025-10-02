#![deny(missing_docs)]
//! vitte-simd — primitives SIMD légères pour Vitte
//!
//! Fournit : additions, multiplications, SAXPY, dot product, somme, ReLU et norme L2.
//!
//! Backends :
//! - `wide` (par défaut conseillé) → vecteurs `f32x4`.
//! - `packed_simd` → vecteurs `f32x8`.
//! - Fallback scalaire si aucun backend activé.
//!
//! API sûre sur slices, sans `unsafe`.

use thiserror::Error;

/// Erreurs SIMD.
#[derive(Debug, Error)]
pub enum SimdError {
    #[error("dimensions incompatibles: {0} vs {1}")]
    Dim(usize, usize),
}

/// Résultat spécialisé.
pub type Result<T> = std::result::Result<T, SimdError>;

// ================================================================================================
// Backend selection
// ================================================================================================

cfg_if::cfg_if! {
    if #[cfg(feature = "wide")] {
        mod backend {
            pub const LANES: usize = 4;
            pub type Vf = wide::f32x4;
            #[inline] pub fn set1(x: f32) -> Vf { Vf::from([x; LANES]) }
            #[inline] pub fn load(a: &[f32]) -> Vf { Vf::from([a[0],a[1],a[2],a[3]]) }
            #[inline] pub fn store(v: Vf, out: &mut [f32]) { out.copy_from_slice(&v.to_array()); }
            #[inline] pub fn add(a: Vf, b: Vf) -> Vf { a + b }
            #[inline] pub fn mul(a: Vf, b: Vf) -> Vf { a * b }
            #[inline] pub fn fmadd(a: Vf, b: Vf, c: Vf) -> Vf { a.mul_add(b, c) }
            #[inline] pub fn hsum(v: Vf) -> f32 { let a = v.to_array(); a.iter().copied().sum() }
        }
    } else if #[cfg(feature = "packed_simd")] {
        mod backend {
            pub const LANES: usize = 8;
            pub type Vf = packed_simd_2::f32x8;
            #[inline] pub fn set1(x: f32) -> Vf { Vf::from_array([x; LANES]) }
            #[inline] pub fn load(a: &[f32]) -> Vf { Vf::from_array([a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7]]) }
            #[inline] pub fn store(v: Vf, out: &mut [f32]) { out.copy_from_slice(&v.to_array()); }
            #[inline] pub fn add(a: Vf, b: Vf) -> Vf { a + b }
            #[inline] pub fn mul(a: Vf, b: Vf) -> Vf { a * b }
            #[inline] pub fn fmadd(a: Vf, b: Vf, c: Vf) -> Vf { a * b + c }
            #[inline] pub fn hsum(v: Vf) -> f32 { v.to_array().iter().copied().sum() }
        }
    } else {
        mod backend {
            pub const LANES: usize = 1;
            pub type Vf = f32;
            #[inline] pub fn set1(x: f32) -> Vf { x }
            #[inline] pub fn load(a: &[f32]) -> Vf { a[0] }
            #[inline] pub fn store(v: Vf, out: &mut [f32]) { out[0] = v; }
            #[inline] pub fn add(a: Vf, b: Vf) -> Vf { a + b }
            #[inline] pub fn mul(a: Vf, b: Vf) -> Vf { a * b }
            #[inline] pub fn fmadd(a: Vf, b: Vf, c: Vf) -> Vf { a * b + c }
            #[inline] pub fn hsum(v: Vf) -> f32 { v }
        }
    }
}

use backend::*;

// ================================================================================================
// Public API
// ================================================================================================

/// Addition élément par élément. Longueurs identiques requises.
pub fn add_vec(a: &[f32], b: &[f32]) -> Result<Vec<f32>> {
    ensure_same_len(a, b)?;
    Ok(binary_op(a, b, add))
}

/// Multiplication élément par élément. Longueurs identiques requises.
pub fn mul_vec(a: &[f32], b: &[f32]) -> Result<Vec<f32>> {
    ensure_same_len(a, b)?;
    Ok(binary_op(a, b, mul))
}

/// y <- a*x + y (SAXPY). Longueurs x et y identiques.
pub fn saxpy(a: f32, x: &[f32], y: &[f32]) -> Result<Vec<f32>> {
    ensure_same_len(x, y)?;
    let mut out = vec![0.0; x.len()];
    let lanes = LANES;
    let mut i = 0;
    let av = set1(a);
    while i + lanes <= x.len() {
        let xv = load(&x[i..i+lanes]);
        let yv = load(&y[i..i+lanes]);
        let zv = fmadd(av, xv, yv);
        store(zv, &mut out[i..i+lanes]);
        i += lanes;
    }
    while i < x.len() { out[i] = a * x[i] + y[i]; i += 1; }
    Ok(out)
}

/// Produit scalaire.
pub fn dot(a: &[f32], b: &[f32]) -> Result<f32> {
    ensure_same_len(a, b)?;
    let mut acc = 0.0f32;
    let lanes = LANES;
    let mut i = 0;
    while i + lanes <= a.len() {
        let av = load(&a[i..i+lanes]);
        let bv = load(&b[i..i+lanes]);
        acc += hsum(mul(av, bv));
        i += lanes;
    }
    while i < a.len() { acc += a[i] * b[i]; i += 1; }
    Ok(acc)
}

/// Somme des éléments.
pub fn sum(x: &[f32]) -> f32 {
    let mut acc = 0.0f32;
    let lanes = LANES;
    let mut i = 0;
    while i + lanes <= x.len() {
        let xv = load(&x[i..i+lanes]);
        acc += hsum(xv);
        i += lanes;
    }
    while i < x.len() { acc += x[i]; i += 1; }
    acc
}

/// Applique ReLU en place.
pub fn relu_inplace(x: &mut [f32]) {
    for v in x { if *v < 0.0 { *v = 0.0; } }
}

/// Norme L2.
pub fn l2_norm(x: &[f32]) -> f32 { sumsq(x).sqrt() }

/// Somme des carrés.
pub fn sumsq(x: &[f32]) -> f32 {
    let mut acc = 0.0f32;
    let lanes = LANES;
    let mut i = 0;
    while i + lanes <= x.len() {
        let xv = load(&x[i..i+lanes]);
        let sq = mul(xv, xv);
        acc += hsum(sq);
        i += lanes;
    }
    while i < x.len() { acc += x[i]*x[i]; i += 1; }
    acc
}

// ================================================================================================
// Internals
// ================================================================================================

fn ensure_same_len(a: &[f32], b: &[f32]) -> Result<()> {
    if a.len() != b.len() { return Err(SimdError::Dim(a.len(), b.len())); }
    Ok(())
}

fn binary_op(a: &[f32], b: &[f32], f: fn(Vf, Vf) -> Vf) -> Vec<f32> {
    let mut out = vec![0.0; a.len()];
    let lanes = LANES;
    let mut i = 0;
    while i + lanes <= a.len() {
        let av = load(&a[i..i+lanes]);
        let bv = load(&b[i..i+lanes]);
        let zv = f(av, bv);
        store(zv, &mut out[i..i+lanes]);
        i += lanes;
    }
    while i < a.len() { out[i] = f32_bin(f, a[i], b[i]); i += 1; }
    out
}

#[inline]
fn f32_bin(f: fn(Vf, Vf) -> Vf, x: f32, y: f32) -> f32 {
    // Utilise le fallback scalaire de l'opération choisie
    if LANES == 1 {
        // Safety: in scalaire, Vf == f32, add/mul opère directement
        let z = f(x, y);
        return z;
    }
    // Pour SIMD, réutilise le chemin vectoriel sur 1 lane en construisant un bloc
    let mut tmp_in = vec![0.0f32; LANES];
    tmp_in[0] = x; let a = load(&tmp_in);
    tmp_in[0] = y; let b = load(&tmp_in);
    let r = f(a, b);
    let mut tmp_out = vec![0.0f32; LANES];
    store(r, &mut tmp_out);
    tmp_out[0]
}

// ================================================================================================
// Tests
// ================================================================================================
#[cfg(test)]
mod tests {
    use super::*;

    fn approx(a: f32, b: f32) -> bool { (a-b).abs() < 1e-5 }

    #[test]
    fn add_mul_dot_sum() {
        let a = vec![1.0,2.0,3.0,4.0,5.0,6.0];
        let b = vec![6.0,5.0,4.0,3.0,2.0,1.0];
        let r = add_vec(&a,&b).unwrap();
        assert_eq!(r, vec![7.0,7.0,7.0,7.0,7.0,7.0]);
        let r2 = mul_vec(&a,&b).unwrap();
        assert_eq!(r2, vec![6.0,10.0,12.0,12.0,10.0,6.0]);
        let d = dot(&a,&b).unwrap();
        assert!(approx(d, 56.0));
        let s = sum(&a);
        assert!(approx(s, 21.0));
    }

    #[test]
    fn saxpy_ok() {
        let x = vec![1.0,2.0,3.0,4.0];
        let y = vec![1.0,1.0,1.0,1.0];
        let z = saxpy(2.0, &x, &y).unwrap();
        assert_eq!(z, vec![3.0,5.0,7.0,9.0]);
    }

    #[test]
    fn relu_and_norm() {
        let mut x = vec![-1.0, 0.5, -0.2, 3.0];
        relu_inplace(&mut x);
        assert_eq!(x, vec![0.0,0.5,0.0,3.0]);
        let n = l2_norm(&x);
        assert!(approx(n, (0.5f32*0.5 + 9.0).sqrt() as f32));
    }

    #[test]
    fn dims_error() {
        let a = vec![1.0,2.0,3.0];
        let b = vec![1.0,2.0];
        assert!(matches!(add_vec(&a,&b), Err(SimdError::Dim(3,2))));
    }
}
