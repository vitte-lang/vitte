#![cfg_attr(not(feature = "std"), no_std)]
//! vitte-gfx2d — canevas 2D minimaliste, rapide, et portable (backend-agnostic)
//!
//! Objectifs :
//! - API de dessin de haut niveau (**Canvas2D**) : `fill_rect`, `stroke_path`, `fill_path`, `circle`, `polyline`, etc.
//! - Tessellation CPU simple (stroke & fill) → **TriangleBatch** (positions/indices/couleurs/UV)
//! - Pile de transformations (Mat3), états (Paint, StrokeStyle, Blend)
//! - Backend abstrait via **RenderBackend** (wgpu/SDL/OpenGL… à brancher côté app)
//! - Zéro dépendance par défaut. `serde` optionnel.
//!
//! Ce crate s'appuie sur `vitte-cs2d` pour la géométrie (Vec2, Mat3, Rect…)

#[cfg(not(feature = "std"))]
extern crate alloc;

#[cfg(not(feature = "std"))]
use alloc::{format, vec, vec::Vec};
#[cfg(feature = "std")]
use std::{vec, vec::Vec};

use core::f32::consts::PI;
use vitte_cs2d::{prelude::*, s};

// ================================ Couleur ================================
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Copy, Clone, Debug, PartialEq)]
#[repr(C)]
pub struct Color {
    pub r: f32,
    pub g: f32,
    pub b: f32,
    pub a: f32,
}
impl Color {
    pub const fn rgba(r: f32, g: f32, b: f32, a: f32) -> Self {
        Self { r, g, b, a }
    }
    pub const fn rgb(r: f32, g: f32, b: f32) -> Self {
        Self { r, g, b, a: 1.0 }
    }
    pub const fn white() -> Self {
        Self::rgb(1.0, 1.0, 1.0)
    }
    pub const fn black() -> Self {
        Self::rgb(0.0, 0.0, 0.0)
    }
    pub fn premul(self) -> Self {
        Self { r: self.r * self.a, g: self.g * self.a, b: self.b * self.a, a: self.a }
    }
}

// ================================ Vertex ================================
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Copy, Clone, Debug, PartialEq)]
#[repr(C)]
pub struct Vertex {
    pub pos: Vec2,
    pub uv: Vec2,
    pub color: Color,
}
impl Vertex {
    pub fn new(pos: Vec2, color: Color) -> Self {
        Self { pos, uv: Vec2::zero(), color }
    }
}

// ============================== TriangleBatch ============================
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Clone, Debug, Default)]
pub struct TriangleBatch {
    pub vertices: Vec<Vertex>,
    pub indices: Vec<u32>,
    pub texture: Option<u32>, // handle arbitraire défini par le backend
    pub blend: BlendMode,
}
impl TriangleBatch {
    pub fn append(&mut self, other: &TriangleBatch) {
        let off = self.vertices.len() as u32;
        self.vertices.extend_from_slice(&other.vertices);
        self.indices.extend(other.indices.iter().map(|i| i + off));
    }
}

#[derive(Copy, Clone, Debug, PartialEq, Eq)]
pub enum BlendMode {
    Alpha,
    Add,
    Multiply,
}

impl Default for BlendMode {
    fn default() -> Self {
        BlendMode::Alpha
    }
}

// ================================ Paint =================================
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Copy, Clone, Debug, PartialEq)]
pub struct Paint {
    pub color: Color,
    pub blend: BlendMode,
}
impl Default for Paint {
    fn default() -> Self {
        Self { color: Color::white(), blend: BlendMode::Alpha }
    }
}

#[derive(Copy, Clone, Debug, PartialEq)]
pub enum LineJoin {
    Miter,
    Bevel,
    Round,
}
#[derive(Copy, Clone, Debug, PartialEq)]
pub enum LineCap {
    Butt,
    Square,
    Round,
}

#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Copy, Clone, Debug, PartialEq)]
pub struct StrokeStyle {
    pub width: Scalar,
    pub join: LineJoin,
    pub cap: LineCap,
    pub miter_limit: Scalar,
}
impl Default for StrokeStyle {
    fn default() -> Self {
        Self { width: s(1.0), join: LineJoin::Miter, cap: LineCap::Butt, miter_limit: s(4.0) }
    }
}

// ================================ Path ==================================
#[derive(Clone, Debug, PartialEq)]
pub enum PathCmd {
    MoveTo(Vec2),
    LineTo(Vec2),
    QuadTo(Vec2, Vec2),
    CubicTo(Vec2, Vec2, Vec2),
    Close,
}

#[derive(Clone, Debug, Default, PartialEq)]
pub struct Path {
    pub cmds: Vec<PathCmd>,
    pub closed: bool,
}
impl Path {
    pub fn new() -> Self {
        Self { cmds: Vec::new(), closed: false }
    }
    pub fn move_to(&mut self, p: Vec2) {
        self.cmds.push(PathCmd::MoveTo(p));
    }
    pub fn line_to(&mut self, p: Vec2) {
        self.cmds.push(PathCmd::LineTo(p));
    }
    pub fn quad_to(&mut self, c: Vec2, p: Vec2) {
        self.cmds.push(PathCmd::QuadTo(c, p));
    }
    pub fn cubic_to(&mut self, c1: Vec2, c2: Vec2, p: Vec2) {
        self.cmds.push(PathCmd::CubicTo(c1, c2, p));
    }
    pub fn close(&mut self) {
        self.cmds.push(PathCmd::Close);
        self.closed = true;
    }

    /// Approxime courbes → polylignes (tessellation seuil de platitude)
    pub fn flatten(&self, tol: Scalar) -> Vec<Vec2> {
        let mut pts: Vec<Vec2> = Vec::new();
        let mut pen = Vec2::zero();
        let mut have_pen = false;
        for cmd in &self.cmds {
            match *cmd {
                PathCmd::MoveTo(p) => {
                    pen = p;
                    have_pen = true;
                    pts.push(p);
                },
                PathCmd::LineTo(p) => {
                    if !have_pen {
                        pen = p;
                        pts.push(p);
                    } else {
                        pts.push(p);
                        pen = p;
                    }
                },
                PathCmd::QuadTo(c, p) => {
                    if !have_pen {
                        pen = c;
                        pts.push(c);
                    }
                    flatten_quad(pen, c, p, tol, &mut pts);
                    pen = p;
                },
                PathCmd::CubicTo(c1, c2, p) => {
                    if !have_pen {
                        pen = c1;
                        pts.push(c1);
                    }
                    flatten_cubic(pen, c1, c2, p, tol, &mut pts);
                    pen = p;
                },
                PathCmd::Close => { /* la fermeture est gérée par l'appelant */ },
            }
        }
        pts
    }
}

#[inline]
fn flatten_quad(p0: Vec2, c: Vec2, p1: Vec2, tol: Scalar, out: &mut Vec<Vec2>) {
    // subdivision récursive de De Casteljau jusqu'à platitude
    fn flat(p0: Vec2, c: Vec2, p1: Vec2, t: Scalar) -> bool {
        let ab = c - p0;
        let bc = p1 - c;
        let d = (bc - ab).length();
        d <= t
    }
    fn rec(p0: Vec2, c: Vec2, p1: Vec2, t: Scalar, out: &mut Vec<Vec2>) {
        if flat(p0, c, p1, t) {
            out.push(p1);
            return;
        }
        let p01 = (p0 + c) * 0.5;
        let c1 = (c + p1) * 0.5;
        let m = (p01 + c1) * 0.5; // split
        rec(p0, p01, m, t, out);
        rec(m, c1, p1, t, out);
    }
    rec(p0, c, p1, tol, out);
}

#[inline]
fn flatten_cubic(p0: Vec2, c1: Vec2, c2: Vec2, p1: Vec2, tol: Scalar, out: &mut Vec<Vec2>) {
    fn flat(p0: Vec2, c1: Vec2, c2: Vec2, p1: Vec2, t: Scalar) -> bool {
        let d1 = (c1 - p0).length();
        let d2 = (c2 - p1).length();
        (d1 + d2) <= t
    }
    fn rec(p0: Vec2, c1: Vec2, c2: Vec2, p1: Vec2, t: Scalar, out: &mut Vec<Vec2>) {
        if flat(p0, c1, c2, p1, t) {
            out.push(p1);
            return;
        }
        let p01 = (p0 + c1) * 0.5;
        let p12 = (c1 + c2) * 0.5;
        let p23 = (c2 + p1) * 0.5;
        let q0 = (p01 + p12) * 0.5;
        let q1 = (p12 + p23) * 0.5;
        let m = (q0 + q1) * 0.5;
        rec(p0, p01, q0, m, t, out);
        rec(m, q1, p23, p1, t, out);
    }
    rec(p0, c1, c2, p1, tol, out);
}

// ============================= Tessellation ==============================

#[derive(Clone, Debug)]
pub struct TessOptions {
    pub tol: Scalar,
}
impl Default for TessOptions {
    fn default() -> Self {
        Self { tol: s(0.75) }
    }
}

/// Stroke polyline → triangles (bevel/round/miter selon style). Très basique mais rapide.
pub fn tessellate_stroke(
    poly: &[Vec2],
    style: &StrokeStyle,
    paint: &Paint,
    out: &mut TriangleBatch,
) {
    if poly.len() < 2 {
        return;
    }
    let w = (style.width / 2.0).max(0.0);
    let color = paint.color.premul();

    let mut add_quad = |a: Vec2, b: Vec2, c: Vec2, d: Vec2| {
        let base = out.vertices.len() as u32;
        out.vertices.push(Vertex::new(a, color));
        out.vertices.push(Vertex::new(b, color));
        out.vertices.push(Vertex::new(c, color));
        out.vertices.push(Vertex::new(d, color));
        out.indices
            .extend_from_slice(&[base, base + 1, base + 2, base, base + 2, base + 3]);
    };

    // offset à gauche/droite par normale
    let mut prev = poly[0];
    for i in 1..poly.len() {
        let cur = poly[i];
        let dir = (cur - prev).normalized();
        let n = dir.perp();
        let a = prev + n * w;
        let b = prev - n * w;
        let c = cur - n * w;
        let d = cur + n * w;
        add_quad(a, b, c, d);
        prev = cur;
    }

    // caps rudimentaires
    match style.cap {
        LineCap::Square => {
            let start_dir = (poly[1] - poly[0]).normalized();
            let end_dir = (poly[poly.len() - 1] - poly[poly.len() - 2]).normalized();
            let n0 = start_dir.perp();
            let a = poly[0] - start_dir * w + n0 * w;
            let b = poly[0] - start_dir * w - n0 * w;
            let c = poly[0] + n0 * w;
            let d = poly[0] - n0 * w;
            add_quad(a, b, d, c);
            let n1 = end_dir.perp();
            let e = poly[poly.len() - 1] + end_dir * w + n1 * w;
            let f = poly[poly.len() - 1] + end_dir * w - n1 * w;
            let g = poly[poly.len() - 1] - n1 * w;
            let h = poly[poly.len() - 1] + n1 * w;
            add_quad(g, h, e, f);
        },
        LineCap::Round => { /* TODO: arcs — version minimale omise pour rester léger */ },
        LineCap::Butt => {},
    }
}

/// Ear-clipping très simple pour polygones simples (sans trous). Retourne triangles.
pub fn tessellate_fill(poly: &[Vec2], paint: &Paint, out: &mut TriangleBatch) {
    if poly.len() < 3 {
        return;
    }
    let mut idx: Vec<usize> = (0..poly.len()).collect();
    let color = paint.color.premul();

    let mut area = 0.0;
    for i in 0..poly.len() {
        let p = poly[i];
        let q = poly[(i + 1) % poly.len()];
        area += (q.x - p.x) * (q.y + p.y);
    }
    let ccw = area < 0.0; // algo de signe simpliste

    let is_ear = |ia: usize, ib: usize, ic: usize, poly: &[Vec2], idx: &[usize]| -> bool {
        let a = poly[idx[ia]];
        let b = poly[idx[ib]];
        let c = poly[idx[ic]];
        // convexité
        let cross = (b - a).perp().dot(c - b);
        if ccw && cross <= 0.0 {
            return false;
        }
        if !ccw && cross >= 0.0 {
            return false;
        }
        // aucun autre point dans le triangle
        for &k in idx.iter() {
            if k == idx[ia] || k == idx[ib] || k == idx[ic] {
                continue;
            }
            if point_in_tri(poly[k], a, b, c) {
                return false;
            }
        }
        true
    };

    while idx.len() > 2 {
        let mut ear_found = false;
        for i in 0..idx.len() {
            let ia = (i + idx.len() - 1) % idx.len();
            let ib = i;
            let ic = (i + 1) % idx.len();
            if is_ear(ia, ib, ic, poly, &idx) {
                let base = out.vertices.len() as u32;
                let a = poly[idx[ia]];
                let b = poly[idx[ib]];
                let c = poly[idx[ic]];
                out.vertices.push(Vertex::new(a, color));
                out.vertices.push(Vertex::new(b, color));
                out.vertices.push(Vertex::new(c, color));
                out.indices.extend_from_slice(&[base, base + 1, base + 2]);
                idx.remove(ib);
                ear_found = true;
                break;
            }
        }
        if !ear_found {
            break;
        } // polygon pathologique
    }
}

#[inline]
fn point_in_tri(p: Vec2, a: Vec2, b: Vec2, c: Vec2) -> bool {
    let v0 = c - a;
    let v1 = b - a;
    let v2 = p - a;
    let den = v0.x * v1.y - v1.x * v0.y;
    if den == 0.0 {
        return false;
    }
    let u = (v2.x * v1.y - v1.x * v2.y) / den;
    let v = (v0.x * v2.y - v2.x * v0.y) / den;
    u >= 0.0 && v >= 0.0 && u + v <= 1.0
}

// ============================ Canvas & Backend ===========================

pub trait RenderBackend {
    fn begin(&mut self, viewport: Rect);
    fn draw(&mut self, batch: &TriangleBatch);
    fn end(&mut self);
}

#[derive(Clone, Debug)]
pub struct State {
    pub transform: Mat3,
    pub paint: Paint,
    pub stroke: StrokeStyle,
}
impl Default for State {
    fn default() -> Self {
        Self {
            transform: Mat3::identity(),
            paint: Paint::default(),
            stroke: StrokeStyle::default(),
        }
    }
}

#[derive(Default)]
pub struct Canvas2D {
    pub state: State,
    stack: Vec<State>,
    batch: TriangleBatch,
    tess: TessOptions,
}

impl Canvas2D {
    pub fn new() -> Self {
        Self {
            state: State::default(),
            stack: Vec::new(),
            batch: TriangleBatch::default(),
            tess: TessOptions::default(),
        }
    }

    // — États —
    pub fn set_paint(&mut self, p: Paint) {
        self.state.paint = p;
    }
    pub fn set_stroke_style(&mut self, s: StrokeStyle) {
        self.state.stroke = s;
    }
    pub fn transform(&mut self, m: Mat3) {
        self.state.transform = self.state.transform.mul(m);
    }
    pub fn save(&mut self) {
        self.stack.push(self.state.clone());
    }
    pub fn restore(&mut self) {
        if let Some(s) = self.stack.pop() {
            self.state = s;
        }
    }

    // — Primitives —
    pub fn fill_rect(&mut self, r: Rect) {
        let mut poly = vec![r.min(), Vec2::new(r.x + r.w, r.y), r.max(), Vec2::new(r.x, r.y + r.h)];
        for p in &mut poly {
            *p = self.state.transform.transform_point(*p);
        }
        tessellate_fill(&poly, &self.state.paint, &mut self.batch);
    }

    pub fn stroke_rect(&mut self, r: Rect) {
        let mut poly = vec![r.min(), Vec2::new(r.x + r.w, r.y), r.max(), Vec2::new(r.x, r.y + r.h)];
        for p in &mut poly {
            *p = self.state.transform.transform_point(*p);
        }
        tessellate_stroke(&poly, &self.state.stroke, &self.state.paint, &mut self.batch);
    }

    pub fn circle(&mut self, c: Vec2, radius: Scalar, fill: bool) {
        let seg = ((radius as f32 * 0.75).max(8.0)) as i32; // segmentation adaptative
        let mut poly: Vec<Vec2> = Vec::new();
        for i in 0..seg {
            let t = i as f32 / seg as f32;
            let ang = t * 2.0 * PI;
            poly.push(Vec2::new(
                c.x + radius * ang.cos() as Scalar,
                c.y + radius * ang.sin() as Scalar,
            ));
        }
        for p in &mut poly {
            *p = self.state.transform.transform_point(*p);
        }
        if fill {
            tessellate_fill(&poly, &self.state.paint, &mut self.batch);
        } else {
            tessellate_stroke(&poly, &self.state.stroke, &self.state.paint, &mut self.batch);
        }
    }

    pub fn stroke_polyline(&mut self, pts: &[Vec2]) {
        if pts.len() < 2 {
            return;
        }
        let poly: Vec<Vec2> =
            pts.iter().map(|p| self.state.transform.transform_point(*p)).collect();
        tessellate_stroke(&poly, &self.state.stroke, &self.state.paint, &mut self.batch);
    }

    pub fn fill_path(&mut self, path: &Path) {
        let mut poly = path.flatten(self.tess.tol);
        if poly.len() < 3 {
            return;
        }
        for p in &mut poly {
            *p = self.state.transform.transform_point(*p);
        }
        tessellate_fill(&poly, &self.state.paint, &mut self.batch);
    }

    pub fn stroke_path(&mut self, path: &Path) {
        let mut poly = path.flatten(self.tess.tol);
        if poly.len() < 2 {
            return;
        }
        for p in &mut poly {
            *p = self.state.transform.transform_point(*p);
        }
        tessellate_stroke(&poly, &self.state.stroke, &self.state.paint, &mut self.batch);
    }

    // — Soumission —
    pub fn take_batch(&mut self) -> TriangleBatch {
        core::mem::take(&mut self.batch)
    }

    pub fn render<B: RenderBackend>(&mut self, backend: &mut B, viewport: Rect) {
        backend.begin(viewport);
        backend.draw(&self.batch);
        backend.end();
        self.batch.vertices.clear();
        self.batch.indices.clear();
    }
}

// ================================ Tests =================================
#[cfg(test)]
mod tests {
    use super::*;

    struct Dummy;
    impl RenderBackend for Dummy {
        fn begin(&mut self, _: Rect) {}
        fn draw(&mut self, _: &TriangleBatch) {}
        fn end(&mut self) {}
    }

    #[test]
    fn stroke_rect_generates_vertices() {
        let mut c = Canvas2D::new();
        c.set_paint(Paint { color: Color::rgb(1.0, 0.0, 0.0), blend: BlendMode::Alpha });
        c.set_stroke_style(StrokeStyle { width: s(2.0), ..Default::default() });
        c.stroke_rect(Rect::new(0.0, 0.0, 10.0, 10.0));
        assert!(c.batch.vertices.len() >= 8);
        let mut b = Dummy;
        c.render(&mut b, Rect::new(0.0, 0.0, 100.0, 100.0));
    }

    #[test]
    fn fill_circle_ok() {
        let mut c = Canvas2D::new();
        c.set_paint(Paint::default());
        c.circle(Vec2::new(0.0, 0.0), s(10.0), true);
        assert!(c.batch.indices.len() >= 3);
    }
}
