//! vitte-debugger — API publique de débogage (embedding, outils, DAP backend)
//!
//! - `Debugger` : façade thread-safe pour piloter une VM Vitte (run, step, vars…)
//! - `VitteVm`  : trait à implémenter par ta VM (ou un adaptateur) pour être débogable
//! - Événements `DebugEvent` : abonnement non-bloquant via mpsc
//!
//! Conçu pour être utilisé directement par un UI (TUI/GUI) ou par un adaptateur DAP.
//!
//! Features conseillées côté crate :
//!   [dependencies]
//!   color-eyre = "0.6"
//!   serde = { version = "1", features = ["derive"] }
//!   crossbeam-channel = "0.5"
//!
//! Exemple rapide :
//!   let (dbg, rx) = Debugger::new(Box::new(MyVm::new()));
//!   dbg.launch("examples/hello.vitte", &[])?;
//!   dbg.set_breakpoints("examples/hello.vitte", &[3,7])?;
//!   dbg.continue_all()?;
//!   while let Ok(ev) = rx.recv() { println!("ev = {ev:?}"); }

use std::collections::HashMap;
use std::path::Path;
use std::sync::{Arc, Mutex};

use color_eyre::eyre::{eyre, Result};
use crossbeam_channel::{unbounded, Receiver, Sender};
use serde::{Deserialize, Serialize};

pub type ThreadId = i64;
pub type FrameId = i64;
pub type VarRef = i64;

/// Représentation d’un frame de pile.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Frame {
    pub id: FrameId,
    pub thread_id: ThreadId,
    pub name: String,
    pub source_path: String,
    pub line: u32,
    pub column: u32,
}

/// Variable lisible/expandable.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Variable {
    pub name: String,
    pub value: String,
    #[serde(default)]
    pub r#type: Option<String>,
    /// > 0 si l’objet est expansible (struct, array, map…)
    #[serde(default)]
    pub variables_reference: VarRef,
}

/// Portée (Locals, Globals, Upvalues, Registers…)
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Scope {
    pub name: String,
    pub variables_reference: VarRef,
    #[serde(default)]
    pub expensive: bool,
}

/// Point d’arrêt demandé vs réellement armé.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Breakpoint {
    pub source_path: String,
    pub requested_line: u32,
    pub verified: bool,
    /// Ligne effective (par ex. correspondance après remapping)
    pub actual_line: u32,
}

/// Événements émis par le moteur de debug (push, non bloquants).
#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum DebugEvent {
    /// Premier paint après attach/launch/config
    Initialized,
    /// L’exécution continue (thread donné ou tous)
    Continued { thread_id: Option<ThreadId> },
    /// Arrêté sur breakpoint
    StoppedBreakpoint { thread_id: ThreadId, source_path: String, line: u32 },
    /// Pas à pas terminé
    StoppedStep { thread_id: ThreadId },
    /// Exception/erreur du programme
    Exception { message: String, source_path: Option<String>, line: Option<u32> },
    /// Process terminé
    Terminated { exit_code: i32 },
    /// Log/Output provenant du programme/VM
    Output { category: String, text: String },
}

/// Contrat minimal pour qu’une VM Vitte soit débogable.
pub trait VitteVm: Send {
    /// Charger/préparer un programme (sans l’exécuter).
    fn launch(&mut self, program: &str, args: &[String]) -> Result<()>;

    /// Continuer l’exécution (thread principal si non pertinent).
    fn continue_all(&mut self) -> Result<()>;

    /// Pause (si supportée).
    fn pause(&mut self) -> Result<()> { Err(eyre!("pause() non supporté")) }

    /// Step over.
    fn step_over(&mut self) -> Result<()>;

    /// Step into (optionnel).
    fn step_in(&mut self) -> Result<()> { Err(eyre!("step_in() non supporté")) }

    /// Step out (optionnel).
    fn step_out(&mut self) -> Result<()> { Err(eyre!("step_out() non supporté")) }

    /// Définir/mettre à jour les breakpoints d’un fichier. Retourne l’état armé.
    fn set_breakpoints(&mut self, source_path: &str, lines: &[u32]) -> Result<Vec<Breakpoint>>;

    /// Pile du thread courant (ou principal).
    fn stack_trace(&self, thread_id: Option<ThreadId>) -> Result<Vec<Frame>>;

    /// Scopes pour un frame (Locals etc.). Retourne un `variables_reference` par scope.
    fn scopes(&self, frame_id: FrameId) -> Result<Vec<Scope>>;

    /// Explorer les variables d’un `variables_reference`.
    fn variables(&self, var_ref: VarRef) -> Result<Vec<Variable>>;

    /// Évaluer une expression dans le contexte courant (optionnel).
    fn evaluate(&mut self, _expr: &str) -> Result<Option<String>> { Ok(None) }

    /// Stop/cleanup propre.
    fn disconnect(&mut self) -> Result<()>;
}

/// État interne du Debugger.
#[derive(Default)]
struct State {
    program: Option<String>,
    breakpoints: HashMap<String, Vec<Breakpoint>>, // key = path canonique
    /// Compteur pour fabriquer des variables_reference uniques (≥ 1)
    next_varref: VarRef,
}

/// Façade thread-safe.
/// - Toutes les méthodes sont O(1)/O(n) simples et délèguent à la VM.
/// - Les événements push sont publiés sur `events_tx`.
pub struct Debugger {
    vm: Arc<Mutex<Box<dyn VitteVm>>>,
    state: Arc<Mutex<State>>,
    events_tx: Sender<DebugEvent>,
}

impl Debugger {
    /// Construit un Debugger et renvoie aussi le Receiver d’événements.
    pub fn new(vm: Box<dyn VitteVm>) -> (Self, Receiver<DebugEvent>) {
        let (tx, rx) = unbounded();
        let dbg = Self {
            vm: Arc::new(Mutex::new(vm)),
            state: Arc::new(Mutex::new(State { program: None, breakpoints: HashMap::new(), next_varref: 2 })),
            events_tx: tx,
        };
        (dbg, rx)
    }

    /// Programme + args (sans exécuter). Émet `Initialized`.
    pub fn launch<P: AsRef<Path>>(&self, program: P, args: &[String]) -> Result<()> {
        let path_str = program.as_ref().to_string_lossy().to_string();
        self.vm.lock().unwrap().launch(&path_str, args)?;
        self.state.lock().unwrap().program = Some(path_str);
        self.push(DebugEvent::Initialized);
        Ok(())
    }

    /// Met en place des breakpoints pour un fichier. Retourne la réalité armée.
    pub fn set_breakpoints<P: AsRef<Path>>(&self, source_path: P, lines: &[u32]) -> Result<Vec<Breakpoint>> {
        let sp = source_path.as_ref().to_string_lossy().to_string();
        let armed = self.vm.lock().unwrap().set_breakpoints(&sp, lines)?;
        self.state.lock().unwrap().breakpoints.insert(sp.clone(), armed.clone());
        Ok(armed)
    }

    /// Continue l’exécution (tous threads si applicable).
    pub fn continue_all(&self) -> Result<()> {
        self.vm.lock().unwrap().continue_all()?;
        self.push(DebugEvent::Continued { thread_id: None });
        Ok(())
    }

    /// Pause (si supportée).
    pub fn pause(&self) -> Result<()> {
        self.vm.lock().unwrap().pause()?;
        // Pas d’event standard immédiat ; le prochain sera Stopped*
        Ok(())
    }

    pub fn step_over(&self) -> Result<()> {
        self.vm.lock().unwrap().step_over()?;
        self.push(DebugEvent::StoppedStep { thread_id: 1 });
        Ok(())
    }

    pub fn step_in(&self) -> Result<()> {
        self.vm.lock().unwrap().step_in()?;
        self.push(DebugEvent::StoppedStep { thread_id: 1 });
        Ok(())
    }

    pub fn step_out(&self) -> Result<()> {
        self.vm.lock().unwrap().step_out()?;
        self.push(DebugEvent::StoppedStep { thread_id: 1 });
        Ok(())
    }

    pub fn stack_trace(&self, thread: Option<ThreadId>) -> Result<Vec<Frame>> {
        self.vm.lock().unwrap().stack_trace(thread)
    }

    pub fn scopes(&self, frame_id: FrameId) -> Result<Vec<Scope>> {
        self.vm.lock().unwrap().scopes(frame_id)
    }

    pub fn variables(&self, var_ref: VarRef) -> Result<Vec<Variable>> {
        self.vm.lock().unwrap().variables(var_ref)
    }

    pub fn evaluate(&self, expr: &str) -> Result<Option<String>> {
        self.vm.lock().unwrap().evaluate(expr)
    }

    /// Terminaison/cleanup.
    pub fn disconnect(&self) -> Result<()> {
        self.vm.lock().unwrap().disconnect()?;
        self.push(DebugEvent::Terminated { exit_code: 0 });
        Ok(())
    }

    /// Helper pour émettre un log/output user-facing.
    pub fn emit_output(&self, category: impl Into<String>, text: impl Into<String>) {
        let _ = self.events_tx.send(DebugEvent::Output { category: category.into(), text: text.into() });
    }

    /// Helper pour signaler un arrêt sur breakpoint (à appeler depuis la VM/adaptateur).
    pub fn emit_breakpoint_hit(&self, source_path: impl Into<String>, line: u32, thread: ThreadId) {
        let _ = self.events_tx.send(DebugEvent::StoppedBreakpoint {
            thread_id: thread,
            source_path: source_path.into(),
            line,
        });
    }

    /// Helper pour signaler une exception VM.
    pub fn emit_exception(&self, message: impl Into<String>, source_path: Option<String>, line: Option<u32>) {
        let _ = self.events_tx.send(DebugEvent::Exception {
            message: message.into(),
            source_path,
            line,
        });
    }

    fn push(&self, ev: DebugEvent) {
        let _ = self.events_tx.send(ev);
    }
}

/* -------------------------------------------------------------
   Implémentation de démonstration : DummyVm (facultatif, tests)
   ------------------------------------------------------------- */

#[cfg(test)]
mod tests {
    use super::*;
    use std::cell::RefCell;

    struct DummyVm {
        loaded: bool,
        lines: Vec<u32>,
        frames: Vec<Frame>,
        vars: HashMap<VarRef, Vec<Variable>>,
    }

    impl DummyVm {
        fn new() -> Self {
            let mut vars = HashMap::new();
            vars.insert(1, vec![
                Variable { name: "x".into(), value: "42".into(), r#type: Some("i32".into()), variables_reference: 0 }
            ]);
            Self {
                loaded: false,
                lines: vec![],
                frames: vec![Frame { id: 10, thread_id: 1, name: "main".into(), source_path: "dummy.vitte".into(), line: 1, column: 1 }],
                vars,
            }
        }
    }

    impl VitteVm for DummyVm {
        fn launch(&mut self, _program: &str, _args: &[String]) -> Result<()> {
            self.loaded = true;
            Ok(())
        }
        fn continue_all(&mut self) -> Result<()> {
            if !self.loaded { return Err(eyre!("not launched")); }
            Ok(())
        }
        fn step_over(&mut self) -> Result<()> { Ok(()) }
        fn set_breakpoints(&mut self, _source_path: &str, lines: &[u32]) -> Result<Vec<Breakpoint>> {
            self.lines = lines.to_vec();
            Ok(lines.iter().map(|l| Breakpoint {
                source_path: "dummy.vitte".into(),
                requested_line: *l,
                verified: true,
                actual_line: *l,
            }).collect())
        }
        fn stack_trace(&self, _thread_id: Option<ThreadId>) -> Result<Vec<Frame>> {
            Ok(self.frames.clone())
        }
        fn scopes(&self, _frame_id: FrameId) -> Result<Vec<Scope>> {
            Ok(vec![Scope { name: "Locals".into(), variables_reference: 1, expensive: false }])
        }
        fn variables(&self, var_ref: VarRef) -> Result<Vec<Variable>> {
            Ok(self.vars.get(&var_ref).cloned().unwrap_or_default())
        }
        fn disconnect(&mut self) -> Result<()> { Ok(()) }
    }

    #[test]
    fn happy_flow() -> Result<()> {
        let (dbg, rx) = Debugger::new(Box::new(DummyVm::new()));
        dbg.launch("dummy.vitte", &[])?;
        assert!(matches!(rx.recv().unwrap(), DebugEvent::Initialized));

        let armed = dbg.set_breakpoints("dummy.vitte", &[3, 7])?;
        assert_eq!(armed.len(), 2);
        dbg.continue_all()?;

        let frames = dbg.stack_trace(None)?;
        assert_eq!(frames.len(), 1);

        let scopes = dbg.scopes(frames[0].id)?;
        assert_eq!(scopes[0].variables_reference, 1);

        let vars = dbg.variables(1)?;
        assert_eq!(vars[0].value, "42");

        dbg.step_over()?;
        assert!(matches!(rx.recv().unwrap(), DebugEvent::StoppedStep{..}));

        dbg.disconnect()?;
        assert!(matches!(rx.recv().unwrap(), DebugEvent::Terminated{..}));
        Ok(())
    }
}
