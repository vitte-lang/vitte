//! vitte-opt — passes d’optimisation pour Vitte IR
//!
//! Contenu :
//! - Simplification de base (const fold, dead code elim)
//! - Inlining simple
//! - Renommage SSA (expérimental)
//! - Optionnel : passes LLVM via `inkwell`
//!
//! API :
//! - [`optimize_module`] pour appliquer toutes les passes par défaut
//! - [`Pass`] (trait), [`PassManager`]

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(clippy::module_name_repetitions, clippy::doc_markdown, clippy::too_many_lines)]

use anyhow::Result;
use vitte_ir::{Instr, Module};

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

/// Trait d’une passe d’optimisation.
pub trait Pass {
    fn name(&self) -> &'static str;
    fn run(&self, m: &mut Module) -> Result<()>;
}

/// Gestionnaire de passes.
pub struct PassManager {
    passes: Vec<Box<dyn Pass + Send + Sync>>,
}

impl PassManager {
    pub fn new() -> Self { Self { passes: Vec::new() } }

    pub fn with(mut self, p: impl Pass + Send + Sync + 'static) -> Self {
        self.passes.push(Box::new(p));
        self
    }

    pub fn run(&self, m: &mut Module) -> Result<()> {
        for p in &self.passes {
            p.run(m)?;
        }
        Ok(())
    }
}

/// Passe : élimination de code mort trivial.
pub struct DeadCodeElim;
impl Pass for DeadCodeElim {
    fn name(&self) -> &'static str { "DeadCodeElim" }
    fn run(&self, m: &mut Module) -> Result<()> {
        for f in &mut m.functions {
            for bb in &mut f.blocks {
                bb.instrs.retain(|i| match i {
                    Instr::Nop => false,
                    _ => true,
                });
            }
        }
        Ok(())
    }
}

/// Passe : constant folding naïf.
pub struct ConstFold;
impl Pass for ConstFold {
    fn name(&self) -> &'static str { "ConstFold" }
    fn run(&self, m: &mut Module) -> Result<()> {
        for f in &mut m.functions {
            for bb in &mut f.blocks {
                for inst in &mut bb.instrs {
                    if let Instr::Bin { op, lhs, rhs, dest } = inst {
                        if let (Ok(a), Ok(b)) = (lhs.parse::<i64>(), rhs.parse::<i64>()) {
                            let val = match op.as_str() {
                                "+" => a + b,
                                "-" => a - b,
                                "*" => a * b,
                                "/" if b != 0 => a / b,
                                _ => continue,
                            };
                            *inst = Instr::assign(dest, vec![val.to_string()]);
                        }
                    }
                }
            }
        }
        Ok(())
    }
}

/// Passe : inlining très simple (stub).
pub struct Inline;
impl Pass for Inline {
    fn name(&self) -> &'static str { "Inline" }
    fn run(&self, _m: &mut Module) -> Result<()> {
        // TODO: impl inlining
        Ok(())
    }
}

/// Applique les passes par défaut.
pub fn optimize_module(m: &mut Module) -> Result<()> {
    PassManager::new()
        .with(ConstFold)
        .with(DeadCodeElim)
        .with(Inline)
        .run(m)
}

// ============================== Tests =====================================

#[cfg(test)]
mod tests {
    use super::*;
    use vitte_ir::ModuleBuilder;

    #[test]
    fn const_fold_adds() {
        let mut b = ModuleBuilder::new("t");
        b.start_function("f");
        b.add_instr(Instr::Bin { op: "+".into(), lhs: "2".into(), rhs: "3".into(), dest: "x".into() });
        b.end_function();
        let mut m = b.finish();
        ConstFold.run(&mut m).unwrap();
        let i = &m.functions[0].blocks[0].instrs[0];
        match i {
            Instr::Assign { dest, operands } => {
                assert_eq!(dest, "x");
                assert_eq!(operands, &["5".to_string()]);
            }
            _ => panic!("not folded"),
        }
    }

    #[test]
    fn dead_code_removed() {
        let mut b = ModuleBuilder::new("t");
        b.start_function("f");
        b.add_instr(Instr::Nop);
        b.end_function();
        let mut m = b.finish();
        DeadCodeElim.run(&mut m).unwrap();
        assert!(m.functions[0].blocks[0].instrs.is_empty());
    }
}