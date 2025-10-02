//! vitte-term — utilitaires terminal **ultra ultra complets**
//!
//! Capacités principales:
//! - Détection TTY, largeur/hauteur, palette, support ANSI basique.
//! - Curseur: show/hide, move, save/restore, position, clear.
//! - Écran alternatif (enter/leave).
//! - Mode raw (activer/désactiver) avec garde RAII.
//! - Événements clavier/souris/redimensionnement (backend `crossterm` par défaut, `termion` option).
//! - Écriture ANSI directe optionnelle (`vitte-ansi`) ou via backend.
//! - Unicode: calcul de largeur, coupe et clamp graphemes.
//! - Timing simple (fps loop) si feature `time`.
//!
//! Zéro `unsafe`.

#![forbid(unsafe_code)]
#![cfg_attr(docsrs, feature(doc_cfg))]

/* ============================== imports ============================== */

#[cfg(all(not(feature="std"), not(feature="alloc-only")))]
compile_error!("Enable `std` (default) or `alloc-only`.") ;

#[cfg(feature="alloc-only")]
extern crate alloc;

#[cfg(feature="alloc-only")]
use alloc::{string::String, vec::Vec};

#[cfg(feature="std")]
use std::{string::String, vec::Vec, io::{self, Write}, time::Duration};

#[cfg(feature="serde")]
use serde::{Serialize, Deserialize};

#[cfg(feature="errors")]
use thiserror::Error;

#[cfg(feature="ansi")]
use vitte_ansi as ansi;

#[cfg(feature="detect")]
use {atty, terminal_size::{terminal_size, Height, Width}};

#[cfg(feature="unicode")]
use {unicode_width::UnicodeWidthStr, unicode_segmentation::UnicodeSegmentation};

#[cfg(feature="crossterm")]
use crossterm::{
    cursor as ct_cursor, event as ct_event, execute, queue,
    terminal as ct_term, style as ct_style,
};

#[cfg(feature="termion")]
use termion::{
    cursor as t_cursor, event as t_event, input::TermRead, raw::{IntoRawMode, RawTerminal},
    screen::AlternateScreen,
};

#[cfg(feature="time")]
use instant::Instant;

/* ============================== erreurs ============================== */

#[cfg(feature="errors")]
#[derive(Debug, Error)]
pub enum TermError {
    #[error("io: {0}")]
    Io(String),
    #[error("backend feature missing")]
    BackendMissing,
}
#[cfg(not(feature="errors"))]
#[derive(Debug)]
pub enum TermError { Io(String), BackendMissing }

pub type Result<T> = core::result::Result<T, TermError>;

#[inline] fn io_err<T>(e: impl core::fmt::Display) -> Result<T> { Err(TermError::Io(e.to_string())) }

/* ============================== infos terminal ============================== */

/// Capacités détectées.
#[cfg_attr(feature="serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug, Default)]
pub struct TermInfo {
    pub is_tty_stdout: bool,
    pub is_tty_stderr: bool,
    pub cols: u16,
    pub rows: u16,
    pub ansi_colors: bool,
}

impl TermInfo {
    /// Détecte les infos. Si `detect` absent, valeurs naïves.
    pub fn detect() -> Self {
        #[cfg(feature="detect")]
        {
            let (cols, rows) = match terminal_size() {
                Some((Width(w), Height(h))) => (w as u16, h as u16),
                None => (80, 24),
            };
            Self {
                is_tty_stdout: atty::is(atty::Stream::Stdout),
                is_tty_stderr: atty::is(atty::Stream::Stderr),
                cols, rows,
                ansi_colors: true, // heuristique simple
            }
        }
        #[cfg(not(feature="detect"))]
        { Self { is_tty_stdout: true, is_tty_stderr: true, cols: 80, rows: 24, ansi_colors: true } }
    }
}

/* ============================== curseur & écran ============================== */

/// Opérations terminal de haut niveau (backend-agnostique).
pub struct Term {
    info: TermInfo,
}

impl Default for Term { fn default() -> Self { Self::new() } }

impl Term {
    pub fn new() -> Self { Self { info: TermInfo::detect() } }
    pub fn info(&self) -> &TermInfo { &self.info }

    /// Efface l’écran entier et place le curseur en (1,1).
    pub fn clear_screen(&self) -> Result<()> {
        #[cfg(feature="crossterm")]
        {
            execute!(io::stdout(), ct_term::Clear(ct_term::ClearType::All), ct_cursor::MoveTo(0,0))
                .map_err(|e| TermError::Io(e.to_string()))?;
            return Ok(());
        }
        #[cfg(not(feature="crossterm"))] {
            print!("\x1B[2J\x1B[H"); let _=io::stdout().flush(); Ok(())
        }
    }

    /// Efface la ligne courante.
    pub fn clear_line(&self) -> Result<()> {
        #[cfg(feature="crossterm")]
        {
            queue!(io::stdout(), ct_term::Clear(ct_term::ClearType::CurrentLine))
                .map_err(|e| TermError::Io(e.to_string()))?;
            let _=io::stdout().flush(); return Ok(());
        }
        #[cfg(not(feature="crossterm"))] {
            print!("\r\x1B[2K"); let _=io::stdout().flush(); Ok(())
        }
    }

    pub fn show_cursor(&self) -> Result<()> {
        #[cfg(feature="crossterm")] { execute!(io::stdout(), ct_cursor::Show).map_err(|e| TermError::Io(e.to_string()))?; return Ok(()); }
        #[cfg(not(feature="crossterm"))] { print!("\x1B[?25h"); let _=io::stdout().flush(); Ok(()) }
    }
    pub fn hide_cursor(&self) -> Result<()> {
        #[cfg(feature="crossterm")] { execute!(io::stdout(), ct_cursor::Hide).map_err(|e| TermError::Io(e.to_string()))?; return Ok(()); }
        #[cfg(not(feature="crossterm"))] { print!("\x1B[?25l"); let _=io::stdout().flush(); Ok(()) }
    }

    pub fn save_cursor(&self) -> Result<()> {
        #[cfg(feature="crossterm")] { execute!(io::stdout(), ct_cursor::SavePosition).map_err(|e| TermError::Io(e.to_string()))?; return Ok(()); }
        #[cfg(not(feature="crossterm"))] { print!("\x1B7"); let _=io::stdout().flush(); Ok(()) }
    }
    pub fn restore_cursor(&self) -> Result<()> {
        #[cfg(feature="crossterm")] { execute!(io::stdout(), ct_cursor::RestorePosition).map_err(|e| TermError::Io(e.to_string()))?; return Ok(()); }
        #[cfg(not(feature="crossterm"))] { print!("\x1B8"); let _=io::stdout().flush(); Ok(()) }
    }

    /// Position absolue 0-based. Clamp simple.
    pub fn move_to(&self, col: u16, row: u16) -> Result<()> {
        #[cfg(feature="crossterm")]
        {
            execute!(io::stdout(), ct_cursor::MoveTo(col, row)).map_err(|e| TermError::Io(e.to_string()))?;
            return Ok(());
        }
        #[cfg(not(feature="crossterm"))] {
            print!("\x1B[{};{}H", row.saturating_add(1), col.saturating_add(1));
            let _=io::stdout().flush(); Ok(())
        }
    }

    /// Écrit sans style puis flush.
    pub fn write(&self, s: &str) -> Result<()> {
        let mut out = io::stdout();
        out.write_all(s.as_bytes()).map_err(|e| TermError::Io(e.to_string()))?;
        out.flush().map_err(|e| TermError::Io(e.to_string()))
    }

    /// Écran alternatif RAII.
    pub fn alt_screen(&self) -> Result<AltScreenGuard> {
        AltScreenGuard::enter()
    }

    /// Mode raw RAII.
    pub fn raw_mode(&self) -> Result<RawModeGuard> {
        RawModeGuard::enter()
    }
}

/* ============================== écran alternatif ============================== */

pub struct AltScreenGuard {
    #[cfg(feature="crossterm")] _flag: (),
    #[cfg(feature="termion")] screen: Option<AlternateScreen<RawTerminal<io::Stdout>>>,
    #[cfg(all(not(feature="crossterm"), not(feature="termion")))] _dummy: (),
}
impl AltScreenGuard {
    pub fn enter() -> Result<Self> {
        #[cfg(feature="crossterm")]
        {
            ct_term::enable_raw_mode().map_err(|e| TermError::Io(e.to_string()))?;
            execute!(io::stdout(), ct_term::EnterAlternateScreen, ct_cursor::Hide)
                .map_err(|e| TermError::Io(e.to_string()))?;
            return Ok(Self{ _flag: () });
        }
        #[cfg(all(feature="termion", not(feature="crossterm")))]
        {
            let raw = io::stdout().into_raw_mode().map_err(|e| TermError::Io(e.to_string()))?;
            let screen = AlternateScreen::from(raw);
            print!("{}", t_cursor::Hide);
            let _=io::stdout().flush();
            Ok(Self { screen: Some(screen) })
        }
        #[cfg(all(not(feature="crossterm"), not(feature="termion")))]
        { print!("\x1B[?1049h\x1B[?25l"); let _=io::stdout().flush(); Ok(Self{ _dummy: () }) }
    }
}
impl Drop for AltScreenGuard {
    fn drop(&mut self) {
        #[cfg(feature="crossterm")]
        {
            let _ = execute!(io::stdout(), ct_cursor::Show, ct_term::LeaveAlternateScreen);
            let _ = ct_term::disable_raw_mode();
        }
        #[cfg(all(feature="termion", not(feature="crossterm")))]
        {
            print!("{}", t_cursor::Show);
            let _=io::stdout().flush();
            // drop screen automatically
        }
        #[cfg(all(not(feature="crossterm"), not(feature="termion")))]
        { let _=write!(io::stdout(), "\x1B[?25h\x1B[?1049l"); let _=io::stdout().flush(); }
    }
}

/* ============================== raw mode ============================== */

pub struct RawModeGuard {
    #[cfg(feature="crossterm")] _flag: (),
    #[cfg(feature="termion")] _rt: Option<RawTerminal<io::Stdout>>,
    #[cfg(all(not(feature="crossterm"), not(feature="termion")))] _dummy: (),
}
impl RawModeGuard {
    pub fn enter() -> Result<Self> {
        #[cfg(feature="crossterm")]
        {
            ct_term::enable_raw_mode().map_err(|e| TermError::Io(e.to_string()))?;
            return Ok(Self { _flag: () });
        }
        #[cfg(all(feature="termion", not(feature="crossterm")))]
        {
            let rt = io::stdout().into_raw_mode().map_err(|e| TermError::Io(e.to_string()))?;
            Ok(Self { _rt: Some(rt) })
        }
        #[cfg(all(not(feature="crossterm"), not(feature="termion")))]
        { Ok(Self { _dummy: () }) }
    }
}
impl Drop for RawModeGuard {
    fn drop(&mut self) {
        #[cfg(feature="crossterm")]
        { let _ = ct_term::disable_raw_mode(); }
        #[cfg(all(feature="termion", not(feature="crossterm")))]
        { /* drop restores */ }
    }
}

/* ============================== événements ============================== */

/// Évènement unifié.
#[cfg_attr(feature="serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug, PartialEq)]
pub enum Event {
    Key(KeyEvent),
    Resize(u16, u16),
    Mouse(MouseEvent),
    Unknown,
}

#[cfg_attr(feature="serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug, PartialEq)]
pub struct KeyEvent {
    pub code: KeyCode,
    pub ctrl: bool,
    pub alt: bool,
    pub shift: bool,
}

#[cfg_attr(feature="serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug, PartialEq)]
pub enum KeyCode {
    Char(char),
    Enter, Esc, Backspace, Tab,
    Up, Down, Left, Right,
    Home, End, PageUp, PageDown,
    Delete, Insert,
    F(u8),
}

#[cfg_attr(feature="serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug, PartialEq)]
pub struct MouseEvent {
    pub kind: MouseKind,
    pub col: u16,
    pub row: u16,
    pub shift: bool,
    pub ctrl: bool,
    pub alt: bool,
}
#[cfg_attr(feature="serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug, PartialEq)]
pub enum MouseKind { Down(u8), Up(u8), Drag(u8), Move, ScrollUp, ScrollDown }

/// Lecture bloquante d’un évènement.
pub fn read_event() -> Result<Event> {
    #[cfg(feature="crossterm")]
    {
        use ct_event::{Event as E, KeyModifiers, KeyCode as KC, MouseEventKind as MK, MouseButton as MB};
        let ev = ct_event::read().map_err(|e| TermError::Io(e.to_string()))?;
        let e = match ev {
            E::Key(k) => {
                let (ctrl, alt, shift) = mods(k.modifiers);
                let code = match k.code {
                    KC::Enter => KeyCode::Enter,
                    KC::Esc => KeyCode::Esc,
                    KC::Backspace => KeyCode::Backspace,
                    KC::Tab => KeyCode::Tab,
                    KC::Up => KeyCode::Up, KC::Down => KeyCode::Down, KC::Left => KeyCode::Left, KC::Right => KeyCode::Right,
                    KC::Home => KeyCode::Home, KC::End => KeyCode::End, KC::PageUp => KeyCode::PageUp, KC::PageDown => KeyCode::PageDown,
                    KC::Delete => KeyCode::Delete, KC::Insert => KeyCode::Insert,
                    KC::F(n) => KeyCode::F(n as u8),
                    KC::Char(c) => KeyCode::Char(c),
                    _ => KeyCode::Char('\0')
                };
                Event::Key(KeyEvent { code, ctrl, alt, shift })
            }
            E::Resize(w,h) => Event::Resize(w as u16, h as u16),
            E::Mouse(m) => {
                let (ctrl, alt, shift) = mods(m.modifiers);
                let kind = match m.kind {
                    MK::Down(b) => MouseKind::Down(button_id(b)),
                    MK::Up(b) => MouseKind::Up(button_id(b)),
                    MK::Drag(b) => MouseKind::Drag(button_id(b)),
                    MK::Moved => MouseKind::Move,
                    MK::ScrollUp => MouseKind::ScrollUp,
                    MK::ScrollDown => MouseKind::ScrollDown,
                    _ => MouseKind::Move,
                };
                Event::Mouse(MouseEvent { kind, col: m.column as u16, row: m.row as u16, ctrl, alt, shift })
            }
            _ => Event::Unknown
        };
        return Ok(e);
    }
    #[cfg(all(feature="termion", not(feature="crossterm")))]
    {
        let stdin = io::stdin();
        for ev in stdin.events() {
            let ev = ev.map_err(|e| TermError::Io(e.to_string()))?;
            use t_event::Event as E;
            let out = match ev {
                E::Key(k) => Event::Key(map_termion_key(k)),
                E::Mouse(me) => Event::Mouse(map_termion_mouse(me)),
                E::Unsupported(_) => Event::Unknown,
                E::FocusGained | E::FocusLost | E::Paste(_) => Event::Unknown,
            };
            return Ok(out);
        }
        Ok(Event::Unknown)
    }
    #[cfg(all(not(feature="crossterm"), not(feature="termion")))]
    { Err(TermError::BackendMissing) }
}

#[cfg(feature="crossterm")]
#[inline]
fn mods(m: ct_event::KeyModifiers) -> (bool,bool,bool) {
    (m.contains(ct_event::KeyModifiers::CONTROL), m.contains(ct_event::KeyModifiers::ALT), m.contains(ct_event::KeyModifiers::SHIFT))
}
#[cfg(feature="crossterm")]
#[inline]
fn button_id(b: ct_event::MouseButton) -> u8 {
    match b { ct_event::MouseButton::Left=>0, ct_event::MouseButton::Middle=>1, ct_event::MouseButton::Right=>2 }
}

#[cfg(all(feature="termion", not(feature="crossterm")))]
fn map_termion_key(k: t_event::Key) -> KeyEvent {
    use t_event::Key::*;
    let (ctrl, alt, shift) = (false,false,false); // termion ne fournit pas toujours les mods
    let code = match k {
        Char('\n') => KeyCode::Enter,
        Esc => KeyCode::Esc,
        Backspace => KeyCode::Backspace,
        Tab => KeyCode::Tab,
        Left => KeyCode::Left, Right => KeyCode::Right, Up => KeyCode::Up, Down => KeyCode::Down,
        Home => KeyCode::Home, End => KeyCode::End, PageUp => KeyCode::PageUp, PageDown => KeyCode::PageDown,
        Delete => KeyCode::Delete, Insert => KeyCode::Insert,
        F(n) => KeyCode::F(n as u8),
        Char(c) => KeyCode::Char(c),
        _ => KeyCode::Char('\0'),
    };
    KeyEvent { code, ctrl, alt, shift }
}
#[cfg(all(feature="termion", not(feature="crossterm")))]
fn map_termion_mouse(m: t_event::MouseEvent) -> MouseEvent {
    use t_event::MouseEvent::*;
    match m {
        Press(btn, x, y) => MouseEvent { kind: MouseKind::Down(match btn { t_event::MouseButton::Left=>0, t_event::MouseButton::Middle=>1, t_event::MouseButton::Right=>2 }), col:x as u16, row:y as u16, ctrl:false, alt:false, shift:false },
        Release(x, y) => MouseEvent { kind: MouseKind::Up(0), col:x as u16, row:y as u16, ctrl:false, alt:false, shift:false },
        Hold(x,y) => MouseEvent { kind: MouseKind::Drag(0), col:x as u16, row:y as u16, ctrl:false, alt:false, shift:false },
    }
}

/* ============================== unicode utils ============================== */

/// Largeur d’affichage (graphemes si feature `unicode`).
pub fn width(s: &str) -> usize {
    #[cfg(feature="unicode")]
    { UnicodeWidthStr::width(s) }
    #[cfg(not(feature="unicode"))]
    { s.len() }
}

/// Tronque une chaîne à une `max_width` d’affichage en conservant graphemes.
pub fn clamp_width(s: &str, max_width: usize) -> String {
    if width(s) <= max_width { return s.to_string(); }
    #[cfg(feature="unicode")]
    {
        let mut cur = 0usize;
        let mut out = String::new();
        for g in UnicodeSegmentation::graphemes(s, true) {
            let w = UnicodeWidthStr::width(g);
            if cur + w > max_width { break; }
            out.push_str(g);
            cur += w;
        }
        out
    }
    #[cfg(not(feature="unicode"))]
    { s.chars().take(max_width).collect() }
}

/* ============================== timing ============================== */

/// Boucle à `fps` en appelant `tick()`. Stoppe si `stop()` renvoie `true`.
#[cfg(feature="time")]
pub fn run_fps_loop<F, S>(fps: u32, mut tick: F, mut stop: S)
where F: FnMut(u32), S: FnMut() -> bool
{
    let fps = fps.max(1).min(240);
    let step = Duration::from_secs_f64(1.0 / fps as f64);
    let mut frame: u32 = 0;
    let mut last = Instant::now();
    while !stop() {
        let now = Instant::now();
        if now.duration_since(last) >= step {
            last = now;
            tick(frame);
            frame = frame.wrapping_add(1);
        } else {
            std::thread::sleep(Duration::from_millis(1));
        }
    }
}

/* ============================== style util (optionnel) ============================== */

/// Applique couleur si `ansi` actif, sinon passthrough.
pub fn colorize(kind: ColorKind, s: &str) -> String {
    #[cfg(feature="ansi")]
    {
        use ColorKind::*;
        match kind {
            Primary => ansi::cyan(s).to_string(),
            Success => ansi::green(s).to_string(),
            Warn    => ansi::yellow(s).to_string(),
            Error   => ansi::red(s).to_string(),
            Info    => ansi::blue(s).to_string(),
            Muted   => ansi::dim(s).to_string(),
        }
    }
    #[cfg(not(feature="ansi"))]
    { s.to_string() }
}

#[cfg_attr(feature="serde", derive(Serialize, Deserialize))]
#[derive(Copy, Clone, Debug, PartialEq, Eq)]
pub enum ColorKind { Primary, Success, Warn, Error, Info, Muted }

/* ============================== tests ============================== */

#[cfg(test)]
mod tests {
    use super::*;
    #[test] fn width_basic() { assert!(width("abc") >= 3); }
    #[test] fn clamp_ok() { assert!(clamp_width("abcdef", 3).len() <= 3); }
}