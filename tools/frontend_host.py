from __future__ import annotations

from dataclasses import dataclass
from enum import Enum, auto
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Tuple
import re


# =============================================================================
# Frontend léger (Python) utilisé par le host bootstrap.
# - Lexe un fichier .vitte en tokens rudimentaires,
# - Parse les blocs/expressions principaux,
# - Émet des diagnostics textuels (ligne/colonne),
# - Retourne 0 si tout va bien, 1 sinon.
# =============================================================================


@dataclass
class Span:
    file: Path
    line: int
    column: int


ERROR_CODE_BY_MESSAGE = {
    "',' ou '}' attendu après un champ de pattern struct": "E1208",
    "Utiliser '}' pour fermer un pattern de struct (pas '.end')": "E1206",
    "',' ou ')' attendu après un paramètre de fonction": "E1101",
    "',' ou '}' attendu après un champ de struct": "E2007",
    "Un littéral de struct se ferme avec '}' (et non '.end')": "E2009",
    "Bloc non terminé ('.end' manquant)": "E1300",
    "Utiliser '}' pour fermer ce bloc": "E1302",
}


@dataclass
class Diagnostic:
    message: str
    span: Span
    code: str = "E0000"
    severity: str = "error"


def _read_context(diag: Diagnostic) -> tuple[str, str] | None:
    """
    Retourne (source_line, caret_line) pour afficher le contexte d'une erreur.
    """
    try:
        lines = diag.span.file.read_text(encoding="utf-8").splitlines()
    except OSError:
        return None
    line_index = diag.span.line - 1
    if line_index < 0 or line_index >= len(lines):
        return None
    src = lines[line_index]
    caret = " " * (max(diag.span.column, 1) - 1) + "^"
    return src, caret


def diag_to_json(diag: Diagnostic) -> dict:
    context = _read_context(diag)
    ctx = None
    if context is not None:
        src, caret = context
        ctx = {
            "line": diag.span.line,
            "column": diag.span.column,
            "source": src,
            "marker": caret,
        }
    return {
        "message": diag.message,
        "code": diag.code,
        "severity": diag.severity,
        "span": {
            "file": str(diag.span.file),
            "line": diag.span.line,
            "column": diag.span.column,
        },
        "context": ctx,
    }


def format_diag(diag: Diagnostic, *, with_context: bool = True) -> str:
    head = (
        f"{diag.severity}[{diag.code}] "
        f"{diag.span.file}:{diag.span.line}:{diag.span.column}: {diag.message}"
    )
    if not with_context:
        return head

    context = _read_context(diag)
    if context is None:
        return head

    src, caret = context
    parts = [
        head,
        f"  --> {diag.span.file}:{diag.span.line}:{diag.span.column}",
        "   |",
        f"{diag.span.line:4} | {src}",
        f"   | {caret}",
    ]
    return "\n".join(parts)


class TokenKind(Enum):
    EOF = auto()
    IDENT = auto()
    INT = auto()
    FLOAT = auto()
    CHAR = auto()
    STRING = auto()

    KW_MODULE = auto()
    KW_IMPORT = auto()
    KW_STRUCT = auto()
    KW_FN = auto()
    KW_CONST = auto()
    KW_LET = auto()
    KW_MUT = auto()
    KW_IF = auto()
    KW_ELIF = auto()
    KW_ELSE = auto()
    KW_FOR = auto()
    KW_IN = auto()
    KW_WHILE = auto()
    KW_MATCH = auto()
    KW_EXPORT = auto()
    KW_ENUM = auto()
    KW_UNION = auto()
    KW_TYPE = auto()
    KW_PROGRAM = auto()
    KW_SCENARIO = auto()
    KW_PIPELINE = auto()
    KW_MOD = auto()
    KW_USE = auto()
    KW_SET = auto()
    KW_SAY = auto()
    KW_DO = auto()
    KW_RET = auto()
    KW_WHEN = auto()
    KW_LOOP = auto()
    KW_FROM = auto()
    KW_TO = auto()
    KW_STEP = auto()
    KW_AS = auto()
    KW_AND = auto()
    KW_OR = auto()
    KW_NOT = auto()
    KW_TRUE = auto()
    KW_FALSE = auto()

    NEWLINE = auto()
    DOT_END = auto()

    L_PAREN = auto()
    R_PAREN = auto()
    L_BRACE = auto()
    R_BRACE = auto()
    L_BRACKET = auto()
    R_BRACKET = auto()
    COMMA = auto()
    COLON = auto()
    COLON_COLON = auto()
    DOT = auto()
    EQUAL = auto()
    PLUS_EQUAL = auto()
    MINUS_EQUAL = auto()
    STAR_EQUAL = auto()
    SLASH_EQUAL = auto()
    ARROW = auto()
    FAT_ARROW = auto()
    PLUS = auto()
    MINUS = auto()
    STAR = auto()
    SLASH = auto()
    PERCENT = auto()
    LT = auto()
    LE = auto()
    GT = auto()
    GE = auto()
    EQ_EQ = auto()
    BANG_EQ = auto()
    BANG = auto()
    AMP_AMP = auto()
    PIPE_PIPE = auto()
    UNKNOWN = auto()


KEYWORDS = {
    "module": TokenKind.KW_MODULE,
    "mod": TokenKind.KW_MOD,
    "import": TokenKind.KW_IMPORT,
    "use": TokenKind.KW_USE,
    "struct": TokenKind.KW_STRUCT,
    "enum": TokenKind.KW_ENUM,
    "union": TokenKind.KW_UNION,
    "type": TokenKind.KW_TYPE,
    "fn": TokenKind.KW_FN,
    "program": TokenKind.KW_PROGRAM,
    "scenario": TokenKind.KW_SCENARIO,
    "pipeline": TokenKind.KW_PIPELINE,
    "const": TokenKind.KW_CONST,
    "let": TokenKind.KW_LET,
    "mut": TokenKind.KW_MUT,
    "if": TokenKind.KW_IF,
    "elif": TokenKind.KW_ELIF,
    "else": TokenKind.KW_ELSE,
    "for": TokenKind.KW_FOR,
    "in": TokenKind.KW_IN,
    "while": TokenKind.KW_WHILE,
    "match": TokenKind.KW_MATCH,
    "export": TokenKind.KW_EXPORT,
    "set": TokenKind.KW_SET,
    "say": TokenKind.KW_SAY,
    "do": TokenKind.KW_DO,
    "ret": TokenKind.KW_RET,
    "when": TokenKind.KW_WHEN,
    "loop": TokenKind.KW_LOOP,
    "from": TokenKind.KW_FROM,
    "to": TokenKind.KW_TO,
    "step": TokenKind.KW_STEP,
    "as": TokenKind.KW_AS,
    "and": TokenKind.KW_AND,
    "or": TokenKind.KW_OR,
    "not": TokenKind.KW_NOT,
    "true": TokenKind.KW_TRUE,
    "false": TokenKind.KW_FALSE,
}


@dataclass
class Token:
    kind: TokenKind
    lexeme: str
    span: Span


class Lexer:
    def __init__(self, text: str, file: Path) -> None:
        self.text = text
        self.file = file
        self.index = 0
        self.line = 1
        self.col = 1
        self.tokens: List[Token] = []
        self.diags: List[Diagnostic] = []

    def is_eof(self) -> bool:
        return self.index >= len(self.text)

    def peek(self) -> str:
        if self.is_eof():
            return "\0"
        return self.text[self.index]

    def peek_next(self) -> str:
        nxt = self.index + 1
        if nxt >= len(self.text):
            return "\0"
        return self.text[nxt]

    def advance(self) -> str:
        ch = self.peek()
        if ch == "\0":
            return ch
        self.index += 1
        if ch == "\n":
            self.line += 1
            self.col = 1
        else:
            self.col += 1
        return ch

    def span_here(self) -> Span:
        return Span(self.file, self.line, self.col)

    def add(self, kind: TokenKind, lexeme: str, span: Span) -> None:
        self.tokens.append(Token(kind, lexeme, span))

    def lex(self) -> Tuple[List[Token], List[Diagnostic]]:
        while not self.is_eof():
            self.lex_one()

        self.tokens.append(Token(TokenKind.EOF, "", Span(self.file, self.line, self.col)))
        return self.tokens, self.diags

    def lex_one(self) -> None:
        c = self.peek()
        if c == "\0":
            return
        if c in (" ", "\t", "\r"):
            self.advance()
            return
        if c == "#":
            self.skip_comment()
            return
        if c == "\n":
            span = self.span_here()
            self.advance()
            self.add(TokenKind.NEWLINE, "\n", span)
            return

        if c == "." and self.text[self.index : self.index + 4] == ".end":
            span = self.span_here()
            for _ in ".end":
                self.advance()
            self.add(TokenKind.DOT_END, ".end", span)
            return

        if c == "'" or c == '"':
            self.lex_string_like()
            return

        if c.isdigit():
            self.lex_number()
            return

        if c.isalpha() or c == "_":
            self.lex_ident()
            return

        self.lex_operator_or_punct()

    def skip_comment(self) -> None:
        while not self.is_eof():
            if self.peek() == "\n":
                break
            self.advance()

    def lex_ident(self) -> None:
        start = self.span_here()
        buf = ""
        while True:
            c = self.peek()
            if not (c.isalnum() or c == "_"):
                break
            buf += self.advance()
        kind = KEYWORDS.get(buf, TokenKind.IDENT)
        self.add(kind, buf, start)

    def lex_number(self) -> None:
        start = self.span_here()
        buf = ""
        seen_dot = False
        while True:
            c = self.peek()
            if c.isdigit():
                buf += self.advance()
                continue
            if c == "." and not seen_dot and self.peek_next().isdigit():
                seen_dot = True
                buf += self.advance()
                continue
            break
        kind = TokenKind.FLOAT if seen_dot else TokenKind.INT
        self.add(kind, buf, start)

    def lex_string_like(self) -> None:
        quote = self.advance()
        start = self.span_here()
        buf = ""
        terminated = False
        while not self.is_eof():
            c = self.peek()
            if c == quote:
                self.advance()
                terminated = True
                break
            if c == "\n" or c == "\0":
                break
            buf += self.advance()
        if not terminated:
            self.diags.append(
                Diagnostic(
                    "Littéral non terminé",
                    start,
                )
            )
        kind = TokenKind.CHAR if quote == "'" else TokenKind.STRING
        self.add(kind, buf, start)

    def lex_operator_or_punct(self) -> None:
        start = self.span_here()
        c = self.advance()
        n = self.peek()

        two_char = {
            ("-", ">"): (TokenKind.ARROW, "->"),
            ("=", ">"): (TokenKind.FAT_ARROW, "=>"),
            ("+", "="): (TokenKind.PLUS_EQUAL, "+="),
            ("-", "="): (TokenKind.MINUS_EQUAL, "-="),
            ("*", "="): (TokenKind.STAR_EQUAL, "*="),
            ("/", "="): (TokenKind.SLASH_EQUAL, "/="),
            ("=", "="): (TokenKind.EQ_EQ, "=="),
            ("!", "="): (TokenKind.BANG_EQ, "!="),
            ("<", "="): (TokenKind.LE, "<="),
            (">", "="): (TokenKind.GE, ">="),
            (":", ":"): (TokenKind.COLON_COLON, "::"),
            ("&", "&"): (TokenKind.AMP_AMP, "&&"),
            ("|", "|"): (TokenKind.PIPE_PIPE, "||"),
        }
        if (c, n) in two_char:
            kind, lexeme = two_char[(c, n)]
            self.advance()
            self.add(kind, lexeme, start)
            return

        single = {
            "(": TokenKind.L_PAREN,
            ")": TokenKind.R_PAREN,
            "{": TokenKind.L_BRACE,
            "}": TokenKind.R_BRACE,
            "[": TokenKind.L_BRACKET,
            "]": TokenKind.R_BRACKET,
            ",": TokenKind.COMMA,
            ":": TokenKind.COLON,
            ".": TokenKind.DOT,
            "=": TokenKind.EQUAL,
            "+": TokenKind.PLUS,
            "-": TokenKind.MINUS,
            "*": TokenKind.STAR,
            "/": TokenKind.SLASH,
            "%": TokenKind.PERCENT,
            "<": TokenKind.LT,
            ">": TokenKind.GT,
            "!": TokenKind.BANG,
        }
        kind = single.get(c, TokenKind.UNKNOWN)
        self.add(kind, c, start)


class Parser:
    def __init__(self, tokens: Sequence[Token]) -> None:
        self.tokens = list(tokens)
        self.index = 0
        self.diags: List[Diagnostic] = []

    def current(self) -> Token:
        return self.tokens[self.index]

    def advance(self) -> Token:
        tok = self.current()
        if tok.kind != TokenKind.EOF:
            self.index += 1
        return tok

    def match(self, *kinds: TokenKind) -> Optional[Token]:
        if self.current().kind in kinds:
            return self.advance()
        return None

    def expect(self, kind: TokenKind, message: str) -> Token:
        tok = self.current()
        if tok.kind == kind:
            return self.advance()
        self.diags.append(Diagnostic(message, tok.span))
        return self.advance()

    def parse_file(self) -> List[Diagnostic]:
        while self.current().kind != TokenKind.EOF:
            if self.current().kind == TokenKind.NEWLINE:
                self.advance()
                continue
            if self.current().kind == TokenKind.KW_MODULE:
                self.parse_module_decl()
                continue
            if self.current().kind == TokenKind.KW_MOD:
                self.parse_mod_decl()
                continue
            if self.current().kind == TokenKind.KW_IMPORT:
                self.parse_import_decl()
                continue
            if self.current().kind == TokenKind.KW_USE:
                self.parse_use_decl()
                continue
            if self.current().kind == TokenKind.KW_STRUCT:
                self.parse_struct_decl()
                continue
            if self.current().kind == TokenKind.KW_TYPE:
                self.parse_type_decl()
                continue
            if self.current().kind == TokenKind.KW_FN:
                self.parse_fn_decl()
                continue
            if self.current().kind in (TokenKind.KW_PROGRAM, TokenKind.KW_SCENARIO, TokenKind.KW_PIPELINE):
                self.parse_program_decl()
                continue
            if self.current().kind == TokenKind.KW_EXPORT:
                # export <decl> : ignorer doucement dans le stub
                self.advance()
                self.skip_to_eol()
                continue
            # Inconnu au toplevel : erreur douce + skip ligne
            tok = self.advance()
            self.diags.append(Diagnostic("Déclaration de haut niveau inattendue", tok.span))
            self.skip_to_eol()
        return self.diags

    # ---- Module level ----------------------------------------------------
    def skip_to_eol(self) -> None:
        while self.current().kind not in (TokenKind.EOF, TokenKind.NEWLINE, TokenKind.DOT_END):
            self.advance()
        if self.current().kind == TokenKind.NEWLINE:
            self.advance()

    def parse_module_decl(self) -> None:
        self.advance()
        if self.current().kind != TokenKind.IDENT:
            self.diags.append(Diagnostic("Nom de module attendu après 'module'", self.current().span))
        self.skip_to_eol()

    def parse_mod_decl(self) -> None:
        self.advance()
        # mod a/b/c : accepter chemins simples "a/b" ou "a.b"
        while self.current().kind in (TokenKind.IDENT, TokenKind.DOT, TokenKind.SLASH):
            self.advance()
        self.skip_to_eol()

    def parse_import_decl(self) -> None:
        self.advance()
        if self.current().kind != TokenKind.IDENT:
            self.diags.append(Diagnostic("Nom de module attendu après 'import'", self.current().span))
        self.skip_to_eol()

    def parse_use_decl(self) -> None:
        self.advance()
        while self.current().kind in (TokenKind.IDENT, TokenKind.DOT, TokenKind.SLASH):
            self.advance()
        if self.match(TokenKind.KW_AS):
            if self.current().kind == TokenKind.IDENT:
                self.advance()
            else:
                self.diags.append(Diagnostic("Alias attendu après 'as' dans use", self.current().span))
        self.skip_to_eol()

    def parse_struct_decl(self) -> None:
        self.advance()
        if self.current().kind == TokenKind.IDENT:
            self.advance()
        else:
            self.diags.append(Diagnostic("Nom de struct attendu après 'struct'", self.current().span))
        self.skip_to_eol()
        if self.current().kind == TokenKind.DOT_END:
            self.advance()
            if self.current().kind == TokenKind.NEWLINE:
                self.advance()

    def parse_type_decl(self) -> None:
        # type <Name> ... .end
        self.advance()
        if self.current().kind == TokenKind.IDENT:
            self.advance()
        else:
            self.diags.append(Diagnostic("Nom de type attendu après 'type'", self.current().span))
        # Consommer un bloc éventuel jusqu'à .end
        self.parse_block()

    def parse_program_decl(self) -> None:
        # program/service/pipeline <module> .end
        self.advance()
        if self.current().kind == TokenKind.IDENT:
            while self.current().kind in (TokenKind.IDENT, TokenKind.DOT):
                self.advance()
        else:
            self.diags.append(Diagnostic("Nom de module attendu après l'entrée", self.current().span))
        self.parse_block()

    # ---- Functions -------------------------------------------------------
    def parse_fn_decl(self) -> None:
        self.advance()  # fn
        if self.current().kind == TokenKind.IDENT:
            self.advance()
        else:
            self.diags.append(Diagnostic("Nom de fonction attendu après 'fn'", self.current().span))
        self.parse_param_list()
        if self.match(TokenKind.ARROW):
            self.parse_type_expr()
        self.parse_block()

    def parse_param_list(self) -> None:
        self.expect(TokenKind.L_PAREN, "Parenthèse ouvrante '(' attendue après le nom de la fonction")
        if self.match(TokenKind.R_PAREN):
            return
        while True:
            if self.current().kind != TokenKind.IDENT:
                self.diags.append(Diagnostic("Nom de paramètre attendu", self.current().span))
            else:
                self.advance()
            if self.match(TokenKind.COLON):
                self.parse_type_expr()
            if self.match(TokenKind.COMMA):
                continue
            if self.match(TokenKind.R_PAREN):
                break
            self.diags.append(Diagnostic("',' ou ')' attendu après un paramètre de fonction", self.current().span))
            break

    def parse_type_expr(self) -> None:
        if self.current().kind == TokenKind.IDENT:
            self.advance()
            while self.match(TokenKind.COLON_COLON) or self.match(TokenKind.DOT):
                self.expect(TokenKind.IDENT, "Identifiant attendu après un séparateur de chemin")
            return
        self.diags.append(Diagnostic("Type attendu", self.current().span))
        self.advance()

    # ---- Blocks / statements --------------------------------------------
    def parse_block(self, stop_kinds: Iterable[TokenKind] = ()) -> None:
        stop_set = set(stop_kinds)
        while True:
            tok = self.current()
            if tok.kind in stop_set:
                return
            if tok.kind == TokenKind.EOF:
                self.diags.append(Diagnostic("Bloc non terminé ('.end' manquant)", tok.span))
                return
            if tok.kind == TokenKind.DOT_END:
                self.advance()
                return
            if tok.kind == TokenKind.KW_ELIF and TokenKind.KW_ELIF in stop_set:
                return
            if tok.kind == TokenKind.NEWLINE:
                self.advance()
                continue
            self.parse_stmt()

    def parse_stmt(self) -> None:
        tok = self.current()
        if tok.kind in (TokenKind.KW_LET, TokenKind.KW_CONST):
            self.parse_let_stmt()
            return
        if tok.kind == TokenKind.KW_SET:
            self.parse_set_stmt()
            return
        if tok.kind == TokenKind.KW_WHILE:
            self.advance()
            self.parse_expr(stop_on_lbrace=True)
            brace_tok = self.match(TokenKind.L_BRACE)
            if brace_tok:
                self.parse_block_in_braces(brace_tok)
                return
            self.parse_block()
            return
        if tok.kind == TokenKind.KW_IF:
            self.advance()
            self.parse_expr(stop_on_lbrace=True)
            brace_tok = self.match(TokenKind.L_BRACE)
            if brace_tok:
                self.parse_block_in_braces(brace_tok)
                return
            self.parse_block(stop_kinds=(TokenKind.KW_ELSE, TokenKind.KW_ELIF))
            while self.match(TokenKind.KW_ELIF):
                self.parse_expr(stop_on_lbrace=True)
                brace_tok = self.match(TokenKind.L_BRACE)
                if brace_tok:
                    self.parse_block_in_braces(brace_tok)
                else:
                    self.parse_block(stop_kinds=(TokenKind.KW_ELSE, TokenKind.KW_ELIF))
            if self.match(TokenKind.KW_ELSE):
                self.parse_block()
            return
        if tok.kind == TokenKind.KW_FOR:
            self.advance()
            if self.current().kind == TokenKind.IDENT:
                self.advance()
            if self.match(TokenKind.KW_IN):
                self.parse_expr(stop_on_lbrace=True)
            brace_tok = self.match(TokenKind.L_BRACE)
            if brace_tok:
                self.parse_block_in_braces(brace_tok)
                return
            self.parse_block()
            return
        if tok.kind == TokenKind.KW_MATCH:
            self.parse_match_stmt()
            return
        if tok.kind == TokenKind.KW_WHEN:
            self.parse_when_stmt()
            return
        if tok.kind == TokenKind.KW_LOOP:
            self.parse_loop_stmt()
            return
        if tok.kind == TokenKind.L_BRACE:
            brace_tok = self.advance()
            self.parse_block_in_braces(brace_tok)
            return
        if tok.kind == TokenKind.IDENT and self.current().lexeme == "return":
            self.advance()
            if self.current().kind not in (TokenKind.NEWLINE, TokenKind.DOT_END, TokenKind.EOF):
                self.parse_expr()
            self.skip_to_eol()
            return
        if tok.kind == TokenKind.KW_RET:
            self.advance()
            if self.current().kind not in (TokenKind.NEWLINE, TokenKind.DOT_END, TokenKind.EOF):
                self.parse_expr()
            self.skip_to_eol()
            return
        if tok.kind == TokenKind.KW_SAY:
            self.advance()
            if self.current().kind not in (TokenKind.NEWLINE, TokenKind.DOT_END, TokenKind.EOF):
                self.parse_expr()
            self.skip_to_eol()
            return
        if tok.kind == TokenKind.KW_DO:
            self.advance()
            if self.current().kind == TokenKind.IDENT:
                self.advance()
            while self.current().kind not in (TokenKind.NEWLINE, TokenKind.DOT_END, TokenKind.EOF):
                self.parse_expr()
            self.skip_to_eol()
            return
        # Expression statement
        self.parse_expr()
        self.skip_to_eol()

    def parse_let_stmt(self) -> None:
        self.advance()  # let
        if self.match(TokenKind.KW_MUT):
            pass
        if self.current().kind == TokenKind.IDENT:
            self.advance()
        else:
            self.diags.append(Diagnostic("Identifiant attendu après 'let'", self.current().span))
        if self.match(TokenKind.COLON):
            self.parse_type_expr()
        if self.match(TokenKind.EQUAL):
            self.parse_expr()

    def parse_set_stmt(self) -> None:
        # phrase: set foo.bar = expr
        self.advance()
        self.parse_phrase_assign_target()
        if (
            self.match(TokenKind.EQUAL)
            or self.match(TokenKind.PLUS_EQUAL)
            or self.match(TokenKind.MINUS_EQUAL)
            or self.match(TokenKind.STAR_EQUAL)
            or self.match(TokenKind.SLASH_EQUAL)
        ):
            self.parse_expr()

    def parse_match_stmt(self) -> None:
        kw = self.advance()
        self.parse_expr()
        while True:
            if self.current().kind == TokenKind.DOT_END:
                self.advance()
                return
            if self.current().kind == TokenKind.EOF:
                self.diags.append(Diagnostic("Bloc 'match' non terminé ('.end' manquant)", kw.span))
                return
            if self.current().kind == TokenKind.NEWLINE:
                self.advance()
                continue
            self.parse_match_arm()

    def parse_match_arm(self) -> None:
        self.parse_match_pattern()
        if self.match(TokenKind.KW_IF):
            self.parse_expr()
        if not self.match(TokenKind.FAT_ARROW):
            self.diags.append(Diagnostic("Symbole '=>' attendu dans un bras de match", self.current().span))
        self.parse_match_arm_body()

    def parse_match_arm_body(self) -> None:
        if self.current().kind == TokenKind.NEWLINE:
            self.advance()
            return
        if self.current().kind == TokenKind.DOT_END:
            return
        if self.current().kind == TokenKind.L_BRACE:
            brace_tok = self.advance()
            self.parse_block_in_braces(brace_tok)
            return
        if self.current().kind == TokenKind.IDENT and self.current().lexeme == "return":
            self.advance()
            if self.current().kind not in (TokenKind.NEWLINE, TokenKind.DOT_END, TokenKind.EOF):
                self.parse_expr()
            self.skip_to_eol()
            return
        self.parse_expr()
        self.skip_to_eol()

    def parse_when_stmt(self) -> None:
        # phrase syntax: when <expr> ... elif/else ... .end
        self.advance()
        self.parse_expr(stop_on_lbrace=True)
        self.parse_block(stop_kinds=(TokenKind.KW_ELSE, TokenKind.KW_ELIF))
        while self.match(TokenKind.KW_ELSE):
            if self.match(TokenKind.KW_WHEN) or self.match(TokenKind.KW_IF) or self.match(TokenKind.KW_ELIF):
                self.parse_expr(stop_on_lbrace=True)
                self.parse_block(stop_kinds=(TokenKind.KW_ELSE, TokenKind.KW_ELIF))
            else:
                self.parse_block()

    def parse_loop_stmt(self) -> None:
        # phrase syntax: loop i from a to b [step c] ... .end
        self.advance()
        if self.current().kind == TokenKind.IDENT:
            self.advance()
        if self.match(TokenKind.KW_FROM):
            self.parse_expr(stop_on_lbrace=True)
        if self.match(TokenKind.KW_TO):
            self.parse_expr(stop_on_lbrace=True)
        if self.match(TokenKind.KW_STEP):
            self.parse_expr(stop_on_lbrace=True)
        self.parse_block()

    # ---- Expressions -----------------------------------------------------
    def parse_match_pattern(self) -> None:
        tok = self.current()
        if tok.kind in (TokenKind.INT, TokenKind.FLOAT, TokenKind.CHAR, TokenKind.STRING):
            self.advance()
            return
        if tok.kind == TokenKind.KW_TRUE or tok.kind == TokenKind.KW_FALSE:
            self.advance()
            return
        if tok.kind == TokenKind.L_PAREN:
            self.parse_tuple_pattern()
            return
        if tok.kind == TokenKind.IDENT:
            id_tok = self.advance()
            if self.match(TokenKind.L_BRACE):
                self.parse_struct_pattern_fields(id_tok)
            return
        if tok.lexeme == "_":
            self.advance()
            return
        self.diags.append(Diagnostic("Pattern de match invalide", tok.span))
        self.advance()

    def parse_tuple_pattern(self) -> None:
        self.expect(TokenKind.L_PAREN, "Parenthèse ouvrante '(' attendue dans un pattern de tuple")
        if self.match(TokenKind.R_PAREN):
            return
        saw_comma = False
        while True:
            self.parse_match_pattern()
            if self.match(TokenKind.COMMA):
                saw_comma = True
                continue
            if self.match(TokenKind.R_PAREN):
                break
            self.diags.append(Diagnostic("',' ou ')' attendu dans un pattern de tuple", self.current().span))
            break
        if not saw_comma:
            # pattern parenthésé : rien à faire (structure déjà consommée)
            pass

    def parse_struct_pattern_fields(self, first_ident: Token) -> None:
        while True:
            if self.current().kind == TokenKind.R_BRACE:
                self.advance()
                return
            if self.current().kind == TokenKind.DOT_END:
                self.diags.append(
                    Diagnostic(
                        "Utiliser '}' pour fermer un pattern de struct (pas '.end')",
                        self.current().span,
                    )
                )
                self.advance()
                return
            if self.current().kind == TokenKind.NEWLINE:
                self.advance()
                continue
            if self.current().kind != TokenKind.IDENT:
                self.diags.append(
                    Diagnostic("Nom de champ attendu dans un pattern de struct", self.current().span)
                )
                self.advance()
                continue
            self.advance()
            if self.match(TokenKind.EQUAL):
                self.parse_match_pattern()
            if self.match(TokenKind.COMMA):
                continue
            if self.match(TokenKind.R_BRACE):
                return
            if self.current().kind == TokenKind.DOT_END:
                self.diags.append(
                    Diagnostic(
                        "Utiliser '}' pour fermer un pattern de struct (pas '.end')",
                        self.current().span,
                    )
                )
                self.advance()
                return
            self.diags.append(
                Diagnostic("',' ou '}' attendu après un champ de pattern struct", self.current().span)
            )
            if self.current().kind == TokenKind.R_BRACE:
                self.advance()
                return
            self.advance()

    def parse_phrase_assign_target(self) -> None:
        if self.current().kind != TokenKind.IDENT:
            self.diags.append(Diagnostic("Cible d'affectation attendue après 'set'", self.current().span))
            return
        self.advance()
        while self.match(TokenKind.DOT):
            self.expect(TokenKind.IDENT, "Identifiant attendu après '.'")

    def parse_expr(self, min_prec: int = 0, stop_on_lbrace: bool = False) -> None:
        lhs = self.parse_primary(stop_on_lbrace=stop_on_lbrace)
        while True:
            tok = self.current()
            if stop_on_lbrace and tok.kind == TokenKind.L_BRACE:
                break
            op_prec = self.binary_prec(tok.kind)
            if op_prec < min_prec:
                break
            op = tok.kind
            self.advance()
            is_assignment = op in (
                TokenKind.EQUAL,
                TokenKind.PLUS_EQUAL,
                TokenKind.MINUS_EQUAL,
                TokenKind.STAR_EQUAL,
                TokenKind.SLASH_EQUAL,
            )
            self.parse_expr(op_prec + (0 if is_assignment else 1), stop_on_lbrace=stop_on_lbrace)
            lhs = tok
        return lhs

    def binary_prec(self, kind: TokenKind) -> int:
        if kind in (TokenKind.EQUAL, TokenKind.PLUS_EQUAL, TokenKind.MINUS_EQUAL, TokenKind.STAR_EQUAL, TokenKind.SLASH_EQUAL):
            return 1
        if kind in (TokenKind.PIPE_PIPE, TokenKind.KW_OR):
            return 2
        if kind in (TokenKind.AMP_AMP, TokenKind.KW_AND):
            return 3
        if kind in (TokenKind.STAR, TokenKind.SLASH):
            return 20
        if kind in (TokenKind.PERCENT,):
            return 18
        if kind in (TokenKind.PLUS, TokenKind.MINUS):
            return 10
        if kind in (TokenKind.EQ_EQ, TokenKind.BANG_EQ):
            return 4
        if kind in (TokenKind.LT, TokenKind.LE, TokenKind.GT, TokenKind.GE):
            return 5
        return -1

    def parse_primary(self, stop_on_lbrace: bool = False) -> Token:
        tok = self.current()
        if tok.kind in (TokenKind.MINUS, TokenKind.KW_NOT, TokenKind.BANG):
            self.advance()
            self.parse_primary()
            return tok
        if tok.kind in (
            TokenKind.INT,
            TokenKind.FLOAT,
            TokenKind.CHAR,
            TokenKind.STRING,
            TokenKind.KW_TRUE,
            TokenKind.KW_FALSE,
        ):
            self.advance()
            return tok
        if tok.kind == TokenKind.IDENT:
            ident_tok = self.advance()
            while self.match(TokenKind.DOT) or self.match(TokenKind.COLON_COLON):
                self.expect(TokenKind.IDENT, "Identifiant attendu après un séparateur de chemin")
            if stop_on_lbrace and self.current().kind == TokenKind.L_BRACE:
                return ident_tok
            if self.match(TokenKind.L_BRACE):
                self.parse_struct_literal_fields(ident_tok)
            return ident_tok
        if tok.kind == TokenKind.L_PAREN:
            self.advance()
            self.parse_expr()
            self.expect(TokenKind.R_PAREN, "Parenthèse fermante ')' attendue")
            return tok
        if tok.kind == TokenKind.L_BRACE:
            self.advance()
            self.parse_block_in_braces(tok)
            return tok
        self.diags.append(Diagnostic("Expression attendue", tok.span))
        self.advance()
        return tok

    def parse_struct_literal_fields(self, start_tok: Token) -> None:
        while True:
            if self.current().kind == TokenKind.R_BRACE:
                self.advance()
                return
            if self.current().kind == TokenKind.DOT_END:
                self.diags.append(
                    Diagnostic(
                        "Un littéral de struct se ferme avec '}' (et non '.end')",
                        self.current().span,
                    )
                )
                self.advance()
                return
            if self.current().kind == TokenKind.NEWLINE:
                self.advance()
                continue
            if self.current().kind != TokenKind.IDENT:
                self.diags.append(
                    Diagnostic("Nom de champ attendu dans un littéral de struct", self.current().span)
                )
                self.advance()
                continue
            self.advance()  # field name
            self.expect(TokenKind.EQUAL, "'=' attendu après le nom du champ")
            self.parse_expr()
            if self.current().kind == TokenKind.NEWLINE:
                self.advance()
                continue
            if self.match(TokenKind.COMMA):
                continue
            if self.match(TokenKind.R_BRACE):
                return
            if self.current().kind == TokenKind.DOT_END:
                self.diags.append(
                    Diagnostic(
                        "Un littéral de struct se ferme avec '}' (et non '.end')",
                        self.current().span,
                    )
                )
                self.advance()
                return
            self.diags.append(
                Diagnostic("',' ou '}' attendu après un champ de struct", self.current().span)
            )
            # Récupération : sauter jusqu'à un séparateur de champ raisonnable
            while self.current().kind not in (
                TokenKind.R_BRACE,
                TokenKind.DOT_END,
                TokenKind.COMMA,
                TokenKind.EOF,
                TokenKind.NEWLINE,
            ):
                self.advance()
            if self.match(TokenKind.COMMA):
                continue
            if self.current().kind == TokenKind.R_BRACE:
                self.advance()
                return
            if self.current().kind == TokenKind.DOT_END:
                self.diags.append(
                    Diagnostic(
                        "Un littéral de struct se ferme avec '}' (et non '.end')",
                        self.current().span,
                    )
                )
                self.advance()
                return
            if self.current().kind == TokenKind.NEWLINE:
                self.advance()

    def parse_block_in_braces(self, start_tok: Token) -> None:
        depth = 1
        while depth > 0:
            tok = self.current()
            if tok.kind == TokenKind.DOT_END:
                self.diags.append(Diagnostic("Utiliser '}' pour fermer ce bloc", tok.span))
                self.advance()
                return
            if tok.kind == TokenKind.EOF:
                self.diags.append(Diagnostic("Bloc '{ ... }' non terminé", start_tok.span))
                return
            if tok.kind == TokenKind.L_BRACE:
                depth += 1
            elif tok.kind == TokenKind.R_BRACE:
                depth -= 1
            self.advance()


def run_frontend(path: Path) -> Tuple[int, List[Diagnostic]]:
    text = path.read_text(encoding="utf-8")
    lexer = Lexer(text, path)
    tokens, lex_diags = lexer.lex()
    parser = Parser(tokens)
    parse_diags = parser.parse_file()
    semantic_diags = run_semantic_checks(path, text)
    diags = list(lex_diags) + list(parse_diags) + semantic_diags
    for diag in diags:
        diag.code = ERROR_CODE_BY_MESSAGE.get(diag.message, diag.code)
    return (1 if diags else 0), diags


# -----------------------------------------------------------------------------
# Semantic checks (very lightweight scope/type simulation)
# -----------------------------------------------------------------------------

STRUCT_DECL_RE = re.compile(r"^\s*struct\s+([A-Za-z_][A-Za-z0-9_]*)")
FN_DECL_RE = re.compile(
    r"^\s*fn\s+([A-Za-z_][A-Za-z0-9_]*)\s*\((?P<params>[^)]*)\)\s*(?:->\s*(?P<ret>.+))?"
)


@dataclass
class FunctionInfo:
    name: str
    params: List[Tuple[str, Optional[str]]]
    return_type: Optional[str]
    header_line: int
    body: List[Tuple[int, str]]


def run_semantic_checks(path: Path, text: str) -> List[Diagnostic]:
    lines = text.splitlines()
    structs = collect_struct_names(lines)
    functions = collect_functions(lines)
    diags: List[Diagnostic] = []
    for fn in functions:
        diags.extend(analyze_function(fn, structs, path))
    return diags


def collect_struct_names(lines: List[str]) -> set[str]:
    names: set[str] = set()
    for line in lines:
        match = STRUCT_DECL_RE.match(line)
        if match:
            names.add(match.group(1))
    return names


def collect_functions(lines: List[str]) -> List[FunctionInfo]:
    functions: List[FunctionInfo] = []
    current: Optional[FunctionInfo] = None
    block_depth = 0
    for idx, raw_line in enumerate(lines, start=1):
        line = raw_line.rstrip("\n")
        stripped = line.strip()

        if current is None:
            match = FN_DECL_RE.match(stripped)
            if match:
                params = parse_param_list(match.group("params") or "")
                ret_ty = (match.group("ret") or "").strip() or None
                current = FunctionInfo(
                    name=match.group(1),
                    params=params,
                    return_type=ret_ty,
                    header_line=idx,
                    body=[],
                )
                block_depth = 0
            continue

        if stripped == ".end":
            if block_depth == 0:
                functions.append(current)
                current = None
                continue
            block_depth = max(block_depth - 1, 0)
            current.body.append((idx, line))
            continue

        current.body.append((idx, line))

        if stripped.startswith("while "):
            block_depth += 1
        elif stripped.startswith("if "):
            block_depth += 1
        elif stripped.startswith("match "):
            block_depth += 1
        elif stripped.startswith("elif "):
            block_depth += 1
        elif stripped.startswith("for "):
            block_depth += 1
        elif stripped.startswith("when "):
            block_depth += 1
        elif stripped.startswith("loop "):
            block_depth += 1

    return functions


def parse_param_list(text: str) -> List[Tuple[str, Optional[str]]]:
    params: List[Tuple[str, Optional[str]]] = []
    stripped = text.strip()
    if not stripped:
        return params
    for raw_param in stripped.split(","):
        param = raw_param.strip()
        if not param:
            continue
        if ":" in param:
            name, ty = param.split(":", 1)
            params.append((name.strip(), ty.strip() or None))
        else:
            params.append((param, None))
    return params


def analyze_function(fn: FunctionInfo, structs: set[str], path: Path) -> List[Diagnostic]:
    diags: List[Diagnostic] = []
    locals_types: Dict[str, str] = {}
    for name, ty in fn.params:
        locals_types[name] = ty or "unknown"

    for line_no, raw_line in fn.body:
        line = raw_line.split("#", 1)[0].rstrip()
        stripped = line.strip()
        if not stripped:
            continue
        if stripped.startswith("let "):
            handle_let(stripped, locals_types, structs, path, line_no, diags)
            continue
        if stripped.startswith("return"):
            expr = stripped[len("return") :].strip()
            expr_type, extra = infer_expr_type(expr, locals_types, structs, path, line_no)
            diags.extend(extra)
            expected = fn.return_type
            if expected:
                if not expr and expected not in ("()", "unit"):
                    diags.append(
                        Diagnostic(
                            f"return expects `{expected}`, got `unit`",
                            Span(path, line_no, 1),
                            code="E7101",
                        )
                    )
                elif expr and expr_type != expected:
                    diags.append(
                        Diagnostic(
                            f"return expects `{expected}`, got `{expr_type or 'unknown'}`",
                            Span(path, line_no, 1),
                            code="E7101",
                        )
                    )
            continue

        if stripped.startswith("ret"):
            expr = stripped[len("ret") :].strip()
            expr_type, extra = infer_expr_type(expr, locals_types, structs, path, line_no)
            diags.extend(extra)
            expected = fn.return_type
            if expected and expr_type and expected not in ("()", "unit") and expr_type not in ("", "unknown", "unit"):
                if expr_type != expected:
                    diags.append(
                        Diagnostic(
                            f"return expects `{expected}`, got `{expr_type}`",
                            Span(path, line_no, 1),
                            code="E7101",
                        )
                    )
            continue

        if stripped.startswith("set "):
            assign = stripped[len("set ") :].strip()
            name = assign.split("=", 1)[0].strip().split()[0] if "=" in assign else assign.split()[0]
            if name:
                locals_types.setdefault(name, "unknown")
            continue
        if stripped.startswith("say ") or stripped.startswith("do "):
            # phrase surface statements : ignorés côté check léger
            continue

        has_compound_assign = any(op in stripped for op in ("+=", "-=", "*=", "/="))
        is_simple_assign = "=" in stripped and all(op not in stripped for op in ("==", ">=", "<=", "!=", "=>"))
        if (has_compound_assign or is_simple_assign) and not stripped.startswith("match ") and not stripped.startswith("return "):
            lhs, rhs = stripped.split("=", 1)
            name = lhs.strip().split()[0].rstrip("+-*/")
            if not name:
                continue
            if name not in locals_types:
                diags.append(
                    Diagnostic(
                        f"assignment to unknown variable `{name}`",
                        Span(path, line_no, 1),
                        code="E7103",
                    )
                )
            else:
                _, extra = infer_expr_type(rhs.strip(), locals_types, structs, path, line_no)
                diags.extend(extra)
            continue

    return diags


def handle_let(
    line: str,
    locals_types: Dict[str, str],
    structs: set[str],
    path: Path,
    line_no: int,
    diags: List[Diagnostic],
) -> None:
    remainder = line[4:].strip()
    if remainder.startswith("mut "):
        remainder = remainder[4:].strip()
    eq_index = remainder.find("=")
    colon_index = remainder.find(":")
    name_slice_end = len(remainder)
    if colon_index != -1 and (eq_index == -1 or colon_index < eq_index):
        name_slice_end = colon_index
    elif eq_index != -1:
        name_slice_end = eq_index
    name_token = remainder[:name_slice_end].strip()
    name = name_token.split()[0] if name_token else ""
    annotated_type: Optional[str] = None
    expr: Optional[str] = None

    if colon_index != -1 and (eq_index == -1 or colon_index < eq_index):
        before = remainder[:colon_index]
        after_colon = remainder[colon_index + 1 :]
        if "=" in after_colon:
            annotated_type, expr = after_colon.split("=", 1)
        else:
            annotated_type = after_colon
    elif eq_index != -1:
        expr = remainder.split("=", 1)[1]

    annotated_type = annotated_type.strip() if annotated_type else None
    expr = expr.strip() if expr else None

    locals_types[name] = annotated_type or "unknown"
    if expr:
        expr_type, extra = infer_expr_type(expr, locals_types, structs, path, line_no)
        diags.extend(extra)
        if locals_types[name] == "unknown" and expr_type not in ("", "unknown"):
            locals_types[name] = expr_type


STRUCT_LITERAL_RE = re.compile(r"^([A-Za-z_][A-Za-z0-9_:.]*)\s*\{")
IDENT_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*")


def infer_expr_type(
    expr: str,
    locals_types: Dict[str, str],
    structs: set[str],
    path: Path,
    line_no: int,
) -> Tuple[str, List[Diagnostic]]:
    diags: List[Diagnostic] = []
    stripped = expr.strip()
    if not stripped:
        return ("unit", diags)

    match = STRUCT_LITERAL_RE.match(stripped)
    if match:
        type_path = match.group(1)
        base = type_path.split("::")[-1].split(".")[-1]
        if base not in structs:
            diags.append(
                Diagnostic(
                    "struct literal with unknown type",
                    Span(path, line_no, 1),
                    code="E7100",
                )
            )
            return ("unknown", diags)
        return (base, diags)

    if stripped[0].isdigit():
        if "." in stripped:
            return ("f64", diags)
        return ("i32", diags)

    if stripped in ("true", "false"):
        return ("bool", diags)

    if stripped.startswith('"'):
        return ("string", diags)
    if stripped.startswith("'"):
        return ("char", diags)

    if "(" in stripped and stripped.endswith(")"):
        callee = stripped.split("(", 1)[0].strip()
        if callee in locals_types:
            return ("unknown", diags)
        # Function call / path – non local ⇒ unresolved symbol but we keep unknown type
        return ("unknown", diags)

    name_match = IDENT_RE.match(stripped)
    if name_match:
        ident = name_match.group(0)
        if ident in locals_types:
            return (locals_types[ident], diags)
        diags.append(
            Diagnostic(
                f"unresolved identifier `{ident}`",
                Span(path, line_no, 1),
                code="E7102",
            )
        )
        return ("unknown", diags)

    return ("unknown", diags)
