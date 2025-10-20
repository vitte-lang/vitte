use std::collections::HashSet;
use std::fs;
use std::path::Path;
use std::sync::{Arc, Mutex};

use color_eyre::eyre::Result;
use crossbeam_channel::Receiver;
use vitte_debugger::prelude::{
    Breakpoint, DebugEvent, Debugger, Frame, FrameId, Scope, StopReason, ThreadId, VarRef,
    Variable, VitteVm,
};

/// Événements émis par le moteur de debug.
#[derive(Debug, Clone)]
pub enum EngineEvent {
    Initialized,
    Continued { thread_id: Option<i64> },
    StoppedBreakpoint { thread_id: i64, source_path: String, line: u32 },
    StoppedStep { thread_id: i64 },
    Exception { message: String, source_path: Option<String>, line: Option<u32> },
    Output { category: String, text: String },
    Terminated { exit_code: i32 },
}

/// Trait attendu par le `Handler` DAP.
pub trait DebugEngine: Send {
    fn launch(&mut self, program: &str, args: &[String]) -> Result<()>;
    fn set_breakpoints(&mut self, source: &str, lines: &[u32]) -> Result<Vec<u32>>;
    fn r#continue(&mut self) -> Result<()>;
    fn step_over(&mut self) -> Result<()>;
    fn stack_trace(&self) -> Result<Vec<StackFrame>>;
    fn variables(&self, variables_ref: i64) -> Result<Vec<VariableEntry>>;
    fn evaluate(&mut self, expr: &str) -> Result<Option<String>> {
        let _ = expr;
        Ok(None)
    }
    fn disconnect(&mut self) -> Result<()>;
    fn drain_events(&mut self) -> Result<Vec<EngineEvent>> {
        Ok(Vec::new())
    }
}

/// Implémentation par défaut : s'appuie sur `vitte-debugger` et un VM jouet.
pub struct DebuggerEngine {
    debugger: Debugger,
    events: Receiver<DebugEvent>,
}

/// Représentation simplifiée d'un frame pour le protocole DAP.
#[derive(Debug, Clone)]
pub struct StackFrame {
    pub id: i64,
    pub name: String,
    pub source_path: String,
    pub line: u32,
    pub column: u32,
}

/// Variable accessible via DAP.
#[derive(Debug, Clone)]
pub struct VariableEntry {
    pub name: String,
    pub value: String,
    pub r#type: Option<String>,
    pub variables_reference: i64,
}

impl DebuggerEngine {
    pub fn new() -> Self {
        let vm_state = Arc::new(Mutex::new(ScriptState::default()));
        let vm = ScriptVm { state: vm_state };
        let (debugger, events) = Debugger::new(Box::new(vm));
        Self { debugger, events }
    }

    fn convert_events(evts: Vec<DebugEvent>) -> Vec<EngineEvent> {
        evts.into_iter()
            .map(|ev| match ev {
                DebugEvent::Initialized => EngineEvent::Initialized,
                DebugEvent::Continued { thread_id } => {
                    EngineEvent::Continued { thread_id: thread_id.map(|t| t as i64) }
                },
                DebugEvent::StoppedBreakpoint { thread_id, source_path, line } => {
                    EngineEvent::StoppedBreakpoint {
                        thread_id: thread_id as i64,
                        source_path,
                        line,
                    }
                },
                DebugEvent::StoppedStep { thread_id } => {
                    EngineEvent::StoppedStep { thread_id: thread_id as i64 }
                },
                DebugEvent::Exception { message, source_path, line } => {
                    EngineEvent::Exception { message, source_path, line }
                },
                DebugEvent::Terminated { exit_code } => EngineEvent::Terminated { exit_code },
                DebugEvent::Output { category, text } => EngineEvent::Output { category, text },
            })
            .collect()
    }
}

impl DebugEngine for DebuggerEngine {
    fn launch(&mut self, program: &str, args: &[String]) -> Result<()> {
        self.debugger.launch(program, args)
    }

    fn set_breakpoints(&mut self, source: &str, lines: &[u32]) -> Result<Vec<u32>> {
        let bps = self.debugger.set_breakpoints(source, lines)?;
        Ok(bps.into_iter().map(|bp| bp.actual_line).filter(|l| *l > 0).collect())
    }

    fn r#continue(&mut self) -> Result<()> {
        let _ = self.debugger.continue_all()?;
        Ok(())
    }

    fn step_over(&mut self) -> Result<()> {
        let _ = self.debugger.step_over()?;
        Ok(())
    }

    fn stack_trace(&self) -> Result<Vec<StackFrame>> {
        let frames = self.debugger.stack_trace(None)?;
        Ok(frames
            .into_iter()
            .map(|f| StackFrame {
                id: f.id,
                name: f.name,
                source_path: f.source_path,
                line: f.line,
                column: f.column,
            })
            .collect())
    }

    fn variables(&self, variables_ref: i64) -> Result<Vec<VariableEntry>> {
        let vars = self.debugger.variables(variables_ref as VarRef)?;
        Ok(vars
            .into_iter()
            .map(|v| VariableEntry {
                name: v.name,
                value: v.value,
                r#type: v.r#type,
                variables_reference: v.variables_reference,
            })
            .collect())
    }

    fn evaluate(&mut self, expr: &str) -> Result<Option<String>> {
        self.debugger.evaluate(expr)
    }

    fn disconnect(&mut self) -> Result<()> {
        self.debugger.disconnect()
    }

    fn drain_events(&mut self) -> Result<Vec<EngineEvent>> {
        let mut collected = Vec::new();
        while let Ok(ev) = self.events.try_recv() {
            collected.push(ev);
        }
        Ok(Self::convert_events(collected))
    }
}

/* -------------------------------------------------------------------------- */
/*                              VM de démonstration                           */
/* -------------------------------------------------------------------------- */

#[derive(Default)]
struct ScriptState {
    program: Option<String>,
    lines: Vec<String>,
    pc: usize,
    breakpoints: HashSet<u32>,
}

impl ScriptState {
    fn reset(&mut self) {
        self.program = None;
        self.lines.clear();
        self.pc = 0;
        self.breakpoints.clear();
    }

    fn current_line(&self) -> u32 {
        if self.lines.is_empty() { 0 } else { (self.pc.min(self.lines.len() - 1) + 1) as u32 }
    }

    fn program_path(&self) -> String {
        self.program.clone().unwrap_or_else(|| "<memory>".into())
    }

    fn current_source_line(&self) -> String {
        self.lines
            .get(self.pc)
            .cloned()
            .unwrap_or_else(|| String::from("// <end of script>"))
    }
}

struct ScriptVm {
    state: Arc<Mutex<ScriptState>>,
}

const THREAD: ThreadId = 1;
const FRAME_ID: FrameId = 1;
const LOCALS_REF: VarRef = 1;

impl ScriptVm {
    fn with_state<F, R>(&self, f: F) -> R
    where
        F: FnOnce(&mut ScriptState) -> R,
    {
        let mut guard = self.state.lock().expect("script state poisoned");
        f(&mut guard)
    }

    fn with_state_ref<F, R>(&self, f: F) -> R
    where
        F: FnOnce(&ScriptState) -> R,
    {
        let guard = self.state.lock().expect("script state poisoned");
        f(&guard)
    }
}

impl VitteVm for ScriptVm {
    fn launch(&mut self, program: &str, _args: &[String]) -> Result<()> {
        let path = Path::new(program);
        let mut state = self.state.lock().expect("script state poisoned");
        state.reset();
        state.program = Some(path.to_string_lossy().to_string());

        let content = fs::read_to_string(path).unwrap_or_else(|_| {
            (1..=12)
                .map(|i| format!("print(\"demo line {i}\")"))
                .collect::<Vec<_>>()
                .join("\n")
        });
        state.lines = content.lines().map(|l| l.to_string()).collect();
        if state.lines.is_empty() {
            state.lines.push(String::from("// empty script"));
        }
        state.pc = 0;
        Ok(())
    }

    fn continue_all(&mut self) -> Result<StopReason> {
        self.with_state(|st| {
            if st.lines.is_empty() {
                return Ok(StopReason::Terminated { exit_code: 0 });
            }
            let len = st.lines.len();
            let mut idx = st.pc + 1; // quitte la ligne courante
            while idx < len {
                let line_num = (idx + 1) as u32;
                if st.breakpoints.contains(&line_num) {
                    st.pc = idx;
                    return Ok(StopReason::Breakpoint {
                        thread_id: THREAD,
                        source_path: st.program_path(),
                        line: line_num,
                    });
                }
                idx += 1;
            }
            st.pc = len - 1;
            Ok(StopReason::Terminated { exit_code: 0 })
        })
    }

    fn step_over(&mut self) -> Result<StopReason> {
        self.with_state(|st| {
            if st.lines.is_empty() {
                return Ok(StopReason::Terminated { exit_code: 0 });
            }
            if st.pc + 1 >= st.lines.len() {
                st.pc = st.lines.len() - 1;
                return Ok(StopReason::Terminated { exit_code: 0 });
            }
            st.pc += 1;
            Ok(StopReason::Step { thread_id: THREAD })
        })
    }

    fn step_in(&mut self) -> Result<StopReason> {
        self.step_over()
    }

    fn step_out(&mut self) -> Result<StopReason> {
        self.with_state(|st| {
            if st.lines.is_empty() {
                return Ok(StopReason::Terminated { exit_code: 0 });
            }
            st.pc = st.lines.len() - 1;
            Ok(StopReason::Step { thread_id: THREAD })
        })
    }

    fn set_breakpoints(&mut self, source_path: &str, lines: &[u32]) -> Result<Vec<Breakpoint>> {
        self.with_state(|st| {
            st.breakpoints.clear();
            st.breakpoints.extend(lines.iter().copied());

            let max_line = st.lines.len() as u32;
            let actual_source =
                if source_path.is_empty() { st.program_path() } else { source_path.to_string() };

            Ok(lines
                .iter()
                .map(|&line| {
                    let verified = line > 0 && line <= max_line;
                    Breakpoint {
                        source_path: actual_source.clone(),
                        requested_line: line,
                        verified,
                        actual_line: if verified { line } else { max_line.max(1) },
                    }
                })
                .collect())
        })
    }

    fn stack_trace(&self, _thread_id: Option<ThreadId>) -> Result<Vec<Frame>> {
        self.with_state_ref(|st| {
            let line = st.current_line().max(1);
            Ok(vec![Frame {
                id: FRAME_ID,
                thread_id: THREAD,
                name: "main".into(),
                source_path: st.program_path(),
                line,
                column: 1,
            }])
        })
    }

    fn scopes(&self, _frame_id: FrameId) -> Result<Vec<Scope>> {
        Ok(vec![Scope {
            name: "Locals".into(),
            variables_reference: LOCALS_REF as i64,
            expensive: false,
        }])
    }

    fn variables(&self, var_ref: VarRef) -> Result<Vec<Variable>> {
        self.with_state_ref(|st| {
            if var_ref != LOCALS_REF {
                return Ok(Vec::new());
            }
            let line_str = st.current_line().to_string();
            let source_line = st.current_source_line();
            let mut vars = Vec::new();
            vars.push(Variable {
                name: "line".into(),
                value: line_str,
                r#type: Some("i64".into()),
                variables_reference: 0,
            });
            vars.push(Variable {
                name: "source".into(),
                value: format!("\"{}\"", st.program_path()),
                r#type: Some("str".into()),
                variables_reference: 0,
            });
            vars.push(Variable {
                name: "code".into(),
                value: format!("\"{}\"", source_line.trim()),
                r#type: Some("str".into()),
                variables_reference: 0,
            });
            Ok(vars)
        })
    }

    fn evaluate(&mut self, expr: &str) -> Result<Option<String>> {
        self.with_state_ref(|st| {
            let trimmed = expr.trim();
            let val = match trimmed {
                "line" => Some(st.current_line().to_string()),
                "program" => Some(st.program_path()),
                "code" => Some(st.current_source_line()),
                _ => None,
            };
            Ok(val)
        })
    }

    fn disconnect(&mut self) -> Result<()> {
        self.with_state(|st| {
            st.reset();
        });
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn debugger_engine_emits_events() -> Result<()> {
        let mut engine = DebuggerEngine::new();
        engine.launch("demo-script.vitte", &[])?;
        let mut events = engine.drain_events()?;
        assert!(matches!(events.as_slice(), [EngineEvent::Initialized]));

        let armed = engine.set_breakpoints("demo-script.vitte", &[2, 5])?;
        assert_eq!(armed, vec![2, 5]);

        engine.r#continue()?;
        events = engine.drain_events()?;
        assert!(events.iter().any(|ev| matches!(ev, EngineEvent::Continued { .. })));
        assert!(
            events
                .iter()
                .any(|ev| matches!(ev, EngineEvent::StoppedBreakpoint { line: 2, .. }))
        );

        engine.step_over()?;
        events = engine.drain_events()?;
        assert!(events.iter().any(|ev| matches!(ev, EngineEvent::StoppedStep { .. })));

        engine.disconnect()?;
        events = engine.drain_events()?;
        assert!(events.iter().any(|ev| matches!(ev, EngineEvent::Terminated { exit_code: 0 })));
        Ok(())
    }
}
