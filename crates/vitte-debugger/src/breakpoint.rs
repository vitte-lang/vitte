//! breakpoint.rs — Gestion des points d'arrêt (files/lines, conditions, logpoints, etc.)
//!
//! - `BreakpointManager` : ajout/màj/clear/list, résolution par fichier/ligne.
//! - `BreakpointRequest` → côté client (DAP) ; `BreakpointResolved` → côté VM.
//! - Support : conditions (expr string), hitCount (N), logpoints, fonctionnels.
//! - Remapping de ligne via hook `LineMapper` (ex: source map / debug info).
//!
//! Thread-safety : non-Send par défaut (stockage simple). Enrobez dans Arc<Mutex<_>> côté appelant si besoin.

use std::collections::{HashMap, HashSet};
use std::path::{Path, PathBuf};

use serde::{Deserialize, Serialize};

pub type FileKey = String;     // chemin canonique (string)
pub type BreakpointId = u64;   // identifiant interne
pub type FuncName = String;
pub type ThreadId = i64;

/// Type de point d'arrêt
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub enum BreakpointKind {
    Line,       // fichier + ligne
    Function,   // nom de fonction
}

/// Requête (côté DAP/UI)
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct BreakpointRequest {
    pub kind: BreakpointKind,
    pub source_path: Option<String>, // requis si Line
    pub line: Option<u32>,           // requis si Line
    pub function: Option<FuncName>,  // requis si Function

    /// Expression booléenne évaluée côté VM (ex: "x > 3 && y == 0").
    pub condition: Option<String>,
    /// "Passer" les N premiers hits (dap: hitCondition peut aussi être ">=10", ici c’est N basique)
    pub hit_count: Option<u64>,
    /// Logpoint : n’arrête pas, log le message ; peut contenir {var}
    pub log_message: Option<String>,
    /// Activation
    pub enabled: bool,
    /// Contrainte de thread (optionnel)
    pub thread: Option<ThreadId>,
}

impl Default for BreakpointRequest {
    fn default() -> Self {
        Self {
            kind: BreakpointKind::Line,
            source_path: None,
            line: None,
            function: None,
            condition: None,
            hit_count: None,
            log_message: None,
            enabled: true,
            thread: None,
        }
    }
}

/// État résolu (côté VM)
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct BreakpointResolved {
    pub id: BreakpointId,
    pub kind: BreakpointKind,
    pub source_path: Option<FileKey>,
    pub line_effective: Option<u32>,
    pub function: Option<FuncName>,

    pub condition: Option<String>,
    pub hit_count: u64, // restant à consommer avant arrêt (0 = prêt)
    pub log_message: Option<String>,
    pub enabled: bool,
    pub thread: Option<ThreadId>,

    /// A été mappé (ligne transformée par LineMapper)
    pub mapped: bool,
    /// Vérifié (placé avec succès)
    pub verified: bool,
    /// Dernière ligne demandée (pour debug)
    pub requested_line: Option<u32>,
}

/// Hook de remapping (ex: source map : .vitte -> bytecode)
pub trait LineMapper {
    /// Retourne (ligne_effective, mapped?)
    fn map_line(&self, file: &FileKey, requested_line: u32) -> (u32, bool);
}

/// Implémentation no-op : identique.
pub struct NoMapper;
impl LineMapper for NoMapper {
    fn map_line(&self, _file: &FileKey, requested_line: u32) -> (u32, bool) {
        (requested_line, false)
    }
}

/// Gère les points d’arrêt par fichier et par fonction.
#[derive(Default)]
pub struct BreakpointManager<M: LineMapper = NoMapper> {
    next_id: BreakpointId,
    files: HashMap<FileKey, Vec<BreakpointResolved>>,
    funcs: HashMap<FuncName, Vec<BreakpointResolved>>,
    mapper: M,
}

impl<M: LineMapper> BreakpointManager<M> {
    pub fn with_mapper(mapper: M) -> Self {
        Self {
            next_id: 1,
            files: HashMap::new(),
            funcs: HashMap::new(),
            mapper,
        }
    }

    /// Canonicalise un chemin en clé stable.
    pub fn canonical_key<P: AsRef<Path>>(p: P) -> FileKey {
        // Ici on se contente de normaliser en String ; à toi d’ajouter
        // une vraie canonicalisation (std::fs::canonicalize) si nécessaire.
        p.as_ref().to_string_lossy().to_string()
    }

    /// Définit la *liste complète* des BPs pour un fichier (style DAP setBreakpoints).
    /// Retourne la liste résolue (verified, actual line).
    pub fn set_breakpoints_for_file<P: AsRef<Path>>(
        &mut self,
        path: P,
        requests: &[BreakpointRequest],
    ) -> Vec<BreakpointResolved> {
        let key = Self::canonical_key(path);
        // Purge et remplace
        let mut resolved = Vec::with_capacity(requests.len());
        for req in requests {
            if req.kind != BreakpointKind::Line {
                continue; // ignore dans cette API
            }
            let line_req = req.line.unwrap_or(1);
            let (line_eff, mapped) = self.mapper.map_line(&key, line_req);

            let bp = BreakpointResolved {
                id: self.alloc_id(),
                kind: BreakpointKind::Line,
                source_path: Some(key.clone()),
                line_effective: Some(line_eff),
                function: None,

                condition: req.condition.clone(),
                hit_count: req.hit_count.unwrap_or(0),
                log_message: req.log_message.clone(),
                enabled: req.enabled,
                thread: req.thread,

                mapped,
                verified: true,
                requested_line: Some(line_req),
            };
            resolved.push(bp);
        }
        self.files.insert(key, resolved.clone());
        resolved
    }

    /// Ajoute un breakpoint de fonction.
    pub fn add_function_breakpoint(&mut self, func: impl Into<FuncName>, req: &BreakpointRequest) -> BreakpointResolved {
        let fname = func.into();
        let bp = BreakpointResolved {
            id: self.alloc_id(),
            kind: BreakpointKind::Function,
            source_path: None,
            line_effective: None,
            function: Some(fname.clone()),

            condition: req.condition.clone(),
            hit_count: req.hit_count.unwrap_or(0),
            log_message: req.log_message.clone(),
            enabled: req.enabled,
            thread: req.thread,

            mapped: false,
            verified: true,
            requested_line: None,
        };
        self.funcs.entry(fname).or_default().push(bp.clone());
        bp
    }

    /// Active/Désactive un breakpoint par id.
    pub fn set_enabled(&mut self, id: BreakpointId, enabled: bool) -> bool {
        self.visit_mut(id, |bp| bp.enabled = enabled)
    }

    /// Supprime un breakpoint par id.
    pub fn remove(&mut self, id: BreakpointId) -> bool {
        let mut removed = false;
        for (_k, v) in self.files.iter_mut() {
            let before = v.len();
            v.retain(|bp| bp.id != id);
            removed |= v.len() != before;
        }
        for (_k, v) in self.funcs.iter_mut() {
            let before = v.len();
            v.retain(|bp| bp.id != id);
            removed |= v.len() != before;
        }
        removed
    }

    /// Liste tous les breakpoints.
    pub fn list_all(&self) -> Vec<BreakpointResolved> {
        let mut out = Vec::new();
        for v in self.files.values() {
            out.extend(v.iter().cloned());
        }
        for v in self.funcs.values() {
            out.extend(v.iter().cloned());
        }
        out
    }

    /// Retourne les BPs d’un fichier (effectifs).
    pub fn list_for_file<P: AsRef<Path>>(&self, path: P) -> Vec<BreakpointResolved> {
        let key = Self::canonical_key(path);
        self.files.get(&key).cloned().unwrap_or_default()
    }

    /// Retourne les BPs d’une fonction.
    pub fn list_for_function(&self, func: &str) -> Vec<BreakpointResolved> {
        self.funcs.get(func).cloned().unwrap_or_default()
    }

    /// Décide si on doit s’arrêter à (file, line, thread). Gère hit_count, condition et logpoints.
    /// - `eval_cond`: callback pour évaluer une condition string dans ton runtime (None = pas de cond).
    /// - Retour:
    ///     - `BreakAction::Stop(id)` → pause VM
    ///     - `BreakAction::Log(id, msg)` → logpoint (ne pas pauser)
    ///     - `BreakAction::None` → continuer
    pub fn should_break<F>(
        &mut self,
        file: &FileKey,
        line: u32,
        thread: Option<ThreadId>,
        mut eval_cond: F,
    ) -> BreakAction
    where
        F: FnMut(&str) -> bool,
    {
        let mut chosen: Option<BreakAction> = None;

        if let Some(list) = self.files.get_mut(file) {
            for bp in list.iter_mut().filter(|bp| bp.enabled) {
                if bp.line_effective == Some(line) && thread_match(bp.thread, thread) {
                    if let Some(act) = decide_action(bp, &mut eval_cond) {
                        chosen = Some(act);
                        break;
                    }
                }
            }
        }
        if chosen.is_some() {
            return chosen.unwrap();
        }

        // Function breakpoints ne sont pas résolus par (file,line) ; c’est au callsite de décider.
        BreakAction::None
    }

    /// À appeler lorsqu’on entre dans une fonction ; renvoie une action éventuelle.
    pub fn hit_function<F>(&mut self, func: &str, thread: Option<ThreadId>, mut eval_cond: F) -> BreakAction
    where
        F: FnMut(&str) -> bool,
    {
        if let Some(list) = self.funcs.get_mut(func) {
            for bp in list.iter_mut().filter(|bp| bp.enabled) {
                if thread_match(bp.thread, thread) {
                    if let Some(act) = decide_action(bp, &mut eval_cond) {
                        return act;
                    }
                }
            }
        }
        BreakAction::None
    }

    fn visit_mut<FN>(&mut self, id: BreakpointId, mut f: FN) -> bool
    where
        FN: FnMut(&mut BreakpointResolved),
    {
        for v in self.files.values_mut() {
            for bp in v.iter_mut() {
                if bp.id == id {
                    f(bp);
                    return true;
                }
            }
        }
        for v in self.funcs.values_mut() {
            for bp in v.iter_mut() {
                if bp.id == id {
                    f(bp);
                    return true;
                }
            }
        }
        false
    }

    fn alloc_id(&mut self) -> BreakpointId {
        let id = self.next_id;
        self.next_id += 1;
        id
    }
}

/// Résultat d’une décision à un site potentiel d’arrêt.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum BreakAction {
    None,
    Stop(BreakpointId),
    Log(BreakpointId, String),
}

fn thread_match(bp_thread: Option<ThreadId>, cur: Option<ThreadId>) -> bool {
    match (bp_thread, cur) {
        (None, _) => true,
        (Some(b), Some(c)) => b == c,
        (Some(_), None) => false,
    }
}

fn decide_action<F>(bp: &mut BreakpointResolved, eval_cond: &mut F) -> Option<BreakAction>
where
    F: FnMut(&str) -> bool,
{
    // Gérer hit_count (skip N premiers hits)
    if bp.hit_count > 0 {
        bp.hit_count -= 1;
        return None;
    }

    // Condition string (si fournie et fausse → pas d’arrêt)
    if let Some(cond) = &bp.condition {
        if !eval_cond(cond) {
            return None;
        }
    }

    // Logpoint ?
    if let Some(msg) = &bp.log_message {
        return Some(BreakAction::Log(bp.id, msg.clone()));
    }

    Some(BreakAction::Stop(bp.id))
}

#[cfg(test)]
mod tests {
    use super::*;

    struct PlusOneMapper;
    impl LineMapper for PlusOneMapper {
        fn map_line(&self, _file: &FileKey, requested_line: u32) -> (u32, bool) {
            (requested_line + 1, true)
        }
    }

    #[test]
    fn set_and_hit_line_bp() {
        let mut mgr = BreakpointManager::with_mapper(NoMapper);
        let reqs = vec![BreakpointRequest {
            source_path: Some("a.vitte".into()),
            line: Some(10),
            ..Default::default()
        }];
        let res = mgr.set_breakpoints_for_file("a.vitte", &reqs);
        assert_eq!(res.len(), 1);
        let key = BreakpointManager::<NoMapper>::canonical_key("a.vitte");
        assert!(matches!(mgr.should_break(&key, 10, None, |_c| true), BreakAction::Stop(_)));
    }

    #[test]
    fn mapping_applied() {
        let mut mgr = BreakpointManager::with_mapper(PlusOneMapper);
        let reqs = vec![BreakpointRequest {
            source_path: Some("a.vitte".into()),
            line: Some(4),
            ..Default::default()
        }];
        let res = mgr.set_breakpoints_for_file("a.vitte", &reqs);
        assert_eq!(res[0].line_effective, Some(5));
    }

    #[test]
    fn hitcount_and_condition_and_log() {
        let mut mgr = BreakpointManager::with_mapper(NoMapper);
        let reqs = vec![BreakpointRequest {
            source_path: Some("a.vitte".into()),
            line: Some(3),
            hit_count: Some(2),               // skip 2 hits
            condition: Some("x>0".into()),
            log_message: Some("here".into()), // logpoint → pas d’arrêt
            ..Default::default()
        }];
        let _ = mgr.set_breakpoints_for_file("a.vitte", &reqs);
        let key = BreakpointManager::<NoMapper>::canonical_key("a.vitte");

        // 1er hit : consume hit_count (2→1)
        assert!(matches!(mgr.should_break(&key, 3, None, |_c| true), BreakAction::None));
        // 2e hit : consume hit_count (1→0)
        assert!(matches!(mgr.should_break(&key, 3, None, |_c| true), BreakAction::None));
        // 3e hit : condition true → logpoint
        match mgr.should_break(&key, 3, None, |_c| true) {
            BreakAction::Log(_, msg) => assert_eq!(msg, "here"),
            other => panic!("expected Log, got {other:?}"),
        }
        // condition false → rien
        assert!(matches!(mgr.should_break(&key, 3, None, |_c| false), BreakAction::None));
    }

    #[test]
    fn function_bp_hit() {
        let mut mgr = BreakpointManager::with_mapper(NoMapper);
        let req = BreakpointRequest {
            kind: BreakpointKind::Function,
            function: Some("main".into()),
            ..Default::default()
        };
        let _bp = mgr.add_function_breakpoint("main", &req);
        assert!(matches!(mgr.hit_function("main", None, |_c| true), BreakAction::Stop(_)));
        assert!(matches!(mgr.hit_function("other", None, |_c| true), BreakAction::None));
    }

    #[test]
    fn enable_disable_remove() {
        let mut mgr = BreakpointManager::with_mapper(NoMapper);
        let reqs = vec![BreakpointRequest {
            source_path: Some("a.vitte".into()),
            line: Some(2),
            ..Default::default()
        }];
        let res = mgr.set_breakpoints_for_file("a.vitte", &reqs);
        let id = res[0].id;

        assert!(mgr.set_enabled(id, false));
        let key = BreakpointManager::<NoMapper>::canonical_key("a.vitte");
        assert!(matches!(mgr.should_break(&key, 2, None, |_c| true), BreakAction::None));

        assert!(mgr.set_enabled(id, true));
        assert!(matches!(mgr.should_break(&key, 2, None, |_c| true), BreakAction::Stop(_)));

        assert!(mgr.remove(id));
        assert!(matches!(mgr.should_break(&key, 2, None, |_c| true), BreakAction::None));
    }
}
