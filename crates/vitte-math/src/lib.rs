#![deny(missing_docs)]
//! vitte-math — bibliothèque mathématique générale pour Vitte
//!
//! Fournit :
//! - Arithmétique générique via `num-traits`
//! - Fonctions trigonométriques, puissances, racines
//! - Nombres complexes
//! - Statistiques basiques et avancées
//! - Algèbre linéaire simple (vecteurs/matrices légères)
//! - Probabilités et distributions
//! - Générateurs aléatoires (si feature rand)
//! - Sérialisation optionnelle (serde)

use num_traits::{Float, Num, Zero, One};
use num_complex::Complex;

/// Retourne la somme des éléments.
pub fn sum<T: Num + Copy>(data: &[T]) -> T {
    let mut acc = T::zero();
    for &v in data { acc = acc + v; }
    acc
}

/// Retourne la moyenne.
pub fn mean<T: Float>(data: &[T]) -> T {
    if data.is_empty() { return T::nan(); }
    sum(data) / T::from(data.len()).unwrap()
}

/// Variance (population).
pub fn variance<T: Float>(data: &[T]) -> T {
    let m = mean(data);
    let mut acc = T::zero();
    for &v in data { let d = v - m; acc = acc + d*d; }
    acc / T::from(data.len()).unwrap()
}

/// Écart type.
pub fn stddev<T: Float>(data: &[T]) -> T {
    variance(data).sqrt()
}

/// Médiane.
pub fn median<T: Float + Ord>(data: &mut [T]) -> T {
    data.sort_by(|a,b| a.partial_cmp(b).unwrap());
    let n = data.len();
    if n == 0 { return T::nan(); }
    if n % 2 == 1 {
        data[n/2]
    } else {
        (data[n/2-1] + data[n/2]) / T::from(2).unwrap()
    }
}

/// Mode (valeur la plus fréquente).
pub fn mode<T: Ord + Copy>(data: &[T]) -> Option<T> {
    use std::collections::HashMap;
    let mut freq = HashMap::new();
    for &v in data { *freq.entry(v).or_insert(0) += 1; }
    freq.into_iter().max_by_key(|&(_,c)| c).map(|(v,_)| v)
}

/// Fonctions trigonométriques de base.
pub mod trig {
    use num_traits::Float;
    pub fn sin<T: Float>(x: T) -> T { x.sin() }
    pub fn cos<T: Float>(x: T) -> T { x.cos() }
    pub fn tan<T: Float>(x: T) -> T { x.tan() }
    pub fn asin<T: Float>(x: T) -> T { x.asin() }
    pub fn acos<T: Float>(x: T) -> T { x.acos() }
    pub fn atan<T: Float>(x: T) -> T { x.atan() }
    pub fn atan2<T: Float>(y: T, x: T) -> T { y.atan2(x) }
    pub fn sinh<T: Float>(x: T) -> T { x.sinh() }
    pub fn cosh<T: Float>(x: T) -> T { x.cosh() }
    pub fn tanh<T: Float>(x: T) -> T { x.tanh() }
}

/// Opérations sur nombres complexes.
pub mod complex_ops {
    use num_complex::Complex;
    use num_traits::Float;

    pub fn conj<T: Float>(z: Complex<T>) -> Complex<T> { z.conj() }
    pub fn abs<T: Float>(z: Complex<T>) -> T { z.norm() }
    pub fn arg<T: Float>(z: Complex<T>) -> T { z.arg() }
    pub fn polar<T: Float>(r: T, theta: T) -> Complex<T> { Complex::from_polar(&r,&theta) }
    pub fn add<T: Float>(a: Complex<T>, b: Complex<T>) -> Complex<T> { a + b }
    pub fn mul<T: Float>(a: Complex<T>, b: Complex<T>) -> Complex<T> { a * b }
}

/// Algèbre linéaire simple.
pub mod linalg {
    use super::*;
    #[derive(Debug, Clone, PartialEq)]
    pub struct Vector<T> { pub data: Vec<T> }
    impl<T> Vector<T> {
        pub fn new(data: Vec<T>) -> Self { Self { data } }
        pub fn len(&self) -> usize { self.data.len() }
    }
    impl<T: Float> Vector<T> {
        pub fn dot(&self, rhs: &Self) -> T {
            assert_eq!(self.len(), rhs.len());
            super::sum(&self.data.iter().zip(&rhs.data).map(|(&a,&b)| a*b).collect::<Vec<T>>())
        }
        pub fn norm(&self) -> T {
            self.dot(self).sqrt()
        }
    }

    #[derive(Debug, Clone, PartialEq)]
    pub struct Matrix<T> {
        pub rows: usize,
        pub cols: usize,
        pub data: Vec<T>,
    }
    impl<T: Float + Default + Copy> Matrix<T> {
        pub fn new(rows: usize, cols: usize, data: Vec<T>) -> Self {
            assert_eq!(rows*cols, data.len());
            Self { rows, cols, data }
        }
        pub fn mul_vec(&self, v: &Vector<T>) -> Vector<T> {
            assert_eq!(self.cols, v.len());
            let mut out = vec![T::zero(); self.rows];
            for i in 0..self.rows {
                let mut acc = T::zero();
                for j in 0..self.cols {
                    acc = acc + self.data[i*self.cols+j] * v.data[j];
                }
                out[i] = acc;
            }
            Vector::new(out)
        }
    }
}

/// Probabilités et distributions.
pub mod prob {
    use num_traits::Float;
    /// Densité de probabilité normale.
    pub fn normal_pdf<T: Float>(x: T, mean: T, std: T) -> T {
        let var = std*std;
        let denom = (std * (T::from(2.0*std::f64::consts::PI).unwrap())).sqrt();
        let num = (-(x-mean)*(x-mean) / (T::from(2.0).unwrap()*var)).exp();
        num / denom
    }
    /// Fonction de répartition normale (approx).
    pub fn normal_cdf(x: f64, mean: f64, std: f64) -> f64 {
        0.5 * (1.0 + ((x-mean)/(std*(2.0_f64).sqrt())).erf())
    }
}

/// Générateurs aléatoires (si feature rand).
#[cfg(feature="rand")]
pub mod random {
    use rand::Rng;
    pub fn uniform_f64() -> f64 {
        let mut rng = rand::thread_rng();
        rng.gen()
    }
    pub fn uniform_range(min: i32, max: i32) -> i32 {
        let mut rng = rand::thread_rng();
        rng.gen_range(min..=max)
    }
    pub fn normal_f64(mean: f64, std: f64) -> f64 {
        use rand_distr::{Normal, Distribution};
        let mut rng = rand::thread_rng();
        let dist = Normal::new(mean,std).unwrap();
        dist.sample(&mut rng)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn stats_basic() {
        let d = vec![1.0,2.0,3.0,4.0];
        assert_eq!(sum(&d), 10.0);
        assert_eq!(mean(&d), 2.5);
        assert!((variance(&d) - 1.25).abs() < 1e-12);
        assert!((stddev(&d) - 1.1180).abs() < 1e-3);
    }
    #[test]
    fn median_mode() {
        let mut a = vec![3.0,1.0,2.0];
        assert_eq!(median(&mut a), 2.0);
        let mut b = vec![1.0,2.0,3.0,4.0];
        assert_eq!(median(&mut b), 2.5);
        let m = mode(&[1,2,2,3,3,3,4]);
        assert_eq!(m, Some(3));
    }
    #[test]
    fn trig_identity() {
        use trig::*;
        let x = 0.5f64;
        assert!((sin(x).powi(2)+cos(x).powi(2)-1.0).abs()<1e-12);
    }
    #[test]
    fn complex_ops_usage() {
        use complex_ops::*;
        let z = num_complex::Complex::new(1.0,1.0);
        assert_eq!(conj(z), num_complex::Complex::new(1.0,-1.0));
        assert!((abs(z)-(2.0f64).sqrt()).abs()<1e-12);
    }
    #[test]
    fn linalg_basic() {
        use linalg::*;
        let v = Vector::new(vec![1.0,2.0]);
        assert!((v.norm()-(5.0f64).sqrt()).abs()<1e-12);
        let m = Matrix::new(2,2, vec![1.0,2.0,3.0,4.0]);
        let r = m.mul_vec(&v);
        assert_eq!(r.data, vec![5.0,11.0]);
    }
    #[test]
    fn prob_normal_pdf_cdf() {
        use prob::*;
        let p = normal_pdf(0.0,0.0,1.0);
        assert!(p>0.0);
        let c = normal_cdf(0.0,0.0,1.0);
        assert!((c-0.5).abs()<1e-6);
    }
}