//! Incremental parsing support for IDE/LSP scenarios.
//! The engine keeps a cached AST and updates only the impacted region when edits occur.

use crate::{Block, Expr, FnItem, Item, Span, Stmt, SyntaxModule, TypeExpr, parse_module};
use std::collections::HashMap;
use std::collections::hash_map::DefaultHasher;
use std::hash::{Hash, Hasher};
use std::ops::Range;

/// Text edit expressed in **byte offsets** over UTF-8 source.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct TextEdit {
    pub range: Range<usize>,
    pub replacement: String,
}

/// Summary of the incremental update.
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct ParseDelta {
    /// Functions whose bodies changed (same name, different content).
    pub changed_functions: Vec<String>,
    /// Newly inserted functions.
    pub inserted_functions: Vec<String>,
    /// Functions removed from the module.
    pub removed_functions: Vec<String>,
    /// Indicates whether the diagnostic set changed.
    pub errors_changed: bool,
    /// True when a full reparse occurred instead of a regional update.
    pub full_reparse: bool,
}

/// Incremental parser keeping an AST snapshot in sync with text edits.
#[derive(Clone, Debug)]
pub struct IncrementalParser {
    source: String,
    module: SyntaxModule,
}

impl IncrementalParser {
    /// Builds a fresh incremental parser from the provided source.
    pub fn new(initial_source: impl Into<String>) -> Self {
        let source = initial_source.into();
        let module = parse_module(&source);
        Self { source, module }
    }

    /// Access the current source backing the parser.
    pub fn source(&self) -> &str {
        &self.source
    }

    /// Access the latest parsed module.
    pub fn module(&self) -> &SyntaxModule {
        &self.module
    }

    /// Applies a text edit and updates the cached AST.
    ///
    /// The update is performed regionally: only the functions overlapping the edit
    /// (or the gap between surrounding functions) are reparsed, while unaffected
    /// functions are shifted in-place.
    pub fn apply_edit(&mut self, edit: TextEdit) -> ParseDelta {
        let old_source = self.source.clone();
        let old_module = self.module.clone();

        let range = edit.range.clone();
        let removed_len = range.end.saturating_sub(range.start);
        self.source.replace_range(range.clone(), &edit.replacement);

        let delta = edit.replacement.len() as isize - removed_len as isize;
        let (region_start_old, region_end_old) = region_bounds(&old_module, &range);
        let region_start_new =
            shift_offset(region_start_old, &range, delta, edit.replacement.len());
        let region_end_new = shift_offset(region_end_old, &range, delta, edit.replacement.len());

        let region_start_new = region_start_new.min(self.source.len());
        let region_end_new = region_end_new.min(self.source.len());

        let region_slice = if region_start_new <= region_end_new {
            &self.source[region_start_new..region_end_new]
        } else {
            &self.source[region_end_new..region_start_new]
        };

        let mut region_module = parse_module(region_slice);
        shift_module(&mut region_module, region_start_new as isize);

        let old_region_len = region_end_old.saturating_sub(region_start_old);
        let new_region_len = region_end_new.saturating_sub(region_start_new);
        let delta_region = new_region_len as isize - old_region_len as isize;

        let mut new_items = Vec::new();
        for item in &old_module.items {
            if item_span(item).end as usize <= region_start_old {
                new_items.push(item.clone());
            }
        }

        new_items.extend(region_module.items.into_iter());

        for item in &old_module.items {
            if item_span(item).start as usize >= region_end_old {
                let mut cloned = item.clone();
                shift_item(&mut cloned, delta_region);
                new_items.push(cloned);
            }
        }

        let mut new_errors = Vec::new();
        for err in &old_module.errors {
            if err.span.end as usize <= region_start_old {
                new_errors.push(err.clone());
            } else if err.span.start as usize >= region_end_old {
                let mut cloned = err.clone();
                shift_span(&mut cloned.span, delta_region);
                new_errors.push(cloned);
            }
        }
        new_errors.extend(region_module.errors.into_iter());
        new_errors.sort_by_key(|e| e.span.start);

        self.module = SyntaxModule {
            items: new_items,
            span: Span::new(0, self.source.len()),
            errors: new_errors,
        };

        let (mut changed, mut inserted, mut removed) = diff_records(
            &collect_fn_records(&old_module, &old_source),
            &collect_fn_records(&self.module, &self.source),
        );

        changed.sort();
        changed.dedup();
        inserted.sort();
        inserted.dedup();
        removed.sort();
        removed.dedup();

        ParseDelta {
            changed_functions: changed,
            inserted_functions: inserted,
            removed_functions: removed,
            errors_changed: old_module.errors != self.module.errors,
            full_reparse: region_start_old == 0
                && region_end_old as usize == old_module.span.end as usize
                && (removed_len == old_source.len() || self.module.items.len() == 0),
        }
    }
}

fn region_bounds(module: &SyntaxModule, edit: &Range<usize>) -> (usize, usize) {
    let mut prev_end = 0usize;
    let mut next_start = module.span.end as usize;
    let mut first_dirty = None;
    let mut last_dirty = None;

    for item in &module.items {
        let span = item_span(item);
        let span_start = span.start as usize;
        let span_end = span.end as usize;

        if span_end <= edit.start {
            prev_end = span_end;
            continue;
        }
        if span_start >= edit.end {
            next_start = span_start;
            break;
        }
        if first_dirty.is_none() {
            first_dirty = Some(span_start);
        }
        last_dirty = Some(span_end);
    }

    let start = first_dirty.unwrap_or(prev_end);
    let end = last_dirty.unwrap_or(next_start);
    (start.min(end), end.max(start))
}

fn shift_offset(offset: usize, edit: &Range<usize>, delta: isize, inserted_len: usize) -> usize {
    if offset < edit.start {
        offset
    } else if offset >= edit.end {
        apply_delta(offset, delta)
    } else {
        edit.start + inserted_len
    }
}

fn apply_delta(offset: usize, delta: isize) -> usize {
    if delta >= 0 {
        offset + delta as usize
    } else {
        offset.saturating_sub(delta.wrapping_neg() as usize)
    }
}

fn item_span(item: &Item) -> Span {
    match item {
        Item::Fn(f) => f.span,
    }
}

fn shift_module(module: &mut SyntaxModule, delta: isize) {
    shift_span(&mut module.span, delta);
    for item in &mut module.items {
        shift_item(item, delta);
    }
    for err in &mut module.errors {
        shift_span(&mut err.span, delta);
    }
}

fn shift_item(item: &mut Item, delta: isize) {
    match item {
        Item::Fn(f) => shift_fn(f, delta),
    }
}

fn shift_fn(func: &mut FnItem, delta: isize) {
    shift_span(&mut func.span, delta);
    for attr in &mut func.attributes {
        shift_span(&mut attr.span, delta);
        if let Some(args) = &mut attr.args {
            for expr in args {
                shift_expr(expr, delta);
            }
        }
    }
    for param in &mut func.params {
        shift_span(&mut param.span, delta);
        if let Some(ty) = &mut param.ty {
            shift_type(ty, delta);
        }
    }
    if let Some(ret) = &mut func.ret {
        shift_type(ret, delta);
    }
    shift_block(&mut func.body, delta);
}

fn shift_type(ty: &mut TypeExpr, delta: isize) {
    match ty {
        TypeExpr::Named(_, span) | TypeExpr::Unit(span) => shift_span(span, delta),
        TypeExpr::Tuple(elems, span) => {
            for elem in elems {
                shift_type(elem, delta);
            }
            shift_span(span, delta);
        }
    }
}

fn shift_block(block: &mut Block, delta: isize) {
    shift_span(&mut block.span, delta);
    for stmt in &mut block.stmts {
        shift_stmt(stmt, delta);
    }
}

fn shift_stmt(stmt: &mut Stmt, delta: isize) {
    match stmt {
        Stmt::Let { attributes, span, ty, init, .. } => {
            shift_span(span, delta);
            for attr in attributes {
                shift_span(&mut attr.span, delta);
                if let Some(args) = &mut attr.args {
                    for expr in args {
                        shift_expr(expr, delta);
                    }
                }
            }
            if let Some(t) = ty {
                shift_type(t, delta);
            }
            if let Some(expr) = init {
                shift_expr(expr, delta);
            }
        }
        Stmt::Expr(expr) => shift_expr(expr, delta),
        Stmt::Return(expr, span) => {
            shift_span(span, delta);
            if let Some(e) = expr {
                shift_expr(e, delta);
            }
        }
    }
}

fn shift_expr(expr: &mut Expr, delta: isize) {
    match expr {
        Expr::Unit(span)
        | Expr::Int(_, span)
        | Expr::Bool(_, span)
        | Expr::Str(_, span)
        | Expr::Var(_, span) => shift_span(span, delta),
        Expr::Call { span, callee, args } => {
            shift_span(span, delta);
            shift_expr(callee, delta);
            for arg in args {
                shift_expr(arg, delta);
            }
        }
        Expr::Binary { span, lhs, rhs, .. } => {
            shift_span(span, delta);
            shift_expr(lhs, delta);
            shift_expr(rhs, delta);
        }
        Expr::Unary { span, expr, .. } => {
            shift_span(span, delta);
            shift_expr(expr, delta);
        }
        Expr::If { span, cond, then_blk, else_blk } => {
            shift_span(span, delta);
            shift_expr(cond, delta);
            shift_block(then_blk, delta);
            if let Some(else_blk) = else_blk {
                shift_block(else_blk, delta);
            }
        }
        Expr::Block(block) => shift_block(block, delta),
    }
}

fn shift_span(span: &mut Span, delta: isize) {
    let start = span.start as isize + delta;
    let end = span.end as isize + delta;
    span.start = start.max(0) as u32;
    span.end = end.max(0) as u32;
}

#[derive(Clone, Debug)]
struct FnRecord {
    name: String,
    hash: u64,
}

fn collect_fn_records(module: &SyntaxModule, source: &str) -> Vec<FnRecord> {
    let mut out = Vec::new();
    for item in &module.items {
        match item {
            Item::Fn(func) => {
                let slice = slice_from_span(source, func.span);
                let mut hasher = DefaultHasher::new();
                slice.hash(&mut hasher);
                out.push(FnRecord { name: func.name.clone(), hash: hasher.finish() });
            }
        }
    }
    out
}

fn diff_records(old: &[FnRecord], new: &[FnRecord]) -> (Vec<String>, Vec<String>, Vec<String>) {
    let mut old_map: HashMap<String, Vec<u64>> = HashMap::new();
    let mut new_map: HashMap<String, Vec<u64>> = HashMap::new();

    for record in old {
        old_map.entry(record.name.clone()).or_default().push(record.hash);
    }
    for record in new {
        new_map.entry(record.name.clone()).or_default().push(record.hash);
    }

    for hashes in old_map.values_mut() {
        hashes.sort_unstable();
    }
    for hashes in new_map.values_mut() {
        hashes.sort_unstable();
    }

    let mut changed = Vec::new();
    let mut inserted = Vec::new();
    let mut removed = Vec::new();

    for (name, old_hashes) in old_map {
        if let Some(new_hashes) = new_map.remove(&name) {
            let shared = old_hashes.len().min(new_hashes.len());
            for idx in 0..shared {
                if old_hashes[idx] != new_hashes[idx] {
                    changed.push(name.clone());
                }
            }
            if new_hashes.len() > shared {
                inserted.extend(std::iter::repeat(name.clone()).take(new_hashes.len() - shared));
            }
            if old_hashes.len() > shared {
                removed.extend(std::iter::repeat(name.clone()).take(old_hashes.len() - shared));
            }
        } else {
            removed.extend(std::iter::repeat(name.clone()).take(old_hashes.len()));
        }
    }

    for (name, hashes) in new_map {
        inserted.extend(std::iter::repeat(name.clone()).take(hashes.len()));
    }

    (changed, inserted, removed)
}

fn slice_from_span<'a>(source: &'a str, span: Span) -> &'a str {
    let len = source.len();
    let start = span.start.min(span.end) as usize;
    let end = span.end.max(span.start) as usize;
    let start = start.min(len);
    let end = end.min(len);
    &source[start..end]
}

#[cfg(test)]
mod tests {
    use super::*;

    fn edit(range: Range<usize>, replacement: &str) -> TextEdit {
        TextEdit { range, replacement: replacement.to_string() }
    }

    #[test]
    fn edit_inside_function_marks_changed_only_one() {
        let src = "fn a() -> int { return 1; }\nfn b() { return true; }\n";
        let mut parser = IncrementalParser::new(src);
        let pos = src.find("return 1").unwrap() + "return ".len();
        let delta = parser.apply_edit(edit(pos..pos + 1, "4"));
        assert_eq!(delta.changed_functions, vec!["a"]);
        assert!(delta.inserted_functions.is_empty());
        assert!(delta.removed_functions.is_empty());
        assert!(!delta.errors_changed);
        assert_eq!(parser.module().items.len(), 2);
    }

    #[test]
    fn insert_new_function_between_existing() {
        let src = "fn a() { return; }\nfn c() { return; }\n";
        let mut parser = IncrementalParser::new(src);
        let insertion = "\nfn b() { return; }\n";
        let pos = src.find("fn c").unwrap();
        let delta = parser.apply_edit(edit(pos..pos, insertion));
        assert_eq!(delta.inserted_functions, vec!["b"]);
        assert!(delta.changed_functions.is_empty());
        assert!(delta.removed_functions.is_empty());
        assert_eq!(parser.module().items.len(), 3);

        let names: Vec<_> = parser
            .module()
            .items
            .iter()
            .map(|item| match item {
                Item::Fn(f) => f.name.as_str(),
            })
            .collect();
        assert_eq!(names, vec!["a", "b", "c"]);
    }

    #[test]
    fn delete_function_reports_removed() {
        let src = "fn a() { return; }\nfn b() { return; }\n";
        let mut parser = IncrementalParser::new(src);
        let start = src.find("fn b").unwrap();
        let delta = parser.apply_edit(edit(start..src.len(), ""));

        assert_eq!(delta.removed_functions, vec!["b"]);
        assert!(delta.changed_functions.is_empty());
        assert!(delta.inserted_functions.is_empty());
        assert_eq!(parser.module().items.len(), 1);
    }
}
