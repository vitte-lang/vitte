// vitte-ansi/tests/sim.rs

use vitte_ansi::render::{Renderer, SimTerm, VEvent, VirtualTerm};
use vitte_ansi::sgr::{FontFamily, SgrSeq, Style, WidthMode};

#[test]
fn sim_is_deterministic_and_balanced() {
    let mut r = SimTerm::new();
    r.styled(
        "hi",
        Style {
            bold: true,
            italic: false,
            underline: true,
            inverse: false,
            font: FontFamily::Default,
            width: WidthMode::Normal,
        },
    );
    let snap = r.snapshot();

    assert_eq!(snap.len(), 3, "Set, Text, Reset");
    assert!(matches!(snap[0], VEvent::Sgr(SgrSeq::Set(_))));
    assert!(matches!(snap[1], VEvent::Text(ref s) if s == "hi"));
    assert!(matches!(snap[2], VEvent::Sgr(SgrSeq::Reset)));
}

#[test]
fn virtual_term_logs_all_events_in_order() {
    let mut v = VirtualTerm::new();
    v.sgr(SgrSeq::Set(Style { bold: true, ..Default::default() }));
    v.text("A");
    v.text("B");
    v.sgr(SgrSeq::Reset);

    let log = v.dump();
    assert_eq!(log.len(), 4);
    assert!(matches!(log[0], VEvent::Sgr(_)));
    assert!(matches!(log[1], VEvent::Text(ref s) if s=="A"));
    assert!(matches!(log[2], VEvent::Text(ref s) if s=="B"));
    assert!(matches!(log[3], VEvent::Sgr(SgrSeq::Reset)));
}

#[test]
fn sgr_to_ansi_degrades_without_doublewidth_or_fonts() {
    // Sans support étendu: pas de ESC #3/#4/#6 ni SGR 10..19
    let s = Style {
        bold: true,
        font: FontFamily::Alt(2),
        width: WidthMode::DoubleWidth,
        ..Default::default()
    };
    let esc = SgrSeq::Set(s).to_ansi(false, false);
    assert!(esc.contains("\x1b["), "SGR de base attendu");
    assert!(!esc.contains("\x1b#3"));
    assert!(!esc.contains("\x1b#4"));
    assert!(!esc.contains("\x1b#6"));
    // police non émise
    assert!(!esc.contains("\x1b[12m"));
}

#[test]
fn sgr_to_ansi_emits_doublewidth_and_font_when_supported() {
    let s = Style {
        bold: false,
        font: FontFamily::Alt(2),
        width: WidthMode::DoubleWidth,
        ..Default::default()
    };
    let esc = SgrSeq::Set(s).to_ansi(true, true);
    // Double largeur: ESC # 6
    assert!(esc.contains("\x1b#6"));
    // Police Alt(2) => 10 + 2 = 12
    assert!(esc.contains("\x1b[12m") || esc.contains(";12m"), "doit contenir SGR 12");
}

#[test]
fn width_top_bottom_sequences_when_supported() {
    let top = SgrSeq::Set(Style { width: WidthMode::DoubleHeightTop, ..Default::default() })
        .to_ansi(true, false);
    let bot = SgrSeq::Set(Style { width: WidthMode::DoubleHeightBottom, ..Default::default() })
        .to_ansi(true, false);
    assert!(top.contains("\x1b#3"));
    assert!(bot.contains("\x1b#4"));
}

#[test]
fn reset_emits_clear_sequence() {
    let r = SgrSeq::Reset.to_ansi(false, false);
    assert_eq!(r, "\x1b[0m");
}
