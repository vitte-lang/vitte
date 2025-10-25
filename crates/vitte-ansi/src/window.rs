// vitte-ansi/src/windows.rs
//! Détection des capacités ANSI sous Windows + activation VT.
//! Couvre ConPTY (Windows Terminal/ConEmu), VT Processing, TTY in/out.

use std::io::{stdin, stdout, IsTerminal};
use windows_sys::Win32::Foundation::HANDLE;
use windows_sys::Win32::System::Console::{
    GetConsoleMode, SetConsoleMode, GetStdHandle,
    ENABLE_VIRTUAL_TERMINAL_PROCESSING, DISABLE_NEWLINE_AUTO_RETURN,
    ENABLE_VIRTUAL_TERMINAL_INPUT,
    STD_OUTPUT_HANDLE, STD_INPUT_HANDLE,
};

use crate::caps::{Capabilities, Capability};

/// Détecte et configure au mieux les capacités Windows.
pub fn detect_capabilities_win() -> Capabilities {
    let mut caps = Capabilities::new();

    // Heuristiques TTY.
    caps.is_tty_out = stdout().is_terminal();
    caps.is_tty_in  = stdin().is_terminal();

    // Heuristique ConPTY / hôtes compatibles (Windows Terminal, ConEmu, etc.).
    if std::env::var_os("WT_SESSION").is_some()
        || std::env::var_os("ConEmuPID").is_some()
        || std::env::var_os("TERM").map_or(false, |t| t.to_string_lossy().contains("xterm"))
    {
        caps.set(Capability::ConPTY);
    }

    // Tente d’activer l’ANSI sur stdout.
    if enable_vt_on(STD_OUTPUT_HANDLE, ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN) {
        caps.set(Capability::AnsiOut);
    }

    // Tente d’activer l’ANSI sur stdin (peu utilisé, mais propre).
    if enable_vt_on(STD_INPUT_HANDLE, ENABLE_VIRTUAL_TERMINAL_INPUT) {
        caps.set(Capability::AnsiIn);
    }

    // Pas de détection fiable pour DoubleWidth / FontSelect sur la console classique.
    // Windows Terminal peut rendre certaines séquences, mais non garanti -> ne pas activer.
    caps
}

/// Active des flags VT sur un handle standard. Retourne true si succès.
fn enable_vt_on(std_handle: i32, flags: u32) -> bool {
    unsafe {
        let h: HANDLE = GetStdHandle(std_handle);
        if h == 0 || h == (-1isize as HANDLE) {
            return false;
        }
        let mut mode: u32 = 0;
        if GetConsoleMode(h, &mut mode) == 0 {
            return false;
        }
        let new_mode = mode | flags;
        if new_mode == mode {
            // Déjà actif.
            return true;
        }
        SetConsoleMode(h, new_mode) != 0
    }
}

#[cfg(test)]
#[cfg(windows)]
mod tests {
    use super::*;

    #[test]
    fn detect_win_runs() {
        let _c = detect_capabilities_win();
        // Juste exécutable sans panic.
    }
}
