//! step.rs — Contrôleur de pas-à-pas (step into/over/out) au niveau ligne-source
//!
//! Usage typique côté VM/adaptateur :
//!   let mut sc = StepController::default();
//!   sc.begin(tid, StepMode::Over, &frames_at_stop);
//!   ...
//!   // À chaque instruction exécutée (avant/après):
//!   if sc.should_stop(&ctx, current_stack_depth) {
//!       // Pauser et rafraîchir l'état; puis :
—      sc.finish(tid); // ou sc.deactivate(tid);
//!   }
//!
//! Hypothèses :
//!   - `ExecContext.source_path` et `ExecContext.line` sont *logiques* (déjà mappés via HookRegistry).
//!   - `current_stack_depth` = nombre de frames visibles (top = 0 → depth=1 ; ici on prend "len").
//!
//! Intégration :
//!   - Combine proprement avec les breakpoints : on évalue d'abord BP, puis le step controller
//!     (si BP ne déclenche pas, le step peut décider d’arrêter).

use std::collections::HashMap;

use serde::{Deserialize, Serialize};

use crate::api::{Frame, ThreadId};
use crate::hook::ExecContext;
use crate::state::StepMode;

/// Instantané de départ pour un plan de step.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct StepPlan {
    pub mode: StepMode,
    pub start_depth: usize,          // profondeur de pile au départ (frames.len())
    pub start_path: Option<String>,  // chemin logique
    pub start_line: Option<u32>,     // ligne logique
}

impl StepPlan {
    fn new(mode: StepMode, frames_at_start: &[Frame]) -> Self {
        let (path, line) = frames_at_start
            .first()
            .map(|f| (Some(f.source_path.clone()), Some(f.line)))
            .unwrap_or((None, None));
        Self {
            mode,
            start_depth: frames_at_start.len(),
            start_path: path,
            start_line: line,
        }
    }
}

/// Contrôleur par thread. Stocke un plan actif jusqu’à ce qu’on décide de s’arrêter.
#[derive(Default)]
pub struct StepController {
    by_thread: HashMap<ThreadId, StepPlan>,
}

impl StepController {
    /// (Ré)initialise un plan de step pour `tid` à partir des frames capturés au moment du stop.
    pub fn begin(&mut self, tid: ThreadId, mode: StepMode, frames_at_start: &[Frame]) {
        self.by_thread.insert(tid, StepPlan::new(mode, frames_at_start));
    }

    /// Désactive le plan de step pour `tid` (à appeler quand on a décidé de s’arrêter).
    pub fn finish(&mut self, tid: ThreadId) {
        self.by_thread.remove(&tid);
    }

    /// Alias sémantique.
    pub fn deactivate(&mut self, tid: ThreadId) { self.finish(tid); }

    /// Le step est-il actif pour `tid` ?
    pub fn is_active(&self, tid: ThreadId) -> bool {
        self.by_thread.contains_key(&tid)
    }

    /// Décide si on doit s’arrêter **maintenant** (avant/après l’instruction courante).
    ///
    /// `ctx` : contexte exécution *logique* (path/line mappés).
    /// `current_stack_depth` : profondeur de pile actuelle (frames.len()).
    pub fn should_stop(&self, ctx: &ExecContext, current_stack_depth: usize) -> bool {
        let Some(plan) = self.by_thread.get(&ctx.thread_id) else { return false; };

        // Localisation courante
        let cur_path = ctx.source_path.as_deref();
        let cur_line = ctx.line;

        // Si pas de source/ligne dispo, on évite d'arrêter (on veut des lieux "visibles").
        if cur_path.is_none() || cur_line.is_none() {
            return false;
        }

        // Helper : a-t-on changé de (path,line) depuis le départ ?
        let moved_line = match (plan.start_path.as_deref(), plan.start_line, cur_path, cur_line) {
            (Some(sp), Some(sl), Some(cp), Some(cl)) => sp != cp || sl != cl,
            _ => false,
        };

        match plan.mode {
            StepMode::Into => {
                // S’arrête dès que l’on *quitte* la ligne de départ (ou le fichier).
                moved_line
            }
            StepMode::Over => {
                // On autorise des appels internes *sans* stopper (profondeur > start).
                // On s’arrête quand :
                //  - la profondeur revient à <= start_depth ET on a *effectivement* changé de ligne
                //  - OU quand on est déjà à profondeur <= start_depth et la ligne a bougé
                if current_stack_depth <= plan.start_depth {
                    moved_line
                } else {
                    // Toujours dans un appel : continue.
                    false
                }
            }
            StepMode::Out => {
                // On veut sortir de la fonction courante : s’arrêter dès que
                // la profondeur devient < start_depth (retour).
                current_stack_depth < plan.start_depth
            }
            StepMode::Continue => {
                // Pas un vrai step plan, par sécurité on ne stoppe jamais ici.
                false
            }
        }
    }
}

/* --------------------------------- Tests ---------------------------------- */

#[cfg(test)]
mod tests {
    use super::*;
    use crate::hook::ExecContext;

    fn frame(path: &str, line: u32) -> Frame {
        Frame { id: 1, thread_id: 1, name: "main".into(), source_path: path.into(), line, column: 1 }
    }
    fn ctx(thread: ThreadId, path: &str, line: u32, depth: usize) -> (ExecContext, usize) {
        (ExecContext {
            thread_id: thread,
            frame_id: Some(1),
            pc: 0,
            function: Some("main".into()),
            source_path: Some(path.into()),
            line: Some(line),
        }, depth)
    }

    #[test]
    fn step_into_stops_on_new_line() {
        let mut sc = StepController::default();
        sc.begin(1, StepMode::Into, &[frame("a.vitte", 10)]);
        // même ligne → pas d'arrêt
        let (c1, d1) = ctx(1, "a.vitte", 10, 1);
        assert!(!sc.should_stop(&c1, d1));
        // nouvelle ligne → stop
        let (c2, d2) = ctx(1, "a.vitte", 11, 1);
        assert!(sc.should_stop(&c2, d2));
    }

    #[test]
    fn step_over_ignores_deeper_until_back_same_or_shallower_and_moved_line() {
        let mut sc = StepController::default();
        sc.begin(1, StepMode::Over, &[frame("a.vitte", 5)]);
        // Appel interne : profondeur 2, même fichier/ligne → continue
        let (c_call, d_call) = ctx(1, "a.vitte", 5, 2);
        assert!(!sc.should_stop(&c_call, d_call));
        // Retour même profondeur de départ mais ligne a changé → stop
        let (c_back, d_back) = ctx(1, "a.vitte", 6, 1);
        assert!(sc.should_stop(&c_back, d_back));
    }

    #[test]
    fn step_out_stops_when_depth_decreases() {
        let mut sc = StepController::default();
        sc.begin(1, StepMode::Out, &[frame("a.vitte", 20), frame("callee.vitte", 1)]);
        // Encore dedans (profondeur identique) → continue
        let (c1, d1) = ctx(1, "callee.vitte", 2, 2);
        assert!(!sc.should_stop(&c1, d1));
        // Remonté (profondeur 1 < 2) → stop
        let (c2, d2) = ctx(1, "a.vitte", 21, 1);
        assert!(sc.should_stop(&c2, d2));
    }

    #[test]
    fn inactive_if_other_thread() {
        let mut sc = StepController::default();
        sc.begin(1, StepMode::Into, &[frame("a.vitte", 1)]);
        let (c, d) = ctx(2, "a.vitte", 2, 1);
        assert!(!sc.should_stop(&c, d));
    }
}
