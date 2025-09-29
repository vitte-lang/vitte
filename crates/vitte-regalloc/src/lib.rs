//! vitte-regalloc — allocation de registres pour Vitte
//!
//! Stratégies disponibles :
//! - Linear Scan (`feature = "linear"`) — rapide, bon pour JITs.
//! - Graph Coloring (`feature = "graph"`) — qualité supérieure, plus coûteux.
//!
//! API principale :
//! - [`RegAllocStrategy`] : sélection d’algo
//! - [`RegAllocResult`]   : résultat (mapping + spills)
//! - [`allocate_module`]  : applique la stratégie à tout un module IR
//!
//! Hypothèses IR minimales (compat léger avec `vitte-ir`) :
//! - Un module contient des fonctions, des blocs et des instructions ordonnées.
//! - Chaque instruction peut définir une destination `dest: String` optionnelle
//!   et référencer des opérandes `operands: Vec<String>` (identifiants de vregs).
//! - On considère tout identifiant non numérique comme un registre virtuel.
//!
//! Remarque : on renvoie un mapping (virt → phys ou spill slot). La réécriture
//! des instructions est laissée au backend codegen.

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(
    clippy::module_name_repetitions,
    clippy::doc_markdown,
    clippy::too_many_lines
)]

use anyhow::{bail, Result};
use std::collections::{BTreeMap, BTreeSet, HashMap, HashSet};
use vitte_ir::{Function, Module};

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

/// Registre physique abstrait.
pub type PhysReg = String;

/// Slot de spill abstrait (offset pile).
pub type SpillSlot = usize;

/// Identifiant de registre virtuel.
pub type VirtReg = String;

/// Stratégie d’allocation.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum RegAllocStrategy {
    #[cfg(feature = "linear")]
    LinearScan,
    #[cfg(feature = "graph")]
    GraphColoring,
}

/// Issue d’allocation pour un vreg.
#[derive(Debug, Clone, PartialEq, Eq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum Assign {
    Reg(PhysReg),
    Spill(SpillSlot),
}

/// Résultat d’allocation pour une fonction.
#[derive(Debug, Clone, Default, PartialEq, Eq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct RegAllocResult {
    /// virt → (reg | spill)
    pub mapping: HashMap<VirtReg, Assign>,
    /// ordre des phys regs offerts
    pub phys: Vec<PhysReg>,
    /// nombre de spills effectués
    pub spills: usize,
}

impl RegAllocResult {
    pub fn assigned_reg(&self, v: &str) -> Option<&PhysReg> {
        match self.mapping.get(v) {
            Some(Assign::Reg(r)) => Some(r),
            _ => None,
        }
    }
    pub fn is_spilled(&self, v: &str) -> bool {
        matches!(self.mapping.get(v), Some(Assign::Spill(_)))
    }
}

/// Alloue les registres pour tout le module. Renvoye un résultat par fonction.
pub fn allocate_module(m: &Module, strat: RegAllocStrategy, phys: &[&str]) -> Result<Vec<RegAllocResult>> {
    if phys.is_empty() {
        bail!("aucun registre physique fourni");
    }
    let phys_vec = phys.iter().map(|s| s.to_string()).collect::<Vec<_>>();
    m.functions
        .iter()
        .map(|f| allocate_function(f, strat, &phys_vec))
        .collect()
}

/// Alloue les registres pour une fonction.
pub fn allocate_function(f: &Function, strat: RegAllocStrategy, phys: &[PhysReg]) -> Result<RegAllocResult> {
    match strat {
        #[cfg(feature = "linear")]
        RegAllocStrategy::LinearScan => linear_scan(f, phys),
        #[cfg(feature = "graph")]
        RegAllocStrategy::GraphColoring => graph_coloring(f, phys),
        #[allow(unreachable_patterns)]
        _ => bail!("stratégie non compilée via features"),
    }
}

/* =========================================================================================
   Collecte des vregs et intervalles de vie
   ===================================================================================== */

/// Représente un intervalle [start, end] inclus pour un vreg.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
struct LiveRange {
    start: usize,
    end: usize,
}

/// Collecte toutes les déf/uses dans l’ordre d’exécution séquentiel
/// et calcule un intervalle de vie approximatif par vreg.
fn liveness_intervals(f: &Function) -> (BTreeMap<VirtReg, LiveRange>, Vec<VirtReg>) {
    let mut pos = 0usize;
    let mut first: HashMap<VirtReg, usize> = HashMap::new();
    let mut last: HashMap<VirtReg, usize> = HashMap::new();
    let mut order: Vec<VirtReg> = Vec::new();

    for bb in &f.blocks {
        for inst in &bb.instrs {
            // operands are uses
            for op in inst.operands() {
                if !is_imm(op) {
                    first.entry(op.clone()).or_insert(pos);
                    last.insert(op.clone(), pos);
                    order.push(op.clone());
                }
            }
            // dest is def
            if let Some(d) = inst.dest() {
                if !is_imm(d) {
                    first.entry(d.clone()).or_insert(pos);
                    last.insert(d.clone(), pos);
                    order.push(d.clone());
                }
            }
            pos += 1;
        }
    }

    let mut map = BTreeMap::new();
    for (v, s) in first {
        let e = last.get(&v).copied().unwrap_or(s);
        map.insert(v, LiveRange { start: s, end: e });
    }
    (map, order)
}

fn is_imm(s: &str) -> bool {
    // imm si entier signé simple
    s.parse::<i64>().is_ok()
}

/* =========================================================================================
   Linear Scan
   ===================================================================================== */

#[cfg(feature = "linear")]
fn linear_scan(f: &Function, phys: &[PhysReg]) -> Result<RegAllocResult> {
    let (intervals, _) = liveness_intervals(f);
    let mut active: Vec<(VirtReg, LiveRange, PhysReg)> = Vec::new();
    let mut res = RegAllocResult { mapping: HashMap::new(), phys: phys.to_vec(), spills: 0 };
    let mut free: Vec<PhysReg> = phys.to_vec();
    let mut stack_slots: usize = 0;

    // trier par début
    let mut ints: Vec<(VirtReg, LiveRange)> = intervals.into_iter().collect();
    ints.sort_by_key(|(_, lr)| lr.start);

    for (v, cur) in ints {
        // expirer
        active.sort_by_key(|(_, lr, _)| lr.end);
        let mut i = 0;
        while i < active.len() {
            if active[i].1.end < cur.start {
                // libérer le phys
                free.push(active[i].2.clone());
                active.remove(i);
            } else {
                i += 1;
            }
        }

        if let Some(r) = free.pop() {
            // assigne un registre physique
            res.mapping.insert(v.clone(), Assign::Reg(r.clone()));
            active.push((v, cur, r));
        } else {
            // spill : choisir l’intervalle avec end le plus grand
            if let Some((idx, _)) = active
                .iter()
                .enumerate()
                .max_by_key(|(_, (_, lr, _))| lr.end)
            {
                let (victim_v, victim_lr, victim_reg) = active[idx].clone();
                if victim_lr.end > cur.end {
                    // spill du victim, réutiliser son registre
                    res.mapping.insert(victim_v.clone(), Assign::Spill(stack_slots));
                    res.spills += 1;
                    stack_slots += 1;
                    // remplacer par v
                    active[idx] = (v.clone(), cur, victim_reg.clone());
                    res.mapping.insert(v, Assign::Reg(victim_reg));
                } else {
                    // spill le courant
                    res.mapping.insert(v, Assign::Spill(stack_slots));
                    res.spills += 1;
                    stack_slots += 1;
                }
            } else {
                // aucun actif, mais free vide (cas bizarre) → spill
                res.mapping.insert(v, Assign::Spill(stack_slots));
                res.spills += 1;
                stack_slots += 1;
            }
        }
    }

    Ok(res)
}

/* =========================================================================================
   Graph Coloring
   ===================================================================================== */

#[cfg(feature = "graph")]
fn graph_coloring(f: &Function, phys: &[PhysReg]) -> Result<RegAllocResult> {
    let (intervals, order) = liveness_intervals(f);
    let mut g = InterferenceGraph::new();

    // ajouter noeuds
    for v in intervals.keys() {
        g.add_node(v.clone());
    }

    // Arêtes si chevauchement d'intervalles
    let keys: Vec<_> = intervals.keys().cloned().collect();
    for i in 0..keys.len() {
        for j in i + 1..keys.len() {
            let a = &intervals[&keys[i]];
            let b = &intervals[&keys[j]];
            if overlap(*a, *b) {
                g.add_edge(keys[i].clone(), keys[j].clone());
            }
        }
    }

    // simplification + coloration gloutonne
    let k = phys.len();
    let stack = g.simplify_stack(k);
    let mut colors: HashMap<VirtReg, Option<usize>> = HashMap::new();

    // assigner couleurs
    for v in stack.into_iter().rev() {
        let mut used = BTreeSet::new();
        for n in g.neighbors(&v) {
            if let Some(Some(c)) = colors.get(n) {
                used.insert(*c);
            }
        }
        // première couleur libre
        let mut c = None;
        for i in 0..k {
            if !used.contains(&i) {
                c = Some(i);
                break;
            }
        }
        colors.insert(v, c);
    }

    // Spills pour les None
    let mut res = RegAllocResult { mapping: HashMap::new(), phys: phys.to_vec(), spills: 0 };
    let mut spill_slot = 0usize;
    // itérer dans l’ordre d’apparition pour stabilité
    let uniq = unique_keep_order(order.into_iter().filter(|s| !is_imm(s)));
    for v in uniq {
        match colors.get(&v).and_then(|o| *o) {
            Some(i) => {
                res.mapping.insert(v, Assign::Reg(phys[i].clone()));
            }
            None => {
                res.mapping.insert(v, Assign::Spill(spill_slot));
                spill_slot += 1;
                res.spills += 1;
            }
        }
    }

    Ok(res)
}

#[cfg(feature = "graph")]
fn overlap(a: LiveRange, b: LiveRange) -> bool {
    !(a.end < b.start || b.end < a.start)
}

#[cfg(feature = "graph")]
#[derive(Debug, Default, Clone)]
struct InterferenceGraph {
    adj: HashMap<VirtReg, HashSet<VirtReg>>,
}
#[cfg(feature = "graph")]
impl InterferenceGraph {
    fn new() -> Self { Self { adj: HashMap::new() } }
    fn add_node(&mut self, v: VirtReg) {
        self.adj.entry(v).or_default();
    }
    fn add_edge(&mut self, a: VirtReg, b: VirtReg) {
        if a == b { return; }
        self.adj.entry(a.clone()).or_default().insert(b.clone());
        self.adj.entry(b).or_default().insert(a);
    }
    fn degree(&self, v: &VirtReg) -> usize {
        self.adj.get(v).map(|s| s.len()).unwrap_or(0)
    }
    fn neighbors(&self, v: &VirtReg) -> impl Iterator<Item = &VirtReg> {
        self.adj.get(v).into_iter().flatten()
    }
    /// K-simplification (retire noeuds de degré < K). Retourne la pile.
    fn simplify_stack(&self, k: usize) -> Vec<VirtReg> {
        let mut g = self.adj.clone();
        let mut stack = Vec::<VirtReg>::new();
        let mut removed = HashSet::<VirtReg>::new();

        loop {
            // chercher noeud avec deg < K
            let mut picked: Option<VirtReg> = None;
            for (v, ns) in g.iter() {
                if removed.contains(v) { continue; }
                if ns.iter().filter(|n| !removed.contains(*n)).count() < k {
                    picked = Some(v.clone());
                    break;
                }
            }
            if let Some(v) = picked {
                removed.insert(v.clone());
                stack.push(v.clone());
            } else {
                // si tous retirés, fini; sinon push un spill candidat
                let remain: Vec<_> = g.keys().filter(|v| !removed.contains(*v)).cloned().collect();
                if remain.is_empty() {
                    break;
                }
                // heuristique: max degré
                let v = remain
                    .into_iter()
                    .max_by_key(|x| g.get(x).map(|s| s.len()).unwrap_or(0))
                    .unwrap();
                removed.insert(v.clone());
                stack.push(v);
            }
            if removed.len() == g.len() {
                break;
            }
        }
        stack
    }
}

/* =========================================================================================
   Utilitaires
   ===================================================================================== */

fn unique_keep_order<I: IntoIterator<Item = T>, T: std::cmp::Eq + std::hash::Hash + Clone>(it: I) -> Vec<T> {
    let mut seen = HashSet::new();
    let mut out = Vec::new();
    for x in it {
        if seen.insert(x.clone()) {
            out.push(x);
        }
    }
    out
}

/* =========================================================================================
   Extensions minimales sur vitte_ir::Instr pour accès générique
   ===================================================================================== */

trait InstrView {
    fn dest(&self) -> Option<&String>;
    fn operands(&self) -> Vec<String>;
}

impl InstrView for vitte_ir::Instr {
    fn dest(&self) -> Option<&String> {
        match self {
            vitte_ir::Instr::Assign { dest, .. } => Some(dest),
            vitte_ir::Instr::Bin { dest, .. } => Some(dest),
            _ => None,
        }
    }
    fn operands(&self) -> Vec<String> {
        match self {
            vitte_ir::Instr::Assign { operands, .. } => operands.clone(),
            vitte_ir::Instr::Bin { lhs, rhs, .. } => vec![lhs.clone(), rhs.clone()],
            _ => Vec::new(),
        }
    }
}

/* =========================================================================================
   Tests
   ===================================================================================== */

#[cfg(test)]
mod tests {
    use super::*;
    use vitte_ir::{Instr, ModuleBuilder};

    fn small_module() -> Module {
        // f:
        //   x = 1
        //   y = 2
        //   z = x + y
        //   w = z + y
        let mut b = ModuleBuilder::new("t");
        b.start_function("f");
        b.add_instr(Instr::assign("x", vec!["1".into()]));
        b.add_instr(Instr::assign("y", vec!["2".into()]));
        b.add_instr(Instr::Bin { op: "+".into(), lhs: "x".into(), rhs: "y".into(), dest: "z".into() });
        b.add_instr(Instr::Bin { op: "+".into(), lhs: "z".into(), rhs: "y".into(), dest: "w".into() });
        b.end_function();
        b.finish()
    }

    #[cfg(feature = "linear")]
    #[test]
    fn linear_scan_allocates() {
        let m = small_module();
        let res = allocate_function(&m.functions[0], RegAllocStrategy::LinearScan, &["r0".into(), "r1".into()]).unwrap();
        assert!(res.mapping.contains_key("x"));
        assert!(res.mapping.contains_key("y"));
        assert!(res.mapping.contains_key("z"));
        assert!(res.mapping.contains_key("w"));
        // au plus 2 registres, possibles spills mais mapping complet
    }

    #[cfg(feature = "graph")]
    #[test]
    fn graph_coloring_allocates() {
        let m = small_module();
        let res = allocate_function(&m.functions[0], RegAllocStrategy::GraphColoring, &["r0".into(), "r1".into(), "r2".into()]).unwrap();
        assert!(res.mapping.values().any(|a| matches!(a, Assign::Reg(_))));
    }

    #[test]
    fn module_api_multiple_functions() {
        let mut b = ModuleBuilder::new("m");
        b.start_function("a");
        b.add_instr(Instr::assign("t0", vec!["0".into()]));
        b.end_function();
        let mut b = b;
        b.start_function("b");
        b.add_instr(Instr::assign("u0", vec!["1".into()]));
        b.end_function();
        let m = b.finish();

        // si aucune feature d'algo active, la stratégie échouera ; on teste seulement la garde des phys non vides
        // Pour environnements avec au moins un algo :
        #[cfg(feature = "linear")]
        {
            let res = allocate_module(&m, RegAllocStrategy::LinearScan, &["r0", "r1"]).unwrap();
            assert_eq!(res.len(), 2);
        }
    }
}