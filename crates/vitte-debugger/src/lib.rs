//! vitte-debugger — façade publique du sous-système de débogage Vitte.
//!
//! Ce crate regroupe :
//! - l'API haut-niveau [`Debugger`] pour piloter une VM Vitte depuis un outil externe,
//! - les traits/structures partagés avec l'adaptateur DAP (`vitte-dap`),
//! - la gestion des breakpoints, du stepping et des hooks de remapping.
//!
//! Les modules internes restent accessibles pour une intégration fine ; le module
//! [`prelude`] expose un import unique couvrant les types essentiels.

#![forbid(unsafe_code)]

pub mod api;
pub mod breakpoint;
pub mod eval;
pub mod hook;
pub mod state;
pub mod step;

pub use api::{
    Breakpoint, DebugEvent, Debugger, Frame, FrameId, Scope, StopReason, ThreadId, VarRef,
    Variable, VitteVm,
};
pub use breakpoint::{
    BreakpointId, BreakpointKind, BreakpointManager, BreakpointRequest, BreakpointResolved,
    FileKey, FuncName, LineMapper, NoMapper,
};
pub use eval::{EvalEnv, Number, Value, format_log_message};
pub use hook::{
    DefaultVmHook, EvalProvider, ExecContext, ExecutionDecider, HookRegistry, NoDecider, NoEval,
    NoObserver, NoPretty, NoSourceMapper, PrettyPrinter, RuntimeEvent, RuntimeObserver,
    SourceMapper, StopDecision,
};
pub use state::{DebugState, StepMode};
pub use step::{StepController, StepPlan};

/// Prélu pratique pour applications outillées (`use vitte_debugger::prelude::*;`).
pub mod prelude {
    pub use crate::api::{
        Breakpoint, DebugEvent, Debugger, Frame, FrameId, Scope, StopReason, ThreadId, VarRef,
        Variable, VitteVm,
    };
    pub use crate::breakpoint::{
        BreakpointId, BreakpointKind, BreakpointManager, BreakpointRequest, BreakpointResolved,
        FileKey, FuncName, LineMapper, NoMapper,
    };
    pub use crate::eval::{EvalEnv, Number, Value, format_log_message};
    pub use crate::hook::{
        DefaultVmHook, EvalProvider, ExecContext, ExecutionDecider, HookRegistry, NoDecider,
        NoEval, NoObserver, NoPretty, NoSourceMapper, PrettyPrinter, RuntimeEvent, RuntimeObserver,
        SourceMapper, StopDecision,
    };
    pub use crate::state::{DebugState, StepMode};
    pub use crate::step::{StepController, StepPlan};
}

/// Retourne `true` si le crate est bien intégré (utile pour tests de présence).
pub fn is_available() -> bool {
    true
}

#[cfg(test)]
mod tests {
    use crate::prelude::*;

    #[test]
    fn prelude_exports_core_types() {
        let _ = StopReason::None;
        let _ = StepMode::Into;
        assert!(crate::is_available());
    }
}
