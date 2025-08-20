//! disasm.rs — Désassembleur “lisible humain” pour Vitte.
//!
//! Deux niveaux :
//!  - `disassemble_compact(chunk)` : 1 ligne par op, constants résolues
//!  - `disassemble_full(chunk, title)` : header + pool + lignes + debug + code (avec labels)
//!
//! Le désassembleur essaie d’être robuste :
//!  - Préfixes de labels pour les cibles de saut (L0001, L0002, …)
//!  - Annotation des `LoadConst` avec un aperçu de la constante
//!  - Détection d’index constants invalides (<invalid>)
//!  - Affichage des offsets de saut ainsi que la destination calculée
//!
//! Aucune dépendance externe : pure std.

use std::collections::HashMap;
use std::fmt::Write as _;
use crate::bytecode::op::Op;
use crate::bytecode::op::Op::*;
use crate::bytecode::{Chunk, ConstValue, Op};


/// Vue compacte : `PC  (line)  OP [annotations…]`
pub fn disassemble_compact(chunk: &Chunk) -> String {
    let mut out = String::new();

    // Préparer labels pour les cibles de saut (même en compact, utile)
    let labels = compute_labels(chunk);

    for (pc_usize, op) in chunk.ops.iter().enumerate() {
        let pc = pc_usize as u32;
        let line = chunk.lines.line_for_pc(pc);
        let line_disp = line.map(|x| x.to_string()).unwrap_or_else(|| "-".into());

        let pretty = fmt_op(chunk, pc, op, &labels);

        let _ = write!(
            out,
            "{pc:05} (line {:>4})  {pretty}\n",
            line_disp
        );
    }
    out
}

/// Vue complète : header + pool + line table + debug + code (avec labels) + compact à la fin.
pub fn disassemble_full(chunk: &Chunk, title: &str) -> String {
    let mut s = String::new();

    // ── Header ──────────────────────────────────────────────────────────────
    {
        let version = chunk.version();
        let stripped = chunk.flags().stripped;
        let n_ops = chunk.ops.len();
        let n_consts = chunk.consts.len();
        let hash = chunk.compute_hash();
        let _ = writeln!(s, "== {title} ==");
        let _ = writeln!(
            s,
            "• version: {}   stripped: {}   ops: {}   consts: {}   hash: 0x{hash:016x}",
            version, stripped, n_ops, n_consts
        );
    }

    // ── Pool de constantes ─────────────────────────────────────────────────
    if chunk.consts.len() > 0 {
        let _ = writeln!(s, "\n# Const Pool");
        for (ix, c) in chunk.consts.iter() {
            let (ty, preview) = fmt_const(c);
            let _ = writeln!(s, "  [{ix:03}] {:<5}  {preview}", ty);
        }
    } else {
        let _ = writeln!(s, "\n# Const Pool (vide)");
    }

    // ── Table des lignes (plages PC→line) ──────────────────────────────────
    {
        let _ = writeln!(s, "\n# Line Table (PC ranges)");
        let mut any = false;
        for (range, line) in chunk.lines.iter_ranges() {
            any = true;
            let _ = writeln!(s, "  [{:05}..{:05})  line {}", range.start, range.end, line);
        }
        if !any {
            let _ = writeln!(s, "  <aucune info de ligne>");
        }
    }

    // ── Debug info ─────────────────────────────────────────────────────────
    {
        let _ = writeln!(s, "\n# Debug");
        if let Some(main) = &chunk.debug.main_file {
            let _ = writeln!(s, "  main_file: {main}");
        }
        if !chunk.debug.files.is_empty() {
            let _ = writeln!(s, "  files ({}):", chunk.debug.files.len());
            for f in &chunk.debug.files {
                let _ = writeln!(s, "    - {f}");
            }
        } else {
            let _ = writeln!(s, "  files: (none)");
        }
        if !chunk.debug.symbols.is_empty() {
            let _ = writeln!(s, "  symbols ({}):", chunk.debug.symbols.len());
            for (sym, pc) in &chunk.debug.symbols {
                let _ = writeln!(s, "    - {:05}  {}", pc, sym);
            }
        } else {
            let _ = writeln!(s, "  symbols: (none)");
        }
    }

    // ── Code (avec labels) ─────────────────────────────────────────────────
    {
        let _ = writeln!(s, "\n# Code");
        let labels = compute_labels(chunk);

        // Largeur colonnes gentille : Lxxxx si présent
        for (pc_usize, op) in chunk.ops.iter().enumerate() {
            let pc = pc_usize as u32;
            let line = chunk.lines.line_for_pc(pc);
            let line_disp = line.map(|x| x.to_string()).unwrap_or_else(|| "-".into());

            // Label de la ligne (si ce PC est cible)
            if let Some(lbl) = labels.get(&pc) {
                let _ = writeln!(s, "{lbl}:");
            }

            let pretty = fmt_op(chunk, pc, op, &labels);

            let _ = writeln!(
                s,
                "  {pc:05} (line {:>4})  {pretty}",
                line_disp
            );
        }
    }

    // ── Append : vue compacte ──────────────────────────────────────────────
    s.push_str("\n# Compact\n");
    s.push_str(&disassemble_compact(chunk));

    s
}

/* ────────────────────────────── Helpers ────────────────────────────── */

/// Retourne une map `pc cible -> label` pour toutes les destinations de saut.
fn compute_labels(chunk: &Chunk) -> HashMap<u32, String> {
    let mut set = HashMap::<u32, String>::new();
    // On collecte d’abord tous les PC cibles
    let mut targets = Vec::<u32>::new();

    for (pc_usize, op) in chunk.ops.iter().enumerate() {
        let pc = pc_usize as u32;
        match *op {
            Op::Jump(off) | Op::JumpIfFalse(off) => {
                // PC pointe sur l’op courant ; next = pc+1 ; destination = next + off
                let dest = (pc as i64 + 1 + off as i64) as i64;
                if dest >= 0 {
                    let d = dest as u32;
                    if (d as usize) < chunk.ops.len() {
                        targets.push(d);
                    }
                }
            }
            _ => {}
        }
    }

    // Assigner des labels L0001, L0002, … dans l’ordre d’apparition
    targets.sort_unstable();
    targets.dedup();
    for (i, pc) in targets.into_iter().enumerate() {
        set.insert(pc, format!("L{:04}", i + 1));
    }
    set
}

/// Formate une instruction avec annotations utiles (const preview, cibles, etc.).
fn fmt_op(chunk: &Chunk, pc: u32, op: &Op, labels: &HashMap<u32, String>) -> String {
    use Op::*;
    match *op {
        LoadConst(ix) => {
            let annex = match chunk.consts.get(ix) {
                Some(ConstValue::Str(ref s)) => format!("; \"{}\"", shorten(s, 60)),
                Some(ref v) => format!("; {}", pretty_const(v, 60)),
                None => "; <invalid>".into(),
            };
            format!("LoadConst {ix}  {annex}")
        }
        LoadLocal(ix)      => format!("LoadLocal {ix}"),
        StoreLocal(ix)     => format!("StoreLocal {ix}"),

        LoadUpvalue(ix)    => format!("LoadUpvalue {ix}"),
        StoreUpvalue(ix)   => format!("StoreUpvalue {ix}"),

        MakeClosure(func_ix, n) => format!("MakeClosure func={func_ix} upvalues={n}"),

        Call(argc)         => format!("Call argc={argc}"),
        TailCall(argc)     => format!("TailCall argc={argc}"),

        Jump(off) => {
            let dest = (pc as i64 + 1 + off as i64).max(0) as u32;
            let lbl = labels.get(&dest).cloned().unwrap_or_else(|| format!("{dest}"));
            format!("Jump {off:+}  -> {lbl}")
        }
        JumpIfFalse(off) => {
            let dest = (pc as i64 + 1 + off as i64).max(0) as u32;
            let lbl = labels.get(&dest).cloned().unwrap_or_else(|| format!("{dest}"));
            format!("JumpIfFalse {off:+}  -> {lbl}")
        }

        Print            => "Print".into(),
        Add              => "Add".into(),
        Sub              => "Sub".into(),
        Mul              => "Mul".into(),
        Div              => "Div".into(),
        Mod              => "Mod".into(),
        Neg              => "Neg".into(),
        Not              => "Not".into(),

        Eq               => "Eq".into(),
        Ne               => "Ne".into(),
        Lt               => "Lt".into(),
        Le               => "Le".into(),
        Gt               => "Gt".into(),
        Ge               => "Ge".into(),

        LoadTrue         => "LoadTrue".into(),
        LoadFalse        => "LoadFalse".into(),
        LoadNull         => "LoadNull".into(),

        Pop              => "Pop".into(),
        Return           => "Return".into(),
        ReturnVoid       => "ReturnVoid".into(),
        Nop              => "Nop".into(),
    }
}

/// Renvoie (type court, aperçu court) pour une constante.
fn fmt_const(c: &ConstValue) -> (&'static str, String) {
    match c {
        ConstValue::Null      => ("null", "null".into()),
        ConstValue::Bool(b)   => ("bool", format!("{b}")),
        ConstValue::I64(i)    => ("i64",  format!("{i}")),
        ConstValue::F64(x)    => ("f64",  format!("{x}")),
        ConstValue::Str(s)    => ("str",  preview_str(s, 80)),
        ConstValue::Bytes(b)  => ("bytes", format!("len={}", b.len())),
    }
}

/// Aperçu constant “humain”.
fn pretty_const(c: &ConstValue, str_max: usize) -> String {
    match c {
        ConstValue::Str(s) => format!("\"{}\"", shorten(s, str_max)),
        ConstValue::Bytes(b) => format!("bytes[{}]", b.len()),
        other => format!("{other}"),
    }
}

/// Prévisualisation d’une chaîne (échappée si besoin).
fn preview_str(s: &str, max: usize) -> String {
    let esc = escape_for_preview(s);
    if esc.len() <= max { format!("\"{esc}\"") } else { format!("\"{}…\"", &esc[..max]) }
}

fn escape_for_preview(s: &str) -> String {
    let mut out = String::with_capacity(s.len());
    for ch in s.chars() {
        match ch {
            '\n' => out.push_str("\\n"),
            '\t' => out.push_str("\\t"),
            '\r' => out.push_str("\\r"),
            '"'  => out.push_str("\\\""),
            '\\' => out.push_str("\\\\"),
            c if c.is_control() => {
                // Protection minimale pour autres contrôles
                use std::fmt::Write as _;
                let _ = write!(out, "\\x{:02X}", c as u32);
            }
            c => out.push(c),
        }
    }
    out
}

fn shorten(s: &str, max: usize) -> String {
    if s.len() <= max { s.to_string() } else { format!("{}…", &s[..max]) }
}
