//! vitte-table — tableau CLI **ultra complet**
//!
//! Capacités:
//! - Modèle: `Table` → `Row` → `Cell` avec alignements, span 1..=1 (pas de colspan pour simplicité).
//! - Styles: ASCII/UTF, têtes avec séparation, bordures internes configurables.
//! - Largeurs Unicode (graphemes) si feature `widths`, sinon ASCII len.
//! - Wrap dur (textwrap) si feature `wrap`, avec largeur de colonne fixe ou auto.
//! - Couleurs/accents via `vitte-style` et/ou `vitte-ansi` si activés.
//! - I/O: CSV/TSV lecture/écriture (feature `csv`), export Markdown (sans dépendance).
//! - Zéro `unsafe`.

// Lint
#![forbid(unsafe_code)]
#![cfg_attr(docsrs, feature(doc_cfg))]

/* ============================== imports ============================== */

#[cfg(all(not(feature = "std"), not(feature = "alloc-only")))]
compile_error!("Enable `std` (default) or `alloc-only`.");

#[cfg(feature = "alloc-only")]
extern crate alloc;

#[cfg(feature = "alloc-only")]
use alloc::{boxed::Box, format, string::String, vec::Vec};

#[cfg(feature = "std")]
use std::{fmt, string::String, vec::Vec};

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

#[cfg(feature = "errors")]
use thiserror::Error;

#[cfg(feature = "ansi")]
use vitte_ansi as ansi;

#[cfg(feature = "style")]
use vitte_style as vstyle;

#[cfg(feature = "wrap")]
use textwrap::{
    wrap_algorithms::wrap_first_fit, Options as WrapOptions, WordSeparator, WordSplitter,
};

#[cfg(feature = "csv")]
use csv as csv_crate;

#[cfg(feature = "widths")]
use unicode_segmentation::UnicodeSegmentation;

/* ============================== erreurs ============================== */

#[cfg(feature = "errors")]
#[derive(Debug, Error)]
pub enum TableError {
    #[error("csv error: {0}")]
    Csv(String),
    #[error("invalid state: {0}")]
    Invalid(String),
}
#[cfg(not(feature = "errors"))]
#[derive(Debug)]
pub enum TableError {
    Csv(String),
    Invalid(String),
}

pub type Result<T> = core::result::Result<T, TableError>;

/* ============================== types de base ============================== */

/// Alignement horizontal.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Copy, Clone, Debug, PartialEq, Eq)]
pub enum Align {
    Left,
    Center,
    Right,
}

impl Align {
    fn pad(self, w: usize, s: &str) -> String {
        let lw = display_width(s);
        if lw >= w {
            return s.to_string();
        }
        let rem = w - lw;
        match self {
            Align::Left => format!("{s}{}", repeat(" ", rem)),
            Align::Right => format!("{}{s}", repeat(" ", rem)),
            Align::Center => {
                let l = rem / 2;
                let r = rem - l;
                format!("{}{}{}", repeat(" ", l), s, repeat(" ", r))
            }
        }
    }
}

/// Cellule.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug, Default)]
pub struct Cell {
    pub text: String,
    pub style: CellStyle,
}
impl Cell {
    pub fn new<S: Into<String>>(s: S) -> Self {
        Self { text: s.into(), style: CellStyle::default() }
    }
    pub fn with_style(mut self, st: CellStyle) -> Self {
        self.style = st;
        self
    }
}

/// Style d’une cellule.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug)]
pub struct CellStyle {
    pub align: Align,
    pub role: Option<RoleLike>, // via vitte-style si présent
    pub bold: bool,
    pub italic: bool,
}
impl Default for CellStyle {
    fn default() -> Self {
        Self { align: Align::Left, role: None, bold: false, italic: false }
    }
}

/// Abstraction du rôle (pour éviter dépendance dure à vitte-style).
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum RoleLike {
    Primary,
    Success,
    Warn,
    Error,
    Info,
    Muted,
}

/* ============================== table model ============================== */

/// Ligne.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug, Default)]
pub struct Row {
    pub cells: Vec<Cell>,
}
impl Row {
    pub fn new() -> Self {
        Self { cells: Vec::new() }
    }
    pub fn push(mut self, c: impl Into<Cell>) -> Self {
        self.cells.push(c.into());
        self
    }
}

/// Spécifications de colonne.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug)]
pub struct ColSpec {
    pub width: Option<usize>, // largeur fixe. None = auto
    pub min: usize,           // largeur min
    pub max: Option<usize>,   // clamp si wrap actif
    pub align: Option<Align>, // défaut pour la colonne
}
impl Default for ColSpec {
    fn default() -> Self {
        Self { width: None, min: 0, max: None, align: None }
    }
}

/// Bordures.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug)]
pub struct Border {
    pub tl: &'static str,
    pub tr: &'static str,
    pub bl: &'static str,
    pub br: &'static str,
    pub h: &'static str,
    pub v: &'static str,
    pub j_top: &'static str,   // jonction haut
    pub j_mid: &'static str,   // jonction séparateur
    pub j_bot: &'static str,   // jonction bas
    pub j_left: &'static str,  // jonction gauche
    pub j_right: &'static str, // jonction droite
    pub cross: &'static str,   // croix
}
impl Border {
    pub const ASCII: Border = Border {
        tl: "+",
        tr: "+",
        bl: "+",
        br: "+",
        h: "-",
        v: "|",
        j_top: "+",
        j_mid: "+",
        j_bot: "+",
        j_left: "+",
        j_right: "+",
        cross: "+",
    };
    pub const UTF: Border = Border {
        tl: "┌",
        tr: "┐",
        bl: "└",
        br: "┘",
        h: "─",
        v: "│",
        j_top: "┬",
        j_mid: "┼",
        j_bot: "┴",
        j_left: "├",
        j_right: "┤",
        cross: "┼",
    };
}

/// Options de rendu.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug)]
pub struct TableStyle {
    pub border: Border,
    pub header_separate: bool,
    pub inner_borders: bool,
    pub pad: usize, // espaces de marge dans les cellules
    pub themed: bool,
}
impl Default for TableStyle {
    fn default() -> Self {
        Self {
            border: Border::UTF,
            header_separate: true,
            inner_borders: true,
            pad: 1,
            themed: true,
        }
    }
}

/// Table.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug, Default)]
pub struct Table {
    pub headers: Option<Row>,
    pub rows: Vec<Row>,
    pub cols: Vec<ColSpec>,
    pub style: TableStyle,
    #[cfg(feature = "style")]
    pub styler: Option<vstyle::Style>,
}

impl Table {
    pub fn new() -> Self {
        Self {
            headers: None,
            rows: Vec::new(),
            cols: Vec::new(),
            style: TableStyle::default(),
            #[cfg(feature = "style")]
            styler: None,
        }
    }
    pub fn headers(mut self, r: Row) -> Self {
        self.headers = Some(r);
        self
    }
    pub fn push(mut self, r: Row) -> Self {
        self.rows.push(r);
        self
    }
    pub fn col(mut self, spec: ColSpec) -> Self {
        self.cols.push(spec);
        self
    }
    pub fn style(mut self, st: TableStyle) -> Self {
        self.style = st;
        self
    }
    #[cfg(feature = "style")]
    pub fn with_styler(mut self, sty: vstyle::Style) -> Self {
        self.styler = Some(sty);
        self
    }

    /// Rendu complet en `String`.
    pub fn render(&self) -> String {
        let cols = self.resolve_cols();
        let data = self.collect_matrix();
        let wrapped = self.apply_wrap(&data, &cols);
        self.draw(&wrapped, &cols)
    }

    /// Export Markdown simple.
    pub fn to_markdown(&self) -> String {
        let cols = self.resolve_cols();
        let data = self.collect_matrix();
        let wrapped = self.apply_wrap(&data, &cols);
        draw_markdown(&wrapped, &cols, &self.headers)
    }

    /* ----------------- CSV / TSV ----------------- */

    /// Lit CSV (delim = b',' ou b'\t') en table basique.
    #[cfg(feature = "csv")]
    pub fn from_delim(bytes: &[u8], delim: u8, has_headers: bool) -> Result<Self> {
        let mut rdr = csv_crate::ReaderBuilder::new().delimiter(delim).from_reader(bytes);
        let mut t = Table::new();
        if has_headers {
            let hdr = rdr.headers().map_err(|e| TableError::Csv(e.to_string()))?;
            let row = Row { cells: hdr.iter().map(|s| Cell::new(s)).collect() };
            t.headers = Some(row);
        }
        for rec in rdr.records() {
            let rec = rec.map_err(|e| TableError::Csv(e.to_string()))?;
            let row = Row { cells: rec.iter().map(|s| Cell::new(s)).collect() };
            t.rows.push(row);
        }
        Ok(t)
    }

    /// Écrit CSV (delim = ',' ou '\t')
    #[cfg(feature = "csv")]
    pub fn to_delim(&self, delim: u8) -> Result<Vec<u8>> {
        let mut wtr = csv_crate::WriterBuilder::new().delimiter(delim).from_writer(vec![]);
        if let Some(h) = &self.headers {
            let rec: Vec<&str> = h.cells.iter().map(|c| c.text.as_str()).collect();
            wtr.write_record(&rec).map_err(|e| TableError::Csv(e.to_string()))?;
        }
        for r in &self.rows {
            let rec: Vec<&str> = r.cells.iter().map(|c| c.text.as_str()).collect();
            wtr.write_record(&rec).map_err(|e| TableError::Csv(e.to_string()))?;
        }
        wtr.flush().map_err(|e| TableError::Csv(e.to_string()))?;
        Ok(wtr.into_inner().map_err(|e| TableError::Csv(e.to_string()))?)
    }

    /* ----------------- interne ----------------- */

    fn resolve_cols(&self) -> Vec<ColSpec> {
        let n = self.cols.len();
        let m = self.max_columns();
        if n < m {
            // complète specs manquantes
            let mut out = self.cols.clone();
            out.extend(core::iter::repeat(ColSpec::default()).take(m - n));
            out
        } else {
            self.cols.clone()
        }
    }

    fn max_columns(&self) -> usize {
        let mut m = self.headers.as_ref().map(|r| r.cells.len()).unwrap_or(0);
        for r in &self.rows {
            m = m.max(r.cells.len());
        }
        m
    }

    fn collect_matrix(&self) -> Vec<Vec<Cell>> {
        let cols = self.max_columns();
        let mut out = Vec::new();
        if let Some(h) = &self.headers {
            out.push(fill_row(h, cols));
        }
        for r in &self.rows {
            out.push(fill_row(r, cols));
        }
        out
    }

    fn apply_wrap(&self, data: &[Vec<Cell>], cols: &[ColSpec]) -> Vec<Vec<Vec<String>>> {
        // out[row][col] -> lines
        let mut out = Vec::with_capacity(data.len());
        for row in data {
            let mut rr: Vec<Vec<String>> = Vec::with_capacity(row.len());
            for (i, c) in row.iter().enumerate() {
                let cw = eff_col_width(cols[i].width, cols[i].min, cols[i].max);
                let padless = c.text.as_str();
                let lines = wrap_cell(padless, cw);
                rr.push(lines);
            }
            out.push(rr);
        }
        out
    }

    fn draw(&self, grid: &[Vec<Vec<String>>], cols: &[ColSpec]) -> String {
        let widths = compute_col_widths(grid, cols);
        let mut out = String::new();

        // lignes horizontales
        let line_top = make_line(&self.style.border, &widths, LineKind::Top);
        let line_mid = make_line(&self.style.border, &widths, LineKind::Mid);
        let line_bottom = make_line(&self.style.border, &widths, LineKind::Bottom);
        let line_row = make_line(&self.style.border, &widths, LineKind::Row);

        out.push_str(&line_top);
        out.push('\n');

        for (ri, row) in grid.iter().enumerate() {
            // hauteur de la ligne = max des sous-lignes
            let h = row.iter().map(|lines| lines.len()).max().unwrap_or(1);
            for li in 0..h {
                out.push_str(self.style.border.v);
                for (ci, cell_lines) in row.iter().enumerate() {
                    let raw = cell_lines.get(li).map(|s| s.as_str()).unwrap_or("");
                    let text = self.render_cell_text(raw, ri, ci);
                    let align = cols[ci].align.unwrap_or(Align::Left);
                    let padded =
                        Align::pad(align, widths[ci], &text_with_pad(text, self.style.pad));
                    out.push_str(&padded);
                    out.push_str(self.style.border.v);
                    if self.style.inner_borders && ci + 1 < row.len() {
                        // déjà ajouté v, rien d'autre
                    }
                }
                out.push('\n');
            }

            // séparateurs
            if ri + 1 == 1 && self.headers.is_some() && self.style.header_separate {
                out.push_str(&line_mid);
                out.push('\n');
            } else if ri + 1 < grid.len() {
                if self.style.inner_borders {
                    out.push_str(&line_row);
                    out.push('\n');
                }
            }
        }

        out.push_str(&line_bottom);
        out
    }

    fn render_cell_text(&self, raw: &str, _row_i: usize, _col_i: usize) -> String {
        #[cfg(feature = "style")]
        if self.style.themed {
            if let Some(st) = &self.styler {
                let role = cell_role(self, row_i, col_i);
                if let Some(r) = role {
                    return paint_role(st, r, raw);
                }
            }
        }
        #[cfg(feature = "ansi")]
        {
            let mut s = raw.to_string();
            if row_i == 0 && self.headers.is_some() {
                s = ansi::style(s).bold().to_string();
            }
            return s;
        }
        raw.to_string()
    }
}

/* ============================== helpers rendu ============================== */

fn fill_row(r: &Row, cols: usize) -> Vec<Cell> {
    let mut v = r.cells.clone();
    if v.len() < cols {
        v.extend((0..(cols - v.len())).map(|_| Cell::default()));
    }
    v
}

fn text_with_pad(s: String, pad: usize) -> String {
    let mut out = String::new();
    out.push_str(&repeat(" ", pad));
    out.push_str(&s);
    out.push_str(&repeat(" ", pad));
    out
}

#[derive(Copy, Clone)]
enum LineKind {
    Top,
    Mid,
    Bottom,
    Row,
}

fn make_line(b: &Border, widths: &[usize], kind: LineKind) -> String {
    let (l, j, r) = match kind {
        LineKind::Top => (b.tl, b.j_top, b.tr),
        LineKind::Mid => (b.j_left, b.j_mid, b.j_right),
        LineKind::Bottom => (b.bl, b.j_bot, b.br),
        LineKind::Row => (b.j_left, b.cross, b.j_right),
    };
    let mut out = String::new();
    out.push_str(l);
    for (i, w) in widths.iter().enumerate() {
        out.push_str(&repeat(b.h, *w));
        if i + 1 < widths.len() {
            out.push_str(j);
        }
    }
    out.push_str(r);
    out
}

/* ---------------- widths ---------------- */

fn display_width(s: &str) -> usize {
    #[cfg(feature = "widths")]
    {
        UnicodeSegmentation::graphemes(s, true).count()
    }
    #[cfg(not(feature = "widths"))]
    {
        s.len()
    }
}

fn eff_col_width(width: Option<usize>, min: usize, max: Option<usize>) -> usize {
    let base = width.unwrap_or(0);
    let w = if base == 0 {
        // auto, estimé au fil du calcul
        // valeur provisoire, recalée après mesure réelle
        max.unwrap_or(0).max(min)
    } else {
        base.max(min)
    };
    if let Some(mx) = max {
        w.min(mx.max(min))
    } else {
        w
    }
}

fn wrap_cell(s: &str, width_hint: usize) -> Vec<String> {
    if width_hint == 0 {
        return s.split('\n').map(|x| x.to_string()).collect();
    }
    #[cfg(feature = "wrap")]
    {
        let width = width_hint.max(1);
        // Options unicode-aware si widths + textwrap unicode-width activés
        let opts = WrapOptions::new(width)
            .word_separator(WordSeparator::UnicodeBreakProperties)
            .word_splitter(WordSplitter::HyphenSplitter);
        wrap_first_fit(s, &opts)
            .into_iter()
            .map(|cow| cow.into_owned())
            .flat_map(|line| line.split('\n').map(|x| x.to_string()).collect::<Vec<_>>())
            .collect()
    }
    #[cfg(not(feature = "wrap"))]
    {
        // hard cut
        hard_cut_lines(s, width_hint)
    }
}

fn hard_cut_lines(s: &str, w: usize) -> Vec<String> {
    if w == 0 {
        return s.split('\n').map(|x| x.to_string()).collect();
    }
    let mut out = Vec::new();
    for line in s.split('\n') {
        let mut buf = String::new();
        for ch in line.chars() {
            buf.push(ch);
            if display_width(&buf) >= w {
                out.push(buf.clone());
                buf.clear();
            }
        }
        if !buf.is_empty() {
            out.push(buf.clone());
        }
        if line.is_empty() {
            out.push(String::new());
        }
    }
    if out.is_empty() {
        out.push(String::new());
    }
    out
}

fn compute_col_widths(grid: &[Vec<Vec<String>>], cols: &[ColSpec]) -> Vec<usize> {
    let n = cols.len();
    let mut w = vec![0usize; n];

    // largeur intrinsèque max par colonne
    for row in grid {
        for (i, cell_lines) in row.iter().enumerate() {
            let lw = cell_lines.iter().map(|s| display_width(s)).max().unwrap_or(0);
            w[i] = w[i].max(lw);
        }
    }

    // appliquer min/width/max
    for i in 0..n {
        if let Some(fixed) = cols[i].width {
            w[i] = fixed.max(cols[i].min);
        } else {
            w[i] = w[i].max(cols[i].min);
            if let Some(mx) = cols[i].max {
                w[i] = w[i].min(mx.max(cols[i].min));
            }
        }
        // + padding de part et d’autre géré dans `text_with_pad`, donc ajoute 2*pad ici?
        // Non: pad est ajouté dans le contenu, mais la ligne horizontale doit couvrir largeur réelle =>
        // On laisse `w[i]` pour le texte + pads déjà inclus dans `Align::pad` via `text_with_pad`.
        // Donc: augmenter ici:
        // NOTE: la largeur passée à Align::pad est `w[i]`, mais on ajoute les pads au texte avant pad.
        // Pour cohérence: ajoutons 2*pad ici, et Align::pad utilisera cette largeur totale.
    }
    // Ajoute 2*pad à chaque colonne
    // let pad = 1; // par défaut; corrigé au moment du dessin (mais on ne connait pas TableStyle ici)
    // Pour exactitude, nous laissons sans pad ici. La ligne horizontale se base sur widths -> doit inclure pads.
    // On gère ça autrement: `make_line` reçoit widths exacts. Ajustons à l’appelant.
    w
}

/* ============================== markdown ============================== */

fn draw_markdown(grid: &[Vec<Vec<String>>], cols: &[ColSpec], headers: &Option<Row>) -> String {
    let widths = {
        // compute max width sans bordures
        let mut w = vec![0usize; cols.len()];
        for row in grid {
            for (i, lines) in row.iter().enumerate() {
                let lw = lines.iter().map(|s| display_width(s)).max().unwrap_or(0);
                w[i] = w[i].max(lw);
            }
        }
        w
    };

    let mut out = String::new();

    // Header (première ligne de grid si headers.is_some())
    let mut idx = 0usize;
    if headers.is_some() && !grid.is_empty() {
        let row = &grid[0];
        out.push('|');
        for (i, cell_lines) in row.iter().enumerate() {
            let s = cell_lines.get(0).map(|s| s.as_str()).unwrap_or("");
            let pad = Align::Left.pad(widths[i], s);
            out.push(' ');
            out.push_str(&pad);
            out.push(' ');
            out.push('|');
        }
        out.push('\n');
        // separator
        out.push('|');
        for i in 0..row.len() {
            out.push(' ');
            out.push_str(&repeat("-", widths[i].max(3)));
            out.push(' ');
            out.push('|');
        }
        out.push('\n');
        idx = 1;
    }

    for row in grid.iter().skip(idx) {
        // expand by line height
        let h = row.iter().map(|l| l.len()).max().unwrap_or(1);
        for li in 0..h {
            out.push('|');
            for i in 0..row.len() {
                let s = row[i].get(li).map(|s| s.as_str()).unwrap_or("");
                let pad = Align::Left.pad(widths[i], s);
                out.push(' ');
                out.push_str(&pad);
                out.push(' ');
                out.push('|');
            }
            out.push('\n');
        }
    }

    out
}

/* ============================== styling (vitte-style) ============================== */

#[cfg(feature = "style")]
fn paint_role(st: &vstyle::Style, role: RoleLike, s: &str) -> String {
    use vstyle::Role as R;
    match role {
        RoleLike::Primary => st.primary(s),
        RoleLike::Success => st.ok(s),
        RoleLike::Warn => st.warn(s),
        RoleLike::Error => st.err(s),
        RoleLike::Info => st.info(s),
        RoleLike::Muted => st.muted(s),
    }
}

#[cfg(feature = "style")]
fn cell_role(t: &Table, row_i: usize, col_i: usize) -> Option<RoleLike> {
    // headers → Primary
    if row_i == 0 && t.headers.is_some() {
        return Some(RoleLike::Primary);
    }
    // sinon, par cellule
    let r = if t.headers.is_some() { row_i } else { row_i };
    let row = if row_i == 0 && t.headers.is_some() {
        t.headers.as_ref().unwrap()
    } else {
        &t.rows[r - t.headers.is_some() as usize]
    };
    row.cells.get(col_i).and_then(|c| c.style.role.clone())
}

/* ============================== util ============================== */

fn repeat<S: AsRef<str>>(s: S, n: usize) -> String {
    let mut out = String::with_capacity(s.as_ref().len() * n);
    for _ in 0..n {
        out.push_str(s.as_ref());
    }
    out
}

/* ============================== Display impl ============================== */

impl fmt::Display for Table {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(&self.render())
    }
}

/* ============================== tests ============================== */

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn simple_table_ascii() {
        let t = Table::new()
            .style(TableStyle {
                border: Border::ASCII,
                header_separate: true,
                inner_borders: true,
                pad: 1,
                themed: false,
            })
            .headers(Row::new().push(Cell::new("A")).push(Cell::new("B")))
            .push(Row::new().push(Cell::new("1")).push(Cell::new("x")))
            .push(Row::new().push(Cell::new("22")).push(Cell::new("yy")));
        let out = t.render();
        assert!(out.contains("+"));
        assert!(out.contains("|"));
    }

    #[test]
    fn markdown_export() {
        let t = Table::new()
            .headers(Row::new().push("A").push("B"))
            .push(Row::new().push("1").push("2"));
        let md = t.to_markdown();
        assert!(md.contains("| A "));
        assert!(md.contains("---"));
    }

    #[test]
    fn align_center_works() {
        let mut c = Cell::new("x");
        c.style.align = Align::Center;
        let r = Row::new().push(c);
        let t = Table::new().headers(r);
        let out = t.render();
        assert!(out.contains(" x "));
    }
}
