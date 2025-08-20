// Remplis ces fonctions si tu n'importes pas les crates réelles.
pub fn tokenize(s: &str) -> usize { s.len() } // fake fallback
pub fn parse(s: &str) -> usize { s.as_bytes().iter().filter(|c| **c == b';').count() }
pub fn vm_exec_sum(n: u64) -> u64 { (0..n).sum() }
pub fn bytecode_add_loop(n: u32) -> u64 { (0..n).map(|i| i as u64).sum() }
pub fn map_fill(n: usize) -> usize {
    let mut m = std::collections::HashMap::with_capacity(n);
    for i in 0..n { m.insert(i, i*i); }
    m.len()
}
pub fn channel_pingpong(n: usize) -> usize {
    use std::sync::mpsc::channel;
    let (tx, rx) = channel();
    std::thread::spawn(move || { for i in 0..n { tx.send(i).ok(); } });
    let mut last = 0; for _ in 0..n { last = rx.recv().unwrap(); } last
}
