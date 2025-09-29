//! vitte-backend-llvm — JIT minimal basé sur LLVM (inkwell).
//!
//! API de démonstration sans IR complexe :
//! - `compile_add_i64`  : (i64, i64) -> i64
//! - `compile_const_i64`: () -> i64
//!
//! Exemple :
//! ```ignore
//! use vitte_backend_llvm::Jit;
//! let mut jit = Jit::new("demo");
//! let addr = jit.compile_add_i64("add").unwrap();
//! let add: extern "C" fn(i64, i64) -> i64 = unsafe { core::mem::transmute(addr) };
//! assert_eq!(add(20, 22), 42);
//! ```

#![deny(missing_docs)]
#![forbid(unsafe_code)]
#![cfg_attr(not(feature = "std"), no_std)]

#[cfg(not(feature = "std"))]
extern crate alloc;

#[cfg(feature = "std")]
use std::{string::String, vec::Vec};

#[cfg(not(feature = "std"))]
use alloc::{string::String, vec::Vec};

use inkwell::context::Context;
use inkwell::execution_engine::ExecutionEngine;
use inkwell::module::Module;
use inkwell::OptimizationLevel;
use vitte_errors::VitteError;

/// JIT LLVM minimal (inkwell).
pub struct Jit<'ctx> {
    ctx: &'ctx Context,
    module: Module<'ctx>,
    ee: ExecutionEngine<'ctx>,
}

impl<'ctx> Jit<'ctx> {
    /// Crée un JIT nommé.
    pub fn new(name: &str) -> Self {
        let ctx = Box::leak(Box::new(Context::create()));
        let module = ctx.create_module(name);
        let ee = module
            .create_jit_execution_engine(OptimizationLevel::Aggressive)
            .expect("impossible de créer l'execution engine");
        Self { ctx, module, ee }
    }

    /// Compile une fonction `(i64, i64) -> i64` nommée `fn_name` qui fait une addition.
    ///
    /// Renvoie l'adresse machine de la fonction compilée.
    pub fn compile_add_i64(&mut self, fn_name: &str) -> Result<usize, VitteError> {
        let i64t = self.ctx.i64_type();

        // type (i64, i64) -> i64
        let fn_ty = i64t.fn_type(&[i64t.into(), i64t.into()], false);
        let func = self.module.add_function(fn_name, fn_ty, None);

        // bloc d'entrée
        let entry = self.ctx.append_basic_block(func, "entry");
        let builder = self.ctx.create_builder();
        builder.position_at_end(entry);

        let a = func.get_first_param().unwrap().into_int_value();
        let b = func.get_nth_param(1).unwrap().into_int_value();
        let sum = builder.build_int_add(a, b, "sum").map_err(map_build_err)?;
        builder.build_return(Some(&sum)).map_err(map_build_err)?;

        // Optionnel: verification interne LLVM
        if func.verify(true) {
            // ok
        }

        // optimisation simple
        self.module
            .create_function_pass_manager()
            .run_on(&func);

        // adresse
        let addr = self
            .ee
            .get_function_address(fn_name)
            .map_err(|e| VitteError::Parse(format!("get_function_address: {e}")))?;

        Ok(addr as usize)
    }

    /// Compile une fonction `() -> i64` qui retourne `imm`.
    pub fn compile_const_i64(&mut self, fn_name: &str, imm: i64) -> Result<usize, VitteError> {
        let i64t = self.ctx.i64_type();

        let fn_ty = i64t.fn_type(&[], false);
        let func = self.module.add_function(fn_name, fn_ty, None);

        let entry = self.ctx.append_basic_block(func, "entry");
        let builder = self.ctx.create_builder();
        builder.position_at_end(entry);

        let c = i64t.const_int(imm as u64, true);
        builder.build_return(Some(&c)).map_err(map_build_err)?;

        if func.verify(true) {
            // ok
        }

        self.module
            .create_function_pass_manager()
            .run_on(&func);

        let addr = self
            .ee
            .get_function_address(fn_name)
            .map_err(|e| VitteError::Parse(format!("get_function_address: {e}")))?;

        Ok(addr as usize)
    }

    /// Exporte le module LLVM en texte IR pour debug.
    pub fn llvm_ir(&self) -> String {
        self.module.print_to_string().to_string()
    }
}

/* ─────────────────────────── Utils ─────────────────────────── */

fn map_build_err(e: inkwell::support::LLVMString) -> VitteError {
    VitteError::Parse(e.to_string())
}

/* ─────────────────────────── Tests ─────────────────────────── */

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn add_works() {
        let mut jit = Jit::new("m");
        let addr = jit.compile_add_i64("add").unwrap();
        // transmutation de l'adresse en pointeur de fonction typé.
        let add: extern "C" fn(i64, i64) -> i64 = unsafe { core::mem::transmute(addr) };
        assert_eq!(add(40, 2), 42);
        assert_eq!(add(-1, 1), 0);
    }

    #[test]
    fn const_works() {
        let mut jit = Jit::new("m2");
        let addr = jit.compile_const_i64("k", 123).unwrap();
        let k: extern "C" fn() -> i64 = unsafe { core::mem::transmute(addr) };
        assert_eq!(k(), 123);
    }
}
