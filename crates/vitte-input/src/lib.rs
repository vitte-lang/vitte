//! vitte-input — unification clavier/souris/gamepad/touch (desktop + web)
//!
//! Couvre :
//! - Desktop (feature `native`): intégration `winit` (clavier/souris) et `gilrs` (gamepads).
//! - Web (feature `web`): écouteurs DOM (KeyboardEvent/MouseEvent/Gamepad).
//!
//! Modèle :
//! - `InputEvent` = flux d’événements normalisés.
//! - `InputState` = état agrégé (touches enfoncées, positions, axes).
//! - `InputManager` = collecte + mise à jour par frame, helpers de requête.
//!
//! Zéro `unsafe`. Sérialisation optionnelle via `serde`.

#![forbid(unsafe_code)]
#![deny(missing_docs)]

use std::collections::{HashMap, HashSet, VecDeque};
use thiserror::Error;

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

/// Résultat.
pub type InputResult<T> = Result<T, InputError>;

/// Erreurs.
#[derive(Debug, Error)]
pub enum InputError {
    /// Opération non supportée dans ce build ou environnement.
    #[error("unsupported: {0}")]
    Unsupported(&'static str),
    /// Autre.
    #[error("{0}")]
    Other(String),
}

/// Identifiant logique de gamepad.
pub type GamepadId = u32;

/// Horodatage monotonique en secondes.
pub type TimeSec = f64;

/// Boutons de souris.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Debug, Copy, Clone, PartialEq, Eq, Hash)]
pub enum MouseButton {
    Left,
    Right,
    Middle,
    Other(u16),
}

/// Codes de touches approximés, stables inter plateformes (sous-ensemble utile).
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Debug, Copy, Clone, PartialEq, Eq, Hash)]
pub enum KeyCode {
    // lettres
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    // chiffres rangée
    Digit0, Digit1, Digit2, Digit3, Digit4, Digit5, Digit6, Digit7, Digit8, Digit9,
    // modifs
    ShiftLeft, ShiftRight, ControlLeft, ControlRight, AltLeft, AltRight, MetaLeft, MetaRight,
    // flèches
    ArrowUp, ArrowDown, ArrowLeft, ArrowRight,
    // édition
    Enter, Escape, Backspace, Tab, Space,
    // pavé
    Insert, Delete, Home, End, PageUp, PageDown,
    // fonction
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    // divers
    Minus, Equal, BracketLeft, BracketRight, Semicolon, Quote, Backquote, Backslash, Comma, Period, Slash,
    // fallback
    Unknown(u32),
}

/// Modificateurs clavier.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Debug, Copy, Clone, Default, PartialEq, Eq)]
pub struct Modifiers {
    /// Shift actif.
    pub shift: bool,
    /// Ctrl actif.
    pub ctrl: bool,
    /// Alt actif.
    pub alt: bool,
    /// Meta/Command actif.
    pub meta: bool,
}

/// Événements d’entrée unifiés.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Debug, Clone, PartialEq)]
pub enum InputEvent {
    /// Touche pressée.
    KeyDown { key: KeyCode, mods: Modifiers, time: TimeSec, repeat: bool },
    /// Touche relâchée.
    KeyUp   { key: KeyCode, mods: Modifiers, time: TimeSec },
    /// Bouton souris pressé.
    MouseDown { button: MouseButton, x: f32, y: f32, mods: Modifiers, time: TimeSec },
    /// Bouton souris relâché.
    MouseUp   { button: MouseButton, x: f32, y: f32, mods: Modifiers, time: TimeSec },
    /// Mouvement souris.
    MouseMove { x: f32, y: f32, dx: f32, dy: f32, time: TimeSec },
    /// Molette (lignes > 0 = haut).
    Wheel { delta_x: f32, delta_y: f32, time: TimeSec },
    /// Connexion de gamepad.
    GamepadConnected { id: GamepadId, name: String, time: TimeSec },
    /// Déconnexion de gamepad.
    GamepadDisconnected { id: GamepadId, time: TimeSec },
    /// Bouton gamepad.
    GamepadButton { id: GamepadId, button: u16, value: f32, pressed: bool, time: TimeSec },
    /// Axe gamepad.
    GamepadAxis { id: GamepadId, axis: u16, value: f32, time: TimeSec },
    /// Tap tactile simple (web).
    TouchTap { id: i32, x: f32, y: f32, time: TimeSec },
}

/// Instantané de l’état souris.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Debug, Copy, Clone, Default)]
pub struct MouseState {
    /// Position en pixels.
    pub x: f32,
    /// Position en pixels.
    pub y: f32,
    /// Delta X dernière frame.
    pub dx: f32,
    /// Delta Y dernière frame.
    pub dy: f32,
    /// Molette cumulée depuis frame précédente.
    pub wheel_x: f32,
    /// Molette cumulée depuis frame précédente.
    pub wheel_y: f32,
    /// Boutons enfoncés.
    pub buttons: [bool; 8],
}

/// État d’un gamepad.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Debug, Clone, Default)]
pub struct GamepadState {
    /// Nom.
    pub name: String,
    /// Boutons (valeur analogique).
    pub buttons: HashMap<u16, f32>,
    /// Axes analogiques normalisés [-1,1].
    pub axes: HashMap<u16, f32>,
    /// Connecté.
    pub connected: bool,
}

/// État agrégé.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Debug, Clone)]
pub struct InputState {
    /// Ensemble touches enfoncées.
    pub keys_down: HashSet<KeyCode>,
    /// Juste pressées à cette frame.
    pub keys_pressed: HashSet<KeyCode>,
    /// Juste relâchées à cette frame.
    pub keys_released: HashSet<KeyCode>,
    /// Modificateurs courants.
    pub mods: Modifiers,
    /// Souris.
    pub mouse: MouseState,
    /// Gamepads.
    pub gamepads: HashMap<GamepadId, GamepadState>,
    /// Compteur de frame.
    pub frame: u64,
}

impl Default for InputState {
    fn default() -> Self {
        Self {
            keys_down: HashSet::new(),
            keys_pressed: HashSet::new(),
            keys_released: HashSet::new(),
            mods: Modifiers::default(),
            mouse: MouseState::default(),
            gamepads: HashMap::new(),
            frame: 0,
        }
    }
}

impl InputState {
    /// Cache les deltas instantanés, avance la frame.
    pub fn next_frame(&mut self) {
        self.keys_pressed.clear();
        self.keys_released.clear();
        self.mouse.dx = 0.0;
        self.mouse.dy = 0.0;
        self.mouse.wheel_x = 0.0;
        self.mouse.wheel_y = 0.0;
        self.frame = self.frame.wrapping_add(1);
    }

    /// Est-ce que `key` est enfoncée.
    pub fn key_down(&self, key: KeyCode) -> bool { self.keys_down.contains(&key) }
    /// `key` vient d’être pressée cette frame.
    pub fn key_pressed(&self, key: KeyCode) -> bool { self.keys_pressed.contains(&key) }
    /// `key` vient d’être relâchée cette frame.
    pub fn key_released(&self, key: KeyCode) -> bool { self.keys_released.contains(&key) }
    /// Bouton souris enfoncé.
    pub fn mouse_down(&self, button: MouseButton) -> bool {
        match button {
            MouseButton::Left   => self.mouse.buttons[0],
            MouseButton::Right  => self.mouse.buttons[1],
            MouseButton::Middle => self.mouse.buttons[2],
            MouseButton::Other(i) => self.mouse.buttons.get(i as usize).copied().unwrap_or(false),
        }
    }
}

/// Gestionnaire d’entrée. Accumule des `InputEvent` et met à jour `InputState`.
pub struct InputManager {
    queue: VecDeque<InputEvent>,
    /// Horloge fournie par l’hôte (secs).
    clock_fn: Box<dyn Fn() -> TimeSec + Send + Sync>,
    /// État courant.
    pub state: InputState,
}

impl Default for InputManager {
    fn default() -> Self {
        Self::new(|| now_secs_std())
    }
}

impl InputManager {
    /// Nouveau manager avec une fonction d’horloge.
    pub fn new<F: 'static + Fn() -> TimeSec + Send + Sync>(clock: F) -> Self {
        Self {
            queue: VecDeque::new(),
            clock_fn: Box::new(clock),
            state: InputState::default(),
        }
    }

    /// Ajoute un événement brut.
    pub fn push(&mut self, ev: InputEvent) {
        self.queue.push_back(ev);
    }

    /// Consomme la queue et met à jour l’état.
    pub fn update(&mut self) {
        self.state.next_frame();
        while let Some(ev) = self.queue.pop_front() {
            self.apply(ev);
        }
        // Intégration web: collecter les events postés par le DOM.
        #[cfg(feature = "web")]
        for ev in crate::web::drain_web_queue() {
            self.apply(ev);
        }
        // Intégration native gamepads: polling gilrs
        #[cfg(all(feature = "native", feature = "std"))]
        if let Some(p) = crate::native::poll_gilrs() {
            for ev in p {
                self.apply(ev);
            }
        }
    }

    fn apply(&mut self, ev: InputEvent) {
        match ev {
            InputEvent::KeyDown { key, mods, repeat, .. } => {
                if !self.state.keys_down.contains(&key) {
                    self.state.keys_pressed.insert(key);
                }
                self.state.keys_down.insert(key);
                self.state.mods = mods;
                if repeat {
                    // rien de spécial: l’appli peut utiliser `keys_down`
                }
            }
            InputEvent::KeyUp { key, mods, .. } => {
                self.state.keys_down.remove(&key);
                self.state.keys_released.insert(key);
                self.state.mods = mods;
            }
            InputEvent::MouseDown { button, x, y, mods, .. } => {
                self.state.mouse.x = x;
                self.state.mouse.y = y;
                self.set_mouse_button(button, true);
                self.state.mods = mods;
            }
            InputEvent::MouseUp { button, x, y, mods, .. } => {
                self.state.mouse.x = x;
                self.state.mouse.y = y;
                self.set_mouse_button(button, false);
                self.state.mods = mods;
            }
            InputEvent::MouseMove { x, y, dx, dy, .. } => {
                self.state.mouse.x = x;
                self.state.mouse.y = y;
                self.state.mouse.dx += dx;
                self.state.mouse.dy += dy;
            }
            InputEvent::Wheel { delta_x, delta_y, .. } => {
                self.state.mouse.wheel_x += delta_x;
                self.state.mouse.wheel_y += delta_y;
            }
            InputEvent::GamepadConnected { id, name, .. } => {
                self.state.gamepads.entry(id).or_default().connected = true;
                self.state.gamepads.entry(id).or_default().name = name;
            }
            InputEvent::GamepadDisconnected { id, .. } => {
                if let Some(g) = self.state.gamepads.get_mut(&id) {
                    g.connected = false;
                }
            }
            InputEvent::GamepadButton { id, button, value, pressed, .. } => {
                let g = self.state.gamepads.entry(id).or_default();
                g.connected = true;
                g.buttons.insert(button, if pressed { value.max(1.0) } else { 0.0 });
            }
            InputEvent::GamepadAxis { id, axis, value, .. } => {
                let g = self.state.gamepads.entry(id).or_default();
                g.connected = true;
                g.axes.insert(axis, value.clamp(-1.0, 1.0));
            }
            InputEvent::TouchTap { .. } => {
                // pas d’état persistant par défaut
            }
        }
    }

    fn set_mouse_button(&mut self, button: MouseButton, down: bool) {
        let idx = match button {
            MouseButton::Left => 0,
            MouseButton::Right => 1,
            MouseButton::Middle => 2,
            MouseButton::Other(i) => i as usize,
        };
        if idx < self.state.mouse.buttons.len() {
            self.state.mouse.buttons[idx] = down;
        }
    }

    /// Horodatage courant via clock.
    pub fn now(&self) -> TimeSec { (self.clock_fn)() }
}

// -------------------- Desktop (winit + gilrs) --------------------

#[cfg(feature = "native")]
pub mod native {
    use super::*;

    /// Map `winit` → `KeyCode`.
    #[inline]
    pub fn map_keycode(k: &winit::keyboard::KeyCode) -> KeyCode {
        use winit::keyboard::KeyCode as W;
        match *k {
            W::KeyA => KeyCode::A, W::KeyB => KeyCode::B, W::KeyC => KeyCode::C, W::KeyD => KeyCode::D,
            W::KeyE => KeyCode::E, W::KeyF => KeyCode::F, W::KeyG => KeyCode::G, W::KeyH => KeyCode::H,
            W::KeyI => KeyCode::I, W::KeyJ => KeyCode::J, W::KeyK => KeyCode::K, W::KeyL => KeyCode::L,
            W::KeyM => KeyCode::M, W::KeyN => KeyCode::N, W::KeyO => KeyCode::O, W::KeyP => KeyCode::P,
            W::KeyQ => KeyCode::Q, W::KeyR => KeyCode::R, W::KeyS => KeyCode::S, W::KeyT => KeyCode::T,
            W::KeyU => KeyCode::U, W::KeyV => KeyCode::V, W::KeyW => KeyCode::W, W::KeyX => KeyCode::X,
            W::KeyY => KeyCode::Y, W::KeyZ => KeyCode::Z,
            W::Digit0 => KeyCode::Digit0, W::Digit1 => KeyCode::Digit1, W::Digit2 => KeyCode::Digit2,
            W::Digit3 => KeyCode::Digit3, W::Digit4 => KeyCode::Digit4, W::Digit5 => KeyCode::Digit5,
            W::Digit6 => KeyCode::Digit6, W::Digit7 => KeyCode::Digit7, W::Digit8 => KeyCode::Digit8, W::Digit9 => KeyCode::Digit9,
            W::ShiftLeft => KeyCode::ShiftLeft, W::ShiftRight => KeyCode::ShiftRight,
            W::ControlLeft => KeyCode::ControlLeft, W::ControlRight => KeyCode::ControlRight,
            W::AltLeft => KeyCode::AltLeft, W::AltRight => KeyCode::AltRight,
            W::MetaLeft => KeyCode::MetaLeft, W::MetaRight => KeyCode::MetaRight,
            W::ArrowUp => KeyCode::ArrowUp, W::ArrowDown => KeyCode::ArrowDown, W::ArrowLeft => KeyCode::ArrowLeft, W::ArrowRight => KeyCode::ArrowRight,
            W::Enter => KeyCode::Enter, W::Escape => KeyCode::Escape, W::Backspace => KeyCode::Backspace, W::Tab => KeyCode::Tab, W::Space => KeyCode::Space,
            W::Insert => KeyCode::Insert, W::Delete => KeyCode::Delete, W::Home => KeyCode::Home, W::End => KeyCode::End,
            W::PageUp => KeyCode::PageUp, W::PageDown => KeyCode::PageDown,
            W::F1 => KeyCode::F1, W::F2 => KeyCode::F2, W::F3 => KeyCode::F3, W::F4 => KeyCode::F4, W::F5 => KeyCode::F5,
            W::F6 => KeyCode::F6, W::F7 => KeyCode::F7, W::F8 => KeyCode::F8, W::F9 => KeyCode::F9, W::F10 => KeyCode::F10, W::F11 => KeyCode::F11, W::F12 => KeyCode::F12,
            W::Minus => KeyCode::Minus, W::Equal => KeyCode::Equal, W::BracketLeft => KeyCode::BracketLeft, W::BracketRight => KeyCode::BracketRight,
            W::Semicolon => KeyCode::Semicolon, W::Quote => KeyCode::Quote, W::Backquote => KeyCode::Backquote, W::Backslash => KeyCode::Backslash,
            W::Comma => KeyCode::Comma, W::Period => KeyCode::Period, W::Slash => KeyCode::Slash,
            _ => KeyCode::Unknown((*k) as u32),
        }
    }

    /// Extrait `Modifiers` de winit.
    #[inline]
    pub fn map_mods(m: &winit::keyboard::ModifiersState) -> Modifiers {
        Modifiers {
            shift: m.shift_key(),
            ctrl: m.control_key(),
            alt: m.alt_key(),
            meta: m.super_key(),
        }
    }

    /// Push des événements clavier/souris dans le manager.
    pub fn handle_winit_event(mgr: &mut crate::InputManager, event: &winit::event::WindowEvent) {
        use winit::event::{ElementState, MouseButton as WM, MouseScrollDelta, TouchPhase, WindowEvent};
        let t = mgr.now();

        match event {
            WindowEvent::KeyboardInput { event, is_synthetic, .. } => {
                if let Some(code) = event.logical_key.clone().to_keycode() {
                    // winit 0.30: KeyEvent → physical/logical. On mappe physical_key si dispo.
                    let key = map_keycode(&code);
                    let mods = map_mods(&event.modifiers);
                    let repeat = *is_synthetic || event.repeat;
                    match event.state {
                        ElementState::Pressed => mgr.push(InputEvent::KeyDown { key, mods, time: t, repeat }),
                        ElementState::Released => mgr.push(InputEvent::KeyUp { key, mods, time: t }),
                    }
                }
            }
            WindowEvent::MouseInput { state, button, .. } => {
                let btn = match button {
                    WM::Left => MouseButton::Left,
                    WM::Right => MouseButton::Right,
                    WM::Middle => MouseButton::Middle,
                    WM::Other(x) => MouseButton::Other(*x as u16),
                };
                let mods = Modifiers::default();
                let (x, y) = (mgr.state.mouse.x, mgr.state.mouse.y);
                match state {
                    ElementState::Pressed => mgr.push(InputEvent::MouseDown { button: btn, x, y, mods, time: t }),
                    ElementState::Released => mgr.push(InputEvent::MouseUp { button: btn, x, y, mods, time: t }),
                }
            }
            WindowEvent::CursorMoved { position, .. } => {
                let x = position.x as f32;
                let y = position.y as f32;
                let dx = x - mgr.state.mouse.x;
                let dy = y - mgr.state.mouse.y;
                mgr.push(InputEvent::MouseMove { x, y, dx, dy, time: t });
            }
            WindowEvent::MouseWheel { delta, .. } => {
                let (mut dx, mut dy) = (0.0, 0.0);
                match delta {
                    MouseScrollDelta::LineDelta(x, y) => { dx = *x; dy = *y; }
                    MouseScrollDelta::PixelDelta(p) => { dx = p.x as f32; dy = p.y as f32; }
                }
                mgr.push(InputEvent::Wheel { delta_x: dx, delta_y: dy, time: t });
            }
            WindowEvent::Touch(touch) => {
                if matches!(touch.phase, TouchPhase::Ended) {
                    mgr.push(InputEvent::TouchTap { id: touch.id as i32, x: touch.location.x as f32, y: touch.location.y as f32, time: t });
                }
            }
            _ => {}
        }
    }

    // ---------------- Gamepads (gilrs) ----------------

    #[cfg(feature = "gilrs")]
    static mut GILRS: Option<gilrs::Gilrs> = None;

    /// Initialise gilrs à la première demande.
    #[cfg(feature = "gilrs")]
    fn gilrs_ref() -> Option<&'static mut gilrs::Gilrs> {
        unsafe {
            if GILRS.is_none() {
                GILRS = gilrs::Gilrs::new().ok();
            }
            GILRS.as_mut()
        }
    }

    /// Poll gilrs et produit des événements.
    /// Retourne None si gilrs indisponible ou non compilé.
    pub fn poll_gilrs() -> Option<Vec<InputEvent>> {
        #[cfg(feature = "gilrs")]
        {
            let mut out = Vec::new();
            let g = gilrs_ref()?;
            let t = now_secs_std();
            while let Some(ev) = g.next_event() {
                use gilrs::{EventType, Button, Axis};
                match ev.event {
                    EventType::Connected => {
                        let name = g.gamepad(ev.id).name().to_string();
                        out.push(InputEvent::GamepadConnected { id: ev.id.0 as u32, name, time: t });
                    }
                    EventType::Disconnected => {
                        out.push(InputEvent::GamepadDisconnected { id: ev.id.0 as u32, time: t });
                    }
                    EventType::ButtonPressed(b, _) | EventType::ButtonChanged(b, 1.0, _) => {
                        out.push(InputEvent::GamepadButton { id: ev.id.0 as u32, button: map_button(b), value: 1.0, pressed: true, time: t });
                    }
                    EventType::ButtonReleased(b, _) | EventType::ButtonChanged(b, 0.0, _) => {
                        out.push(InputEvent::GamepadButton { id: ev.id.0 as u32, button: map_button(b), value: 0.0, pressed: false, time: t });
                    }
                    EventType::AxisChanged(a, v, _) => {
                        out.push(InputEvent::GamepadAxis { id: ev.id.0 as u32, axis: map_axis(a), value: v as f32, time: t });
                    }
                    _ => {}
                }
            }
            Some(out)
        }
        #[cfg(not(feature = "gilrs"))]
        {
            None
        }
    }

    #[cfg(feature = "gilrs")]
    fn map_button(b: gilrs::Button) -> u16 {
        use gilrs::Button::*;
        match b {
            South => 0, East => 1, West => 2, North => 3,
            LeftTrigger => 4, RightTrigger => 5,
            LeftTrigger2 => 6, RightTrigger2 => 7,
            Select => 8, Start => 9, Mode => 10,
            LeftThumb => 11, RightThumb => 12,
            DPadUp => 13, DPadDown => 14, DPadLeft => 15, DPadRight => 16,
            Other(i) => 100 + i as u16,
        }
    }

    #[cfg(feature = "gilrs")]
    fn map_axis(a: gilrs::Axis) -> u16 {
        use gilrs::Axis::*;
        match a {
            LeftStickX => 0, LeftStickY => 1,
            RightStickX => 2, RightStickY => 3,
            LeftZ => 4, RightZ => 5,
            DPadX => 6, DPadY => 7,
            Other(i) => 100 + i as u16,
        }
    }
}

// -------------------- Web (wasm) --------------------

#[cfg(feature = "web")]
pub mod web {
    use super::*;
    use wasm_bindgen::prelude::*;
    use wasm_bindgen::JsCast;
    use web_sys::{window, KeyboardEvent, MouseEvent, WheelEvent};

    thread_local! {
        static Q: std::cell::RefCell<Vec<InputEvent>> = const { std::cell::RefCell::new(Vec::new()) };
    }

    fn push(ev: InputEvent) {
        Q.with(|q| q.borrow_mut().push(ev));
    }

    /// Draine la queue web (appelé par `InputManager::update`).
    pub(crate) fn drain_web_queue() -> Vec<InputEvent> {
        Q.with(|q| std::mem::take(&mut *q.borrow_mut()))
    }

    /// Installe des écouteurs de base (clavier/souris) sur `window`.
    #[wasm_bindgen]
    pub fn init_listeners() -> Result<(), JsValue> {
        let win = window().ok_or_else(|| JsValue::from_str("no window"))?;

        // keyboard
        {
            let keydown = Closure::<dyn FnMut(KeyboardEvent)>::new(move |e: KeyboardEvent| {
                let key = map_key_str(&e.code());
                let mods = Modifiers { shift: e.shift_key(), ctrl: e.ctrl_key(), alt: e.alt_key(), meta: e.meta_key() };
                let t = now_secs_js();
                push(InputEvent::KeyDown { key, mods, time: t, repeat: e.repeat() });
            });
            win.add_event_listener_with_callback("keydown", keydown.as_ref().unchecked_ref())?;
            keydown.forget();

            let keyup = Closure::<dyn FnMut(KeyboardEvent)>::new(move |e: KeyboardEvent| {
                let key = map_key_str(&e.code());
                let mods = Modifiers { shift: e.shift_key(), ctrl: e.ctrl_key(), alt: e.alt_key(), meta: e.meta_key() };
                let t = now_secs_js();
                push(InputEvent::KeyUp { key, mods, time: t });
            });
            win.add_event_listener_with_callback("keyup", keyup.as_ref().unchecked_ref())?;
            keyup.forget();
        }

        // mouse
        {
            let mousemove = Closure::<dyn FnMut(MouseEvent)>::new(move |e: MouseEvent| {
                let t = now_secs_js();
                let x = e.client_x() as f32; let y = e.client_y() as f32;
                let dx = e.movement_x() as f32; let dy = e.movement_y() as f32;
                push(InputEvent::MouseMove { x, y, dx, dy, time: t });
            });
            win.add_event_listener_with_callback("mousemove", mousemove.as_ref().unchecked_ref())?;
            mousemove.forget();

            let mousedown = Closure::<dyn FnMut(MouseEvent)>::new(move |e: MouseEvent| {
                let t = now_secs_js();
                let x = e.client_x() as f32; let y = e.client_y() as f32;
                let btn = map_mouse_button(e.button());
                let mods = Modifiers { shift: e.shift_key(), ctrl: e.ctrl_key(), alt: e.alt_key(), meta: e.meta_key() };
                push(InputEvent::MouseDown { button: btn, x, y, mods, time: t });
            });
            win.add_event_listener_with_callback("mousedown", mousedown.as_ref().unchecked_ref())?;
            mousedown.forget();

            let mouseup = Closure::<dyn FnMut(MouseEvent)>::new(move |e: MouseEvent| {
                let t = now_secs_js();
                let x = e.client_x() as f32; let y = e.client_y() as f32;
                let btn = map_mouse_button(e.button());
                let mods = Modifiers { shift: e.shift_key(), ctrl: e.ctrl_key(), alt: e.alt_key(), meta: e.meta_key() };
                push(InputEvent::MouseUp { button: btn, x, y, mods, time: t });
            });
            win.add_event_listener_with_callback("mouseup", mouseup.as_ref().unchecked_ref())?;
            mouseup.forget();

            let wheel = Closure::<dyn FnMut(WheelEvent)>::new(move |e: WheelEvent| {
                let t = now_secs_js();
                push(InputEvent::Wheel { delta_x: e.delta_x() as f32, delta_y: e.delta_y() as f32, time: t });
            });
            win.add_event_listener_with_callback("wheel", wheel.as_ref().unchecked_ref())?;
            wheel.forget();
        }

        Ok(())
    }

    fn map_key_str(code: &str) -> KeyCode {
        use KeyCode::*;
        match code {
            "KeyA"=>A,"KeyB"=>B,"KeyC"=>C,"KeyD"=>D,"KeyE"=>E,"KeyF"=>F,"KeyG"=>G,"KeyH"=>H,"KeyI"=>I,"KeyJ"=>J,"KeyK"=>K,"KeyL"=>L,"KeyM"=>M,
            "KeyN"=>N,"KeyO"=>O,"KeyP"=>P,"KeyQ"=>Q,"KeyR"=>R,"KeyS"=>S,"KeyT"=>T,"KeyU"=>U,"KeyV"=>V,"KeyW"=>W,"KeyX"=>X,"KeyY"=>Y,"KeyZ"=>Z,
            "Digit0"=>Digit0,"Digit1"=>Digit1,"Digit2"=>Digit2,"Digit3"=>Digit3,"Digit4"=>Digit4,"Digit5"=>Digit5,"Digit6"=>Digit6,"Digit7"=>Digit7,"Digit8"=>Digit8,"Digit9"=>Digit9,
            "ShiftLeft"=>ShiftLeft,"ShiftRight"=>ShiftRight,"ControlLeft"=>ControlLeft,"ControlRight"=>ControlRight,"AltLeft"=>AltLeft,"AltRight"=>AltRight,"MetaLeft"=>MetaLeft,"MetaRight"=>MetaRight,
            "ArrowUp"=>ArrowUp,"ArrowDown"=>ArrowDown,"ArrowLeft"=>ArrowLeft,"ArrowRight"=>ArrowRight,
            "Enter"=>Enter,"Escape"=>Escape,"Backspace"=>Backspace,"Tab"=>Tab,"Space"=>Space,
            "Insert"=>Insert,"Delete"=>Delete,"Home"=>Home,"End"=>End,"PageUp"=>PageUp,"PageDown"=>PageDown,
            "F1"=>F1,"F2"=>F2,"F3"=>F3,"F4"=>F4,"F5"=>F5,"F6"=>F6,"F7"=>F7,"F8"=>F8,"F9"=>F9,"F10"=>F10,"F11"=>F11,"F12"=>F12,
            "Minus"=>Minus,"Equal"=>Equal,"BracketLeft"=>BracketLeft,"BracketRight"=>BracketRight,"Semicolon"=>Semicolon,"Quote"=>Quote,"Backquote"=>Backquote,"Backslash"=>Backslash,"Comma"=>Comma,"Period"=>Period,"Slash"=>Slash,
            _ => Unknown(0),
        }
    }

    fn map_mouse_button(b: i16) -> MouseButton {
        match b {
            0 => MouseButton::Left,
            1 => MouseButton::Middle,
            2 => MouseButton::Right,
            x => MouseButton::Other(x as u16),
        }
    }
}

// -------------------- Utilitaires communs --------------------

/// Horloge standard (secs, f64).
#[inline]
fn now_secs_std() -> TimeSec {
    use std::time::{SystemTime, UNIX_EPOCH};
    let t = SystemTime::now().duration_since(UNIX_EPOCH).unwrap_or_default();
    t.as_secs_f64()
}

#[cfg(feature = "web")]
#[inline]
fn now_secs_js() -> TimeSec {
    // performance.now() en ms
    web_sys::window()
        .and_then(|w| w.performance())
        .map(|p| p.now() as f64 / 1000.0)
        .unwrap_or(0.0)
}

// -------------------- Tests --------------------

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn key_press_release_cycle() {
        let mut m = InputManager::default();
        let t = m.now();
        m.push(InputEvent::KeyDown { key: KeyCode::A, mods: Modifiers::default(), time: t, repeat: false });
        m.update();
        assert!(m.state.key_down(KeyCode::A));
        assert!(m.state.key_pressed(KeyCode::A));
        m.update();
        assert!(m.state.key_down(KeyCode::A));
        assert!(!m.state.key_pressed(KeyCode::A));
        m.push(InputEvent::KeyUp { key: KeyCode::A, mods: Modifiers::default(), time: m.now() });
        m.update();
        assert!(!m.state.key_down(KeyCode::A));
        assert!(m.state.key_released(KeyCode::A));
    }

    #[test]
    fn mouse_move_and_wheel() {
        let mut m = InputManager::default();
        m.push(InputEvent::MouseMove { x: 10.0, y: 20.0, dx: 10.0, dy: 20.0, time: m.now() });
        m.push(InputEvent::Wheel { delta_x: 1.0, delta_y: -2.0, time: m.now() });
        m.update();
        assert_eq!(m.state.mouse.x, 10.0);
        assert_eq!(m.state.mouse.y, 20.0);
        assert_eq!(m.state.mouse.dx, 10.0);
        assert_eq!(m.state.mouse.dy, 20.0);
        assert_eq!(m.state.mouse.wheel_x, 1.0);
        assert_eq!(m.state.mouse.wheel_y, -2.0);
        m.update();
        assert_eq!(m.state.mouse.dx, 0.0);
        assert_eq!(m.state.mouse.wheel_y, 0.0);
    }

    #[test]
    fn gamepad_axis_button_state() {
        let mut m = InputManager::default();
        let id = 1;
        m.push(InputEvent::GamepadConnected { id, name: "pad".into(), time: m.now() });
        m.push(InputEvent::GamepadAxis { id, axis: 0, value: 0.5, time: m.now() });
        m.push(InputEvent::GamepadButton { id, button: 1, value: 1.0, pressed: true, time: m.now() });
        m.update();
        let g = m.state.gamepads.get(&id).unwrap();
        assert!(g.connected);
        assert_eq!(g.axes.get(&0).copied().unwrap_or(0.0), 0.5);
        assert_eq!(g.buttons.get(&1).copied().unwrap_or(0.0), 1.0);
    }
}