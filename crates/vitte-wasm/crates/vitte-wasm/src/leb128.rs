//! leb128 — encodage LEB128 utilitaire

pub fn write_u32_leb(mut n: u32, out: &mut Vec<u8>) {
    loop {
        let mut b = (n & 0x7f) as u8;
        n >>= 7;
        if n != 0 { b |= 0x80; }
        out.push(b);
        if n == 0 { break; }
    }
}
