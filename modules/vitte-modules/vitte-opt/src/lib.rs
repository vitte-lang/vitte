#![cfg_attr(not(feature = "std"), no_std)]
//! vitte-opt — **Passes & analyses** pour le compilateur/IR Vitte
//!
//! Objectifs :
//! - Un **framework de passes** générique (pipeline, stats, invalidations)
//! - Un **socle d'analyses** prêtes à l'emploi : *dominators*, *liveness*, *dataflow* générique
//! - Des **passes classiques** branchables sur une API IR minimale: *DCE*, *ConstProp*, *SimplifyCFG*
//! - `no_std`-friendly (via `alloc`) ; `std` simplement pour tests & confort
//!
//! 👉 Ce crate ne dépend **pas** d'un IR concret. Il expose un trait [`Program`] à implémenter
//! par `vitte-ir` ou tout autre backend. Les passes n'appellent que ces méthodes.
//!
//! # Exemple d'usage
//! ```ignore
//! use vitte_opt as opt;
//! # struct MyIr { /* ... */ }
//! # impl opt::Program for MyIr { /* ... */ }
//! let mut ir = MyIr{ /* ... */ };
//! let mut pm = opt::PassManager::<MyIr>::default();
//! pm.add(opt::passes::SimplifyCfg::default());
//! pm.add(opt::passes::ConstProp::default());
//! pm.add(opt::passes::DeadCodeElim::default());
//! let report = pm.run_module(&mut ir);
//! eprintln!("{}", report.human());
//! ```

#[cfg(not(feature = "std"))]
extern crate alloc;

#[cfg(not(feature = "std"))]
use alloc::{
    collections::{BTreeMap as HashMap, BTreeSet as HashSet},
    string::String,
    vec::Vec,
};
#[cfg(feature = "std")]
use std::{
    collections::{HashMap, HashSet},
    string::String,
    vec::Vec,
};
// ======================================================================
// IR Adaptor
// ======================================================================

/// API minimale requise par les analyses/passes — à **implémenter côté IR**.
pub trait Program {
    type Func: Copy + Eq + core::hash::Hash;
    type Block: Copy + Eq + core::hash::Hash;
    type Inst: Copy + Eq + core::hash::Hash;
    type Value: Copy + Eq + core::hash::Hash;

    // ---- structure ----
    fn functions(&self) -> Vec<Self::Func>;
    fn blocks(&self, f: Self::Func) -> Vec<Self::Block>;
    fn entry_block(&self, f: Self::Func) -> Self::Block;
    fn preds(&self, f: Self::Func, b: Self::Block) -> Vec<Self::Block>;
    fn succs(&self, f: Self::Func, b: Self::Block) -> Vec<Self::Block>;

    // ---- instructions / valeurs ----
    fn block_insts(&self, f: Self::Func, b: Self::Block) -> Vec<Self::Inst>;
    fn inst_uses(&self, f: Self::Func, i: Self::Inst) -> Vec<Self::Value>;
    fn value_users(&self, f: Self::Func, v: Self::Value) -> Vec<Self::Inst>;

    /// Remplace toutes les utilisations de `from` par `to` — retourne le nb de remplacements.
    fn replace_uses(&mut self, f: Self::Func, from: Self::Value, to: Self::Value) -> usize;
    /// Supprime une instruction si possible — retourne vrai en cas de suppression.
    fn remove_inst(&mut self, f: Self::Func, i: Self::Inst) -> bool;

    // ---- propriétés utiles aux passes ----
    /// Vrai si l'instruction a des **effets de bord** (IO, store, call impure…), donc non supprimable même si valeur morte.
    fn has_side_effects(&self, f: Self::Func, i: Self::Inst) -> bool;
    /// Vrai si l'instruction est **trivialement morte** (aucun utilisateur et pas d'effets de bord).
    fn is_trivially_dead(&self, f: Self::Func, i: Self::Inst) -> bool {
        !self.has_side_effects(f, i)
            && self.inst_uses(f, i).is_empty()
            && self.users_count_of_inst_result(f, i) == 0
    }
    /// Compte approximatif d'utilisateurs du résultat de l'instruction.
    fn users_count_of_inst_result(&self, f: Self::Func, i: Self::Inst) -> usize;

    // ---- utilitaires spécifiques à certaines passes (optionnels) ----
    /// Essaie de **plier** des constantes dans un bloc ; retourne le nombre de changements.
    fn constant_fold_block(&mut self, _f: Self::Func, _b: Self::Block) -> usize {
        0
    }
    /// Tente de supprimer/recoller les blocs triviaux (BB à un seul `goto` etc.).
    fn remove_block_if_trivial(&mut self, _f: Self::Func, _b: Self::Block) -> bool {
        false
    }

    // ---- use/def par bloc (pour liveness) ----
    /// Retourne (USE, DEF) **par bloc** — valeurs lues avant définition locale, et valeurs définies localement.
    fn block_use_def(&self, f: Self::Func, b: Self::Block) -> (Vec<Self::Value>, Vec<Self::Value>);
}

// ======================================================================
// Pass framework
// ======================================================================

#[derive(Default)]
pub struct PassManager<P: Program> {
    passes: Vec<Box<dyn Pass<P>>>,
}

impl<P: Program> PassManager<P> {
    pub fn new() -> Self {
        Self { passes: Vec::new() }
    }
    pub fn add<T: Pass<P> + 'static>(&mut self, pass: T) {
        self.passes.push(Box::new(pass));
    }

    pub fn run_module(&mut self, prog: &mut P) -> Report<P> {
        let mut report = Report::default();
        let funs = prog.functions();
        for &f in &funs {
            report.functions += 1;
            let mut ctx = Ctx::default();
            for p in self.passes.iter_mut() {
                let name = p.name().to_string();
                let r = p.run_on_function(prog, f, &mut ctx);
                report.passes.push((name, r));
                report.merge(&r);
                // invalidations
                if r.invalidate_cfg {
                    ctx.invalidate_cfg();
                }
                if r.invalidate_liveness {
                    ctx.invalidate_liveness_of(f);
                }
                if r.invalidate_doms {
                    ctx.invalidate_doms_of(f);
                }
            }
        }
        report
    }
}

/// Contexte partagé entre passes — héberge caches d'analyses.
pub struct Ctx<P: Program> {
    doms: HashMap<P::Func, Dominators<P::Block>>, // recalculé si invalidé
    live: HashMap<P::Func, Liveness<P::Block, P::Value>>, // recalculé si invalidé
}

impl<P: Program> Default for Ctx<P> {
    fn default() -> Self {
        Self { doms: HashMap::default(), live: HashMap::default() }
    }
}

impl<P: Program> Ctx<P> {
    pub fn dominators(&mut self, prog: &P, f: P::Func) -> &Dominators<P::Block> {
        if !self.doms.contains_key(&f) {
            let d = Dominators::compute(prog, f);
            self.doms.insert(f, d);
        }
        self.doms.get(&f).unwrap()
    }
    pub fn liveness(&mut self, prog: &P, f: P::Func) -> &Liveness<P::Block, P::Value> {
        if !self.live.contains_key(&f) {
            let l = Liveness::compute(prog, f);
            self.live.insert(f, l);
        }
        self.live.get(&f).unwrap()
    }

    pub fn invalidate_cfg(&mut self) {
        self.doms.clear(); /* liveness potentiellement obsolète */
        self.live.clear();
    }
    pub fn invalidate_doms_of(&mut self, f: P::Func) {
        self.doms.remove(&f);
    }
    pub fn invalidate_liveness_of(&mut self, f: P::Func) {
        self.live.remove(&f);
    }
}

/// Résultat d'une passe (mutations + invalidations).
#[derive(Clone, Copy, Default)]
pub struct PassResult {
    pub changes: usize,
    pub invalidate_cfg: bool,
    pub invalidate_doms: bool,
    pub invalidate_liveness: bool,
}
impl PassResult {
    pub fn changed(mut self) -> Self {
        self.changes += 1;
        self
    }
    pub fn invalidate_all(mut self) -> Self {
        self.invalidate_cfg = true;
        self.invalidate_doms = true;
        self.invalidate_liveness = true;
        self
    }
}

pub trait Pass<P: Program> {
    fn name(&self) -> &str;
    fn run_on_function(&mut self, prog: &mut P, f: P::Func, ctx: &mut Ctx<P>) -> PassResult;
}

/// Rapport d'exécution d'un pipeline.
#[derive(Clone)]
pub struct Report<P: Program> {
    pub functions: usize,
    pub passes: Vec<(String, PassResult)>,
    _phantom: core::marker::PhantomData<P>,
}
impl<P: Program> Default for Report<P> {
    fn default() -> Self {
        Self { functions: 0, passes: Vec::new(), _phantom: core::marker::PhantomData }
    }
}
impl<P: Program> Report<P> {
    pub fn merge(&mut self, r: &PassResult) {
        // l'appelant pousse le (name, r). Ici on ne fait que stats de top-niveau.
        let _ = r;
    }
    pub fn human(&self) -> String {
        let mut s = String::new();
        s.push_str(&format!("functions: {}\n", self.functions));
        for (name, r) in &self.passes {
            s.push_str(&format!("- {}: {} changes\n", name, r.changes));
        }
        s
    }
}

// ======================================================================
// Analyses
// ======================================================================

/// Dominators simples via itération de points fixes (rapide, fiable).
#[derive(Clone)]
pub struct Dominators<B: Copy + Eq + core::hash::Hash> {
    pub idom: HashMap<B, B>,
}

impl<B: Copy + Eq + core::hash::Hash> core::fmt::Debug for Dominators<B> {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        f.debug_struct("Dominators").field("idom", &self.idom.len()).finish()
    }
}

impl<B: Copy + Eq + core::hash::Hash> Dominators<B> {
    pub fn compute<P>(prog: &P, f: P::Func) -> Self
    where
        P: Program<Block = B>,
    {
        let blocks = prog.blocks(f);
        let n = blocks.len();
        let mut index: HashMap<B, usize> = HashMap::default();
        for (i, &b) in blocks.iter().enumerate() {
            index.insert(b, i);
        }
        let mut dom = vec![BitSet::new(n); n];
        let entry = prog.entry_block(f);
        let entry_i = *index.get(&entry).unwrap();
        for i in 0..n {
            dom[i].fill_all(n);
        }
        dom[entry_i].clear_all();
        dom[entry_i].insert(entry_i);

        // itération : dom[b] = {b} ∪ ⋂_{p∈preds[b]} dom[p]
        let mut changed = true;
        while changed {
            changed = false;
            for &b in &blocks {
                if b == entry {
                    continue;
                }
                let bi = index[&b];
                let preds = prog.preds(f, b);
                if preds.is_empty() {
                    continue;
                }
                let mut inter = BitSet::new(n);
                inter.fill_all(n);
                for p in preds {
                    inter &= dom[index[&p]].clone();
                }
                let mut new = inter;
                new.insert(bi);
                if new != dom[bi] {
                    dom[bi] = new;
                    changed = true;
                }
            }
        }

        // idom : idom(b) = unique d ∈ dom(b)\{b} qui ne domine pas strictement d'autres candidats
        let mut idom: HashMap<B, B> = HashMap::default();
        for &b in &blocks {
            if b == entry {
                continue;
            }
            let bi = index[&b];
            // candidats: tout d dans dom(b) sauf b
            let mut best: Option<B> = None;
            for d_i in dom[bi].iter_ones() {
                if d_i == bi {
                    continue;
                }
                let d = blocks[d_i];
                // d est idom si: pour tout autre c != b,d dans dom(b), d ∉ dom(c) (i.e., c ne domine pas b plus strictement que d)
                let mut is_idom = true;
                for c_i in dom[bi].iter_ones() {
                    if c_i == bi || c_i == d_i {
                        continue;
                    }
                    // si d ∈ dom[c], alors d domine c, donc pas idom
                    if dom[c_i].contains(d_i) {
                        is_idom = false;
                        break;
                    }
                }
                if is_idom {
                    best = Some(d);
                    break;
                }
            }
            if let Some(d) = best {
                idom.insert(b, d);
            }
        }
        Self { idom }
    }

    /// Vrai si `a` domine `b` (réflexif).
    pub fn dominates(&self, a: B, b: B) -> bool {
        if a == b {
            return true;
        }
        let mut cur = b;
        while let Some(&id) = self.idom.get(&cur) {
            if id == a {
                return true;
            }
            if id == cur {
                break;
            }
            cur = id;
        }
        false
    }
}

/// Liveness classique (live-in / live-out par bloc) via dataflow backward *union*.
#[derive(Clone)]
pub struct Liveness<B, V>
where
    B: Copy + Eq + core::hash::Hash,
    V: Copy + Eq + core::hash::Hash,
{
    pub live_in: HashMap<B, BitSet>,
    pub live_out: HashMap<B, BitSet>,
    pub value_index: HashMap<V, usize>,
    pub index_value: Vec<V>,
}

impl<B, V> Liveness<B, V>
where
    B: Copy + Eq + core::hash::Hash,
    V: Copy + Eq + core::hash::Hash,
{
    pub fn compute<P>(prog: &P, f: P::Func) -> Self
    where
        P: Program<Block = B, Value = V>,
    {
        let blocks = prog.blocks(f);
        // indexation des valeurs : union des USE/DEF de tous les blocs
        let mut values: HashSet<V> = HashSet::default();
        for &b in &blocks {
            let (u, d) = prog.block_use_def(f, b);
            for v in u {
                values.insert(v);
            }
            for v in d {
                values.insert(v);
            }
        }
        let index_value: Vec<V> = values.iter().copied().collect();
        let mut value_index: HashMap<V, usize> = HashMap::default();
        for (i, v) in index_value.iter().enumerate() {
            value_index.insert(*v, i);
        }
        let nvals = value_index.len();

        let mut live_in: HashMap<B, BitSet> = HashMap::default();
        let mut live_out: HashMap<B, BitSet> = HashMap::default();
        for &b in &blocks {
            live_in.insert(b, BitSet::new(nvals));
            live_out.insert(b, BitSet::new(nvals));
        }

        // Dataflow backward: in[b] = use[b] ∪ (out[b] - def[b]); out[b] = ⋃ in[s]
        let mut changed = true;
        while changed {
            changed = false;
            for &b in &blocks {
                // out[b]
                let mut outb = BitSet::new(nvals);
                for s in prog.succs(f, b) {
                    outb |= live_in[&s].clone();
                }

                // in[b]
                let (use_b, def_b) = prog.block_use_def(f, b);
                let mut inb = outb.clone();
                // inb = outb - def
                for v in def_b {
                    if let Some(&i) = value_index.get(&v) {
                        inb.remove(i);
                    }
                }
                // inb |= use
                for v in use_b {
                    if let Some(&i) = value_index.get(&v) {
                        inb.insert(i);
                    }
                }

                if inb != live_in[&b] || outb != live_out[&b] {
                    live_in.insert(b, inb);
                    live_out.insert(b, outb);
                    changed = true;
                }
            }
        }

        Self { live_in, live_out, value_index, index_value }
    }
}

// ======================================================================
// BitSet utilitaire (compact, sans allocs superflues)
// ======================================================================

#[derive(Clone, PartialEq, Eq)]
pub struct BitSet {
    words: Vec<u64>,
    len: usize,
}
impl core::fmt::Debug for BitSet {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        write!(f, "BitSet({} bits)", self.len)
    }
}
impl BitSet {
    pub fn new(bits: usize) -> Self {
        let n = (bits + 63) / 64;
        Self { words: vec![0; n], len: bits }
    }
    pub fn fill_all(&mut self, bits: usize) {
        *self = Self::new(bits);
        for w in &mut self.words {
            *w = !0;
        }
        self.trim_tail();
    }
    pub fn clear_all(&mut self) {
        for w in &mut self.words {
            *w = 0;
        }
    }
    pub fn insert(&mut self, i: usize) {
        let (w, b) = (i / 64, i % 64);
        if w < self.words.len() {
            self.words[w] |= 1u64 << b;
        }
    }
    pub fn remove(&mut self, i: usize) {
        let (w, b) = (i / 64, i % 64);
        if w < self.words.len() {
            self.words[w] &= !(1u64 << b);
        }
    }
    pub fn contains(&self, i: usize) -> bool {
        let (w, b) = (i / 64, i % 64);
        self.words.get(w).map(|x| (x >> b) & 1 == 1).unwrap_or(false)
    }
    pub fn iter_ones(&self) -> impl Iterator<Item = usize> + '_ {
        self.words.iter().enumerate().flat_map(|(wi, &w)| BitIter { w, base: wi * 64 })
    }
    fn trim_tail(&mut self) {
        let extra = self.words.len() * 64 - self.len;
        if extra > 0 {
            let mask = !0u64 >> extra;
            if let Some(last) = self.words.last_mut() {
                *last &= mask;
            }
        }
    }
}

struct BitIter {
    w: u64,
    base: usize,
}
impl Iterator for BitIter {
    type Item = usize;
    fn next(&mut self) -> Option<Self::Item> {
        if self.w == 0 {
            return None;
        }
        let t = self.w & (!self.w + 1);
        let i = self.base + t.trailing_zeros() as usize;
        self.w &= self.w - 1;
        Some(i)
    }
}

impl core::ops::BitAndAssign for BitSet {
    fn bitand_assign(&mut self, rhs: Self) {
        for (a, b) in self.words.iter_mut().zip(rhs.words.iter()) {
            *a &= *b;
        }
    }
}
impl core::ops::BitOrAssign for BitSet {
    fn bitor_assign(&mut self, rhs: Self) {
        if rhs.words.len() > self.words.len() {
            self.words.resize(rhs.words.len(), 0);
        }
        for (i, b) in rhs.words.iter().enumerate() {
            if i < self.words.len() {
                self.words[i] |= *b;
            }
        }
    }
}
impl core::ops::BitAnd for BitSet {
    type Output = Self;
    fn bitand(self, rhs: Self) -> Self {
        let mut s = self.clone();
        s &= rhs;
        s
    }
}
impl core::ops::BitOr for BitSet {
    type Output = Self;
    fn bitor(self, rhs: Self) -> Self {
        let mut s = self.clone();
        s |= rhs;
        s
    }
}

// ======================================================================
// Passes
// ======================================================================

pub mod passes {
    use super::*;

    // -------------------------- SimplifyCFG --------------------------
    #[derive(Default, Clone)]
    pub struct SimplifyCfg;
    impl<P: Program> Pass<P> for SimplifyCfg {
        fn name(&self) -> &str {
            "SimplifyCFG"
        }
        fn run_on_function(&mut self, prog: &mut P, f: P::Func, _ctx: &mut Ctx<P>) -> PassResult {
            let mut res = PassResult::default();
            let blocks = prog.blocks(f);
            for &b in &blocks {
                if prog.remove_block_if_trivial(f, b) {
                    res = res.changed();
                    res.invalidate_cfg = true;
                    res.invalidate_doms = true;
                }
            }
            res
        }
    }

    // -------------------------- ConstProp ---------------------------
    #[derive(Default, Clone)]
    pub struct ConstProp;
    impl<P: Program> Pass<P> for ConstProp {
        fn name(&self) -> &str {
            "ConstProp"
        }
        fn run_on_function(&mut self, prog: &mut P, f: P::Func, _ctx: &mut Ctx<P>) -> PassResult {
            let mut res = PassResult::default();
            for &b in &prog.blocks(f) {
                let n = prog.constant_fold_block(f, b);
                if n > 0 {
                    res.changes += n;
                    res.invalidate_liveness = true;
                }
            }
            res
        }
    }

    // -------------------------- DeadCodeElim ------------------------
    #[derive(Default, Clone)]
    pub struct DeadCodeElim;
    impl<P: Program> Pass<P> for DeadCodeElim {
        fn name(&self) -> &str {
            "DCE"
        }
        fn run_on_function(&mut self, prog: &mut P, f: P::Func, _ctx: &mut Ctx<P>) -> PassResult {
            let mut res = PassResult::default();
            // petite passe gloutonne: supprime ce qui est trivialement mort
            for &b in &prog.blocks(f) {
                for i in prog.block_insts(f, b) {
                    if prog.is_trivially_dead(f, i) {
                        if prog.remove_inst(f, i) {
                            res = res.changed();
                            res.invalidate_liveness = true;
                        }
                    }
                }
            }
            res
        }
    }
}

// ======================================================================
// Tests (avec un IR factice très minimal)
// ======================================================================

#[cfg(test)]
mod tests {
    use super::*;

    // Un micro IR de test: graphe de blocs seulement (pas de vraies instructions)
    #[derive(Clone, Copy, PartialEq, Eq, Hash, Debug)]
    struct F(u32);
    #[derive(Clone, Copy, PartialEq, Eq, Hash, Debug)]
    struct B(u32);
    #[derive(Clone, Copy, PartialEq, Eq, Hash, Debug)]
    struct I(u32);
    #[derive(Clone, Copy, PartialEq, Eq, Hash, Debug)]
    struct V(u32);

    #[derive(Default)]
    struct Mock {
        funs: Vec<F>,
        blocks: HashMap<F, Vec<B>>,
        entry: HashMap<F, B>,
        preds: HashMap<(F, B), Vec<B>>,
        succs: HashMap<(F, B), Vec<B>>,
        insts: HashMap<(F, B), Vec<I>>,
        uses: HashMap<(F, I), Vec<V>>,
        users: HashMap<(F, V), Vec<I>>,
        sidefx: HashMap<(F, I), bool>,
        use_def: HashMap<(F, B), (Vec<V>, Vec<V>)>,
    }

    impl Program for Mock {
        type Func = F;
        type Block = B;
        type Inst = I;
        type Value = V;
        fn functions(&self) -> Vec<F> {
            self.funs.clone()
        }
        fn blocks(&self, f: F) -> Vec<B> {
            self.blocks.get(&f).cloned().unwrap_or_default()
        }
        fn entry_block(&self, f: F) -> B {
            self.entry[&f]
        }
        fn preds(&self, f: F, b: B) -> Vec<B> {
            self.preds.get(&(f, b)).cloned().unwrap_or_default()
        }
        fn succs(&self, f: F, b: B) -> Vec<B> {
            self.succs.get(&(f, b)).cloned().unwrap_or_default()
        }
        fn block_insts(&self, f: F, b: B) -> Vec<I> {
            self.insts.get(&(f, b)).cloned().unwrap_or_default()
        }
        fn inst_uses(&self, f: F, i: I) -> Vec<V> {
            self.uses.get(&(f, i)).cloned().unwrap_or_default()
        }
        fn value_users(&self, f: F, v: V) -> Vec<I> {
            self.users.get(&(f, v)).cloned().unwrap_or_default()
        }
        fn replace_uses(&mut self, _: F, _from: V, _to: V) -> usize {
            0
        }
        fn remove_inst(&mut self, _: F, _: I) -> bool {
            true
        }
        fn has_side_effects(&self, f: F, i: I) -> bool {
            *self.sidefx.get(&(f, i)).unwrap_or(&false)
        }
        fn users_count_of_inst_result(&self, _: F, _: I) -> usize {
            0
        }
        fn block_use_def(&self, f: F, b: B) -> (Vec<V>, Vec<V>) {
            self.use_def.get(&(f, b)).cloned().unwrap_or_default()
        }
    }

    #[test]
    fn doms_basic() {
        // f: entry -> b1 -> b2 ; entry -> b2 (diamant sans branche join)
        let f = F(0);
        let entry = B(0);
        let b1 = B(1);
        let b2 = B(2);
        let mut m = Mock::default();
        m.funs = vec![f];
        m.entry.insert(f, entry);
        m.blocks.insert(f, vec![entry, b1, b2]);
        m.succs.insert((f, entry), vec![b1, b2]);
        m.preds.insert((f, b1), vec![entry]);
        m.preds.insert((f, b2), vec![entry, b1]);
        m.succs.insert((f, b1), vec![b2]);
        m.succs.insert((f, b2), vec![]);
        let d = Dominators::compute(&m, f);
        assert!(d.dominates(entry, b1));
        assert!(d.dominates(entry, b2));
        assert!(d.dominates(b1, b2));
    }

    #[test]
    fn liveness_basic() {
        let f = F(0);
        let b0 = B(0);
        let b1 = B(1);
        let v0 = V(0);
        let v1 = V(1);
        let mut m = Mock::default();
        m.funs = vec![f];
        m.entry.insert(f, b0);
        m.blocks.insert(f, vec![b0, b1]);
        m.succs.insert((f, b0), vec![b1]);
        m.preds.insert((f, b1), vec![b0]);
        m.use_def.insert((f, b0), (vec![v0], vec![v1])); // use v0, def v1
        m.use_def.insert((f, b1), (vec![v1], vec![])); // b1 use v1
        let l = Liveness::compute(&m, f);
        // v1 vivant en sortie de b0
        let i_v1 = l.value_index[&v1];
        assert!(l.live_out[&b0].contains(i_v1));
    }

    #[test]
    fn pm_runs() {
        let f = F(0);
        let b0 = B(0);
        let mut m = Mock::default();
        m.funs = vec![f];
        m.entry.insert(f, b0);
        m.blocks.insert(f, vec![b0]);
        let mut pm = PassManager::<Mock>::default();
        pm.add(passes::SimplifyCfg::default());
        pm.add(passes::ConstProp::default());
        pm.add(passes::DeadCodeElim::default());
        let _r = pm.run_module(&mut m);
    }
}
