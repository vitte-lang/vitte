//! debug.rs — Pont VM ↔ Débogueur
//!
//! Ce module intègre vitte-vm avec vitte-debugger :
//! - installe des hooks sur l’exécution (avant/après chaque opcode)
//! - délègue au BreakpointManager & StepController
//! - notifie le DebugState des arrêts (StopReason)
//!
//! Usage typique dans la VM :
//!   let mut dbg = VmDebugger::new(thread_id, &mut debug_state);
//!   vm.set_debug_hook(Box::new(dbg));
//!
//! Ensuite, à chaque instruction exécutée, `on_instruction` est appelé.
//!

use std::sync::{Arc, Mutex};

use color_eyre::eyre::{Result, eyre};
use log::{debug, warn};

use vitte_debugger::api::{ThreadId, StopReason, Frame};
use vitte_debugger::breakpoint::BreakpointManager;
use vitte_debugger::hook::{HookRegistry, ExecContext};
use vitte_debugger::state::DebugState;
use vitte_debugger::step::StepController;

/// Interface que la VM expose pour brancher un hook debug.
pub trait VmHook {
    /// Appelée avant l’exécution d’une instruction.
    fn on_instruction(&mut self, ctx: &ExecContext) -> Result<()>;
}

/// Adaptateur qui connecte la VM au vitte-debugger
pub struct VmDebugger {
    pub thread_id: ThreadId,
    pub state: Arc<Mutex<DebugState>>,
    pub breakpoints: Arc<Mutex<BreakpointManager>>,
    pub steps: Arc<Mutex<StepController>>,
    pub hooks: Arc<Mutex<HookRegistry>>,
}

impl VmDebugger {
    pub fn new(
        thread_id: ThreadId,
        state: Arc<Mutex<DebugState>>,
        breakpoints: Arc<Mutex<BreakpointManager>>,
        steps: Arc<Mutex<StepController>>,
        hooks: Arc<Mutex<HookRegistry>>,
    ) -> Self {
        Self { thread_id, state, breakpoints, steps, hooks }
    }

    /// Décide si on doit stopper l’exécution à ce point.
    fn should_stop(&self, ctx: &ExecContext, stack_depth: usize) -> Option<StopReason> {
        // 1. Breakpoints
        if let Some(path) = &ctx.source_path {
            if let Some(line) = ctx.line {
                let bps = self.breakpoints.lock().unwrap();
                if bps.hit(path, line) {
                    return Some(StopReason::Breakpoint);
                }
            }
        }

        // 2. Stepping
        {
            let steps = self.steps.lock().unwrap();
            if steps.should_stop(ctx, stack_depth) {
                return Some(StopReason::Step);
            }
        }

        None
    }

    /// Notifie le DebugState qu’on s’arrête ici.
    fn notify_stop(&self, reason: StopReason, ctx: &ExecContext) -> Result<()> {
        let mut st = self.state.lock().unwrap();
        st.record_stop(self.thread_id, reason, ctx)
    }
}

impl VmHook for VmDebugger {
    fn on_instruction(&mut self, ctx: &ExecContext) -> Result<()> {
        let depth = ctx.frame_id.map(|_| 1).unwrap_or(1); // TODO : calculer vraie profondeur pile
        if let Some(reason) = self.should_stop(ctx, depth) {
            debug!("Thread {} stopped: {:?}", self.thread_id, reason);
            self.notify_stop(reason, ctx)?;
        }
        Ok(())
    }
}
