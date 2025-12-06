#!/usr/bin/env python3
"""
vittec-stage1 – front-end executable rules (resolver + type checker, MVP subset)

This stage1 binary is intentionally small and pure-Python, but it applies the
minimal type system rules described in docs/type-system.md:
  - modules, imports, structs, enums, aliases, functions with annotated params/return,
  - local bindings (let/let mut), assignments, returns,
  - expressions (literals, struct/enum construction, calls, field access, binops),
  - control-flow blocks (if/elif/else, while, match with enum variants + `_`).

Diagnostics are emitted with file/line/col information whenever a rule is
violated (unknown symbols, wrong argument types, invalid assignments, etc.).
The goal is not to be a full parser, but to exercise the executable version of
the documented rules so tests and examples surface type issues early.
"""
from __future__ import annotations

import argparse
import dataclasses
import json
import re
import sys
from pathlib import Path
from typing import Dict, List, Optional, Tuple

# -----------------------------------------------------------------------------
# Lexing
# -----------------------------------------------------------------------------


@dataclasses.dataclass
class Token:
    kind: str
    value: str
    line: int
    col: int


TOKEN_REGEX = re.compile(
    r"""
    (?P<ws>[ \t]+)|
    (?P<comment>\#.*$)|
    (?P<string>"([^"\\]|\\.)*")|
    (?P<number>0x[0-9A-Fa-f]+|\d+\.\d+|\d+)|        # ints / floats (approx)
    (?P<arrow>->)|
    (?P<fat_arrow>=\>)|
    (?P<double_colon>::)|
    (?P<op>==|!=|<=|>=)|
    (?P<ident>[A-Za-z_][A-Za-z0-9_]*)|
    (?P<sym>[{}()\[\],:+\-*/%\.=<>])|
    (?P<unknown>.)
""",
    re.VERBOSE,
)


def tokenize(text: str) -> List[Token]:
    tokens: List[Token] = []
    for lineno, line in enumerate(text.splitlines(), start=1):
        idx = 0
        while idx < len(line):
            match = TOKEN_REGEX.match(line, idx)
            if not match:
                tokens.append(Token("unknown", line[idx], lineno, idx + 1))
                idx += 1
                continue

            kind = match.lastgroup or "unknown"
            value = match.group(kind)
            idx = match.end()

            if kind in ("ws", "comment"):
                continue
            if kind == "unknown":
                tokens.append(Token(kind, value, lineno, match.start() + 1))
                continue
            tokens.append(Token(kind, value, lineno, match.start() + 1))
        tokens.append(Token("newline", "\\n", lineno, len(line) + 1))
    tokens.append(Token("eof", "", lineno if tokens else 1, 1))
    return tokens


# -----------------------------------------------------------------------------
# Diagnostics
# -----------------------------------------------------------------------------


@dataclasses.dataclass
class Diagnostic:
    severity: str  # "error" ou "warning"
    message: str
    line: int
    col: int

    def format(self) -> str:
        return f"diag:{self.severity}:L{self.line}C{self.col}:{self.message}"


class DiagSink:
    def __init__(self) -> None:
        self.items: List[Diagnostic] = []

    def error(self, line: int, col: int, message: str) -> None:
        self.items.append(Diagnostic("error", message, line, col))

    def warn(self, line: int, col: int, message: str) -> None:
        self.items.append(Diagnostic("warning", message, line, col))

    def has_errors(self) -> bool:
        return any(d.severity == "error" for d in self.items)


# -----------------------------------------------------------------------------
# Types and symbols
# -----------------------------------------------------------------------------


PRIMITIVES = {
    "i8",
    "i16",
    "i32",
    "i64",
    "u8",
    "u16",
    "u32",
    "u64",
    "f32",
    "f64",
    "bool",
    "string",
    "never",
    "()",
}


class Type:
    def __init__(self, name: str, kind: str = "named") -> None:
        self.name = name
        self.kind = kind  # primitive | struct | enum | union | alias | fn | unknown

    def __repr__(self) -> str:
        return f"Type({self.name})"

    def is_primitive(self) -> bool:
        return self.kind == "primitive"


class TypeEnv:
    def __init__(self, module: str) -> None:
        self.module = module
        self.types: Dict[str, Type] = {}
        self.alias_target: Dict[str, str] = {}
        self.enums: Dict[str, Dict[str, List[Tuple[str, str]]]] = {}
        self.struct_fields: Dict[str, Dict[str, str]] = {}
        self.union_fields: Dict[str, Dict[str, str]] = {}
        self.consts: Dict[str, str] = {}
        self.functions: Dict[str, Tuple[List[str], str]] = {}
        for prim in PRIMITIVES:
            self.types[prim] = Type(prim, "primitive")

    def qualify(self, name: str) -> str:
        if "." in name or name in PRIMITIVES:
            return name
        return f"{self.module}.{name}"

    def add_struct(self, name: str, fields: Dict[str, str]) -> None:
        qname = self.qualify(name)
        self.types[qname] = Type(qname, "struct")
        self.struct_fields[qname] = fields

    def add_union(self, name: str, fields: Dict[str, str]) -> None:
        qname = self.qualify(name)
        self.types[qname] = Type(qname, "union")
        self.union_fields[qname] = fields

    def add_enum(self, name: str, variants: Dict[str, List[Tuple[str, str]]]) -> None:
        qname = self.qualify(name)
        self.types[qname] = Type(qname, "enum")
        self.enums[qname] = variants

    def add_alias(self, name: str, target: str) -> None:
        qname = self.qualify(name)
        self.types[qname] = Type(qname, "alias")
        self.alias_target[qname] = target

    def add_function(self, name: str, params: List[str], ret: str) -> None:
        qname = self.qualify(name)
        self.functions[qname] = (params, ret)

    def add_const(self, name: str, ty: str) -> None:
        qname = self.qualify(name)
        self.consts[name] = ty
        self.consts[qname] = ty

    def resolve(self, name: str) -> Optional[Type]:
        qname = self.qualify(name)
        if qname in self.types:
            return self.types[qname]
        if name in self.types:
            return self.types[name]
        return None

    def resolve_alias(self, name: str) -> str:
        visited = set()
        cur = name
        while cur in self.alias_target and cur not in visited:
            visited.add(cur)
            cur = self.alias_target[cur]
        return cur


# -----------------------------------------------------------------------------
# Expression parsing
# -----------------------------------------------------------------------------


class ExprParser:
    def __init__(self, tokens: List[Token]) -> None:
        self.tokens = tokens
        self.idx = 0

    def peek(self) -> Token:
        return self.tokens[self.idx]

    def advance(self) -> Token:
        tok = self.tokens[self.idx]
        self.idx += 1
        return tok

    def parse(self):
        return self.parse_binary(0)

    PRECEDENCE = {
        "or": 1,
        "and": 2,
        "==": 3,
        "!=": 3,
        "<": 4,
        "<=": 4,
        ">": 4,
        ">=": 4,
        "+": 5,
        "-": 5,
        "*": 6,
        "/": 6,
        "%": 6,
    }

    def parse_primary(self):
        tok = self.advance()
        if tok.kind == "number":
            if "." in tok.value:
                return ("literal", "f64")
            return ("literal", "i32")
        if tok.kind == "string":
            return ("literal", "string")
        if tok.kind == "ident" and tok.value in ("true", "false"):
            return ("literal", "bool")
        if tok.kind == "sym" and tok.value == "(":
            expr = self.parse()
            self.expect("sym", ")")
            return expr
        if tok.kind in ("ident",):
            return self.parse_postfix(tok)
        if tok.kind == "sym" and tok.value == "(" and self.peek().kind == "sym" and self.peek().value == ")":
            self.advance()
            return ("literal", "()")
        return ("unknown", tok.value)

    def parse_postfix(self, tok: Token):
        base = ("path", tok.value)
        while True:
            nxt = self.peek()
            if nxt.kind == "double_colon":
                self.advance()
                variant = self.expect("ident")
                base = ("enum_ctor", base, variant.value)
            elif nxt.kind == "sym" and nxt.value == "(":
                self.advance()
                args = []
                if not (self.peek().kind == "sym" and self.peek().value == ")"):
                    while True:
                        args.append(self.parse())
                        if self.peek().kind == "sym" and self.peek().value == ",":
                            self.advance()
                            continue
                        break
                self.expect("sym", ")")
                base = ("call", base, args)
            elif nxt.kind == "sym" and nxt.value == "{":
                # struct literal
                self.advance()
                fields = {}
                while not (self.peek().kind == "sym" and self.peek().value == "}"):
                    field_name = self.expect("ident").value
                    self.expect("sym", "=")
                    fields[field_name] = self.parse()
                    if self.peek().kind == "sym" and self.peek().value == ",":
                        self.advance()
                        continue
                    else:
                        break
                self.expect("sym", "}")
                base = ("struct_lit", base, fields)
            elif nxt.kind == "sym" and nxt.value == ".":
                self.advance()
                field = self.expect("ident")
                base = ("field", base, field.value)
            else:
                break
        return base

    def parse_binary(self, min_prec: int):
        left = self.parse_primary()
        while True:
            tok = self.peek()
            op = None
            if tok.kind == "op":
                op = tok.value
            elif tok.kind == "sym" and tok.value in "+-*/%<>":
                op = tok.value
            elif tok.kind == "ident" and tok.value in ("and", "or"):
                op = tok.value
            if op is None or self.PRECEDENCE.get(op, 0) < min_prec:
                break
            prec = self.PRECEDENCE[op]
            self.advance()
            right = self.parse_binary(prec + 1)
            left = ("binop", op, left, right)
        return left

    def expect(self, kind: str, value: Optional[str] = None) -> Token:
        tok = self.advance()
        if tok.kind != kind or (value is not None and tok.value != value):
            raise ValueError(f"expected {kind} {value or ''}, got {tok.kind} {tok.value}")
        return tok


# -----------------------------------------------------------------------------
# Parser + type checker (simple, line-driven)
# -----------------------------------------------------------------------------


def parse_type_name(raw: str, env: TypeEnv) -> Optional[str]:
    raw = raw.strip()
    if not raw:
        return None
    if raw.endswith("[]"):
        elem = parse_type_name(raw[:-2].strip(), env)
        return f"{elem}[]" if elem else None
    lowered = raw.lower()
    alias_map = {"int": "i32", "bool": "bool", "string": "string"}
    if lowered in alias_map:
        return alias_map[lowered]
    if raw in PRIMITIVES:
        return raw
    return env.qualify(raw)


def type_eq(a: str, b: str, env: TypeEnv) -> bool:
    return env.resolve_alias(a) == env.resolve_alias(b)


def infer_expr_type(expr, env: TypeEnv, locals_env: List[Dict[str, Tuple[str, bool]]], diags: DiagSink, line: int, col: int) -> str:
    kind = expr[0]
    if kind == "literal":
        return expr[1]
    if kind == "path":
        name = expr[1]
        # try local
        for scope in reversed(locals_env):
            if name in scope:
                return scope[name][0]
        if name in env.consts:
            return env.consts[name]
        qname = env.qualify(name)
        if qname in env.consts:
            return env.consts[qname]
        # type path treated as named type
        ty = env.resolve(name)
        if ty:
            return ty.name
        diags.error(line, col, f"unresolved identifier `{name}`")
        return "unknown"
    if kind == "field":
        base_ty = infer_expr_type(expr[1], env, locals_env, diags, line, col)
        field = expr[2]
        qbase = env.resolve_alias(base_ty)
        if qbase in env.struct_fields and field in env.struct_fields[qbase]:
            return env.struct_fields[qbase][field]
        diags.error(line, col, f"unknown field `{field}` on type `{base_ty}`")
        return "unknown"
    if kind == "struct_lit":
        base_expr = expr[1]
        base_ty = None
        if base_expr[0] == "path":
            base_ty = parse_type_name(base_expr[1], env)
        if not base_ty or base_ty not in env.struct_fields:
            diags.error(line, col, "struct literal with unknown type")
            return "unknown"
        expected_fields = env.struct_fields[env.resolve_alias(base_ty)]
        for fname, sub_expr in expr[2].items():
            if fname not in expected_fields:
                diags.error(line, col, f"field `{fname}` not in struct `{base_ty}`")
                continue
            sub_ty = infer_expr_type(sub_expr, env, locals_env, diags, line, col)
            exp_ty = expected_fields[fname]
            if not type_eq(sub_ty, exp_ty, env):
                diags.error(line, col, f"field `{fname}` expects `{exp_ty}`, got `{sub_ty}`")
        for fname in expected_fields:
            if fname not in expr[2]:
                diags.error(line, col, f"missing field `{fname}` for struct `{base_ty}`")
        return base_ty
    if kind == "enum_ctor":
        type_expr, variant = expr[1], expr[2]
        if type_expr[0] != "path":
            diags.error(line, col, "enum constructor requires enum path")
            return "unknown"
        enum_ty_name = parse_type_name(type_expr[1], env)
        if not enum_ty_name or enum_ty_name not in env.enums:
            diags.error(line, col, f"unknown enum `{type_expr[1]}`")
            return "unknown"
        if variant not in env.enums[enum_ty_name]:
            diags.error(line, col, f"unknown variant `{variant}` for enum `{enum_ty_name}`")
            return "unknown"
        return enum_ty_name
    if kind == "call":
        callee_expr, args = expr[1], expr[2]
        callee_type = None
        if callee_expr[0] == "path":
            func_name = callee_expr[1]
            qname = env.qualify(func_name) if "." not in func_name else func_name
            if qname in env.functions:
                param_types, ret_type = env.functions[qname]
                for idx, arg in enumerate(args):
                    arg_ty = infer_expr_type(arg, env, locals_env, diags, line, col)
                    if idx < len(param_types):
                        expected = param_types[idx]
                        if not type_eq(arg_ty, expected, env):
                            diags.error(line, col, f"arg {idx+1} of `{qname}` expects `{expected}`, got `{arg_ty}`")
                    else:
                        diags.error(line, col, f"too many args for `{qname}`")
                if len(args) < len(env.functions[qname][0]):
                    diags.error(line, col, f"not enough args for `{qname}` (expected {len(param_types)})")
                return ret_type
            # enum ctor call handled above when attached to enum_ctor base
            if func_name in ("print_help", "print_version"):
                return "()"
            diags.error(line, col, f"unresolved function `{func_name}`")
            return "unknown"
        if callee_expr[0] == "enum_ctor":
            enum_ty = infer_expr_type(callee_expr, env, locals_env, diags, line, col)
            enum_name = env.resolve_alias(enum_ty)
            variant = callee_expr[2]
            payload = env.enums.get(enum_name, {}).get(variant, [])
            if len(args) != len(payload):
                diags.error(line, col, f"variant `{variant}` expects {len(payload)} args, got {len(args)}")
            else:
                for idx, (field_name, field_ty) in enumerate(payload):
                    arg_ty = infer_expr_type(args[idx], env, locals_env, diags, line, col)
                    if not type_eq(arg_ty, field_ty, env):
                        diags.error(line, col, f"variant field `{field_name}` expects `{field_ty}`, got `{arg_ty}`")
            return enum_ty
        if callee_expr[0] == "field":
            base, method = callee_expr[1], callee_expr[2]
            if base[0] == "path":
                if base[1] == "env" and method == "args":
                    return "string[]"
                if base[1] == "lc_common":
                    if method in ("starts_with",):
                        return "bool"
                    if method in ("after_prefix", "print_line", "print_warning"):
                        return "string"
                if base[1] == "build_muffin" and method == "load_manifest":
                    return "unknown"
                if base[1] == "engine":
                    return "bool"
            return "unknown"
        return "unknown"
    if kind == "binop":
        op = expr[1]
        lhs_ty = infer_expr_type(expr[2], env, locals_env, diags, line, col)
        rhs_ty = infer_expr_type(expr[3], env, locals_env, diags, line, col)
        if op in ("==", "!=", "<", "<=", ">", ">=", "and", "or"):
            if lhs_ty != "unknown" and rhs_ty != "unknown" and not type_eq(lhs_ty, rhs_ty, env):
                diags.error(line, col, f"operands of `{op}` must match (lhs={lhs_ty}, rhs={rhs_ty})")
            return "bool"
        if op in ("+", "-", "*", "/", "%"):
            if lhs_ty not in PRIMITIVES or rhs_ty not in PRIMITIVES:
                diags.error(line, col, f"operator `{op}` requires numeric primitives")
                return "unknown"
            if lhs_ty != rhs_ty:
                diags.error(line, col, f"operands of `{op}` must have same type (lhs={lhs_ty}, rhs={rhs_ty})")
            return lhs_ty
        return "unknown"
    return "unknown"


# -----------------------------------------------------------------------------
# Statement/type checking helpers
# -----------------------------------------------------------------------------


def tokenize_expr(text: str, line: int) -> List[Token]:
    expr_tokens = tokenize(text)
    # Drop trailing newline/eof
    expr_tokens = [t for t in expr_tokens if t.kind not in ("newline", "eof")]
    if not expr_tokens:
        expr_tokens = [Token("ident", "", line, 1)]
    return expr_tokens


def parse_expression(text: str, env: TypeEnv, locals_env, diags: DiagSink, line: int, col: int):
    try:
        tokens = tokenize_expr(text, line)
        parser = ExprParser(tokens + [Token("eof", "", line, col)])
        expr = parser.parse()
        return expr
    except Exception as exc:  # pragma: no cover - defensive
        diags.error(line, col, f"failed to parse expression `{text}`: {exc}")
        return ("unknown", text)


def strip_inline_comment(line: str) -> str:
    """Remove trailing # comment outside of string literals."""
    in_str = False
    escaped = False
    for idx, ch in enumerate(line):
        if ch == "\\" and not escaped:
            escaped = True
            continue
        if ch == '"' and not escaped:
            in_str = not in_str
        if ch == "#" and not in_str:
            return line[:idx]
        escaped = False
    return line


def has_unquoted_equal(text: str) -> bool:
    """Return true if `=` appears outside of string literals."""
    in_str = False
    escaped = False
    for ch in text:
        if ch == "\\" and not escaped:
            escaped = True
            continue
        if ch == '"' and not escaped:
            in_str = not in_str
        if ch == "=" and not in_str:
            return True
        escaped = False
    return False


def check_statement(line_text: str, env: TypeEnv, locals_env, diags: DiagSink, line: int, current_fn: Tuple[str, List[str], str]):
    text = line_text.strip()
    if not text or text.startswith("#"):
        return
    if text.startswith("let "):
        rest = text[len("let ") :]
        is_mut = True
        if rest.startswith("mut "):
            is_mut = True
            rest = rest[len("mut ") :]
        if ":" in rest.split("=")[0]:
            # annotated let
            before_eq, expr_src = rest.split("=", 1)
            name_part, type_part = before_eq.split(":", 1)
            name = name_part.strip()
            ty_name = parse_type_name(type_part.strip(), env)
            expr = parse_expression(expr_src.strip(), env, locals_env, diags, line, 1)
            expr_ty = infer_expr_type(expr, env, locals_env, diags, line, 1)
            if ty_name and not type_eq(expr_ty, ty_name, env):
                diags.error(line, 1, f"let `{name}` expects `{ty_name}`, got `{expr_ty}`")
            locals_env[-1][name] = (ty_name or "unknown", is_mut)
        else:
            name, expr_src = rest.split("=", 1)
            expr = parse_expression(expr_src.strip(), env, locals_env, diags, line, 1)
            expr_ty = infer_expr_type(expr, env, locals_env, diags, line, 1)
            locals_env[-1][name.strip()] = (expr_ty, is_mut)
        return

    if text.startswith("const "):
        # treat like immutable let
        rest = text[len("const ") :]
        name_part, expr_src = rest.split("=", 1)
        if ":" in name_part:
            n, t = name_part.split(":", 1)
            ty_name = parse_type_name(t.strip(), env)
        else:
            ty_name = None
            n = name_part
        expr = parse_expression(expr_src.strip(), env, locals_env, diags, line, 1)
        expr_ty = infer_expr_type(expr, env, locals_env, diags, line, 1)
        if ty_name and not type_eq(expr_ty, ty_name, env):
            diags.error(line, 1, f"const `{n.strip()}` expects `{ty_name}`, got `{expr_ty}`")
        locals_env[-1][n.strip()] = (ty_name or expr_ty, False)
        return

    if text.startswith("return"):
        expr_src = text[len("return") :].strip()
        ret_ty = current_fn[2]
        if expr_src == "" and ret_ty != "()":
            diags.error(line, 1, f"return type `{ret_ty}` requires a value")
            return
        expr = parse_expression(expr_src, env, locals_env, diags, line, 1) if expr_src else ("literal", "()")
        expr_ty = infer_expr_type(expr, env, locals_env, diags, line, 1)
        if not type_eq(expr_ty, ret_ty, env):
            diags.error(line, 1, f"return expects `{ret_ty}`, got `{expr_ty}`")
        return

    if text.startswith("if "):
        cond_src = text[len("if ") :].strip()
        if cond_src.startswith("not "):
            cond_src = cond_src[len("not ") :].strip()
        cond_expr = parse_expression(cond_src, env, locals_env, diags, line, 1)
        cond_ty = infer_expr_type(cond_expr, env, locals_env, diags, line, 1)
        if cond_ty not in ("bool", "unknown"):
            diags.error(line, 1, f"if condition expects `bool`, got `{cond_ty}`")
        locals_env.append({})
        return "push"

    if text.startswith("elif "):
        cond_src = text[len("elif ") :].strip()
        cond_expr = parse_expression(cond_src, env, locals_env, diags, line, 1)
        cond_ty = infer_expr_type(cond_expr, env, locals_env, diags, line, 1)
        if cond_ty not in ("bool", "unknown"):
            diags.error(line, 1, f"elif condition expects `bool`, got `{cond_ty}`")
        locals_env[-1].clear()
        return

    if text.startswith("else"):
        locals_env[-1].clear()
        return

    if text.startswith("while "):
        cond_src = text[len("while ") :].strip()
        cond_expr = parse_expression(cond_src, env, locals_env, diags, line, 1)
        cond_ty = infer_expr_type(cond_expr, env, locals_env, diags, line, 1)
        if cond_ty not in ("bool", "unknown"):
            diags.error(line, 1, f"while condition expects `bool`, got `{cond_ty}`")
        locals_env.append({})
        return "push"

    if text.startswith("for "):
        body = text[len("for ") :].strip()
        if " in " in body:
            var_part, iter_part = body.split(" in ", 1)
            var_name = var_part.strip().rstrip(":")
            iter_raw = iter_part.strip().rstrip(":")
            iter_expr = parse_expression(iter_raw, env, locals_env, diags, line, 1)
            iter_ty = infer_expr_type(iter_expr, env, locals_env, diags, line, 1)
            elem_ty = "unknown"
            if iter_ty.endswith("[]"):
                elem_ty = iter_ty[:-2]
            locals_env.append({var_name: (elem_ty, True)})
        else:
            locals_env.append({})
        return "push"

    if text == "continue" or text == "break":
        return

    if text.startswith("match "):
        # match arms handled by caller
        return

    # Assignment or expression statement
    if has_unquoted_equal(text) and not text.startswith("type "):
        lhs, rhs = text.split("=", 1)
        lhs = lhs.strip()
        rhs_expr = parse_expression(rhs.strip(), env, locals_env, diags, line, 1)
        rhs_ty = infer_expr_type(rhs_expr, env, locals_env, diags, line, 1)
        if "." in lhs:
            var_name, field = lhs.split(".", 1)
            var_ty = None
            mutable = False
            for scope in reversed(locals_env):
                if var_name in scope:
                    var_ty, mutable = scope[var_name]
                    break
            if var_ty is None:
                diags.error(line, 1, f"assignment to unknown variable `{var_name}`")
            else:
                if not mutable:
                    diags.error(line, 1, f"cannot assign to immutable binding `{var_name}`")
                base_ty = env.resolve_alias(var_ty)
                if base_ty not in env.struct_fields or field not in env.struct_fields[base_ty]:
                    diags.error(line, 1, f"unknown field `{field}` on `{var_ty}`")
                else:
                    expected = env.struct_fields[base_ty][field]
                    if not type_eq(rhs_ty, expected, env):
                        diags.error(line, 1, f"field `{field}` expects `{expected}`, got `{rhs_ty}`")
        else:
            found = None
            for scope in reversed(locals_env):
                if lhs in scope:
                    found = scope[lhs]
                    break
            if not found:
                diags.error(line, 1, f"assignment to unknown variable `{lhs}`")
            else:
                lhs_ty, mutable = found
                if not mutable:
                    diags.error(line, 1, f"cannot assign to immutable binding `{lhs}`")
                elif rhs_ty != "unknown" and lhs_ty != "unknown" and not type_eq(rhs_ty, lhs_ty, env):
                    diags.error(line, 1, f"assignment expects `{lhs_ty}`, got `{rhs_ty}`")
        return

    # Expression-only statement: still type-check to surface missing symbols.
    expr = parse_expression(text, env, locals_env, diags, line, 1)
    infer_expr_type(expr, env, locals_env, diags, line, 1)


# -----------------------------------------------------------------------------
# File parsing (declarative, line-based)
# -----------------------------------------------------------------------------


def parse_source(text: str, diags: DiagSink):
    tokens = tokenize(text)
    lines = text.splitlines()
    module_name = "root"
    env = TypeEnv(module_name)
    locals_env: List[Dict[str, Tuple[str, bool]]] = []
    current_fn: Optional[Tuple[str, List[str], str]] = None
    block_stack: List[str] = []
    pending_struct = None
    pending_enum = None
    pending_union = None
    pending_match_scrutinee: List[str] = []
    pending_match_enum: List[Optional[str]] = []

    def close_block():
        nonlocal current_fn
        if not block_stack:
            return
        kind = block_stack.pop()
        if kind in ("if", "while", "block", "match-arm"):
            if locals_env:
                locals_env.pop()
        if kind == "fn":
            current_fn = None

    # Detect module name early for qualification.
    for raw_line in lines:
        stripped = strip_inline_comment(raw_line).strip()
        if stripped.startswith("module "):
            module_name = stripped.split(" ", 1)[1].strip()
            env.module = module_name
            break

    # Pre-scan for function signatures to allow forward references.
    for raw_line in lines:
        stripped = strip_inline_comment(raw_line).strip()
        if stripped.startswith("fn ") and "->" in stripped and "(" in stripped:
            header = stripped[len("fn ") :]
            before_ret, ret_raw = header.split("->", 1)
            name_and_params = before_ret.strip()
            if "(" in name_and_params and name_and_params.endswith(":"):
                fname, params_raw = name_and_params.split("(", 1)
                params_raw = params_raw[:-1]
                param_types: List[str] = []
                if params_raw.strip():
                    for part in params_raw.split(","):
                        if ":" not in part:
                            pieces = part.strip().split()
                            if len(pieces) == 2:
                                pty_raw, _ = pieces
                            else:
                                continue
                        else:
                            _, pty_raw = part.split(":", 1)
                        pty = parse_type_name(pty_raw.strip(), env) or "unknown"
                        param_types.append(pty)
                ret_type = parse_type_name(ret_raw.strip().rstrip(":"), env) or "unknown"
                env.add_function(fname.strip(), param_types, ret_type)

    line_idx = 0
    while line_idx < len(lines):
        line_no = line_idx + 1
        line = strip_inline_comment(lines[line_idx])
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            line_idx += 1
            continue

        if stripped == ".end":
            if block_stack and block_stack[-1] == "match":
                # pop potential arm scope before match
                if locals_env:
                    locals_env.pop()
                block_stack.pop()
                pending_match_scrutinee.pop()
                pending_match_enum.pop()
            else:
                close_block()
            line_idx += 1
            continue

        if current_fn is None:
            if stripped.startswith("module "):
                module_name = stripped.split(" ", 1)[1].strip()
                env.module = module_name
                line_idx += 1
                continue
            if stripped.startswith("const "):
                # Top-level constants
                rest = stripped[len("const ") :]
                name_part, expr_src = rest.split("=", 1)
                if ":" in name_part:
                    n, t = name_part.split(":", 1)
                    ty_name = parse_type_name(t.strip(), env)
                else:
                    ty_name = None
                    n = name_part
                expr = parse_expression(expr_src.strip(), env, [{}], diags, line_no, 1)
                expr_ty = infer_expr_type(expr, env, [{}], diags, line_no, 1)
                const_ty = ty_name or expr_ty
                env.add_const(n.strip(), const_ty)
                line_idx += 1
                continue
            if stripped.startswith("import "):
                # imports are recorded but not loaded; placeholder
                line_idx += 1
                continue
            if stripped.startswith("struct "):
                pending_struct = stripped.split(" ", 1)[1].strip()
                block_stack.append("struct")
                struct_fields: Dict[str, str] = {}
                line_idx += 1
                while line_idx < len(lines):
                    lno = line_idx + 1
                    l = strip_inline_comment(lines[line_idx]).strip()
                    if not l or l.startswith("#"):
                        line_idx += 1
                        continue
                    if l == ".end":
                        block_stack.pop()
                        env.add_struct(pending_struct, struct_fields)
                        pending_struct = None
                        break
                    if ":" not in l:
                            diags.error(lno, 1, f"invalid struct field syntax: `{l}`")
                    else:
                        fname, fty = l.split(":", 1)
                        fty_name = parse_type_name(fty.strip(), env)
                        if fty_name:
                            struct_fields[fname.strip()] = fty_name
                        else:
                            diags.error(lno, 1, f"unknown field type `{fty.strip()}`")
                    line_idx += 1
                line_idx += 1
                continue
            if stripped.startswith("union "):
                pending_union = stripped.split(" ", 1)[1].strip()
                block_stack.append("union")
                union_fields: Dict[str, str] = {}
                line_idx += 1
                while line_idx < len(lines):
                    lno = line_idx + 1
                    l = strip_inline_comment(lines[line_idx]).strip()
                    if not l or l.startswith("#"):
                        line_idx += 1
                        continue
                    if l == ".end":
                        block_stack.pop()
                        env.add_union(pending_union, union_fields)
                        pending_union = None
                        break
                    if ":" not in l:
                        diags.error(lno, 1, f"invalid union field syntax: `{l}`")
                    else:
                        fname, fty = l.split(":", 1)
                        fty_name = parse_type_name(fty.strip(), env)
                        if fty_name:
                            union_fields[fname.strip()] = fty_name
                        else:
                            diags.error(lno, 1, f"unknown field type `{fty.strip()}`")
                    line_idx += 1
                line_idx += 1
                continue
            if stripped.startswith("enum "):
                pending_enum = stripped.split(" ", 1)[1].strip()
                block_stack.append("enum")
                variants: Dict[str, List[Tuple[str, str]]] = {}
                line_idx += 1
                while line_idx < len(lines):
                    lno = line_idx + 1
                    l = strip_inline_comment(lines[line_idx]).strip()
                    if not l or l.startswith("#"):
                        line_idx += 1
                        continue
                    if l == ".end":
                        block_stack.pop()
                        env.add_enum(pending_enum, variants)
                        pending_enum = None
                        break
                    if "(" in l and l.endswith(")"):
                        vname, payload_raw = l.split("(", 1)
                        payload_raw = payload_raw[:-1]
                        fields = []
                        if payload_raw.strip():
                            for part in payload_raw.split(","):
                                if ":" not in part:
                                    diags.error(lno, 1, f"invalid enum payload `{part.strip()}`")
                                    continue
                                fname, fty = part.split(":", 1)
                                fty_name = parse_type_name(fty.strip(), env)
                                if fty_name:
                                    fields.append((fname.strip(), fty_name))
                                else:
                                    diags.error(lno, 1, f"unknown payload type `{fty.strip()}`")
                        variants[vname.strip()] = fields
                    else:
                        variants[l] = []
                    line_idx += 1
                line_idx += 1
                continue
            if stripped.startswith("type "):
                _, rest = stripped.split(" ", 1)
                if "=" not in rest:
                    diags.error(line_no, 1, "type alias requires `=`")
                else:
                    alias_name, target_raw = rest.split("=", 1)
                    target_name = parse_type_name(target_raw.strip(), env)
                    if target_name:
                        env.add_alias(alias_name.strip(), target_name)
                    else:
                        diags.error(line_no, 1, f"unknown target type `{target_raw.strip()}`")
                line_idx += 1
                continue
            if stripped.startswith("program "):
                pname = stripped[len("program ") :].rstrip(":").strip()
                current_fn = (env.qualify(pname), [], "()")
                block_stack.append("fn")
                locals_env = [{}]
                line_idx += 1
                continue
            if stripped.startswith("fn "):
                header = stripped[len("fn ") :]
                if "->" not in header:
                    diags.error(line_no, 1, "function must declare return type")
                    line_idx += 1
                    continue
                before_ret, ret_raw = header.split("->", 1)
                name_and_params = before_ret.strip()
                if "(" not in name_and_params or not name_and_params.endswith(")"):
                    diags.error(line_no, 1, "invalid function signature")
                    line_idx += 1
                    continue
                fname, params_raw = name_and_params.split("(", 1)
                params_raw = params_raw[:-1]
                params: List[Tuple[str, str]] = []
                param_types: List[str] = []
                if params_raw.strip():
                    for part in params_raw.split(","):
                        if ":" not in part:
                            pieces = part.strip().split()
                            if len(pieces) == 2:
                                # Accept shorthand like "String argv"
                                pty_raw, pname = pieces
                            else:
                                diags.error(line_no, 1, f"parameter missing type: `{part.strip()}`")
                                continue
                        else:
                            pname, pty_raw = part.split(":", 1)
                        pty = parse_type_name(pty_raw.strip(), env)
                        if not pty:
                            diags.error(line_no, 1, f"unknown parameter type `{pty_raw.strip()}`")
                            pty = "unknown"
                        params.append((pname.strip(), pty))
                        param_types.append(pty)
                ret_type = parse_type_name(ret_raw.strip().rstrip(":"), env) or "unknown"
                env.add_function(fname.strip(), param_types, ret_type)
                current_fn = (env.qualify(fname.strip()), param_types, ret_type)
                block_stack.append("fn")
                locals_env = [{}]
                for pname, pty in params:
                    locals_env[-1][pname] = (pty, False)
                line_idx += 1
                continue
            # Unknown top-level
            diags.error(line_no, 1, f"unexpected top-level item: `{stripped}`")
            line_idx += 1
            continue

        # In-function statements and control blocks
        if stripped.startswith("match "):
            scrut_expr = parse_expression(stripped[len("match ") :].strip(), env, locals_env, diags, line_no, 1)
            scrut_ty = infer_expr_type(scrut_expr, env, locals_env, diags, line_no, 1)
            pending_match_scrutinee.append(scrut_ty)
            pending_match_enum.append(scrut_ty if scrut_ty in env.enums else None)
            block_stack.append("match")
            locals_env.append({})
            line_idx += 1
            continue

        if "=>" in stripped and block_stack and block_stack[-1] == "match":
            # new match arm
            if locals_env:
                locals_env[-1].clear()
            pattern_raw, _ = stripped.split("=>", 1)
            pattern_raw = pattern_raw.strip()
            scrut_ty = pending_match_scrutinee[-1]
            enum_name = pending_match_enum[-1]
            if pattern_raw == "_":
                pass
            elif "::" in pattern_raw:
                enum_part, variant_part = pattern_raw.split("::", 1)
                enum_ty = parse_type_name(enum_part.strip(), env)
                variant_name = variant_part.strip()
                if enum_name and enum_ty and env.resolve_alias(enum_ty) != env.resolve_alias(enum_name):
                    diags.error(line_no, 1, f"match arm enum `{enum_ty}` does not match scrutinee `{enum_name}`")
                if enum_ty and env.resolve_alias(enum_ty) in env.enums:
                    enum_q = env.resolve_alias(enum_ty)
                    if variant_name not in env.enums[enum_q]:
                        diags.error(line_no, 1, f"unknown variant `{variant_name}` for enum `{enum_q}`")
                    else:
                        payload = env.enums[enum_q][variant_name]
                        # bind payload names if present
                        if "(" in variant_name:
                            # not expected; handled above
                            pass
                        if "(" in pattern_raw and pattern_raw.endswith(")"):
                            payload_section = pattern_raw.split("(", 1)[1][:-1]
                            if payload_section.strip():
                                payload_names = [p.strip() for p in payload_section.split(",")]
                            else:
                                payload_names = []
                        else:
                            payload_names = []
                        if payload_names and len(payload_names) != len(payload):
                            diags.error(line_no, 1, f"variant `{variant_name}` expects {len(payload)} bindings, got {len(payload_names)}")
                        for idx, (field_name, field_ty) in enumerate(payload):
                            if idx < len(payload_names):
                                locals_env[-1][payload_names[idx]] = (field_ty, False)
            else:
                # simple binding pattern
                locals_env[-1][pattern_raw] = (scrut_ty, False)
            block_stack.append("match-arm")
            line_idx += 1
            continue

        result = check_statement(stripped, env, locals_env, diags, line_no, current_fn)
        if result == "push":
            block_stack.append("block")
        line_idx += 1

    return tokens, env, diags


# -----------------------------------------------------------------------------
# Entrée CLI
# -----------------------------------------------------------------------------


def main():
    parser = argparse.ArgumentParser(description="vittec-stage1 partial compiler")
    parser.add_argument("input", nargs="?", help="Path to Muffin/Vitte source")
    parser.add_argument(
        "--dump-json",
        action="store_true",
        help="Dump parsed module/types as JSON (debug helper).",
    )
    args = parser.parse_args()

    if not args.input:
        sys.stderr.write("diag:error:missing-input-path\n")
        return 1

    path = Path(args.input)
    if not path.exists():
        sys.stderr.write(f"diag:error:file-not-found:{path}\n")
        return 1

    text = path.read_text(encoding="utf-8", errors="ignore")
    if not text.strip():
        sys.stderr.write(f"diag:error:empty-source:{path}\n")
        return 1

    tokens = tokenize(text)
    diags = DiagSink()
    if path.suffix == ".vitte":
        tokens, env, diags = parse_source(text, diags)
    else:
        env = TypeEnv("root")

    lines = len(text.splitlines())
    sys.stdout.write(f"[stage1] lexed-tokens={len(tokens)} lines={lines}\n")

    if args.dump_json:
        data = {
            "module": env.module,
            "types": sorted(env.types.keys()),
            "functions": sorted(env.functions.keys()),
            "diagnostics": [d.__dict__ for d in diags.items],
        }
        sys.stdout.write(json.dumps(data, indent=2))
        sys.stdout.write("\n")
    else:
        # preview first tokens
        preview = []
        for tok in tokens:
            if tok.kind == "newline":
                continue
            preview.append(f"L{tok.line}:{tok.value}")
            if len(preview) >= 6:
                break
        sys.stdout.write(f"[stage1] ast-preview={'; '.join(preview)}\n")
        if diags.items:
            for d in diags.items:
                sys.stdout.write(d.format() + "\n")
        else:
            sys.stdout.write("[typecheck] ok\n")

    return 0 if not diags.has_errors() else 1


if __name__ == "__main__":
    sys.exit(main())
