//! hook.rs — Points d'extension (hooks) entre la VM Vitte et le debugger
//!
//! Objectifs :
//! - Fournir un contrat **stable** pour :
//!   * Remapper chemins/numéros de ligne (source maps, inlining, macro-expansion)
//!   * Décider d’un arrêt (break/step/exception) *in situ*
//!   * Offrir un environnement d’évaluation (conditions DAP, logpoints)
//!   * Pretty-printers (affichage humain des structures VM)
//!   * Observer les événements runtime (loaded, continued, stopped, output…)
//!
//! - Proposer un **HookRegistry** multicast : plusieurs hooks peuvent coexister
//!   (p.ex. un hook “source-map”, un hook “coverage”, un hook “telemetry”).
//!
//! Intégration :
//!   - La VM appelle HookRegistry aux sites critiques (load, avant/pendant exécution).
//!   - Le debugger peut fournir des implémentations (ex: `EvalEnv` depuis frame courant).
//!
//! Dépendances internes : réutilise `EvalEnv` (eval.rs) et `LineMapper` (breakpoint.rs).

use std::sync::Arc;

use color_eyre::eyre::{Result, eyre};
use serde::{Deserialize, Serialize};

use crate::eval::EvalEnv;
use crate::breakpoint::LineMapper;

/* ------------------------------ Types utiles ------------------------------ */

pub type ThreadId = i64;
pub type FrameId = i64;
pub type Pc = u64;

/// Contexte minimal au site d’exécution courant (pour prise de décision).
#[derive(Debug, Clone, Serialize, Deserialize, Default)]
pub struct ExecContext {
    pub thread_id: ThreadId,
    pub frame_id: Option<FrameId>,
    pub pc: Pc,                       // program counter / offset bytecode
    pub function: Option<String>,     // nom logique si dispo
    pub source_path: Option<String>,  // chemin “logique” (après mapping si souhaité)
    pub line: Option<u32>,            // ligne “logique” (idem)
}

/// Décision proposée par un hook avant l’exécution d’une instruction.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum StopDecision {
    /// Ne force rien (laisser les autres hooks décider ou la VM trancher).
    None,
    /// Logpoint : log sans pause.
    Log(String),
    /// Stop : demander une pause (break) au debugger.
    Stop,
}

impl Default for StopDecision {
    fn default() -> Self { StopDecision::None }
}

/// Événements observables par les hooks.
#[derive(Debug, Clone)]
pub enum RuntimeEvent {
    ProgramLoaded { path: String },
    Continued { thread_id: Option<ThreadId> },
    StoppedBreakpoint { thread_id: ThreadId, source_path: String, line: u32 },
    StoppedStep { thread_id: ThreadId },
    Exception { message: String, source_path: Option<String>, line: Option<u32> },
    Output { category: String, text: String },
    Terminated { exit_code: i32 },
}

/* ------------------------------- Traits Hook ------------------------------ */

/// 1) Mapping de chemin/ligne (souvent alimenté par des debug infos).
///    Étend `LineMapper` (breakpoint.rs) pour inclure la translation de chemin.
pub trait SourceMapper: Send + Sync + LineMapper {
    /// Remappe un chemin “physique” VM → chemin “logique” utilisateur.
    /// Par défaut, identique (no-op).
    fn map_path(&self, raw_path: &str) -> String {
        raw_path.to_string()
    }
}

/// 2) Fournisseur d’environnement d’évaluation (pour conditions/logpoints).
///    La VM ou le debugger peut exposer le frame courant via un `EvalEnv`.
pub trait EvalProvider: Send + Sync {
    /// Retourne un `EvalEnv` pour (thread, frame) si disponible.
    fn env_for(&self, _thread: ThreadId, _frame: Option<FrameId>) -> Option<Arc<dyn EvalEnv>>;
}

/// 3) Décideur au site d’exécution (juste avant une instruction).
///    Permet d’implémenter des politiques custom (watchpoints, filtres, etc.).
pub trait ExecutionDecider: Send + Sync {
    /// Laisse la possibilité d’examiner le contexte pour décider.
    /// Peut s’appuyer sur `EvalProvider` via le `HookRegistry` (voir plus bas).
    fn before_instruction(&self, _ctx: &ExecContext, _registry: &HookRegistry) -> StopDecision {
        StopDecision::None
    }
}

/// 4) Pretty-printers : transforme une valeur VM → chaîne riche.
///    L’impl peut ignorer (None) si elle ne sait pas formater ce type.
pub trait PrettyPrinter: Send + Sync {
    fn pretty_print(&self, _type_name: &str, _bytes: &[u8]) -> Option<String> {
        None
    }
}

/// 5) Observateur d’événements runtime (télémetry, traces, plugins).
pub trait RuntimeObserver: Send + Sync {
    fn on_event(&self, _ev: &RuntimeEvent) {}
}

/* ------------------------------ NOP defaults ------------------------------ */

/// Implémentations no-op pratiques.
pub struct NoSourceMapper;
impl LineMapper for NoSourceMapper {
    fn map_line(&self, _file: &String, requested_line: u32) -> (u32, bool) { (requested_line, false) }
}
impl SourceMapper for NoSourceMapper {}

pub struct NoEval;
impl EvalProvider for NoEval {
    fn env_for(&self, _thread: ThreadId, _frame: Option<FrameId>) -> Option<Arc<dyn EvalEnv>> { None }
}

pub struct NoDecider;
impl ExecutionDecider for NoDecider {}

pub struct NoPretty;
impl PrettyPrinter for NoPretty {}

pub struct NoObserver;
impl RuntimeObserver for NoObserver {}

/* ------------------------------ HookRegistry ------------------------------ */

/// Registre multicast de hooks.  
/// Il peut agréger plusieurs implémentations pour chaque catégorie.
#[derive(Default)]
pub struct HookRegistry {
    mappers:     Vec<Arc<dyn SourceMapper>>,
    evals:       Vec<Arc<dyn EvalProvider>>,
    deciders:    Vec<Arc<dyn ExecutionDecider>>,
    printers:    Vec<Arc<dyn PrettyPrinter>>,
    observers:   Vec<Arc<dyn RuntimeObserver>>,
}

impl HookRegistry {
    pub fn new() -> Self { Self::default() }

    /* -- enregistrement -- */
    pub fn add_mapper(&mut self, h: Arc<dyn SourceMapper>) { self.mappers.push(h); }
    pub fn add_eval_provider(&mut self, h: Arc<dyn EvalProvider>) { self.evals.push(h); }
    pub fn add_decider(&mut self, h: Arc<dyn ExecutionDecider>) { self.deciders.push(h); }
    pub fn add_printer(&mut self, h: Arc<dyn PrettyPrinter>) { self.printers.push(h); }
    pub fn add_observer(&mut self, h: Arc<dyn RuntimeObserver>) { self.observers.push(h); }

    /* -- mapping chemin/ligne -- */

    /// Applique en chaîne les mappeurs ; le dernier a le dernier mot.
    pub fn map_path(&self, raw_path: &str) -> String {
        let mut p = raw_path.to_string();
        for m in &self.mappers {
            p = m.map_path(&p);
        }
        p
    }

    /// Applique en chaîne le remapping de ligne (fichier mappé attendu).
    /// Retourne (ligne_effective, a_été_mappé_par_au_moins_un).
    pub fn map_line(&self, file_key: &str, requested_line: u32) -> (u32, bool) {
        let mut line = requested_line;
        let mut any = false;
        let key = file_key.to_string();
        for m in &self.mappers {
            let (l2, mapped) = m.map_line(&key, line);
            line = l2;
            any |= mapped;
        }
        (line, any)
    }

    /* -- environnement d’évaluation -- */

    /// Récupère le premier `EvalEnv` disponible (priorité à l’ordre d’enregistrement).
    pub fn eval_env(&self, thread: ThreadId, frame: Option<FrameId>) -> Option<Arc<dyn EvalEnv>> {
        for e in &self.evals {
            if let Some(env) = e.env_for(thread, frame) {
                return Some(env);
            }
        }
        None
    }

    /* -- décision d’arrêt -- */

    /// Interroge tous les déciders ; première décision non-None l’emporte.
    pub fn decide_before_instruction(&self, ctx: &ExecContext) -> StopDecision {
        for d in &self.deciders {
            match d.before_instruction(ctx, self) {
                StopDecision::None => continue,
                other => return other,
            }
        }
        StopDecision::None
    }

    /* -- pretty print -- */

    /// Interroge les pretty-printers ; le premier qui sait formater gagne.
    pub fn pretty(&self, type_name: &str, bytes: &[u8]) -> Option<String> {
        for p in &self.printers {
            if let Some(s) = p.pretty_print(type_name, bytes) {
                return Some(s);
            }
        }
        None
    }

    /* -- events -- */
    pub fn emit(&self, ev: RuntimeEvent) {
        for o in &self.observers {
            o.on_event(&ev);
        }
    }
}

/* ------------------------ Helpers : Hooks de confort ----------------------- */

/// Hook combiné très utile côté VM :
/// - wrappe un `SourceMapper` + `EvalProvider` pour fabriquer un `ExecContext`
///   cohérent (chemin/ligne mappés) et décider via `ExecutionDecider`.
pub struct DefaultVmHook {
    pub hooks: Arc<HookRegistry>,
}

impl DefaultVmHook {
    /// Exemple d’usage dans la VM, juste avant d’exécuter une instruction :
    /// ```
    /// // VM-side (pseudo):
    /// // let ctx = ExecContext{ thread_id, frame_id: Some(fid), pc, function: Some(func.to_string()),
    /// //                        source_path: Some(hooks.map_path(raw_path)),
    /// //                        line: Some(mapped_line) };
    /// // match default_vm_hook.decide(&ctx) { ... }
    /// ```
    pub fn decide(&self, ctx: &ExecContext) -> StopDecision {
        self.hooks.decide_before_instruction(ctx)
    }

    /// Obtenir un EvalEnv pour conditions/logpoints.
    pub fn env(&self, thread: ThreadId, frame: Option<FrameId>) -> Option<Arc<dyn EvalEnv>> {
        self.hooks.eval_env(thread, frame)
    }

    /// Mapping utilitaires (VM → logique).
    pub fn map_source(&self, raw_path: &str, requested_line: u32) -> (String, u32, bool) {
        let p = self.hooks.map_path(raw_path);
        let (line, mapped) = self.hooks.map_line(&p, requested_line);
        (p, line, mapped)
    }
}

/* --------------------------------- Tests ---------------------------------- */

#[cfg(test)]
mod tests {
    use super::*;
    use std::sync::Mutex;
    use crate::breakpoint::LineMapper;
    use crate::eval::{EvalEnv, Value};

    struct PlusTenMapper;
    impl LineMapper for PlusTenMapper {
        fn map_line(&self, _file: &String, requested_line: u32) -> (u32, bool) { (requested_line + 10, true) }
    }
    impl SourceMapper for PlusTenMapper {
        fn map_path(&self, raw: &str) -> String { format!("MAPPED::{raw}") }
    }

    struct SimpleEnv;
    impl EvalEnv for SimpleEnv {
        fn get_var(&self, name: &str) -> Option<Value> {
            if name == "x" { Some(Value::Int(42)) } else { None }
        }
    }
    struct SimpleEvalProvider;
    impl EvalProvider for SimpleEvalProvider {
        fn env_for(&self, _t: ThreadId, _f: Option<FrameId>) -> Option<Arc<dyn EvalEnv>> {
            Some(Arc::new(SimpleEnv))
        }
    }

    struct StepEveryN { n: u32, counter: Mutex<u32> }
    impl ExecutionDecider for StepEveryN {
        fn before_instruction(&self, _ctx: &ExecContext, _r: &HookRegistry) -> StopDecision {
            let mut c = self.counter.lock().unwrap();
            *c += 1;
            if *c % self.n == 0 { StopDecision::Stop } else { StopDecision::None }
        }
    }

    #[test]
    fn mapping_chain_and_decide() {
        let mut reg = HookRegistry::new();
        reg.add_mapper(Arc::new(PlusTenMapper));
        reg.add_eval_provider(Arc::new(SimpleEvalProvider));
        reg.add_decider(Arc::new(StepEveryN{ n: 3, counter: Mutex::new(0) }));

        let (p, l, mapped) = (reg.map_path("a.vitte"), reg.map_line("a.vitte", 5).0, reg.map_line("a.vitte", 5).1);
        assert!(p.starts_with("MAPPED::"));
        assert_eq!(l, 15);
        assert!(mapped);

        let ctx = ExecContext {
            thread_id: 1,
            frame_id: Some(10),
            pc: 99,
            function: Some("main".into()),
            source_path: Some(p),
            line: Some(l),
        };
        assert_eq!(reg.decide_before_instruction(&ctx), StopDecision::None);
        assert_eq!(reg.decide_before_instruction(&ctx), StopDecision::None);
        assert_eq!(reg.decide_before_instruction(&ctx), StopDecision::Stop);
    }

    #[test]
    fn pretty_and_events() {
        struct HexPrinter;
        impl PrettyPrinter for HexPrinter {
            fn pretty_print(&self, ty: &str, bytes: &[u8]) -> Option<String> {
                if ty == "u32" && bytes.len() == 4 {
                    let v = u32::from_le_bytes(bytes.try_into().unwrap());
                    return Some(format!("0x{v:08x}"));
                }
                None
            }
        }
        struct CounterObs(std::sync::Mutex<usize>);
        impl RuntimeObserver for CounterObs {
            fn on_event(&self, _ev: &RuntimeEvent) {
                *self.0.lock().unwrap() += 1;
            }
        }

        let mut reg = HookRegistry::new();
        reg.add_printer(Arc::new(HexPrinter));
        let obs = Arc::new(CounterObs(std::sync::Mutex::new(0)));
        reg.add_observer(obs.clone());

        assert_eq!(reg.pretty("u32", &1234u32.to_le_bytes()).unwrap(), format!("0x{:08x}", 1234));
        reg.emit(RuntimeEvent::Output{ category:"log".into(), text:"hello".into() });
        assert_eq!(*obs.0.lock().unwrap(), 1);
    }
}
