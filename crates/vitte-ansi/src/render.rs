// vitte-ansi/src/render.rs
//! Backends de rendu ANSI : réel (stdout), virtuel (headless), simulation (tests).

use crate::caps::{detect_capabilities, Capability};
use crate::sgr::SgrSeq;

/// Interface commune de rendu.
pub trait Renderer: Send {
    fn write_str(&mut self, s: &str);
    fn sgr(&mut self, seq: SgrSeq);

    #[inline]
    fn text(&mut self, s: &str) { self.write_str(s); }

    #[inline]
    fn styled(&mut self, s: &str, style: crate::sgr::Style) {
        self.sgr(SgrSeq::Set(style));
        self.write_str(s);
        self.sgr(SgrSeq::Reset);
    }
}

/// Terminal réel : écrit vers stdout. Dégrade selon capacités détectées.
pub struct RealTerm {
    support_double: bool,
    support_fonts: bool,
}

impl RealTerm {
    pub fn stdout() -> Self {
        let caps = detect_capabilities();
        Self {
            support_double: caps.has(Capability::DoubleWidth),
            support_fonts: caps.has(Capability::FontSelect),
        }
    }
}

impl Renderer for RealTerm {
    fn write_str(&mut self, s: &str) {
        use std::io::Write;
        let _ = std::io::stdout().write_all(s.as_bytes());
    }

    fn sgr(&mut self, seq: SgrSeq) {
        let esc = match seq {
            SgrSeq::Set(_) | SgrSeq::Reset => seq.to_ansi(self.support_double, self.support_fonts),
            _ => seq.to_ansi(false, false),
        };
        self.write_str(&esc);
    }
}

/// Événements logués par le terminal virtuel.
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum VEvent {
    Text(String),
    Sgr(SgrSeq),
}

/// Terminal virtuel headless : journalise les opérations.
#[derive(Default)]
pub struct VirtualTerm {
    log: Vec<VEvent>,
}

impl VirtualTerm {
    pub fn new() -> Self { Self { log: Vec::new() } }
    pub fn dump(&self) -> &[VEvent] { &self.log }
    pub fn into_log(self) -> Vec<VEvent> { self.log }
}

impl Renderer for VirtualTerm {
    fn write_str(&mut self, s: &str) { self.log.push(VEvent::Text(s.to_string())); }
    fn sgr(&mut self, seq: SgrSeq)   { self.log.push(VEvent::Sgr(seq)); }
}

/// Mode simulation stable pour tests. Wrapper de VirtualTerm.
pub struct SimTerm(VirtualTerm);

impl SimTerm {
    pub fn new() -> Self { Self(VirtualTerm::new()) }
    pub fn snapshot(&self) -> Vec<VEvent> { self.0.dump().to_vec() }
    pub fn into_log(self) -> Vec<VEvent> { self.0.into_log() }
}

impl Renderer for SimTerm {
    fn write_str(&mut self, s: &str) { self.0.write_str(s); }
    fn sgr(&mut self, seq: SgrSeq)   { self.0.sgr(seq); }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::sgr::{Style, FontFamily, WidthMode};

    #[test]
    fn realterm_emits_reset() {
        let mut r = RealTerm::stdout();
        r.sgr(SgrSeq::Reset); // smoke test
    }

    #[test]
    fn virtual_logs_in_order() {
        let mut v = VirtualTerm::new();
        v.sgr(SgrSeq::Set(Style { bold: true, ..Default::default() }));
        v.text("X");
        v.sgr(SgrSeq::Reset);
        let log = v.dump();
        assert!(matches!(log[0], VEvent::Sgr(_)));
        assert!(matches!(log[1], VEvent::Text(ref s) if s=="X"));
        assert!(matches!(log[2], VEvent::Sgr(SgrSeq::Reset)));
    }

    #[test]
    fn sim_is_deterministic() {
        let mut s = SimTerm::new();
        s.styled("Hi", Style { font: FontFamily::Alt(1), width: WidthMode::Normal, ..Default::default() });
        let snap = s.snapshot();
        assert_eq!(snap.len(), 3);
    }
}
