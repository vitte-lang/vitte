from __future__ import annotations

from dataclasses import dataclass
from enum import Enum, auto
from pathlib import Path
from typing import Iterable, List, Optional, Sequence, Tuple


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


@dataclass
class Diagnostic:
    message: str
    span: Span


def format_diag(diag: Diagnostic) -> str:
    return f"{diag.span.file}:{diag.span.line}:{diag.span.column}: {diag.message}"


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
    KW_LET = auto()
    KW_MUT = auto()
    KW_IF = auto()
    KW_ELSE = auto()
    KW_WHILE = auto()
    KW_MATCH = auto()
    KW_TRUE = auto()
    KW_FALSE = auto()

    NEWLINE = auto()
    DOT_END = auto()

    L_PAREN = auto()
    R_PAREN = auto()
    L_BRACE = auto()
    R_BRACE = auto()
    COMMA = auto()
    COLON = auto()
    COLON_COLON = auto()
    DOT = auto()
    EQUAL = auto()
    ARROW = auto()
    FAT_ARROW = auto()
    PLUS = auto()
    MINUS = auto()
    STAR = auto()
    SLASH = auto()
    LT = auto()
    LE = auto()
    GT = auto()
    GE = auto()
    EQ_EQ = auto()
    BANG_EQ = auto()
    UNKNOWN = auto()


KEYWORDS = {
    "module": TokenKind.KW_MODULE,
    "import": TokenKind.KW_IMPORT,
    "struct": TokenKind.KW_STRUCT,
    "fn": TokenKind.KW_FN,
    "let": TokenKind.KW_LET,
    "mut": TokenKind.KW_MUT,
    "if": TokenKind.KW_IF,
    "else": TokenKind.KW_ELSE,
    "while": TokenKind.KW_WHILE,
    "match": TokenKind.KW_MATCH,
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
            ("=", "="): (TokenKind.EQ_EQ, "=="),
            ("!", "="): (TokenKind.BANG_EQ, "!="),
            ("<", "="): (TokenKind.LE, "<="),
            (">", "="): (TokenKind.GE, ">="),
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
            ",": TokenKind.COMMA,
            ":": TokenKind.COLON,
            ".": TokenKind.DOT,
            "=": TokenKind.EQUAL,
            "+": TokenKind.PLUS,
            "-": TokenKind.MINUS,
            "*": TokenKind.STAR,
            "/": TokenKind.SLASH,
            "<": TokenKind.LT,
            ">": TokenKind.GT,
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
            if self.current().kind == TokenKind.KW_IMPORT:
                self.parse_import_decl()
                continue
            if self.current().kind == TokenKind.KW_STRUCT:
                self.parse_struct_decl()
                continue
            if self.current().kind == TokenKind.KW_FN:
                self.parse_fn_decl()
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

    def parse_import_decl(self) -> None:
        self.advance()
        if self.current().kind != TokenKind.IDENT:
            self.diags.append(Diagnostic("Nom de module attendu après 'import'", self.current().span))
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
            if tok.kind == TokenKind.NEWLINE:
                self.advance()
                continue
            self.parse_stmt()

    def parse_stmt(self) -> None:
        tok = self.current()
        if tok.kind == TokenKind.KW_LET:
            self.parse_let_stmt()
            return
        if tok.kind == TokenKind.KW_WHILE:
            self.advance()
            self.parse_expr()
            self.parse_block()
            return
        if tok.kind == TokenKind.KW_IF:
            self.advance()
            self.parse_expr()
            self.parse_block(stop_kinds=(TokenKind.KW_ELSE,))
            if self.match(TokenKind.KW_ELSE):
                self.parse_block()
            return
        if tok.kind == TokenKind.KW_MATCH:
            self.parse_match_stmt()
            return
        if tok.kind == TokenKind.IDENT and self.current().lexeme == "return":
            self.advance()
            if self.current().kind not in (TokenKind.NEWLINE, TokenKind.DOT_END, TokenKind.EOF):
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
            self.parse_block_in_braces(self.current())
            return
        if self.current().kind == TokenKind.IDENT and self.current().lexeme == "return":
            self.advance()
            if self.current().kind not in (TokenKind.NEWLINE, TokenKind.DOT_END, TokenKind.EOF):
                self.parse_expr()
            self.skip_to_eol()
            return
        self.parse_expr()
        self.skip_to_eol()

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

    def parse_expr(self, min_prec: int = 0) -> None:
        lhs = self.parse_primary()
        while True:
            tok = self.current()
            op_prec = self.binary_prec(tok.kind)
            if op_prec < min_prec:
                break
            op = tok.kind
            self.advance()
            self.parse_expr(op_prec + (0 if op == TokenKind.EQUAL else 1))
            lhs = tok
        return lhs

    def binary_prec(self, kind: TokenKind) -> int:
        if kind == TokenKind.EQUAL:
            return 1
        if kind in (TokenKind.STAR, TokenKind.SLASH):
            return 20
        if kind in (TokenKind.PLUS, TokenKind.MINUS):
            return 10
        if kind in (TokenKind.EQ_EQ, TokenKind.BANG_EQ):
            return 4
        if kind in (TokenKind.LT, TokenKind.LE, TokenKind.GT, TokenKind.GE):
            return 5
        return -1

    def parse_primary(self) -> Token:
        tok = self.current()
        if tok.kind == TokenKind.MINUS:
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
    diags = list(lex_diags) + list(parse_diags)
    return (1 if diags else 0), diags
