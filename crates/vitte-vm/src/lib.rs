//! vitte-vm — VM stack-based (stub)
pub struct Vm;
impl Vm { pub fn new() -> Self { Self } pub fn run_bytecode(&mut self, _bytes: &[u8]) -> i32 { 0 } }
