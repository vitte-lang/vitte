//! state.rs — État interne du débogueur (threads, frames, variables, steps)
//!
//! Rôle :
//! - Conserver l’instantané côté adaptateur (pas la vérité VM !)
//! - Mapper `variablesReference` → listes de `Variable` (DAP-friendly)
//! - Garder la raison du dernier arrêt et le mode de step demandé
//! - Gérer les threads, leurs frames, et les scopes exposés
//!
//! Intégration typique :
//!   - À chaque pause VM → `set_stop_reason(...)`, `set_thread_frames(...)`, `install_scope(...)`
//!   - Pour les variables DAP → `vars_for(varref)`
//!   - Pour le stepping → `set_step_mode(...)`, `step_mode()`, `clear_step_mode()`
//!
//! Important : l’état ici est *miroir* côté adaptateur. La source de vérité
//! reste ta VM. Nettoie/rafraîchis cet état à chaque stop pour éviter le stale.

use std::collections::HashMap;
use std::time::{Duration, SystemTime};

use color_eyre::eyre::Result;
use serde::{Deserialize, Serialize};

use crate::api::{Frame, Variable, VarRef, ThreadId, FrameId, Scope};

/// État d’un thread (côté adaptateur)
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum ThreadState {
    Running,
    Paused,
    Exited(i32),
}

/// Raison d’arrêt (dernière pause observée)
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub enum StopReason {
    Breakpoint { source_path: String, line: u32 },
    Step,
    Exception { message: String, source_path: Option<String>, line: Option<u32> },
    PauseRequested,
    Unknown,
}

/// Mode de step demandé par le client
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum StepMode {
    Continue,
    Over,
    Into,
    Out,
}

/// Un scope installé dans la table de variables
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct InstalledScope {
    pub scope: Scope,
    /// Cache “plat” des variables de ce scope (résolues côté VM à l’instant T)
    pub vars: Vec<Variable>,
    /// Heure d’installation (utile si tu veux expirer)
    pub installed_at: SystemTime,
}

/// Infos par thread
#[derive(Debug, Clone, Serialize, Deserialize, Default)]
pub struct ThreadInfo {
    pub id: ThreadId,
    pub name: Option<String>,
    pub state: ThreadState,
    /// Frames du dernier stop (frame[0] = top)
    pub frames: Vec<Frame>,
    /// Frame courant (index dans `frames`)
    pub current_frame_ix: usize,
    /// varref → variables (scopes/objets)
    pub var_tables: HashMap<VarRef, Vec<Variable>>,
    /// varref alloués mais vides (réservations)
    pub var_reserved: Vec<VarRef>,
    /// Scopes installés lors du dernier stop
    pub scopes: Vec<InstalledScope>,
    /// Dernière raison d’arrêt
    pub last_stop: Option<StopReason>,
    /// Mode de step
    pub step_mode: Option<StepMode>,
}

impl ThreadInfo {
    fn new(id: ThreadId) -> Self {
        Self {
            id,
            ..Default::default()
        }
    }

    pub fn current_frame(&self) -> Option<&Frame> {
        self.frames.get(self.current_frame_ix)
    }

    pub fn set_frames(&mut self, frames: Vec<Frame>) {
        self.frames = frames;
        self.current_frame_ix = 0.min(self.frames.len().saturating_sub(1));
    }

    pub fn set_state(&mut self, st: ThreadState) {
        self.state = st;
    }

    pub fn set_stop_reason(&mut self, r: StopReason) {
        self.last_stop = Some(r);
        self.state = ThreadState::Paused;
    }

    pub fn clear_stop_reason(&mut self) {
        self.last_stop = None;
    }

    pub fn set_step(&mut self, m: StepMode) {
        self.step_mode = Some(m);
    }

    pub fn clear_step(&mut self) {
        self.step_mode = None;
    }

    pub fn install_scope(&mut self, scope: Scope, vars: Vec<Variable>) {
        let vr = scope.variables_reference;
        self.var_tables.insert(vr, vars.clone());
        self.scopes.push(InstalledScope {
            scope,
            vars,
            installed_at: SystemTime::now(),
        });
    }

    pub fn vars_for(&self, varref: VarRef) -> Option<&[Variable]> {
        self.var_tables.get(&varref).map(|v| v.as_slice())
    }

    pub fn put_var_table(&mut self, varref: VarRef, vars: Vec<Variable>) {
        self.var_tables.insert(varref, vars);
    }

    pub fn reserve_varref(&mut self, varref: VarRef) {
        self.var_reserved.push(varref);
    }

    pub fn clear_vars(&mut self) {
        self.var_tables.clear();
        self.var_reserved.clear();
        self.scopes.clear();
    }
}

/// État global du débogueur (tous threads)
#[derive(Debug, Default, Serialize, Deserialize)]
pub struct DebugState {
    /// thread_id → info
    threads: HashMap<ThreadId, ThreadInfo>,
    /// Compteur global pour fabriquer des variablesReference uniques (≥ 2)
    next_varref: VarRef,
    /// Watch expressions (persistantes côté client)
    watches: Vec<String>,
}

impl DebugState {
    pub fn new() -> Self {
        Self { next_varref: 2, ..Default::default() }
    }

    /* ----------------------------- Threads -------------------------------- */

    pub fn ensure_thread(&mut self, tid: ThreadId) -> &mut ThreadInfo {
        self.threads.entry(tid).or_insert_with(|| ThreadInfo::new(tid))
    }

    pub fn remove_thread(&mut self, tid: ThreadId) {
        self.threads.remove(&tid);
    }

    pub fn thread(&self, tid: ThreadId) -> Option<&ThreadInfo> {
        self.threads.get(&tid)
    }

    pub fn thread_mut(&mut self, tid: ThreadId) -> Option<&mut ThreadInfo> {
        self.threads.get_mut(&tid)
    }

    pub fn list_threads(&self) -> impl Iterator<Item=(&ThreadId, &ThreadInfo)> {
        self.threads.iter()
    }

    /* ----------------------------- Frames --------------------------------- */

    /// Remplace l’ensemble des frames d’un thread au moment d’un stop
    pub fn set_thread_frames(&mut self, tid: ThreadId, frames: Vec<Frame>) {
        let t = self.ensure_thread(tid);
        t.set_frames(frames);
        t.set_state(ThreadState::Paused);
    }

    /// Choisir le frame courant (par index)
    pub fn select_frame(&mut self, tid: ThreadId, ix: usize) -> bool {
        if let Some(t) = self.threads.get_mut(&tid) {
            if ix < t.frames.len() {
                t.current_frame_ix = ix;
                return true;
            }
        }
        false
    }

    /* ------------------------- Variables / Scopes ------------------------- */

    /// Alloue un nouveau variablesReference unique
    pub fn alloc_varref(&mut self) -> VarRef {
        let vr = self.next_varref.max(2);
        self.next_varref = vr + 1;
        vr
    }

    /// Installe un scope (variablesReference + snapshot de variables)
    pub fn install_scope(&mut self, tid: ThreadId, scope: Scope, vars: Vec<Variable>) {
        let t = self.ensure_thread(tid);
        t.install_scope(scope, vars);
    }

    /// Associe un tableau de variables à un varref
    pub fn put_var_table(&mut self, tid: ThreadId, varref: VarRef, vars: Vec<Variable>) {
        let t = self.ensure_thread(tid);
        t.put_var_table(varref, vars);
    }

    /// Accède aux variables d’un varref (read-only)
    pub fn vars_for(&self, tid: ThreadId, varref: VarRef) -> Option<&[Variable]> {
        self.threads.get(&tid).and_then(|t| t.vars_for(varref))
    }

    /// Réserve un varref (utile si tu veux présenter un scope vide d’abord)
    pub fn reserve_varref(&mut self, tid: ThreadId, varref: VarRef) {
        let t = self.ensure_thread(tid);
        t.reserve_varref(varref);
    }

    /// Nettoyage complet des tables de variables d’un thread (en général à chaque stop)
    pub fn clear_thread_vars(&mut self, tid: ThreadId) {
        if let Some(t) = self.threads.get_mut(&tid) {
            t.clear_vars();
        }
    }

    /* --------------------------- Stop & Stepping -------------------------- */

    pub fn set_stop_reason(&mut self, tid: ThreadId, r: StopReason) {
        let t = self.ensure_thread(tid);
        t.set_stop_reason(r);
    }

    pub fn clear_stop_reason(&mut self, tid: ThreadId) {
        if let Some(t) = self.threads.get_mut(&tid) {
            t.clear_stop_reason();
        }
    }

    pub fn set_step_mode(&mut self, tid: ThreadId, m: StepMode) {
        let t = self.ensure_thread(tid);
        t.set_step(m);
    }

    pub fn clear_step_mode(&mut self, tid: ThreadId) {
        if let Some(t) = self.threads.get_mut(&tid) {
            t.clear_step();
        }
    }

    pub fn step_mode(&self, tid: ThreadId) -> Option<StepMode> {
        self.threads.get(&tid).and_then(|t| t.step_mode)
    }

    pub fn set_thread_state(&mut self, tid: ThreadId, st: ThreadState) {
        let t = self.ensure_thread(tid);
        t.set_state(st);
    }

    /* ----------------------------- Watches -------------------------------- */

    pub fn set_watches(&mut self, list: Vec<String>) { self.watches = list; }
    pub fn watches(&self) -> &[String] { &self.watches }

    /* ----------------------------- Utilitaires ---------------------------- */

    /// Reset “doux” d’un thread lors d’un nouveau stop (frames & vars)
    pub fn refresh_for_stop(&mut self, tid: ThreadId, frames: Vec<Frame>, reason: StopReason) {
        self.set_thread_frames(tid, frames);
        self.clear_thread_vars(tid);
        self.set_stop_reason(tid, reason);
    }
}

/* --------------------------------- Tests ---------------------------------- */

#[cfg(test)]
mod tests {
    use super::*;
    use crate::api::{Frame, Scope, Variable};

    fn dummy_frame(id: i64, line: u32) -> Frame {
        Frame { id, thread_id: 1, name: "main".into(), source_path: "x.vitte".into(), line, column: 1 }
    }
    fn var(name: &str, val: &str) -> Variable {
        Variable { name: name.into(), value: val.into(), r#type: None, variables_reference: 0 }
    }

    #[test]
    fn alloc_and_vars() {
        let mut st = DebugState::new();
        let tid = 1;
        let vr = st.alloc_varref();
        assert!(vr >= 2);
        st.put_var_table(tid, vr, vec![var("x","42")]);
        let got = st.vars_for(tid, vr).unwrap();
        assert_eq!(got[0].name, "x");
    }

    #[test]
    fn frames_and_stop() {
        let mut st = DebugState::new();
        let tid = 1;
        st.refresh_for_stop(tid, vec![dummy_frame(10, 7)], StopReason::Step);
        let th = st.thread(tid).unwrap();
        assert!(matches!(th.state, ThreadState::Paused));
        assert!(matches!(th.last_stop, Some(StopReason::Step)));
        assert_eq!(th.frames[0].line, 7);
    }

    #[test]
    fn scopes_install() {
        let mut st = DebugState::new();
        let tid = 1;
        let vr = st.alloc_varref();
        let sc = Scope { name: "Locals".into(), variables_reference: vr, expensive: false };
        st.install_scope(tid, sc, vec![var("a","1"), var("b","2")]);
        let vs = st.vars_for(tid, vr).unwrap();
        assert_eq!(vs.len(), 2);
    }

    #[test]
    fn step_modes() {
        let mut st = DebugState::new();
        let tid = 1;
        st.set_step_mode(tid, StepMode::Over);
        assert!(matches!(st.step_mode(tid), Some(StepMode::Over)));
        st.clear_step_mode(tid);
        assert!(st.step_mode(tid).is_none());
    }
}
