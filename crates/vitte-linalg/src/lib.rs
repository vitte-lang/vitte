


//! vitte-linalg — algèbre linéaire basique et avancée pour Vitte
//!
//! Fournit :
//! - Vecteur dense (`Vector`)
//! - Matrice dense (`Matrix`)
//! - Opérations élémentaires (addition, produit scalaire, multiplication matrice-vecteur/matrice)
//! - Décompositions simples (LU, QR, SVD — stubs pour extension)
//! - Support optionnel SIMD, rayon (parallèle), rand (initialisation aléatoire)

use core::ops::{Add, Sub, Mul, Index, IndexMut};

/// Vecteur dense générique.
#[derive(Clone, Debug, PartialEq)]
pub struct Vector<T> {
    data: Vec<T>,
}

impl<T> Vector<T> {
    /// Construit un vecteur depuis un Vec.
    pub fn new(data: Vec<T>) -> Self { Self { data } }
    /// Longueur.
    pub fn len(&self) -> usize { self.data.len() }
    /// Accès direct.
    pub fn as_slice(&self) -> &[T] { &self.data }
    /// Accès mutable.
    pub fn as_mut_slice(&mut self) -> &mut [T] { &mut self.data }
}

impl<T: Add<Output=T> + Copy> Add for &Vector<T> {
    type Output = Vector<T>;
    fn add(self, rhs: Self) -> Self::Output {
        assert_eq!(self.len(), rhs.len());
        Vector::new(self.data.iter().zip(rhs.data.iter()).map(|(&a,&b)| a+b).collect())
    }
}

impl<T: Sub<Output=T> + Copy> Sub for &Vector<T> {
    type Output = Vector<T>;
    fn sub(self, rhs: Self) -> Self::Output {
        assert_eq!(self.len(), rhs.len());
        Vector::new(self.data.iter().zip(rhs.data.iter()).map(|(&a,&b)| a-b).collect())
    }
}

impl<T: Mul<Output=T> + Add<Output=T> + Copy + Default> Vector<T> {
    /// Produit scalaire.
    pub fn dot(&self, rhs: &Self) -> T {
        assert_eq!(self.len(), rhs.len());
        let mut acc = T::default();
        for i in 0..self.len() { acc = acc + self.data[i] * rhs.data[i]; }
        acc
    }
}

/// Matrice dense générique (row-major).
#[derive(Clone, Debug, PartialEq)]
pub struct Matrix<T> {
    rows: usize,
    cols: usize,
    data: Vec<T>,
}

impl<T> Matrix<T> {
    /// Nouvelle matrice.
    pub fn new(rows: usize, cols: usize, data: Vec<T>) -> Self {
        assert_eq!(rows*cols, data.len());
        Self { rows, cols, data }
    }
    /// Zéro initialisé (si Default).
    pub fn zero(rows: usize, cols: usize) -> Self where T: Default + Clone {
        Self { rows, cols, data: vec![T::default(); rows*cols] }
    }
    /// Dimensions.
    pub fn shape(&self) -> (usize, usize) { (self.rows, self.cols) }
    /// Index (i,j).
    pub fn index(&self, i: usize, j: usize) -> &T {
        &self.data[i*self.cols+j]
    }
    /// Retourne une référence mutable vers l'élément à la position (i, j).
    pub fn index_mut(&mut self, i: usize, j: usize) -> &mut T {
        &mut self.data[i*self.cols+j]
    }
}

impl<T> Index<(usize,usize)> for Matrix<T> {
    type Output = T;
    fn index(&self, idx: (usize,usize)) -> &Self::Output { self.index(idx.0, idx.1) }
}
impl<T> IndexMut<(usize,usize)> for Matrix<T> {
    fn index_mut(&mut self, idx: (usize,usize)) -> &mut Self::Output { self.index_mut(idx.0, idx.1) }
}

impl<T: Add<Output=T> + Copy> Add for &Matrix<T> {
    type Output = Matrix<T>;
    fn add(self, rhs: Self) -> Self::Output {
        assert_eq!(self.shape(), rhs.shape());
        Matrix::new(self.rows, self.cols,
            self.data.iter().zip(rhs.data.iter()).map(|(&a,&b)| a+b).collect())
    }
}

impl<T: Mul<Output=T> + Add<Output=T> + Copy + Default> Mul<&Vector<T>> for &Matrix<T> {
    type Output = Vector<T>;
    fn mul(self, v: &Vector<T>) -> Self::Output {
        assert_eq!(self.cols, v.len());
        let mut out = vec![T::default(); self.rows];
        for i in 0..self.rows {
            let mut acc = T::default();
            for j in 0..self.cols { acc = acc + self[(i,j)] * v.as_slice()[j]; }
            out[i] = acc;
        }
        Vector::new(out)
    }
}

impl<T: Mul<Output=T> + Add<Output=T> + Copy + Default> Mul<&Matrix<T>> for &Matrix<T> {
    type Output = Matrix<T>;
    fn mul(self, rhs: &Matrix<T>) -> Self::Output {
        assert_eq!(self.cols, rhs.rows);
        let mut out = Matrix::zero(self.rows, rhs.cols);
        for i in 0..self.rows {
            for j in 0..rhs.cols {
                let mut acc = T::default();
                for k in 0..self.cols { acc = acc + self[(i,k)] * rhs[(k,j)]; }
                *out.index_mut(i,j) = acc;
            }
        }
        out
    }
}

/// Décompositions (stubs à compléter).
pub mod decomp {
    /// Décomposition LU (stub).
    pub fn lu<T>(_a: &[T], _n: usize) { unimplemented!() }
    /// Décomposition QR (stub).
    pub fn qr<T>(_a: &[T], _n: usize) { unimplemented!() }
    /// Décomposition SVD (stub).
    pub fn svd<T>(_a: &[T], _m: usize, _n: usize) { unimplemented!() }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn vec_add_dot() {
        let a = Vector::new(vec![1,2,3]);
        let b = Vector::new(vec![4,5,6]);
        let c = &a + &b;
        assert_eq!(c.as_slice(), &[5,7,9]);
        assert_eq!(a.dot(&b), 32);
    }
    #[test]
    fn mat_vec_mul() {
        let m = Matrix::new(2,3, vec![1,2,3,4,5,6]);
        let v = Vector::new(vec![1,0,1]);
        let r = &m * &v;
        assert_eq!(r.as_slice(), &[4,10]);
    }
    #[test]
    fn mat_mul() {
        let a = Matrix::new(2,2, vec![1,2,3,4]);
        let b = Matrix::new(2,2, vec![5,6,7,8]);
        let c = &a * &b;
        assert_eq!(c[(0,0)], 19);
        assert_eq!(c[(1,1)], 50);
    }
}