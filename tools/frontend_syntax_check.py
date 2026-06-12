#!/usr/bin/env python3
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import argparse
import json
import sys


KEYWORDS = {
    "space",
    "use",
    "export",
    "const",
    "static",
    "global",
    "type",
    "opaque",
    "extern",
    "form",
    "class",
    "union",
    "bits",
    "pick",
    "flags",
    "trait",
    "impl",
    "proc",
    "intrinsic",
    "compiler",
    "query",
    "pass",
    "backend",
    "diagnostic",
    "macro",
    "comptime",
    "static_assert",
    "test",
    "bench",
    "entry",
    "pub",
    "priv",
    "unsafe",
    "async",
    "inline",
    "noinline",
    "naked",
    "interrupt",
    "where",
    "effects",
    "requires",
    "operator",
    "as",
    "for",
    "at",
    "let",
    "set",
    "give",
    "try",
    "defer",
    "asm",
    "emit",
    "assert",
    "panic",
    "unreachable",
    "if",
    "elif",
    "else",
    "while",
    "loop",
    "in",
    "break",
    "continue",
    "select",
    "when",
    "match",
    "case",
    "with",
    "critical",
    "mut",
    "owned",
    "borrow",
    "move",
    "await",
    "not",
    "and",
    "or",
    "is",
    "self",
    "ref",
    "dyn",
    "impl",
    "true",
    "false",
    "null",
    "sizeof",
    "alignof",
    "offsetof",
    "typeof",
    "nameof",
    "map",
    "resource",
    "async",
    "const",
    "volatile",
    "atomic",
    "user",
    "kernel",
    "phys",
    "mmio",
    "dma",
    "noexcept",
}

DECL_KEYWORDS = {
    "space",
    "use",
    "export",
    "const",
    "static",
    "global",
    "type",
    "opaque",
    "extern",
    "form",
    "class",
    "union",
    "bits",
    "pick",
    "flags",
    "trait",
    "impl",
    "proc",
    "intrinsic",
    "compiler",
    "query",
    "pass",
    "backend",
    "diagnostic",
    "macro",
    "comptime",
    "static_assert",
    "test",
    "bench",
    "entry",
}

PROC_MODIFIERS = {"async", "unsafe", "const", "inline", "noinline", "naked", "interrupt"}
VISIBILITY = {"pub", "priv"}
TYPE_QUALIFIERS = {"const", "volatile", "atomic", "mut", "owned", "borrow", "user", "kernel", "phys", "mmio", "dma"}
PRIMITIVE_TYPES = {
    "void",
    "never",
    "unit",
    "bool",
    "char",
    "rune",
    "str",
    "string",
    "bytes",
    "cstr",
    "int",
    "i8",
    "i16",
    "i32",
    "i64",
    "i128",
    "u8",
    "u16",
    "u32",
    "u64",
    "u128",
    "usize",
    "isize",
    "intptr",
    "uintptr",
    "f16",
    "f32",
    "f64",
    "f128",
    "c_char",
    "c_int",
    "c_uint",
    "c_long",
    "c_ulong",
    "c_void",
    "TokenId",
    "NodeId",
    "DefId",
    "HirId",
    "MirId",
    "TypeId",
    "SymbolId",
    "ScopeId",
    "BlockId",
    "ValueId",
    "InstrId",
    "Self",
}


@dataclass
class Token:
    kind: str
    text: str
    line: int
    column: int


@dataclass
class Diagnostic:
    code: str
    message: str
    line: int
    column: int

    def as_dict(self) -> dict[str, object]:
        return {
            "code": self.code,
            "message": self.message,
            "line": self.line,
            "column": self.column,
        }


class Lexer:
    def __init__(self, text: str):
        self.text = text
        self.i = 0
        self.line = 1
        self.col = 1

    def eof(self) -> bool:
        return self.i >= len(self.text)

    def peek(self, n: int = 0) -> str:
        j = self.i + n
        if j >= len(self.text):
            return ""
        return self.text[j]

    def bump(self) -> str:
        ch = self.peek()
        if not ch:
            return ""
        self.i += 1
        if ch == "\n":
            self.line += 1
            self.col = 1
        else:
            self.col += 1
        return ch

    def skip_ws_and_comments(self) -> None:
        while not self.eof():
            ch = self.peek()
            two = self.peek() + self.peek(1)
            three = two + self.peek(2)
            if ch in " \t\r\n":
                self.bump()
                continue
            if ch == "#":
                while not self.eof() and self.peek() != "\n":
                    self.bump()
                continue
            if two == "//":
                while not self.eof() and self.peek() != "\n":
                    self.bump()
                continue
            if two == "/*":
                self.bump()
                self.bump()
                while not self.eof() and (self.peek() + self.peek(1)) != "*/":
                    self.bump()
                if not self.eof():
                    self.bump()
                    self.bump()
                continue
            if three == "<<<":
                self.bump()
                self.bump()
                self.bump()
                while not self.eof() and (self.peek() + self.peek(1) + self.peek(2)) != ">>>":
                    self.bump()
                if not self.eof():
                    self.bump()
                    self.bump()
                    self.bump()
                continue
            break

    def read_ident(self) -> Token:
        line, col = self.line, self.col
        buf = []
        while True:
            ch = self.peek()
            if ch.isalnum() or ch == "_":
                buf.append(self.bump())
            else:
                break
        text = "".join(buf)
        kind = "keyword" if text in KEYWORDS else "ident"
        return Token(kind, text, line, col)

    def read_number(self) -> Token:
        line, col = self.line, self.col
        buf = []
        while True:
            ch = self.peek()
            if ch.isdigit() or ch in "._xobabcdefABCDEF+-":
                buf.append(self.bump())
            else:
                break
        return Token("number", "".join(buf), line, col)

    def read_string(self) -> Token:
        quote = self.bump()
        line, col = self.line, self.col - 1
        buf = [quote]
        while not self.eof():
            ch = self.bump()
            buf.append(ch)
            if ch == "\\":
                if not self.eof():
                    buf.append(self.bump())
                continue
            if ch == quote:
                break
        return Token("string", "".join(buf), line, col)

    def next_token(self) -> Token:
        self.skip_ws_and_comments()
        if self.eof():
            return Token("eof", "", self.line, self.col)
        ch = self.peek()
        line, col = self.line, self.col
        if ch.isalpha() or ch == "_":
            return self.read_ident()
        if ch.isdigit() or (ch == "-" and self.peek(1).isdigit()):
            return self.read_number()
        if ch in "\"'":
            return self.read_string()
        for sym in (
            "...",
            "..=",
            "->",
            "::",
            "..",
            "==",
            "!=",
            "<=",
            ">=",
            "<<=",
            ">>=",
            "<<",
            ">>",
            "+=",
            "-=",
            "*=",
            "/=",
            "%=",
            "&=",
            "|=",
            "^=",
            "??",
            "=>",
            "#[",
            "#!",
        ):
            if self.text.startswith(sym, self.i):
                for _ in sym:
                    self.bump()
                return Token("symbol", sym, line, col)
        self.bump()
        return Token("symbol", ch, line, col)


class Parser:
    def __init__(self, tokens: list[Token]):
        self.tokens = tokens
        self.i = 0
        self.diagnostics: list[Diagnostic] = []
        self.max_diags = 64
        self.stmt_count = 0
        self.expr_count = 0
        self.type_count = 0
        self.pattern_count = 0
        self.decl_count = 0

    def current(self) -> Token:
        return self.tokens[min(self.i, len(self.tokens) - 1)]

    def peek(self, n: int = 1) -> Token:
        return self.tokens[min(self.i + n, len(self.tokens) - 1)]

    def advance(self) -> Token:
        tok = self.current()
        if self.i < len(self.tokens):
            self.i += 1
        return tok

    def at(self, text: str) -> bool:
        return self.current().text == text

    def at_any(self, texts: set[str]) -> bool:
        return self.current().text in texts

    def at_kind(self, kind: str) -> bool:
        return self.current().kind == kind

    def error(self, code: str, message: str, tok: Token | None = None) -> None:
        if len(self.diagnostics) >= self.max_diags:
            return
        tok = tok or self.current()
        self.diagnostics.append(Diagnostic(code, message, tok.line, tok.column))

    def expect(self, text: str, code: str) -> bool:
        if self.at(text):
            self.advance()
            return True
        self.error(code, f"expected `{text}`")
        return False

    def optional(self, text: str) -> bool:
        if self.at(text):
            self.advance()
            return True
        return False

    def expect_ident(self, code: str) -> bool:
        if self.at_kind("ident") or self.at_kind("keyword"):
            self.advance()
            return True
        self.error(code, "expected identifier")
        return False

    def at_literal(self) -> bool:
        return self.at_kind("number") or self.at_kind("string") or self.at_any({"true", "false", "null"})

    def at_path_start(self) -> bool:
        return self.at_kind("ident") or self.at_kind("keyword")

    def maybe_recover(self, boundary: set[str] | None = None) -> None:
        if boundary is None:
            boundary = {";", "}", "]", ")", ","}
        while not self.at_kind("eof"):
            if self.current().text in boundary:
                return
            self.advance()

    def path_starts_struct_literal(self) -> bool:
        if not self.at("{"):
            return False
        first = self.peek()
        second = self.peek(2)
        if first.text in {"}", ".."}:
            return True
        if first.kind not in {"ident", "keyword"}:
            return False
        return second.text in {":", ",", "}"}

    def skip_balanced(self, opener: str, closer: str, code: str) -> bool:
        if not self.expect(opener, code):
            return False
        depth = 1
        while depth > 0 and not self.at_kind("eof"):
            tok = self.advance()
            if tok.text == opener:
                depth += 1
            elif tok.text == closer:
                depth -= 1
        if depth != 0:
            self.error("P000_UNBALANCED", f"unclosed `{opener}` block")
            return False
        return True

    def skip_until(self, boundary: set[str]) -> None:
        depth_paren = 0
        depth_bracket = 0
        depth_brace = 0
        while not self.at_kind("eof"):
            tok = self.current().text
            if depth_paren == 0 and depth_bracket == 0 and depth_brace == 0 and tok in boundary:
                return
            if tok == "(":
                depth_paren += 1
            elif tok == ")":
                if depth_paren == 0 and ")" in boundary:
                    return
                depth_paren = max(0, depth_paren - 1)
            elif tok == "[":
                depth_bracket += 1
            elif tok == "]":
                if depth_bracket == 0 and "]" in boundary:
                    return
                depth_bracket = max(0, depth_bracket - 1)
            elif tok == "{":
                depth_brace += 1
            elif tok == "}":
                if depth_brace == 0 and "}" in boundary:
                    return
                depth_brace = max(0, depth_brace - 1)
            self.advance()

    def parse_attrs(self) -> None:
        while self.at("#[") or self.at("#!"):
            self.advance()
            self.parse_path()
            if self.at("("):
                self.skip_balanced("(", ")", "PATTR001")
            self.expect("]", "PATTR002")

    def parse_path(self) -> bool:
        if not self.expect_ident("PPATH001"):
            return False
        while self.at("/") or self.at(".") or self.at("::"):
            if self.at(".") and self.peek().text in {"{", "*"}:
                break
            self.advance()
            self.expect_ident("PPATH002")
        return True

    def parse_module_path(self) -> bool:
        while self.at("."):
            self.advance()
        return self.parse_path()

    def parse_ident_list(self) -> None:
        self.expect_ident("PLIST001")
        while self.at(","):
            self.advance()
            if self.at("}") or self.at(")"):
                break
            self.expect_ident("PLIST002")

    def parse_generic_params(self) -> None:
        if not self.at("["):
            return
        self.advance()
        if self.at("]"):
            self.advance()
            return
        while not self.at("]") and not self.at_kind("eof"):
            if self.at("const") or self.at("comptime"):
                self.advance()
                self.expect_ident("PGEN001")
                self.expect(":", "PGEN002")
                self.parse_type_expr()
            else:
                self.expect_ident("PGEN003")
                if self.at(":"):
                    self.advance()
                    self.parse_type_expr()
                if self.at("="):
                    self.advance()
                    self.parse_type_expr()
            if not self.optional(","):
                break
        self.expect("]", "PGEN004")

    def parse_where_clause(self) -> None:
        if not self.optional("where"):
            return
        while not self.at_kind("eof") and not self.at("{") and not self.at(";"):
            if self.at(","):
                self.advance()
                continue
            self.parse_type_expr()
            if self.at(":") or self.at("=="):
                self.advance()
                self.parse_type_expr()
            elif self.at_any({"<", "<=", ">", ">="}):
                self.advance()
                self.parse_expr()
            else:
                break

    def parse_proc_modifiers(self) -> None:
        while self.at_any(VISIBILITY | PROC_MODIFIERS | {"extern"}):
            tok = self.current().text
            self.advance()
            if tok == "extern":
                if self.at_kind("string") or self.at_any({"C", "sysv64", "win64", "interrupt", "naked"}):
                    self.advance()
            elif tok == "pub" and self.at("("):
                self.skip_balanced("(", ")", "PVIS001")

    def parse_visibility_prefix(self) -> None:
        if self.at_any(VISIBILITY):
            tok = self.current().text
            self.advance()
            if tok == "pub" and self.at("("):
                self.skip_balanced("(", ")", "PVIS_PREFIX001")

    def starts_proc_decl(self) -> bool:
        if self.at("proc") or self.at_any(PROC_MODIFIERS | {"extern"}):
            return True
        if self.at_any(VISIBILITY):
            return self.peek().text in PROC_MODIFIERS | {"extern", "proc"}
        return False

    def parse_proc_name(self) -> None:
        if self.optional("operator"):
            if not self.at_kind("eof"):
                self.advance()
            return
        self.expect_ident("PPROC_NAME")

    def parse_param(self) -> None:
        if self.at("..."):
            self.advance()
            return
        if self.optional("&"):
            self.optional("mut")
            if self.optional("self"):
                return
            self.parse_pattern()
        elif self.optional("mut") or self.optional("owned") or self.optional("borrow") or self.optional("move"):
            self.parse_pattern()
        elif self.optional("self"):
            return
        else:
            self.parse_pattern()
        if self.at(":"):
            self.advance()
            if self.at("..."):
                self.advance()
                self.parse_type_expr()
            else:
                self.parse_type_expr()
        if self.at("="):
            self.advance()
            self.parse_expr()

    def parse_param_list(self) -> None:
        if self.at(")"):
            return
        while not self.at(")") and not self.at_kind("eof"):
            self.parse_param()
            if not self.optional(","):
                break

    def parse_lambda_param_list(self) -> None:
        if self.at("|"):
            return
        while not self.at("|") and not self.at_kind("eof"):
            self.parse_pattern_atom()
            if self.at(":"):
                self.advance()
                self.parse_type_expr()
            if not self.optional(","):
                break

    def parse_proc_suffixes(self) -> None:
        while True:
            if self.optional("effects") or self.optional("requires"):
                self.expect("(", "PSFX001")
                self.skip_until({")"})
                self.expect(")", "PSFX002")
                continue
            if self.at("where"):
                self.parse_where_clause()
                continue
            if self.optional("noexcept"):
                continue
            break

    def parse_proc_signature_core(self) -> None:
        self.parse_proc_name()
        self.parse_generic_params()
        self.expect("(", "PPROC001")
        self.parse_param_list()
        self.expect(")", "PPROC002")
        if self.at("->"):
            self.advance()
            self.parse_type_expr()
        self.parse_proc_suffixes()

    def parse_proc(self) -> None:
        self.parse_proc_modifiers()
        self.expect("proc", "PPROC003")
        self.parse_proc_signature_core()
        if self.at("{"):
            self.parse_block()
        else:
            self.expect(";", "PPROC004")

    def parse_block(self) -> None:
        if not self.expect("{", "PBLOCK001"):
            return
        while not self.at("}") and not self.at_kind("eof"):
            self.parse_stmt()
        self.expect("}", "PBLOCK002")

    def parse_stmt_end(self, code: str) -> None:
        self.expect(";", code)

    def parse_stmt(self) -> None:
        self.stmt_count += 1
        self.parse_attrs()
        if self.at("const"):
            self.advance()
            self.expect_ident("PSTMT_CONST001")
            if self.at(":"):
                self.advance()
                self.parse_type_expr()
            self.expect("=", "PSTMT_CONST002")
            self.parse_expr()
            self.parse_stmt_end("PSTMT_CONST003")
            return
        if self.at("let"):
            self.advance()
            self.optional("mut")
            self.parse_pattern()
            if self.at(":"):
                self.advance()
                self.parse_type_expr()
            self.expect("=", "PSTMT_LET001")
            self.parse_expr()
            self.parse_stmt_end("PSTMT_LET002")
            return
        if self.at("set"):
            self.advance()
            self.parse_unary_expr()
            if self.at_any({"=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>="}):
                self.advance()
                self.parse_expr()
            else:
                self.error("PSTMT_SET001", "expected assignment operator")
            self.parse_stmt_end("PSTMT_SET002")
            return
        if self.at("give"):
            self.advance()
            if not self.at(";"):
                self.parse_expr()
            self.parse_stmt_end("PSTMT_GIVE001")
            return
        if self.at("try"):
            self.advance()
            self.parse_expr()
            self.parse_stmt_end("PSTMT_TRY001")
            return
        if self.at("defer"):
            self.advance()
            self.parse_block()
            return
        if self.at("asm"):
            self.advance()
            self.expect("(", "PSTMT_ASM001")
            self.skip_until({")"})
            self.expect(")", "PSTMT_ASM002")
            self.parse_stmt_end("PSTMT_ASM003")
            return
        if self.at("unsafe"):
            self.advance()
            self.parse_block()
            return
        if self.at("emit"):
            self.advance()
            self.parse_expr()
            self.parse_stmt_end("PSTMT_EMIT001")
            return
        if self.at("assert"):
            self.advance()
            self.parse_expr()
            if self.at(","):
                self.advance()
                self.parse_expr()
            self.parse_stmt_end("PSTMT_ASSERT001")
            return
        if self.at("panic"):
            self.advance()
            self.parse_expr()
            self.parse_stmt_end("PSTMT_PANIC001")
            return
        if self.at("unreachable"):
            self.advance()
            self.parse_stmt_end("PSTMT_UNREACH001")
            return
        if self.at("if"):
            self.parse_if_expr(is_stmt=True)
            return
        if self.at("while"):
            self.advance()
            self.parse_expr()
            self.parse_block()
            return
        if self.at("loop"):
            self.advance()
            self.parse_block()
            return
        if self.at("for"):
            self.advance()
            self.parse_pattern()
            self.expect("in", "PSTMT_FOR001")
            self.parse_expr()
            self.parse_block()
            return
        if self.at("break"):
            self.advance()
            if not self.at(";"):
                self.parse_expr()
            self.parse_stmt_end("PSTMT_BREAK001")
            return
        if self.at("continue"):
            self.advance()
            self.parse_stmt_end("PSTMT_CONTINUE001")
            return
        if self.at("select"):
            self.advance()
            self.parse_expr()
            self.expect("{", "PSTMT_SELECT001")
            while self.at("when"):
                self.advance()
                self.parse_pattern()
                self.parse_block()
            if self.at("else"):
                self.advance()
                self.parse_block()
            self.expect("}", "PSTMT_SELECT002")
            return
        if self.at("match"):
            self.parse_match_expr(is_stmt=True)
            return
        if self.at("when"):
            self.advance()
            self.parse_unary_expr()
            self.expect("is", "PSTMT_WHEN001")
            self.parse_pattern()
            self.parse_block()
            return
        if self.at("with"):
            self.advance()
            self.parse_expr()
            if self.at("as"):
                self.advance()
                self.parse_pattern()
            self.parse_block()
            return
        if self.at("critical"):
            self.advance()
            self.parse_block()
            return
        self.parse_expr()
        self.parse_stmt_end("PSTMT_EXPR001")

    def parse_expr(self) -> None:
        self.expr_count += 1
        self.parse_assign_expr()

    def parse_assign_expr(self) -> None:
        self.parse_ternary_expr()
        if self.at_any({"=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>="}):
            self.advance()
            self.parse_assign_expr()

    def parse_ternary_expr(self) -> None:
        self.parse_coalesce_expr()
        if self.at("?"):
            self.advance()
            self.parse_expr()
            self.expect(":", "PEXPR_TERN001")
            self.parse_expr()

    def parse_coalesce_expr(self) -> None:
        self.parse_range_expr()
        while self.at("??"):
            self.advance()
            self.parse_range_expr()

    def parse_range_expr(self) -> None:
        self.parse_bin_chain(self.parse_or_expr, {"..", "..="})

    def parse_or_expr(self) -> None:
        self.parse_bin_chain(self.parse_and_expr, {"or", "||"})

    def parse_and_expr(self) -> None:
        self.parse_bin_chain(self.parse_bit_or_expr, {"and", "&&"})

    def parse_bit_or_expr(self) -> None:
        self.parse_bin_chain(self.parse_bit_xor_expr, {"|"})

    def parse_bit_xor_expr(self) -> None:
        self.parse_bin_chain(self.parse_bit_and_expr, {"^"})

    def parse_bit_and_expr(self) -> None:
        self.parse_bin_chain(self.parse_eq_expr, {"&"})

    def parse_eq_expr(self) -> None:
        self.parse_bin_chain(self.parse_rel_expr, {"==", "!="})

    def parse_rel_expr(self) -> None:
        self.parse_shift_expr()
        while self.at_any({"<", "<=", ">", ">=", "in"}):
            self.advance()
            self.parse_shift_expr()
        if self.at("not") and self.peek().text == "in":
            self.advance()
            self.advance()
            self.parse_shift_expr()

    def parse_shift_expr(self) -> None:
        self.parse_bin_chain(self.parse_add_expr, {"<<", ">>"})

    def parse_add_expr(self) -> None:
        self.parse_bin_chain(self.parse_mul_expr, {"+", "-"})

    def parse_mul_expr(self) -> None:
        self.parse_bin_chain(self.parse_cast_expr, {"*", "/", "%"})

    def parse_bin_chain(self, leaf, operators: set[str]) -> None:
        leaf()
        while self.at_any(operators):
            self.advance()
            leaf()

    def parse_cast_expr(self) -> None:
        self.parse_unary_expr()
        while self.at("as") or self.at("is"):
            if self.optional("as"):
                self.parse_type_expr()
            else:
                self.advance()
                if self.optional("not"):
                    self.parse_pattern()
                else:
                    self.parse_pattern()

    def parse_unary_expr(self) -> None:
        if self.at_any({"not", "-", "~", "*", "&", "move", "await"}):
            self.advance()
            self.parse_unary_expr()
            return
        self.parse_postfix_expr()

    def parse_postfix_expr(self) -> None:
        self.parse_primary()
        while True:
            if self.at("("):
                self.advance()
                self.parse_arg_list(")")
                self.expect(")", "PEXPR_CALL001")
                continue
            if self.at("."):
                self.advance()
                if self.at("await"):
                    self.advance()
                else:
                    self.expect_ident("PEXPR_MEMBER001")
                continue
            if self.at("["):
                self.advance()
                if not self.at("]"):
                    self.parse_expr()
                    if self.at(":"):
                        self.advance()
                        if not self.at("]") and not self.at(":"):
                            self.parse_expr()
                        if self.at(":"):
                            self.advance()
                            self.parse_expr()
                self.expect("]", "PEXPR_INDEX001")
                continue
            if self.at("?"):
                self.advance()
                continue
            break

    def parse_primary(self) -> None:
        if self.at_literal():
            self.advance()
            return
        if self.at("b") and self.peek().kind == "string":
            self.advance()
            self.advance()
            return
        if self.at("sizeof") or self.at("alignof") or self.at("offsetof") or self.at("typeof") or self.at("nameof"):
            self.advance()
            self.expect("(", "PBUILTIN001")
            if self.at("offsetof"):
                self.parse_type_expr()
                self.expect(",", "PBUILTIN002")
                self.expect_ident("PBUILTIN003")
            elif self.at("alignof"):
                self.parse_type_expr()
            elif self.at("nameof"):
                self.parse_path()
            else:
                self.parse_expr()
            self.expect(")", "PBUILTIN004")
            return
        if self.at("if"):
            self.parse_if_expr(is_stmt=False)
            return
        if self.at("match"):
            self.parse_match_expr(is_stmt=False)
            return
        if self.at("unsafe"):
            self.advance()
            self.parse_block()
            return
        if self.at("proc"):
            self.advance()
            self.expect("(", "PPROC_EXPR001")
            self.parse_param_list()
            self.expect(")", "PPROC_EXPR002")
            if self.at("->"):
                self.advance()
                self.parse_type_expr()
            self.parse_proc_suffixes()
            self.parse_block()
            return
        if self.at("|"):
            self.advance()
            if not self.at("|"):
                self.parse_lambda_param_list()
            self.expect("|", "PLAMBDA001")
            if self.at("{"):
                self.parse_block()
            else:
                self.parse_expr()
            return
        if self.at("map"):
            self.advance()
            self.expect("{", "PMAP001")
            if not self.at("}"):
                while True:
                    self.parse_expr()
                    self.expect(":", "PMAP002")
                    self.parse_expr()
                    if not self.optional(","):
                        break
            self.expect("}", "PMAP003")
            return
        if self.at("resource"):
            self.advance()
            self.expect("{", "PRES001")
            if not self.at("}"):
                while True:
                    self.expect_ident("PRES002")
                    self.expect(":", "PRES003")
                    self.parse_expr()
                    if not self.optional(","):
                        break
            self.expect("}", "PRES004")
            return
        if self.at("set") and self.peek().text == "{":
            self.advance()
            self.expect("{", "PSET001")
            if not self.at("}"):
                self.parse_arg_list("}")
            self.expect("}", "PSET002")
            return
        if self.at("["):
            self.advance()
            if not self.at("]"):
                self.parse_expr()
                if self.at("for"):
                    self.advance()
                    self.parse_pattern()
                    self.expect("in", "PLISTCOMP001")
                    self.parse_expr()
                    if self.at("if"):
                        self.advance()
                        self.parse_expr()
                else:
                    while self.optional(","):
                        if self.at("]"):
                            break
                        self.parse_expr()
            self.expect("]", "PLIST001")
            return
        if self.at("{"):
            self.parse_block()
            return
        if self.at("("):
            self.advance()
            if not self.at(")"):
                self.parse_expr()
                if self.at(","):
                    while self.optional(","):
                        if self.at(")"):
                            break
                        self.parse_expr()
                else:
                    pass
            self.expect(")", "PPRIMARY001")
            return
        if self.at_path_start():
            self.parse_path()
            if self.path_starts_struct_literal():
                self.advance()
                if not self.at("}"):
                    while True:
                        if self.at(".."):
                            self.advance()
                            self.parse_expr()
                        else:
                            self.expect_ident("PSTRUCT001")
                            if self.at(":"):
                                self.advance()
                                self.parse_expr()
                        if not self.optional(","):
                            break
                self.expect("}", "PSTRUCT002")
            return
        self.error("PPRIMARY999", f"unexpected expression token `{self.current().text}`")
        self.advance()

    def parse_arg_list(self, closer: str) -> None:
        if self.at(closer):
            return
        while not self.at(closer) and not self.at_kind("eof"):
            if self.at_path_start() and self.peek().text == ":":
                self.expect_ident("PARG001")
                self.expect(":", "PARG002")
                self.parse_expr()
            else:
                self.parse_expr()
            if not self.optional(","):
                break

    def parse_if_expr(self, is_stmt: bool) -> None:
        self.expect("if", "PIF001")
        self.parse_expr()
        self.parse_block()
        while self.at("elif"):
            self.advance()
            self.parse_expr()
            self.parse_block()
        if self.at("else"):
            self.advance()
            self.parse_block()
        if not is_stmt:
            return

    def parse_match_expr(self, is_stmt: bool) -> None:
        self.expect("match", "PMATCH001")
        self.parse_expr()
        self.expect("{", "PMATCH002")
        while self.at("case"):
            self.advance()
            self.parse_pattern()
            if self.at("if"):
                self.advance()
                self.parse_expr()
            if self.at("=>"):
                self.advance()
                self.parse_expr()
                self.expect(";", "PMATCH003")
            else:
                self.parse_block()
        if self.at("else"):
            self.advance()
            if self.at("=>"):
                self.advance()
                self.parse_expr()
                self.expect(";", "PMATCH004")
            else:
                self.parse_block()
        self.expect("}", "PMATCH005")
        if not is_stmt:
            return

    def parse_pattern(self) -> None:
        self.pattern_count += 1
        self.parse_pattern_atom()
        while self.at("|"):
            self.advance()
            self.parse_pattern_atom()

    def parse_pattern_atom(self) -> None:
        if self.at("_"):
            self.advance()
            return
        if self.at_literal():
            self.advance()
            if self.at("..") or self.at("..="):
                self.advance()
                if self.at_literal():
                    self.advance()
                else:
                    self.error("PPAT_RANGE001", "expected range end literal")
            return
        if self.at("mut") or self.at("ref"):
            self.advance()
            self.expect_ident("PPAT_BIND001")
            return
        if self.at("["):
            self.advance()
            if not self.at("]"):
                while True:
                    self.parse_pattern()
                    if not self.optional(","):
                        break
            self.expect("]", "PPAT_LIST001")
            return
        if self.at("("):
            self.advance()
            self.parse_pattern()
            self.expect(",", "PPAT_TUPLE001")
            while not self.at(")") and not self.at_kind("eof"):
                self.parse_pattern()
                if not self.optional(","):
                    break
            self.expect(")", "PPAT_TUPLE002")
            return
        if self.at_path_start():
            self.parse_path()
            if self.at("("):
                self.advance()
                if not self.at(")"):
                    while True:
                        self.parse_pattern()
                        if not self.optional(","):
                            break
                self.expect(")", "PPAT_CTOR001")
            elif self.at("{"):
                self.advance()
                if not self.at("}"):
                    while True:
                        if self.at(".."):
                            self.advance()
                        else:
                            self.expect_ident("PPAT_STRUCT001")
                            if self.at(":"):
                                self.advance()
                                self.parse_pattern()
                        if not self.optional(","):
                            break
                self.expect("}", "PPAT_STRUCT002")
            return
        self.error("PPAT999", f"unexpected pattern token `{self.current().text}`")
        self.advance()

    def parse_type_expr(self) -> None:
        self.type_count += 1
        self.parse_type_prefix()
        while self.at("|"):
            self.advance()
            self.parse_type_prefix()

    def parse_type_prefix(self) -> None:
        while self.at_any(TYPE_QUALIFIERS):
            self.advance()
        if self.at("&"):
            self.advance()
            if self.at("'"):
                self.advance()
                self.expect_ident("PTYPE_LT001")
            self.optional("mut")
            self.parse_type_expr()
            return
        if self.at("*"):
            self.advance()
            if self.at_any({"const", "mut", "volatile", "user", "kernel", "phys", "mmio", "dma"}):
                self.advance()
            self.parse_type_expr()
            return
        if self.at("?"):
            self.advance()
            self.parse_type_expr()
            return
        self.parse_type_atom()

    def parse_type_atom(self) -> None:
        if self.at("["):
            self.advance()
            self.parse_type_expr()
            if self.at(";"):
                self.advance()
                self.parse_expr()
            self.expect("]", "PTYPE_ARRAY001")
            return
        if self.at("("):
            self.advance()
            self.parse_type_expr()
            self.expect(",", "PTYPE_TUPLE001")
            while not self.at(")") and not self.at_kind("eof"):
                self.parse_type_expr()
                if not self.optional(","):
                    break
            self.expect(")", "PTYPE_TUPLE002")
            return
        if self.at("proc"):
            self.advance()
            self.expect("(", "PTYPE_PROC001")
            if not self.at(")"):
                while True:
                    if self.at_path_start() and self.peek().text == ":":
                        self.expect_ident("PTYPE_PROC002")
                        self.expect(":", "PTYPE_PROC003")
                    self.parse_type_expr()
                    if not self.optional(","):
                        break
            self.expect(")", "PTYPE_PROC004")
            if self.at("->"):
                self.advance()
                self.parse_type_expr()
            return
        if self.at("dyn") or self.at("impl"):
            self.advance()
            self.parse_type_expr()
            while self.at("+"):
                self.advance()
                self.parse_type_expr()
            return
        if self.at_path_start():
            self.parse_path()
            if self.at("["):
                self.advance()
                if not self.at("]"):
                    while True:
                        if self.at_path_start() and self.peek().text == ":":
                            self.expect_ident("PTYPE_ARG001")
                            self.expect(":", "PTYPE_ARG002")
                        self.parse_type_expr()
                        if not self.optional(","):
                            break
                self.expect("]", "PTYPE_ARG003")
            return
        if self.at_any(PRIMITIVE_TYPES):
            self.advance()
            return
        self.error("PTYPE999", f"unexpected type token `{self.current().text}`")
        self.advance()

    def parse_field_item(self) -> None:
        self.parse_attrs()
        if self.at_any(VISIBILITY):
            self.advance()
            if self.at("("):
                self.skip_balanced("(", ")", "PFIELD_VIS001")
        self.expect_ident("PFIELD001")
        self.expect(":", "PFIELD002")
        self.parse_type_expr()
        if self.at("="):
            self.advance()
            self.parse_expr()

    def parse_form_body(self) -> None:
        self.expect("{", "PFORM001")
        while not self.at("}") and not self.at_kind("eof"):
            self.parse_attrs()
            if self.starts_proc_decl():
                self.parse_proc()
            elif self.at("const") or self.at("static") or self.at("global"):
                self.parse_const_like_decl(require_semi=False)
            elif self.at("type"):
                self.parse_type_alias_decl(require_semi=False)
            else:
                self.parse_field_item()
                self.optional(",")
        self.expect("}", "PFORM002")

    def parse_trait_body(self) -> None:
        self.expect("{", "PTRAIT001")
        while not self.at("}") and not self.at_kind("eof"):
            self.parse_attrs()
            if self.starts_proc_decl():
                self.parse_proc()
            elif self.at("const") or self.at("static") or self.at("global"):
                self.parse_const_like_decl(require_semi=False)
            elif self.at("type"):
                self.advance()
                self.expect_ident("PTRAIT_TYPE001")
                if self.at(":"):
                    self.advance()
                    self.parse_type_expr()
                if self.at("="):
                    self.advance()
                    self.parse_type_expr()
                self.expect(";", "PTRAIT_TYPE002")
            else:
                self.error("PTRAIT999", f"unexpected trait item `{self.current().text}`")
                self.maybe_recover({";", "}"})
                self.optional(";")
        self.expect("}", "PTRAIT002")

    def parse_const_like_decl(self, require_semi: bool = True) -> None:
        self.advance()
        self.expect_ident("PDECL_CONST001")
        if self.at("["):
            self.parse_generic_params()
        if self.at(":"):
            self.advance()
            self.parse_type_expr()
        self.expect("=", "PDECL_CONST002")
        self.parse_expr()
        if require_semi:
            self.optional(";")

    def parse_type_alias_decl(self, require_semi: bool = True) -> None:
        self.expect("type", "PDECL_TYPE001")
        self.expect_ident("PDECL_TYPE002")
        self.parse_generic_params()
        self.expect("=", "PDECL_TYPE003")
        self.parse_type_expr()
        if require_semi:
            self.optional(";")

    def parse_impl_body(self) -> None:
        self.expect("{", "PIMPL001")
        while not self.at("}") and not self.at_kind("eof"):
            self.parse_attrs()
            if self.starts_proc_decl():
                self.parse_proc()
            elif self.at("const") or self.at("static") or self.at("global"):
                self.parse_const_like_decl(require_semi=False)
            elif self.at("type"):
                self.parse_type_alias_decl(require_semi=False)
            else:
                self.error("PIMPL999", f"unexpected impl item `{self.current().text}`")
                self.maybe_recover({";", "}"})
                self.optional(";")
        self.expect("}", "PIMPL002")

    def parse_extern_decl(self) -> None:
        self.expect("extern", "PEXTERN001")
        if self.optional("type"):
            self.expect_ident("PEXTERN002")
            self.parse_generic_params()
            self.optional(";")
            return
        if self.at_kind("string") or self.at_any({"C", "sysv64", "win64", "interrupt", "naked"}):
            self.advance()
            self.expect("{", "PEXTERN003")
            while not self.at("}") and not self.at_kind("eof"):
                self.parse_attrs()
                if self.at("proc"):
                    self.parse_proc()
                elif self.at("const"):
                    self.parse_const_like_decl(require_semi=False)
                elif self.at("extern"):
                    self.parse_extern_decl()
                else:
                    self.error("PEXTERN004", f"unexpected extern item `{self.current().text}`")
                    self.maybe_recover({";", "}"})
                    self.optional(";")
            self.expect("}", "PEXTERN005")
            return
        self.error("PEXTERN999", "expected extern type or extern block")

    def parse_decl(self) -> None:
        prefix_start = self.i
        self.parse_attrs()
        self.parse_visibility_prefix()
        tok = self.current()
        if tok.kind == "eof":
            if self.i > prefix_start:
                self.error("PATTR003", "attribute block must precede a declaration")
            return
        if tok.text not in DECL_KEYWORDS and tok.text not in VISIBILITY | PROC_MODIFIERS:
            self.error("P0001", f"unexpected top-level token `{tok.text}`", tok)
            self.advance()
            return

        self.decl_count += 1

        if tok.text == "space":
            self.advance()
            self.parse_module_path()
            return
        if tok.text == "use":
            self.advance()
            self.parse_module_path()
            if self.at("."):
                self.advance()
                if self.at("*"):
                    self.advance()
                elif self.at("{"):
                    self.skip_balanced("{", "}", "PUSE001")
            if self.at("as"):
                self.advance()
                self.expect_ident("PUSE002")
            return
        if tok.text == "export":
            self.advance()
            if self.at("*"):
                self.advance()
                return
            if self.at("{"):
                self.advance()
                self.parse_ident_list()
                self.expect("}", "PEXPORT001")
                return
            self.parse_ident_list()
            return
        if tok.text in {"const", "static", "global"}:
            self.parse_const_like_decl()
            return
        if tok.text == "type":
            self.parse_type_alias_decl()
            return
        if tok.text == "opaque":
            self.advance()
            self.expect("type", "POPAQUE001")
            self.expect_ident("POPAQUE002")
            self.parse_generic_params()
            if self.at("="):
                self.advance()
                self.parse_type_expr()
            self.optional(";")
            return
        if tok.text == "extern":
            self.parse_extern_decl()
            return
        if tok.text in {"form", "class", "union"}:
            self.advance()
            self.expect_ident("PDECL_FORM001")
            self.parse_generic_params()
            if self.at(":"):
                self.advance()
                self.parse_type_expr()
            self.parse_where_clause()
            self.parse_form_body()
            return
        if tok.text in {"bits", "pick", "flags"}:
            self.advance()
            self.expect_ident("PDECL_ENUM001")
            self.parse_generic_params()
            if self.at(":"):
                self.advance()
                self.parse_type_expr()
            self.parse_where_clause()
            self.expect("{", "PDECL_ENUM002")
            if not self.at("}"):
                while not self.at("}") and not self.at_kind("eof"):
                    if self.at("case"):
                        self.advance()
                    self.expect_ident("PDECL_ENUM003")
                    if self.at("("):
                        self.skip_balanced("(", ")", "PDECL_ENUM004")
                    if self.at("="):
                        self.advance()
                        self.parse_expr()
                    if not self.optional(","):
                        break
            self.expect("}", "PDECL_ENUM005")
            return
        if tok.text == "trait":
            self.advance()
            self.expect_ident("PTRAIT003")
            self.parse_generic_params()
            self.parse_where_clause()
            self.parse_trait_body()
            return
        if tok.text == "impl":
            self.advance()
            self.parse_type_expr()
            if self.at("for"):
                self.advance()
                self.parse_type_expr()
            self.parse_where_clause()
            self.parse_impl_body()
            return
        if tok.text in VISIBILITY | PROC_MODIFIERS or tok.text == "proc":
            self.parse_proc()
            return
        if tok.text in {"intrinsic", "query"}:
            self.advance()
            self.expect_ident("PDECL_QUERY001")
            self.parse_generic_params()
            self.expect("(", "PDECL_QUERY002")
            self.parse_param_list()
            self.expect(")", "PDECL_QUERY003")
            if self.at("->"):
                self.advance()
                self.parse_type_expr()
            self.parse_proc_suffixes()
            if tok.text == "query" and self.at("{"):
                self.parse_block()
            else:
                self.expect(";", "PDECL_QUERY004")
            return
        if tok.text in {"compiler", "pass", "backend", "diagnostic", "macro", "entry"}:
            self.advance()
            self.expect_ident("PDECL_META001")
            if tok.text == "macro":
                self.expect("(", "PMACRO001")
                self.skip_until({")"})
                self.expect(")", "PMACRO002")
            elif tok.text == "entry":
                self.expect("at", "PENTRY001")
                self.parse_module_path()
            self.skip_until({"{", ";"})
            if self.at("{"):
                self.skip_balanced("{", "}", "PMETA001")
            else:
                self.expect(";", "PMETA002")
            return
        if tok.text == "comptime":
            self.advance()
            self.parse_block()
            return
        if tok.text == "static_assert":
            self.advance()
            self.expect("(", "PASSERT001")
            self.parse_expr()
            if self.at(","):
                self.advance()
                self.parse_expr()
            self.expect(")", "PASSERT002")
            self.optional(";")
            return
        if tok.text in {"test", "bench"}:
            self.advance()
            if self.at_kind("string"):
                self.advance()
            self.parse_block()
            return

        self.error("P9999", f"unsupported declaration `{tok.text}`", tok)
        self.advance()

    def parse_program(self) -> dict[str, object]:
        while not self.at_kind("eof"):
            before = self.i
            self.parse_decl()
            if self.i == before:
                self.error("PLOOP", "parser made no progress")
                self.advance()
        return {
            "valid": not self.diagnostics,
            "toplevel_count": self.decl_count,
            "stmt_count": self.stmt_count,
            "expr_count": self.expr_count,
            "type_count": self.type_count,
            "pattern_count": self.pattern_count,
            "diagnostics": [d.as_dict() for d in self.diagnostics],
        }


def lex(text: str) -> list[Token]:
    lx = Lexer(text)
    tokens = []
    while True:
        tok = lx.next_token()
        tokens.append(tok)
        if tok.kind == "eof":
            return tokens


def parse_file(path: Path) -> dict[str, object]:
    text = path.read_text(encoding="utf-8")
    tokens = lex(text)
    parsed = Parser(tokens).parse_program()
    parsed["path"] = str(path)
    parsed["token_count"] = len(tokens)
    return parsed


def collect_files(paths: list[str]) -> list[Path]:
    result: list[Path] = []
    for raw in paths:
        p = Path(raw)
        if p.is_dir():
            result.extend(sorted(p.rglob("*.vit")))
        else:
            result.append(p)
    return result


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("paths", nargs="+")
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args(argv)

    files = collect_files(args.paths)
    results = [parse_file(path) for path in files]
    failed = [item for item in results if not item["valid"]]

    if args.json:
        print(json.dumps(results, indent=2))
    else:
        for item in results:
            status = "PASS" if item["valid"] else "FAIL"
            print(
                f"[{status}] {item['path']} "
                f"toplevel={item['toplevel_count']} "
                f"stmt={item['stmt_count']} "
                f"expr={item['expr_count']} "
                f"type={item['type_count']} "
                f"pattern={item['pattern_count']} "
                f"tokens={item['token_count']}"
            )
            for diag in item["diagnostics"]:
                print(
                    f"  - {diag['code']}:{diag['line']}:{diag['column']} {diag['message']}"
                )
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
