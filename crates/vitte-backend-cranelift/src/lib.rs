//! vitte-backend-cranelift — JIT simple basé sur Cranelift.
//!
//! API minimale pour compiler quelques fonctions jouets, sans dépendre d’un IR complexe.
//! - `compile_add_i64` : (i64, i64) -> i64
//! - `compile_const_i64`: () -> i64
//!
//! Exemple:
//! ```rust
//! use vitte_backend_cranelift::Jit;
//! let mut jit = Jit::new();
//! let f = jit.compile_add_i64().unwrap();
//! let add: extern "C" fn(i64,i64)->i64 = unsafe { core::mem::transmute(f) };
//! assert_eq!(add(20, 22), 42);
//! ```

#![deny(missing_docs)]
#![forbid(unsafe_code)]
#![cfg_attr(not(feature = "std"), no_std)]

#[cfg(not(feature = "std"))]
extern crate alloc;

use core::sync::atomic::{AtomicUsize, Ordering};

#[cfg(feature = "std")]
use std::{string::String, vec::Vec};

#[cfg(not(feature = "std"))]
use alloc::{string::String, vec::Vec};

use cranelift::prelude::*;
use cranelift_jit::{JITBuilder, JITModule};
use cranelift_module::{default_libcall_names, Linkage, Module};
use vitte_errors::VitteError;

/// JIT Cranelift minimal.
pub struct Jit {
    module: JITModule,
    fctx: FunctionBuilderContext,
    name_ctr: AtomicUsize,
}

impl Jit {
    /// Nouveau JIT.
    pub fn new() -> Self {
        let builder = JITBuilder::new(default_libcall_names());
        let module = JITModule::new(builder);
        Self {
            module,
            fctx: FunctionBuilderContext::new(),
            name_ctr: AtomicUsize::new(0),
        }
    }

    /// Compile une fonction `(i64,i64) -> i64` qui effectue une addition.
    pub fn compile_add_i64(&mut self) -> Result<*const u8, VitteError> {
        let mut ctx = self.module.make_context();

        // Signature (i64, i64) -> i64
        {
            let sig = &mut ctx.func.signature;
            sig.params.push(AbiParam::new(types::I64));
            sig.params.push(AbiParam::new(types::I64));
            sig.returns.push(AbiParam::new(types::I64));
        }

        // Corps
        {
            let mut builder = FunctionBuilder::new(&mut ctx.func, &mut self.fctx);
            let block = builder.create_block();
            builder.append_block_params_for_function_params(block);
            builder.switch_to_block(block);
            builder.seal_block(block);

            let a = builder.block_params(block)[0];
            let b = builder.block_params(block)[1];
            let sum = builder.ins().iadd(a, b);
            builder.ins().return_(&[sum]);

            builder.finalize();
        }

        // Déclaration + définition
        let name = self.unique_name("add_i64");
        let id = self
            .module
            .declare_function(&name, Linkage::Export, &ctx.func.signature)
            .map_err(|e| VitteError::Parse(e.to_string()))?;
        self.module
            .define_function(id, &mut ctx)
            .map_err(|e| VitteError::Parse(e.to_string()))?;

        self.module.clear_context(&mut ctx);
        self.module.finalize_definitions().map_err(map_mod_err)?;

        Ok(self.module.get_finalized_function(id))
    }

    /// Compile une fonction `() -> i64` qui retourne `imm`.
    pub fn compile_const_i64(&mut self, imm: i64) -> Result<*const u8, VitteError> {
        let mut ctx = self.module.make_context();

        // Signature () -> i64
        ctx.func.signature.returns.push(AbiParam::new(types::I64));

        // Corps
        {
            let mut builder = FunctionBuilder::new(&mut ctx.func, &mut self.fctx);
            let block = builder.create_block();
            builder.switch_to_block(block);
            builder.seal_block(block);

            let c = builder.ins().iconst(types::I64, imm);
            builder.ins().return_(&[c]);

            builder.finalize();
        }

        // Déclaration + définition
        let name = self.unique_name("const_i64");
        let id = self
            .module
            .declare_function(&name, Linkage::Export, &ctx.func.signature)
            .map_err(|e| VitteError::Parse(e.to_string()))?;
        self.module
            .define_function(id, &mut ctx)
            .map_err(|e| VitteError::Parse(e.to_string()))?;

        self.module.clear_context(&mut ctx);
        self.module.finalize_definitions().map_err(map_mod_err)?;

        Ok(self.module.get_finalized_function(id))
    }

    fn unique_name(&self, base: &str) -> String {
        let n = self.name_ctr.fetch_add(1, Ordering::Relaxed);
        let mut s = String::from(base);
        s.push('_');
        push_u(s, n)
    }
}

/* ─────────────────────────── Utils ─────────────────────────── */

fn map_mod_err(e: cranelift_module::ModuleError) -> VitteError {
    VitteError::Parse(e.to_string())
}

// Petit util sans alloc additionnelle pour concaténer un entier.
fn push_u(mut s: String, mut n: usize) -> String {
    // collect digits reversed
    let mut buf: [u8; 20] = [0; 20];
    let mut i = 0;
    if n == 0 {
        buf[0] = b'0';
        i = 1;
    } else {
        while n > 0 {
            buf[i] = b'0' + (n % 10) as u8;
            n /= 10;
            i += 1;
        }
    }
    // reverse into string
    while i > 0 {
        i -= 1;
        s.push(buf[i] as char);
    }
    s
}

/* ─────────────────────────── Tests ─────────────────────────── */

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn add_works() {
        let mut jit = Jit::new();
        let f = jit.compile_add_i64().unwrap();
        let add: extern "C" fn(i64, i64) -> i64 = unsafe { core::mem::transmute(f) };
        assert_eq!(add(40, 2), 42);
        assert_eq!(add(-1, 1), 0);
    }

    #[test]
    fn const_works() {
        let mut jit = Jit::new();
        let f = jit.compile_const_i64(123).unwrap();
        let k: extern "C" fn() -> i64 = unsafe { core::mem::transmute(f) };
        assert_eq!(k(), 123);
    }
}
