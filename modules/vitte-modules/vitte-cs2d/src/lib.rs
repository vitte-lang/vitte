#![cfg_attr(not(feature = "std"), no_std)]
//! vitte-cs2d — utilitaires 2D compacts et solides pour Vitte
//!
//! Objectifs :
//! - math 2D (Vec2, Mat3) avec f32 par défaut (activez la feature `f64` pour f64)
//! - géométrie (Rect, Circle, Segment, Line)
//! - collisions et balayage (AABB, circle, segment)
//! - grille de hachage spatiale minimaliste
//! - A* sur grille pour pathfinding
//! - PRNG LCG sans dépendances
//!
//! Sans dépendances par défaut. `serde` est optionnel.

#[cfg(not(feature = "std"))]
extern crate alloc;

#[cfg(not(feature = "std"))]
use alloc::{
    collections::{BTreeMap, BTreeSet, BinaryHeap},
    vec,
    vec::Vec,
};
#[cfg(feature = "std")]
use std::{
    collections::{BinaryHeap, HashMap, HashSet},
    vec,
    vec::Vec,
};

use core::{
    cmp::Ordering,
    fmt,
    hash::Hash,
    ops::{Add, AddAssign, Div, DivAssign, Mul, MulAssign, Neg, Sub, SubAssign},
};

// =============================== Scalar ===============================
#[cfg(feature = "f64")]
pub type Scalar = f64;
#[cfg(not(feature = "f64"))]
pub type Scalar = f32;

pub const fn s(v: f32) -> Scalar {
    v as Scalar
}

// ================================ Vec2 ================================
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Copy, Clone, Default, PartialEq, PartialOrd)]
#[repr(C)]
pub struct Vec2 {
    pub x: Scalar,
    pub y: Scalar,
}

impl fmt::Debug for Vec2 {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "Vec2({:.4}, {:.4})", self.x, self.y)
    }
}

impl Vec2 {
    #[inline]
    pub const fn new(x: Scalar, y: Scalar) -> Self {
        Self { x, y }
    }
    #[inline]
    pub const fn splat(v: Scalar) -> Self {
        Self { x: v, y: v }
    }
    #[inline]
    pub const fn zero() -> Self {
        Self { x: 0.0 as Scalar, y: 0.0 as Scalar }
    }
    #[inline]
    pub const fn one() -> Self {
        Self { x: 1.0 as Scalar, y: 1.0 as Scalar }
    }
    #[inline]
    pub const fn unit_x() -> Self {
        Self { x: 1.0 as Scalar, y: 0.0 as Scalar }
    }
    #[inline]
    pub const fn unit_y() -> Self {
        Self { x: 0.0 as Scalar, y: 1.0 as Scalar }
    }

    #[inline]
    pub fn dot(self, rhs: Self) -> Scalar {
        self.x * rhs.x + self.y * rhs.y
    }
    #[inline]
    pub fn perp(self) -> Self {
        Self::new(-self.y, self.x)
    }
    #[inline]
    pub fn length2(self) -> Scalar {
        self.dot(self)
    }
    #[inline]
    pub fn length(self) -> Scalar {
        self.length2().sqrt()
    }
    #[inline]
    pub fn distance(self, rhs: Self) -> Scalar {
        (self - rhs).length()
    }
    #[inline]
    pub fn normalized(self) -> Self {
        let l = self.length();
        if l > 0.0 as Scalar {
            self / l
        } else {
            Self::zero()
        }
    }
    #[inline]
    pub fn clamp_length(self, min: Scalar, max: Scalar) -> Self {
        let l2 = self.length2();
        if l2 == 0.0 as Scalar {
            return Self::zero();
        }
        let l = l2.sqrt();
        let cl = if l < min {
            min
        } else if l > max {
            max
        } else {
            l
        };
        self * (cl / l)
    }
    #[inline]
    pub fn rotate(self, radians: Scalar) -> Self {
        let (s, c) = radians.sin_cos();
        Self::new(self.x * c - self.y * s, self.x * s + self.y * c)
    }
    #[inline]
    pub fn lerp(a: Self, b: Self, t: Scalar) -> Self {
        a + (b - a) * t
    }
    #[inline]
    pub fn hadamard(self, rhs: Self) -> Self {
        Self::new(self.x * rhs.x, self.y * rhs.y)
    }
    #[inline]
    pub fn is_finite(&self) -> bool {
        self.x.is_finite() && self.y.is_finite()
    }
}

impl Add for Vec2 {
    type Output = Self;
    #[inline]
    fn add(self, rhs: Self) -> Self {
        Self::new(self.x + rhs.x, self.y + rhs.y)
    }
}
impl Sub for Vec2 {
    type Output = Self;
    #[inline]
    fn sub(self, rhs: Self) -> Self {
        Self::new(self.x - rhs.x, self.y - rhs.y)
    }
}
impl Mul<Scalar> for Vec2 {
    type Output = Self;
    #[inline]
    fn mul(self, s: Scalar) -> Self {
        Self::new(self.x * s, self.y * s)
    }
}
impl Div<Scalar> for Vec2 {
    type Output = Self;
    #[inline]
    fn div(self, s: Scalar) -> Self {
        Self::new(self.x / s, self.y / s)
    }
}
impl AddAssign for Vec2 {
    #[inline]
    fn add_assign(&mut self, rhs: Self) {
        self.x += rhs.x;
        self.y += rhs.y;
    }
}
impl SubAssign for Vec2 {
    #[inline]
    fn sub_assign(&mut self, rhs: Self) {
        self.x -= rhs.x;
        self.y -= rhs.y;
    }
}
impl MulAssign<Scalar> for Vec2 {
    #[inline]
    fn mul_assign(&mut self, s: Scalar) {
        self.x *= s;
        self.y *= s;
    }
}
impl DivAssign<Scalar> for Vec2 {
    #[inline]
    fn div_assign(&mut self, s: Scalar) {
        self.x /= s;
        self.y /= s;
    }
}
impl Neg for Vec2 {
    type Output = Self;
    #[inline]
    fn neg(self) -> Self {
        Self::new(-self.x, -self.y)
    }
}

// ================================ Mat3 ================================
/// Matrice 3x3 (affine 2D). Colonne-major.
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Copy, Clone, PartialEq)]
pub struct Mat3 {
    pub m: [Scalar; 9],
}

impl fmt::Debug for Mat3 {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "Mat3({:?})", &self.m)
    }
}

impl Mat3 {
    #[inline]
    pub const fn identity() -> Self {
        Self { m: [1.0 as Scalar, 0.0 as Scalar, 0.0 as Scalar, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0] }
    }
    #[inline]
    pub const fn from_cols(c0: [Scalar; 3], c1: [Scalar; 3], c2: [Scalar; 3]) -> Self {
        Self { m: [c0[0], c0[1], c0[2], c1[0], c1[1], c1[2], c2[0], c2[1], c2[2]] }
    }
    #[inline]
    pub fn translate(v: Vec2) -> Self {
        Self::from_cols([1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [v.x, v.y, 1.0])
    }
    #[inline]
    pub fn scale(v: Vec2) -> Self {
        Self::from_cols([v.x, 0.0, 0.0], [0.0, v.y, 0.0], [0.0, 0.0, 1.0])
    }
    #[inline]
    pub fn rotate(r: Scalar) -> Self {
        let (s, c) = r.sin_cos();
        Self::from_cols([c, s, 0.0], [-s, c, 0.0], [0.0, 0.0, 1.0])
    }
    #[inline]
    pub fn mul(self, rhs: Self) -> Self {
        let a = &self.m;
        let b = &rhs.m;
        let m00 = a[0] * b[0] + a[3] * b[1] + a[6] * b[2];
        let m01 = a[1] * b[0] + a[4] * b[1] + a[7] * b[2];
        let m02 = a[2] * b[0] + a[5] * b[1] + a[8] * b[2];
        let m10 = a[0] * b[3] + a[3] * b[4] + a[6] * b[5];
        let m11 = a[1] * b[3] + a[4] * b[4] + a[7] * b[5];
        let m12 = a[2] * b[3] + a[5] * b[4] + a[8] * b[5];
        let m20 = a[0] * b[6] + a[3] * b[7] + a[6] * b[8];
        let m21 = a[1] * b[6] + a[4] * b[7] + a[7] * b[8];
        let m22 = a[2] * b[6] + a[5] * b[7] + a[8] * b[8];
        Self { m: [m00, m01, m02, m10, m11, m12, m20, m21, m22] }
    }
    #[inline]
    pub fn transform_point(&self, p: Vec2) -> Vec2 {
        let a = &self.m;
        Vec2::new(a[0] * p.x + a[3] * p.y + a[6], a[1] * p.x + a[4] * p.y + a[7])
    }
    #[inline]
    pub fn transform_vec(&self, v: Vec2) -> Vec2 {
        let a = &self.m;
        Vec2::new(a[0] * v.x + a[3] * v.y, a[1] * v.x + a[4] * v.y)
    }
    pub fn inverse(&self) -> Option<Self> {
        let m = &self.m;
        let (a, b, c, d, e, f, g, h, i) = (m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8]);
        let co00 = e * i - f * h;
        let co01 = -(d * i - f * g);
        let co02 = d * h - e * g;
        let co10 = -(b * i - c * h);
        let co11 = a * i - c * g;
        let co12 = -(a * h - b * g);
        let co20 = b * f - c * e;
        let co21 = -(a * f - c * d);
        let co22 = a * e - b * d;
        let det = a * co00 + b * co01 + c * co02;
        if det.abs() <= (1e-9 as Scalar) {
            return None;
        }
        let inv_det = 1.0 as Scalar / det;
        Some(Self {
            m: [
                co00 * inv_det,
                co10 * inv_det,
                co20 * inv_det,
                co01 * inv_det,
                co11 * inv_det,
                co21 * inv_det,
                co02 * inv_det,
                co12 * inv_det,
                co22 * inv_det,
            ],
        })
    }
}

// =============================== Geometry =============================
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Copy, Clone, Debug, Default, PartialEq)]
#[repr(C)]
pub struct Rect {
    pub x: Scalar,
    pub y: Scalar,
    pub w: Scalar,
    pub h: Scalar,
}
impl Rect {
    #[inline]
    pub const fn new(x: Scalar, y: Scalar, w: Scalar, h: Scalar) -> Self {
        Self { x, y, w, h }
    }
    #[inline]
    pub fn min(&self) -> Vec2 {
        Vec2::new(self.x, self.y)
    }
    #[inline]
    pub fn max(&self) -> Vec2 {
        Vec2::new(self.x + self.w, self.y + self.h)
    }
    #[inline]
    pub fn center(&self) -> Vec2 {
        Vec2::new(self.x + self.w * 0.5, self.y + self.h * 0.5)
    }
    #[inline]
    pub fn contains_pt(&self, p: Vec2) -> bool {
        p.x >= self.x && p.y >= self.y && p.x <= self.x + self.w && p.y <= self.y + self.h
    }
    #[inline]
    pub fn intersects(&self, b: &Rect) -> bool {
        !(self.x + self.w < b.x
            || b.x + b.w < self.x
            || self.y + self.h < b.y
            || b.y + b.h < self.y)
    }
    #[inline]
    pub fn intersection(&self, b: &Rect) -> Option<Rect> {
        let x1 = self.x.max(b.x);
        let y1 = self.y.max(b.y);
        let x2 = (self.x + self.w).min(b.x + b.w);
        let y2 = (self.y + self.h).min(b.y + b.h);
        if x2 >= x1 && y2 >= y1 {
            Some(Rect::new(x1, y1, x2 - x1, y2 - y1))
        } else {
            None
        }
    }
    #[inline]
    pub fn expanded(&self, by: Scalar) -> Rect {
        Rect::new(self.x - by, self.y - by, self.w + by * 2.0, self.h + by * 2.0)
    }
}

#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Copy, Clone, Debug, Default, PartialEq)]
#[repr(C)]
pub struct Circle {
    pub c: Vec2,
    pub r: Scalar,
}
impl Circle {
    #[inline]
    pub const fn new(c: Vec2, r: Scalar) -> Self {
        Self { c, r }
    }
}

#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Copy, Clone, Debug, Default, PartialEq)]
#[repr(C)]
pub struct Segment {
    pub a: Vec2,
    pub b: Vec2,
}
impl Segment {
    #[inline]
    pub const fn new(a: Vec2, b: Vec2) -> Self {
        Self { a, b }
    }
    pub fn direction(&self) -> Vec2 {
        self.b - self.a
    }
    pub fn closest_t_to_point(&self, p: Vec2) -> Scalar {
        let ab = self.b - self.a;
        let ap = p - self.a;
        let ab2 = ab.length2();
        if ab2 <= 0.0 {
            return 0.0;
        };
        (ap.dot(ab) / ab2).clamp(0.0, 1.0)
    }
    pub fn closest_point(&self, p: Vec2) -> Vec2 {
        let t = self.closest_t_to_point(p);
        self.a + (self.b - self.a) * t
    }
}

#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Copy, Clone, Debug, Default, PartialEq)]
#[repr(C)]
pub struct Line {
    pub p: Vec2,
    pub d: Vec2,
}
impl Line {
    #[inline]
    pub const fn new(p: Vec2, d: Vec2) -> Self {
        Self { p, d }
    }
}

// ============================== Collision =============================
#[derive(Copy, Clone, Debug, Default, PartialEq)]
pub struct Hit {
    pub time: Scalar, // t d'entrée [0,1]
    pub normal: Vec2, // normale du contact
    pub delta: Vec2,  // déplacement corrigé (vel * time)
}

pub mod collide {
    use super::*;

    #[inline]
    pub fn aabb_aabb(a: &Rect, b: &Rect) -> bool {
        a.intersects(b)
    }
    #[inline]
    pub fn circle_circle(a: &Circle, b: &Circle) -> bool {
        a.c.distance(b.c) <= (a.r + b.r)
    }
    pub fn rect_circle(r: &Rect, c: &Circle) -> bool {
        let cx = c.c.x.clamp(r.x, r.x + r.w);
        let cy = c.c.y.clamp(r.y, r.y + r.h);
        let dx = c.c.x - cx;
        let dy = c.c.y - cy;
        (dx * dx + dy * dy) <= c.r * c.r
    }

    /// Intersections de segments (retourne t,u si croisement strict, inclusifs bord)
    pub fn segment_segment(a: &Segment, b: &Segment) -> Option<(Scalar, Scalar)> {
        let r = a.b - a.a;
        let s = b.b - b.a;
        let qp = b.a - a.a;
        let rxs = r.x * s.y - r.y * s.x;
        let qpxr = qp.x * r.y - qp.y * r.x;
        if rxs.abs() <= (1e-9 as Scalar) {
            return None;
        } // parallèles ou colinéaires
        let t = (qp.x * s.y - qp.y * s.x) / rxs;
        let u = qpxr / rxs;
        if t >= 0.0 && t <= 1.0 && u >= 0.0 && u <= 1.0 {
            Some((t, u))
        } else {
            None
        }
    }

    /// Balayage AABB contre AABB (Minkowski). Retourne le premier impact dans [0,1] si collision.
    pub fn sweep_aabb(moving: &Rect, vel: Vec2, target: &Rect) -> Option<Hit> {
        // Minkowski sum: on soustrait target
        let expanded = Rect::new(
            target.x - moving.w,
            target.y - moving.h,
            target.w + moving.w,
            target.h + moving.h,
        );
        // Ray cast from moving.pos to expanded
        let invx = if vel.x != 0.0 { 1.0 / vel.x } else { Scalar::INFINITY } as Scalar;
        let invy = if vel.y != 0.0 { 1.0 / vel.y } else { Scalar::INFINITY } as Scalar;
        let tx1 = (expanded.x - moving.x) * invx;
        let tx2 = (expanded.x + expanded.w - moving.x) * invx;
        let ty1 = (expanded.y - moving.y) * invy;
        let ty2 = (expanded.y + expanded.h - moving.y) * invy;
        let (txmin, txmax) = if tx1 < tx2 { (tx1, tx2) } else { (tx2, tx1) };
        let (tymin, tymax) = if ty1 < ty2 { (ty1, ty2) } else { (ty2, ty1) };
        let t_enter = txmin.max(tymin);
        let t_exit = txmax.min(tymax);
        if t_enter > t_exit || t_exit < 0.0 || t_enter > 1.0 {
            return None;
        }
        let normal = if txmin > tymin {
            Vec2::new(if invx < 0.0 { 1.0 } else { -1.0 } as Scalar, 0.0 as Scalar)
        } else {
            Vec2::new(0.0 as Scalar, if invy < 0.0 { 1.0 } else { -1.0 } as Scalar)
        };
        Some(Hit { time: t_enter.max(0.0), normal, delta: vel * t_enter.max(0.0) })
    }
}

// =========================== Spatial Hash Grid ========================
#[cfg(feature = "std")]
pub mod spatial {
    use super::*;
    #[derive(Clone)]
    pub struct SpatialHashGrid<T: Clone + Eq + Hash> {
        pub cell: Scalar,
        pub inv: Scalar,
        buckets: HashMap<(i32, i32), Vec<T>>,
    }
    impl<T: Clone + Eq + Hash> SpatialHashGrid<T> {
        pub fn new(cell: Scalar) -> Self {
            Self { cell, inv: 1.0 / cell, buckets: HashMap::new() }
        }
        #[inline]
        fn key(&self, p: Vec2) -> (i32, i32) {
            ((p.x * self.inv).floor() as i32, (p.y * self.inv).floor() as i32)
        }
        #[inline]
        fn keys_rect(&self, r: &Rect) -> impl Iterator<Item = (i32, i32)> {
            let min = self.key(r.min());
            let max = self.key(r.max());
            (min.0..=max.0).flat_map(move |x| (min.1..=max.1).map(move |y| (x, y)))
        }
        pub fn insert_point(&mut self, id: T, p: Vec2) {
            self.buckets.entry(self.key(p)).or_default().push(id);
        }
        pub fn insert_rect(&mut self, id: T, r: Rect) {
            for k in self.keys_rect(&r) {
                self.buckets.entry(k).or_default().push(id.clone());
            }
        }
        pub fn query_rect<'a>(&'a self, r: &Rect) -> impl Iterator<Item = &'a T> {
            let mut out: Vec<&T> = Vec::new();
            let mut seen: HashSet<*const T> = HashSet::new();
            for k in self.keys_rect(r) {
                if let Some(v) = self.buckets.get(&k) {
                    for it in v {
                        let ptr = it as *const T;
                        if seen.insert(ptr) {
                            out.push(it);
                        }
                    }
                }
            }
            out.into_iter()
        }
        pub fn clear(&mut self) {
            self.buckets.clear();
        }
    }
}

#[cfg(not(feature = "std"))]
pub mod spatial {
    use super::*;
    #[derive(Clone)]
    pub struct SpatialHashGrid<T: Clone + Ord> {
        pub cell: Scalar,
        pub inv: Scalar,
        buckets: BTreeMap<(i32, i32), Vec<T>>,
    }
    impl<T: Clone + Ord> SpatialHashGrid<T> {
        pub fn new(cell: Scalar) -> Self {
            Self { cell, inv: 1.0 / cell, buckets: BTreeMap::new() }
        }
        #[inline]
        fn key(&self, p: Vec2) -> (i32, i32) {
            ((p.x * self.inv).floor() as i32, (p.y * self.inv).floor() as i32)
        }
        #[inline]
        fn keys_rect(&self, r: &Rect) -> impl Iterator<Item = (i32, i32)> {
            let min = self.key(r.min());
            let max = self.key(r.max());
            (min.0..=max.0).flat_map(move |x| (min.1..=max.1).map(move |y| (x, y)))
        }
        pub fn insert_point(&mut self, id: T, p: Vec2) {
            self.buckets.entry(self.key(p)).or_default().push(id);
        }
        pub fn insert_rect(&mut self, id: T, r: Rect) {
            for k in self.keys_rect(&r) {
                self.buckets.entry(k).or_default().push(id.clone());
            }
        }
        pub fn query_rect<'a>(&'a self, r: &Rect) -> impl Iterator<Item = &'a T> {
            let mut out: Vec<&T> = Vec::new();
            let mut seen: BTreeSet<&T> = BTreeSet::new();
            for k in self.keys_rect(r) {
                if let Some(v) = self.buckets.get(&k) {
                    for it in v {
                        if seen.insert(it) {
                            out.push(it);
                        }
                    }
                }
            }
            out.into_iter()
        }
        pub fn clear(&mut self) {
            self.buckets.clear();
        }
    }
}

// ============================== Pathfinding ===========================
pub mod path {
    use super::*;
    #[derive(Clone, Copy, Eq, PartialEq)]
    struct Node {
        f: i32,
        g: i32,
        x: i32,
        y: i32,
    }
    impl Ord for Node {
        fn cmp(&self, other: &Self) -> Ordering {
            other.f.cmp(&self.f).then_with(|| (self.g).cmp(&other.g))
        }
    }
    impl PartialOrd for Node {
        fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
            Some(self.cmp(other))
        }
    }

    #[cfg(not(feature = "std"))]
    use alloc::collections::BTreeMap as Map;
    #[cfg(feature = "std")]
    use std::collections::HashMap as Map;

    /// Grille booléenne (walkable=true).
    #[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
    pub struct Grid {
        pub w: i32,
        pub h: i32,
        pub cells: Vec<bool>,
    }
    impl Grid {
        pub fn new(w: i32, h: i32) -> Self {
            Self { w, h, cells: vec![true; (w * h) as usize] }
        }
        #[inline]
        fn idx(&self, x: i32, y: i32) -> usize {
            (x + y * self.w) as usize
        }
        pub fn set_block(&mut self, x: i32, y: i32, walkable: bool) {
            if x >= 0 && y >= 0 && x < self.w && y < self.h {
                let idx = self.idx(x, y);
                self.cells[idx] = walkable;
            }
        }
        #[inline]
        pub fn is_walkable(&self, x: i32, y: i32) -> bool {
            x >= 0 && y >= 0 && x < self.w && y < self.h && self.cells[self.idx(x, y)]
        }
        pub fn astar(&self, sx: i32, sy: i32, tx: i32, ty: i32) -> Option<Vec<(i32, i32)>> {
            if !self.is_walkable(sx, sy) || !self.is_walkable(tx, ty) {
                return None;
            }
            let mut open: BinaryHeap<Node> = BinaryHeap::new();
            let mut came: Map<(i32, i32), (i32, i32)> = Map::new();
            let mut gscore: Map<(i32, i32), i32> = Map::new();
            let h = |x: i32, y: i32| (tx - x).abs() + (ty - y).abs();
            let start = Node { f: h(sx, sy), g: 0, x: sx, y: sy };
            open.push(start);
            gscore.insert((sx, sy), 0);
            let neigh = |x: i32, y: i32| [(x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)];
            while let Some(Node { f: _, g, x, y }) = open.pop() {
                if x == tx && y == ty {
                    // reconstruit
                    let mut cur = (x, y);
                    let mut path = Vec::new();
                    path.push(cur);
                    while let Some(&p) = came.get(&cur) {
                        cur = p;
                        path.push(cur);
                        if cur == (sx, sy) {
                            break;
                        }
                    }
                    path.reverse();
                    return Some(path);
                }
                for (nx, ny) in neigh(x, y) {
                    if !self.is_walkable(nx, ny) {
                        continue;
                    }
                    let tentative = g + 1;
                    let better = match gscore.get(&(nx, ny)) {
                        Some(&old) => tentative < old,
                        None => true,
                    };
                    if better {
                        came.insert((nx, ny), (x, y));
                        gscore.insert((nx, ny), tentative);
                        let node = Node { g: tentative, f: tentative + h(nx, ny), x: nx, y: ny };
                        open.push(node);
                    }
                }
            }
            None
        }
    }
}

// ================================ RNG =================================
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Clone, Debug)]
pub struct Lcg {
    state: u64,
}
impl Lcg {
    pub fn new(seed: u64) -> Self {
        let s = if seed == 0 { 0x9E3779B97F4A7C15 } else { seed };
        Self { state: s }
    }
    #[inline]
    fn next_u64(&mut self) -> u64 {
        // xorshift* simple
        let mut x = self.state;
        x ^= x >> 12;
        x ^= x << 25;
        x ^= x >> 27;
        self.state = x;
        x.wrapping_mul(0x2545F4914F6CDD1D)
    }
    #[inline]
    pub fn next_u32(&mut self) -> u32 {
        (self.next_u64() >> 32) as u32
    }
    #[inline]
    pub fn next_f32(&mut self) -> f32 {
        (self.next_u32() as f32) / (u32::MAX as f32)
    }
    #[inline]
    pub fn range_f32(&mut self, min: f32, max: f32) -> f32 {
        min + (max - min) * self.next_f32()
    }
    #[inline]
    pub fn jitter2(&mut self, p: Vec2, amt: Scalar) -> Vec2 {
        let jx = self.range_f32(-1.0, 1.0) as Scalar * amt;
        let jy = self.range_f32(-1.0, 1.0) as Scalar * amt;
        Vec2::new(p.x + jx, p.y + jy)
    }
}

// ============================== Transforms ============================
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Copy, Clone, Debug, PartialEq)]
pub struct Transform2D {
    pub t: Vec2,
    pub r: Scalar,
    pub s: Vec2,
}
impl Default for Transform2D {
    fn default() -> Self {
        Self { t: Vec2::zero(), r: 0.0 as Scalar, s: Vec2::one() }
    }
}
impl Transform2D {
    pub fn matrix(&self) -> Mat3 {
        Mat3::translate(self.t).mul(Mat3::rotate(self.r)).mul(Mat3::scale(self.s))
    }
    pub fn apply_to_point(&self, p: Vec2) -> Vec2 {
        self.matrix().transform_point(p)
    }
    pub fn apply_to_vec(&self, v: Vec2) -> Vec2 {
        self.matrix().transform_vec(v)
    }
}

// ============================== Prelude ===============================
pub mod prelude {
    pub use super::{
        collide, path::Grid, spatial::SpatialHashGrid, Circle, Lcg, Line, Mat3, Rect, Scalar,
        Segment, Transform2D, Vec2,
    };
}

// ================================ Tests ===============================
#[cfg(test)]
mod tests {
    use super::*;
    use core::f32::consts::PI;

    #[test]
    fn vec2_ops() {
        let a = Vec2::new(3.0, 4.0);
        assert!((a.length() - 5.0).abs() < 1e-6);
        let b = Vec2::unit_x();
        assert_eq!((a + b).x, 4.0);
    }
    #[test]
    fn mat3_inv() {
        let m = Mat3::translate(Vec2::new(2.0, 3.0))
            .mul(Mat3::rotate(PI / 4.0))
            .mul(Mat3::scale(Vec2::new(2.0, 2.0)));
        let inv = m.inverse().unwrap();
        let p = Vec2::new(1.0, 2.0);
        let q = m.transform_point(p);
        let r = inv.transform_point(q);
        assert!((r.x - p.x).abs() < 1e-5 && (r.y - p.y).abs() < 1e-5);
    }
    #[test]
    fn rect_circle_hit() {
        let r = Rect::new(0.0, 0.0, 10.0, 10.0);
        let c = Circle::new(Vec2::new(12.0, 5.0), 2.0);
        assert!(collide::rect_circle(&r, &c));
    }
    #[test]
    fn sweep() {
        let m = Rect::new(0.0, 0.0, 2.0, 2.0);
        let t = Rect::new(5.0, 0.0, 2.0, 2.0);
        let h = collide::sweep_aabb(&m, Vec2::new(6.0, 0.0), &t).unwrap();
        assert!(h.time > 0.0 && h.time < 1.0);
    }
    #[test]
    fn a_star_basic() {
        let mut g = path::Grid::new(5, 5);
        g.set_block(2, 2, false);
        let p = g.astar(0, 0, 4, 4).unwrap();
        assert!(p.first() == Some(&(0, 0)) && p.last() == Some(&(4, 4)));
    }
}
