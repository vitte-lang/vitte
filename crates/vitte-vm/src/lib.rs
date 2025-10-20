//! vitte-vm — VM stack-based (minimal interpreter for VITBC bytecode)

use core::fmt;

#[cfg(not(feature = "std"))]
extern crate alloc;

#[cfg(not(feature = "std"))]
use alloc::vec::Vec;
#[cfg(feature = "std")]
use std::vec::Vec;

/// Error type returned by the VM when decoding or executing bytecode fails.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum VmError {
    /// Bytecode header or section layout is invalid.
    InvalidFormat(&'static str),
    /// Reached the end of the byte stream unexpectedly.
    UnexpectedEof,
    /// Tried to pop from an empty stack.
    StackUnderflow,
}

impl fmt::Display for VmError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            VmError::InvalidFormat(msg) => write!(f, "invalid bytecode format: {msg}"),
            VmError::UnexpectedEof => write!(f, "unexpected end of bytecode stream"),
            VmError::StackUnderflow => write!(f, "stack underflow"),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for VmError {}

/// Minimal stack-based interpreter capable of executing the subset of opcodes
/// emitted by the current demo compiler (`Op::ConstI64`, `Op::AddI64`, `Op::Call`, `Op::Ret`).
#[derive(Default)]
pub struct Vm {
    stack: Vec<i64>,
}

impl Vm {
    /// Creates a new VM instance.
    pub fn new() -> Self {
        Self { stack: Vec::with_capacity(32) }
    }

    /// Executes a VITBC bytecode buffer and returns the resulting exit code.
    ///
    /// The interpreter currently understands the minimal `VITBC` container
    /// produced by the reference compiler along with the historical `VBC0`
    /// stub format (used by the CLI before the real compiler was connected).
    pub fn run_bytecode(&mut self, bytes: &[u8]) -> Result<i32, VmError> {
        if bytes.is_empty() {
            return Ok(0);
        }

        // Legacy stub format support (used by CLI fallbacks).
        if bytes.len() >= 4 && &bytes[..4] == b"VBC0" {
            return Ok(0);
        }

        let code = extract_code_section(bytes)?;
        self.stack.clear();

        let mut pc = 0;
        while pc < code.len() {
            match code[pc] {
                0x00 => {
                    // Nop
                    pc += 1;
                }
                0x01 => {
                    // ConstI64 — requires 8 trailing bytes.
                    if pc + 9 > code.len() {
                        return Err(VmError::UnexpectedEof);
                    }
                    let mut buf = [0u8; 8];
                    buf.copy_from_slice(&code[pc + 1..pc + 9]);
                    let value = i64::from_le_bytes(buf);
                    self.stack.push(value);
                    pc += 9;
                }
                0x02 => {
                    // AddI64 — pops two operands and pushes the sum.
                    let rhs = self.stack.pop().ok_or(VmError::StackUnderflow)?;
                    let lhs = self.stack.pop().ok_or(VmError::StackUnderflow)?;
                    self.stack.push(lhs.wrapping_add(rhs));
                    pc += 1;
                }
                0x10 => {
                    // Ret — terminate execution and report top-of-stack.
                    let value = self.stack.pop().unwrap_or(0);
                    return Ok(value as i32);
                }
                0x20 => {
                    // Call — consumes the call target (u32) and simulates a trivial callee.
                    if pc + 5 > code.len() {
                        return Err(VmError::UnexpectedEof);
                    }
                    // Consume the encoded function index even though we don't dispatch yet.
                    pc += 5;
                    let arg = self.stack.pop().unwrap_or(0);
                    self.stack.push(arg);
                }
                _ => return Err(VmError::InvalidFormat("opcode inconnu")),
            }
        }

        // Reached the end without encountering a Ret. Return the last value if available.
        Ok(self.stack.pop().unwrap_or(0) as i32)
    }
}

fn extract_code_section(bytes: &[u8]) -> Result<&[u8], VmError> {
    const MAGIC: &[u8] = b"VITBC\0";
    if bytes.len() < MAGIC.len() + 2 {
        return Err(VmError::InvalidFormat("header trop court"));
    }
    if &bytes[..MAGIC.len()] != MAGIC {
        return Err(VmError::InvalidFormat("signature VITBC manquante"));
    }

    let mut offset = MAGIC.len() + 2; // skip version
    let mut code_section = None;
    while offset + 8 <= bytes.len() {
        let tag = &bytes[offset..offset + 4];
        let len = u32::from_le_bytes([
            bytes[offset + 4],
            bytes[offset + 5],
            bytes[offset + 6],
            bytes[offset + 7],
        ]) as usize;
        offset += 8;
        if offset + len > bytes.len() {
            return Err(VmError::UnexpectedEof);
        }
        if tag == b"CODE" {
            code_section = Some(&bytes[offset..offset + len]);
        }
        offset += len;
    }

    code_section.ok_or(VmError::InvalidFormat("section CODE absente"))
}
