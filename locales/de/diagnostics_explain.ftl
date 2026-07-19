# Vitte diagnostic explanations (German)
#
# Schluessel: CODE.summary, CODE.cause, CODE.step1, CODE.fix, CODE.example

E0001.summary = The Parser expected a Name for something (variable, Typ, Modul, etc.).
E0001.cause = The parser or lexer could not form the next valid source construct.
E0001.step1 = Look at the highlighted token and complete or remove the construct around it.
E0001.fix = complete the syntax shape named by the parser label at the highlighted token
E0001.example = proc main() -> i32 { return 0 }

E0002.summary = The Parser expected an Ausdruck at this location.
E0002.cause = The parser or lexer could not form the next valid source construct.
E0002.step1 = Look at the highlighted token and complete or remove the construct around it.
E0002.fix = complete the syntax shape named by the parser label at the highlighted token
E0002.example = proc main() -> i32 { return 0 }

E0003.summary = The Parser expected a Muster.
E0003.cause = The parser or lexer could not form the next valid source construct.
E0003.step1 = Look at the highlighted token and complete or remove the construct around it.
E0003.fix = complete the syntax shape named by the parser label at the highlighted token
E0003.example = when x is Option.Some { return 0 }

E0004.summary = The Parser expected a Typ Name.
E0004.cause = The parser or lexer could not form the next valid source construct.
E0004.step1 = Look at the highlighted token and complete or remove the construct around it.
E0004.fix = complete the syntax shape named by the parser label at the highlighted token
E0004.example = proc id(x: int) -> int { return x }

E0005.summary = A Block was opened but not closed with 'end' or '.end'.
E0005.cause = The parser or lexer could not form the next valid source construct.
E0005.step1 = Look at the highlighted token and complete or remove the construct around it.
E0005.fix = complete the syntax shape named by the parser label at the highlighted token
E0005.example = form Point\n  field x as int\n.end

E0006.summary = An attribute muss followed by a proc Deklaration sein.
E0006.cause = The parser or lexer could not form the next valid source construct.
E0006.step1 = Look at the highlighted token and complete or remove the construct around it.
E0006.fix = complete the syntax shape named by the parser label at the highlighted token
E0006.example = #[inline]\nproc add(a: int, b: int) -> int { return a + b }

E0007.summary = The Parser expected a top-level Deklaration.
E0007.cause = The parser or lexer could not form the next valid source construct.
E0007.step1 = Look at the highlighted token and complete or remove the construct around it.
E0007.fix = complete the syntax shape named by the parser label at the highlighted token
E0007.example = space my/app\nproc main() -> int { return 0 }

E0008.summary = A Muster bound the same Name more than once.
E0008.cause = The parser or lexer could not form the next valid source construct.
E0008.step1 = Look at the highlighted token and complete or remove the construct around it.
E0008.fix = complete the syntax shape named by the parser label at the highlighted token
E0008.example = when Pair(x, y) { return 0 }

E0009.summary = A referenced Typ Name was fehlend.
E0009.cause = The parser or lexer could not form the next valid source construct.
E0009.step1 = Look at the highlighted token and complete or remove the construct around it.
E0009.fix = complete the syntax shape named by the parser label at the highlighted token
E0009.example = use std/core/option.Option\nproc f(x: Option[int]) -> int { return 0 }

E0010.summary = The base Typ of a generisch was fehlend.
E0010.cause = The parser or lexer could not form the next valid source construct.
E0010.step1 = Look at the highlighted token and complete or remove the construct around it.
E0010.fix = complete the syntax shape named by the parser label at the highlighted token
E0010.example = use std/core/option.Option\nlet x: Option[int] = Option.None

E0011.summary = A generisch Typ must include at least one Argument.
E0011.cause = The parser or lexer could not form the next valid source construct.
E0011.step1 = Look at the highlighted token and complete or remove the construct around it.
E0011.fix = complete the syntax shape named by the parser label at the highlighted token
E0011.example = let x: Option[int] = Option.None

E0012.summary = This Typ form is not supported yet.
E0012.cause = The parser or lexer could not form the next valid source construct.
E0012.step1 = Look at the highlighted token and complete or remove the construct around it.
E0012.fix = complete the syntax shape named by the parser label at the highlighted token
E0012.example = let p: *int = &value

E0013.summary = A procedure with an explicit Rueckgabe Typ has a Pfad that reaches the end without returning a Wert.
E0013.cause = The parser or lexer could not form the next valid source construct.
E0013.step1 = Look at the highlighted token and complete or remove the construct around it.
E0013.fix = complete the syntax shape named by the parser label at the highlighted token
E0013.example = proc to_code(ok: bool) -> int {\n  if ok { give 0 }\n  give 1\n}

E0014.summary = An invocation is fehlend its Aufrufziel.
E0014.cause = The parser or lexer could not form the next valid source construct.
E0014.step1 = Look at the highlighted token and complete or remove the construct around it.
E0014.fix = complete the syntax shape named by the parser label at the highlighted token
E0014.example = print(\"hi\")

E0015.summary = This Ausdruck is not supported by the HIR lowering yet.
E0015.cause = The parser or lexer could not form the next valid source construct.
E0015.step1 = Look at the highlighted token and complete or remove the construct around it.
E0015.fix = complete the syntax shape named by the parser label at the highlighted token
E0015.example = let x = value

E0016.summary = This Muster is not supported by the HIR lowering yet.
E0016.cause = The parser or lexer could not form the next valid source construct.
E0016.step1 = Look at the highlighted token and complete or remove the construct around it.
E0016.fix = complete the syntax shape named by the parser label at the highlighted token
E0016.example = when x is Option.Some { return 0 }

E0017.summary = This Anweisung is not supported by the HIR lowering yet.
E0017.cause = The parser or lexer could not form the next valid source construct.
E0017.step1 = Look at the highlighted token and complete or remove the construct around it.
E0017.fix = complete the syntax shape named by the parser label at the highlighted token
E0017.example = return 0

E0018.summary = An extern procedure cannot define a body.
E0018.cause = The parser or lexer could not form the next valid source construct.
E0018.step1 = Look at the highlighted token and complete or remove the construct around it.
E0018.fix = complete the syntax shape named by the parser label at the highlighted token
E0018.example = #[extern]\nproc puts(s: string) -> int

E1001.summary = Muster Bindung doppelt.
E1001.cause = The general phase found code that violates this diagnostic rule.
E1001.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1001.fix = repair the highlighted compiler contract before checking later diagnostics
E1001.example = vitte check path/to/file.vit

E1002.summary = Typ (did you mean a built-in like int/i32/i64/i128/u32/u64/u128/bool/string?) unbekannt.
E1002.cause = The general phase found code that violates this diagnostic rule.
E1002.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1002.fix = repair the highlighted compiler contract before checking later diagnostics
E1002.example = vitte check path/to/file.vit

E1003.summary = generisch base Typ unbekannt.
E1003.cause = The general phase found code that violates this diagnostic rule.
E1003.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1003.fix = repair the highlighted compiler contract before checking later diagnostics
E1003.example = vitte check path/to/file.vit

E1004.summary = generisch Typ erfordert mindestens ein Argument.
E1004.cause = The general phase found code that violates this diagnostic rule.
E1004.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1004.fix = repair the highlighted compiler contract before checking later diagnostics
E1004.example = vitte check path/to/file.vit

E1005.summary = Bezeichner unbekannt.
E1005.cause = The general phase found code that violates this diagnostic rule.
E1005.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1005.fix = repair the highlighted compiler contract before checking later diagnostics
E1005.example = vitte check path/to/file.vit

E1006.summary = generisch Typ erfordert mindestens ein Typ Argument.
E1006.cause = The general phase found code that violates this diagnostic rule.
E1006.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1006.fix = repair the highlighted compiler contract before checking later diagnostics
E1006.example = vitte check path/to/file.vit

E1007.summary = Umwandlung between signed and unsigned values ungueltig.
E1007.cause = The general phase found code that violates this diagnostic rule.
E1007.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1007.fix = repair the highlighted compiler contract before checking later diagnostics
E1007.example = vitte check path/to/file.vit

E1010.summary = stdlib Modul von active stdlib Profil abgelehnt.
E1010.cause = The general phase found code that violates this diagnostic rule.
E1010.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1010.fix = repair the highlighted compiler contract before checking later diagnostics
E1010.example = vitte check path/to/file.vit

E1011.summary = strict-imports requires explicit alias.
E1011.cause = The general phase found code that violates this diagnostic rule.
E1011.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1011.fix = repair the highlighted compiler contract before checking later diagnostics
E1011.example = vitte check path/to/file.vit

E1012.summary = strict-imports verbietet unused Import aliases.
E1012.cause = The general phase found code that violates this diagnostic rule.
E1012.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1012.fix = repair the highlighted compiler contract before checking later diagnostics
E1012.example = vitte check path/to/file.vit

E1013.summary = strict-imports verbietet non-canonical Import paths.
E1013.cause = The general phase found code that violates this diagnostic rule.
E1013.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1013.fix = repair the highlighted compiler contract before checking later diagnostics
E1013.example = vitte check path/to/file.vit

E1014.summary = stdlib Modul fehlend.
E1014.cause = The general phase found code that violates this diagnostic rule.
E1014.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1014.fix = repair the highlighted compiler contract before checking later diagnostics
E1014.example = vitte check path/to/file.vit

E1015.summary = experimental Modul Import denied.
E1015.cause = The general phase found code that violates this diagnostic rule.
E1015.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1015.fix = repair the highlighted compiler contract before checking later diagnostics
E1015.example = vitte check path/to/file.vit

E1016.summary = intern Modul Import denied.
E1016.cause = The general phase found code that violates this diagnostic rule.
E1016.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1016.fix = repair the highlighted compiler contract before checking later diagnostics
E1016.example = vitte check path/to/file.vit

E1017.summary = re-export Symbol Konflikt.
E1017.cause = The general phase found code that violates this diagnostic rule.
E1017.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1017.fix = repair the highlighted compiler contract before checking later diagnostics
E1017.example = vitte check path/to/file.vit

E1018.summary = ambiguous Import Pfad.
E1018.cause = The general phase found code that violates this diagnostic rule.
E1018.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1018.fix = repair the highlighted compiler contract before checking later diagnostics
E1018.example = vitte check path/to/file.vit

E1019.summary = strict-modules verbietet glob imports.
E1019.cause = The general phase found code that violates this diagnostic rule.
E1019.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1019.fix = repair the highlighted compiler contract before checking later diagnostics
E1019.example = vitte check path/to/file.vit

E1020.summary = legacy Import Pfad ist veraltet.
E1020.cause = The general phase found code that violates this diagnostic rule.
E1020.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1020.fix = repair the highlighted compiler contract before checking later diagnostics
E1020.example = vitte check path/to/file.vit

E1021.summary = entry Modul Pfad muss canonical sein.
E1021.cause = The general phase found code that violates this diagnostic rule.
E1021.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1021.fix = repair the highlighted compiler contract before checking later diagnostics
E1021.example = vitte check path/to/file.vit

E1022.summary = entry Name doppelt.
E1022.cause = The general phase found code that violates this diagnostic rule.
E1022.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1022.fix = repair the highlighted compiler contract before checking later diagnostics
E1022.example = vitte check path/to/file.vit

E1023.summary = share referenziert unbekanntes Symbol.
E1023.cause = The general phase found code that violates this diagnostic rule.
E1023.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1023.fix = repair the highlighted compiler contract before checking later diagnostics
E1023.example = vitte check path/to/file.vit

E1024.summary = Symbol in share list doppelt.
E1024.cause = The general phase found code that violates this diagnostic rule.
E1024.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1024.fix = repair the highlighted compiler contract before checking later diagnostics
E1024.example = vitte check path/to/file.vit

E1025.summary = Symbol nicht von Modul exportiert.
E1025.cause = The general phase found code that violates this diagnostic rule.
E1025.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1025.fix = repair the highlighted compiler contract before checking later diagnostics
E1025.example = vitte check path/to/file.vit

E1026.summary = share Deklaration doppelt.
E1026.cause = The general phase found code that violates this diagnostic rule.
E1026.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1026.fix = repair the highlighted compiler contract before checking later diagnostics
E1026.example = vitte check path/to/file.vit

E1027.summary = Import Bindung doppelt.
E1027.cause = The general phase found code that violates this diagnostic rule.
E1027.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1027.fix = repair the highlighted compiler contract before checking later diagnostics
E1027.example = vitte check path/to/file.vit

E1028.summary = Import Bindung kollidiert mit lokal Deklaration.
E1028.cause = The general phase found code that violates this diagnostic rule.
E1028.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1028.fix = repair the highlighted compiler contract before checking later diagnostics
E1028.example = vitte check path/to/file.vit

E1029.summary = lokal Deklaration Name doppelt.
E1029.cause = The general phase found code that violates this diagnostic rule.
E1029.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1029.fix = repair the highlighted compiler contract before checking later diagnostics
E1029.example = vitte check path/to/file.vit

E1030.summary = Modul alias Member not exported.
E1030.cause = The general phase found code that violates this diagnostic rule.
E1030.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1030.fix = repair the highlighted compiler contract before checking later diagnostics
E1030.example = vitte check path/to/file.vit

E1031.summary = qualified Typ Member fehlend.
E1031.cause = The general phase found code that violates this diagnostic rule.
E1031.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1031.fix = repair the highlighted compiler contract before checking later diagnostics
E1031.example = vitte check path/to/file.vit

E1032.summary = Ausdruck ist nicht aufrufbar.
E1032.cause = The general phase found code that violates this diagnostic rule.
E1032.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1032.fix = repair the highlighted compiler contract before checking later diagnostics
E1032.example = vitte check path/to/file.vit

E2001.summary = Typ nicht unterstuetzt.
E2001.cause = The general phase found code that violates this diagnostic rule.
E2001.step1 = Fix the first span reported for this diagnostic, then run the command again.
E2001.fix = repair the highlighted compiler contract before checking later diagnostics
E2001.example = vitte check path/to/file.vit

E2002.summary = invoke has no Aufrufziel.
E2002.cause = The general phase found code that violates this diagnostic rule.
E2002.step1 = Fix the first span reported for this diagnostic, then run the command again.
E2002.fix = repair the highlighted compiler contract before checking later diagnostics
E2002.example = vitte check path/to/file.vit

E2003.summary = Ausdruck in HIR nicht unterstuetzt.
E2003.cause = The general phase found code that violates this diagnostic rule.
E2003.step1 = Fix the first span reported for this diagnostic, then run the command again.
E2003.fix = repair the highlighted compiler contract before checking later diagnostics
E2003.example = vitte check path/to/file.vit

E2004.summary = Muster in HIR nicht unterstuetzt.
E2004.cause = The general phase found code that violates this diagnostic rule.
E2004.step1 = Fix the first span reported for this diagnostic, then run the command again.
E2004.fix = repair the highlighted compiler contract before checking later diagnostics
E2004.example = vitte check path/to/file.vit

E2005.summary = Anweisung in HIR nicht unterstuetzt.
E2005.cause = The general phase found code that violates this diagnostic rule.
E2005.step1 = Fix the first span reported for this diagnostic, then run the command again.
E2005.fix = repair the highlighted compiler contract before checking later diagnostics
E2005.example = vitte check path/to/file.vit

E2006.summary = HIR Typ kind unerwartet.
E2006.cause = The general phase found code that violates this diagnostic rule.
E2006.step1 = Fix the first span reported for this diagnostic, then run the command again.
E2006.fix = repair the highlighted compiler contract before checking later diagnostics
E2006.example = vitte check path/to/file.vit

E2007.summary = HIR expr kind unerwartet.
E2007.cause = The general phase found code that violates this diagnostic rule.
E2007.step1 = Fix the first span reported for this diagnostic, then run the command again.
E2007.fix = repair the highlighted compiler contract before checking later diagnostics
E2007.example = vitte check path/to/file.vit

E2008.summary = HIR stmt kind unerwartet.
E2008.cause = The general phase found code that violates this diagnostic rule.
E2008.step1 = Fix the first span reported for this diagnostic, then run the command again.
E2008.fix = repair the highlighted compiler contract before checking later diagnostics
E2008.example = vitte check path/to/file.vit

E2009.summary = HIR Muster kind unerwartet.
E2009.cause = The general phase found code that violates this diagnostic rule.
E2009.step1 = Fix the first span reported for this diagnostic, then run the command again.
E2009.fix = repair the highlighted compiler contract before checking later diagnostics
E2009.example = vitte check path/to/file.vit

E2010.summary = HIR decl kind unerwartet.
E2010.cause = The general phase found code that violates this diagnostic rule.
E2010.step1 = Fix the first span reported for this diagnostic, then run the command again.
E2010.fix = repair the highlighted compiler contract before checking later diagnostics
E2010.example = vitte check path/to/file.vit

LEX_E_INVALID_CHAR.summary = Zeichen ungueltig.
LEX_E_INVALID_CHAR.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_CHAR.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_CHAR.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_CHAR.example = proc main() -> int { give 0; }

LEX_E_INVALID_NUMBER.summary = numeric Literal ungueltig.
LEX_E_INVALID_NUMBER.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_NUMBER.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_NUMBER.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_NUMBER.example = proc main() -> int { give 0; }

LEX_E_INVALID_ESCAPE.summary = Escape sequence ungueltig.
LEX_E_INVALID_ESCAPE.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_ESCAPE.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_ESCAPE.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_ESCAPE.example = proc main() -> int { give 0; }

LEX_E_INVALID_UNICODE_ESCAPE.summary = unicode Escape sequence ungueltig.
LEX_E_INVALID_UNICODE_ESCAPE.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_UNICODE_ESCAPE.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_UNICODE_ESCAPE.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_UNICODE_ESCAPE.example = proc main() -> int { give 0; }

LEX_E_INVALID_UTF8.summary = UTF-8 byte sequence ungueltig.
LEX_E_INVALID_UTF8.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_UTF8.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_UTF8.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_UTF8.example = proc main() -> int { give 0; }

LEX_E_INVALID_CHAR_LITERAL.summary = Zeichen Literal ungueltig.
LEX_E_INVALID_CHAR_LITERAL.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_CHAR_LITERAL.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_CHAR_LITERAL.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_CHAR_LITERAL.example = proc main() -> int { give 0; }

LEX_E_INVALID_IDENTIFIER.summary = Bezeichner ungueltig.
LEX_E_INVALID_IDENTIFIER.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_IDENTIFIER.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_IDENTIFIER.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_IDENTIFIER.example = proc main() -> int { give 0; }

LEX_E_UNTERMINATED_STRING.summary = string Literal nicht beendet.
LEX_E_UNTERMINATED_STRING.cause = The parser or lexer could not form the next valid source construct.
LEX_E_UNTERMINATED_STRING.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_UNTERMINATED_STRING.fix = add the closing `"` on the same line; do not close a string with a single quote
LEX_E_UNTERMINATED_STRING.example = proc main() -> int { give 0; }

LEX_E_UNTERMINATED_CHAR.summary = Zeichen Literal nicht beendet.
LEX_E_UNTERMINATED_CHAR.cause = The parser or lexer could not form the next valid source construct.
LEX_E_UNTERMINATED_CHAR.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_UNTERMINATED_CHAR.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_UNTERMINATED_CHAR.example = proc main() -> int { give 0; }

LEX_E_UNTERMINATED_BLOCK_COMMENT.summary = Block comment nicht beendet.
LEX_E_UNTERMINATED_BLOCK_COMMENT.cause = The parser or lexer could not form the next valid source construct.
LEX_E_UNTERMINATED_BLOCK_COMMENT.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_UNTERMINATED_BLOCK_COMMENT.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_UNTERMINATED_BLOCK_COMMENT.example = proc main() -> int { give 0; }

LEX_E_UNTERMINATED_REGION_COMMENT.summary = region comment nicht beendet.
LEX_E_UNTERMINATED_REGION_COMMENT.cause = The parser or lexer could not form the next valid source construct.
LEX_E_UNTERMINATED_REGION_COMMENT.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_UNTERMINATED_REGION_COMMENT.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_UNTERMINATED_REGION_COMMENT.example = proc main() -> int { give 0; }

LEX_E_UNEXPECTED_EOF.summary = end of Datei unerwartet.
LEX_E_UNEXPECTED_EOF.cause = The parser or lexer could not form the next valid source construct.
LEX_E_UNEXPECTED_EOF.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_UNEXPECTED_EOF.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_UNEXPECTED_EOF.example = proc main() -> int { give 0; }

LEX_E_TOKEN_TOO_LARGE.summary = Token zu.
LEX_E_TOKEN_TOO_LARGE.cause = The parser or lexer could not form the next valid source construct.
LEX_E_TOKEN_TOO_LARGE.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_TOKEN_TOO_LARGE.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_TOKEN_TOO_LARGE.example = proc main() -> int { give 0; }

PLOOP.summary = Parser hat keinen Fortschritt gemacht.
PLOOP.cause = The parser or lexer could not form the next valid source construct.
PLOOP.step1 = Look at the highlighted token and complete or remove the construct around it.
PLOOP.fix = complete the syntax shape named by the parser label at the highlighted token
PLOOP.example = proc main() -> int { give 0; }

PPRIMARY999.summary = Ausdruck Token unerwartet.
PPRIMARY999.cause = The parser or lexer could not form the next valid source construct.
PPRIMARY999.step1 = Look at the highlighted token and complete or remove the construct around it.
PPRIMARY999.fix = complete the syntax shape named by the parser label at the highlighted token
PPRIMARY999.example = proc main() -> int { give 0; }

PSTMT007.summary = Zuweisung Operator erwartet.
PSTMT007.cause = The parser or lexer could not form the next valid source construct.
PSTMT007.step1 = Look at the highlighted token and complete or remove the construct around it.
PSTMT007.fix = complete the syntax shape named by the parser label at the highlighted token
PSTMT007.example = proc main() -> int { give 0; }

P0001.summary = top-level Token unerwartet.
P0001.cause = The parser or lexer could not form the next valid source construct.
P0001.step1 = Look at the highlighted token and complete or remove the construct around it.
P0001.fix = complete the syntax shape named by the parser label at the highlighted token
P0001.example = proc main() -> int { give 0; }

P000_UNBALANCED.summary = Block nicht geschlossen.
P000_UNBALANCED.cause = The parser or lexer could not form the next valid source construct.
P000_UNBALANCED.step1 = Look at the highlighted token and complete or remove the construct around it.
P000_UNBALANCED.fix = complete the syntax shape named by the parser label at the highlighted token
P000_UNBALANCED.example = proc main() -> int { give 0; }

LEX_E_INVALID_FLOAT.summary = Fliesszahl ungueltig.
LEX_E_INVALID_FLOAT.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_FLOAT.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_FLOAT.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_FLOAT.example = proc main() -> int { give 0; }

LEX_E_INVALID_BINARY.summary = binaer ungueltig.
LEX_E_INVALID_BINARY.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_BINARY.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_BINARY.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_BINARY.example = proc main() -> int { give 0; }

LEX_E_INVALID_OCTAL.summary = octal ungueltig.
LEX_E_INVALID_OCTAL.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_OCTAL.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_OCTAL.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_OCTAL.example = proc main() -> int { give 0; }

LEX_E_INVALID_HEX.summary = hex ungueltig.
LEX_E_INVALID_HEX.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_HEX.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_HEX.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_HEX.example = proc main() -> int { give 0; }

LEX_E_INVALID_INDENTATION.summary = Einrueckung ungueltig.
LEX_E_INVALID_INDENTATION.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_INDENTATION.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_INDENTATION.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_INDENTATION.example = proc main() -> int { give 0; }

LEX_E_INVALID_TOKEN.summary = Token ungueltig.
LEX_E_INVALID_TOKEN.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_TOKEN.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_TOKEN.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_TOKEN.example = proc main() -> int { give 0; }

LEX_E_INVALID_UNICODE.summary = unicode ungueltig.
LEX_E_INVALID_UNICODE.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_UNICODE.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_UNICODE.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_UNICODE.example = proc main() -> int { give 0; }

LEX_E_UNTERMINATED_COMMENT.summary = comment nicht beendet.
LEX_E_UNTERMINATED_COMMENT.cause = The parser or lexer could not form the next valid source construct.
LEX_E_UNTERMINATED_COMMENT.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_UNTERMINATED_COMMENT.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_UNTERMINATED_COMMENT.example = proc main() -> int { give 0; }

PATTR003.summary = pattr003.
PATTR003.cause = The parser or lexer could not form the next valid source construct.
PATTR003.step1 = Look at the highlighted token and complete or remove the construct around it.
PATTR003.fix = complete the syntax shape named by the parser label at the highlighted token
PATTR003.example = proc main() -> int { give 0; }

PARSE_E_TOPLEVEL_DECL_EXPECTED.summary = top-level Deklaration expected.
PARSE_E_TOPLEVEL_DECL_EXPECTED.cause = The parser or lexer could not form the next valid source construct.
PARSE_E_TOPLEVEL_DECL_EXPECTED.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_E_TOPLEVEL_DECL_EXPECTED.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_E_TOPLEVEL_DECL_EXPECTED.example = proc main() -> int { give 0; }

PARSE_E_INCOMPLETE_EXPR.summary = incomplete Ausdruck.
PARSE_E_INCOMPLETE_EXPR.cause = The parser or lexer could not form the next valid source construct.
PARSE_E_INCOMPLETE_EXPR.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_E_INCOMPLETE_EXPR.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_E_INCOMPLETE_EXPR.example = proc main() -> int { give 0; }

PARSE_E_MISSING_RPAREN.summary = closing parenthesis fehlt.
PARSE_E_MISSING_RPAREN.cause = The parser or lexer could not form the next valid source construct.
PARSE_E_MISSING_RPAREN.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_E_MISSING_RPAREN.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_E_MISSING_RPAREN.example = proc main() -> int { give 0; }

PARSE_E_MISSING_COMMA.summary = comma fehlt.
PARSE_E_MISSING_COMMA.cause = The parser or lexer could not form the next valid source construct.
PARSE_E_MISSING_COMMA.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_E_MISSING_COMMA.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_E_MISSING_COMMA.example = proc main() -> int { give 0; }

PARSE_E_TYPE_EXPECTED.summary = Typ expected.
PARSE_E_TYPE_EXPECTED.cause = The parser or lexer could not form the next valid source construct.
PARSE_E_TYPE_EXPECTED.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_E_TYPE_EXPECTED.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_E_TYPE_EXPECTED.example = proc main() -> int { give 0; }

PARSE_E_PATTERN_EXPECTED.summary = Muster expected.
PARSE_E_PATTERN_EXPECTED.cause = The parser or lexer could not form the next valid source construct.
PARSE_E_PATTERN_EXPECTED.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_E_PATTERN_EXPECTED.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_E_PATTERN_EXPECTED.example = proc main() -> int { give 0; }

PARSE_E_BLOCK_EXPECTED.summary = Block expected.
PARSE_E_BLOCK_EXPECTED.cause = The parser or lexer could not form the next valid source construct.
PARSE_E_BLOCK_EXPECTED.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_E_BLOCK_EXPECTED.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_E_BLOCK_EXPECTED.example = proc main() -> int { give 0; }

PARSE_E_UNCLOSED_BLOCK.summary = Block nicht geschlossen.
PARSE_E_UNCLOSED_BLOCK.cause = The parser or lexer could not form the next valid source construct.
PARSE_E_UNCLOSED_BLOCK.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_E_UNCLOSED_BLOCK.fix = add `}` to close the highlighted block, then run the checker again for its parent block
PARSE_E_UNCLOSED_BLOCK.example = proc main() -> int { give 0; }

PARSE_E_EXPECTED_TOKEN.summary = Token erwartet.
PARSE_E_EXPECTED_TOKEN.cause = The parser or lexer could not form the next valid source construct.
PARSE_E_EXPECTED_TOKEN.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_E_EXPECTED_TOKEN.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_E_EXPECTED_TOKEN.example = proc main() -> int { give 0; }

PARSE_E_PARAMETER_COLON_EXPECTED.summary = parameter colon expected.
PARSE_E_PARAMETER_COLON_EXPECTED.cause = The parser or lexer could not form the next valid source construct.
PARSE_E_PARAMETER_COLON_EXPECTED.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_E_PARAMETER_COLON_EXPECTED.fix = insert `:` between the parameter name and its type, for example `right: f64`
PARSE_E_PARAMETER_COLON_EXPECTED.example = proc main() -> int { give 0; }

PARSE_E_UNEXPECTED_TOKEN.summary = Token unerwartet.
PARSE_E_UNEXPECTED_TOKEN.cause = The parser or lexer could not form the next valid source construct.
PARSE_E_UNEXPECTED_TOKEN.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_E_UNEXPECTED_TOKEN.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_E_UNEXPECTED_TOKEN.example = proc main() -> int { give 0; }

PARSE_EXPECTED_EXPR.summary = parse expected expr.
PARSE_EXPECTED_EXPR.cause = The parser or lexer could not form the next valid source construct.
PARSE_EXPECTED_EXPR.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_EXPECTED_EXPR.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_EXPECTED_EXPR.example = proc main() -> int { give 0; }

PARSE_EXPECTED_TYPE.summary = parse expected Typ.
PARSE_EXPECTED_TYPE.cause = The parser or lexer could not form the next valid source construct.
PARSE_EXPECTED_TYPE.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_EXPECTED_TYPE.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_EXPECTED_TYPE.example = proc main() -> int { give 0; }

PARSE_EXPECTED_PATTERN.summary = parse expected Muster.
PARSE_EXPECTED_PATTERN.cause = The parser or lexer could not form the next valid source construct.
PARSE_EXPECTED_PATTERN.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_EXPECTED_PATTERN.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_EXPECTED_PATTERN.example = proc main() -> int { give 0; }

PARSE_EXPECTED_BLOCK.summary = parse expected Block.
PARSE_EXPECTED_BLOCK.cause = The parser or lexer could not form the next valid source construct.
PARSE_EXPECTED_BLOCK.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_EXPECTED_BLOCK.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_EXPECTED_BLOCK.example = proc main() -> int { give 0; }

PARSE_EXPECTED_IDENTIFIER.summary = parse expected Bezeichner.
PARSE_EXPECTED_IDENTIFIER.cause = The parser or lexer could not form the next valid source construct.
PARSE_EXPECTED_IDENTIFIER.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_EXPECTED_IDENTIFIER.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_EXPECTED_IDENTIFIER.example = proc main() -> int { give 0; }

FAST0001.summary = fast0001.
FAST0001.cause = The parser or lexer could not form the next valid source construct.
FAST0001.step1 = Look at the highlighted token and complete or remove the construct around it.
FAST0001.fix = complete the syntax shape named by the parser label at the highlighted token
FAST0001.example = proc main() -> int { give 0; }

FLEX0001.summary = flex0001.
FLEX0001.cause = The parser or lexer could not form the next valid source construct.
FLEX0001.step1 = Look at the highlighted token and complete or remove the construct around it.
FLEX0001.fix = complete the syntax shape named by the parser label at the highlighted token
FLEX0001.example = proc main() -> int { give 0; }

AST_E_INVALID_NODE.summary = Knoten ungueltig.
AST_E_INVALID_NODE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
AST_E_INVALID_NODE.step1 = Fix the first span reported for this diagnostic, then run the command again.
AST_E_INVALID_NODE.fix = repair the highlighted compiler contract before checking later diagnostics
AST_E_INVALID_NODE.example = vitte check path/to/file.vit

AST_E_EMPTY_MODULE.summary = leer Modul.
AST_E_EMPTY_MODULE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
AST_E_EMPTY_MODULE.step1 = Fix the first span reported for this diagnostic, then run the command again.
AST_E_EMPTY_MODULE.fix = repair the highlighted compiler contract before checking later diagnostics
AST_E_EMPTY_MODULE.example = vitte check path/to/file.vit

AST_E_INVALID_DECL.summary = decl ungueltig.
AST_E_INVALID_DECL.cause = The compiler pipeline phase found code that violates this diagnostic rule.
AST_E_INVALID_DECL.step1 = Fix the first span reported for this diagnostic, then run the command again.
AST_E_INVALID_DECL.fix = repair the highlighted compiler contract before checking later diagnostics
AST_E_INVALID_DECL.example = vitte check path/to/file.vit

AST_E_INVALID_EXPR.summary = expr ungueltig.
AST_E_INVALID_EXPR.cause = The compiler pipeline phase found code that violates this diagnostic rule.
AST_E_INVALID_EXPR.step1 = Fix the first span reported for this diagnostic, then run the command again.
AST_E_INVALID_EXPR.fix = repair the highlighted compiler contract before checking later diagnostics
AST_E_INVALID_EXPR.example = vitte check path/to/file.vit

AST_E_INVALID_PATTERN.summary = Muster ungueltig.
AST_E_INVALID_PATTERN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
AST_E_INVALID_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
AST_E_INVALID_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
AST_E_INVALID_PATTERN.example = vitte check path/to/file.vit

AST_E_INVALID_ATTRIBUTE.summary = attribute ungueltig.
AST_E_INVALID_ATTRIBUTE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
AST_E_INVALID_ATTRIBUTE.step1 = Fix the first span reported for this diagnostic, then run the command again.
AST_E_INVALID_ATTRIBUTE.fix = repair the highlighted compiler contract before checking later diagnostics
AST_E_INVALID_ATTRIBUTE.example = vitte check path/to/file.vit

AST_E_DUPLICATE_FIELD.summary = Feld doppelt.
AST_E_DUPLICATE_FIELD.cause = The compiler pipeline phase found code that violates this diagnostic rule.
AST_E_DUPLICATE_FIELD.step1 = Fix the first span reported for this diagnostic, then run the command again.
AST_E_DUPLICATE_FIELD.fix = repair the highlighted compiler contract before checking later diagnostics
AST_E_DUPLICATE_FIELD.example = vitte check path/to/file.vit

AST_E_INVALID_VISIBILITY.summary = Sichtbarkeit ungueltig.
AST_E_INVALID_VISIBILITY.cause = The compiler pipeline phase found code that violates this diagnostic rule.
AST_E_INVALID_VISIBILITY.step1 = Fix the first span reported for this diagnostic, then run the command again.
AST_E_INVALID_VISIBILITY.fix = repair the highlighted compiler contract before checking later diagnostics
AST_E_INVALID_VISIBILITY.example = vitte check path/to/file.vit

AST_E_INVALID_ENTRY.summary = entry ungueltig.
AST_E_INVALID_ENTRY.cause = The compiler pipeline phase found code that violates this diagnostic rule.
AST_E_INVALID_ENTRY.step1 = Fix the first span reported for this diagnostic, then run the command again.
AST_E_INVALID_ENTRY.fix = repair the highlighted compiler contract before checking later diagnostics
AST_E_INVALID_ENTRY.example = vitte check path/to/file.vit

SEMA_E_DUPLICATE_SYMBOL.summary = Symbol doppelt.
SEMA_E_DUPLICATE_SYMBOL.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_DUPLICATE_SYMBOL.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_DUPLICATE_SYMBOL.fix = rename one declaration or remove the duplicate from the same scope
SEMA_E_DUPLICATE_SYMBOL.example = use vitte/core

SEMA_E_UNKNOWN_IDENTIFIER.summary = Bezeichner unbekannt.
SEMA_E_UNKNOWN_IDENTIFIER.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_UNKNOWN_IDENTIFIER.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_UNKNOWN_IDENTIFIER.fix = declare the missing identifier in scope or import the module that exports it
SEMA_E_UNKNOWN_IDENTIFIER.example = use vitte/core

SEMA_E_AMBIGUOUS_SYMBOL.summary = ambiguous Symbol.
SEMA_E_AMBIGUOUS_SYMBOL.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_AMBIGUOUS_SYMBOL.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_AMBIGUOUS_SYMBOL.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_AMBIGUOUS_SYMBOL.example = use vitte/core

SEMA_E_SHADOWING_FORBIDDEN.summary = shadowing is forbidden.
SEMA_E_SHADOWING_FORBIDDEN.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_SHADOWING_FORBIDDEN.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_SHADOWING_FORBIDDEN.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_SHADOWING_FORBIDDEN.example = use vitte/core

SEMA_E_UNKNOWN_FIELD.summary = Feld does not exist.
SEMA_E_UNKNOWN_FIELD.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_UNKNOWN_FIELD.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_UNKNOWN_FIELD.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_UNKNOWN_FIELD.example = use vitte/core

SEMA_E_UNKNOWN_VARIANT.summary = variant does not exist.
SEMA_E_UNKNOWN_VARIANT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_UNKNOWN_VARIANT.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_UNKNOWN_VARIANT.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_UNKNOWN_VARIANT.example = use vitte/core

SEMA_E_UNKNOWN_FUNCTION.summary = function does not exist.
SEMA_E_UNKNOWN_FUNCTION.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_UNKNOWN_FUNCTION.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_UNKNOWN_FUNCTION.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_UNKNOWN_FUNCTION.example = use vitte/core

SEMA_E_UNKNOWN_SYMBOL.summary = Symbol unbekannt.
SEMA_E_UNKNOWN_SYMBOL.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_UNKNOWN_SYMBOL.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_UNKNOWN_SYMBOL.fix = declare the missing identifier in scope or import the module that exports it
SEMA_E_UNKNOWN_SYMBOL.example = use vitte/core

SEMA_E_INVALID_IMPORT.summary = Import ungueltig.
SEMA_E_INVALID_IMPORT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_INVALID_IMPORT.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_INVALID_IMPORT.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_INVALID_IMPORT.example = use vitte/core

SEMA_E_INVALID_EXPORT.summary = export ungueltig.
SEMA_E_INVALID_EXPORT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_INVALID_EXPORT.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_INVALID_EXPORT.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_INVALID_EXPORT.example = use vitte/core

SEMA_E_INVALID_ATTRIBUTE.summary = attribute ungueltig.
SEMA_E_INVALID_ATTRIBUTE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_INVALID_ATTRIBUTE.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_INVALID_ATTRIBUTE.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_INVALID_ATTRIBUTE.example = use vitte/core

SEMA_E_INVALID_VISIBILITY.summary = Sichtbarkeit ungueltig.
SEMA_E_INVALID_VISIBILITY.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_INVALID_VISIBILITY.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_INVALID_VISIBILITY.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_INVALID_VISIBILITY.example = use vitte/core

SEMA_E_INVALID_CONTROL_FLOW.summary = control Kontrollfluss ungueltig.
SEMA_E_INVALID_CONTROL_FLOW.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_INVALID_CONTROL_FLOW.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_INVALID_CONTROL_FLOW.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_INVALID_CONTROL_FLOW.example = use vitte/core

SEMA_E_INVALID_MODULE.summary = Modul ungueltig.
SEMA_E_INVALID_MODULE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_INVALID_MODULE.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_INVALID_MODULE.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_INVALID_MODULE.example = use vitte/core

SEMA_E_INVALID_ASSIGN_TARGET.summary = assign Ziel ungueltig.
SEMA_E_INVALID_ASSIGN_TARGET.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_INVALID_ASSIGN_TARGET.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_INVALID_ASSIGN_TARGET.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_INVALID_ASSIGN_TARGET.example = use vitte/core

SEMA_E_UNDECLARED_TARGET.summary = undeclared Ziel.
SEMA_E_UNDECLARED_TARGET.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_UNDECLARED_TARGET.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_UNDECLARED_TARGET.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_UNDECLARED_TARGET.example = use vitte/core

SEMA_E_MISSING_BINDING.summary = Bindung fehlt.
SEMA_E_MISSING_BINDING.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_MISSING_BINDING.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_MISSING_BINDING.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_MISSING_BINDING.example = use vitte/core

SEMA_E_DUPLICATE_BINDING.summary = Bindung doppelt.
SEMA_E_DUPLICATE_BINDING.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_DUPLICATE_BINDING.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_DUPLICATE_BINDING.fix = rename one declaration or remove the duplicate from the same scope
SEMA_E_DUPLICATE_BINDING.example = use vitte/core

SEMA_E_DUPLICATE_ITEM.summary = Element doppelt.
SEMA_E_DUPLICATE_ITEM.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_DUPLICATE_ITEM.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_DUPLICATE_ITEM.fix = rename one declaration or remove the duplicate from the same scope
SEMA_E_DUPLICATE_ITEM.example = use vitte/core

SEMA_E_INVALID_HIR.summary = hir ungueltig.
SEMA_E_INVALID_HIR.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_INVALID_HIR.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_INVALID_HIR.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_INVALID_HIR.example = use vitte/core

SEMA_E_INTERNAL.summary = intern.
SEMA_E_INTERNAL.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_INTERNAL.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_INTERNAL.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_INTERNAL.example = use vitte/core

TYPECK_E_UNKNOWN_NAME.summary = Name unbekannt.
TYPECK_E_UNKNOWN_NAME.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_UNKNOWN_NAME.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_UNKNOWN_NAME.fix = make the expression type match the type contract named by the type checker
TYPECK_E_UNKNOWN_NAME.example = let count: int = 1

TYPECK_E_UNKNOWN_TYPE.summary = Typ unbekannt.
TYPECK_E_UNKNOWN_TYPE.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_UNKNOWN_TYPE.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_UNKNOWN_TYPE.fix = make the expression type match the type contract named by the type checker
TYPECK_E_UNKNOWN_TYPE.example = let count: int = 1

TYPECK_E_UNKNOWN_ITEM.summary = Element unbekannt.
TYPECK_E_UNKNOWN_ITEM.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_UNKNOWN_ITEM.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_UNKNOWN_ITEM.fix = make the expression type match the type contract named by the type checker
TYPECK_E_UNKNOWN_ITEM.example = let count: int = 1

TYPECK_W_UNRESOLVED_NAME.summary = unresolved Name.
TYPECK_W_UNRESOLVED_NAME.cause = The inferred type does not satisfy the type required at this location.
TYPECK_W_UNRESOLVED_NAME.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_W_UNRESOLVED_NAME.fix = make the expression type match the type contract named by the type checker
TYPECK_W_UNRESOLVED_NAME.example = let count: int = 1

TYPECK_E_INVALID_EXPR.summary = expr ungueltig.
TYPECK_E_INVALID_EXPR.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_INVALID_EXPR.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_INVALID_EXPR.fix = make the expression type match the type contract named by the type checker
TYPECK_E_INVALID_EXPR.example = let count: int = 1

TYPECK_E_INVALID_DEREF.summary = deref ungueltig.
TYPECK_E_INVALID_DEREF.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_INVALID_DEREF.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_INVALID_DEREF.fix = make the expression type match the type contract named by the type checker
TYPECK_E_INVALID_DEREF.example = let count: int = 1

TYPECK_E_BINARY_MISMATCH.summary = binaer Nichtuebereinstimmung.
TYPECK_E_BINARY_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_BINARY_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_BINARY_MISMATCH.fix = make both operands valid for the operator before MIR lowering
TYPECK_E_BINARY_MISMATCH.example = let count: int = 1

TYPECK_E_ASSIGN_MISMATCH.summary = Zuweisung Typ Nichtuebereinstimmung.
TYPECK_E_ASSIGN_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_ASSIGN_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_ASSIGN_MISMATCH.fix = assign a value of the declared binding type, or change the binding annotation at its declaration
TYPECK_E_ASSIGN_MISMATCH.example = let count: int = 1

TYPECK_E_RETURN_MISMATCH.summary = Rueckgabe Typ Nichtuebereinstimmung.
TYPECK_E_RETURN_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_RETURN_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_RETURN_MISMATCH.fix = make the expression type match the type contract named by the type checker
TYPECK_E_RETURN_MISMATCH.example = let count: int = 1

TYPECK_E_CONDITION_TYPE.summary = Bedingung Typ.
TYPECK_E_CONDITION_TYPE.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_CONDITION_TYPE.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_CONDITION_TYPE.fix = make the condition produce bool, for example by adding an explicit comparison
TYPECK_E_CONDITION_TYPE.example = let count: int = 1

TYPECK_E_UNKNOWN_MEMBER.summary = Member unbekannt.
TYPECK_E_UNKNOWN_MEMBER.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_UNKNOWN_MEMBER.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_UNKNOWN_MEMBER.fix = make the expression type match the type contract named by the type checker
TYPECK_E_UNKNOWN_MEMBER.example = let count: int = 1

TYPECK_E_INDEX_TYPE.summary = Index Typ.
TYPECK_E_INDEX_TYPE.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_INDEX_TYPE.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_INDEX_TYPE.fix = make the expression type match the type contract named by the type checker
TYPECK_E_INDEX_TYPE.example = let count: int = 1

TYPECK_E_INVALID_INDEX_TARGET.summary = Index Ziel ungueltig.
TYPECK_E_INVALID_INDEX_TARGET.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_INVALID_INDEX_TARGET.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_INVALID_INDEX_TARGET.fix = make the expression type match the type contract named by the type checker
TYPECK_E_INVALID_INDEX_TARGET.example = let count: int = 1

TYPECK_E_IF_BRANCH_MISMATCH.summary = if branch Nichtuebereinstimmung.
TYPECK_E_IF_BRANCH_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_IF_BRANCH_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_IF_BRANCH_MISMATCH.fix = make the expression type match the type contract named by the type checker
TYPECK_E_IF_BRANCH_MISMATCH.example = let count: int = 1

TYPECK_E_COMPARE_MISMATCH.summary = compare Nichtuebereinstimmung.
TYPECK_E_COMPARE_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_COMPARE_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_COMPARE_MISMATCH.fix = make the expression type match the type contract named by the type checker
TYPECK_E_COMPARE_MISMATCH.example = let count: int = 1

TYPECK_E_INVALID_CAST.summary = Umwandlung ungueltig.
TYPECK_E_INVALID_CAST.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_INVALID_CAST.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_INVALID_CAST.fix = make the expression type match the type contract named by the type checker
TYPECK_E_INVALID_CAST.example = let count: int = 1

TYPECK_E_MATCH_NON_EXHAUSTIVE.summary = match non exhaustive.
TYPECK_E_MATCH_NON_EXHAUSTIVE.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_MATCH_NON_EXHAUSTIVE.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_MATCH_NON_EXHAUSTIVE.fix = make the expression type match the type contract named by the type checker
TYPECK_E_MATCH_NON_EXHAUSTIVE.example = let count: int = 1

TYPECK_E_INVALID_CALL_TARGET.summary = Aufruf Ziel ungueltig.
TYPECK_E_INVALID_CALL_TARGET.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_INVALID_CALL_TARGET.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_INVALID_CALL_TARGET.fix = make the expression type match the type contract named by the type checker
TYPECK_E_INVALID_CALL_TARGET.example = let count: int = 1

TYPECK_E_ARGUMENT_MISMATCH.summary = Aufruf Argument Typ Nichtuebereinstimmung.
TYPECK_E_ARGUMENT_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_ARGUMENT_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_ARGUMENT_MISMATCH.fix = make the expression type match the type contract named by the type checker
TYPECK_E_ARGUMENT_MISMATCH.example = let count: int = 1

TYPECK_E_CALL_ARITY.summary = wrong Zahl of Aufruf Argumente.
TYPECK_E_CALL_ARITY.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_CALL_ARITY.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_CALL_ARITY.fix = make the expression type match the type contract named by the type checker
TYPECK_E_CALL_ARITY.example = let count: int = 1

TYPECK_E_GENERIC_INFERENCE.summary = generisch Typ could not be inferred.
TYPECK_E_GENERIC_INFERENCE.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_GENERIC_INFERENCE.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_GENERIC_INFERENCE.fix = make the expression type match the type contract named by the type checker
TYPECK_E_GENERIC_INFERENCE.example = let count: int = 1

TYPECK_E_TRAIT_BOUND.summary = trait constraint is not satisfied.
TYPECK_E_TRAIT_BOUND.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_TRAIT_BOUND.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_TRAIT_BOUND.fix = make the expression type match the type contract named by the type checker
TYPECK_E_TRAIT_BOUND.example = let count: int = 1

TYPECK_E_CAUSE_CHAIN_MISSING.summary = Typ diagnostic is fehlend a cause chain.
TYPECK_E_CAUSE_CHAIN_MISSING.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_CAUSE_CHAIN_MISSING.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_CAUSE_CHAIN_MISSING.fix = make the expression type match the type contract named by the type checker
TYPECK_E_CAUSE_CHAIN_MISSING.example = let count: int = 1

TYPECK_E_CONFLICTING_IMPL.summary = conflicting trait implementation.
TYPECK_E_CONFLICTING_IMPL.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_CONFLICTING_IMPL.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_CONFLICTING_IMPL.fix = make the expression type match the type contract named by the type checker
TYPECK_E_CONFLICTING_IMPL.example = let count: int = 1

TYPECK_E_CONSTRAINT_CYCLE.summary = cyclic generisch constraint.
TYPECK_E_CONSTRAINT_CYCLE.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_CONSTRAINT_CYCLE.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_CONSTRAINT_CYCLE.fix = make the expression type match the type contract named by the type checker
TYPECK_E_CONSTRAINT_CYCLE.example = let count: int = 1

TYPECK_E_AMBIGUOUS_METHOD.summary = ambiguous method resolution.
TYPECK_E_AMBIGUOUS_METHOD.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_AMBIGUOUS_METHOD.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_AMBIGUOUS_METHOD.fix = make the expression type match the type contract named by the type checker
TYPECK_E_AMBIGUOUS_METHOD.example = let count: int = 1

TYPECK_E_USE_BEFORE_INIT.summary = use before init.
TYPECK_E_USE_BEFORE_INIT.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_USE_BEFORE_INIT.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_USE_BEFORE_INIT.fix = make the expression type match the type contract named by the type checker
TYPECK_E_USE_BEFORE_INIT.example = let count: int = 1

TYPECK_E_USE_AFTER_MOVE.summary = use after Verschiebung.
TYPECK_E_USE_AFTER_MOVE.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_USE_AFTER_MOVE.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_USE_AFTER_MOVE.fix = make the expression type match the type contract named by the type checker
TYPECK_E_USE_AFTER_MOVE.example = let count: int = 1

TYPECK_E_MALFORMED_UNARY.summary = malformed unary.
TYPECK_E_MALFORMED_UNARY.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_MALFORMED_UNARY.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_MALFORMED_UNARY.fix = make the expression type match the type contract named by the type checker
TYPECK_E_MALFORMED_UNARY.example = let count: int = 1

TYPECK_E_UNARY_MISMATCH.summary = unary Nichtuebereinstimmung.
TYPECK_E_UNARY_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_UNARY_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_UNARY_MISMATCH.fix = make the expression type match the type contract named by the type checker
TYPECK_E_UNARY_MISMATCH.example = let count: int = 1

TYPECK_E_MALFORMED_BORROW.summary = malformed Ausleihe.
TYPECK_E_MALFORMED_BORROW.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_MALFORMED_BORROW.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_MALFORMED_BORROW.fix = make the expression type match the type contract named by the type checker
TYPECK_E_MALFORMED_BORROW.example = let count: int = 1

TYPECK_E_MALFORMED_MEMBER.summary = malformed Member.
TYPECK_E_MALFORMED_MEMBER.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_MALFORMED_MEMBER.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_MALFORMED_MEMBER.fix = make the expression type match the type contract named by the type checker
TYPECK_E_MALFORMED_MEMBER.example = let count: int = 1

TYPECK_E_MALFORMED_INDEX.summary = malformed Index.
TYPECK_E_MALFORMED_INDEX.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_MALFORMED_INDEX.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_MALFORMED_INDEX.fix = make the expression type match the type contract named by the type checker
TYPECK_E_MALFORMED_INDEX.example = let count: int = 1

TYPECK_E_MALFORMED_BINARY.summary = malformed binaer.
TYPECK_E_MALFORMED_BINARY.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_MALFORMED_BINARY.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_MALFORMED_BINARY.fix = make the expression type match the type contract named by the type checker
TYPECK_E_MALFORMED_BINARY.example = let count: int = 1

TYPECK_E_MALFORMED_CAST.summary = malformed Umwandlung.
TYPECK_E_MALFORMED_CAST.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_MALFORMED_CAST.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_MALFORMED_CAST.fix = make the expression type match the type contract named by the type checker
TYPECK_E_MALFORMED_CAST.example = let count: int = 1

TYPECK_E_MALFORMED_IF.summary = malformed if.
TYPECK_E_MALFORMED_IF.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_MALFORMED_IF.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_MALFORMED_IF.fix = make the expression type match the type contract named by the type checker
TYPECK_E_MALFORMED_IF.example = let count: int = 1

TYPECK_E_INVALID_HIR.summary = hir ungueltig.
TYPECK_E_INVALID_HIR.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_INVALID_HIR.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_INVALID_HIR.fix = make the expression type match the type contract named by the type checker
TYPECK_E_INVALID_HIR.example = let count: int = 1

TYPECK_E_INTERNAL.summary = intern.
TYPECK_E_INTERNAL.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_INTERNAL.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_INTERNAL.fix = make the expression type match the type contract named by the type checker
TYPECK_E_INTERNAL.example = let count: int = 1

TYPE0001.summary = type0001.
TYPE0001.cause = The inferred type does not satisfy the type required at this location.
TYPE0001.step1 = Compare the expected and found types in the diagnostic labels.
TYPE0001.fix = make the expression type match the type contract named by the type checker
TYPE0001.example = let count: int = 1

TYPE0002.summary = type0002.
TYPE0002.cause = The inferred type does not satisfy the type required at this location.
TYPE0002.step1 = Compare the expected and found types in the diagnostic labels.
TYPE0002.fix = make the expression type match the type contract named by the type checker
TYPE0002.example = let count: int = 1

TYPE0003.summary = type0003.
TYPE0003.cause = The inferred type does not satisfy the type required at this location.
TYPE0003.step1 = Compare the expected and found types in the diagnostic labels.
TYPE0003.fix = make the expression type match the type contract named by the type checker
TYPE0003.example = let count: int = 1

TYPE0004.summary = type0004.
TYPE0004.cause = The inferred type does not satisfy the type required at this location.
TYPE0004.step1 = Compare the expected and found types in the diagnostic labels.
TYPE0004.fix = make the expression type match the type contract named by the type checker
TYPE0004.example = let count: int = 1

TYPE0005.summary = type0005.
TYPE0005.cause = The inferred type does not satisfy the type required at this location.
TYPE0005.step1 = Compare the expected and found types in the diagnostic labels.
TYPE0005.fix = make the expression type match the type contract named by the type checker
TYPE0005.example = let count: int = 1

TYPE0006.summary = type0006.
TYPE0006.cause = The inferred type does not satisfy the type required at this location.
TYPE0006.step1 = Compare the expected and found types in the diagnostic labels.
TYPE0006.fix = make the expression type match the type contract named by the type checker
TYPE0006.example = let count: int = 1

TYPE0007.summary = type0007.
TYPE0007.cause = The inferred type does not satisfy the type required at this location.
TYPE0007.step1 = Compare the expected and found types in the diagnostic labels.
TYPE0007.fix = make the expression type match the type contract named by the type checker
TYPE0007.example = let count: int = 1

TYPE0008.summary = type0008.
TYPE0008.cause = The inferred type does not satisfy the type required at this location.
TYPE0008.step1 = Compare the expected and found types in the diagnostic labels.
TYPE0008.fix = make the expression type match the type contract named by the type checker
TYPE0008.example = let count: int = 1

TYPE0009.summary = type0009.
TYPE0009.cause = The inferred type does not satisfy the type required at this location.
TYPE0009.step1 = Compare the expected and found types in the diagnostic labels.
TYPE0009.fix = make the expression type match the type contract named by the type checker
TYPE0009.example = let count: int = 1

TYPE0010.summary = type0010.
TYPE0010.cause = The inferred type does not satisfy the type required at this location.
TYPE0010.step1 = Compare the expected and found types in the diagnostic labels.
TYPE0010.fix = make the expression type match the type contract named by the type checker
TYPE0010.example = let count: int = 1

TYPE9999.summary = type9999.
TYPE9999.cause = The inferred type does not satisfy the type required at this location.
TYPE9999.step1 = Compare the expected and found types in the diagnostic labels.
TYPE9999.fix = make the expression type match the type contract named by the type checker
TYPE9999.example = let count: int = 1

BORROWCK_E_MOVE_AFTER_MOVE.summary = Verschiebung after Verschiebung.
BORROWCK_E_MOVE_AFTER_MOVE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_MOVE_AFTER_MOVE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_MOVE_AFTER_MOVE.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_MOVE_AFTER_MOVE.example = let view = &value

BORROWCK_E_USE_AFTER_MOVE.summary = Wert verwendet after Verschiebung.
BORROWCK_E_USE_AFTER_MOVE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_USE_AFTER_MOVE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_USE_AFTER_MOVE.fix = borrow before the move, clone explicitly, or move the later use before ownership transfer
BORROWCK_E_USE_AFTER_MOVE.example = let view = &value

BORROWCK_E_BORROW_OF_MOVED_VALUE.summary = Ausleihe of verschoben Wert.
BORROWCK_E_BORROW_OF_MOVED_VALUE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_BORROW_OF_MOVED_VALUE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_BORROW_OF_MOVED_VALUE.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_BORROW_OF_MOVED_VALUE.example = let view = &value

BORROWCK_E_MUTABLE_BORROW_CONFLICT.summary = veraenderbar Ausleihe Konflikt.
BORROWCK_E_MUTABLE_BORROW_CONFLICT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_MUTABLE_BORROW_CONFLICT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_MUTABLE_BORROW_CONFLICT.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_MUTABLE_BORROW_CONFLICT.example = let view = &value

BORROWCK_E_SHARED_BORROW_CONFLICT.summary = geteilt Ausleihe Konflikt.
BORROWCK_E_SHARED_BORROW_CONFLICT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_SHARED_BORROW_CONFLICT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_SHARED_BORROW_CONFLICT.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_SHARED_BORROW_CONFLICT.example = let view = &value

BORROWCK_E_WRITE_WHILE_BORROWED.summary = Schreibzugriff waehrend ausgeliehen.
BORROWCK_E_WRITE_WHILE_BORROWED.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_WRITE_WHILE_BORROWED.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_WRITE_WHILE_BORROWED.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_WRITE_WHILE_BORROWED.example = let view = &value

BORROWCK_E_MOVE_WHILE_BORROWED.summary = Verschiebung waehrend ausgeliehen.
BORROWCK_E_MOVE_WHILE_BORROWED.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_MOVE_WHILE_BORROWED.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_MOVE_WHILE_BORROWED.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_MOVE_WHILE_BORROWED.example = let view = &value

BORROWCK_E_DROP_WHILE_BORROWED.summary = Freigabe waehrend ausgeliehen.
BORROWCK_E_DROP_WHILE_BORROWED.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_DROP_WHILE_BORROWED.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_DROP_WHILE_BORROWED.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_DROP_WHILE_BORROWED.example = let view = &value

BORROWCK_E_ASSIGN_WHILE_BORROWED.summary = assign waehrend ausgeliehen.
BORROWCK_E_ASSIGN_WHILE_BORROWED.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_ASSIGN_WHILE_BORROWED.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_ASSIGN_WHILE_BORROWED.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_ASSIGN_WHILE_BORROWED.example = let view = &value

BORROWCK_E_RETURN_REF_TO_LOCAL.summary = Rueckgabe ref to lokal.
BORROWCK_E_RETURN_REF_TO_LOCAL.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_RETURN_REF_TO_LOCAL.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_RETURN_REF_TO_LOCAL.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_RETURN_REF_TO_LOCAL.example = let view = &value

BORROWCK_E_RETURN_BORROW_OF_LOCAL.summary = Rueckgabe Ausleihe of lokal.
BORROWCK_E_RETURN_BORROW_OF_LOCAL.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_RETURN_BORROW_OF_LOCAL.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_RETURN_BORROW_OF_LOCAL.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_RETURN_BORROW_OF_LOCAL.example = let view = &value

BORROWCK_E_DANGLING_REFERENCE.summary = dangling Referenz.
BORROWCK_E_DANGLING_REFERENCE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_DANGLING_REFERENCE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_DANGLING_REFERENCE.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_DANGLING_REFERENCE.example = let view = &value

BORROWCK_E_LIFETIME_TOO_SHORT.summary = Lebensdauer zu short.
BORROWCK_E_LIFETIME_TOO_SHORT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_LIFETIME_TOO_SHORT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_LIFETIME_TOO_SHORT.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_LIFETIME_TOO_SHORT.example = let view = &value

BORROWCK_E_IMMUTABLE_ASSIGN.summary = immutable assign.
BORROWCK_E_IMMUTABLE_ASSIGN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_IMMUTABLE_ASSIGN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_IMMUTABLE_ASSIGN.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_IMMUTABLE_ASSIGN.example = let view = &value

BORROWCK_E_USE_AFTER_DROP.summary = use after Freigabe.
BORROWCK_E_USE_AFTER_DROP.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_USE_AFTER_DROP.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_USE_AFTER_DROP.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_USE_AFTER_DROP.example = let view = &value

BORROWCK_E_DOUBLE_DROP.summary = double Freigabe.
BORROWCK_E_DOUBLE_DROP.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_DOUBLE_DROP.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_DOUBLE_DROP.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_DOUBLE_DROP.example = let view = &value

BORROWCK_E_UNINITIALIZED_USE.summary = nicht initialisiert use.
BORROWCK_E_UNINITIALIZED_USE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_UNINITIALIZED_USE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_UNINITIALIZED_USE.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_UNINITIALIZED_USE.example = let view = &value

BORROWCK_E_MOVE_AFTER_BORROW.summary = Verschiebung after Ausleihe.
BORROWCK_E_MOVE_AFTER_BORROW.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_MOVE_AFTER_BORROW.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_MOVE_AFTER_BORROW.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_MOVE_AFTER_BORROW.example = let view = &value

BORROWCK_E_MUTABLE_ALIAS.summary = veraenderbar alias.
BORROWCK_E_MUTABLE_ALIAS.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_MUTABLE_ALIAS.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_MUTABLE_ALIAS.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_MUTABLE_ALIAS.example = let view = &value

BORROWCK_E_INTERNAL.summary = intern.
BORROWCK_E_INTERNAL.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_INTERNAL.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_INTERNAL.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_INTERNAL.example = let view = &value

BORROWCK_E_UNKNOWN.summary = unbekannt.
BORROWCK_E_UNKNOWN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_UNKNOWN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_UNKNOWN.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_UNKNOWN.example = let view = &value

CONST_EVAL_E_DIVISION_BY_ZERO.summary = Division by Null in Konstante evaluation.
CONST_EVAL_E_DIVISION_BY_ZERO.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_EVAL_E_DIVISION_BY_ZERO.step1 = Reduce the constant expression at the reported span.
CONST_EVAL_E_DIVISION_BY_ZERO.fix = change the const divisor to a non-zero value or guard the expression before const evaluation
CONST_EVAL_E_DIVISION_BY_ZERO.example = const size: int = 4

CONST_EVAL_E_UNKNOWN_NAME.summary = Name unbekannt.
CONST_EVAL_E_UNKNOWN_NAME.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_EVAL_E_UNKNOWN_NAME.step1 = Reduce the constant expression at the reported span.
CONST_EVAL_E_UNKNOWN_NAME.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_EVAL_E_UNKNOWN_NAME.example = const size: int = 4

CONST_EVAL_E_NON_CONST_CALL.summary = non const Aufruf.
CONST_EVAL_E_NON_CONST_CALL.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_EVAL_E_NON_CONST_CALL.step1 = Reduce the constant expression at the reported span.
CONST_EVAL_E_NON_CONST_CALL.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_EVAL_E_NON_CONST_CALL.example = const size: int = 4

CONST_EVAL_E_MUTATION_IN_CONST.summary = mutation in const.
CONST_EVAL_E_MUTATION_IN_CONST.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_EVAL_E_MUTATION_IN_CONST.step1 = Reduce the constant expression at the reported span.
CONST_EVAL_E_MUTATION_IN_CONST.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_EVAL_E_MUTATION_IN_CONST.example = const size: int = 4

CONST_EVAL_E_UNSUPPORTED_EXPR.summary = expr nicht unterstuetzt.
CONST_EVAL_E_UNSUPPORTED_EXPR.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_EVAL_E_UNSUPPORTED_EXPR.step1 = Reduce the constant expression at the reported span.
CONST_EVAL_E_UNSUPPORTED_EXPR.fix = replace the const expression with literals, supported arithmetic, or another compile-time value
CONST_EVAL_E_UNSUPPORTED_EXPR.example = const size: int = 4

CONST_EVAL_E_OVERFLOW.summary = Ueberlauf.
CONST_EVAL_E_OVERFLOW.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_EVAL_E_OVERFLOW.step1 = Reduce the constant expression at the reported span.
CONST_EVAL_E_OVERFLOW.fix = use a wider const type or reduce the arithmetic result before overflow
CONST_EVAL_E_OVERFLOW.example = const size: int = 4

CONST_EVAL_E_STATIC_ASSERT_FAILED.summary = static Assertion fehlgeschlagen.
CONST_EVAL_E_STATIC_ASSERT_FAILED.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_EVAL_E_STATIC_ASSERT_FAILED.step1 = Reduce the constant expression at the reported span.
CONST_EVAL_E_STATIC_ASSERT_FAILED.fix = make the asserted const condition true or remove the invalid compile-time assumption
CONST_EVAL_E_STATIC_ASSERT_FAILED.example = const size: int = 4

CONST_EVAL_E_CYCLE.summary = Zyklus.
CONST_EVAL_E_CYCLE.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_EVAL_E_CYCLE.step1 = Reduce the constant expression at the reported span.
CONST_EVAL_E_CYCLE.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_EVAL_E_CYCLE.example = const size: int = 4

CONST_EVAL_E_PARSE.summary = parse.
CONST_EVAL_E_PARSE.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_EVAL_E_PARSE.step1 = Reduce the constant expression at the reported span.
CONST_EVAL_E_PARSE.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_EVAL_E_PARSE.example = const size: int = 4

CONST_EVAL_E_UNKNOWN.summary = unbekannt.
CONST_EVAL_E_UNKNOWN.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_EVAL_E_UNKNOWN.step1 = Reduce the constant expression at the reported span.
CONST_EVAL_E_UNKNOWN.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_EVAL_E_UNKNOWN.example = const size: int = 4

MOD_E_MODULE_NOT_FOUND.summary = Modul fehlend.
MOD_E_MODULE_NOT_FOUND.cause = The general phase found code that violates this diagnostic rule.
MOD_E_MODULE_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
MOD_E_MODULE_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
MOD_E_MODULE_NOT_FOUND.example = vitte check path/to/file.vit

MOD_E_IMPORT_CYCLE.summary = Import Zyklus detected.
MOD_E_IMPORT_CYCLE.cause = The general phase found code that violates this diagnostic rule.
MOD_E_IMPORT_CYCLE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MOD_E_IMPORT_CYCLE.fix = repair the highlighted compiler contract before checking later diagnostics
MOD_E_IMPORT_CYCLE.example = vitte check path/to/file.vit

MOD_E_SYMBOL_NOT_EXPORTED.summary = Symbol is nicht von Modul exportiert.
MOD_E_SYMBOL_NOT_EXPORTED.cause = The general phase found code that violates this diagnostic rule.
MOD_E_SYMBOL_NOT_EXPORTED.step1 = Fix the first span reported for this diagnostic, then run the command again.
MOD_E_SYMBOL_NOT_EXPORTED.fix = repair the highlighted compiler contract before checking later diagnostics
MOD_E_SYMBOL_NOT_EXPORTED.example = vitte check path/to/file.vit

MOD_E_IMPORT_NOT_FOUND.summary = Import fehlend.
MOD_E_IMPORT_NOT_FOUND.cause = The general phase found code that violates this diagnostic rule.
MOD_E_IMPORT_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
MOD_E_IMPORT_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
MOD_E_IMPORT_NOT_FOUND.example = vitte check path/to/file.vit

MOD_E_PACKAGE_MISSING.summary = package fehlend.
MOD_E_PACKAGE_MISSING.cause = The general phase found code that violates this diagnostic rule.
MOD_E_PACKAGE_MISSING.step1 = Fix the first span reported for this diagnostic, then run the command again.
MOD_E_PACKAGE_MISSING.fix = repair the highlighted compiler contract before checking later diagnostics
MOD_E_PACKAGE_MISSING.example = vitte check path/to/file.vit

MOD_E_STDLIB_MISSING.summary = stdlib fehlend.
MOD_E_STDLIB_MISSING.cause = The general phase found code that violates this diagnostic rule.
MOD_E_STDLIB_MISSING.step1 = Fix the first span reported for this diagnostic, then run the command again.
MOD_E_STDLIB_MISSING.fix = repair the highlighted compiler contract before checking later diagnostics
MOD_E_STDLIB_MISSING.example = vitte check path/to/file.vit

MOD_E_AMBIGUOUS_MODULE.summary = ambiguous Modul Pfad.
MOD_E_AMBIGUOUS_MODULE.cause = The general phase found code that violates this diagnostic rule.
MOD_E_AMBIGUOUS_MODULE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MOD_E_AMBIGUOUS_MODULE.fix = repair the highlighted compiler contract before checking later diagnostics
MOD_E_AMBIGUOUS_MODULE.example = vitte check path/to/file.vit

HIR_E_INVALID_EXPR.summary = expr ungueltig.
HIR_E_INVALID_EXPR.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_EXPR.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_EXPR.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_EXPR.example = vitte check path/to/file.vit

HIR_E_INVALID_STMT.summary = stmt ungueltig.
HIR_E_INVALID_STMT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_STMT.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_STMT.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_STMT.example = vitte check path/to/file.vit

HIR_E_INVALID_PATTERN.summary = Muster ungueltig.
HIR_E_INVALID_PATTERN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_PATTERN.example = vitte check path/to/file.vit

HIR_E_INVALID_TYPE.summary = Typ ungueltig.
HIR_E_INVALID_TYPE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_TYPE.example = vitte check path/to/file.vit

HIR_E_MISSING_SYMBOL.summary = Symbol fehlt.
HIR_E_MISSING_SYMBOL.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_MISSING_SYMBOL.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_MISSING_SYMBOL.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_MISSING_SYMBOL.example = vitte check path/to/file.vit

HIR_E_INVALID_CONTROL_FLOW.summary = control Kontrollfluss ungueltig.
HIR_E_INVALID_CONTROL_FLOW.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_CONTROL_FLOW.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_CONTROL_FLOW.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_CONTROL_FLOW.example = vitte check path/to/file.vit

HIR_E_LOWERING_FAILED.summary = lowering fehlgeschlagen.
HIR_E_LOWERING_FAILED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_LOWERING_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_LOWERING_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_LOWERING_FAILED.example = vitte check path/to/file.vit

MIR_E_INVALID_BLOCK.summary = Block ungueltig.
MIR_E_INVALID_BLOCK.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_BLOCK.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_BLOCK.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_BLOCK.example = vitte check path/to/file.vit

MIR_E_INVALID_TERMINATOR.summary = Terminator ungueltig.
MIR_E_INVALID_TERMINATOR.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_TERMINATOR.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_TERMINATOR.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_TERMINATOR.example = vitte check path/to/file.vit

MIR_E_UNREACHABLE_BLOCK.summary = unerreichbar Block.
MIR_E_UNREACHABLE_BLOCK.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_UNREACHABLE_BLOCK.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_UNREACHABLE_BLOCK.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_UNREACHABLE_BLOCK.example = vitte check path/to/file.vit

MIR_E_INVALID_OPERAND.summary = Operand ungueltig.
MIR_E_INVALID_OPERAND.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_OPERAND.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_OPERAND.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_OPERAND.example = vitte check path/to/file.vit

MIR_E_INVALID_PLACE.summary = place ungueltig.
MIR_E_INVALID_PLACE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_PLACE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_PLACE.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_PLACE.example = vitte check path/to/file.vit

MIR_E_DATAFLOW_CONFLICT.summary = dataflow Konflikt.
MIR_E_DATAFLOW_CONFLICT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_DATAFLOW_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_DATAFLOW_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_DATAFLOW_CONFLICT.example = vitte check path/to/file.vit

MIR_E_VERIFICATION_FAILED.summary = verification fehlgeschlagen.
MIR_E_VERIFICATION_FAILED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_VERIFICATION_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_VERIFICATION_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_VERIFICATION_FAILED.example = vitte check path/to/file.vit

IR_E_INVALID_MODULE.summary = Modul ungueltig.
IR_E_INVALID_MODULE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_MODULE.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_MODULE.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_MODULE.example = vitte check path/to/file.vit

IR_E_INVALID_FUNCTION.summary = function ungueltig.
IR_E_INVALID_FUNCTION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_FUNCTION.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_FUNCTION.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_FUNCTION.example = vitte check path/to/file.vit

IR_E_INVALID_BLOCK.summary = Block ungueltig.
IR_E_INVALID_BLOCK.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_BLOCK.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_BLOCK.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_BLOCK.example = vitte check path/to/file.vit

IR_E_INVALID_INSTRUCTION.summary = Instruktion ungueltig.
IR_E_INVALID_INSTRUCTION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_INSTRUCTION.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_INSTRUCTION.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_INSTRUCTION.example = vitte check path/to/file.vit

IR_E_TYPE_MISMATCH.summary = Typ Nichtuebereinstimmung.
IR_E_TYPE_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_TYPE_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_TYPE_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_TYPE_MISMATCH.example = vitte check path/to/file.vit

IR_E_VERIFY_FAILED.summary = verify fehlgeschlagen.
IR_E_VERIFY_FAILED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_VERIFY_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_VERIFY_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_VERIFY_FAILED.example = vitte check path/to/file.vit

BACKEND_E_UNSUPPORTED_TARGET.summary = Ziel nicht unterstuetzt.
BACKEND_E_UNSUPPORTED_TARGET.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_UNSUPPORTED_TARGET.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_UNSUPPORTED_TARGET.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_UNSUPPORTED_TARGET.example = vitte build app.vit -o app

BACKEND_E_UNSUPPORTED_FEATURE.summary = Funktion nicht unterstuetzt.
BACKEND_E_UNSUPPORTED_FEATURE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_UNSUPPORTED_FEATURE.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_UNSUPPORTED_FEATURE.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_UNSUPPORTED_FEATURE.example = vitte build app.vit -o app

BACKEND_E_CODEGEN_FAILED.summary = Codeerzeugung fehlgeschlagen.
BACKEND_E_CODEGEN_FAILED.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_CODEGEN_FAILED.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_CODEGEN_FAILED.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_CODEGEN_FAILED.example = vitte build app.vit -o app

BACKEND_E_OBJECT_WRITE_FAILED.summary = Objekt Schreibzugriff fehlgeschlagen.
BACKEND_E_OBJECT_WRITE_FAILED.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_OBJECT_WRITE_FAILED.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_OBJECT_WRITE_FAILED.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_OBJECT_WRITE_FAILED.example = vitte build app.vit -o app

BACKEND_E_ASSEMBLER_FAILED.summary = assembler fehlgeschlagen.
BACKEND_E_ASSEMBLER_FAILED.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_ASSEMBLER_FAILED.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_ASSEMBLER_FAILED.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_ASSEMBLER_FAILED.example = vitte build app.vit -o app

BACKEND_E_ABI_MISMATCH.summary = abi Nichtuebereinstimmung.
BACKEND_E_ABI_MISMATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_ABI_MISMATCH.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_ABI_MISMATCH.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_ABI_MISMATCH.example = vitte build app.vit -o app

BACKEND_E_NATIVE_TOOL_MISSING.summary = native Werkzeug fehlend.
BACKEND_E_NATIVE_TOOL_MISSING.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_NATIVE_TOOL_MISSING.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_NATIVE_TOOL_MISSING.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_NATIVE_TOOL_MISSING.example = vitte build app.vit -o app

LINK_E_UNDEFINED_SYMBOL.summary = undefiniert Symbol.
LINK_E_UNDEFINED_SYMBOL.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_UNDEFINED_SYMBOL.step1 = Check the target triple and the first backend or linker note.
LINK_E_UNDEFINED_SYMBOL.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_UNDEFINED_SYMBOL.example = vitte build app.vit -o app

LINK_E_DUPLICATE_SYMBOL.summary = Symbol doppelt.
LINK_E_DUPLICATE_SYMBOL.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_DUPLICATE_SYMBOL.step1 = Check the target triple and the first backend or linker note.
LINK_E_DUPLICATE_SYMBOL.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_DUPLICATE_SYMBOL.example = vitte build app.vit -o app

LINK_E_LIBRARY_NOT_FOUND.summary = library fehlend.
LINK_E_LIBRARY_NOT_FOUND.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_LIBRARY_NOT_FOUND.step1 = Check the target triple and the first backend or linker note.
LINK_E_LIBRARY_NOT_FOUND.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_LIBRARY_NOT_FOUND.example = vitte build app.vit -o app

LINK_E_OBJECT_NOT_FOUND.summary = Objekt fehlend.
LINK_E_OBJECT_NOT_FOUND.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_OBJECT_NOT_FOUND.step1 = Check the target triple and the first backend or linker note.
LINK_E_OBJECT_NOT_FOUND.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_OBJECT_NOT_FOUND.example = vitte build app.vit -o app

LINK_E_UNSUPPORTED_FORMAT.summary = Format nicht unterstuetzt.
LINK_E_UNSUPPORTED_FORMAT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_UNSUPPORTED_FORMAT.step1 = Check the target triple and the first backend or linker note.
LINK_E_UNSUPPORTED_FORMAT.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_UNSUPPORTED_FORMAT.example = vitte build app.vit -o app

LINK_E_SYSTEM_LINKER_FAILED.summary = system linker fehlgeschlagen.
LINK_E_SYSTEM_LINKER_FAILED.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_SYSTEM_LINKER_FAILED.step1 = Check the target triple and the first backend or linker note.
LINK_E_SYSTEM_LINKER_FAILED.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_SYSTEM_LINKER_FAILED.example = vitte build app.vit -o app

RUNTIME_E_PANIC.summary = Panik.
RUNTIME_E_PANIC.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_PANIC.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_PANIC.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_PANIC.example = vitte check path/to/file.vit

RUNTIME_E_ASSERT_FAILED.summary = Assertion fehlgeschlagen.
RUNTIME_E_ASSERT_FAILED.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_ASSERT_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_ASSERT_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_ASSERT_FAILED.example = vitte check path/to/file.vit

RUNTIME_E_STACK_OVERFLOW.summary = Stack Ueberlauf.
RUNTIME_E_STACK_OVERFLOW.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_STACK_OVERFLOW.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_STACK_OVERFLOW.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_STACK_OVERFLOW.example = vitte check path/to/file.vit

RUNTIME_E_OUT_OF_MEMORY.summary = out of memory.
RUNTIME_E_OUT_OF_MEMORY.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_OUT_OF_MEMORY.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_OUT_OF_MEMORY.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_OUT_OF_MEMORY.example = vitte check path/to/file.vit

RUNTIME_E_DIVISION_BY_ZERO.summary = Division by Null.
RUNTIME_E_DIVISION_BY_ZERO.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_DIVISION_BY_ZERO.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_DIVISION_BY_ZERO.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_DIVISION_BY_ZERO.example = vitte check path/to/file.vit

RUNTIME_E_BOUNDS_CHECK.summary = bounds check.
RUNTIME_E_BOUNDS_CHECK.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_BOUNDS_CHECK.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_BOUNDS_CHECK.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_BOUNDS_CHECK.example = vitte check path/to/file.vit

RUNTIME_E_NULL_DEREF.summary = Null deref.
RUNTIME_E_NULL_DEREF.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_NULL_DEREF.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_NULL_DEREF.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_NULL_DEREF.example = vitte check path/to/file.vit

DRIVER_E_INVALID_ARGUMENT.summary = Argument ungueltig.
DRIVER_E_INVALID_ARGUMENT.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_ARGUMENT.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_ARGUMENT.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_ARGUMENT.example = vitte check src/main.vit --lang en

DRIVER_E_MISSING_INPUT.summary = Eingabe fehlt.
DRIVER_E_MISSING_INPUT.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_MISSING_INPUT.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_MISSING_INPUT.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_MISSING_INPUT.example = vitte check src/main.vit --lang en

DRIVER_E_INPUT_NOT_FOUND.summary = Eingabe fehlend.
DRIVER_E_INPUT_NOT_FOUND.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INPUT_NOT_FOUND.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INPUT_NOT_FOUND.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INPUT_NOT_FOUND.example = vitte check src/main.vit --lang en

DRIVER_E_OUTPUT_WRITE_FAILED.summary = Ausgabe Schreibzugriff fehlgeschlagen.
DRIVER_E_OUTPUT_WRITE_FAILED.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_OUTPUT_WRITE_FAILED.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_OUTPUT_WRITE_FAILED.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_OUTPUT_WRITE_FAILED.example = vitte check src/main.vit --lang en

DRIVER_E_CACHE_READ_FAILED.summary = cache read fehlgeschlagen.
DRIVER_E_CACHE_READ_FAILED.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_CACHE_READ_FAILED.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_CACHE_READ_FAILED.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_CACHE_READ_FAILED.example = vitte check src/main.vit --lang en

DRIVER_E_CACHE_WRITE_FAILED.summary = cache Schreibzugriff fehlgeschlagen.
DRIVER_E_CACHE_WRITE_FAILED.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_CACHE_WRITE_FAILED.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_CACHE_WRITE_FAILED.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_CACHE_WRITE_FAILED.example = vitte check src/main.vit --lang en

DRIVER_E_PROFILE_NOT_FOUND.summary = Profil fehlend.
DRIVER_E_PROFILE_NOT_FOUND.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_PROFILE_NOT_FOUND.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_PROFILE_NOT_FOUND.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_PROFILE_NOT_FOUND.example = vitte check src/main.vit --lang en

DRIVER_E_TARGET_NOT_FOUND.summary = Ziel fehlend.
DRIVER_E_TARGET_NOT_FOUND.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_TARGET_NOT_FOUND.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_TARGET_NOT_FOUND.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_TARGET_NOT_FOUND.example = vitte check src/main.vit --lang en

BOOTSTRAP_E_STAGE_FAILURE.summary = stage failure.
BOOTSTRAP_E_STAGE_FAILURE.cause = The bootstrap phase found code that violates this diagnostic rule.
BOOTSTRAP_E_STAGE_FAILURE.step1 = Fix the first span reported for this diagnostic, then run the command again.
BOOTSTRAP_E_STAGE_FAILURE.fix = repair the highlighted compiler contract before checking later diagnostics
BOOTSTRAP_E_STAGE_FAILURE.example = vitte check path/to/file.vit

BOOTSTRAP_E_SEED_MISSING.summary = seed fehlend.
BOOTSTRAP_E_SEED_MISSING.cause = The bootstrap phase found code that violates this diagnostic rule.
BOOTSTRAP_E_SEED_MISSING.step1 = Fix the first span reported for this diagnostic, then run the command again.
BOOTSTRAP_E_SEED_MISSING.fix = repair the highlighted compiler contract before checking later diagnostics
BOOTSTRAP_E_SEED_MISSING.example = vitte check path/to/file.vit

BOOTSTRAP_E_COMPILER_MISSING.summary = compiler fehlend.
BOOTSTRAP_E_COMPILER_MISSING.cause = The bootstrap phase found code that violates this diagnostic rule.
BOOTSTRAP_E_COMPILER_MISSING.step1 = Fix the first span reported for this diagnostic, then run the command again.
BOOTSTRAP_E_COMPILER_MISSING.fix = repair the highlighted compiler contract before checking later diagnostics
BOOTSTRAP_E_COMPILER_MISSING.example = vitte check path/to/file.vit

BOOTSTRAP_E_SELF_CHECK_FAILED.summary = self check fehlgeschlagen.
BOOTSTRAP_E_SELF_CHECK_FAILED.cause = The bootstrap phase found code that violates this diagnostic rule.
BOOTSTRAP_E_SELF_CHECK_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
BOOTSTRAP_E_SELF_CHECK_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
BOOTSTRAP_E_SELF_CHECK_FAILED.example = vitte check path/to/file.vit

BOOTSTRAP_E_ARTIFACT_INVALID.summary = artifact ungueltig.
BOOTSTRAP_E_ARTIFACT_INVALID.cause = The bootstrap phase found code that violates this diagnostic rule.
BOOTSTRAP_E_ARTIFACT_INVALID.step1 = Fix the first span reported for this diagnostic, then run the command again.
BOOTSTRAP_E_ARTIFACT_INVALID.fix = repair the highlighted compiler contract before checking later diagnostics
BOOTSTRAP_E_ARTIFACT_INVALID.example = vitte check path/to/file.vit

E_BOOTSTRAP_CONST_TYPE.summary = bootstrap Konstante has wrong Typ.
E_BOOTSTRAP_CONST_TYPE.cause = The general phase found code that violates this diagnostic rule.
E_BOOTSTRAP_CONST_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
E_BOOTSTRAP_CONST_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
E_BOOTSTRAP_CONST_TYPE.example = vitte check path/to/file.vit

E_BOOTSTRAP_DUP_PROC.summary = bootstrap procedure doppelt.
E_BOOTSTRAP_DUP_PROC.cause = The general phase found code that violates this diagnostic rule.
E_BOOTSTRAP_DUP_PROC.step1 = Fix the first span reported for this diagnostic, then run the command again.
E_BOOTSTRAP_DUP_PROC.fix = repair the highlighted compiler contract before checking later diagnostics
E_BOOTSTRAP_DUP_PROC.example = vitte check path/to/file.vit

E_BOOTSTRAP_UNKNOWN_CONST.summary = bootstrap Konstante unbekannt.
E_BOOTSTRAP_UNKNOWN_CONST.cause = The general phase found code that violates this diagnostic rule.
E_BOOTSTRAP_UNKNOWN_CONST.step1 = Fix the first span reported for this diagnostic, then run the command again.
E_BOOTSTRAP_UNKNOWN_CONST.fix = repair the highlighted compiler contract before checking later diagnostics
E_BOOTSTRAP_UNKNOWN_CONST.example = vitte check path/to/file.vit

E_BOOTSTRAP_UNKNOWN_PROC.summary = bootstrap procedure nicht unterstuetzt.
E_BOOTSTRAP_UNKNOWN_PROC.cause = The general phase found code that violates this diagnostic rule.
E_BOOTSTRAP_UNKNOWN_PROC.step1 = Fix the first span reported for this diagnostic, then run the command again.
E_BOOTSTRAP_UNKNOWN_PROC.fix = repair the highlighted compiler contract before checking later diagnostics
E_BOOTSTRAP_UNKNOWN_PROC.example = vitte check path/to/file.vit

LIMIT_FILE_SIZE_MAX.summary = Datei size max.
LIMIT_FILE_SIZE_MAX.cause = The input exceeded a configured compiler safety limit.
LIMIT_FILE_SIZE_MAX.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_FILE_SIZE_MAX.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_FILE_SIZE_MAX.example = vitte check src/main.vit

LIMIT_TOKEN_SIZE_MAX.summary = Token size max.
LIMIT_TOKEN_SIZE_MAX.cause = The input exceeded a configured compiler safety limit.
LIMIT_TOKEN_SIZE_MAX.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_TOKEN_SIZE_MAX.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_TOKEN_SIZE_MAX.example = vitte check src/main.vit

LIMIT_AST_DEPTH_MAX.summary = ast depth max.
LIMIT_AST_DEPTH_MAX.cause = The input exceeded a configured compiler safety limit.
LIMIT_AST_DEPTH_MAX.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_AST_DEPTH_MAX.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_AST_DEPTH_MAX.example = vitte check src/main.vit

LIMIT_EXPR_DEPTH_MAX.summary = expr depth max.
LIMIT_EXPR_DEPTH_MAX.cause = The input exceeded a configured compiler safety limit.
LIMIT_EXPR_DEPTH_MAX.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_EXPR_DEPTH_MAX.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_EXPR_DEPTH_MAX.example = vitte check src/main.vit

LIMIT_IMPORT_DEPTH_MAX.summary = Import depth max.
LIMIT_IMPORT_DEPTH_MAX.cause = The input exceeded a configured compiler safety limit.
LIMIT_IMPORT_DEPTH_MAX.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_IMPORT_DEPTH_MAX.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_IMPORT_DEPTH_MAX.example = vitte check src/main.vit

LIMIT_MODULE_COUNT_MAX.summary = Modul count max.
LIMIT_MODULE_COUNT_MAX.cause = The input exceeded a configured compiler safety limit.
LIMIT_MODULE_COUNT_MAX.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_MODULE_COUNT_MAX.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_MODULE_COUNT_MAX.example = vitte check src/main.vit

LIMIT_DIAGNOSTICS_MAX.summary = zu many Diagnosen emitted.
LIMIT_DIAGNOSTICS_MAX.cause = The input exceeded a configured compiler safety limit.
LIMIT_DIAGNOSTICS_MAX.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_DIAGNOSTICS_MAX.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_DIAGNOSTICS_MAX.example = vitte check src/main.vit

LIMIT_SYMBOL_COUNT_MAX.summary = Symbol count max.
LIMIT_SYMBOL_COUNT_MAX.cause = The input exceeded a configured compiler safety limit.
LIMIT_SYMBOL_COUNT_MAX.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_SYMBOL_COUNT_MAX.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_SYMBOL_COUNT_MAX.example = vitte check src/main.vit

LIMIT_PARSER_RECURSION_MAX.summary = Parser recursion max.
LIMIT_PARSER_RECURSION_MAX.cause = The input exceeded a configured compiler safety limit.
LIMIT_PARSER_RECURSION_MAX.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_PARSER_RECURSION_MAX.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_PARSER_RECURSION_MAX.example = vitte check src/main.vit

LIMIT_MACRO_EXPANSION_MAX.summary = macro expansion max.
LIMIT_MACRO_EXPANSION_MAX.cause = The input exceeded a configured compiler safety limit.
LIMIT_MACRO_EXPANSION_MAX.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_MACRO_EXPANSION_MAX.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_MACRO_EXPANSION_MAX.example = vitte check src/main.vit

MACRO_E_EXPANSION_FAILED.summary = expansion fehlgeschlagen.
MACRO_E_EXPANSION_FAILED.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_EXPANSION_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_EXPANSION_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_EXPANSION_FAILED.example = vitte check path/to/file.vit

MACRO_E_RECURSION_LIMIT.summary = recursion limit.
MACRO_E_RECURSION_LIMIT.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_RECURSION_LIMIT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_RECURSION_LIMIT.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_RECURSION_LIMIT.example = vitte check path/to/file.vit

MACRO_E_INVALID_ARGUMENT.summary = Argument ungueltig.
MACRO_E_INVALID_ARGUMENT.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_ARGUMENT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_ARGUMENT.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_ARGUMENT.example = vitte check path/to/file.vit

MACRO_E_UNKNOWN_MACRO.summary = macro unbekannt.
MACRO_E_UNKNOWN_MACRO.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_UNKNOWN_MACRO.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_UNKNOWN_MACRO.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_UNKNOWN_MACRO.example = vitte check path/to/file.vit

MACRO_E_UNSTABLE_FEATURE.summary = unstable Funktion.
MACRO_E_UNSTABLE_FEATURE.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_UNSTABLE_FEATURE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_UNSTABLE_FEATURE.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_UNSTABLE_FEATURE.example = vitte check path/to/file.vit

SYNTAX_E_EXPECTED_IDENTIFIER.summary = Bezeichner erwartet.
SYNTAX_E_EXPECTED_IDENTIFIER.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_EXPECTED_IDENTIFIER.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_EXPECTED_IDENTIFIER.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_EXPECTED_IDENTIFIER.example = proc main() -> int { give 0; }

SYNTAX_E_EXPECTED_EXPRESSION.summary = Ausdruck erwartet.
SYNTAX_E_EXPECTED_EXPRESSION.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_EXPECTED_EXPRESSION.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_EXPECTED_EXPRESSION.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_EXPECTED_EXPRESSION.example = proc main() -> int { give 0; }

SYNTAX_E_EXPECTED_TYPE.summary = Typ erwartet.
SYNTAX_E_EXPECTED_TYPE.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_EXPECTED_TYPE.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_EXPECTED_TYPE.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_EXPECTED_TYPE.example = proc main() -> int { give 0; }

SYNTAX_E_EXPECTED_PATTERN.summary = Muster erwartet.
SYNTAX_E_EXPECTED_PATTERN.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_EXPECTED_PATTERN.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_EXPECTED_PATTERN.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_EXPECTED_PATTERN.example = proc main() -> int { give 0; }

SYNTAX_E_EXPECTED_BLOCK.summary = Block erwartet.
SYNTAX_E_EXPECTED_BLOCK.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_EXPECTED_BLOCK.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_EXPECTED_BLOCK.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_EXPECTED_BLOCK.example = proc main() -> int { give 0; }

SYNTAX_E_EXPECTED_DELIMITER.summary = Trennzeichen erwartet.
SYNTAX_E_EXPECTED_DELIMITER.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_EXPECTED_DELIMITER.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_EXPECTED_DELIMITER.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_EXPECTED_DELIMITER.example = proc main() -> int { give 0; }

SYNTAX_E_UNEXPECTED_TOKEN.summary = Token unerwartet.
SYNTAX_E_UNEXPECTED_TOKEN.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_UNEXPECTED_TOKEN.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_UNEXPECTED_TOKEN.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_UNEXPECTED_TOKEN.example = proc main() -> int { give 0; }

SYNTAX_E_UNBALANCED_DELIMITER.summary = unbalanced Trennzeichen.
SYNTAX_E_UNBALANCED_DELIMITER.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_UNBALANCED_DELIMITER.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_UNBALANCED_DELIMITER.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_UNBALANCED_DELIMITER.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_ATTRIBUTE.summary = attribute ungueltig.
SYNTAX_E_INVALID_ATTRIBUTE.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_ATTRIBUTE.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_ATTRIBUTE.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_ATTRIBUTE.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_DECLARATION.summary = Deklaration ungueltig.
SYNTAX_E_INVALID_DECLARATION.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_DECLARATION.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_DECLARATION.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_DECLARATION.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_STATEMENT.summary = Anweisung ungueltig.
SYNTAX_E_INVALID_STATEMENT.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_STATEMENT.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_STATEMENT.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_STATEMENT.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_EXPRESSION.summary = Ausdruck ungueltig.
SYNTAX_E_INVALID_EXPRESSION.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_EXPRESSION.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_EXPRESSION.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_EXPRESSION.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_PATTERN.summary = Muster ungueltig.
SYNTAX_E_INVALID_PATTERN.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_PATTERN.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_PATTERN.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_PATTERN.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_LITERAL.summary = Literal ungueltig.
SYNTAX_E_INVALID_LITERAL.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_LITERAL.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_LITERAL.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_LITERAL.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_OPERATOR.summary = Operator ungueltig.
SYNTAX_E_INVALID_OPERATOR.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_OPERATOR.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_OPERATOR.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_OPERATOR.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_MODIFIER.summary = modifier ungueltig.
SYNTAX_E_INVALID_MODIFIER.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_MODIFIER.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_MODIFIER.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_MODIFIER.example = proc main() -> int { give 0; }

SYNTAX_E_MISSING_BODY.summary = body fehlt.
SYNTAX_E_MISSING_BODY.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_MISSING_BODY.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_MISSING_BODY.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_MISSING_BODY.example = proc main() -> int { give 0; }

SYNTAX_E_MISSING_RETURN.summary = Rueckgabe fehlt.
SYNTAX_E_MISSING_RETURN.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_MISSING_RETURN.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_MISSING_RETURN.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_MISSING_RETURN.example = proc main() -> int { give 0; }

SYNTAX_E_DUPLICATE_NAME.summary = Name doppelt.
SYNTAX_E_DUPLICATE_NAME.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_DUPLICATE_NAME.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_DUPLICATE_NAME.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_DUPLICATE_NAME.example = proc main() -> int { give 0; }

SYNTAX_E_UNKNOWN_NAME.summary = Name unbekannt.
SYNTAX_E_UNKNOWN_NAME.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_UNKNOWN_NAME.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_UNKNOWN_NAME.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_UNKNOWN_NAME.example = proc main() -> int { give 0; }

SYNTAX_E_UNKNOWN_TYPE.summary = Typ unbekannt.
SYNTAX_E_UNKNOWN_TYPE.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_UNKNOWN_TYPE.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_UNKNOWN_TYPE.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_UNKNOWN_TYPE.example = proc main() -> int { give 0; }

SYNTAX_E_UNKNOWN_MODULE.summary = Modul unbekannt.
SYNTAX_E_UNKNOWN_MODULE.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_UNKNOWN_MODULE.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_UNKNOWN_MODULE.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_UNKNOWN_MODULE.example = proc main() -> int { give 0; }

SYNTAX_E_UNKNOWN_MEMBER.summary = Member unbekannt.
SYNTAX_E_UNKNOWN_MEMBER.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_UNKNOWN_MEMBER.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_UNKNOWN_MEMBER.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_UNKNOWN_MEMBER.example = proc main() -> int { give 0; }

SYNTAX_E_AMBIGUOUS_NAME.summary = ambiguous Name.
SYNTAX_E_AMBIGUOUS_NAME.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_AMBIGUOUS_NAME.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_AMBIGUOUS_NAME.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_AMBIGUOUS_NAME.example = proc main() -> int { give 0; }

SYNTAX_E_PRIVATE_SYMBOL.summary = private Symbol.
SYNTAX_E_PRIVATE_SYMBOL.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_PRIVATE_SYMBOL.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_PRIVATE_SYMBOL.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_PRIVATE_SYMBOL.example = proc main() -> int { give 0; }

SYNTAX_E_IMPORT_NOT_FOUND.summary = Import fehlend.
SYNTAX_E_IMPORT_NOT_FOUND.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_IMPORT_NOT_FOUND.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_IMPORT_NOT_FOUND.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_IMPORT_NOT_FOUND.example = proc main() -> int { give 0; }

SYNTAX_E_IMPORT_CYCLE.summary = Import Zyklus.
SYNTAX_E_IMPORT_CYCLE.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_IMPORT_CYCLE.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_IMPORT_CYCLE.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_IMPORT_CYCLE.example = proc main() -> int { give 0; }

SYNTAX_E_EXPORT_CONFLICT.summary = export Konflikt.
SYNTAX_E_EXPORT_CONFLICT.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_EXPORT_CONFLICT.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_EXPORT_CONFLICT.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_EXPORT_CONFLICT.example = proc main() -> int { give 0; }

SYNTAX_E_ARITY_MISMATCH.summary = arity Nichtuebereinstimmung.
SYNTAX_E_ARITY_MISMATCH.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_ARITY_MISMATCH.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_ARITY_MISMATCH.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_ARITY_MISMATCH.example = proc main() -> int { give 0; }

SYNTAX_E_ARGUMENT_MISMATCH.summary = Argument Nichtuebereinstimmung.
SYNTAX_E_ARGUMENT_MISMATCH.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_ARGUMENT_MISMATCH.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_ARGUMENT_MISMATCH.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_ARGUMENT_MISMATCH.example = proc main() -> int { give 0; }

SYNTAX_E_ASSIGNMENT_MISMATCH.summary = Zuweisung Nichtuebereinstimmung.
SYNTAX_E_ASSIGNMENT_MISMATCH.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_ASSIGNMENT_MISMATCH.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_ASSIGNMENT_MISMATCH.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_ASSIGNMENT_MISMATCH.example = proc main() -> int { give 0; }

SYNTAX_E_BRANCH_MISMATCH.summary = branch Nichtuebereinstimmung.
SYNTAX_E_BRANCH_MISMATCH.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_BRANCH_MISMATCH.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_BRANCH_MISMATCH.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_BRANCH_MISMATCH.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_CALL.summary = Aufruf ungueltig.
SYNTAX_E_INVALID_CALL.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_CALL.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_CALL.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_CALL.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_CAST.summary = Umwandlung ungueltig.
SYNTAX_E_INVALID_CAST.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_CAST.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_CAST.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_CAST.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_INDEX.summary = Index ungueltig.
SYNTAX_E_INVALID_INDEX.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_INDEX.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_INDEX.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_INDEX.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_DEREF.summary = deref ungueltig.
SYNTAX_E_INVALID_DEREF.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_DEREF.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_DEREF.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_DEREF.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_BORROW.summary = Ausleihe ungueltig.
SYNTAX_E_INVALID_BORROW.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_BORROW.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_BORROW.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_BORROW.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_MOVE.summary = Verschiebung ungueltig.
SYNTAX_E_INVALID_MOVE.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_MOVE.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_MOVE.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_MOVE.example = proc main() -> int { give 0; }

SYNTAX_E_USE_AFTER_MOVE.summary = use after Verschiebung.
SYNTAX_E_USE_AFTER_MOVE.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_USE_AFTER_MOVE.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_USE_AFTER_MOVE.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_USE_AFTER_MOVE.example = proc main() -> int { give 0; }

SYNTAX_E_USE_AFTER_DROP.summary = use after Freigabe.
SYNTAX_E_USE_AFTER_DROP.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_USE_AFTER_DROP.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_USE_AFTER_DROP.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_USE_AFTER_DROP.example = proc main() -> int { give 0; }

SYNTAX_E_USE_BEFORE_INIT.summary = use before init.
SYNTAX_E_USE_BEFORE_INIT.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_USE_BEFORE_INIT.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_USE_BEFORE_INIT.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_USE_BEFORE_INIT.example = proc main() -> int { give 0; }

SYNTAX_E_DOUBLE_DROP.summary = double Freigabe.
SYNTAX_E_DOUBLE_DROP.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_DOUBLE_DROP.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_DOUBLE_DROP.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_DOUBLE_DROP.example = proc main() -> int { give 0; }

SYNTAX_E_BORROW_CONFLICT.summary = Ausleihe Konflikt.
SYNTAX_E_BORROW_CONFLICT.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_BORROW_CONFLICT.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_BORROW_CONFLICT.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_BORROW_CONFLICT.example = proc main() -> int { give 0; }

SYNTAX_E_MUTABILITY_CONFLICT.summary = mutability Konflikt.
SYNTAX_E_MUTABILITY_CONFLICT.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_MUTABILITY_CONFLICT.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_MUTABILITY_CONFLICT.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_MUTABILITY_CONFLICT.example = proc main() -> int { give 0; }

SYNTAX_E_LIFETIME_TOO_SHORT.summary = Lebensdauer zu short.
SYNTAX_E_LIFETIME_TOO_SHORT.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_LIFETIME_TOO_SHORT.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_LIFETIME_TOO_SHORT.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_LIFETIME_TOO_SHORT.example = proc main() -> int { give 0; }

SYNTAX_E_DANGLING_REFERENCE.summary = dangling Referenz.
SYNTAX_E_DANGLING_REFERENCE.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_DANGLING_REFERENCE.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_DANGLING_REFERENCE.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_DANGLING_REFERENCE.example = proc main() -> int { give 0; }

SYNTAX_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
SYNTAX_E_NON_EXHAUSTIVE_MATCH.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_NON_EXHAUSTIVE_MATCH.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_NON_EXHAUSTIVE_MATCH.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_NON_EXHAUSTIVE_MATCH.example = proc main() -> int { give 0; }

SYNTAX_E_UNREACHABLE_PATTERN.summary = unerreichbar Muster.
SYNTAX_E_UNREACHABLE_PATTERN.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_UNREACHABLE_PATTERN.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_UNREACHABLE_PATTERN.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_UNREACHABLE_PATTERN.example = proc main() -> int { give 0; }

SYNTAX_E_CONST_REQUIRED.summary = const required.
SYNTAX_E_CONST_REQUIRED.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_CONST_REQUIRED.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_CONST_REQUIRED.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_CONST_REQUIRED.example = proc main() -> int { give 0; }

SYNTAX_E_CONST_OVERFLOW.summary = const Ueberlauf.
SYNTAX_E_CONST_OVERFLOW.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_CONST_OVERFLOW.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_CONST_OVERFLOW.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_CONST_OVERFLOW.example = proc main() -> int { give 0; }

SYNTAX_E_CONST_DIVISION_BY_ZERO.summary = const Division by Null.
SYNTAX_E_CONST_DIVISION_BY_ZERO.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_CONST_DIVISION_BY_ZERO.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_CONST_DIVISION_BY_ZERO.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_CONST_DIVISION_BY_ZERO.example = proc main() -> int { give 0; }

SYNTAX_E_CONST_CYCLE.summary = const Zyklus.
SYNTAX_E_CONST_CYCLE.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_CONST_CYCLE.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_CONST_CYCLE.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_CONST_CYCLE.example = proc main() -> int { give 0; }

SYNTAX_E_MACRO_NOT_FOUND.summary = macro fehlend.
SYNTAX_E_MACRO_NOT_FOUND.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_MACRO_NOT_FOUND.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_MACRO_NOT_FOUND.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_MACRO_NOT_FOUND.example = proc main() -> int { give 0; }

SYNTAX_E_MACRO_RECURSION.summary = macro recursion.
SYNTAX_E_MACRO_RECURSION.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_MACRO_RECURSION.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_MACRO_RECURSION.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_MACRO_RECURSION.example = proc main() -> int { give 0; }

SYNTAX_E_MACRO_EXPANSION_FAILED.summary = macro expansion fehlgeschlagen.
SYNTAX_E_MACRO_EXPANSION_FAILED.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_MACRO_EXPANSION_FAILED.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_MACRO_EXPANSION_FAILED.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_MACRO_EXPANSION_FAILED.example = proc main() -> int { give 0; }

SYNTAX_E_TRAIT_NOT_IMPLEMENTED.summary = trait not implemented.
SYNTAX_E_TRAIT_NOT_IMPLEMENTED.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_TRAIT_NOT_IMPLEMENTED.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_TRAIT_NOT_IMPLEMENTED.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_TRAIT_NOT_IMPLEMENTED.example = proc main() -> int { give 0; }

SYNTAX_E_TRAIT_AMBIGUOUS.summary = trait ambiguous.
SYNTAX_E_TRAIT_AMBIGUOUS.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_TRAIT_AMBIGUOUS.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_TRAIT_AMBIGUOUS.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_TRAIT_AMBIGUOUS.example = proc main() -> int { give 0; }

SYNTAX_E_GENERIC_ARGUMENT_MISSING.summary = generisch Argument fehlend.
SYNTAX_E_GENERIC_ARGUMENT_MISSING.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_GENERIC_ARGUMENT_MISSING.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_GENERIC_ARGUMENT_MISSING.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_GENERIC_ARGUMENT_MISSING.example = proc main() -> int { give 0; }

SYNTAX_E_GENERIC_BOUND_FAILED.summary = generisch bound fehlgeschlagen.
SYNTAX_E_GENERIC_BOUND_FAILED.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_GENERIC_BOUND_FAILED.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_GENERIC_BOUND_FAILED.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_GENERIC_BOUND_FAILED.example = proc main() -> int { give 0; }

SYNTAX_E_UNSUPPORTED_TARGET.summary = Ziel nicht unterstuetzt.
SYNTAX_E_UNSUPPORTED_TARGET.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_UNSUPPORTED_TARGET.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_UNSUPPORTED_TARGET.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_UNSUPPORTED_TARGET.example = proc main() -> int { give 0; }

SYNTAX_E_ABI_MISMATCH.summary = abi Nichtuebereinstimmung.
SYNTAX_E_ABI_MISMATCH.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_ABI_MISMATCH.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_ABI_MISMATCH.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_ABI_MISMATCH.example = proc main() -> int { give 0; }

SYNTAX_E_LINK_FAILED.summary = link fehlgeschlagen.
SYNTAX_E_LINK_FAILED.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_LINK_FAILED.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_LINK_FAILED.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_LINK_FAILED.example = proc main() -> int { give 0; }

SYNTAX_E_RUNTIME_PANIC.summary = runtime Panik.
SYNTAX_E_RUNTIME_PANIC.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_RUNTIME_PANIC.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_RUNTIME_PANIC.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_RUNTIME_PANIC.example = proc main() -> int { give 0; }

NAME_E_EXPECTED_IDENTIFIER.summary = Bezeichner erwartet.
NAME_E_EXPECTED_IDENTIFIER.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_EXPECTED_IDENTIFIER.step1 = Check the spelling and the nearest import or declaration.
NAME_E_EXPECTED_IDENTIFIER.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_EXPECTED_IDENTIFIER.example = use vitte/core

NAME_E_EXPECTED_EXPRESSION.summary = Ausdruck erwartet.
NAME_E_EXPECTED_EXPRESSION.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_EXPECTED_EXPRESSION.step1 = Check the spelling and the nearest import or declaration.
NAME_E_EXPECTED_EXPRESSION.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_EXPECTED_EXPRESSION.example = use vitte/core

NAME_E_EXPECTED_TYPE.summary = Typ erwartet.
NAME_E_EXPECTED_TYPE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_EXPECTED_TYPE.step1 = Check the spelling and the nearest import or declaration.
NAME_E_EXPECTED_TYPE.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_EXPECTED_TYPE.example = use vitte/core

NAME_E_EXPECTED_PATTERN.summary = Muster erwartet.
NAME_E_EXPECTED_PATTERN.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_EXPECTED_PATTERN.step1 = Check the spelling and the nearest import or declaration.
NAME_E_EXPECTED_PATTERN.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_EXPECTED_PATTERN.example = use vitte/core

NAME_E_EXPECTED_BLOCK.summary = Block erwartet.
NAME_E_EXPECTED_BLOCK.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_EXPECTED_BLOCK.step1 = Check the spelling and the nearest import or declaration.
NAME_E_EXPECTED_BLOCK.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_EXPECTED_BLOCK.example = use vitte/core

NAME_E_EXPECTED_DELIMITER.summary = Trennzeichen erwartet.
NAME_E_EXPECTED_DELIMITER.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_EXPECTED_DELIMITER.step1 = Check the spelling and the nearest import or declaration.
NAME_E_EXPECTED_DELIMITER.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_EXPECTED_DELIMITER.example = use vitte/core

NAME_E_UNEXPECTED_TOKEN.summary = Token unerwartet.
NAME_E_UNEXPECTED_TOKEN.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_UNEXPECTED_TOKEN.step1 = Check the spelling and the nearest import or declaration.
NAME_E_UNEXPECTED_TOKEN.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_UNEXPECTED_TOKEN.example = use vitte/core

NAME_E_UNBALANCED_DELIMITER.summary = unbalanced Trennzeichen.
NAME_E_UNBALANCED_DELIMITER.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_UNBALANCED_DELIMITER.step1 = Check the spelling and the nearest import or declaration.
NAME_E_UNBALANCED_DELIMITER.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_UNBALANCED_DELIMITER.example = use vitte/core

NAME_E_INVALID_ATTRIBUTE.summary = attribute ungueltig.
NAME_E_INVALID_ATTRIBUTE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_ATTRIBUTE.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_ATTRIBUTE.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_ATTRIBUTE.example = use vitte/core

NAME_E_INVALID_DECLARATION.summary = Deklaration ungueltig.
NAME_E_INVALID_DECLARATION.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_DECLARATION.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_DECLARATION.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_DECLARATION.example = use vitte/core

NAME_E_INVALID_STATEMENT.summary = Anweisung ungueltig.
NAME_E_INVALID_STATEMENT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_STATEMENT.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_STATEMENT.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_STATEMENT.example = use vitte/core

NAME_E_INVALID_EXPRESSION.summary = Ausdruck ungueltig.
NAME_E_INVALID_EXPRESSION.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_EXPRESSION.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_EXPRESSION.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_EXPRESSION.example = use vitte/core

NAME_E_INVALID_PATTERN.summary = Muster ungueltig.
NAME_E_INVALID_PATTERN.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_PATTERN.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_PATTERN.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_PATTERN.example = use vitte/core

NAME_E_INVALID_LITERAL.summary = Literal ungueltig.
NAME_E_INVALID_LITERAL.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_LITERAL.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_LITERAL.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_LITERAL.example = use vitte/core

NAME_E_INVALID_OPERATOR.summary = Operator ungueltig.
NAME_E_INVALID_OPERATOR.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_OPERATOR.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_OPERATOR.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_OPERATOR.example = use vitte/core

NAME_E_INVALID_MODIFIER.summary = modifier ungueltig.
NAME_E_INVALID_MODIFIER.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_MODIFIER.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_MODIFIER.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_MODIFIER.example = use vitte/core

NAME_E_MISSING_BODY.summary = body fehlt.
NAME_E_MISSING_BODY.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_MISSING_BODY.step1 = Check the spelling and the nearest import or declaration.
NAME_E_MISSING_BODY.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_MISSING_BODY.example = use vitte/core

NAME_E_MISSING_RETURN.summary = Rueckgabe fehlt.
NAME_E_MISSING_RETURN.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_MISSING_RETURN.step1 = Check the spelling and the nearest import or declaration.
NAME_E_MISSING_RETURN.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_MISSING_RETURN.example = use vitte/core

NAME_E_DUPLICATE_NAME.summary = Name doppelt.
NAME_E_DUPLICATE_NAME.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_DUPLICATE_NAME.step1 = Check the spelling and the nearest import or declaration.
NAME_E_DUPLICATE_NAME.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_DUPLICATE_NAME.example = use vitte/core

NAME_E_UNKNOWN_NAME.summary = Name unbekannt.
NAME_E_UNKNOWN_NAME.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_UNKNOWN_NAME.step1 = Check the spelling and the nearest import or declaration.
NAME_E_UNKNOWN_NAME.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_UNKNOWN_NAME.example = use vitte/core

NAME_E_UNKNOWN_TYPE.summary = Typ unbekannt.
NAME_E_UNKNOWN_TYPE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_UNKNOWN_TYPE.step1 = Check the spelling and the nearest import or declaration.
NAME_E_UNKNOWN_TYPE.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_UNKNOWN_TYPE.example = use vitte/core

NAME_E_UNKNOWN_MODULE.summary = Modul unbekannt.
NAME_E_UNKNOWN_MODULE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_UNKNOWN_MODULE.step1 = Check the spelling and the nearest import or declaration.
NAME_E_UNKNOWN_MODULE.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_UNKNOWN_MODULE.example = use vitte/core

NAME_E_UNKNOWN_MEMBER.summary = Member unbekannt.
NAME_E_UNKNOWN_MEMBER.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_UNKNOWN_MEMBER.step1 = Check the spelling and the nearest import or declaration.
NAME_E_UNKNOWN_MEMBER.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_UNKNOWN_MEMBER.example = use vitte/core

NAME_E_AMBIGUOUS_NAME.summary = ambiguous Name.
NAME_E_AMBIGUOUS_NAME.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_AMBIGUOUS_NAME.step1 = Check the spelling and the nearest import or declaration.
NAME_E_AMBIGUOUS_NAME.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_AMBIGUOUS_NAME.example = use vitte/core

NAME_E_PRIVATE_SYMBOL.summary = private Symbol.
NAME_E_PRIVATE_SYMBOL.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_PRIVATE_SYMBOL.step1 = Check the spelling and the nearest import or declaration.
NAME_E_PRIVATE_SYMBOL.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_PRIVATE_SYMBOL.example = use vitte/core

NAME_E_IMPORT_NOT_FOUND.summary = Import fehlend.
NAME_E_IMPORT_NOT_FOUND.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_IMPORT_NOT_FOUND.step1 = Check the spelling and the nearest import or declaration.
NAME_E_IMPORT_NOT_FOUND.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_IMPORT_NOT_FOUND.example = use vitte/core

NAME_E_IMPORT_CYCLE.summary = Import Zyklus.
NAME_E_IMPORT_CYCLE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_IMPORT_CYCLE.step1 = Check the spelling and the nearest import or declaration.
NAME_E_IMPORT_CYCLE.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_IMPORT_CYCLE.example = use vitte/core

NAME_E_EXPORT_CONFLICT.summary = export Konflikt.
NAME_E_EXPORT_CONFLICT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_EXPORT_CONFLICT.step1 = Check the spelling and the nearest import or declaration.
NAME_E_EXPORT_CONFLICT.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_EXPORT_CONFLICT.example = use vitte/core

NAME_E_ARITY_MISMATCH.summary = arity Nichtuebereinstimmung.
NAME_E_ARITY_MISMATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_ARITY_MISMATCH.step1 = Check the spelling and the nearest import or declaration.
NAME_E_ARITY_MISMATCH.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_ARITY_MISMATCH.example = use vitte/core

NAME_E_ARGUMENT_MISMATCH.summary = Argument Nichtuebereinstimmung.
NAME_E_ARGUMENT_MISMATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_ARGUMENT_MISMATCH.step1 = Check the spelling and the nearest import or declaration.
NAME_E_ARGUMENT_MISMATCH.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_ARGUMENT_MISMATCH.example = use vitte/core

NAME_E_ASSIGNMENT_MISMATCH.summary = Zuweisung Nichtuebereinstimmung.
NAME_E_ASSIGNMENT_MISMATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_ASSIGNMENT_MISMATCH.step1 = Check the spelling and the nearest import or declaration.
NAME_E_ASSIGNMENT_MISMATCH.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_ASSIGNMENT_MISMATCH.example = use vitte/core

NAME_E_BRANCH_MISMATCH.summary = branch Nichtuebereinstimmung.
NAME_E_BRANCH_MISMATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_BRANCH_MISMATCH.step1 = Check the spelling and the nearest import or declaration.
NAME_E_BRANCH_MISMATCH.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_BRANCH_MISMATCH.example = use vitte/core

NAME_E_INVALID_CALL.summary = Aufruf ungueltig.
NAME_E_INVALID_CALL.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_CALL.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_CALL.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_CALL.example = use vitte/core

NAME_E_INVALID_CAST.summary = Umwandlung ungueltig.
NAME_E_INVALID_CAST.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_CAST.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_CAST.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_CAST.example = use vitte/core

NAME_E_INVALID_INDEX.summary = Index ungueltig.
NAME_E_INVALID_INDEX.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_INDEX.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_INDEX.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_INDEX.example = use vitte/core

NAME_E_INVALID_DEREF.summary = deref ungueltig.
NAME_E_INVALID_DEREF.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_DEREF.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_DEREF.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_DEREF.example = use vitte/core

NAME_E_INVALID_BORROW.summary = Ausleihe ungueltig.
NAME_E_INVALID_BORROW.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_BORROW.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_BORROW.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_BORROW.example = use vitte/core

NAME_E_INVALID_MOVE.summary = Verschiebung ungueltig.
NAME_E_INVALID_MOVE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_MOVE.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_MOVE.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_MOVE.example = use vitte/core

NAME_E_USE_AFTER_MOVE.summary = use after Verschiebung.
NAME_E_USE_AFTER_MOVE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_USE_AFTER_MOVE.step1 = Check the spelling and the nearest import or declaration.
NAME_E_USE_AFTER_MOVE.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_USE_AFTER_MOVE.example = use vitte/core

NAME_E_USE_AFTER_DROP.summary = use after Freigabe.
NAME_E_USE_AFTER_DROP.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_USE_AFTER_DROP.step1 = Check the spelling and the nearest import or declaration.
NAME_E_USE_AFTER_DROP.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_USE_AFTER_DROP.example = use vitte/core

NAME_E_USE_BEFORE_INIT.summary = use before init.
NAME_E_USE_BEFORE_INIT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_USE_BEFORE_INIT.step1 = Check the spelling and the nearest import or declaration.
NAME_E_USE_BEFORE_INIT.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_USE_BEFORE_INIT.example = use vitte/core

NAME_E_DOUBLE_DROP.summary = double Freigabe.
NAME_E_DOUBLE_DROP.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_DOUBLE_DROP.step1 = Check the spelling and the nearest import or declaration.
NAME_E_DOUBLE_DROP.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_DOUBLE_DROP.example = use vitte/core

NAME_E_BORROW_CONFLICT.summary = Ausleihe Konflikt.
NAME_E_BORROW_CONFLICT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_BORROW_CONFLICT.step1 = Check the spelling and the nearest import or declaration.
NAME_E_BORROW_CONFLICT.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_BORROW_CONFLICT.example = use vitte/core

NAME_E_MUTABILITY_CONFLICT.summary = mutability Konflikt.
NAME_E_MUTABILITY_CONFLICT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_MUTABILITY_CONFLICT.step1 = Check the spelling and the nearest import or declaration.
NAME_E_MUTABILITY_CONFLICT.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_MUTABILITY_CONFLICT.example = use vitte/core

NAME_E_LIFETIME_TOO_SHORT.summary = Lebensdauer zu short.
NAME_E_LIFETIME_TOO_SHORT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_LIFETIME_TOO_SHORT.step1 = Check the spelling and the nearest import or declaration.
NAME_E_LIFETIME_TOO_SHORT.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_LIFETIME_TOO_SHORT.example = use vitte/core

NAME_E_DANGLING_REFERENCE.summary = dangling Referenz.
NAME_E_DANGLING_REFERENCE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_DANGLING_REFERENCE.step1 = Check the spelling and the nearest import or declaration.
NAME_E_DANGLING_REFERENCE.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_DANGLING_REFERENCE.example = use vitte/core

NAME_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
NAME_E_NON_EXHAUSTIVE_MATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_NON_EXHAUSTIVE_MATCH.step1 = Check the spelling and the nearest import or declaration.
NAME_E_NON_EXHAUSTIVE_MATCH.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_NON_EXHAUSTIVE_MATCH.example = use vitte/core

NAME_E_UNREACHABLE_PATTERN.summary = unerreichbar Muster.
NAME_E_UNREACHABLE_PATTERN.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_UNREACHABLE_PATTERN.step1 = Check the spelling and the nearest import or declaration.
NAME_E_UNREACHABLE_PATTERN.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_UNREACHABLE_PATTERN.example = use vitte/core

NAME_E_CONST_REQUIRED.summary = const required.
NAME_E_CONST_REQUIRED.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_CONST_REQUIRED.step1 = Check the spelling and the nearest import or declaration.
NAME_E_CONST_REQUIRED.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_CONST_REQUIRED.example = use vitte/core

NAME_E_CONST_OVERFLOW.summary = const Ueberlauf.
NAME_E_CONST_OVERFLOW.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_CONST_OVERFLOW.step1 = Check the spelling and the nearest import or declaration.
NAME_E_CONST_OVERFLOW.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_CONST_OVERFLOW.example = use vitte/core

NAME_E_CONST_DIVISION_BY_ZERO.summary = const Division by Null.
NAME_E_CONST_DIVISION_BY_ZERO.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_CONST_DIVISION_BY_ZERO.step1 = Check the spelling and the nearest import or declaration.
NAME_E_CONST_DIVISION_BY_ZERO.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_CONST_DIVISION_BY_ZERO.example = use vitte/core

NAME_E_CONST_CYCLE.summary = const Zyklus.
NAME_E_CONST_CYCLE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_CONST_CYCLE.step1 = Check the spelling and the nearest import or declaration.
NAME_E_CONST_CYCLE.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_CONST_CYCLE.example = use vitte/core

NAME_E_MACRO_NOT_FOUND.summary = macro fehlend.
NAME_E_MACRO_NOT_FOUND.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_MACRO_NOT_FOUND.step1 = Check the spelling and the nearest import or declaration.
NAME_E_MACRO_NOT_FOUND.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_MACRO_NOT_FOUND.example = use vitte/core

NAME_E_MACRO_RECURSION.summary = macro recursion.
NAME_E_MACRO_RECURSION.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_MACRO_RECURSION.step1 = Check the spelling and the nearest import or declaration.
NAME_E_MACRO_RECURSION.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_MACRO_RECURSION.example = use vitte/core

NAME_E_MACRO_EXPANSION_FAILED.summary = macro expansion fehlgeschlagen.
NAME_E_MACRO_EXPANSION_FAILED.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_MACRO_EXPANSION_FAILED.step1 = Check the spelling and the nearest import or declaration.
NAME_E_MACRO_EXPANSION_FAILED.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_MACRO_EXPANSION_FAILED.example = use vitte/core

NAME_E_TRAIT_NOT_IMPLEMENTED.summary = trait not implemented.
NAME_E_TRAIT_NOT_IMPLEMENTED.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_TRAIT_NOT_IMPLEMENTED.step1 = Check the spelling and the nearest import or declaration.
NAME_E_TRAIT_NOT_IMPLEMENTED.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_TRAIT_NOT_IMPLEMENTED.example = use vitte/core

NAME_E_TRAIT_AMBIGUOUS.summary = trait ambiguous.
NAME_E_TRAIT_AMBIGUOUS.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_TRAIT_AMBIGUOUS.step1 = Check the spelling and the nearest import or declaration.
NAME_E_TRAIT_AMBIGUOUS.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_TRAIT_AMBIGUOUS.example = use vitte/core

NAME_E_GENERIC_ARGUMENT_MISSING.summary = generisch Argument fehlend.
NAME_E_GENERIC_ARGUMENT_MISSING.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_GENERIC_ARGUMENT_MISSING.step1 = Check the spelling and the nearest import or declaration.
NAME_E_GENERIC_ARGUMENT_MISSING.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_GENERIC_ARGUMENT_MISSING.example = use vitte/core

NAME_E_GENERIC_BOUND_FAILED.summary = generisch bound fehlgeschlagen.
NAME_E_GENERIC_BOUND_FAILED.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_GENERIC_BOUND_FAILED.step1 = Check the spelling and the nearest import or declaration.
NAME_E_GENERIC_BOUND_FAILED.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_GENERIC_BOUND_FAILED.example = use vitte/core

NAME_E_UNSUPPORTED_TARGET.summary = Ziel nicht unterstuetzt.
NAME_E_UNSUPPORTED_TARGET.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_UNSUPPORTED_TARGET.step1 = Check the spelling and the nearest import or declaration.
NAME_E_UNSUPPORTED_TARGET.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_UNSUPPORTED_TARGET.example = use vitte/core

NAME_E_ABI_MISMATCH.summary = abi Nichtuebereinstimmung.
NAME_E_ABI_MISMATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_ABI_MISMATCH.step1 = Check the spelling and the nearest import or declaration.
NAME_E_ABI_MISMATCH.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_ABI_MISMATCH.example = use vitte/core

NAME_E_LINK_FAILED.summary = link fehlgeschlagen.
NAME_E_LINK_FAILED.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_LINK_FAILED.step1 = Check the spelling and the nearest import or declaration.
NAME_E_LINK_FAILED.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_LINK_FAILED.example = use vitte/core

NAME_E_RUNTIME_PANIC.summary = runtime Panik.
NAME_E_RUNTIME_PANIC.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_RUNTIME_PANIC.step1 = Check the spelling and the nearest import or declaration.
NAME_E_RUNTIME_PANIC.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_RUNTIME_PANIC.example = use vitte/core

MODULE_E_EXPECTED_IDENTIFIER.summary = Bezeichner erwartet.
MODULE_E_EXPECTED_IDENTIFIER.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_EXPECTED_IDENTIFIER.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_EXPECTED_IDENTIFIER.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_EXPECTED_IDENTIFIER.example = use vitte/core

MODULE_E_EXPECTED_EXPRESSION.summary = Ausdruck erwartet.
MODULE_E_EXPECTED_EXPRESSION.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_EXPECTED_EXPRESSION.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_EXPECTED_EXPRESSION.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_EXPECTED_EXPRESSION.example = use vitte/core

MODULE_E_EXPECTED_TYPE.summary = Typ erwartet.
MODULE_E_EXPECTED_TYPE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_EXPECTED_TYPE.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_EXPECTED_TYPE.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_EXPECTED_TYPE.example = use vitte/core

MODULE_E_EXPECTED_PATTERN.summary = Muster erwartet.
MODULE_E_EXPECTED_PATTERN.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_EXPECTED_PATTERN.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_EXPECTED_PATTERN.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_EXPECTED_PATTERN.example = use vitte/core

MODULE_E_EXPECTED_BLOCK.summary = Block erwartet.
MODULE_E_EXPECTED_BLOCK.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_EXPECTED_BLOCK.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_EXPECTED_BLOCK.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_EXPECTED_BLOCK.example = use vitte/core

MODULE_E_EXPECTED_DELIMITER.summary = Trennzeichen erwartet.
MODULE_E_EXPECTED_DELIMITER.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_EXPECTED_DELIMITER.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_EXPECTED_DELIMITER.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_EXPECTED_DELIMITER.example = use vitte/core

MODULE_E_UNEXPECTED_TOKEN.summary = Token unerwartet.
MODULE_E_UNEXPECTED_TOKEN.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_UNEXPECTED_TOKEN.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_UNEXPECTED_TOKEN.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_UNEXPECTED_TOKEN.example = use vitte/core

MODULE_E_UNBALANCED_DELIMITER.summary = unbalanced Trennzeichen.
MODULE_E_UNBALANCED_DELIMITER.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_UNBALANCED_DELIMITER.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_UNBALANCED_DELIMITER.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_UNBALANCED_DELIMITER.example = use vitte/core

MODULE_E_INVALID_ATTRIBUTE.summary = attribute ungueltig.
MODULE_E_INVALID_ATTRIBUTE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_ATTRIBUTE.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_ATTRIBUTE.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_ATTRIBUTE.example = use vitte/core

MODULE_E_INVALID_DECLARATION.summary = Deklaration ungueltig.
MODULE_E_INVALID_DECLARATION.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_DECLARATION.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_DECLARATION.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_DECLARATION.example = use vitte/core

MODULE_E_INVALID_STATEMENT.summary = Anweisung ungueltig.
MODULE_E_INVALID_STATEMENT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_STATEMENT.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_STATEMENT.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_STATEMENT.example = use vitte/core

MODULE_E_INVALID_EXPRESSION.summary = Ausdruck ungueltig.
MODULE_E_INVALID_EXPRESSION.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_EXPRESSION.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_EXPRESSION.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_EXPRESSION.example = use vitte/core

MODULE_E_INVALID_PATTERN.summary = Muster ungueltig.
MODULE_E_INVALID_PATTERN.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_PATTERN.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_PATTERN.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_PATTERN.example = use vitte/core

MODULE_E_INVALID_LITERAL.summary = Literal ungueltig.
MODULE_E_INVALID_LITERAL.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_LITERAL.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_LITERAL.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_LITERAL.example = use vitte/core

MODULE_E_INVALID_OPERATOR.summary = Operator ungueltig.
MODULE_E_INVALID_OPERATOR.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_OPERATOR.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_OPERATOR.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_OPERATOR.example = use vitte/core

MODULE_E_INVALID_MODIFIER.summary = modifier ungueltig.
MODULE_E_INVALID_MODIFIER.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_MODIFIER.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_MODIFIER.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_MODIFIER.example = use vitte/core

MODULE_E_MISSING_BODY.summary = body fehlt.
MODULE_E_MISSING_BODY.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_MISSING_BODY.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_MISSING_BODY.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_MISSING_BODY.example = use vitte/core

MODULE_E_MISSING_RETURN.summary = Rueckgabe fehlt.
MODULE_E_MISSING_RETURN.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_MISSING_RETURN.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_MISSING_RETURN.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_MISSING_RETURN.example = use vitte/core

MODULE_E_DUPLICATE_NAME.summary = Name doppelt.
MODULE_E_DUPLICATE_NAME.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_DUPLICATE_NAME.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_DUPLICATE_NAME.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_DUPLICATE_NAME.example = use vitte/core

MODULE_E_UNKNOWN_NAME.summary = Name unbekannt.
MODULE_E_UNKNOWN_NAME.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_UNKNOWN_NAME.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_UNKNOWN_NAME.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_UNKNOWN_NAME.example = use vitte/core

MODULE_E_UNKNOWN_TYPE.summary = Typ unbekannt.
MODULE_E_UNKNOWN_TYPE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_UNKNOWN_TYPE.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_UNKNOWN_TYPE.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_UNKNOWN_TYPE.example = use vitte/core

MODULE_E_UNKNOWN_MODULE.summary = Modul unbekannt.
MODULE_E_UNKNOWN_MODULE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_UNKNOWN_MODULE.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_UNKNOWN_MODULE.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_UNKNOWN_MODULE.example = use vitte/core

MODULE_E_UNKNOWN_MEMBER.summary = Member unbekannt.
MODULE_E_UNKNOWN_MEMBER.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_UNKNOWN_MEMBER.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_UNKNOWN_MEMBER.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_UNKNOWN_MEMBER.example = use vitte/core

MODULE_E_AMBIGUOUS_NAME.summary = ambiguous Name.
MODULE_E_AMBIGUOUS_NAME.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_AMBIGUOUS_NAME.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_AMBIGUOUS_NAME.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_AMBIGUOUS_NAME.example = use vitte/core

MODULE_E_PRIVATE_SYMBOL.summary = private Symbol.
MODULE_E_PRIVATE_SYMBOL.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_PRIVATE_SYMBOL.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_PRIVATE_SYMBOL.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_PRIVATE_SYMBOL.example = use vitte/core

MODULE_E_IMPORT_NOT_FOUND.summary = Import fehlend.
MODULE_E_IMPORT_NOT_FOUND.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_IMPORT_NOT_FOUND.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_IMPORT_NOT_FOUND.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_IMPORT_NOT_FOUND.example = use vitte/core

MODULE_E_IMPORT_CYCLE.summary = Import Zyklus.
MODULE_E_IMPORT_CYCLE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_IMPORT_CYCLE.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_IMPORT_CYCLE.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_IMPORT_CYCLE.example = use vitte/core

MODULE_E_EXPORT_CONFLICT.summary = export Konflikt.
MODULE_E_EXPORT_CONFLICT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_EXPORT_CONFLICT.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_EXPORT_CONFLICT.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_EXPORT_CONFLICT.example = use vitte/core

MODULE_E_ARITY_MISMATCH.summary = arity Nichtuebereinstimmung.
MODULE_E_ARITY_MISMATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_ARITY_MISMATCH.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_ARITY_MISMATCH.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_ARITY_MISMATCH.example = use vitte/core

MODULE_E_ARGUMENT_MISMATCH.summary = Argument Nichtuebereinstimmung.
MODULE_E_ARGUMENT_MISMATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_ARGUMENT_MISMATCH.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_ARGUMENT_MISMATCH.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_ARGUMENT_MISMATCH.example = use vitte/core

MODULE_E_ASSIGNMENT_MISMATCH.summary = Zuweisung Nichtuebereinstimmung.
MODULE_E_ASSIGNMENT_MISMATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_ASSIGNMENT_MISMATCH.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_ASSIGNMENT_MISMATCH.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_ASSIGNMENT_MISMATCH.example = use vitte/core

MODULE_E_BRANCH_MISMATCH.summary = branch Nichtuebereinstimmung.
MODULE_E_BRANCH_MISMATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_BRANCH_MISMATCH.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_BRANCH_MISMATCH.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_BRANCH_MISMATCH.example = use vitte/core

MODULE_E_INVALID_CALL.summary = Aufruf ungueltig.
MODULE_E_INVALID_CALL.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_CALL.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_CALL.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_CALL.example = use vitte/core

MODULE_E_INVALID_CAST.summary = Umwandlung ungueltig.
MODULE_E_INVALID_CAST.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_CAST.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_CAST.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_CAST.example = use vitte/core

MODULE_E_INVALID_INDEX.summary = Index ungueltig.
MODULE_E_INVALID_INDEX.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_INDEX.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_INDEX.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_INDEX.example = use vitte/core

MODULE_E_INVALID_DEREF.summary = deref ungueltig.
MODULE_E_INVALID_DEREF.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_DEREF.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_DEREF.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_DEREF.example = use vitte/core

MODULE_E_INVALID_BORROW.summary = Ausleihe ungueltig.
MODULE_E_INVALID_BORROW.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_BORROW.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_BORROW.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_BORROW.example = use vitte/core

MODULE_E_INVALID_MOVE.summary = Verschiebung ungueltig.
MODULE_E_INVALID_MOVE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_MOVE.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_MOVE.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_MOVE.example = use vitte/core

MODULE_E_USE_AFTER_MOVE.summary = use after Verschiebung.
MODULE_E_USE_AFTER_MOVE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_USE_AFTER_MOVE.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_USE_AFTER_MOVE.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_USE_AFTER_MOVE.example = use vitte/core

MODULE_E_USE_AFTER_DROP.summary = use after Freigabe.
MODULE_E_USE_AFTER_DROP.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_USE_AFTER_DROP.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_USE_AFTER_DROP.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_USE_AFTER_DROP.example = use vitte/core

MODULE_E_USE_BEFORE_INIT.summary = use before init.
MODULE_E_USE_BEFORE_INIT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_USE_BEFORE_INIT.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_USE_BEFORE_INIT.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_USE_BEFORE_INIT.example = use vitte/core

MODULE_E_DOUBLE_DROP.summary = double Freigabe.
MODULE_E_DOUBLE_DROP.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_DOUBLE_DROP.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_DOUBLE_DROP.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_DOUBLE_DROP.example = use vitte/core

MODULE_E_BORROW_CONFLICT.summary = Ausleihe Konflikt.
MODULE_E_BORROW_CONFLICT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_BORROW_CONFLICT.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_BORROW_CONFLICT.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_BORROW_CONFLICT.example = use vitte/core

MODULE_E_MUTABILITY_CONFLICT.summary = mutability Konflikt.
MODULE_E_MUTABILITY_CONFLICT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_MUTABILITY_CONFLICT.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_MUTABILITY_CONFLICT.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_MUTABILITY_CONFLICT.example = use vitte/core

MODULE_E_LIFETIME_TOO_SHORT.summary = Lebensdauer zu short.
MODULE_E_LIFETIME_TOO_SHORT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_LIFETIME_TOO_SHORT.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_LIFETIME_TOO_SHORT.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_LIFETIME_TOO_SHORT.example = use vitte/core

MODULE_E_DANGLING_REFERENCE.summary = dangling Referenz.
MODULE_E_DANGLING_REFERENCE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_DANGLING_REFERENCE.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_DANGLING_REFERENCE.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_DANGLING_REFERENCE.example = use vitte/core

MODULE_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
MODULE_E_NON_EXHAUSTIVE_MATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_NON_EXHAUSTIVE_MATCH.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_NON_EXHAUSTIVE_MATCH.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_NON_EXHAUSTIVE_MATCH.example = use vitte/core

MODULE_E_UNREACHABLE_PATTERN.summary = unerreichbar Muster.
MODULE_E_UNREACHABLE_PATTERN.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_UNREACHABLE_PATTERN.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_UNREACHABLE_PATTERN.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_UNREACHABLE_PATTERN.example = use vitte/core

MODULE_E_CONST_REQUIRED.summary = const required.
MODULE_E_CONST_REQUIRED.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_CONST_REQUIRED.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_CONST_REQUIRED.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_CONST_REQUIRED.example = use vitte/core

MODULE_E_CONST_OVERFLOW.summary = const Ueberlauf.
MODULE_E_CONST_OVERFLOW.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_CONST_OVERFLOW.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_CONST_OVERFLOW.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_CONST_OVERFLOW.example = use vitte/core

MODULE_E_CONST_DIVISION_BY_ZERO.summary = const Division by Null.
MODULE_E_CONST_DIVISION_BY_ZERO.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_CONST_DIVISION_BY_ZERO.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_CONST_DIVISION_BY_ZERO.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_CONST_DIVISION_BY_ZERO.example = use vitte/core

MODULE_E_CONST_CYCLE.summary = const Zyklus.
MODULE_E_CONST_CYCLE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_CONST_CYCLE.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_CONST_CYCLE.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_CONST_CYCLE.example = use vitte/core

MODULE_E_MACRO_NOT_FOUND.summary = macro fehlend.
MODULE_E_MACRO_NOT_FOUND.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_MACRO_NOT_FOUND.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_MACRO_NOT_FOUND.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_MACRO_NOT_FOUND.example = use vitte/core

MODULE_E_MACRO_RECURSION.summary = macro recursion.
MODULE_E_MACRO_RECURSION.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_MACRO_RECURSION.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_MACRO_RECURSION.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_MACRO_RECURSION.example = use vitte/core

MODULE_E_MACRO_EXPANSION_FAILED.summary = macro expansion fehlgeschlagen.
MODULE_E_MACRO_EXPANSION_FAILED.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_MACRO_EXPANSION_FAILED.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_MACRO_EXPANSION_FAILED.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_MACRO_EXPANSION_FAILED.example = use vitte/core

MODULE_E_TRAIT_NOT_IMPLEMENTED.summary = trait not implemented.
MODULE_E_TRAIT_NOT_IMPLEMENTED.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_TRAIT_NOT_IMPLEMENTED.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_TRAIT_NOT_IMPLEMENTED.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_TRAIT_NOT_IMPLEMENTED.example = use vitte/core

MODULE_E_TRAIT_AMBIGUOUS.summary = trait ambiguous.
MODULE_E_TRAIT_AMBIGUOUS.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_TRAIT_AMBIGUOUS.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_TRAIT_AMBIGUOUS.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_TRAIT_AMBIGUOUS.example = use vitte/core

MODULE_E_GENERIC_ARGUMENT_MISSING.summary = generisch Argument fehlend.
MODULE_E_GENERIC_ARGUMENT_MISSING.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_GENERIC_ARGUMENT_MISSING.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_GENERIC_ARGUMENT_MISSING.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_GENERIC_ARGUMENT_MISSING.example = use vitte/core

MODULE_E_GENERIC_BOUND_FAILED.summary = generisch bound fehlgeschlagen.
MODULE_E_GENERIC_BOUND_FAILED.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_GENERIC_BOUND_FAILED.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_GENERIC_BOUND_FAILED.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_GENERIC_BOUND_FAILED.example = use vitte/core

MODULE_E_UNSUPPORTED_TARGET.summary = Ziel nicht unterstuetzt.
MODULE_E_UNSUPPORTED_TARGET.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_UNSUPPORTED_TARGET.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_UNSUPPORTED_TARGET.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_UNSUPPORTED_TARGET.example = use vitte/core

MODULE_E_ABI_MISMATCH.summary = abi Nichtuebereinstimmung.
MODULE_E_ABI_MISMATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_ABI_MISMATCH.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_ABI_MISMATCH.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_ABI_MISMATCH.example = use vitte/core

MODULE_E_LINK_FAILED.summary = link fehlgeschlagen.
MODULE_E_LINK_FAILED.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_LINK_FAILED.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_LINK_FAILED.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_LINK_FAILED.example = use vitte/core

MODULE_E_RUNTIME_PANIC.summary = runtime Panik.
MODULE_E_RUNTIME_PANIC.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_RUNTIME_PANIC.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_RUNTIME_PANIC.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_RUNTIME_PANIC.example = use vitte/core

TYPE_E_EXPECTED_IDENTIFIER.summary = Bezeichner erwartet.
TYPE_E_EXPECTED_IDENTIFIER.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_EXPECTED_IDENTIFIER.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_EXPECTED_IDENTIFIER.fix = make the expression type match the type contract named by the type checker
TYPE_E_EXPECTED_IDENTIFIER.example = let count: int = 1

TYPE_E_EXPECTED_EXPRESSION.summary = Ausdruck erwartet.
TYPE_E_EXPECTED_EXPRESSION.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_EXPECTED_EXPRESSION.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_EXPECTED_EXPRESSION.fix = make the expression type match the type contract named by the type checker
TYPE_E_EXPECTED_EXPRESSION.example = let count: int = 1

TYPE_E_EXPECTED_TYPE.summary = Typ erwartet.
TYPE_E_EXPECTED_TYPE.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_EXPECTED_TYPE.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_EXPECTED_TYPE.fix = make the expression type match the type contract named by the type checker
TYPE_E_EXPECTED_TYPE.example = let count: int = 1

TYPE_E_EXPECTED_PATTERN.summary = Muster erwartet.
TYPE_E_EXPECTED_PATTERN.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_EXPECTED_PATTERN.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_EXPECTED_PATTERN.fix = make the expression type match the type contract named by the type checker
TYPE_E_EXPECTED_PATTERN.example = let count: int = 1

TYPE_E_EXPECTED_BLOCK.summary = Block erwartet.
TYPE_E_EXPECTED_BLOCK.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_EXPECTED_BLOCK.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_EXPECTED_BLOCK.fix = make the expression type match the type contract named by the type checker
TYPE_E_EXPECTED_BLOCK.example = let count: int = 1

TYPE_E_EXPECTED_DELIMITER.summary = Trennzeichen erwartet.
TYPE_E_EXPECTED_DELIMITER.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_EXPECTED_DELIMITER.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_EXPECTED_DELIMITER.fix = make the expression type match the type contract named by the type checker
TYPE_E_EXPECTED_DELIMITER.example = let count: int = 1

TYPE_E_UNEXPECTED_TOKEN.summary = Token unerwartet.
TYPE_E_UNEXPECTED_TOKEN.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_UNEXPECTED_TOKEN.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_UNEXPECTED_TOKEN.fix = make the expression type match the type contract named by the type checker
TYPE_E_UNEXPECTED_TOKEN.example = let count: int = 1

TYPE_E_UNBALANCED_DELIMITER.summary = unbalanced Trennzeichen.
TYPE_E_UNBALANCED_DELIMITER.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_UNBALANCED_DELIMITER.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_UNBALANCED_DELIMITER.fix = make the expression type match the type contract named by the type checker
TYPE_E_UNBALANCED_DELIMITER.example = let count: int = 1

TYPE_E_INVALID_ATTRIBUTE.summary = attribute ungueltig.
TYPE_E_INVALID_ATTRIBUTE.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_ATTRIBUTE.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_ATTRIBUTE.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_ATTRIBUTE.example = let count: int = 1

TYPE_E_INVALID_DECLARATION.summary = Deklaration ungueltig.
TYPE_E_INVALID_DECLARATION.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_DECLARATION.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_DECLARATION.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_DECLARATION.example = let count: int = 1

TYPE_E_INVALID_STATEMENT.summary = Anweisung ungueltig.
TYPE_E_INVALID_STATEMENT.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_STATEMENT.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_STATEMENT.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_STATEMENT.example = let count: int = 1

TYPE_E_INVALID_EXPRESSION.summary = Ausdruck ungueltig.
TYPE_E_INVALID_EXPRESSION.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_EXPRESSION.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_EXPRESSION.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_EXPRESSION.example = let count: int = 1

TYPE_E_INVALID_PATTERN.summary = Muster ungueltig.
TYPE_E_INVALID_PATTERN.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_PATTERN.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_PATTERN.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_PATTERN.example = let count: int = 1

TYPE_E_INVALID_LITERAL.summary = Literal ungueltig.
TYPE_E_INVALID_LITERAL.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_LITERAL.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_LITERAL.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_LITERAL.example = let count: int = 1

TYPE_E_INVALID_OPERATOR.summary = Operator ungueltig.
TYPE_E_INVALID_OPERATOR.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_OPERATOR.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_OPERATOR.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_OPERATOR.example = let count: int = 1

TYPE_E_INVALID_MODIFIER.summary = modifier ungueltig.
TYPE_E_INVALID_MODIFIER.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_MODIFIER.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_MODIFIER.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_MODIFIER.example = let count: int = 1

TYPE_E_MISSING_BODY.summary = body fehlt.
TYPE_E_MISSING_BODY.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_MISSING_BODY.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_MISSING_BODY.fix = make the expression type match the type contract named by the type checker
TYPE_E_MISSING_BODY.example = let count: int = 1

TYPE_E_MISSING_RETURN.summary = Rueckgabe fehlt.
TYPE_E_MISSING_RETURN.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_MISSING_RETURN.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_MISSING_RETURN.fix = make the expression type match the type contract named by the type checker
TYPE_E_MISSING_RETURN.example = let count: int = 1

TYPE_E_DUPLICATE_NAME.summary = Name doppelt.
TYPE_E_DUPLICATE_NAME.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_DUPLICATE_NAME.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_DUPLICATE_NAME.fix = make the expression type match the type contract named by the type checker
TYPE_E_DUPLICATE_NAME.example = let count: int = 1

TYPE_E_UNKNOWN_NAME.summary = Name unbekannt.
TYPE_E_UNKNOWN_NAME.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_UNKNOWN_NAME.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_UNKNOWN_NAME.fix = make the expression type match the type contract named by the type checker
TYPE_E_UNKNOWN_NAME.example = let count: int = 1

TYPE_E_UNKNOWN_TYPE.summary = Typ unbekannt.
TYPE_E_UNKNOWN_TYPE.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_UNKNOWN_TYPE.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_UNKNOWN_TYPE.fix = make the expression type match the type contract named by the type checker
TYPE_E_UNKNOWN_TYPE.example = let count: int = 1

TYPE_E_UNKNOWN_MODULE.summary = Modul unbekannt.
TYPE_E_UNKNOWN_MODULE.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_UNKNOWN_MODULE.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_UNKNOWN_MODULE.fix = make the expression type match the type contract named by the type checker
TYPE_E_UNKNOWN_MODULE.example = let count: int = 1

TYPE_E_UNKNOWN_MEMBER.summary = Member unbekannt.
TYPE_E_UNKNOWN_MEMBER.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_UNKNOWN_MEMBER.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_UNKNOWN_MEMBER.fix = make the expression type match the type contract named by the type checker
TYPE_E_UNKNOWN_MEMBER.example = let count: int = 1

TYPE_E_AMBIGUOUS_NAME.summary = ambiguous Name.
TYPE_E_AMBIGUOUS_NAME.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_AMBIGUOUS_NAME.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_AMBIGUOUS_NAME.fix = make the expression type match the type contract named by the type checker
TYPE_E_AMBIGUOUS_NAME.example = let count: int = 1

TYPE_E_PRIVATE_SYMBOL.summary = private Symbol.
TYPE_E_PRIVATE_SYMBOL.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_PRIVATE_SYMBOL.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_PRIVATE_SYMBOL.fix = make the expression type match the type contract named by the type checker
TYPE_E_PRIVATE_SYMBOL.example = let count: int = 1

TYPE_E_IMPORT_NOT_FOUND.summary = Import fehlend.
TYPE_E_IMPORT_NOT_FOUND.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_IMPORT_NOT_FOUND.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_IMPORT_NOT_FOUND.fix = make the expression type match the type contract named by the type checker
TYPE_E_IMPORT_NOT_FOUND.example = let count: int = 1

TYPE_E_IMPORT_CYCLE.summary = Import Zyklus.
TYPE_E_IMPORT_CYCLE.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_IMPORT_CYCLE.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_IMPORT_CYCLE.fix = make the expression type match the type contract named by the type checker
TYPE_E_IMPORT_CYCLE.example = let count: int = 1

TYPE_E_EXPORT_CONFLICT.summary = export Konflikt.
TYPE_E_EXPORT_CONFLICT.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_EXPORT_CONFLICT.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_EXPORT_CONFLICT.fix = make the expression type match the type contract named by the type checker
TYPE_E_EXPORT_CONFLICT.example = let count: int = 1

TYPE_E_ARITY_MISMATCH.summary = arity Nichtuebereinstimmung.
TYPE_E_ARITY_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_ARITY_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_ARITY_MISMATCH.fix = make the expression type match the type contract named by the type checker
TYPE_E_ARITY_MISMATCH.example = let count: int = 1

TYPE_E_ARGUMENT_MISMATCH.summary = Argument Nichtuebereinstimmung.
TYPE_E_ARGUMENT_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_ARGUMENT_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_ARGUMENT_MISMATCH.fix = make the expression type match the type contract named by the type checker
TYPE_E_ARGUMENT_MISMATCH.example = let count: int = 1

TYPE_E_ASSIGNMENT_MISMATCH.summary = Zuweisung Nichtuebereinstimmung.
TYPE_E_ASSIGNMENT_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_ASSIGNMENT_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_ASSIGNMENT_MISMATCH.fix = make the expression type match the type contract named by the type checker
TYPE_E_ASSIGNMENT_MISMATCH.example = let count: int = 1

TYPE_E_BRANCH_MISMATCH.summary = branch Nichtuebereinstimmung.
TYPE_E_BRANCH_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_BRANCH_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_BRANCH_MISMATCH.fix = make the expression type match the type contract named by the type checker
TYPE_E_BRANCH_MISMATCH.example = let count: int = 1

TYPE_E_INVALID_CALL.summary = Aufruf ungueltig.
TYPE_E_INVALID_CALL.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_CALL.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_CALL.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_CALL.example = let count: int = 1

TYPE_E_INVALID_CAST.summary = Umwandlung ungueltig.
TYPE_E_INVALID_CAST.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_CAST.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_CAST.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_CAST.example = let count: int = 1

TYPE_E_INVALID_INDEX.summary = Index ungueltig.
TYPE_E_INVALID_INDEX.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_INDEX.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_INDEX.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_INDEX.example = let count: int = 1

TYPE_E_INVALID_DEREF.summary = deref ungueltig.
TYPE_E_INVALID_DEREF.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_DEREF.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_DEREF.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_DEREF.example = let count: int = 1

TYPE_E_INVALID_BORROW.summary = Ausleihe ungueltig.
TYPE_E_INVALID_BORROW.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_BORROW.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_BORROW.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_BORROW.example = let count: int = 1

TYPE_E_INVALID_MOVE.summary = Verschiebung ungueltig.
TYPE_E_INVALID_MOVE.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_MOVE.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_MOVE.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_MOVE.example = let count: int = 1

TYPE_E_USE_AFTER_MOVE.summary = use after Verschiebung.
TYPE_E_USE_AFTER_MOVE.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_USE_AFTER_MOVE.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_USE_AFTER_MOVE.fix = make the expression type match the type contract named by the type checker
TYPE_E_USE_AFTER_MOVE.example = let count: int = 1

TYPE_E_USE_AFTER_DROP.summary = use after Freigabe.
TYPE_E_USE_AFTER_DROP.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_USE_AFTER_DROP.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_USE_AFTER_DROP.fix = make the expression type match the type contract named by the type checker
TYPE_E_USE_AFTER_DROP.example = let count: int = 1

TYPE_E_USE_BEFORE_INIT.summary = use before init.
TYPE_E_USE_BEFORE_INIT.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_USE_BEFORE_INIT.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_USE_BEFORE_INIT.fix = make the expression type match the type contract named by the type checker
TYPE_E_USE_BEFORE_INIT.example = let count: int = 1

TYPE_E_DOUBLE_DROP.summary = double Freigabe.
TYPE_E_DOUBLE_DROP.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_DOUBLE_DROP.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_DOUBLE_DROP.fix = make the expression type match the type contract named by the type checker
TYPE_E_DOUBLE_DROP.example = let count: int = 1

TYPE_E_BORROW_CONFLICT.summary = Ausleihe Konflikt.
TYPE_E_BORROW_CONFLICT.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_BORROW_CONFLICT.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_BORROW_CONFLICT.fix = make the expression type match the type contract named by the type checker
TYPE_E_BORROW_CONFLICT.example = let count: int = 1

TYPE_E_MUTABILITY_CONFLICT.summary = mutability Konflikt.
TYPE_E_MUTABILITY_CONFLICT.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_MUTABILITY_CONFLICT.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_MUTABILITY_CONFLICT.fix = make the expression type match the type contract named by the type checker
TYPE_E_MUTABILITY_CONFLICT.example = let count: int = 1

TYPE_E_LIFETIME_TOO_SHORT.summary = Lebensdauer zu short.
TYPE_E_LIFETIME_TOO_SHORT.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_LIFETIME_TOO_SHORT.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_LIFETIME_TOO_SHORT.fix = make the expression type match the type contract named by the type checker
TYPE_E_LIFETIME_TOO_SHORT.example = let count: int = 1

TYPE_E_DANGLING_REFERENCE.summary = dangling Referenz.
TYPE_E_DANGLING_REFERENCE.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_DANGLING_REFERENCE.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_DANGLING_REFERENCE.fix = make the expression type match the type contract named by the type checker
TYPE_E_DANGLING_REFERENCE.example = let count: int = 1

TYPE_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
TYPE_E_NON_EXHAUSTIVE_MATCH.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_NON_EXHAUSTIVE_MATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_NON_EXHAUSTIVE_MATCH.fix = make the expression type match the type contract named by the type checker
TYPE_E_NON_EXHAUSTIVE_MATCH.example = let count: int = 1

TYPE_E_UNREACHABLE_PATTERN.summary = unerreichbar Muster.
TYPE_E_UNREACHABLE_PATTERN.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_UNREACHABLE_PATTERN.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_UNREACHABLE_PATTERN.fix = make the expression type match the type contract named by the type checker
TYPE_E_UNREACHABLE_PATTERN.example = let count: int = 1

TYPE_E_CONST_REQUIRED.summary = const required.
TYPE_E_CONST_REQUIRED.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_CONST_REQUIRED.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_CONST_REQUIRED.fix = make the expression type match the type contract named by the type checker
TYPE_E_CONST_REQUIRED.example = let count: int = 1

TYPE_E_CONST_OVERFLOW.summary = const Ueberlauf.
TYPE_E_CONST_OVERFLOW.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_CONST_OVERFLOW.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_CONST_OVERFLOW.fix = make the expression type match the type contract named by the type checker
TYPE_E_CONST_OVERFLOW.example = let count: int = 1

TYPE_E_CONST_DIVISION_BY_ZERO.summary = const Division by Null.
TYPE_E_CONST_DIVISION_BY_ZERO.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_CONST_DIVISION_BY_ZERO.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_CONST_DIVISION_BY_ZERO.fix = make the expression type match the type contract named by the type checker
TYPE_E_CONST_DIVISION_BY_ZERO.example = let count: int = 1

TYPE_E_CONST_CYCLE.summary = const Zyklus.
TYPE_E_CONST_CYCLE.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_CONST_CYCLE.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_CONST_CYCLE.fix = make the expression type match the type contract named by the type checker
TYPE_E_CONST_CYCLE.example = let count: int = 1

TYPE_E_MACRO_NOT_FOUND.summary = macro fehlend.
TYPE_E_MACRO_NOT_FOUND.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_MACRO_NOT_FOUND.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_MACRO_NOT_FOUND.fix = make the expression type match the type contract named by the type checker
TYPE_E_MACRO_NOT_FOUND.example = let count: int = 1

TYPE_E_MACRO_RECURSION.summary = macro recursion.
TYPE_E_MACRO_RECURSION.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_MACRO_RECURSION.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_MACRO_RECURSION.fix = make the expression type match the type contract named by the type checker
TYPE_E_MACRO_RECURSION.example = let count: int = 1

TYPE_E_MACRO_EXPANSION_FAILED.summary = macro expansion fehlgeschlagen.
TYPE_E_MACRO_EXPANSION_FAILED.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_MACRO_EXPANSION_FAILED.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_MACRO_EXPANSION_FAILED.fix = make the expression type match the type contract named by the type checker
TYPE_E_MACRO_EXPANSION_FAILED.example = let count: int = 1

TYPE_E_TRAIT_NOT_IMPLEMENTED.summary = trait not implemented.
TYPE_E_TRAIT_NOT_IMPLEMENTED.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_TRAIT_NOT_IMPLEMENTED.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_TRAIT_NOT_IMPLEMENTED.fix = make the expression type match the type contract named by the type checker
TYPE_E_TRAIT_NOT_IMPLEMENTED.example = let count: int = 1

TYPE_E_TRAIT_AMBIGUOUS.summary = trait ambiguous.
TYPE_E_TRAIT_AMBIGUOUS.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_TRAIT_AMBIGUOUS.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_TRAIT_AMBIGUOUS.fix = make the expression type match the type contract named by the type checker
TYPE_E_TRAIT_AMBIGUOUS.example = let count: int = 1

TYPE_E_GENERIC_ARGUMENT_MISSING.summary = generisch Argument fehlend.
TYPE_E_GENERIC_ARGUMENT_MISSING.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_GENERIC_ARGUMENT_MISSING.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_GENERIC_ARGUMENT_MISSING.fix = make the expression type match the type contract named by the type checker
TYPE_E_GENERIC_ARGUMENT_MISSING.example = let count: int = 1

TYPE_E_GENERIC_BOUND_FAILED.summary = generisch bound fehlgeschlagen.
TYPE_E_GENERIC_BOUND_FAILED.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_GENERIC_BOUND_FAILED.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_GENERIC_BOUND_FAILED.fix = make the expression type match the type contract named by the type checker
TYPE_E_GENERIC_BOUND_FAILED.example = let count: int = 1

TYPE_E_UNSUPPORTED_TARGET.summary = Ziel nicht unterstuetzt.
TYPE_E_UNSUPPORTED_TARGET.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_UNSUPPORTED_TARGET.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_UNSUPPORTED_TARGET.fix = make the expression type match the type contract named by the type checker
TYPE_E_UNSUPPORTED_TARGET.example = let count: int = 1

TYPE_E_ABI_MISMATCH.summary = abi Nichtuebereinstimmung.
TYPE_E_ABI_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_ABI_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_ABI_MISMATCH.fix = make the expression type match the type contract named by the type checker
TYPE_E_ABI_MISMATCH.example = let count: int = 1

TYPE_E_LINK_FAILED.summary = link fehlgeschlagen.
TYPE_E_LINK_FAILED.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_LINK_FAILED.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_LINK_FAILED.fix = make the expression type match the type contract named by the type checker
TYPE_E_LINK_FAILED.example = let count: int = 1

TYPE_E_RUNTIME_PANIC.summary = runtime Panik.
TYPE_E_RUNTIME_PANIC.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_RUNTIME_PANIC.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_RUNTIME_PANIC.fix = make the expression type match the type contract named by the type checker
TYPE_E_RUNTIME_PANIC.example = let count: int = 1

GENERIC_E_EXPECTED_IDENTIFIER.summary = Bezeichner erwartet.
GENERIC_E_EXPECTED_IDENTIFIER.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_EXPECTED_IDENTIFIER.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_EXPECTED_IDENTIFIER.fix = make the expression type match the type contract named by the type checker
GENERIC_E_EXPECTED_IDENTIFIER.example = let count: int = 1

GENERIC_E_EXPECTED_EXPRESSION.summary = Ausdruck erwartet.
GENERIC_E_EXPECTED_EXPRESSION.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_EXPECTED_EXPRESSION.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_EXPECTED_EXPRESSION.fix = make the expression type match the type contract named by the type checker
GENERIC_E_EXPECTED_EXPRESSION.example = let count: int = 1

GENERIC_E_EXPECTED_TYPE.summary = Typ erwartet.
GENERIC_E_EXPECTED_TYPE.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_EXPECTED_TYPE.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_EXPECTED_TYPE.fix = make the expression type match the type contract named by the type checker
GENERIC_E_EXPECTED_TYPE.example = let count: int = 1

GENERIC_E_EXPECTED_PATTERN.summary = Muster erwartet.
GENERIC_E_EXPECTED_PATTERN.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_EXPECTED_PATTERN.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_EXPECTED_PATTERN.fix = make the expression type match the type contract named by the type checker
GENERIC_E_EXPECTED_PATTERN.example = let count: int = 1

GENERIC_E_EXPECTED_BLOCK.summary = Block erwartet.
GENERIC_E_EXPECTED_BLOCK.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_EXPECTED_BLOCK.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_EXPECTED_BLOCK.fix = make the expression type match the type contract named by the type checker
GENERIC_E_EXPECTED_BLOCK.example = let count: int = 1

GENERIC_E_EXPECTED_DELIMITER.summary = Trennzeichen erwartet.
GENERIC_E_EXPECTED_DELIMITER.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_EXPECTED_DELIMITER.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_EXPECTED_DELIMITER.fix = make the expression type match the type contract named by the type checker
GENERIC_E_EXPECTED_DELIMITER.example = let count: int = 1

GENERIC_E_UNEXPECTED_TOKEN.summary = Token unerwartet.
GENERIC_E_UNEXPECTED_TOKEN.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_UNEXPECTED_TOKEN.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_UNEXPECTED_TOKEN.fix = make the expression type match the type contract named by the type checker
GENERIC_E_UNEXPECTED_TOKEN.example = let count: int = 1

GENERIC_E_UNBALANCED_DELIMITER.summary = unbalanced Trennzeichen.
GENERIC_E_UNBALANCED_DELIMITER.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_UNBALANCED_DELIMITER.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_UNBALANCED_DELIMITER.fix = make the expression type match the type contract named by the type checker
GENERIC_E_UNBALANCED_DELIMITER.example = let count: int = 1

GENERIC_E_INVALID_ATTRIBUTE.summary = attribute ungueltig.
GENERIC_E_INVALID_ATTRIBUTE.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_ATTRIBUTE.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_ATTRIBUTE.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_ATTRIBUTE.example = let count: int = 1

GENERIC_E_INVALID_DECLARATION.summary = Deklaration ungueltig.
GENERIC_E_INVALID_DECLARATION.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_DECLARATION.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_DECLARATION.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_DECLARATION.example = let count: int = 1

GENERIC_E_INVALID_STATEMENT.summary = Anweisung ungueltig.
GENERIC_E_INVALID_STATEMENT.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_STATEMENT.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_STATEMENT.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_STATEMENT.example = let count: int = 1

GENERIC_E_INVALID_EXPRESSION.summary = Ausdruck ungueltig.
GENERIC_E_INVALID_EXPRESSION.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_EXPRESSION.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_EXPRESSION.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_EXPRESSION.example = let count: int = 1

GENERIC_E_INVALID_PATTERN.summary = Muster ungueltig.
GENERIC_E_INVALID_PATTERN.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_PATTERN.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_PATTERN.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_PATTERN.example = let count: int = 1

GENERIC_E_INVALID_LITERAL.summary = Literal ungueltig.
GENERIC_E_INVALID_LITERAL.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_LITERAL.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_LITERAL.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_LITERAL.example = let count: int = 1

GENERIC_E_INVALID_OPERATOR.summary = Operator ungueltig.
GENERIC_E_INVALID_OPERATOR.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_OPERATOR.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_OPERATOR.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_OPERATOR.example = let count: int = 1

GENERIC_E_INVALID_MODIFIER.summary = modifier ungueltig.
GENERIC_E_INVALID_MODIFIER.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_MODIFIER.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_MODIFIER.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_MODIFIER.example = let count: int = 1

GENERIC_E_MISSING_BODY.summary = body fehlt.
GENERIC_E_MISSING_BODY.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_MISSING_BODY.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_MISSING_BODY.fix = make the expression type match the type contract named by the type checker
GENERIC_E_MISSING_BODY.example = let count: int = 1

GENERIC_E_MISSING_RETURN.summary = Rueckgabe fehlt.
GENERIC_E_MISSING_RETURN.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_MISSING_RETURN.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_MISSING_RETURN.fix = make the expression type match the type contract named by the type checker
GENERIC_E_MISSING_RETURN.example = let count: int = 1

GENERIC_E_DUPLICATE_NAME.summary = Name doppelt.
GENERIC_E_DUPLICATE_NAME.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_DUPLICATE_NAME.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_DUPLICATE_NAME.fix = make the expression type match the type contract named by the type checker
GENERIC_E_DUPLICATE_NAME.example = let count: int = 1

GENERIC_E_UNKNOWN_NAME.summary = Name unbekannt.
GENERIC_E_UNKNOWN_NAME.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_UNKNOWN_NAME.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_UNKNOWN_NAME.fix = make the expression type match the type contract named by the type checker
GENERIC_E_UNKNOWN_NAME.example = let count: int = 1

GENERIC_E_UNKNOWN_TYPE.summary = Typ unbekannt.
GENERIC_E_UNKNOWN_TYPE.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_UNKNOWN_TYPE.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_UNKNOWN_TYPE.fix = make the expression type match the type contract named by the type checker
GENERIC_E_UNKNOWN_TYPE.example = let count: int = 1

GENERIC_E_UNKNOWN_MODULE.summary = Modul unbekannt.
GENERIC_E_UNKNOWN_MODULE.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_UNKNOWN_MODULE.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_UNKNOWN_MODULE.fix = make the expression type match the type contract named by the type checker
GENERIC_E_UNKNOWN_MODULE.example = let count: int = 1

GENERIC_E_UNKNOWN_MEMBER.summary = Member unbekannt.
GENERIC_E_UNKNOWN_MEMBER.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_UNKNOWN_MEMBER.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_UNKNOWN_MEMBER.fix = make the expression type match the type contract named by the type checker
GENERIC_E_UNKNOWN_MEMBER.example = let count: int = 1

GENERIC_E_AMBIGUOUS_NAME.summary = ambiguous Name.
GENERIC_E_AMBIGUOUS_NAME.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_AMBIGUOUS_NAME.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_AMBIGUOUS_NAME.fix = make the expression type match the type contract named by the type checker
GENERIC_E_AMBIGUOUS_NAME.example = let count: int = 1

GENERIC_E_PRIVATE_SYMBOL.summary = private Symbol.
GENERIC_E_PRIVATE_SYMBOL.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_PRIVATE_SYMBOL.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_PRIVATE_SYMBOL.fix = make the expression type match the type contract named by the type checker
GENERIC_E_PRIVATE_SYMBOL.example = let count: int = 1

GENERIC_E_IMPORT_NOT_FOUND.summary = Import fehlend.
GENERIC_E_IMPORT_NOT_FOUND.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_IMPORT_NOT_FOUND.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_IMPORT_NOT_FOUND.fix = make the expression type match the type contract named by the type checker
GENERIC_E_IMPORT_NOT_FOUND.example = let count: int = 1

GENERIC_E_IMPORT_CYCLE.summary = Import Zyklus.
GENERIC_E_IMPORT_CYCLE.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_IMPORT_CYCLE.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_IMPORT_CYCLE.fix = make the expression type match the type contract named by the type checker
GENERIC_E_IMPORT_CYCLE.example = let count: int = 1

GENERIC_E_EXPORT_CONFLICT.summary = export Konflikt.
GENERIC_E_EXPORT_CONFLICT.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_EXPORT_CONFLICT.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_EXPORT_CONFLICT.fix = make the expression type match the type contract named by the type checker
GENERIC_E_EXPORT_CONFLICT.example = let count: int = 1

GENERIC_E_ARITY_MISMATCH.summary = arity Nichtuebereinstimmung.
GENERIC_E_ARITY_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_ARITY_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_ARITY_MISMATCH.fix = make the expression type match the type contract named by the type checker
GENERIC_E_ARITY_MISMATCH.example = let count: int = 1

GENERIC_E_ARGUMENT_MISMATCH.summary = Argument Nichtuebereinstimmung.
GENERIC_E_ARGUMENT_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_ARGUMENT_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_ARGUMENT_MISMATCH.fix = make the expression type match the type contract named by the type checker
GENERIC_E_ARGUMENT_MISMATCH.example = let count: int = 1

GENERIC_E_ASSIGNMENT_MISMATCH.summary = Zuweisung Nichtuebereinstimmung.
GENERIC_E_ASSIGNMENT_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_ASSIGNMENT_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_ASSIGNMENT_MISMATCH.fix = make the expression type match the type contract named by the type checker
GENERIC_E_ASSIGNMENT_MISMATCH.example = let count: int = 1

GENERIC_E_BRANCH_MISMATCH.summary = branch Nichtuebereinstimmung.
GENERIC_E_BRANCH_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_BRANCH_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_BRANCH_MISMATCH.fix = make the expression type match the type contract named by the type checker
GENERIC_E_BRANCH_MISMATCH.example = let count: int = 1

GENERIC_E_INVALID_CALL.summary = Aufruf ungueltig.
GENERIC_E_INVALID_CALL.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_CALL.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_CALL.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_CALL.example = let count: int = 1

GENERIC_E_INVALID_CAST.summary = Umwandlung ungueltig.
GENERIC_E_INVALID_CAST.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_CAST.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_CAST.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_CAST.example = let count: int = 1

GENERIC_E_INVALID_INDEX.summary = Index ungueltig.
GENERIC_E_INVALID_INDEX.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_INDEX.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_INDEX.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_INDEX.example = let count: int = 1

GENERIC_E_INVALID_DEREF.summary = deref ungueltig.
GENERIC_E_INVALID_DEREF.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_DEREF.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_DEREF.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_DEREF.example = let count: int = 1

GENERIC_E_INVALID_BORROW.summary = Ausleihe ungueltig.
GENERIC_E_INVALID_BORROW.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_BORROW.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_BORROW.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_BORROW.example = let count: int = 1

GENERIC_E_INVALID_MOVE.summary = Verschiebung ungueltig.
GENERIC_E_INVALID_MOVE.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_MOVE.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_MOVE.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_MOVE.example = let count: int = 1

GENERIC_E_USE_AFTER_MOVE.summary = use after Verschiebung.
GENERIC_E_USE_AFTER_MOVE.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_USE_AFTER_MOVE.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_USE_AFTER_MOVE.fix = make the expression type match the type contract named by the type checker
GENERIC_E_USE_AFTER_MOVE.example = let count: int = 1

GENERIC_E_USE_AFTER_DROP.summary = use after Freigabe.
GENERIC_E_USE_AFTER_DROP.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_USE_AFTER_DROP.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_USE_AFTER_DROP.fix = make the expression type match the type contract named by the type checker
GENERIC_E_USE_AFTER_DROP.example = let count: int = 1

GENERIC_E_USE_BEFORE_INIT.summary = use before init.
GENERIC_E_USE_BEFORE_INIT.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_USE_BEFORE_INIT.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_USE_BEFORE_INIT.fix = make the expression type match the type contract named by the type checker
GENERIC_E_USE_BEFORE_INIT.example = let count: int = 1

GENERIC_E_DOUBLE_DROP.summary = double Freigabe.
GENERIC_E_DOUBLE_DROP.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_DOUBLE_DROP.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_DOUBLE_DROP.fix = make the expression type match the type contract named by the type checker
GENERIC_E_DOUBLE_DROP.example = let count: int = 1

GENERIC_E_BORROW_CONFLICT.summary = Ausleihe Konflikt.
GENERIC_E_BORROW_CONFLICT.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_BORROW_CONFLICT.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_BORROW_CONFLICT.fix = make the expression type match the type contract named by the type checker
GENERIC_E_BORROW_CONFLICT.example = let count: int = 1

GENERIC_E_MUTABILITY_CONFLICT.summary = mutability Konflikt.
GENERIC_E_MUTABILITY_CONFLICT.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_MUTABILITY_CONFLICT.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_MUTABILITY_CONFLICT.fix = make the expression type match the type contract named by the type checker
GENERIC_E_MUTABILITY_CONFLICT.example = let count: int = 1

GENERIC_E_LIFETIME_TOO_SHORT.summary = Lebensdauer zu short.
GENERIC_E_LIFETIME_TOO_SHORT.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_LIFETIME_TOO_SHORT.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_LIFETIME_TOO_SHORT.fix = make the expression type match the type contract named by the type checker
GENERIC_E_LIFETIME_TOO_SHORT.example = let count: int = 1

GENERIC_E_DANGLING_REFERENCE.summary = dangling Referenz.
GENERIC_E_DANGLING_REFERENCE.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_DANGLING_REFERENCE.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_DANGLING_REFERENCE.fix = make the expression type match the type contract named by the type checker
GENERIC_E_DANGLING_REFERENCE.example = let count: int = 1

GENERIC_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
GENERIC_E_NON_EXHAUSTIVE_MATCH.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_NON_EXHAUSTIVE_MATCH.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_NON_EXHAUSTIVE_MATCH.fix = make the expression type match the type contract named by the type checker
GENERIC_E_NON_EXHAUSTIVE_MATCH.example = let count: int = 1

GENERIC_E_UNREACHABLE_PATTERN.summary = unerreichbar Muster.
GENERIC_E_UNREACHABLE_PATTERN.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_UNREACHABLE_PATTERN.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_UNREACHABLE_PATTERN.fix = make the expression type match the type contract named by the type checker
GENERIC_E_UNREACHABLE_PATTERN.example = let count: int = 1

GENERIC_E_CONST_REQUIRED.summary = const required.
GENERIC_E_CONST_REQUIRED.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_CONST_REQUIRED.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_CONST_REQUIRED.fix = make the expression type match the type contract named by the type checker
GENERIC_E_CONST_REQUIRED.example = let count: int = 1

GENERIC_E_CONST_OVERFLOW.summary = const Ueberlauf.
GENERIC_E_CONST_OVERFLOW.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_CONST_OVERFLOW.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_CONST_OVERFLOW.fix = make the expression type match the type contract named by the type checker
GENERIC_E_CONST_OVERFLOW.example = let count: int = 1

GENERIC_E_CONST_DIVISION_BY_ZERO.summary = const Division by Null.
GENERIC_E_CONST_DIVISION_BY_ZERO.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_CONST_DIVISION_BY_ZERO.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_CONST_DIVISION_BY_ZERO.fix = make the expression type match the type contract named by the type checker
GENERIC_E_CONST_DIVISION_BY_ZERO.example = let count: int = 1

GENERIC_E_CONST_CYCLE.summary = const Zyklus.
GENERIC_E_CONST_CYCLE.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_CONST_CYCLE.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_CONST_CYCLE.fix = make the expression type match the type contract named by the type checker
GENERIC_E_CONST_CYCLE.example = let count: int = 1

GENERIC_E_MACRO_NOT_FOUND.summary = macro fehlend.
GENERIC_E_MACRO_NOT_FOUND.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_MACRO_NOT_FOUND.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_MACRO_NOT_FOUND.fix = make the expression type match the type contract named by the type checker
GENERIC_E_MACRO_NOT_FOUND.example = let count: int = 1

GENERIC_E_MACRO_RECURSION.summary = macro recursion.
GENERIC_E_MACRO_RECURSION.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_MACRO_RECURSION.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_MACRO_RECURSION.fix = make the expression type match the type contract named by the type checker
GENERIC_E_MACRO_RECURSION.example = let count: int = 1

GENERIC_E_MACRO_EXPANSION_FAILED.summary = macro expansion fehlgeschlagen.
GENERIC_E_MACRO_EXPANSION_FAILED.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_MACRO_EXPANSION_FAILED.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_MACRO_EXPANSION_FAILED.fix = make the expression type match the type contract named by the type checker
GENERIC_E_MACRO_EXPANSION_FAILED.example = let count: int = 1

GENERIC_E_TRAIT_NOT_IMPLEMENTED.summary = trait not implemented.
GENERIC_E_TRAIT_NOT_IMPLEMENTED.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_TRAIT_NOT_IMPLEMENTED.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_TRAIT_NOT_IMPLEMENTED.fix = make the expression type match the type contract named by the type checker
GENERIC_E_TRAIT_NOT_IMPLEMENTED.example = let count: int = 1

GENERIC_E_TRAIT_AMBIGUOUS.summary = trait ambiguous.
GENERIC_E_TRAIT_AMBIGUOUS.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_TRAIT_AMBIGUOUS.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_TRAIT_AMBIGUOUS.fix = make the expression type match the type contract named by the type checker
GENERIC_E_TRAIT_AMBIGUOUS.example = let count: int = 1

GENERIC_E_GENERIC_ARGUMENT_MISSING.summary = generisch Argument fehlend.
GENERIC_E_GENERIC_ARGUMENT_MISSING.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_GENERIC_ARGUMENT_MISSING.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_GENERIC_ARGUMENT_MISSING.fix = make the expression type match the type contract named by the type checker
GENERIC_E_GENERIC_ARGUMENT_MISSING.example = let count: int = 1

GENERIC_E_GENERIC_BOUND_FAILED.summary = generisch bound fehlgeschlagen.
GENERIC_E_GENERIC_BOUND_FAILED.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_GENERIC_BOUND_FAILED.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_GENERIC_BOUND_FAILED.fix = make the expression type match the type contract named by the type checker
GENERIC_E_GENERIC_BOUND_FAILED.example = let count: int = 1

GENERIC_E_UNSUPPORTED_TARGET.summary = Ziel nicht unterstuetzt.
GENERIC_E_UNSUPPORTED_TARGET.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_UNSUPPORTED_TARGET.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_UNSUPPORTED_TARGET.fix = make the expression type match the type contract named by the type checker
GENERIC_E_UNSUPPORTED_TARGET.example = let count: int = 1

GENERIC_E_ABI_MISMATCH.summary = abi Nichtuebereinstimmung.
GENERIC_E_ABI_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_ABI_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_ABI_MISMATCH.fix = make the expression type match the type contract named by the type checker
GENERIC_E_ABI_MISMATCH.example = let count: int = 1

GENERIC_E_LINK_FAILED.summary = link fehlgeschlagen.
GENERIC_E_LINK_FAILED.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_LINK_FAILED.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_LINK_FAILED.fix = make the expression type match the type contract named by the type checker
GENERIC_E_LINK_FAILED.example = let count: int = 1

GENERIC_E_RUNTIME_PANIC.summary = runtime Panik.
GENERIC_E_RUNTIME_PANIC.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_RUNTIME_PANIC.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_RUNTIME_PANIC.fix = make the expression type match the type contract named by the type checker
GENERIC_E_RUNTIME_PANIC.example = let count: int = 1

TRAIT_E_EXPECTED_IDENTIFIER.summary = Bezeichner erwartet.
TRAIT_E_EXPECTED_IDENTIFIER.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_EXPECTED_IDENTIFIER.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_EXPECTED_IDENTIFIER.fix = make the expression type match the type contract named by the type checker
TRAIT_E_EXPECTED_IDENTIFIER.example = let count: int = 1

TRAIT_E_EXPECTED_EXPRESSION.summary = Ausdruck erwartet.
TRAIT_E_EXPECTED_EXPRESSION.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_EXPECTED_EXPRESSION.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_EXPECTED_EXPRESSION.fix = make the expression type match the type contract named by the type checker
TRAIT_E_EXPECTED_EXPRESSION.example = let count: int = 1

TRAIT_E_EXPECTED_TYPE.summary = Typ erwartet.
TRAIT_E_EXPECTED_TYPE.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_EXPECTED_TYPE.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_EXPECTED_TYPE.fix = make the expression type match the type contract named by the type checker
TRAIT_E_EXPECTED_TYPE.example = let count: int = 1

TRAIT_E_EXPECTED_PATTERN.summary = Muster erwartet.
TRAIT_E_EXPECTED_PATTERN.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_EXPECTED_PATTERN.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_EXPECTED_PATTERN.fix = make the expression type match the type contract named by the type checker
TRAIT_E_EXPECTED_PATTERN.example = let count: int = 1

TRAIT_E_EXPECTED_BLOCK.summary = Block erwartet.
TRAIT_E_EXPECTED_BLOCK.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_EXPECTED_BLOCK.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_EXPECTED_BLOCK.fix = make the expression type match the type contract named by the type checker
TRAIT_E_EXPECTED_BLOCK.example = let count: int = 1

TRAIT_E_EXPECTED_DELIMITER.summary = Trennzeichen erwartet.
TRAIT_E_EXPECTED_DELIMITER.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_EXPECTED_DELIMITER.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_EXPECTED_DELIMITER.fix = make the expression type match the type contract named by the type checker
TRAIT_E_EXPECTED_DELIMITER.example = let count: int = 1

TRAIT_E_UNEXPECTED_TOKEN.summary = Token unerwartet.
TRAIT_E_UNEXPECTED_TOKEN.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_UNEXPECTED_TOKEN.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_UNEXPECTED_TOKEN.fix = make the expression type match the type contract named by the type checker
TRAIT_E_UNEXPECTED_TOKEN.example = let count: int = 1

TRAIT_E_UNBALANCED_DELIMITER.summary = unbalanced Trennzeichen.
TRAIT_E_UNBALANCED_DELIMITER.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_UNBALANCED_DELIMITER.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_UNBALANCED_DELIMITER.fix = make the expression type match the type contract named by the type checker
TRAIT_E_UNBALANCED_DELIMITER.example = let count: int = 1

TRAIT_E_INVALID_ATTRIBUTE.summary = attribute ungueltig.
TRAIT_E_INVALID_ATTRIBUTE.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_ATTRIBUTE.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_ATTRIBUTE.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_ATTRIBUTE.example = let count: int = 1

TRAIT_E_INVALID_DECLARATION.summary = Deklaration ungueltig.
TRAIT_E_INVALID_DECLARATION.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_DECLARATION.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_DECLARATION.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_DECLARATION.example = let count: int = 1

TRAIT_E_INVALID_STATEMENT.summary = Anweisung ungueltig.
TRAIT_E_INVALID_STATEMENT.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_STATEMENT.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_STATEMENT.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_STATEMENT.example = let count: int = 1

TRAIT_E_INVALID_EXPRESSION.summary = Ausdruck ungueltig.
TRAIT_E_INVALID_EXPRESSION.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_EXPRESSION.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_EXPRESSION.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_EXPRESSION.example = let count: int = 1

TRAIT_E_INVALID_PATTERN.summary = Muster ungueltig.
TRAIT_E_INVALID_PATTERN.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_PATTERN.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_PATTERN.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_PATTERN.example = let count: int = 1

TRAIT_E_INVALID_LITERAL.summary = Literal ungueltig.
TRAIT_E_INVALID_LITERAL.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_LITERAL.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_LITERAL.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_LITERAL.example = let count: int = 1

TRAIT_E_INVALID_OPERATOR.summary = Operator ungueltig.
TRAIT_E_INVALID_OPERATOR.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_OPERATOR.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_OPERATOR.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_OPERATOR.example = let count: int = 1

TRAIT_E_INVALID_MODIFIER.summary = modifier ungueltig.
TRAIT_E_INVALID_MODIFIER.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_MODIFIER.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_MODIFIER.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_MODIFIER.example = let count: int = 1

TRAIT_E_MISSING_BODY.summary = body fehlt.
TRAIT_E_MISSING_BODY.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_MISSING_BODY.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_MISSING_BODY.fix = make the expression type match the type contract named by the type checker
TRAIT_E_MISSING_BODY.example = let count: int = 1

TRAIT_E_MISSING_RETURN.summary = Rueckgabe fehlt.
TRAIT_E_MISSING_RETURN.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_MISSING_RETURN.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_MISSING_RETURN.fix = make the expression type match the type contract named by the type checker
TRAIT_E_MISSING_RETURN.example = let count: int = 1

TRAIT_E_DUPLICATE_NAME.summary = Name doppelt.
TRAIT_E_DUPLICATE_NAME.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_DUPLICATE_NAME.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_DUPLICATE_NAME.fix = make the expression type match the type contract named by the type checker
TRAIT_E_DUPLICATE_NAME.example = let count: int = 1

TRAIT_E_UNKNOWN_NAME.summary = Name unbekannt.
TRAIT_E_UNKNOWN_NAME.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_UNKNOWN_NAME.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_UNKNOWN_NAME.fix = make the expression type match the type contract named by the type checker
TRAIT_E_UNKNOWN_NAME.example = let count: int = 1

TRAIT_E_UNKNOWN_TYPE.summary = Typ unbekannt.
TRAIT_E_UNKNOWN_TYPE.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_UNKNOWN_TYPE.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_UNKNOWN_TYPE.fix = make the expression type match the type contract named by the type checker
TRAIT_E_UNKNOWN_TYPE.example = let count: int = 1

TRAIT_E_UNKNOWN_MODULE.summary = Modul unbekannt.
TRAIT_E_UNKNOWN_MODULE.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_UNKNOWN_MODULE.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_UNKNOWN_MODULE.fix = make the expression type match the type contract named by the type checker
TRAIT_E_UNKNOWN_MODULE.example = let count: int = 1

TRAIT_E_UNKNOWN_MEMBER.summary = Member unbekannt.
TRAIT_E_UNKNOWN_MEMBER.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_UNKNOWN_MEMBER.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_UNKNOWN_MEMBER.fix = make the expression type match the type contract named by the type checker
TRAIT_E_UNKNOWN_MEMBER.example = let count: int = 1

TRAIT_E_AMBIGUOUS_NAME.summary = ambiguous Name.
TRAIT_E_AMBIGUOUS_NAME.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_AMBIGUOUS_NAME.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_AMBIGUOUS_NAME.fix = make the expression type match the type contract named by the type checker
TRAIT_E_AMBIGUOUS_NAME.example = let count: int = 1

TRAIT_E_PRIVATE_SYMBOL.summary = private Symbol.
TRAIT_E_PRIVATE_SYMBOL.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_PRIVATE_SYMBOL.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_PRIVATE_SYMBOL.fix = make the expression type match the type contract named by the type checker
TRAIT_E_PRIVATE_SYMBOL.example = let count: int = 1

TRAIT_E_IMPORT_NOT_FOUND.summary = Import fehlend.
TRAIT_E_IMPORT_NOT_FOUND.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_IMPORT_NOT_FOUND.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_IMPORT_NOT_FOUND.fix = make the expression type match the type contract named by the type checker
TRAIT_E_IMPORT_NOT_FOUND.example = let count: int = 1

TRAIT_E_IMPORT_CYCLE.summary = Import Zyklus.
TRAIT_E_IMPORT_CYCLE.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_IMPORT_CYCLE.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_IMPORT_CYCLE.fix = make the expression type match the type contract named by the type checker
TRAIT_E_IMPORT_CYCLE.example = let count: int = 1

TRAIT_E_EXPORT_CONFLICT.summary = export Konflikt.
TRAIT_E_EXPORT_CONFLICT.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_EXPORT_CONFLICT.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_EXPORT_CONFLICT.fix = make the expression type match the type contract named by the type checker
TRAIT_E_EXPORT_CONFLICT.example = let count: int = 1

TRAIT_E_ARITY_MISMATCH.summary = arity Nichtuebereinstimmung.
TRAIT_E_ARITY_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_ARITY_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_ARITY_MISMATCH.fix = make the expression type match the type contract named by the type checker
TRAIT_E_ARITY_MISMATCH.example = let count: int = 1

TRAIT_E_ARGUMENT_MISMATCH.summary = Argument Nichtuebereinstimmung.
TRAIT_E_ARGUMENT_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_ARGUMENT_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_ARGUMENT_MISMATCH.fix = make the expression type match the type contract named by the type checker
TRAIT_E_ARGUMENT_MISMATCH.example = let count: int = 1

TRAIT_E_ASSIGNMENT_MISMATCH.summary = Zuweisung Nichtuebereinstimmung.
TRAIT_E_ASSIGNMENT_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_ASSIGNMENT_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_ASSIGNMENT_MISMATCH.fix = make the expression type match the type contract named by the type checker
TRAIT_E_ASSIGNMENT_MISMATCH.example = let count: int = 1

TRAIT_E_BRANCH_MISMATCH.summary = branch Nichtuebereinstimmung.
TRAIT_E_BRANCH_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_BRANCH_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_BRANCH_MISMATCH.fix = make the expression type match the type contract named by the type checker
TRAIT_E_BRANCH_MISMATCH.example = let count: int = 1

TRAIT_E_INVALID_CALL.summary = Aufruf ungueltig.
TRAIT_E_INVALID_CALL.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_CALL.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_CALL.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_CALL.example = let count: int = 1

TRAIT_E_INVALID_CAST.summary = Umwandlung ungueltig.
TRAIT_E_INVALID_CAST.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_CAST.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_CAST.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_CAST.example = let count: int = 1

TRAIT_E_INVALID_INDEX.summary = Index ungueltig.
TRAIT_E_INVALID_INDEX.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_INDEX.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_INDEX.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_INDEX.example = let count: int = 1

TRAIT_E_INVALID_DEREF.summary = deref ungueltig.
TRAIT_E_INVALID_DEREF.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_DEREF.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_DEREF.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_DEREF.example = let count: int = 1

TRAIT_E_INVALID_BORROW.summary = Ausleihe ungueltig.
TRAIT_E_INVALID_BORROW.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_BORROW.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_BORROW.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_BORROW.example = let count: int = 1

TRAIT_E_INVALID_MOVE.summary = Verschiebung ungueltig.
TRAIT_E_INVALID_MOVE.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_MOVE.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_MOVE.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_MOVE.example = let count: int = 1

TRAIT_E_USE_AFTER_MOVE.summary = use after Verschiebung.
TRAIT_E_USE_AFTER_MOVE.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_USE_AFTER_MOVE.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_USE_AFTER_MOVE.fix = make the expression type match the type contract named by the type checker
TRAIT_E_USE_AFTER_MOVE.example = let count: int = 1

TRAIT_E_USE_AFTER_DROP.summary = use after Freigabe.
TRAIT_E_USE_AFTER_DROP.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_USE_AFTER_DROP.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_USE_AFTER_DROP.fix = make the expression type match the type contract named by the type checker
TRAIT_E_USE_AFTER_DROP.example = let count: int = 1

TRAIT_E_USE_BEFORE_INIT.summary = use before init.
TRAIT_E_USE_BEFORE_INIT.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_USE_BEFORE_INIT.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_USE_BEFORE_INIT.fix = make the expression type match the type contract named by the type checker
TRAIT_E_USE_BEFORE_INIT.example = let count: int = 1

TRAIT_E_DOUBLE_DROP.summary = double Freigabe.
TRAIT_E_DOUBLE_DROP.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_DOUBLE_DROP.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_DOUBLE_DROP.fix = make the expression type match the type contract named by the type checker
TRAIT_E_DOUBLE_DROP.example = let count: int = 1

TRAIT_E_BORROW_CONFLICT.summary = Ausleihe Konflikt.
TRAIT_E_BORROW_CONFLICT.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_BORROW_CONFLICT.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_BORROW_CONFLICT.fix = make the expression type match the type contract named by the type checker
TRAIT_E_BORROW_CONFLICT.example = let count: int = 1

TRAIT_E_MUTABILITY_CONFLICT.summary = mutability Konflikt.
TRAIT_E_MUTABILITY_CONFLICT.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_MUTABILITY_CONFLICT.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_MUTABILITY_CONFLICT.fix = make the expression type match the type contract named by the type checker
TRAIT_E_MUTABILITY_CONFLICT.example = let count: int = 1

TRAIT_E_LIFETIME_TOO_SHORT.summary = Lebensdauer zu short.
TRAIT_E_LIFETIME_TOO_SHORT.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_LIFETIME_TOO_SHORT.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_LIFETIME_TOO_SHORT.fix = make the expression type match the type contract named by the type checker
TRAIT_E_LIFETIME_TOO_SHORT.example = let count: int = 1

TRAIT_E_DANGLING_REFERENCE.summary = dangling Referenz.
TRAIT_E_DANGLING_REFERENCE.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_DANGLING_REFERENCE.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_DANGLING_REFERENCE.fix = make the expression type match the type contract named by the type checker
TRAIT_E_DANGLING_REFERENCE.example = let count: int = 1

TRAIT_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
TRAIT_E_NON_EXHAUSTIVE_MATCH.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_NON_EXHAUSTIVE_MATCH.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_NON_EXHAUSTIVE_MATCH.fix = make the expression type match the type contract named by the type checker
TRAIT_E_NON_EXHAUSTIVE_MATCH.example = let count: int = 1

TRAIT_E_UNREACHABLE_PATTERN.summary = unerreichbar Muster.
TRAIT_E_UNREACHABLE_PATTERN.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_UNREACHABLE_PATTERN.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_UNREACHABLE_PATTERN.fix = make the expression type match the type contract named by the type checker
TRAIT_E_UNREACHABLE_PATTERN.example = let count: int = 1

TRAIT_E_CONST_REQUIRED.summary = const required.
TRAIT_E_CONST_REQUIRED.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_CONST_REQUIRED.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_CONST_REQUIRED.fix = make the expression type match the type contract named by the type checker
TRAIT_E_CONST_REQUIRED.example = let count: int = 1

TRAIT_E_CONST_OVERFLOW.summary = const Ueberlauf.
TRAIT_E_CONST_OVERFLOW.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_CONST_OVERFLOW.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_CONST_OVERFLOW.fix = make the expression type match the type contract named by the type checker
TRAIT_E_CONST_OVERFLOW.example = let count: int = 1

TRAIT_E_CONST_DIVISION_BY_ZERO.summary = const Division by Null.
TRAIT_E_CONST_DIVISION_BY_ZERO.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_CONST_DIVISION_BY_ZERO.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_CONST_DIVISION_BY_ZERO.fix = make the expression type match the type contract named by the type checker
TRAIT_E_CONST_DIVISION_BY_ZERO.example = let count: int = 1

TRAIT_E_CONST_CYCLE.summary = const Zyklus.
TRAIT_E_CONST_CYCLE.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_CONST_CYCLE.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_CONST_CYCLE.fix = make the expression type match the type contract named by the type checker
TRAIT_E_CONST_CYCLE.example = let count: int = 1

TRAIT_E_MACRO_NOT_FOUND.summary = macro fehlend.
TRAIT_E_MACRO_NOT_FOUND.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_MACRO_NOT_FOUND.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_MACRO_NOT_FOUND.fix = make the expression type match the type contract named by the type checker
TRAIT_E_MACRO_NOT_FOUND.example = let count: int = 1

TRAIT_E_MACRO_RECURSION.summary = macro recursion.
TRAIT_E_MACRO_RECURSION.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_MACRO_RECURSION.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_MACRO_RECURSION.fix = make the expression type match the type contract named by the type checker
TRAIT_E_MACRO_RECURSION.example = let count: int = 1

TRAIT_E_MACRO_EXPANSION_FAILED.summary = macro expansion fehlgeschlagen.
TRAIT_E_MACRO_EXPANSION_FAILED.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_MACRO_EXPANSION_FAILED.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_MACRO_EXPANSION_FAILED.fix = make the expression type match the type contract named by the type checker
TRAIT_E_MACRO_EXPANSION_FAILED.example = let count: int = 1

TRAIT_E_TRAIT_NOT_IMPLEMENTED.summary = trait not implemented.
TRAIT_E_TRAIT_NOT_IMPLEMENTED.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_TRAIT_NOT_IMPLEMENTED.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_TRAIT_NOT_IMPLEMENTED.fix = make the expression type match the type contract named by the type checker
TRAIT_E_TRAIT_NOT_IMPLEMENTED.example = let count: int = 1

TRAIT_E_TRAIT_AMBIGUOUS.summary = trait ambiguous.
TRAIT_E_TRAIT_AMBIGUOUS.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_TRAIT_AMBIGUOUS.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_TRAIT_AMBIGUOUS.fix = make the expression type match the type contract named by the type checker
TRAIT_E_TRAIT_AMBIGUOUS.example = let count: int = 1

TRAIT_E_GENERIC_ARGUMENT_MISSING.summary = generisch Argument fehlend.
TRAIT_E_GENERIC_ARGUMENT_MISSING.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_GENERIC_ARGUMENT_MISSING.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_GENERIC_ARGUMENT_MISSING.fix = make the expression type match the type contract named by the type checker
TRAIT_E_GENERIC_ARGUMENT_MISSING.example = let count: int = 1

TRAIT_E_GENERIC_BOUND_FAILED.summary = generisch bound fehlgeschlagen.
TRAIT_E_GENERIC_BOUND_FAILED.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_GENERIC_BOUND_FAILED.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_GENERIC_BOUND_FAILED.fix = make the expression type match the type contract named by the type checker
TRAIT_E_GENERIC_BOUND_FAILED.example = let count: int = 1

TRAIT_E_UNSUPPORTED_TARGET.summary = Ziel nicht unterstuetzt.
TRAIT_E_UNSUPPORTED_TARGET.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_UNSUPPORTED_TARGET.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_UNSUPPORTED_TARGET.fix = make the expression type match the type contract named by the type checker
TRAIT_E_UNSUPPORTED_TARGET.example = let count: int = 1

TRAIT_E_ABI_MISMATCH.summary = abi Nichtuebereinstimmung.
TRAIT_E_ABI_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_ABI_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_ABI_MISMATCH.fix = make the expression type match the type contract named by the type checker
TRAIT_E_ABI_MISMATCH.example = let count: int = 1

TRAIT_E_LINK_FAILED.summary = link fehlgeschlagen.
TRAIT_E_LINK_FAILED.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_LINK_FAILED.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_LINK_FAILED.fix = make the expression type match the type contract named by the type checker
TRAIT_E_LINK_FAILED.example = let count: int = 1

TRAIT_E_RUNTIME_PANIC.summary = runtime Panik.
TRAIT_E_RUNTIME_PANIC.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_RUNTIME_PANIC.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_RUNTIME_PANIC.fix = make the expression type match the type contract named by the type checker
TRAIT_E_RUNTIME_PANIC.example = let count: int = 1

OWNERSHIP_E_EXPECTED_IDENTIFIER.summary = Bezeichner erwartet.
OWNERSHIP_E_EXPECTED_IDENTIFIER.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_EXPECTED_IDENTIFIER.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_EXPECTED_IDENTIFIER.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_EXPECTED_IDENTIFIER.example = let view = &value

OWNERSHIP_E_EXPECTED_EXPRESSION.summary = Ausdruck erwartet.
OWNERSHIP_E_EXPECTED_EXPRESSION.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_EXPECTED_EXPRESSION.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_EXPECTED_EXPRESSION.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_EXPECTED_EXPRESSION.example = let view = &value

OWNERSHIP_E_EXPECTED_TYPE.summary = Typ erwartet.
OWNERSHIP_E_EXPECTED_TYPE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_EXPECTED_TYPE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_EXPECTED_TYPE.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_EXPECTED_TYPE.example = let view = &value

OWNERSHIP_E_EXPECTED_PATTERN.summary = Muster erwartet.
OWNERSHIP_E_EXPECTED_PATTERN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_EXPECTED_PATTERN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_EXPECTED_PATTERN.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_EXPECTED_PATTERN.example = let view = &value

OWNERSHIP_E_EXPECTED_BLOCK.summary = Block erwartet.
OWNERSHIP_E_EXPECTED_BLOCK.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_EXPECTED_BLOCK.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_EXPECTED_BLOCK.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_EXPECTED_BLOCK.example = let view = &value

OWNERSHIP_E_EXPECTED_DELIMITER.summary = Trennzeichen erwartet.
OWNERSHIP_E_EXPECTED_DELIMITER.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_EXPECTED_DELIMITER.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_EXPECTED_DELIMITER.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_EXPECTED_DELIMITER.example = let view = &value

OWNERSHIP_E_UNEXPECTED_TOKEN.summary = Token unerwartet.
OWNERSHIP_E_UNEXPECTED_TOKEN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_UNEXPECTED_TOKEN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_UNEXPECTED_TOKEN.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_UNEXPECTED_TOKEN.example = let view = &value

OWNERSHIP_E_UNBALANCED_DELIMITER.summary = unbalanced Trennzeichen.
OWNERSHIP_E_UNBALANCED_DELIMITER.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_UNBALANCED_DELIMITER.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_UNBALANCED_DELIMITER.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_UNBALANCED_DELIMITER.example = let view = &value

OWNERSHIP_E_INVALID_ATTRIBUTE.summary = attribute ungueltig.
OWNERSHIP_E_INVALID_ATTRIBUTE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_ATTRIBUTE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_ATTRIBUTE.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_ATTRIBUTE.example = let view = &value

OWNERSHIP_E_INVALID_DECLARATION.summary = Deklaration ungueltig.
OWNERSHIP_E_INVALID_DECLARATION.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_DECLARATION.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_DECLARATION.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_DECLARATION.example = let view = &value

OWNERSHIP_E_INVALID_STATEMENT.summary = Anweisung ungueltig.
OWNERSHIP_E_INVALID_STATEMENT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_STATEMENT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_STATEMENT.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_STATEMENT.example = let view = &value

OWNERSHIP_E_INVALID_EXPRESSION.summary = Ausdruck ungueltig.
OWNERSHIP_E_INVALID_EXPRESSION.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_EXPRESSION.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_EXPRESSION.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_EXPRESSION.example = let view = &value

OWNERSHIP_E_INVALID_PATTERN.summary = Muster ungueltig.
OWNERSHIP_E_INVALID_PATTERN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_PATTERN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_PATTERN.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_PATTERN.example = let view = &value

OWNERSHIP_E_INVALID_LITERAL.summary = Literal ungueltig.
OWNERSHIP_E_INVALID_LITERAL.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_LITERAL.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_LITERAL.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_LITERAL.example = let view = &value

OWNERSHIP_E_INVALID_OPERATOR.summary = Operator ungueltig.
OWNERSHIP_E_INVALID_OPERATOR.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_OPERATOR.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_OPERATOR.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_OPERATOR.example = let view = &value

OWNERSHIP_E_INVALID_MODIFIER.summary = modifier ungueltig.
OWNERSHIP_E_INVALID_MODIFIER.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_MODIFIER.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_MODIFIER.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_MODIFIER.example = let view = &value

OWNERSHIP_E_MISSING_BODY.summary = body fehlt.
OWNERSHIP_E_MISSING_BODY.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_MISSING_BODY.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_MISSING_BODY.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_MISSING_BODY.example = let view = &value

OWNERSHIP_E_MISSING_RETURN.summary = Rueckgabe fehlt.
OWNERSHIP_E_MISSING_RETURN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_MISSING_RETURN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_MISSING_RETURN.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_MISSING_RETURN.example = let view = &value

OWNERSHIP_E_DUPLICATE_NAME.summary = Name doppelt.
OWNERSHIP_E_DUPLICATE_NAME.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_DUPLICATE_NAME.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_DUPLICATE_NAME.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_DUPLICATE_NAME.example = let view = &value

OWNERSHIP_E_UNKNOWN_NAME.summary = Name unbekannt.
OWNERSHIP_E_UNKNOWN_NAME.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_UNKNOWN_NAME.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_UNKNOWN_NAME.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_UNKNOWN_NAME.example = let view = &value

OWNERSHIP_E_UNKNOWN_TYPE.summary = Typ unbekannt.
OWNERSHIP_E_UNKNOWN_TYPE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_UNKNOWN_TYPE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_UNKNOWN_TYPE.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_UNKNOWN_TYPE.example = let view = &value

OWNERSHIP_E_UNKNOWN_MODULE.summary = Modul unbekannt.
OWNERSHIP_E_UNKNOWN_MODULE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_UNKNOWN_MODULE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_UNKNOWN_MODULE.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_UNKNOWN_MODULE.example = let view = &value

OWNERSHIP_E_UNKNOWN_MEMBER.summary = Member unbekannt.
OWNERSHIP_E_UNKNOWN_MEMBER.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_UNKNOWN_MEMBER.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_UNKNOWN_MEMBER.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_UNKNOWN_MEMBER.example = let view = &value

OWNERSHIP_E_AMBIGUOUS_NAME.summary = ambiguous Name.
OWNERSHIP_E_AMBIGUOUS_NAME.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_AMBIGUOUS_NAME.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_AMBIGUOUS_NAME.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_AMBIGUOUS_NAME.example = let view = &value

OWNERSHIP_E_PRIVATE_SYMBOL.summary = private Symbol.
OWNERSHIP_E_PRIVATE_SYMBOL.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_PRIVATE_SYMBOL.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_PRIVATE_SYMBOL.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_PRIVATE_SYMBOL.example = let view = &value

OWNERSHIP_E_IMPORT_NOT_FOUND.summary = Import fehlend.
OWNERSHIP_E_IMPORT_NOT_FOUND.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_IMPORT_NOT_FOUND.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_IMPORT_NOT_FOUND.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_IMPORT_NOT_FOUND.example = let view = &value

OWNERSHIP_E_IMPORT_CYCLE.summary = Import Zyklus.
OWNERSHIP_E_IMPORT_CYCLE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_IMPORT_CYCLE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_IMPORT_CYCLE.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_IMPORT_CYCLE.example = let view = &value

OWNERSHIP_E_EXPORT_CONFLICT.summary = export Konflikt.
OWNERSHIP_E_EXPORT_CONFLICT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_EXPORT_CONFLICT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_EXPORT_CONFLICT.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_EXPORT_CONFLICT.example = let view = &value

OWNERSHIP_E_ARITY_MISMATCH.summary = arity Nichtuebereinstimmung.
OWNERSHIP_E_ARITY_MISMATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_ARITY_MISMATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_ARITY_MISMATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_ARITY_MISMATCH.example = let view = &value

OWNERSHIP_E_ARGUMENT_MISMATCH.summary = Argument Nichtuebereinstimmung.
OWNERSHIP_E_ARGUMENT_MISMATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_ARGUMENT_MISMATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_ARGUMENT_MISMATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_ARGUMENT_MISMATCH.example = let view = &value

OWNERSHIP_E_ASSIGNMENT_MISMATCH.summary = Zuweisung Nichtuebereinstimmung.
OWNERSHIP_E_ASSIGNMENT_MISMATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_ASSIGNMENT_MISMATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_ASSIGNMENT_MISMATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_ASSIGNMENT_MISMATCH.example = let view = &value

OWNERSHIP_E_BRANCH_MISMATCH.summary = branch Nichtuebereinstimmung.
OWNERSHIP_E_BRANCH_MISMATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_BRANCH_MISMATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_BRANCH_MISMATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_BRANCH_MISMATCH.example = let view = &value

OWNERSHIP_E_INVALID_CALL.summary = Aufruf ungueltig.
OWNERSHIP_E_INVALID_CALL.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_CALL.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_CALL.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_CALL.example = let view = &value

OWNERSHIP_E_INVALID_CAST.summary = Umwandlung ungueltig.
OWNERSHIP_E_INVALID_CAST.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_CAST.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_CAST.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_CAST.example = let view = &value

OWNERSHIP_E_INVALID_INDEX.summary = Index ungueltig.
OWNERSHIP_E_INVALID_INDEX.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_INDEX.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_INDEX.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_INDEX.example = let view = &value

OWNERSHIP_E_INVALID_DEREF.summary = deref ungueltig.
OWNERSHIP_E_INVALID_DEREF.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_DEREF.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_DEREF.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_DEREF.example = let view = &value

OWNERSHIP_E_INVALID_BORROW.summary = Ausleihe ungueltig.
OWNERSHIP_E_INVALID_BORROW.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_BORROW.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_BORROW.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_BORROW.example = let view = &value

OWNERSHIP_E_INVALID_MOVE.summary = Verschiebung ungueltig.
OWNERSHIP_E_INVALID_MOVE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_MOVE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_MOVE.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_MOVE.example = let view = &value

OWNERSHIP_E_USE_AFTER_MOVE.summary = use after Verschiebung.
OWNERSHIP_E_USE_AFTER_MOVE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_USE_AFTER_MOVE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_USE_AFTER_MOVE.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_USE_AFTER_MOVE.example = let view = &value

OWNERSHIP_E_USE_AFTER_DROP.summary = use after Freigabe.
OWNERSHIP_E_USE_AFTER_DROP.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_USE_AFTER_DROP.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_USE_AFTER_DROP.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_USE_AFTER_DROP.example = let view = &value

OWNERSHIP_E_USE_BEFORE_INIT.summary = use before init.
OWNERSHIP_E_USE_BEFORE_INIT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_USE_BEFORE_INIT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_USE_BEFORE_INIT.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_USE_BEFORE_INIT.example = let view = &value

OWNERSHIP_E_DOUBLE_DROP.summary = double Freigabe.
OWNERSHIP_E_DOUBLE_DROP.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_DOUBLE_DROP.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_DOUBLE_DROP.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_DOUBLE_DROP.example = let view = &value

OWNERSHIP_E_BORROW_CONFLICT.summary = Ausleihe Konflikt.
OWNERSHIP_E_BORROW_CONFLICT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_BORROW_CONFLICT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_BORROW_CONFLICT.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_BORROW_CONFLICT.example = let view = &value

OWNERSHIP_E_MUTABILITY_CONFLICT.summary = mutability Konflikt.
OWNERSHIP_E_MUTABILITY_CONFLICT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_MUTABILITY_CONFLICT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_MUTABILITY_CONFLICT.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_MUTABILITY_CONFLICT.example = let view = &value

OWNERSHIP_E_LIFETIME_TOO_SHORT.summary = Lebensdauer zu short.
OWNERSHIP_E_LIFETIME_TOO_SHORT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_LIFETIME_TOO_SHORT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_LIFETIME_TOO_SHORT.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_LIFETIME_TOO_SHORT.example = let view = &value

OWNERSHIP_E_DANGLING_REFERENCE.summary = dangling Referenz.
OWNERSHIP_E_DANGLING_REFERENCE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_DANGLING_REFERENCE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_DANGLING_REFERENCE.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_DANGLING_REFERENCE.example = let view = &value

OWNERSHIP_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
OWNERSHIP_E_NON_EXHAUSTIVE_MATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_NON_EXHAUSTIVE_MATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_NON_EXHAUSTIVE_MATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_NON_EXHAUSTIVE_MATCH.example = let view = &value

OWNERSHIP_E_UNREACHABLE_PATTERN.summary = unerreichbar Muster.
OWNERSHIP_E_UNREACHABLE_PATTERN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_UNREACHABLE_PATTERN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_UNREACHABLE_PATTERN.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_UNREACHABLE_PATTERN.example = let view = &value

OWNERSHIP_E_CONST_REQUIRED.summary = const required.
OWNERSHIP_E_CONST_REQUIRED.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_CONST_REQUIRED.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_CONST_REQUIRED.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_CONST_REQUIRED.example = let view = &value

OWNERSHIP_E_CONST_OVERFLOW.summary = const Ueberlauf.
OWNERSHIP_E_CONST_OVERFLOW.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_CONST_OVERFLOW.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_CONST_OVERFLOW.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_CONST_OVERFLOW.example = let view = &value

OWNERSHIP_E_CONST_DIVISION_BY_ZERO.summary = const Division by Null.
OWNERSHIP_E_CONST_DIVISION_BY_ZERO.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_CONST_DIVISION_BY_ZERO.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_CONST_DIVISION_BY_ZERO.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_CONST_DIVISION_BY_ZERO.example = let view = &value

OWNERSHIP_E_CONST_CYCLE.summary = const Zyklus.
OWNERSHIP_E_CONST_CYCLE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_CONST_CYCLE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_CONST_CYCLE.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_CONST_CYCLE.example = let view = &value

OWNERSHIP_E_MACRO_NOT_FOUND.summary = macro fehlend.
OWNERSHIP_E_MACRO_NOT_FOUND.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_MACRO_NOT_FOUND.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_MACRO_NOT_FOUND.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_MACRO_NOT_FOUND.example = let view = &value

OWNERSHIP_E_MACRO_RECURSION.summary = macro recursion.
OWNERSHIP_E_MACRO_RECURSION.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_MACRO_RECURSION.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_MACRO_RECURSION.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_MACRO_RECURSION.example = let view = &value

OWNERSHIP_E_MACRO_EXPANSION_FAILED.summary = macro expansion fehlgeschlagen.
OWNERSHIP_E_MACRO_EXPANSION_FAILED.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_MACRO_EXPANSION_FAILED.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_MACRO_EXPANSION_FAILED.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_MACRO_EXPANSION_FAILED.example = let view = &value

OWNERSHIP_E_TRAIT_NOT_IMPLEMENTED.summary = trait not implemented.
OWNERSHIP_E_TRAIT_NOT_IMPLEMENTED.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_TRAIT_NOT_IMPLEMENTED.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_TRAIT_NOT_IMPLEMENTED.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_TRAIT_NOT_IMPLEMENTED.example = let view = &value

OWNERSHIP_E_TRAIT_AMBIGUOUS.summary = trait ambiguous.
OWNERSHIP_E_TRAIT_AMBIGUOUS.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_TRAIT_AMBIGUOUS.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_TRAIT_AMBIGUOUS.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_TRAIT_AMBIGUOUS.example = let view = &value

OWNERSHIP_E_GENERIC_ARGUMENT_MISSING.summary = generisch Argument fehlend.
OWNERSHIP_E_GENERIC_ARGUMENT_MISSING.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_GENERIC_ARGUMENT_MISSING.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_GENERIC_ARGUMENT_MISSING.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_GENERIC_ARGUMENT_MISSING.example = let view = &value

OWNERSHIP_E_GENERIC_BOUND_FAILED.summary = generisch bound fehlgeschlagen.
OWNERSHIP_E_GENERIC_BOUND_FAILED.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_GENERIC_BOUND_FAILED.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_GENERIC_BOUND_FAILED.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_GENERIC_BOUND_FAILED.example = let view = &value

OWNERSHIP_E_UNSUPPORTED_TARGET.summary = Ziel nicht unterstuetzt.
OWNERSHIP_E_UNSUPPORTED_TARGET.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_UNSUPPORTED_TARGET.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_UNSUPPORTED_TARGET.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_UNSUPPORTED_TARGET.example = let view = &value

OWNERSHIP_E_ABI_MISMATCH.summary = abi Nichtuebereinstimmung.
OWNERSHIP_E_ABI_MISMATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_ABI_MISMATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_ABI_MISMATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_ABI_MISMATCH.example = let view = &value

OWNERSHIP_E_LINK_FAILED.summary = link fehlgeschlagen.
OWNERSHIP_E_LINK_FAILED.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_LINK_FAILED.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_LINK_FAILED.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_LINK_FAILED.example = let view = &value

OWNERSHIP_E_RUNTIME_PANIC.summary = runtime Panik.
OWNERSHIP_E_RUNTIME_PANIC.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_RUNTIME_PANIC.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_RUNTIME_PANIC.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_RUNTIME_PANIC.example = let view = &value

LIFETIME_E_EXPECTED_IDENTIFIER.summary = Bezeichner erwartet.
LIFETIME_E_EXPECTED_IDENTIFIER.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_EXPECTED_IDENTIFIER.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_EXPECTED_IDENTIFIER.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_EXPECTED_IDENTIFIER.example = let view = &value

LIFETIME_E_EXPECTED_EXPRESSION.summary = Ausdruck erwartet.
LIFETIME_E_EXPECTED_EXPRESSION.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_EXPECTED_EXPRESSION.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_EXPECTED_EXPRESSION.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_EXPECTED_EXPRESSION.example = let view = &value

LIFETIME_E_EXPECTED_TYPE.summary = Typ erwartet.
LIFETIME_E_EXPECTED_TYPE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_EXPECTED_TYPE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_EXPECTED_TYPE.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_EXPECTED_TYPE.example = let view = &value

LIFETIME_E_EXPECTED_PATTERN.summary = Muster erwartet.
LIFETIME_E_EXPECTED_PATTERN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_EXPECTED_PATTERN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_EXPECTED_PATTERN.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_EXPECTED_PATTERN.example = let view = &value

LIFETIME_E_EXPECTED_BLOCK.summary = Block erwartet.
LIFETIME_E_EXPECTED_BLOCK.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_EXPECTED_BLOCK.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_EXPECTED_BLOCK.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_EXPECTED_BLOCK.example = let view = &value

LIFETIME_E_EXPECTED_DELIMITER.summary = Trennzeichen erwartet.
LIFETIME_E_EXPECTED_DELIMITER.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_EXPECTED_DELIMITER.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_EXPECTED_DELIMITER.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_EXPECTED_DELIMITER.example = let view = &value

LIFETIME_E_UNEXPECTED_TOKEN.summary = Token unerwartet.
LIFETIME_E_UNEXPECTED_TOKEN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_UNEXPECTED_TOKEN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_UNEXPECTED_TOKEN.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_UNEXPECTED_TOKEN.example = let view = &value

LIFETIME_E_UNBALANCED_DELIMITER.summary = unbalanced Trennzeichen.
LIFETIME_E_UNBALANCED_DELIMITER.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_UNBALANCED_DELIMITER.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_UNBALANCED_DELIMITER.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_UNBALANCED_DELIMITER.example = let view = &value

LIFETIME_E_INVALID_ATTRIBUTE.summary = attribute ungueltig.
LIFETIME_E_INVALID_ATTRIBUTE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_ATTRIBUTE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_ATTRIBUTE.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_ATTRIBUTE.example = let view = &value

LIFETIME_E_INVALID_DECLARATION.summary = Deklaration ungueltig.
LIFETIME_E_INVALID_DECLARATION.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_DECLARATION.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_DECLARATION.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_DECLARATION.example = let view = &value

LIFETIME_E_INVALID_STATEMENT.summary = Anweisung ungueltig.
LIFETIME_E_INVALID_STATEMENT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_STATEMENT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_STATEMENT.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_STATEMENT.example = let view = &value

LIFETIME_E_INVALID_EXPRESSION.summary = Ausdruck ungueltig.
LIFETIME_E_INVALID_EXPRESSION.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_EXPRESSION.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_EXPRESSION.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_EXPRESSION.example = let view = &value

LIFETIME_E_INVALID_PATTERN.summary = Muster ungueltig.
LIFETIME_E_INVALID_PATTERN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_PATTERN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_PATTERN.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_PATTERN.example = let view = &value

LIFETIME_E_INVALID_LITERAL.summary = Literal ungueltig.
LIFETIME_E_INVALID_LITERAL.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_LITERAL.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_LITERAL.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_LITERAL.example = let view = &value

LIFETIME_E_INVALID_OPERATOR.summary = Operator ungueltig.
LIFETIME_E_INVALID_OPERATOR.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_OPERATOR.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_OPERATOR.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_OPERATOR.example = let view = &value

LIFETIME_E_INVALID_MODIFIER.summary = modifier ungueltig.
LIFETIME_E_INVALID_MODIFIER.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_MODIFIER.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_MODIFIER.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_MODIFIER.example = let view = &value

LIFETIME_E_MISSING_BODY.summary = body fehlt.
LIFETIME_E_MISSING_BODY.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_MISSING_BODY.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_MISSING_BODY.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_MISSING_BODY.example = let view = &value

LIFETIME_E_MISSING_RETURN.summary = Rueckgabe fehlt.
LIFETIME_E_MISSING_RETURN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_MISSING_RETURN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_MISSING_RETURN.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_MISSING_RETURN.example = let view = &value

LIFETIME_E_DUPLICATE_NAME.summary = Name doppelt.
LIFETIME_E_DUPLICATE_NAME.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_DUPLICATE_NAME.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_DUPLICATE_NAME.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_DUPLICATE_NAME.example = let view = &value

LIFETIME_E_UNKNOWN_NAME.summary = Name unbekannt.
LIFETIME_E_UNKNOWN_NAME.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_UNKNOWN_NAME.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_UNKNOWN_NAME.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_UNKNOWN_NAME.example = let view = &value

LIFETIME_E_UNKNOWN_TYPE.summary = Typ unbekannt.
LIFETIME_E_UNKNOWN_TYPE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_UNKNOWN_TYPE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_UNKNOWN_TYPE.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_UNKNOWN_TYPE.example = let view = &value

LIFETIME_E_UNKNOWN_MODULE.summary = Modul unbekannt.
LIFETIME_E_UNKNOWN_MODULE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_UNKNOWN_MODULE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_UNKNOWN_MODULE.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_UNKNOWN_MODULE.example = let view = &value

LIFETIME_E_UNKNOWN_MEMBER.summary = Member unbekannt.
LIFETIME_E_UNKNOWN_MEMBER.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_UNKNOWN_MEMBER.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_UNKNOWN_MEMBER.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_UNKNOWN_MEMBER.example = let view = &value

LIFETIME_E_AMBIGUOUS_NAME.summary = ambiguous Name.
LIFETIME_E_AMBIGUOUS_NAME.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_AMBIGUOUS_NAME.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_AMBIGUOUS_NAME.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_AMBIGUOUS_NAME.example = let view = &value

LIFETIME_E_PRIVATE_SYMBOL.summary = private Symbol.
LIFETIME_E_PRIVATE_SYMBOL.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_PRIVATE_SYMBOL.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_PRIVATE_SYMBOL.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_PRIVATE_SYMBOL.example = let view = &value

LIFETIME_E_IMPORT_NOT_FOUND.summary = Import fehlend.
LIFETIME_E_IMPORT_NOT_FOUND.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_IMPORT_NOT_FOUND.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_IMPORT_NOT_FOUND.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_IMPORT_NOT_FOUND.example = let view = &value

LIFETIME_E_IMPORT_CYCLE.summary = Import Zyklus.
LIFETIME_E_IMPORT_CYCLE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_IMPORT_CYCLE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_IMPORT_CYCLE.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_IMPORT_CYCLE.example = let view = &value

LIFETIME_E_EXPORT_CONFLICT.summary = export Konflikt.
LIFETIME_E_EXPORT_CONFLICT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_EXPORT_CONFLICT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_EXPORT_CONFLICT.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_EXPORT_CONFLICT.example = let view = &value

LIFETIME_E_ARITY_MISMATCH.summary = arity Nichtuebereinstimmung.
LIFETIME_E_ARITY_MISMATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_ARITY_MISMATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_ARITY_MISMATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_ARITY_MISMATCH.example = let view = &value

LIFETIME_E_ARGUMENT_MISMATCH.summary = Argument Nichtuebereinstimmung.
LIFETIME_E_ARGUMENT_MISMATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_ARGUMENT_MISMATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_ARGUMENT_MISMATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_ARGUMENT_MISMATCH.example = let view = &value

LIFETIME_E_ASSIGNMENT_MISMATCH.summary = Zuweisung Nichtuebereinstimmung.
LIFETIME_E_ASSIGNMENT_MISMATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_ASSIGNMENT_MISMATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_ASSIGNMENT_MISMATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_ASSIGNMENT_MISMATCH.example = let view = &value

LIFETIME_E_BRANCH_MISMATCH.summary = branch Nichtuebereinstimmung.
LIFETIME_E_BRANCH_MISMATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_BRANCH_MISMATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_BRANCH_MISMATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_BRANCH_MISMATCH.example = let view = &value

LIFETIME_E_INVALID_CALL.summary = Aufruf ungueltig.
LIFETIME_E_INVALID_CALL.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_CALL.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_CALL.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_CALL.example = let view = &value

LIFETIME_E_INVALID_CAST.summary = Umwandlung ungueltig.
LIFETIME_E_INVALID_CAST.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_CAST.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_CAST.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_CAST.example = let view = &value

LIFETIME_E_INVALID_INDEX.summary = Index ungueltig.
LIFETIME_E_INVALID_INDEX.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_INDEX.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_INDEX.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_INDEX.example = let view = &value

LIFETIME_E_INVALID_DEREF.summary = deref ungueltig.
LIFETIME_E_INVALID_DEREF.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_DEREF.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_DEREF.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_DEREF.example = let view = &value

LIFETIME_E_INVALID_BORROW.summary = Ausleihe ungueltig.
LIFETIME_E_INVALID_BORROW.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_BORROW.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_BORROW.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_BORROW.example = let view = &value

LIFETIME_E_INVALID_MOVE.summary = Verschiebung ungueltig.
LIFETIME_E_INVALID_MOVE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_MOVE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_MOVE.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_MOVE.example = let view = &value

LIFETIME_E_USE_AFTER_MOVE.summary = use after Verschiebung.
LIFETIME_E_USE_AFTER_MOVE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_USE_AFTER_MOVE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_USE_AFTER_MOVE.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_USE_AFTER_MOVE.example = let view = &value

LIFETIME_E_USE_AFTER_DROP.summary = use after Freigabe.
LIFETIME_E_USE_AFTER_DROP.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_USE_AFTER_DROP.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_USE_AFTER_DROP.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_USE_AFTER_DROP.example = let view = &value

LIFETIME_E_USE_BEFORE_INIT.summary = use before init.
LIFETIME_E_USE_BEFORE_INIT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_USE_BEFORE_INIT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_USE_BEFORE_INIT.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_USE_BEFORE_INIT.example = let view = &value

LIFETIME_E_DOUBLE_DROP.summary = double Freigabe.
LIFETIME_E_DOUBLE_DROP.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_DOUBLE_DROP.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_DOUBLE_DROP.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_DOUBLE_DROP.example = let view = &value

LIFETIME_E_BORROW_CONFLICT.summary = Ausleihe Konflikt.
LIFETIME_E_BORROW_CONFLICT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_BORROW_CONFLICT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_BORROW_CONFLICT.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_BORROW_CONFLICT.example = let view = &value

LIFETIME_E_MUTABILITY_CONFLICT.summary = mutability Konflikt.
LIFETIME_E_MUTABILITY_CONFLICT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_MUTABILITY_CONFLICT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_MUTABILITY_CONFLICT.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_MUTABILITY_CONFLICT.example = let view = &value

LIFETIME_E_LIFETIME_TOO_SHORT.summary = Lebensdauer zu short.
LIFETIME_E_LIFETIME_TOO_SHORT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_LIFETIME_TOO_SHORT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_LIFETIME_TOO_SHORT.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_LIFETIME_TOO_SHORT.example = let view = &value

LIFETIME_E_DANGLING_REFERENCE.summary = dangling Referenz.
LIFETIME_E_DANGLING_REFERENCE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_DANGLING_REFERENCE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_DANGLING_REFERENCE.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_DANGLING_REFERENCE.example = let view = &value

LIFETIME_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
LIFETIME_E_NON_EXHAUSTIVE_MATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_NON_EXHAUSTIVE_MATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_NON_EXHAUSTIVE_MATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_NON_EXHAUSTIVE_MATCH.example = let view = &value

LIFETIME_E_UNREACHABLE_PATTERN.summary = unerreichbar Muster.
LIFETIME_E_UNREACHABLE_PATTERN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_UNREACHABLE_PATTERN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_UNREACHABLE_PATTERN.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_UNREACHABLE_PATTERN.example = let view = &value

LIFETIME_E_CONST_REQUIRED.summary = const required.
LIFETIME_E_CONST_REQUIRED.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_CONST_REQUIRED.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_CONST_REQUIRED.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_CONST_REQUIRED.example = let view = &value

LIFETIME_E_CONST_OVERFLOW.summary = const Ueberlauf.
LIFETIME_E_CONST_OVERFLOW.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_CONST_OVERFLOW.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_CONST_OVERFLOW.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_CONST_OVERFLOW.example = let view = &value

LIFETIME_E_CONST_DIVISION_BY_ZERO.summary = const Division by Null.
LIFETIME_E_CONST_DIVISION_BY_ZERO.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_CONST_DIVISION_BY_ZERO.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_CONST_DIVISION_BY_ZERO.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_CONST_DIVISION_BY_ZERO.example = let view = &value

LIFETIME_E_CONST_CYCLE.summary = const Zyklus.
LIFETIME_E_CONST_CYCLE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_CONST_CYCLE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_CONST_CYCLE.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_CONST_CYCLE.example = let view = &value

LIFETIME_E_MACRO_NOT_FOUND.summary = macro fehlend.
LIFETIME_E_MACRO_NOT_FOUND.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_MACRO_NOT_FOUND.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_MACRO_NOT_FOUND.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_MACRO_NOT_FOUND.example = let view = &value

LIFETIME_E_MACRO_RECURSION.summary = macro recursion.
LIFETIME_E_MACRO_RECURSION.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_MACRO_RECURSION.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_MACRO_RECURSION.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_MACRO_RECURSION.example = let view = &value

LIFETIME_E_MACRO_EXPANSION_FAILED.summary = macro expansion fehlgeschlagen.
LIFETIME_E_MACRO_EXPANSION_FAILED.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_MACRO_EXPANSION_FAILED.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_MACRO_EXPANSION_FAILED.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_MACRO_EXPANSION_FAILED.example = let view = &value

LIFETIME_E_TRAIT_NOT_IMPLEMENTED.summary = trait not implemented.
LIFETIME_E_TRAIT_NOT_IMPLEMENTED.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_TRAIT_NOT_IMPLEMENTED.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_TRAIT_NOT_IMPLEMENTED.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_TRAIT_NOT_IMPLEMENTED.example = let view = &value

LIFETIME_E_TRAIT_AMBIGUOUS.summary = trait ambiguous.
LIFETIME_E_TRAIT_AMBIGUOUS.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_TRAIT_AMBIGUOUS.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_TRAIT_AMBIGUOUS.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_TRAIT_AMBIGUOUS.example = let view = &value

LIFETIME_E_GENERIC_ARGUMENT_MISSING.summary = generisch Argument fehlend.
LIFETIME_E_GENERIC_ARGUMENT_MISSING.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_GENERIC_ARGUMENT_MISSING.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_GENERIC_ARGUMENT_MISSING.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_GENERIC_ARGUMENT_MISSING.example = let view = &value

LIFETIME_E_GENERIC_BOUND_FAILED.summary = generisch bound fehlgeschlagen.
LIFETIME_E_GENERIC_BOUND_FAILED.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_GENERIC_BOUND_FAILED.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_GENERIC_BOUND_FAILED.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_GENERIC_BOUND_FAILED.example = let view = &value

LIFETIME_E_UNSUPPORTED_TARGET.summary = Ziel nicht unterstuetzt.
LIFETIME_E_UNSUPPORTED_TARGET.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_UNSUPPORTED_TARGET.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_UNSUPPORTED_TARGET.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_UNSUPPORTED_TARGET.example = let view = &value

LIFETIME_E_ABI_MISMATCH.summary = abi Nichtuebereinstimmung.
LIFETIME_E_ABI_MISMATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_ABI_MISMATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_ABI_MISMATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_ABI_MISMATCH.example = let view = &value

LIFETIME_E_LINK_FAILED.summary = link fehlgeschlagen.
LIFETIME_E_LINK_FAILED.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_LINK_FAILED.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_LINK_FAILED.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_LINK_FAILED.example = let view = &value

LIFETIME_E_RUNTIME_PANIC.summary = runtime Panik.
LIFETIME_E_RUNTIME_PANIC.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_RUNTIME_PANIC.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_RUNTIME_PANIC.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_RUNTIME_PANIC.example = let view = &value

CONST_E_EXPECTED_IDENTIFIER.summary = Bezeichner erwartet.
CONST_E_EXPECTED_IDENTIFIER.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_EXPECTED_IDENTIFIER.step1 = Reduce the constant expression at the reported span.
CONST_E_EXPECTED_IDENTIFIER.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_EXPECTED_IDENTIFIER.example = const size: int = 4

CONST_E_EXPECTED_EXPRESSION.summary = Ausdruck erwartet.
CONST_E_EXPECTED_EXPRESSION.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_EXPECTED_EXPRESSION.step1 = Reduce the constant expression at the reported span.
CONST_E_EXPECTED_EXPRESSION.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_EXPECTED_EXPRESSION.example = const size: int = 4

CONST_E_EXPECTED_TYPE.summary = Typ erwartet.
CONST_E_EXPECTED_TYPE.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_EXPECTED_TYPE.step1 = Reduce the constant expression at the reported span.
CONST_E_EXPECTED_TYPE.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_EXPECTED_TYPE.example = const size: int = 4

CONST_E_EXPECTED_PATTERN.summary = Muster erwartet.
CONST_E_EXPECTED_PATTERN.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_EXPECTED_PATTERN.step1 = Reduce the constant expression at the reported span.
CONST_E_EXPECTED_PATTERN.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_EXPECTED_PATTERN.example = const size: int = 4

CONST_E_EXPECTED_BLOCK.summary = Block erwartet.
CONST_E_EXPECTED_BLOCK.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_EXPECTED_BLOCK.step1 = Reduce the constant expression at the reported span.
CONST_E_EXPECTED_BLOCK.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_EXPECTED_BLOCK.example = const size: int = 4

CONST_E_EXPECTED_DELIMITER.summary = Trennzeichen erwartet.
CONST_E_EXPECTED_DELIMITER.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_EXPECTED_DELIMITER.step1 = Reduce the constant expression at the reported span.
CONST_E_EXPECTED_DELIMITER.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_EXPECTED_DELIMITER.example = const size: int = 4

CONST_E_UNEXPECTED_TOKEN.summary = Token unerwartet.
CONST_E_UNEXPECTED_TOKEN.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_UNEXPECTED_TOKEN.step1 = Reduce the constant expression at the reported span.
CONST_E_UNEXPECTED_TOKEN.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_UNEXPECTED_TOKEN.example = const size: int = 4

CONST_E_UNBALANCED_DELIMITER.summary = unbalanced Trennzeichen.
CONST_E_UNBALANCED_DELIMITER.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_UNBALANCED_DELIMITER.step1 = Reduce the constant expression at the reported span.
CONST_E_UNBALANCED_DELIMITER.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_UNBALANCED_DELIMITER.example = const size: int = 4

CONST_E_INVALID_ATTRIBUTE.summary = attribute ungueltig.
CONST_E_INVALID_ATTRIBUTE.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_ATTRIBUTE.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_ATTRIBUTE.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_ATTRIBUTE.example = const size: int = 4

CONST_E_INVALID_DECLARATION.summary = Deklaration ungueltig.
CONST_E_INVALID_DECLARATION.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_DECLARATION.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_DECLARATION.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_DECLARATION.example = const size: int = 4

CONST_E_INVALID_STATEMENT.summary = Anweisung ungueltig.
CONST_E_INVALID_STATEMENT.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_STATEMENT.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_STATEMENT.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_STATEMENT.example = const size: int = 4

CONST_E_INVALID_EXPRESSION.summary = Ausdruck ungueltig.
CONST_E_INVALID_EXPRESSION.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_EXPRESSION.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_EXPRESSION.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_EXPRESSION.example = const size: int = 4

CONST_E_INVALID_PATTERN.summary = Muster ungueltig.
CONST_E_INVALID_PATTERN.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_PATTERN.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_PATTERN.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_PATTERN.example = const size: int = 4

CONST_E_INVALID_LITERAL.summary = Literal ungueltig.
CONST_E_INVALID_LITERAL.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_LITERAL.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_LITERAL.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_LITERAL.example = const size: int = 4

CONST_E_INVALID_OPERATOR.summary = Operator ungueltig.
CONST_E_INVALID_OPERATOR.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_OPERATOR.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_OPERATOR.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_OPERATOR.example = const size: int = 4

CONST_E_INVALID_MODIFIER.summary = modifier ungueltig.
CONST_E_INVALID_MODIFIER.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_MODIFIER.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_MODIFIER.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_MODIFIER.example = const size: int = 4

CONST_E_MISSING_BODY.summary = body fehlt.
CONST_E_MISSING_BODY.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_MISSING_BODY.step1 = Reduce the constant expression at the reported span.
CONST_E_MISSING_BODY.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_MISSING_BODY.example = const size: int = 4

CONST_E_MISSING_RETURN.summary = Rueckgabe fehlt.
CONST_E_MISSING_RETURN.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_MISSING_RETURN.step1 = Reduce the constant expression at the reported span.
CONST_E_MISSING_RETURN.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_MISSING_RETURN.example = const size: int = 4

CONST_E_DUPLICATE_NAME.summary = Name doppelt.
CONST_E_DUPLICATE_NAME.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_DUPLICATE_NAME.step1 = Reduce the constant expression at the reported span.
CONST_E_DUPLICATE_NAME.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_DUPLICATE_NAME.example = const size: int = 4

CONST_E_UNKNOWN_NAME.summary = Name unbekannt.
CONST_E_UNKNOWN_NAME.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_UNKNOWN_NAME.step1 = Reduce the constant expression at the reported span.
CONST_E_UNKNOWN_NAME.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_UNKNOWN_NAME.example = const size: int = 4

CONST_E_UNKNOWN_TYPE.summary = Typ unbekannt.
CONST_E_UNKNOWN_TYPE.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_UNKNOWN_TYPE.step1 = Reduce the constant expression at the reported span.
CONST_E_UNKNOWN_TYPE.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_UNKNOWN_TYPE.example = const size: int = 4

CONST_E_UNKNOWN_MODULE.summary = Modul unbekannt.
CONST_E_UNKNOWN_MODULE.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_UNKNOWN_MODULE.step1 = Reduce the constant expression at the reported span.
CONST_E_UNKNOWN_MODULE.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_UNKNOWN_MODULE.example = const size: int = 4

CONST_E_UNKNOWN_MEMBER.summary = Member unbekannt.
CONST_E_UNKNOWN_MEMBER.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_UNKNOWN_MEMBER.step1 = Reduce the constant expression at the reported span.
CONST_E_UNKNOWN_MEMBER.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_UNKNOWN_MEMBER.example = const size: int = 4

CONST_E_AMBIGUOUS_NAME.summary = ambiguous Name.
CONST_E_AMBIGUOUS_NAME.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_AMBIGUOUS_NAME.step1 = Reduce the constant expression at the reported span.
CONST_E_AMBIGUOUS_NAME.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_AMBIGUOUS_NAME.example = const size: int = 4

CONST_E_PRIVATE_SYMBOL.summary = private Symbol.
CONST_E_PRIVATE_SYMBOL.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_PRIVATE_SYMBOL.step1 = Reduce the constant expression at the reported span.
CONST_E_PRIVATE_SYMBOL.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_PRIVATE_SYMBOL.example = const size: int = 4

CONST_E_IMPORT_NOT_FOUND.summary = Import fehlend.
CONST_E_IMPORT_NOT_FOUND.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_IMPORT_NOT_FOUND.step1 = Reduce the constant expression at the reported span.
CONST_E_IMPORT_NOT_FOUND.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_IMPORT_NOT_FOUND.example = const size: int = 4

CONST_E_IMPORT_CYCLE.summary = Import Zyklus.
CONST_E_IMPORT_CYCLE.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_IMPORT_CYCLE.step1 = Reduce the constant expression at the reported span.
CONST_E_IMPORT_CYCLE.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_IMPORT_CYCLE.example = const size: int = 4

CONST_E_EXPORT_CONFLICT.summary = export Konflikt.
CONST_E_EXPORT_CONFLICT.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_EXPORT_CONFLICT.step1 = Reduce the constant expression at the reported span.
CONST_E_EXPORT_CONFLICT.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_EXPORT_CONFLICT.example = const size: int = 4

CONST_E_ARITY_MISMATCH.summary = arity Nichtuebereinstimmung.
CONST_E_ARITY_MISMATCH.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_ARITY_MISMATCH.step1 = Reduce the constant expression at the reported span.
CONST_E_ARITY_MISMATCH.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_ARITY_MISMATCH.example = const size: int = 4

CONST_E_ARGUMENT_MISMATCH.summary = Argument Nichtuebereinstimmung.
CONST_E_ARGUMENT_MISMATCH.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_ARGUMENT_MISMATCH.step1 = Reduce the constant expression at the reported span.
CONST_E_ARGUMENT_MISMATCH.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_ARGUMENT_MISMATCH.example = const size: int = 4

CONST_E_ASSIGNMENT_MISMATCH.summary = Zuweisung Nichtuebereinstimmung.
CONST_E_ASSIGNMENT_MISMATCH.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_ASSIGNMENT_MISMATCH.step1 = Reduce the constant expression at the reported span.
CONST_E_ASSIGNMENT_MISMATCH.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_ASSIGNMENT_MISMATCH.example = const size: int = 4

CONST_E_BRANCH_MISMATCH.summary = branch Nichtuebereinstimmung.
CONST_E_BRANCH_MISMATCH.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_BRANCH_MISMATCH.step1 = Reduce the constant expression at the reported span.
CONST_E_BRANCH_MISMATCH.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_BRANCH_MISMATCH.example = const size: int = 4

CONST_E_INVALID_CALL.summary = Aufruf ungueltig.
CONST_E_INVALID_CALL.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_CALL.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_CALL.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_CALL.example = const size: int = 4

CONST_E_INVALID_CAST.summary = Umwandlung ungueltig.
CONST_E_INVALID_CAST.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_CAST.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_CAST.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_CAST.example = const size: int = 4

CONST_E_INVALID_INDEX.summary = Index ungueltig.
CONST_E_INVALID_INDEX.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_INDEX.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_INDEX.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_INDEX.example = const size: int = 4

CONST_E_INVALID_DEREF.summary = deref ungueltig.
CONST_E_INVALID_DEREF.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_DEREF.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_DEREF.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_DEREF.example = const size: int = 4

CONST_E_INVALID_BORROW.summary = Ausleihe ungueltig.
CONST_E_INVALID_BORROW.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_BORROW.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_BORROW.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_BORROW.example = const size: int = 4

CONST_E_INVALID_MOVE.summary = Verschiebung ungueltig.
CONST_E_INVALID_MOVE.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_MOVE.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_MOVE.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_MOVE.example = const size: int = 4

CONST_E_USE_AFTER_MOVE.summary = use after Verschiebung.
CONST_E_USE_AFTER_MOVE.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_USE_AFTER_MOVE.step1 = Reduce the constant expression at the reported span.
CONST_E_USE_AFTER_MOVE.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_USE_AFTER_MOVE.example = const size: int = 4

CONST_E_USE_AFTER_DROP.summary = use after Freigabe.
CONST_E_USE_AFTER_DROP.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_USE_AFTER_DROP.step1 = Reduce the constant expression at the reported span.
CONST_E_USE_AFTER_DROP.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_USE_AFTER_DROP.example = const size: int = 4

CONST_E_USE_BEFORE_INIT.summary = use before init.
CONST_E_USE_BEFORE_INIT.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_USE_BEFORE_INIT.step1 = Reduce the constant expression at the reported span.
CONST_E_USE_BEFORE_INIT.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_USE_BEFORE_INIT.example = const size: int = 4

CONST_E_DOUBLE_DROP.summary = double Freigabe.
CONST_E_DOUBLE_DROP.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_DOUBLE_DROP.step1 = Reduce the constant expression at the reported span.
CONST_E_DOUBLE_DROP.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_DOUBLE_DROP.example = const size: int = 4

CONST_E_BORROW_CONFLICT.summary = Ausleihe Konflikt.
CONST_E_BORROW_CONFLICT.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_BORROW_CONFLICT.step1 = Reduce the constant expression at the reported span.
CONST_E_BORROW_CONFLICT.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_BORROW_CONFLICT.example = const size: int = 4

CONST_E_MUTABILITY_CONFLICT.summary = mutability Konflikt.
CONST_E_MUTABILITY_CONFLICT.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_MUTABILITY_CONFLICT.step1 = Reduce the constant expression at the reported span.
CONST_E_MUTABILITY_CONFLICT.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_MUTABILITY_CONFLICT.example = const size: int = 4

CONST_E_LIFETIME_TOO_SHORT.summary = Lebensdauer zu short.
CONST_E_LIFETIME_TOO_SHORT.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_LIFETIME_TOO_SHORT.step1 = Reduce the constant expression at the reported span.
CONST_E_LIFETIME_TOO_SHORT.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_LIFETIME_TOO_SHORT.example = const size: int = 4

CONST_E_DANGLING_REFERENCE.summary = dangling Referenz.
CONST_E_DANGLING_REFERENCE.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_DANGLING_REFERENCE.step1 = Reduce the constant expression at the reported span.
CONST_E_DANGLING_REFERENCE.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_DANGLING_REFERENCE.example = const size: int = 4

CONST_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
CONST_E_NON_EXHAUSTIVE_MATCH.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_NON_EXHAUSTIVE_MATCH.step1 = Reduce the constant expression at the reported span.
CONST_E_NON_EXHAUSTIVE_MATCH.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_NON_EXHAUSTIVE_MATCH.example = const size: int = 4

CONST_E_UNREACHABLE_PATTERN.summary = unerreichbar Muster.
CONST_E_UNREACHABLE_PATTERN.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_UNREACHABLE_PATTERN.step1 = Reduce the constant expression at the reported span.
CONST_E_UNREACHABLE_PATTERN.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_UNREACHABLE_PATTERN.example = const size: int = 4

CONST_E_CONST_REQUIRED.summary = const required.
CONST_E_CONST_REQUIRED.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_CONST_REQUIRED.step1 = Reduce the constant expression at the reported span.
CONST_E_CONST_REQUIRED.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_CONST_REQUIRED.example = const size: int = 4

CONST_E_CONST_OVERFLOW.summary = const Ueberlauf.
CONST_E_CONST_OVERFLOW.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_CONST_OVERFLOW.step1 = Reduce the constant expression at the reported span.
CONST_E_CONST_OVERFLOW.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_CONST_OVERFLOW.example = const size: int = 4

CONST_E_CONST_DIVISION_BY_ZERO.summary = const Division by Null.
CONST_E_CONST_DIVISION_BY_ZERO.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_CONST_DIVISION_BY_ZERO.step1 = Reduce the constant expression at the reported span.
CONST_E_CONST_DIVISION_BY_ZERO.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_CONST_DIVISION_BY_ZERO.example = const size: int = 4

CONST_E_CONST_CYCLE.summary = const Zyklus.
CONST_E_CONST_CYCLE.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_CONST_CYCLE.step1 = Reduce the constant expression at the reported span.
CONST_E_CONST_CYCLE.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_CONST_CYCLE.example = const size: int = 4

CONST_E_MACRO_NOT_FOUND.summary = macro fehlend.
CONST_E_MACRO_NOT_FOUND.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_MACRO_NOT_FOUND.step1 = Reduce the constant expression at the reported span.
CONST_E_MACRO_NOT_FOUND.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_MACRO_NOT_FOUND.example = const size: int = 4

CONST_E_MACRO_RECURSION.summary = macro recursion.
CONST_E_MACRO_RECURSION.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_MACRO_RECURSION.step1 = Reduce the constant expression at the reported span.
CONST_E_MACRO_RECURSION.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_MACRO_RECURSION.example = const size: int = 4

CONST_E_MACRO_EXPANSION_FAILED.summary = macro expansion fehlgeschlagen.
CONST_E_MACRO_EXPANSION_FAILED.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_MACRO_EXPANSION_FAILED.step1 = Reduce the constant expression at the reported span.
CONST_E_MACRO_EXPANSION_FAILED.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_MACRO_EXPANSION_FAILED.example = const size: int = 4

CONST_E_TRAIT_NOT_IMPLEMENTED.summary = trait not implemented.
CONST_E_TRAIT_NOT_IMPLEMENTED.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_TRAIT_NOT_IMPLEMENTED.step1 = Reduce the constant expression at the reported span.
CONST_E_TRAIT_NOT_IMPLEMENTED.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_TRAIT_NOT_IMPLEMENTED.example = const size: int = 4

CONST_E_TRAIT_AMBIGUOUS.summary = trait ambiguous.
CONST_E_TRAIT_AMBIGUOUS.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_TRAIT_AMBIGUOUS.step1 = Reduce the constant expression at the reported span.
CONST_E_TRAIT_AMBIGUOUS.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_TRAIT_AMBIGUOUS.example = const size: int = 4

CONST_E_GENERIC_ARGUMENT_MISSING.summary = generisch Argument fehlend.
CONST_E_GENERIC_ARGUMENT_MISSING.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_GENERIC_ARGUMENT_MISSING.step1 = Reduce the constant expression at the reported span.
CONST_E_GENERIC_ARGUMENT_MISSING.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_GENERIC_ARGUMENT_MISSING.example = const size: int = 4

CONST_E_GENERIC_BOUND_FAILED.summary = generisch bound fehlgeschlagen.
CONST_E_GENERIC_BOUND_FAILED.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_GENERIC_BOUND_FAILED.step1 = Reduce the constant expression at the reported span.
CONST_E_GENERIC_BOUND_FAILED.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_GENERIC_BOUND_FAILED.example = const size: int = 4

CONST_E_UNSUPPORTED_TARGET.summary = Ziel nicht unterstuetzt.
CONST_E_UNSUPPORTED_TARGET.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_UNSUPPORTED_TARGET.step1 = Reduce the constant expression at the reported span.
CONST_E_UNSUPPORTED_TARGET.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_UNSUPPORTED_TARGET.example = const size: int = 4

CONST_E_ABI_MISMATCH.summary = abi Nichtuebereinstimmung.
CONST_E_ABI_MISMATCH.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_ABI_MISMATCH.step1 = Reduce the constant expression at the reported span.
CONST_E_ABI_MISMATCH.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_ABI_MISMATCH.example = const size: int = 4

CONST_E_LINK_FAILED.summary = link fehlgeschlagen.
CONST_E_LINK_FAILED.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_LINK_FAILED.step1 = Reduce the constant expression at the reported span.
CONST_E_LINK_FAILED.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_LINK_FAILED.example = const size: int = 4

CONST_E_RUNTIME_PANIC.summary = runtime Panik.
CONST_E_RUNTIME_PANIC.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_RUNTIME_PANIC.step1 = Reduce the constant expression at the reported span.
CONST_E_RUNTIME_PANIC.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_RUNTIME_PANIC.example = const size: int = 4

MACRO_E_EXPECTED_IDENTIFIER.summary = Bezeichner erwartet.
MACRO_E_EXPECTED_IDENTIFIER.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_EXPECTED_IDENTIFIER.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_EXPECTED_IDENTIFIER.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_EXPECTED_IDENTIFIER.example = vitte check path/to/file.vit

MACRO_E_EXPECTED_EXPRESSION.summary = Ausdruck erwartet.
MACRO_E_EXPECTED_EXPRESSION.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_EXPECTED_EXPRESSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_EXPECTED_EXPRESSION.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_EXPECTED_EXPRESSION.example = vitte check path/to/file.vit

MACRO_E_EXPECTED_TYPE.summary = Typ erwartet.
MACRO_E_EXPECTED_TYPE.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_EXPECTED_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_EXPECTED_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_EXPECTED_TYPE.example = vitte check path/to/file.vit

MACRO_E_EXPECTED_PATTERN.summary = Muster erwartet.
MACRO_E_EXPECTED_PATTERN.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_EXPECTED_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_EXPECTED_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_EXPECTED_PATTERN.example = vitte check path/to/file.vit

MACRO_E_EXPECTED_BLOCK.summary = Block erwartet.
MACRO_E_EXPECTED_BLOCK.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_EXPECTED_BLOCK.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_EXPECTED_BLOCK.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_EXPECTED_BLOCK.example = vitte check path/to/file.vit

MACRO_E_EXPECTED_DELIMITER.summary = Trennzeichen erwartet.
MACRO_E_EXPECTED_DELIMITER.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_EXPECTED_DELIMITER.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_EXPECTED_DELIMITER.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_EXPECTED_DELIMITER.example = vitte check path/to/file.vit

MACRO_E_UNEXPECTED_TOKEN.summary = Token unerwartet.
MACRO_E_UNEXPECTED_TOKEN.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_UNEXPECTED_TOKEN.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_UNEXPECTED_TOKEN.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_UNEXPECTED_TOKEN.example = vitte check path/to/file.vit

MACRO_E_UNBALANCED_DELIMITER.summary = unbalanced Trennzeichen.
MACRO_E_UNBALANCED_DELIMITER.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_UNBALANCED_DELIMITER.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_UNBALANCED_DELIMITER.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_UNBALANCED_DELIMITER.example = vitte check path/to/file.vit

MACRO_E_INVALID_ATTRIBUTE.summary = attribute ungueltig.
MACRO_E_INVALID_ATTRIBUTE.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_ATTRIBUTE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_ATTRIBUTE.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_ATTRIBUTE.example = vitte check path/to/file.vit

MACRO_E_INVALID_DECLARATION.summary = Deklaration ungueltig.
MACRO_E_INVALID_DECLARATION.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_DECLARATION.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_DECLARATION.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_DECLARATION.example = vitte check path/to/file.vit

MACRO_E_INVALID_STATEMENT.summary = Anweisung ungueltig.
MACRO_E_INVALID_STATEMENT.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_STATEMENT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_STATEMENT.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_STATEMENT.example = vitte check path/to/file.vit

MACRO_E_INVALID_EXPRESSION.summary = Ausdruck ungueltig.
MACRO_E_INVALID_EXPRESSION.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_EXPRESSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_EXPRESSION.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_EXPRESSION.example = vitte check path/to/file.vit

MACRO_E_INVALID_PATTERN.summary = Muster ungueltig.
MACRO_E_INVALID_PATTERN.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_PATTERN.example = vitte check path/to/file.vit

MACRO_E_INVALID_LITERAL.summary = Literal ungueltig.
MACRO_E_INVALID_LITERAL.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_LITERAL.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_LITERAL.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_LITERAL.example = vitte check path/to/file.vit

MACRO_E_INVALID_OPERATOR.summary = Operator ungueltig.
MACRO_E_INVALID_OPERATOR.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_OPERATOR.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_OPERATOR.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_OPERATOR.example = vitte check path/to/file.vit

MACRO_E_INVALID_MODIFIER.summary = modifier ungueltig.
MACRO_E_INVALID_MODIFIER.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_MODIFIER.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_MODIFIER.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_MODIFIER.example = vitte check path/to/file.vit

MACRO_E_MISSING_BODY.summary = body fehlt.
MACRO_E_MISSING_BODY.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_MISSING_BODY.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_MISSING_BODY.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_MISSING_BODY.example = vitte check path/to/file.vit

MACRO_E_MISSING_RETURN.summary = Rueckgabe fehlt.
MACRO_E_MISSING_RETURN.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_MISSING_RETURN.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_MISSING_RETURN.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_MISSING_RETURN.example = vitte check path/to/file.vit

MACRO_E_DUPLICATE_NAME.summary = Name doppelt.
MACRO_E_DUPLICATE_NAME.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_DUPLICATE_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_DUPLICATE_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_DUPLICATE_NAME.example = vitte check path/to/file.vit

MACRO_E_UNKNOWN_NAME.summary = Name unbekannt.
MACRO_E_UNKNOWN_NAME.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_UNKNOWN_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_UNKNOWN_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_UNKNOWN_NAME.example = vitte check path/to/file.vit

MACRO_E_UNKNOWN_TYPE.summary = Typ unbekannt.
MACRO_E_UNKNOWN_TYPE.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_UNKNOWN_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_UNKNOWN_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_UNKNOWN_TYPE.example = vitte check path/to/file.vit

MACRO_E_UNKNOWN_MODULE.summary = Modul unbekannt.
MACRO_E_UNKNOWN_MODULE.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_UNKNOWN_MODULE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_UNKNOWN_MODULE.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_UNKNOWN_MODULE.example = vitte check path/to/file.vit

MACRO_E_UNKNOWN_MEMBER.summary = Member unbekannt.
MACRO_E_UNKNOWN_MEMBER.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_UNKNOWN_MEMBER.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_UNKNOWN_MEMBER.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_UNKNOWN_MEMBER.example = vitte check path/to/file.vit

MACRO_E_AMBIGUOUS_NAME.summary = ambiguous Name.
MACRO_E_AMBIGUOUS_NAME.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_AMBIGUOUS_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_AMBIGUOUS_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_AMBIGUOUS_NAME.example = vitte check path/to/file.vit

MACRO_E_PRIVATE_SYMBOL.summary = private Symbol.
MACRO_E_PRIVATE_SYMBOL.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_PRIVATE_SYMBOL.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_PRIVATE_SYMBOL.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_PRIVATE_SYMBOL.example = vitte check path/to/file.vit

MACRO_E_IMPORT_NOT_FOUND.summary = Import fehlend.
MACRO_E_IMPORT_NOT_FOUND.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_IMPORT_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_IMPORT_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_IMPORT_NOT_FOUND.example = vitte check path/to/file.vit

MACRO_E_IMPORT_CYCLE.summary = Import Zyklus.
MACRO_E_IMPORT_CYCLE.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_IMPORT_CYCLE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_IMPORT_CYCLE.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_IMPORT_CYCLE.example = vitte check path/to/file.vit

MACRO_E_EXPORT_CONFLICT.summary = export Konflikt.
MACRO_E_EXPORT_CONFLICT.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_EXPORT_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_EXPORT_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_EXPORT_CONFLICT.example = vitte check path/to/file.vit

MACRO_E_ARITY_MISMATCH.summary = arity Nichtuebereinstimmung.
MACRO_E_ARITY_MISMATCH.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_ARITY_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_ARITY_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_ARITY_MISMATCH.example = vitte check path/to/file.vit

MACRO_E_ARGUMENT_MISMATCH.summary = Argument Nichtuebereinstimmung.
MACRO_E_ARGUMENT_MISMATCH.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_ARGUMENT_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_ARGUMENT_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_ARGUMENT_MISMATCH.example = vitte check path/to/file.vit

MACRO_E_ASSIGNMENT_MISMATCH.summary = Zuweisung Nichtuebereinstimmung.
MACRO_E_ASSIGNMENT_MISMATCH.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_ASSIGNMENT_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_ASSIGNMENT_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_ASSIGNMENT_MISMATCH.example = vitte check path/to/file.vit

MACRO_E_BRANCH_MISMATCH.summary = branch Nichtuebereinstimmung.
MACRO_E_BRANCH_MISMATCH.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_BRANCH_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_BRANCH_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_BRANCH_MISMATCH.example = vitte check path/to/file.vit

MACRO_E_INVALID_CALL.summary = Aufruf ungueltig.
MACRO_E_INVALID_CALL.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_CALL.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_CALL.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_CALL.example = vitte check path/to/file.vit

MACRO_E_INVALID_CAST.summary = Umwandlung ungueltig.
MACRO_E_INVALID_CAST.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_CAST.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_CAST.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_CAST.example = vitte check path/to/file.vit

MACRO_E_INVALID_INDEX.summary = Index ungueltig.
MACRO_E_INVALID_INDEX.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_INDEX.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_INDEX.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_INDEX.example = vitte check path/to/file.vit

MACRO_E_INVALID_DEREF.summary = deref ungueltig.
MACRO_E_INVALID_DEREF.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_DEREF.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_DEREF.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_DEREF.example = vitte check path/to/file.vit

MACRO_E_INVALID_BORROW.summary = Ausleihe ungueltig.
MACRO_E_INVALID_BORROW.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_BORROW.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_BORROW.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_BORROW.example = vitte check path/to/file.vit

MACRO_E_INVALID_MOVE.summary = Verschiebung ungueltig.
MACRO_E_INVALID_MOVE.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_MOVE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_MOVE.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_MOVE.example = vitte check path/to/file.vit

MACRO_E_USE_AFTER_MOVE.summary = use after Verschiebung.
MACRO_E_USE_AFTER_MOVE.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_USE_AFTER_MOVE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_USE_AFTER_MOVE.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_USE_AFTER_MOVE.example = vitte check path/to/file.vit

MACRO_E_USE_AFTER_DROP.summary = use after Freigabe.
MACRO_E_USE_AFTER_DROP.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_USE_AFTER_DROP.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_USE_AFTER_DROP.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_USE_AFTER_DROP.example = vitte check path/to/file.vit

MACRO_E_USE_BEFORE_INIT.summary = use before init.
MACRO_E_USE_BEFORE_INIT.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_USE_BEFORE_INIT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_USE_BEFORE_INIT.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_USE_BEFORE_INIT.example = vitte check path/to/file.vit

MACRO_E_DOUBLE_DROP.summary = double Freigabe.
MACRO_E_DOUBLE_DROP.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_DOUBLE_DROP.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_DOUBLE_DROP.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_DOUBLE_DROP.example = vitte check path/to/file.vit

MACRO_E_BORROW_CONFLICT.summary = Ausleihe Konflikt.
MACRO_E_BORROW_CONFLICT.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_BORROW_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_BORROW_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_BORROW_CONFLICT.example = vitte check path/to/file.vit

MACRO_E_MUTABILITY_CONFLICT.summary = mutability Konflikt.
MACRO_E_MUTABILITY_CONFLICT.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_MUTABILITY_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_MUTABILITY_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_MUTABILITY_CONFLICT.example = vitte check path/to/file.vit

MACRO_E_LIFETIME_TOO_SHORT.summary = Lebensdauer zu short.
MACRO_E_LIFETIME_TOO_SHORT.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_LIFETIME_TOO_SHORT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_LIFETIME_TOO_SHORT.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_LIFETIME_TOO_SHORT.example = vitte check path/to/file.vit

MACRO_E_DANGLING_REFERENCE.summary = dangling Referenz.
MACRO_E_DANGLING_REFERENCE.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_DANGLING_REFERENCE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_DANGLING_REFERENCE.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_DANGLING_REFERENCE.example = vitte check path/to/file.vit

MACRO_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
MACRO_E_NON_EXHAUSTIVE_MATCH.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_NON_EXHAUSTIVE_MATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_NON_EXHAUSTIVE_MATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_NON_EXHAUSTIVE_MATCH.example = vitte check path/to/file.vit

MACRO_E_UNREACHABLE_PATTERN.summary = unerreichbar Muster.
MACRO_E_UNREACHABLE_PATTERN.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_UNREACHABLE_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_UNREACHABLE_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_UNREACHABLE_PATTERN.example = vitte check path/to/file.vit

MACRO_E_CONST_REQUIRED.summary = const required.
MACRO_E_CONST_REQUIRED.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_CONST_REQUIRED.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_CONST_REQUIRED.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_CONST_REQUIRED.example = vitte check path/to/file.vit

MACRO_E_CONST_OVERFLOW.summary = const Ueberlauf.
MACRO_E_CONST_OVERFLOW.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_CONST_OVERFLOW.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_CONST_OVERFLOW.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_CONST_OVERFLOW.example = vitte check path/to/file.vit

MACRO_E_CONST_DIVISION_BY_ZERO.summary = const Division by Null.
MACRO_E_CONST_DIVISION_BY_ZERO.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_CONST_DIVISION_BY_ZERO.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_CONST_DIVISION_BY_ZERO.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_CONST_DIVISION_BY_ZERO.example = vitte check path/to/file.vit

MACRO_E_CONST_CYCLE.summary = const Zyklus.
MACRO_E_CONST_CYCLE.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_CONST_CYCLE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_CONST_CYCLE.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_CONST_CYCLE.example = vitte check path/to/file.vit

MACRO_E_MACRO_NOT_FOUND.summary = macro fehlend.
MACRO_E_MACRO_NOT_FOUND.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_MACRO_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_MACRO_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_MACRO_NOT_FOUND.example = vitte check path/to/file.vit

MACRO_E_MACRO_RECURSION.summary = macro recursion.
MACRO_E_MACRO_RECURSION.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_MACRO_RECURSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_MACRO_RECURSION.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_MACRO_RECURSION.example = vitte check path/to/file.vit

MACRO_E_MACRO_EXPANSION_FAILED.summary = macro expansion fehlgeschlagen.
MACRO_E_MACRO_EXPANSION_FAILED.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_MACRO_EXPANSION_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_MACRO_EXPANSION_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_MACRO_EXPANSION_FAILED.example = vitte check path/to/file.vit

MACRO_E_TRAIT_NOT_IMPLEMENTED.summary = trait not implemented.
MACRO_E_TRAIT_NOT_IMPLEMENTED.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_TRAIT_NOT_IMPLEMENTED.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_TRAIT_NOT_IMPLEMENTED.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_TRAIT_NOT_IMPLEMENTED.example = vitte check path/to/file.vit

MACRO_E_TRAIT_AMBIGUOUS.summary = trait ambiguous.
MACRO_E_TRAIT_AMBIGUOUS.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_TRAIT_AMBIGUOUS.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_TRAIT_AMBIGUOUS.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_TRAIT_AMBIGUOUS.example = vitte check path/to/file.vit

MACRO_E_GENERIC_ARGUMENT_MISSING.summary = generisch Argument fehlend.
MACRO_E_GENERIC_ARGUMENT_MISSING.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_GENERIC_ARGUMENT_MISSING.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_GENERIC_ARGUMENT_MISSING.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_GENERIC_ARGUMENT_MISSING.example = vitte check path/to/file.vit

MACRO_E_GENERIC_BOUND_FAILED.summary = generisch bound fehlgeschlagen.
MACRO_E_GENERIC_BOUND_FAILED.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_GENERIC_BOUND_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_GENERIC_BOUND_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_GENERIC_BOUND_FAILED.example = vitte check path/to/file.vit

MACRO_E_UNSUPPORTED_TARGET.summary = Ziel nicht unterstuetzt.
MACRO_E_UNSUPPORTED_TARGET.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_UNSUPPORTED_TARGET.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_UNSUPPORTED_TARGET.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_UNSUPPORTED_TARGET.example = vitte check path/to/file.vit

MACRO_E_ABI_MISMATCH.summary = abi Nichtuebereinstimmung.
MACRO_E_ABI_MISMATCH.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_ABI_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_ABI_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_ABI_MISMATCH.example = vitte check path/to/file.vit

MACRO_E_LINK_FAILED.summary = link fehlgeschlagen.
MACRO_E_LINK_FAILED.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_LINK_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_LINK_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_LINK_FAILED.example = vitte check path/to/file.vit

MACRO_E_RUNTIME_PANIC.summary = runtime Panik.
MACRO_E_RUNTIME_PANIC.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_RUNTIME_PANIC.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_RUNTIME_PANIC.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_RUNTIME_PANIC.example = vitte check path/to/file.vit

HIR_E_EXPECTED_IDENTIFIER.summary = Bezeichner erwartet.
HIR_E_EXPECTED_IDENTIFIER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_EXPECTED_IDENTIFIER.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_EXPECTED_IDENTIFIER.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_EXPECTED_IDENTIFIER.example = vitte check path/to/file.vit

HIR_E_EXPECTED_EXPRESSION.summary = Ausdruck erwartet.
HIR_E_EXPECTED_EXPRESSION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_EXPECTED_EXPRESSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_EXPECTED_EXPRESSION.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_EXPECTED_EXPRESSION.example = vitte check path/to/file.vit

HIR_E_EXPECTED_TYPE.summary = Typ erwartet.
HIR_E_EXPECTED_TYPE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_EXPECTED_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_EXPECTED_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_EXPECTED_TYPE.example = vitte check path/to/file.vit

HIR_E_EXPECTED_PATTERN.summary = Muster erwartet.
HIR_E_EXPECTED_PATTERN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_EXPECTED_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_EXPECTED_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_EXPECTED_PATTERN.example = vitte check path/to/file.vit

HIR_E_EXPECTED_BLOCK.summary = Block erwartet.
HIR_E_EXPECTED_BLOCK.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_EXPECTED_BLOCK.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_EXPECTED_BLOCK.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_EXPECTED_BLOCK.example = vitte check path/to/file.vit

HIR_E_EXPECTED_DELIMITER.summary = Trennzeichen erwartet.
HIR_E_EXPECTED_DELIMITER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_EXPECTED_DELIMITER.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_EXPECTED_DELIMITER.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_EXPECTED_DELIMITER.example = vitte check path/to/file.vit

HIR_E_UNEXPECTED_TOKEN.summary = Token unerwartet.
HIR_E_UNEXPECTED_TOKEN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_UNEXPECTED_TOKEN.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_UNEXPECTED_TOKEN.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_UNEXPECTED_TOKEN.example = vitte check path/to/file.vit

HIR_E_UNBALANCED_DELIMITER.summary = unbalanced Trennzeichen.
HIR_E_UNBALANCED_DELIMITER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_UNBALANCED_DELIMITER.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_UNBALANCED_DELIMITER.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_UNBALANCED_DELIMITER.example = vitte check path/to/file.vit

HIR_E_INVALID_ATTRIBUTE.summary = attribute ungueltig.
HIR_E_INVALID_ATTRIBUTE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_ATTRIBUTE.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_ATTRIBUTE.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_ATTRIBUTE.example = vitte check path/to/file.vit

HIR_E_INVALID_DECLARATION.summary = Deklaration ungueltig.
HIR_E_INVALID_DECLARATION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_DECLARATION.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_DECLARATION.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_DECLARATION.example = vitte check path/to/file.vit

HIR_E_INVALID_STATEMENT.summary = Anweisung ungueltig.
HIR_E_INVALID_STATEMENT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_STATEMENT.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_STATEMENT.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_STATEMENT.example = vitte check path/to/file.vit

HIR_E_INVALID_EXPRESSION.summary = Ausdruck ungueltig.
HIR_E_INVALID_EXPRESSION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_EXPRESSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_EXPRESSION.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_EXPRESSION.example = vitte check path/to/file.vit

HIR_E_INVALID_LITERAL.summary = Literal ungueltig.
HIR_E_INVALID_LITERAL.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_LITERAL.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_LITERAL.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_LITERAL.example = vitte check path/to/file.vit

HIR_E_INVALID_OPERATOR.summary = Operator ungueltig.
HIR_E_INVALID_OPERATOR.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_OPERATOR.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_OPERATOR.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_OPERATOR.example = vitte check path/to/file.vit

HIR_E_INVALID_MODIFIER.summary = modifier ungueltig.
HIR_E_INVALID_MODIFIER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_MODIFIER.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_MODIFIER.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_MODIFIER.example = vitte check path/to/file.vit

HIR_E_MISSING_BODY.summary = body fehlt.
HIR_E_MISSING_BODY.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_MISSING_BODY.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_MISSING_BODY.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_MISSING_BODY.example = vitte check path/to/file.vit

HIR_E_MISSING_RETURN.summary = Rueckgabe fehlt.
HIR_E_MISSING_RETURN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_MISSING_RETURN.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_MISSING_RETURN.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_MISSING_RETURN.example = vitte check path/to/file.vit

HIR_E_DUPLICATE_NAME.summary = Name doppelt.
HIR_E_DUPLICATE_NAME.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_DUPLICATE_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_DUPLICATE_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_DUPLICATE_NAME.example = vitte check path/to/file.vit

HIR_E_UNKNOWN_NAME.summary = Name unbekannt.
HIR_E_UNKNOWN_NAME.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_UNKNOWN_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_UNKNOWN_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_UNKNOWN_NAME.example = vitte check path/to/file.vit

HIR_E_UNKNOWN_TYPE.summary = Typ unbekannt.
HIR_E_UNKNOWN_TYPE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_UNKNOWN_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_UNKNOWN_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_UNKNOWN_TYPE.example = vitte check path/to/file.vit

HIR_E_UNKNOWN_MODULE.summary = Modul unbekannt.
HIR_E_UNKNOWN_MODULE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_UNKNOWN_MODULE.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_UNKNOWN_MODULE.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_UNKNOWN_MODULE.example = vitte check path/to/file.vit

HIR_E_UNKNOWN_MEMBER.summary = Member unbekannt.
HIR_E_UNKNOWN_MEMBER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_UNKNOWN_MEMBER.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_UNKNOWN_MEMBER.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_UNKNOWN_MEMBER.example = vitte check path/to/file.vit

HIR_E_AMBIGUOUS_NAME.summary = ambiguous Name.
HIR_E_AMBIGUOUS_NAME.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_AMBIGUOUS_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_AMBIGUOUS_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_AMBIGUOUS_NAME.example = vitte check path/to/file.vit

HIR_E_PRIVATE_SYMBOL.summary = private Symbol.
HIR_E_PRIVATE_SYMBOL.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_PRIVATE_SYMBOL.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_PRIVATE_SYMBOL.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_PRIVATE_SYMBOL.example = vitte check path/to/file.vit

HIR_E_IMPORT_NOT_FOUND.summary = Import fehlend.
HIR_E_IMPORT_NOT_FOUND.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_IMPORT_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_IMPORT_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_IMPORT_NOT_FOUND.example = vitte check path/to/file.vit

HIR_E_IMPORT_CYCLE.summary = Import Zyklus.
HIR_E_IMPORT_CYCLE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_IMPORT_CYCLE.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_IMPORT_CYCLE.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_IMPORT_CYCLE.example = vitte check path/to/file.vit

HIR_E_EXPORT_CONFLICT.summary = export Konflikt.
HIR_E_EXPORT_CONFLICT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_EXPORT_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_EXPORT_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_EXPORT_CONFLICT.example = vitte check path/to/file.vit

HIR_E_ARITY_MISMATCH.summary = arity Nichtuebereinstimmung.
HIR_E_ARITY_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_ARITY_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_ARITY_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_ARITY_MISMATCH.example = vitte check path/to/file.vit

HIR_E_ARGUMENT_MISMATCH.summary = Argument Nichtuebereinstimmung.
HIR_E_ARGUMENT_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_ARGUMENT_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_ARGUMENT_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_ARGUMENT_MISMATCH.example = vitte check path/to/file.vit

HIR_E_ASSIGNMENT_MISMATCH.summary = Zuweisung Nichtuebereinstimmung.
HIR_E_ASSIGNMENT_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_ASSIGNMENT_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_ASSIGNMENT_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_ASSIGNMENT_MISMATCH.example = vitte check path/to/file.vit

HIR_E_BRANCH_MISMATCH.summary = branch Nichtuebereinstimmung.
HIR_E_BRANCH_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_BRANCH_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_BRANCH_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_BRANCH_MISMATCH.example = vitte check path/to/file.vit

HIR_E_INVALID_CALL.summary = Aufruf ungueltig.
HIR_E_INVALID_CALL.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_CALL.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_CALL.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_CALL.example = vitte check path/to/file.vit

HIR_E_INVALID_CAST.summary = Umwandlung ungueltig.
HIR_E_INVALID_CAST.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_CAST.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_CAST.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_CAST.example = vitte check path/to/file.vit

HIR_E_INVALID_INDEX.summary = Index ungueltig.
HIR_E_INVALID_INDEX.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_INDEX.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_INDEX.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_INDEX.example = vitte check path/to/file.vit

HIR_E_INVALID_DEREF.summary = deref ungueltig.
HIR_E_INVALID_DEREF.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_DEREF.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_DEREF.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_DEREF.example = vitte check path/to/file.vit

HIR_E_INVALID_BORROW.summary = Ausleihe ungueltig.
HIR_E_INVALID_BORROW.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_BORROW.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_BORROW.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_BORROW.example = vitte check path/to/file.vit

HIR_E_INVALID_MOVE.summary = Verschiebung ungueltig.
HIR_E_INVALID_MOVE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_MOVE.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_MOVE.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_MOVE.example = vitte check path/to/file.vit

HIR_E_USE_AFTER_MOVE.summary = use after Verschiebung.
HIR_E_USE_AFTER_MOVE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_USE_AFTER_MOVE.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_USE_AFTER_MOVE.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_USE_AFTER_MOVE.example = vitte check path/to/file.vit

HIR_E_USE_AFTER_DROP.summary = use after Freigabe.
HIR_E_USE_AFTER_DROP.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_USE_AFTER_DROP.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_USE_AFTER_DROP.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_USE_AFTER_DROP.example = vitte check path/to/file.vit

HIR_E_USE_BEFORE_INIT.summary = use before init.
HIR_E_USE_BEFORE_INIT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_USE_BEFORE_INIT.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_USE_BEFORE_INIT.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_USE_BEFORE_INIT.example = vitte check path/to/file.vit

HIR_E_DOUBLE_DROP.summary = double Freigabe.
HIR_E_DOUBLE_DROP.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_DOUBLE_DROP.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_DOUBLE_DROP.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_DOUBLE_DROP.example = vitte check path/to/file.vit

HIR_E_BORROW_CONFLICT.summary = Ausleihe Konflikt.
HIR_E_BORROW_CONFLICT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_BORROW_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_BORROW_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_BORROW_CONFLICT.example = vitte check path/to/file.vit

HIR_E_MUTABILITY_CONFLICT.summary = mutability Konflikt.
HIR_E_MUTABILITY_CONFLICT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_MUTABILITY_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_MUTABILITY_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_MUTABILITY_CONFLICT.example = vitte check path/to/file.vit

HIR_E_LIFETIME_TOO_SHORT.summary = Lebensdauer zu short.
HIR_E_LIFETIME_TOO_SHORT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_LIFETIME_TOO_SHORT.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_LIFETIME_TOO_SHORT.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_LIFETIME_TOO_SHORT.example = vitte check path/to/file.vit

HIR_E_DANGLING_REFERENCE.summary = dangling Referenz.
HIR_E_DANGLING_REFERENCE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_DANGLING_REFERENCE.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_DANGLING_REFERENCE.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_DANGLING_REFERENCE.example = vitte check path/to/file.vit

HIR_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
HIR_E_NON_EXHAUSTIVE_MATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_NON_EXHAUSTIVE_MATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_NON_EXHAUSTIVE_MATCH.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_NON_EXHAUSTIVE_MATCH.example = vitte check path/to/file.vit

HIR_E_UNREACHABLE_PATTERN.summary = unerreichbar Muster.
HIR_E_UNREACHABLE_PATTERN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_UNREACHABLE_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_UNREACHABLE_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_UNREACHABLE_PATTERN.example = vitte check path/to/file.vit

HIR_E_CONST_REQUIRED.summary = const required.
HIR_E_CONST_REQUIRED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_CONST_REQUIRED.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_CONST_REQUIRED.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_CONST_REQUIRED.example = vitte check path/to/file.vit

HIR_E_CONST_OVERFLOW.summary = const Ueberlauf.
HIR_E_CONST_OVERFLOW.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_CONST_OVERFLOW.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_CONST_OVERFLOW.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_CONST_OVERFLOW.example = vitte check path/to/file.vit

HIR_E_CONST_DIVISION_BY_ZERO.summary = const Division by Null.
HIR_E_CONST_DIVISION_BY_ZERO.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_CONST_DIVISION_BY_ZERO.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_CONST_DIVISION_BY_ZERO.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_CONST_DIVISION_BY_ZERO.example = vitte check path/to/file.vit

HIR_E_CONST_CYCLE.summary = const Zyklus.
HIR_E_CONST_CYCLE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_CONST_CYCLE.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_CONST_CYCLE.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_CONST_CYCLE.example = vitte check path/to/file.vit

HIR_E_MACRO_NOT_FOUND.summary = macro fehlend.
HIR_E_MACRO_NOT_FOUND.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_MACRO_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_MACRO_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_MACRO_NOT_FOUND.example = vitte check path/to/file.vit

HIR_E_MACRO_RECURSION.summary = macro recursion.
HIR_E_MACRO_RECURSION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_MACRO_RECURSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_MACRO_RECURSION.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_MACRO_RECURSION.example = vitte check path/to/file.vit

HIR_E_MACRO_EXPANSION_FAILED.summary = macro expansion fehlgeschlagen.
HIR_E_MACRO_EXPANSION_FAILED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_MACRO_EXPANSION_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_MACRO_EXPANSION_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_MACRO_EXPANSION_FAILED.example = vitte check path/to/file.vit

HIR_E_TRAIT_NOT_IMPLEMENTED.summary = trait not implemented.
HIR_E_TRAIT_NOT_IMPLEMENTED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_TRAIT_NOT_IMPLEMENTED.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_TRAIT_NOT_IMPLEMENTED.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_TRAIT_NOT_IMPLEMENTED.example = vitte check path/to/file.vit

HIR_E_TRAIT_AMBIGUOUS.summary = trait ambiguous.
HIR_E_TRAIT_AMBIGUOUS.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_TRAIT_AMBIGUOUS.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_TRAIT_AMBIGUOUS.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_TRAIT_AMBIGUOUS.example = vitte check path/to/file.vit

HIR_E_GENERIC_ARGUMENT_MISSING.summary = generisch Argument fehlend.
HIR_E_GENERIC_ARGUMENT_MISSING.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_GENERIC_ARGUMENT_MISSING.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_GENERIC_ARGUMENT_MISSING.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_GENERIC_ARGUMENT_MISSING.example = vitte check path/to/file.vit

HIR_E_GENERIC_BOUND_FAILED.summary = generisch bound fehlgeschlagen.
HIR_E_GENERIC_BOUND_FAILED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_GENERIC_BOUND_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_GENERIC_BOUND_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_GENERIC_BOUND_FAILED.example = vitte check path/to/file.vit

HIR_E_UNSUPPORTED_TARGET.summary = Ziel nicht unterstuetzt.
HIR_E_UNSUPPORTED_TARGET.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_UNSUPPORTED_TARGET.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_UNSUPPORTED_TARGET.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_UNSUPPORTED_TARGET.example = vitte check path/to/file.vit

HIR_E_ABI_MISMATCH.summary = abi Nichtuebereinstimmung.
HIR_E_ABI_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_ABI_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_ABI_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_ABI_MISMATCH.example = vitte check path/to/file.vit

HIR_E_LINK_FAILED.summary = link fehlgeschlagen.
HIR_E_LINK_FAILED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_LINK_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_LINK_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_LINK_FAILED.example = vitte check path/to/file.vit

HIR_E_RUNTIME_PANIC.summary = runtime Panik.
HIR_E_RUNTIME_PANIC.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_RUNTIME_PANIC.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_RUNTIME_PANIC.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_RUNTIME_PANIC.example = vitte check path/to/file.vit

MIR_E_EXPECTED_IDENTIFIER.summary = Bezeichner erwartet.
MIR_E_EXPECTED_IDENTIFIER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_EXPECTED_IDENTIFIER.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_EXPECTED_IDENTIFIER.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_EXPECTED_IDENTIFIER.example = vitte check path/to/file.vit

MIR_E_EXPECTED_EXPRESSION.summary = Ausdruck erwartet.
MIR_E_EXPECTED_EXPRESSION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_EXPECTED_EXPRESSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_EXPECTED_EXPRESSION.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_EXPECTED_EXPRESSION.example = vitte check path/to/file.vit

MIR_E_EXPECTED_TYPE.summary = Typ erwartet.
MIR_E_EXPECTED_TYPE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_EXPECTED_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_EXPECTED_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_EXPECTED_TYPE.example = vitte check path/to/file.vit

MIR_E_EXPECTED_PATTERN.summary = Muster erwartet.
MIR_E_EXPECTED_PATTERN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_EXPECTED_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_EXPECTED_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_EXPECTED_PATTERN.example = vitte check path/to/file.vit

MIR_E_EXPECTED_BLOCK.summary = Block erwartet.
MIR_E_EXPECTED_BLOCK.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_EXPECTED_BLOCK.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_EXPECTED_BLOCK.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_EXPECTED_BLOCK.example = vitte check path/to/file.vit

MIR_E_EXPECTED_DELIMITER.summary = Trennzeichen erwartet.
MIR_E_EXPECTED_DELIMITER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_EXPECTED_DELIMITER.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_EXPECTED_DELIMITER.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_EXPECTED_DELIMITER.example = vitte check path/to/file.vit

MIR_E_UNEXPECTED_TOKEN.summary = Token unerwartet.
MIR_E_UNEXPECTED_TOKEN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_UNEXPECTED_TOKEN.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_UNEXPECTED_TOKEN.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_UNEXPECTED_TOKEN.example = vitte check path/to/file.vit

MIR_E_UNBALANCED_DELIMITER.summary = unbalanced Trennzeichen.
MIR_E_UNBALANCED_DELIMITER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_UNBALANCED_DELIMITER.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_UNBALANCED_DELIMITER.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_UNBALANCED_DELIMITER.example = vitte check path/to/file.vit

MIR_E_INVALID_ATTRIBUTE.summary = attribute ungueltig.
MIR_E_INVALID_ATTRIBUTE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_ATTRIBUTE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_ATTRIBUTE.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_ATTRIBUTE.example = vitte check path/to/file.vit

MIR_E_INVALID_DECLARATION.summary = Deklaration ungueltig.
MIR_E_INVALID_DECLARATION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_DECLARATION.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_DECLARATION.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_DECLARATION.example = vitte check path/to/file.vit

MIR_E_INVALID_STATEMENT.summary = Anweisung ungueltig.
MIR_E_INVALID_STATEMENT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_STATEMENT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_STATEMENT.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_STATEMENT.example = vitte check path/to/file.vit

MIR_E_INVALID_EXPRESSION.summary = Ausdruck ungueltig.
MIR_E_INVALID_EXPRESSION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_EXPRESSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_EXPRESSION.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_EXPRESSION.example = vitte check path/to/file.vit

MIR_E_INVALID_PATTERN.summary = Muster ungueltig.
MIR_E_INVALID_PATTERN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_PATTERN.example = vitte check path/to/file.vit

MIR_E_INVALID_LITERAL.summary = Literal ungueltig.
MIR_E_INVALID_LITERAL.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_LITERAL.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_LITERAL.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_LITERAL.example = vitte check path/to/file.vit

MIR_E_INVALID_OPERATOR.summary = Operator ungueltig.
MIR_E_INVALID_OPERATOR.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_OPERATOR.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_OPERATOR.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_OPERATOR.example = vitte check path/to/file.vit

MIR_E_INVALID_MODIFIER.summary = modifier ungueltig.
MIR_E_INVALID_MODIFIER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_MODIFIER.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_MODIFIER.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_MODIFIER.example = vitte check path/to/file.vit

MIR_E_MISSING_BODY.summary = body fehlt.
MIR_E_MISSING_BODY.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_MISSING_BODY.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_MISSING_BODY.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_MISSING_BODY.example = vitte check path/to/file.vit

MIR_E_MISSING_RETURN.summary = Rueckgabe fehlt.
MIR_E_MISSING_RETURN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_MISSING_RETURN.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_MISSING_RETURN.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_MISSING_RETURN.example = vitte check path/to/file.vit

MIR_E_DUPLICATE_NAME.summary = Name doppelt.
MIR_E_DUPLICATE_NAME.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_DUPLICATE_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_DUPLICATE_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_DUPLICATE_NAME.example = vitte check path/to/file.vit

MIR_E_UNKNOWN_NAME.summary = Name unbekannt.
MIR_E_UNKNOWN_NAME.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_UNKNOWN_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_UNKNOWN_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_UNKNOWN_NAME.example = vitte check path/to/file.vit

MIR_E_UNKNOWN_TYPE.summary = Typ unbekannt.
MIR_E_UNKNOWN_TYPE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_UNKNOWN_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_UNKNOWN_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_UNKNOWN_TYPE.example = vitte check path/to/file.vit

MIR_E_UNKNOWN_MODULE.summary = Modul unbekannt.
MIR_E_UNKNOWN_MODULE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_UNKNOWN_MODULE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_UNKNOWN_MODULE.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_UNKNOWN_MODULE.example = vitte check path/to/file.vit

MIR_E_UNKNOWN_MEMBER.summary = Member unbekannt.
MIR_E_UNKNOWN_MEMBER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_UNKNOWN_MEMBER.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_UNKNOWN_MEMBER.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_UNKNOWN_MEMBER.example = vitte check path/to/file.vit

MIR_E_AMBIGUOUS_NAME.summary = ambiguous Name.
MIR_E_AMBIGUOUS_NAME.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_AMBIGUOUS_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_AMBIGUOUS_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_AMBIGUOUS_NAME.example = vitte check path/to/file.vit

MIR_E_PRIVATE_SYMBOL.summary = private Symbol.
MIR_E_PRIVATE_SYMBOL.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_PRIVATE_SYMBOL.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_PRIVATE_SYMBOL.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_PRIVATE_SYMBOL.example = vitte check path/to/file.vit

MIR_E_IMPORT_NOT_FOUND.summary = Import fehlend.
MIR_E_IMPORT_NOT_FOUND.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_IMPORT_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_IMPORT_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_IMPORT_NOT_FOUND.example = vitte check path/to/file.vit

MIR_E_IMPORT_CYCLE.summary = Import Zyklus.
MIR_E_IMPORT_CYCLE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_IMPORT_CYCLE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_IMPORT_CYCLE.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_IMPORT_CYCLE.example = vitte check path/to/file.vit

MIR_E_EXPORT_CONFLICT.summary = export Konflikt.
MIR_E_EXPORT_CONFLICT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_EXPORT_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_EXPORT_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_EXPORT_CONFLICT.example = vitte check path/to/file.vit

MIR_E_ARITY_MISMATCH.summary = arity Nichtuebereinstimmung.
MIR_E_ARITY_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_ARITY_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_ARITY_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_ARITY_MISMATCH.example = vitte check path/to/file.vit

MIR_E_ARGUMENT_MISMATCH.summary = Argument Nichtuebereinstimmung.
MIR_E_ARGUMENT_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_ARGUMENT_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_ARGUMENT_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_ARGUMENT_MISMATCH.example = vitte check path/to/file.vit

MIR_E_ASSIGNMENT_MISMATCH.summary = Zuweisung Nichtuebereinstimmung.
MIR_E_ASSIGNMENT_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_ASSIGNMENT_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_ASSIGNMENT_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_ASSIGNMENT_MISMATCH.example = vitte check path/to/file.vit

MIR_E_BRANCH_MISMATCH.summary = branch Nichtuebereinstimmung.
MIR_E_BRANCH_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_BRANCH_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_BRANCH_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_BRANCH_MISMATCH.example = vitte check path/to/file.vit

MIR_E_INVALID_CALL.summary = Aufruf ungueltig.
MIR_E_INVALID_CALL.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_CALL.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_CALL.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_CALL.example = vitte check path/to/file.vit

MIR_E_INVALID_CAST.summary = Umwandlung ungueltig.
MIR_E_INVALID_CAST.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_CAST.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_CAST.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_CAST.example = vitte check path/to/file.vit

MIR_E_INVALID_INDEX.summary = Index ungueltig.
MIR_E_INVALID_INDEX.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_INDEX.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_INDEX.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_INDEX.example = vitte check path/to/file.vit

MIR_E_INVALID_DEREF.summary = deref ungueltig.
MIR_E_INVALID_DEREF.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_DEREF.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_DEREF.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_DEREF.example = vitte check path/to/file.vit

MIR_E_INVALID_BORROW.summary = Ausleihe ungueltig.
MIR_E_INVALID_BORROW.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_BORROW.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_BORROW.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_BORROW.example = vitte check path/to/file.vit

MIR_E_INVALID_MOVE.summary = Verschiebung ungueltig.
MIR_E_INVALID_MOVE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_MOVE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_MOVE.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_MOVE.example = vitte check path/to/file.vit

MIR_E_USE_AFTER_MOVE.summary = use after Verschiebung.
MIR_E_USE_AFTER_MOVE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_USE_AFTER_MOVE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_USE_AFTER_MOVE.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_USE_AFTER_MOVE.example = vitte check path/to/file.vit

MIR_E_USE_AFTER_DROP.summary = use after Freigabe.
MIR_E_USE_AFTER_DROP.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_USE_AFTER_DROP.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_USE_AFTER_DROP.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_USE_AFTER_DROP.example = vitte check path/to/file.vit

MIR_E_USE_BEFORE_INIT.summary = use before init.
MIR_E_USE_BEFORE_INIT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_USE_BEFORE_INIT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_USE_BEFORE_INIT.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_USE_BEFORE_INIT.example = vitte check path/to/file.vit

MIR_E_DOUBLE_DROP.summary = double Freigabe.
MIR_E_DOUBLE_DROP.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_DOUBLE_DROP.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_DOUBLE_DROP.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_DOUBLE_DROP.example = vitte check path/to/file.vit

MIR_E_BORROW_CONFLICT.summary = Ausleihe Konflikt.
MIR_E_BORROW_CONFLICT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_BORROW_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_BORROW_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_BORROW_CONFLICT.example = vitte check path/to/file.vit

MIR_E_MUTABILITY_CONFLICT.summary = mutability Konflikt.
MIR_E_MUTABILITY_CONFLICT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_MUTABILITY_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_MUTABILITY_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_MUTABILITY_CONFLICT.example = vitte check path/to/file.vit

MIR_E_LIFETIME_TOO_SHORT.summary = Lebensdauer zu short.
MIR_E_LIFETIME_TOO_SHORT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_LIFETIME_TOO_SHORT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_LIFETIME_TOO_SHORT.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_LIFETIME_TOO_SHORT.example = vitte check path/to/file.vit

MIR_E_DANGLING_REFERENCE.summary = dangling Referenz.
MIR_E_DANGLING_REFERENCE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_DANGLING_REFERENCE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_DANGLING_REFERENCE.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_DANGLING_REFERENCE.example = vitte check path/to/file.vit

MIR_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
MIR_E_NON_EXHAUSTIVE_MATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_NON_EXHAUSTIVE_MATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_NON_EXHAUSTIVE_MATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_NON_EXHAUSTIVE_MATCH.example = vitte check path/to/file.vit

MIR_E_UNREACHABLE_PATTERN.summary = unerreichbar Muster.
MIR_E_UNREACHABLE_PATTERN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_UNREACHABLE_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_UNREACHABLE_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_UNREACHABLE_PATTERN.example = vitte check path/to/file.vit

MIR_E_CONST_REQUIRED.summary = const required.
MIR_E_CONST_REQUIRED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_CONST_REQUIRED.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_CONST_REQUIRED.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_CONST_REQUIRED.example = vitte check path/to/file.vit

MIR_E_CONST_OVERFLOW.summary = const Ueberlauf.
MIR_E_CONST_OVERFLOW.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_CONST_OVERFLOW.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_CONST_OVERFLOW.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_CONST_OVERFLOW.example = vitte check path/to/file.vit

MIR_E_CONST_DIVISION_BY_ZERO.summary = const Division by Null.
MIR_E_CONST_DIVISION_BY_ZERO.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_CONST_DIVISION_BY_ZERO.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_CONST_DIVISION_BY_ZERO.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_CONST_DIVISION_BY_ZERO.example = vitte check path/to/file.vit

MIR_E_CONST_CYCLE.summary = const Zyklus.
MIR_E_CONST_CYCLE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_CONST_CYCLE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_CONST_CYCLE.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_CONST_CYCLE.example = vitte check path/to/file.vit

MIR_E_MACRO_NOT_FOUND.summary = macro fehlend.
MIR_E_MACRO_NOT_FOUND.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_MACRO_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_MACRO_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_MACRO_NOT_FOUND.example = vitte check path/to/file.vit

MIR_E_MACRO_RECURSION.summary = macro recursion.
MIR_E_MACRO_RECURSION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_MACRO_RECURSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_MACRO_RECURSION.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_MACRO_RECURSION.example = vitte check path/to/file.vit

MIR_E_MACRO_EXPANSION_FAILED.summary = macro expansion fehlgeschlagen.
MIR_E_MACRO_EXPANSION_FAILED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_MACRO_EXPANSION_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_MACRO_EXPANSION_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_MACRO_EXPANSION_FAILED.example = vitte check path/to/file.vit

MIR_E_TRAIT_NOT_IMPLEMENTED.summary = trait not implemented.
MIR_E_TRAIT_NOT_IMPLEMENTED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_TRAIT_NOT_IMPLEMENTED.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_TRAIT_NOT_IMPLEMENTED.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_TRAIT_NOT_IMPLEMENTED.example = vitte check path/to/file.vit

MIR_E_TRAIT_AMBIGUOUS.summary = trait ambiguous.
MIR_E_TRAIT_AMBIGUOUS.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_TRAIT_AMBIGUOUS.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_TRAIT_AMBIGUOUS.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_TRAIT_AMBIGUOUS.example = vitte check path/to/file.vit

MIR_E_GENERIC_ARGUMENT_MISSING.summary = generisch Argument fehlend.
MIR_E_GENERIC_ARGUMENT_MISSING.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_GENERIC_ARGUMENT_MISSING.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_GENERIC_ARGUMENT_MISSING.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_GENERIC_ARGUMENT_MISSING.example = vitte check path/to/file.vit

MIR_E_GENERIC_BOUND_FAILED.summary = generisch bound fehlgeschlagen.
MIR_E_GENERIC_BOUND_FAILED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_GENERIC_BOUND_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_GENERIC_BOUND_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_GENERIC_BOUND_FAILED.example = vitte check path/to/file.vit

MIR_E_UNSUPPORTED_TARGET.summary = Ziel nicht unterstuetzt.
MIR_E_UNSUPPORTED_TARGET.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_UNSUPPORTED_TARGET.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_UNSUPPORTED_TARGET.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_UNSUPPORTED_TARGET.example = vitte check path/to/file.vit

MIR_E_ABI_MISMATCH.summary = abi Nichtuebereinstimmung.
MIR_E_ABI_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_ABI_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_ABI_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_ABI_MISMATCH.example = vitte check path/to/file.vit

MIR_E_LINK_FAILED.summary = link fehlgeschlagen.
MIR_E_LINK_FAILED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_LINK_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_LINK_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_LINK_FAILED.example = vitte check path/to/file.vit

MIR_E_RUNTIME_PANIC.summary = runtime Panik.
MIR_E_RUNTIME_PANIC.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_RUNTIME_PANIC.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_RUNTIME_PANIC.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_RUNTIME_PANIC.example = vitte check path/to/file.vit

IR_E_EXPECTED_IDENTIFIER.summary = Bezeichner erwartet.
IR_E_EXPECTED_IDENTIFIER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_EXPECTED_IDENTIFIER.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_EXPECTED_IDENTIFIER.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_EXPECTED_IDENTIFIER.example = vitte check path/to/file.vit

IR_E_EXPECTED_EXPRESSION.summary = Ausdruck erwartet.
IR_E_EXPECTED_EXPRESSION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_EXPECTED_EXPRESSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_EXPECTED_EXPRESSION.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_EXPECTED_EXPRESSION.example = vitte check path/to/file.vit

IR_E_EXPECTED_TYPE.summary = Typ erwartet.
IR_E_EXPECTED_TYPE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_EXPECTED_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_EXPECTED_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_EXPECTED_TYPE.example = vitte check path/to/file.vit

IR_E_EXPECTED_PATTERN.summary = Muster erwartet.
IR_E_EXPECTED_PATTERN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_EXPECTED_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_EXPECTED_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_EXPECTED_PATTERN.example = vitte check path/to/file.vit

IR_E_EXPECTED_BLOCK.summary = Block erwartet.
IR_E_EXPECTED_BLOCK.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_EXPECTED_BLOCK.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_EXPECTED_BLOCK.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_EXPECTED_BLOCK.example = vitte check path/to/file.vit

IR_E_EXPECTED_DELIMITER.summary = Trennzeichen erwartet.
IR_E_EXPECTED_DELIMITER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_EXPECTED_DELIMITER.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_EXPECTED_DELIMITER.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_EXPECTED_DELIMITER.example = vitte check path/to/file.vit

IR_E_UNEXPECTED_TOKEN.summary = Token unerwartet.
IR_E_UNEXPECTED_TOKEN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_UNEXPECTED_TOKEN.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_UNEXPECTED_TOKEN.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_UNEXPECTED_TOKEN.example = vitte check path/to/file.vit

IR_E_UNBALANCED_DELIMITER.summary = unbalanced Trennzeichen.
IR_E_UNBALANCED_DELIMITER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_UNBALANCED_DELIMITER.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_UNBALANCED_DELIMITER.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_UNBALANCED_DELIMITER.example = vitte check path/to/file.vit

IR_E_INVALID_ATTRIBUTE.summary = attribute ungueltig.
IR_E_INVALID_ATTRIBUTE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_ATTRIBUTE.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_ATTRIBUTE.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_ATTRIBUTE.example = vitte check path/to/file.vit

IR_E_INVALID_DECLARATION.summary = Deklaration ungueltig.
IR_E_INVALID_DECLARATION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_DECLARATION.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_DECLARATION.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_DECLARATION.example = vitte check path/to/file.vit

IR_E_INVALID_STATEMENT.summary = Anweisung ungueltig.
IR_E_INVALID_STATEMENT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_STATEMENT.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_STATEMENT.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_STATEMENT.example = vitte check path/to/file.vit

IR_E_INVALID_EXPRESSION.summary = Ausdruck ungueltig.
IR_E_INVALID_EXPRESSION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_EXPRESSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_EXPRESSION.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_EXPRESSION.example = vitte check path/to/file.vit

IR_E_INVALID_PATTERN.summary = Muster ungueltig.
IR_E_INVALID_PATTERN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_PATTERN.example = vitte check path/to/file.vit

IR_E_INVALID_LITERAL.summary = Literal ungueltig.
IR_E_INVALID_LITERAL.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_LITERAL.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_LITERAL.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_LITERAL.example = vitte check path/to/file.vit

IR_E_INVALID_OPERATOR.summary = Operator ungueltig.
IR_E_INVALID_OPERATOR.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_OPERATOR.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_OPERATOR.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_OPERATOR.example = vitte check path/to/file.vit

IR_E_INVALID_MODIFIER.summary = modifier ungueltig.
IR_E_INVALID_MODIFIER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_MODIFIER.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_MODIFIER.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_MODIFIER.example = vitte check path/to/file.vit

IR_E_MISSING_BODY.summary = body fehlt.
IR_E_MISSING_BODY.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_MISSING_BODY.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_MISSING_BODY.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_MISSING_BODY.example = vitte check path/to/file.vit

IR_E_MISSING_RETURN.summary = Rueckgabe fehlt.
IR_E_MISSING_RETURN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_MISSING_RETURN.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_MISSING_RETURN.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_MISSING_RETURN.example = vitte check path/to/file.vit

IR_E_DUPLICATE_NAME.summary = Name doppelt.
IR_E_DUPLICATE_NAME.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_DUPLICATE_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_DUPLICATE_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_DUPLICATE_NAME.example = vitte check path/to/file.vit

IR_E_UNKNOWN_NAME.summary = Name unbekannt.
IR_E_UNKNOWN_NAME.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_UNKNOWN_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_UNKNOWN_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_UNKNOWN_NAME.example = vitte check path/to/file.vit

IR_E_UNKNOWN_TYPE.summary = Typ unbekannt.
IR_E_UNKNOWN_TYPE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_UNKNOWN_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_UNKNOWN_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_UNKNOWN_TYPE.example = vitte check path/to/file.vit

IR_E_UNKNOWN_MODULE.summary = Modul unbekannt.
IR_E_UNKNOWN_MODULE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_UNKNOWN_MODULE.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_UNKNOWN_MODULE.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_UNKNOWN_MODULE.example = vitte check path/to/file.vit

IR_E_UNKNOWN_MEMBER.summary = Member unbekannt.
IR_E_UNKNOWN_MEMBER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_UNKNOWN_MEMBER.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_UNKNOWN_MEMBER.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_UNKNOWN_MEMBER.example = vitte check path/to/file.vit

IR_E_AMBIGUOUS_NAME.summary = ambiguous Name.
IR_E_AMBIGUOUS_NAME.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_AMBIGUOUS_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_AMBIGUOUS_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_AMBIGUOUS_NAME.example = vitte check path/to/file.vit

IR_E_PRIVATE_SYMBOL.summary = private Symbol.
IR_E_PRIVATE_SYMBOL.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_PRIVATE_SYMBOL.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_PRIVATE_SYMBOL.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_PRIVATE_SYMBOL.example = vitte check path/to/file.vit

IR_E_IMPORT_NOT_FOUND.summary = Import fehlend.
IR_E_IMPORT_NOT_FOUND.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_IMPORT_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_IMPORT_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_IMPORT_NOT_FOUND.example = vitte check path/to/file.vit

IR_E_IMPORT_CYCLE.summary = Import Zyklus.
IR_E_IMPORT_CYCLE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_IMPORT_CYCLE.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_IMPORT_CYCLE.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_IMPORT_CYCLE.example = vitte check path/to/file.vit

IR_E_EXPORT_CONFLICT.summary = export Konflikt.
IR_E_EXPORT_CONFLICT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_EXPORT_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_EXPORT_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_EXPORT_CONFLICT.example = vitte check path/to/file.vit

IR_E_ARITY_MISMATCH.summary = arity Nichtuebereinstimmung.
IR_E_ARITY_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_ARITY_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_ARITY_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_ARITY_MISMATCH.example = vitte check path/to/file.vit

IR_E_ARGUMENT_MISMATCH.summary = Argument Nichtuebereinstimmung.
IR_E_ARGUMENT_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_ARGUMENT_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_ARGUMENT_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_ARGUMENT_MISMATCH.example = vitte check path/to/file.vit

IR_E_ASSIGNMENT_MISMATCH.summary = Zuweisung Nichtuebereinstimmung.
IR_E_ASSIGNMENT_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_ASSIGNMENT_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_ASSIGNMENT_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_ASSIGNMENT_MISMATCH.example = vitte check path/to/file.vit

IR_E_BRANCH_MISMATCH.summary = branch Nichtuebereinstimmung.
IR_E_BRANCH_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_BRANCH_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_BRANCH_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_BRANCH_MISMATCH.example = vitte check path/to/file.vit

IR_E_INVALID_CALL.summary = Aufruf ungueltig.
IR_E_INVALID_CALL.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_CALL.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_CALL.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_CALL.example = vitte check path/to/file.vit

IR_E_INVALID_CAST.summary = Umwandlung ungueltig.
IR_E_INVALID_CAST.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_CAST.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_CAST.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_CAST.example = vitte check path/to/file.vit

IR_E_INVALID_INDEX.summary = Index ungueltig.
IR_E_INVALID_INDEX.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_INDEX.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_INDEX.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_INDEX.example = vitte check path/to/file.vit

IR_E_INVALID_DEREF.summary = deref ungueltig.
IR_E_INVALID_DEREF.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_DEREF.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_DEREF.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_DEREF.example = vitte check path/to/file.vit

IR_E_INVALID_BORROW.summary = Ausleihe ungueltig.
IR_E_INVALID_BORROW.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_BORROW.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_BORROW.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_BORROW.example = vitte check path/to/file.vit

IR_E_INVALID_MOVE.summary = Verschiebung ungueltig.
IR_E_INVALID_MOVE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_MOVE.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_MOVE.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_MOVE.example = vitte check path/to/file.vit

IR_E_USE_AFTER_MOVE.summary = use after Verschiebung.
IR_E_USE_AFTER_MOVE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_USE_AFTER_MOVE.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_USE_AFTER_MOVE.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_USE_AFTER_MOVE.example = vitte check path/to/file.vit

IR_E_USE_AFTER_DROP.summary = use after Freigabe.
IR_E_USE_AFTER_DROP.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_USE_AFTER_DROP.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_USE_AFTER_DROP.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_USE_AFTER_DROP.example = vitte check path/to/file.vit

IR_E_USE_BEFORE_INIT.summary = use before init.
IR_E_USE_BEFORE_INIT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_USE_BEFORE_INIT.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_USE_BEFORE_INIT.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_USE_BEFORE_INIT.example = vitte check path/to/file.vit

IR_E_DOUBLE_DROP.summary = double Freigabe.
IR_E_DOUBLE_DROP.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_DOUBLE_DROP.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_DOUBLE_DROP.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_DOUBLE_DROP.example = vitte check path/to/file.vit

IR_E_BORROW_CONFLICT.summary = Ausleihe Konflikt.
IR_E_BORROW_CONFLICT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_BORROW_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_BORROW_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_BORROW_CONFLICT.example = vitte check path/to/file.vit

IR_E_MUTABILITY_CONFLICT.summary = mutability Konflikt.
IR_E_MUTABILITY_CONFLICT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_MUTABILITY_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_MUTABILITY_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_MUTABILITY_CONFLICT.example = vitte check path/to/file.vit

IR_E_LIFETIME_TOO_SHORT.summary = Lebensdauer zu short.
IR_E_LIFETIME_TOO_SHORT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_LIFETIME_TOO_SHORT.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_LIFETIME_TOO_SHORT.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_LIFETIME_TOO_SHORT.example = vitte check path/to/file.vit

IR_E_DANGLING_REFERENCE.summary = dangling Referenz.
IR_E_DANGLING_REFERENCE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_DANGLING_REFERENCE.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_DANGLING_REFERENCE.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_DANGLING_REFERENCE.example = vitte check path/to/file.vit

IR_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
IR_E_NON_EXHAUSTIVE_MATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_NON_EXHAUSTIVE_MATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_NON_EXHAUSTIVE_MATCH.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_NON_EXHAUSTIVE_MATCH.example = vitte check path/to/file.vit

IR_E_UNREACHABLE_PATTERN.summary = unerreichbar Muster.
IR_E_UNREACHABLE_PATTERN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_UNREACHABLE_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_UNREACHABLE_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_UNREACHABLE_PATTERN.example = vitte check path/to/file.vit

IR_E_CONST_REQUIRED.summary = const required.
IR_E_CONST_REQUIRED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_CONST_REQUIRED.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_CONST_REQUIRED.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_CONST_REQUIRED.example = vitte check path/to/file.vit

IR_E_CONST_OVERFLOW.summary = const Ueberlauf.
IR_E_CONST_OVERFLOW.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_CONST_OVERFLOW.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_CONST_OVERFLOW.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_CONST_OVERFLOW.example = vitte check path/to/file.vit

IR_E_CONST_DIVISION_BY_ZERO.summary = const Division by Null.
IR_E_CONST_DIVISION_BY_ZERO.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_CONST_DIVISION_BY_ZERO.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_CONST_DIVISION_BY_ZERO.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_CONST_DIVISION_BY_ZERO.example = vitte check path/to/file.vit

IR_E_CONST_CYCLE.summary = const Zyklus.
IR_E_CONST_CYCLE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_CONST_CYCLE.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_CONST_CYCLE.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_CONST_CYCLE.example = vitte check path/to/file.vit

IR_E_MACRO_NOT_FOUND.summary = macro fehlend.
IR_E_MACRO_NOT_FOUND.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_MACRO_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_MACRO_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_MACRO_NOT_FOUND.example = vitte check path/to/file.vit

IR_E_MACRO_RECURSION.summary = macro recursion.
IR_E_MACRO_RECURSION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_MACRO_RECURSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_MACRO_RECURSION.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_MACRO_RECURSION.example = vitte check path/to/file.vit

IR_E_MACRO_EXPANSION_FAILED.summary = macro expansion fehlgeschlagen.
IR_E_MACRO_EXPANSION_FAILED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_MACRO_EXPANSION_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_MACRO_EXPANSION_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_MACRO_EXPANSION_FAILED.example = vitte check path/to/file.vit

IR_E_TRAIT_NOT_IMPLEMENTED.summary = trait not implemented.
IR_E_TRAIT_NOT_IMPLEMENTED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_TRAIT_NOT_IMPLEMENTED.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_TRAIT_NOT_IMPLEMENTED.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_TRAIT_NOT_IMPLEMENTED.example = vitte check path/to/file.vit

IR_E_TRAIT_AMBIGUOUS.summary = trait ambiguous.
IR_E_TRAIT_AMBIGUOUS.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_TRAIT_AMBIGUOUS.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_TRAIT_AMBIGUOUS.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_TRAIT_AMBIGUOUS.example = vitte check path/to/file.vit

IR_E_GENERIC_ARGUMENT_MISSING.summary = generisch Argument fehlend.
IR_E_GENERIC_ARGUMENT_MISSING.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_GENERIC_ARGUMENT_MISSING.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_GENERIC_ARGUMENT_MISSING.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_GENERIC_ARGUMENT_MISSING.example = vitte check path/to/file.vit

IR_E_GENERIC_BOUND_FAILED.summary = generisch bound fehlgeschlagen.
IR_E_GENERIC_BOUND_FAILED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_GENERIC_BOUND_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_GENERIC_BOUND_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_GENERIC_BOUND_FAILED.example = vitte check path/to/file.vit

IR_E_UNSUPPORTED_TARGET.summary = Ziel nicht unterstuetzt.
IR_E_UNSUPPORTED_TARGET.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_UNSUPPORTED_TARGET.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_UNSUPPORTED_TARGET.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_UNSUPPORTED_TARGET.example = vitte check path/to/file.vit

IR_E_ABI_MISMATCH.summary = abi Nichtuebereinstimmung.
IR_E_ABI_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_ABI_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_ABI_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_ABI_MISMATCH.example = vitte check path/to/file.vit

IR_E_LINK_FAILED.summary = link fehlgeschlagen.
IR_E_LINK_FAILED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_LINK_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_LINK_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_LINK_FAILED.example = vitte check path/to/file.vit

IR_E_RUNTIME_PANIC.summary = runtime Panik.
IR_E_RUNTIME_PANIC.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_RUNTIME_PANIC.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_RUNTIME_PANIC.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_RUNTIME_PANIC.example = vitte check path/to/file.vit

BACKEND_E_EXPECTED_IDENTIFIER.summary = Bezeichner erwartet.
BACKEND_E_EXPECTED_IDENTIFIER.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_EXPECTED_IDENTIFIER.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_EXPECTED_IDENTIFIER.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_EXPECTED_IDENTIFIER.example = vitte build app.vit -o app

BACKEND_E_EXPECTED_EXPRESSION.summary = Ausdruck erwartet.
BACKEND_E_EXPECTED_EXPRESSION.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_EXPECTED_EXPRESSION.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_EXPECTED_EXPRESSION.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_EXPECTED_EXPRESSION.example = vitte build app.vit -o app

BACKEND_E_EXPECTED_TYPE.summary = Typ erwartet.
BACKEND_E_EXPECTED_TYPE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_EXPECTED_TYPE.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_EXPECTED_TYPE.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_EXPECTED_TYPE.example = vitte build app.vit -o app

BACKEND_E_EXPECTED_PATTERN.summary = Muster erwartet.
BACKEND_E_EXPECTED_PATTERN.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_EXPECTED_PATTERN.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_EXPECTED_PATTERN.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_EXPECTED_PATTERN.example = vitte build app.vit -o app

BACKEND_E_EXPECTED_BLOCK.summary = Block erwartet.
BACKEND_E_EXPECTED_BLOCK.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_EXPECTED_BLOCK.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_EXPECTED_BLOCK.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_EXPECTED_BLOCK.example = vitte build app.vit -o app

BACKEND_E_EXPECTED_DELIMITER.summary = Trennzeichen erwartet.
BACKEND_E_EXPECTED_DELIMITER.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_EXPECTED_DELIMITER.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_EXPECTED_DELIMITER.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_EXPECTED_DELIMITER.example = vitte build app.vit -o app

BACKEND_E_UNEXPECTED_TOKEN.summary = Token unerwartet.
BACKEND_E_UNEXPECTED_TOKEN.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_UNEXPECTED_TOKEN.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_UNEXPECTED_TOKEN.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_UNEXPECTED_TOKEN.example = vitte build app.vit -o app

BACKEND_E_UNBALANCED_DELIMITER.summary = unbalanced Trennzeichen.
BACKEND_E_UNBALANCED_DELIMITER.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_UNBALANCED_DELIMITER.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_UNBALANCED_DELIMITER.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_UNBALANCED_DELIMITER.example = vitte build app.vit -o app

BACKEND_E_INVALID_ATTRIBUTE.summary = attribute ungueltig.
BACKEND_E_INVALID_ATTRIBUTE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_ATTRIBUTE.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_ATTRIBUTE.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_ATTRIBUTE.example = vitte build app.vit -o app

BACKEND_E_INVALID_DECLARATION.summary = Deklaration ungueltig.
BACKEND_E_INVALID_DECLARATION.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_DECLARATION.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_DECLARATION.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_DECLARATION.example = vitte build app.vit -o app

BACKEND_E_INVALID_STATEMENT.summary = Anweisung ungueltig.
BACKEND_E_INVALID_STATEMENT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_STATEMENT.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_STATEMENT.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_STATEMENT.example = vitte build app.vit -o app

BACKEND_E_INVALID_EXPRESSION.summary = Ausdruck ungueltig.
BACKEND_E_INVALID_EXPRESSION.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_EXPRESSION.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_EXPRESSION.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_EXPRESSION.example = vitte build app.vit -o app

BACKEND_E_INVALID_PATTERN.summary = Muster ungueltig.
BACKEND_E_INVALID_PATTERN.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_PATTERN.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_PATTERN.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_PATTERN.example = vitte build app.vit -o app

BACKEND_E_INVALID_LITERAL.summary = Literal ungueltig.
BACKEND_E_INVALID_LITERAL.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_LITERAL.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_LITERAL.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_LITERAL.example = vitte build app.vit -o app

BACKEND_E_INVALID_OPERATOR.summary = Operator ungueltig.
BACKEND_E_INVALID_OPERATOR.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_OPERATOR.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_OPERATOR.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_OPERATOR.example = vitte build app.vit -o app

BACKEND_E_INVALID_MODIFIER.summary = modifier ungueltig.
BACKEND_E_INVALID_MODIFIER.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_MODIFIER.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_MODIFIER.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_MODIFIER.example = vitte build app.vit -o app

BACKEND_E_MISSING_BODY.summary = body fehlt.
BACKEND_E_MISSING_BODY.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_MISSING_BODY.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_MISSING_BODY.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_MISSING_BODY.example = vitte build app.vit -o app

BACKEND_E_MISSING_RETURN.summary = Rueckgabe fehlt.
BACKEND_E_MISSING_RETURN.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_MISSING_RETURN.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_MISSING_RETURN.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_MISSING_RETURN.example = vitte build app.vit -o app

BACKEND_E_DUPLICATE_NAME.summary = Name doppelt.
BACKEND_E_DUPLICATE_NAME.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_DUPLICATE_NAME.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_DUPLICATE_NAME.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_DUPLICATE_NAME.example = vitte build app.vit -o app

BACKEND_E_UNKNOWN_NAME.summary = Name unbekannt.
BACKEND_E_UNKNOWN_NAME.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_UNKNOWN_NAME.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_UNKNOWN_NAME.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_UNKNOWN_NAME.example = vitte build app.vit -o app

BACKEND_E_UNKNOWN_TYPE.summary = Typ unbekannt.
BACKEND_E_UNKNOWN_TYPE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_UNKNOWN_TYPE.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_UNKNOWN_TYPE.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_UNKNOWN_TYPE.example = vitte build app.vit -o app

BACKEND_E_UNKNOWN_MODULE.summary = Modul unbekannt.
BACKEND_E_UNKNOWN_MODULE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_UNKNOWN_MODULE.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_UNKNOWN_MODULE.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_UNKNOWN_MODULE.example = vitte build app.vit -o app

BACKEND_E_UNKNOWN_MEMBER.summary = Member unbekannt.
BACKEND_E_UNKNOWN_MEMBER.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_UNKNOWN_MEMBER.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_UNKNOWN_MEMBER.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_UNKNOWN_MEMBER.example = vitte build app.vit -o app

BACKEND_E_AMBIGUOUS_NAME.summary = ambiguous Name.
BACKEND_E_AMBIGUOUS_NAME.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_AMBIGUOUS_NAME.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_AMBIGUOUS_NAME.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_AMBIGUOUS_NAME.example = vitte build app.vit -o app

BACKEND_E_PRIVATE_SYMBOL.summary = private Symbol.
BACKEND_E_PRIVATE_SYMBOL.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_PRIVATE_SYMBOL.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_PRIVATE_SYMBOL.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_PRIVATE_SYMBOL.example = vitte build app.vit -o app

BACKEND_E_IMPORT_NOT_FOUND.summary = Import fehlend.
BACKEND_E_IMPORT_NOT_FOUND.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_IMPORT_NOT_FOUND.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_IMPORT_NOT_FOUND.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_IMPORT_NOT_FOUND.example = vitte build app.vit -o app

BACKEND_E_IMPORT_CYCLE.summary = Import Zyklus.
BACKEND_E_IMPORT_CYCLE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_IMPORT_CYCLE.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_IMPORT_CYCLE.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_IMPORT_CYCLE.example = vitte build app.vit -o app

BACKEND_E_EXPORT_CONFLICT.summary = export Konflikt.
BACKEND_E_EXPORT_CONFLICT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_EXPORT_CONFLICT.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_EXPORT_CONFLICT.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_EXPORT_CONFLICT.example = vitte build app.vit -o app

BACKEND_E_ARITY_MISMATCH.summary = arity Nichtuebereinstimmung.
BACKEND_E_ARITY_MISMATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_ARITY_MISMATCH.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_ARITY_MISMATCH.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_ARITY_MISMATCH.example = vitte build app.vit -o app

BACKEND_E_ARGUMENT_MISMATCH.summary = Argument Nichtuebereinstimmung.
BACKEND_E_ARGUMENT_MISMATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_ARGUMENT_MISMATCH.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_ARGUMENT_MISMATCH.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_ARGUMENT_MISMATCH.example = vitte build app.vit -o app

BACKEND_E_ASSIGNMENT_MISMATCH.summary = Zuweisung Nichtuebereinstimmung.
BACKEND_E_ASSIGNMENT_MISMATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_ASSIGNMENT_MISMATCH.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_ASSIGNMENT_MISMATCH.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_ASSIGNMENT_MISMATCH.example = vitte build app.vit -o app

BACKEND_E_BRANCH_MISMATCH.summary = branch Nichtuebereinstimmung.
BACKEND_E_BRANCH_MISMATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_BRANCH_MISMATCH.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_BRANCH_MISMATCH.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_BRANCH_MISMATCH.example = vitte build app.vit -o app

BACKEND_E_INVALID_CALL.summary = Aufruf ungueltig.
BACKEND_E_INVALID_CALL.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_CALL.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_CALL.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_CALL.example = vitte build app.vit -o app

BACKEND_E_INVALID_CAST.summary = Umwandlung ungueltig.
BACKEND_E_INVALID_CAST.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_CAST.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_CAST.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_CAST.example = vitte build app.vit -o app

BACKEND_E_INVALID_INDEX.summary = Index ungueltig.
BACKEND_E_INVALID_INDEX.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_INDEX.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_INDEX.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_INDEX.example = vitte build app.vit -o app

BACKEND_E_INVALID_DEREF.summary = deref ungueltig.
BACKEND_E_INVALID_DEREF.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_DEREF.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_DEREF.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_DEREF.example = vitte build app.vit -o app

BACKEND_E_INVALID_BORROW.summary = Ausleihe ungueltig.
BACKEND_E_INVALID_BORROW.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_BORROW.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_BORROW.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_BORROW.example = vitte build app.vit -o app

BACKEND_E_INVALID_MOVE.summary = Verschiebung ungueltig.
BACKEND_E_INVALID_MOVE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_MOVE.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_MOVE.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_MOVE.example = vitte build app.vit -o app

BACKEND_E_USE_AFTER_MOVE.summary = use after Verschiebung.
BACKEND_E_USE_AFTER_MOVE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_USE_AFTER_MOVE.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_USE_AFTER_MOVE.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_USE_AFTER_MOVE.example = vitte build app.vit -o app

BACKEND_E_USE_AFTER_DROP.summary = use after Freigabe.
BACKEND_E_USE_AFTER_DROP.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_USE_AFTER_DROP.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_USE_AFTER_DROP.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_USE_AFTER_DROP.example = vitte build app.vit -o app

BACKEND_E_USE_BEFORE_INIT.summary = use before init.
BACKEND_E_USE_BEFORE_INIT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_USE_BEFORE_INIT.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_USE_BEFORE_INIT.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_USE_BEFORE_INIT.example = vitte build app.vit -o app

BACKEND_E_DOUBLE_DROP.summary = double Freigabe.
BACKEND_E_DOUBLE_DROP.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_DOUBLE_DROP.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_DOUBLE_DROP.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_DOUBLE_DROP.example = vitte build app.vit -o app

BACKEND_E_BORROW_CONFLICT.summary = Ausleihe Konflikt.
BACKEND_E_BORROW_CONFLICT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_BORROW_CONFLICT.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_BORROW_CONFLICT.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_BORROW_CONFLICT.example = vitte build app.vit -o app

BACKEND_E_MUTABILITY_CONFLICT.summary = mutability Konflikt.
BACKEND_E_MUTABILITY_CONFLICT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_MUTABILITY_CONFLICT.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_MUTABILITY_CONFLICT.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_MUTABILITY_CONFLICT.example = vitte build app.vit -o app

BACKEND_E_LIFETIME_TOO_SHORT.summary = Lebensdauer zu short.
BACKEND_E_LIFETIME_TOO_SHORT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_LIFETIME_TOO_SHORT.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_LIFETIME_TOO_SHORT.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_LIFETIME_TOO_SHORT.example = vitte build app.vit -o app

BACKEND_E_DANGLING_REFERENCE.summary = dangling Referenz.
BACKEND_E_DANGLING_REFERENCE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_DANGLING_REFERENCE.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_DANGLING_REFERENCE.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_DANGLING_REFERENCE.example = vitte build app.vit -o app

BACKEND_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
BACKEND_E_NON_EXHAUSTIVE_MATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_NON_EXHAUSTIVE_MATCH.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_NON_EXHAUSTIVE_MATCH.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_NON_EXHAUSTIVE_MATCH.example = vitte build app.vit -o app

BACKEND_E_UNREACHABLE_PATTERN.summary = unerreichbar Muster.
BACKEND_E_UNREACHABLE_PATTERN.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_UNREACHABLE_PATTERN.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_UNREACHABLE_PATTERN.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_UNREACHABLE_PATTERN.example = vitte build app.vit -o app

BACKEND_E_CONST_REQUIRED.summary = const required.
BACKEND_E_CONST_REQUIRED.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_CONST_REQUIRED.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_CONST_REQUIRED.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_CONST_REQUIRED.example = vitte build app.vit -o app

BACKEND_E_CONST_OVERFLOW.summary = const Ueberlauf.
BACKEND_E_CONST_OVERFLOW.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_CONST_OVERFLOW.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_CONST_OVERFLOW.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_CONST_OVERFLOW.example = vitte build app.vit -o app

BACKEND_E_CONST_DIVISION_BY_ZERO.summary = const Division by Null.
BACKEND_E_CONST_DIVISION_BY_ZERO.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_CONST_DIVISION_BY_ZERO.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_CONST_DIVISION_BY_ZERO.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_CONST_DIVISION_BY_ZERO.example = vitte build app.vit -o app

BACKEND_E_CONST_CYCLE.summary = const Zyklus.
BACKEND_E_CONST_CYCLE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_CONST_CYCLE.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_CONST_CYCLE.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_CONST_CYCLE.example = vitte build app.vit -o app

BACKEND_E_MACRO_NOT_FOUND.summary = macro fehlend.
BACKEND_E_MACRO_NOT_FOUND.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_MACRO_NOT_FOUND.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_MACRO_NOT_FOUND.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_MACRO_NOT_FOUND.example = vitte build app.vit -o app

BACKEND_E_MACRO_RECURSION.summary = macro recursion.
BACKEND_E_MACRO_RECURSION.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_MACRO_RECURSION.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_MACRO_RECURSION.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_MACRO_RECURSION.example = vitte build app.vit -o app

BACKEND_E_MACRO_EXPANSION_FAILED.summary = macro expansion fehlgeschlagen.
BACKEND_E_MACRO_EXPANSION_FAILED.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_MACRO_EXPANSION_FAILED.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_MACRO_EXPANSION_FAILED.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_MACRO_EXPANSION_FAILED.example = vitte build app.vit -o app

BACKEND_E_TRAIT_NOT_IMPLEMENTED.summary = trait not implemented.
BACKEND_E_TRAIT_NOT_IMPLEMENTED.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_TRAIT_NOT_IMPLEMENTED.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_TRAIT_NOT_IMPLEMENTED.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_TRAIT_NOT_IMPLEMENTED.example = vitte build app.vit -o app

BACKEND_E_TRAIT_AMBIGUOUS.summary = trait ambiguous.
BACKEND_E_TRAIT_AMBIGUOUS.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_TRAIT_AMBIGUOUS.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_TRAIT_AMBIGUOUS.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_TRAIT_AMBIGUOUS.example = vitte build app.vit -o app

BACKEND_E_GENERIC_ARGUMENT_MISSING.summary = generisch Argument fehlend.
BACKEND_E_GENERIC_ARGUMENT_MISSING.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_GENERIC_ARGUMENT_MISSING.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_GENERIC_ARGUMENT_MISSING.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_GENERIC_ARGUMENT_MISSING.example = vitte build app.vit -o app

BACKEND_E_GENERIC_BOUND_FAILED.summary = generisch bound fehlgeschlagen.
BACKEND_E_GENERIC_BOUND_FAILED.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_GENERIC_BOUND_FAILED.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_GENERIC_BOUND_FAILED.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_GENERIC_BOUND_FAILED.example = vitte build app.vit -o app

BACKEND_E_LINK_FAILED.summary = link fehlgeschlagen.
BACKEND_E_LINK_FAILED.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_LINK_FAILED.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_LINK_FAILED.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_LINK_FAILED.example = vitte build app.vit -o app

BACKEND_E_RUNTIME_PANIC.summary = runtime Panik.
BACKEND_E_RUNTIME_PANIC.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_RUNTIME_PANIC.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_RUNTIME_PANIC.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_RUNTIME_PANIC.example = vitte build app.vit -o app

LINK_E_EXPECTED_IDENTIFIER.summary = Bezeichner erwartet.
LINK_E_EXPECTED_IDENTIFIER.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_EXPECTED_IDENTIFIER.step1 = Check the target triple and the first backend or linker note.
LINK_E_EXPECTED_IDENTIFIER.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_EXPECTED_IDENTIFIER.example = vitte build app.vit -o app

LINK_E_EXPECTED_EXPRESSION.summary = Ausdruck erwartet.
LINK_E_EXPECTED_EXPRESSION.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_EXPECTED_EXPRESSION.step1 = Check the target triple and the first backend or linker note.
LINK_E_EXPECTED_EXPRESSION.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_EXPECTED_EXPRESSION.example = vitte build app.vit -o app

LINK_E_EXPECTED_TYPE.summary = Typ erwartet.
LINK_E_EXPECTED_TYPE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_EXPECTED_TYPE.step1 = Check the target triple and the first backend or linker note.
LINK_E_EXPECTED_TYPE.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_EXPECTED_TYPE.example = vitte build app.vit -o app

LINK_E_EXPECTED_PATTERN.summary = Muster erwartet.
LINK_E_EXPECTED_PATTERN.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_EXPECTED_PATTERN.step1 = Check the target triple and the first backend or linker note.
LINK_E_EXPECTED_PATTERN.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_EXPECTED_PATTERN.example = vitte build app.vit -o app

LINK_E_EXPECTED_BLOCK.summary = Block erwartet.
LINK_E_EXPECTED_BLOCK.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_EXPECTED_BLOCK.step1 = Check the target triple and the first backend or linker note.
LINK_E_EXPECTED_BLOCK.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_EXPECTED_BLOCK.example = vitte build app.vit -o app

LINK_E_EXPECTED_DELIMITER.summary = Trennzeichen erwartet.
LINK_E_EXPECTED_DELIMITER.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_EXPECTED_DELIMITER.step1 = Check the target triple and the first backend or linker note.
LINK_E_EXPECTED_DELIMITER.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_EXPECTED_DELIMITER.example = vitte build app.vit -o app

LINK_E_UNEXPECTED_TOKEN.summary = Token unerwartet.
LINK_E_UNEXPECTED_TOKEN.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_UNEXPECTED_TOKEN.step1 = Check the target triple and the first backend or linker note.
LINK_E_UNEXPECTED_TOKEN.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_UNEXPECTED_TOKEN.example = vitte build app.vit -o app

LINK_E_UNBALANCED_DELIMITER.summary = unbalanced Trennzeichen.
LINK_E_UNBALANCED_DELIMITER.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_UNBALANCED_DELIMITER.step1 = Check the target triple and the first backend or linker note.
LINK_E_UNBALANCED_DELIMITER.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_UNBALANCED_DELIMITER.example = vitte build app.vit -o app

LINK_E_INVALID_ATTRIBUTE.summary = attribute ungueltig.
LINK_E_INVALID_ATTRIBUTE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_ATTRIBUTE.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_ATTRIBUTE.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_ATTRIBUTE.example = vitte build app.vit -o app

LINK_E_INVALID_DECLARATION.summary = Deklaration ungueltig.
LINK_E_INVALID_DECLARATION.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_DECLARATION.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_DECLARATION.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_DECLARATION.example = vitte build app.vit -o app

LINK_E_INVALID_STATEMENT.summary = Anweisung ungueltig.
LINK_E_INVALID_STATEMENT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_STATEMENT.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_STATEMENT.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_STATEMENT.example = vitte build app.vit -o app

LINK_E_INVALID_EXPRESSION.summary = Ausdruck ungueltig.
LINK_E_INVALID_EXPRESSION.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_EXPRESSION.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_EXPRESSION.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_EXPRESSION.example = vitte build app.vit -o app

LINK_E_INVALID_PATTERN.summary = Muster ungueltig.
LINK_E_INVALID_PATTERN.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_PATTERN.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_PATTERN.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_PATTERN.example = vitte build app.vit -o app

LINK_E_INVALID_LITERAL.summary = Literal ungueltig.
LINK_E_INVALID_LITERAL.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_LITERAL.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_LITERAL.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_LITERAL.example = vitte build app.vit -o app

LINK_E_INVALID_OPERATOR.summary = Operator ungueltig.
LINK_E_INVALID_OPERATOR.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_OPERATOR.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_OPERATOR.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_OPERATOR.example = vitte build app.vit -o app

LINK_E_INVALID_MODIFIER.summary = modifier ungueltig.
LINK_E_INVALID_MODIFIER.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_MODIFIER.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_MODIFIER.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_MODIFIER.example = vitte build app.vit -o app

LINK_E_MISSING_BODY.summary = body fehlt.
LINK_E_MISSING_BODY.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_MISSING_BODY.step1 = Check the target triple and the first backend or linker note.
LINK_E_MISSING_BODY.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_MISSING_BODY.example = vitte build app.vit -o app

LINK_E_MISSING_RETURN.summary = Rueckgabe fehlt.
LINK_E_MISSING_RETURN.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_MISSING_RETURN.step1 = Check the target triple and the first backend or linker note.
LINK_E_MISSING_RETURN.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_MISSING_RETURN.example = vitte build app.vit -o app

LINK_E_DUPLICATE_NAME.summary = Name doppelt.
LINK_E_DUPLICATE_NAME.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_DUPLICATE_NAME.step1 = Check the target triple and the first backend or linker note.
LINK_E_DUPLICATE_NAME.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_DUPLICATE_NAME.example = vitte build app.vit -o app

LINK_E_UNKNOWN_NAME.summary = Name unbekannt.
LINK_E_UNKNOWN_NAME.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_UNKNOWN_NAME.step1 = Check the target triple and the first backend or linker note.
LINK_E_UNKNOWN_NAME.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_UNKNOWN_NAME.example = vitte build app.vit -o app

LINK_E_UNKNOWN_TYPE.summary = Typ unbekannt.
LINK_E_UNKNOWN_TYPE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_UNKNOWN_TYPE.step1 = Check the target triple and the first backend or linker note.
LINK_E_UNKNOWN_TYPE.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_UNKNOWN_TYPE.example = vitte build app.vit -o app

LINK_E_UNKNOWN_MODULE.summary = Modul unbekannt.
LINK_E_UNKNOWN_MODULE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_UNKNOWN_MODULE.step1 = Check the target triple and the first backend or linker note.
LINK_E_UNKNOWN_MODULE.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_UNKNOWN_MODULE.example = vitte build app.vit -o app

LINK_E_UNKNOWN_MEMBER.summary = Member unbekannt.
LINK_E_UNKNOWN_MEMBER.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_UNKNOWN_MEMBER.step1 = Check the target triple and the first backend or linker note.
LINK_E_UNKNOWN_MEMBER.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_UNKNOWN_MEMBER.example = vitte build app.vit -o app

LINK_E_AMBIGUOUS_NAME.summary = ambiguous Name.
LINK_E_AMBIGUOUS_NAME.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_AMBIGUOUS_NAME.step1 = Check the target triple and the first backend or linker note.
LINK_E_AMBIGUOUS_NAME.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_AMBIGUOUS_NAME.example = vitte build app.vit -o app

LINK_E_PRIVATE_SYMBOL.summary = private Symbol.
LINK_E_PRIVATE_SYMBOL.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_PRIVATE_SYMBOL.step1 = Check the target triple and the first backend or linker note.
LINK_E_PRIVATE_SYMBOL.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_PRIVATE_SYMBOL.example = vitte build app.vit -o app

LINK_E_IMPORT_NOT_FOUND.summary = Import fehlend.
LINK_E_IMPORT_NOT_FOUND.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_IMPORT_NOT_FOUND.step1 = Check the target triple and the first backend or linker note.
LINK_E_IMPORT_NOT_FOUND.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_IMPORT_NOT_FOUND.example = vitte build app.vit -o app

LINK_E_IMPORT_CYCLE.summary = Import Zyklus.
LINK_E_IMPORT_CYCLE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_IMPORT_CYCLE.step1 = Check the target triple and the first backend or linker note.
LINK_E_IMPORT_CYCLE.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_IMPORT_CYCLE.example = vitte build app.vit -o app

LINK_E_EXPORT_CONFLICT.summary = export Konflikt.
LINK_E_EXPORT_CONFLICT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_EXPORT_CONFLICT.step1 = Check the target triple and the first backend or linker note.
LINK_E_EXPORT_CONFLICT.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_EXPORT_CONFLICT.example = vitte build app.vit -o app

LINK_E_ARITY_MISMATCH.summary = arity Nichtuebereinstimmung.
LINK_E_ARITY_MISMATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_ARITY_MISMATCH.step1 = Check the target triple and the first backend or linker note.
LINK_E_ARITY_MISMATCH.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_ARITY_MISMATCH.example = vitte build app.vit -o app

LINK_E_ARGUMENT_MISMATCH.summary = Argument Nichtuebereinstimmung.
LINK_E_ARGUMENT_MISMATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_ARGUMENT_MISMATCH.step1 = Check the target triple and the first backend or linker note.
LINK_E_ARGUMENT_MISMATCH.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_ARGUMENT_MISMATCH.example = vitte build app.vit -o app

LINK_E_ASSIGNMENT_MISMATCH.summary = Zuweisung Nichtuebereinstimmung.
LINK_E_ASSIGNMENT_MISMATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_ASSIGNMENT_MISMATCH.step1 = Check the target triple and the first backend or linker note.
LINK_E_ASSIGNMENT_MISMATCH.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_ASSIGNMENT_MISMATCH.example = vitte build app.vit -o app

LINK_E_BRANCH_MISMATCH.summary = branch Nichtuebereinstimmung.
LINK_E_BRANCH_MISMATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_BRANCH_MISMATCH.step1 = Check the target triple and the first backend or linker note.
LINK_E_BRANCH_MISMATCH.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_BRANCH_MISMATCH.example = vitte build app.vit -o app

LINK_E_INVALID_CALL.summary = Aufruf ungueltig.
LINK_E_INVALID_CALL.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_CALL.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_CALL.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_CALL.example = vitte build app.vit -o app

LINK_E_INVALID_CAST.summary = Umwandlung ungueltig.
LINK_E_INVALID_CAST.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_CAST.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_CAST.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_CAST.example = vitte build app.vit -o app

LINK_E_INVALID_INDEX.summary = Index ungueltig.
LINK_E_INVALID_INDEX.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_INDEX.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_INDEX.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_INDEX.example = vitte build app.vit -o app

LINK_E_INVALID_DEREF.summary = deref ungueltig.
LINK_E_INVALID_DEREF.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_DEREF.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_DEREF.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_DEREF.example = vitte build app.vit -o app

LINK_E_INVALID_BORROW.summary = Ausleihe ungueltig.
LINK_E_INVALID_BORROW.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_BORROW.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_BORROW.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_BORROW.example = vitte build app.vit -o app

LINK_E_INVALID_MOVE.summary = Verschiebung ungueltig.
LINK_E_INVALID_MOVE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_MOVE.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_MOVE.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_MOVE.example = vitte build app.vit -o app

LINK_E_USE_AFTER_MOVE.summary = use after Verschiebung.
LINK_E_USE_AFTER_MOVE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_USE_AFTER_MOVE.step1 = Check the target triple and the first backend or linker note.
LINK_E_USE_AFTER_MOVE.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_USE_AFTER_MOVE.example = vitte build app.vit -o app

LINK_E_USE_AFTER_DROP.summary = use after Freigabe.
LINK_E_USE_AFTER_DROP.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_USE_AFTER_DROP.step1 = Check the target triple and the first backend or linker note.
LINK_E_USE_AFTER_DROP.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_USE_AFTER_DROP.example = vitte build app.vit -o app

LINK_E_USE_BEFORE_INIT.summary = use before init.
LINK_E_USE_BEFORE_INIT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_USE_BEFORE_INIT.step1 = Check the target triple and the first backend or linker note.
LINK_E_USE_BEFORE_INIT.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_USE_BEFORE_INIT.example = vitte build app.vit -o app

LINK_E_DOUBLE_DROP.summary = double Freigabe.
LINK_E_DOUBLE_DROP.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_DOUBLE_DROP.step1 = Check the target triple and the first backend or linker note.
LINK_E_DOUBLE_DROP.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_DOUBLE_DROP.example = vitte build app.vit -o app

LINK_E_BORROW_CONFLICT.summary = Ausleihe Konflikt.
LINK_E_BORROW_CONFLICT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_BORROW_CONFLICT.step1 = Check the target triple and the first backend or linker note.
LINK_E_BORROW_CONFLICT.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_BORROW_CONFLICT.example = vitte build app.vit -o app

LINK_E_MUTABILITY_CONFLICT.summary = mutability Konflikt.
LINK_E_MUTABILITY_CONFLICT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_MUTABILITY_CONFLICT.step1 = Check the target triple and the first backend or linker note.
LINK_E_MUTABILITY_CONFLICT.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_MUTABILITY_CONFLICT.example = vitte build app.vit -o app

LINK_E_LIFETIME_TOO_SHORT.summary = Lebensdauer zu short.
LINK_E_LIFETIME_TOO_SHORT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_LIFETIME_TOO_SHORT.step1 = Check the target triple and the first backend or linker note.
LINK_E_LIFETIME_TOO_SHORT.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_LIFETIME_TOO_SHORT.example = vitte build app.vit -o app

LINK_E_DANGLING_REFERENCE.summary = dangling Referenz.
LINK_E_DANGLING_REFERENCE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_DANGLING_REFERENCE.step1 = Check the target triple and the first backend or linker note.
LINK_E_DANGLING_REFERENCE.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_DANGLING_REFERENCE.example = vitte build app.vit -o app

LINK_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
LINK_E_NON_EXHAUSTIVE_MATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_NON_EXHAUSTIVE_MATCH.step1 = Check the target triple and the first backend or linker note.
LINK_E_NON_EXHAUSTIVE_MATCH.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_NON_EXHAUSTIVE_MATCH.example = vitte build app.vit -o app

LINK_E_UNREACHABLE_PATTERN.summary = unerreichbar Muster.
LINK_E_UNREACHABLE_PATTERN.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_UNREACHABLE_PATTERN.step1 = Check the target triple and the first backend or linker note.
LINK_E_UNREACHABLE_PATTERN.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_UNREACHABLE_PATTERN.example = vitte build app.vit -o app

LINK_E_CONST_REQUIRED.summary = const required.
LINK_E_CONST_REQUIRED.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_CONST_REQUIRED.step1 = Check the target triple and the first backend or linker note.
LINK_E_CONST_REQUIRED.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_CONST_REQUIRED.example = vitte build app.vit -o app

LINK_E_CONST_OVERFLOW.summary = const Ueberlauf.
LINK_E_CONST_OVERFLOW.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_CONST_OVERFLOW.step1 = Check the target triple and the first backend or linker note.
LINK_E_CONST_OVERFLOW.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_CONST_OVERFLOW.example = vitte build app.vit -o app

LINK_E_CONST_DIVISION_BY_ZERO.summary = const Division by Null.
LINK_E_CONST_DIVISION_BY_ZERO.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_CONST_DIVISION_BY_ZERO.step1 = Check the target triple and the first backend or linker note.
LINK_E_CONST_DIVISION_BY_ZERO.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_CONST_DIVISION_BY_ZERO.example = vitte build app.vit -o app

LINK_E_CONST_CYCLE.summary = const Zyklus.
LINK_E_CONST_CYCLE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_CONST_CYCLE.step1 = Check the target triple and the first backend or linker note.
LINK_E_CONST_CYCLE.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_CONST_CYCLE.example = vitte build app.vit -o app

LINK_E_MACRO_NOT_FOUND.summary = macro fehlend.
LINK_E_MACRO_NOT_FOUND.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_MACRO_NOT_FOUND.step1 = Check the target triple and the first backend or linker note.
LINK_E_MACRO_NOT_FOUND.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_MACRO_NOT_FOUND.example = vitte build app.vit -o app

LINK_E_MACRO_RECURSION.summary = macro recursion.
LINK_E_MACRO_RECURSION.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_MACRO_RECURSION.step1 = Check the target triple and the first backend or linker note.
LINK_E_MACRO_RECURSION.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_MACRO_RECURSION.example = vitte build app.vit -o app

LINK_E_MACRO_EXPANSION_FAILED.summary = macro expansion fehlgeschlagen.
LINK_E_MACRO_EXPANSION_FAILED.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_MACRO_EXPANSION_FAILED.step1 = Check the target triple and the first backend or linker note.
LINK_E_MACRO_EXPANSION_FAILED.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_MACRO_EXPANSION_FAILED.example = vitte build app.vit -o app

LINK_E_TRAIT_NOT_IMPLEMENTED.summary = trait not implemented.
LINK_E_TRAIT_NOT_IMPLEMENTED.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_TRAIT_NOT_IMPLEMENTED.step1 = Check the target triple and the first backend or linker note.
LINK_E_TRAIT_NOT_IMPLEMENTED.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_TRAIT_NOT_IMPLEMENTED.example = vitte build app.vit -o app

LINK_E_TRAIT_AMBIGUOUS.summary = trait ambiguous.
LINK_E_TRAIT_AMBIGUOUS.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_TRAIT_AMBIGUOUS.step1 = Check the target triple and the first backend or linker note.
LINK_E_TRAIT_AMBIGUOUS.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_TRAIT_AMBIGUOUS.example = vitte build app.vit -o app

LINK_E_GENERIC_ARGUMENT_MISSING.summary = generisch Argument fehlend.
LINK_E_GENERIC_ARGUMENT_MISSING.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_GENERIC_ARGUMENT_MISSING.step1 = Check the target triple and the first backend or linker note.
LINK_E_GENERIC_ARGUMENT_MISSING.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_GENERIC_ARGUMENT_MISSING.example = vitte build app.vit -o app

LINK_E_GENERIC_BOUND_FAILED.summary = generisch bound fehlgeschlagen.
LINK_E_GENERIC_BOUND_FAILED.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_GENERIC_BOUND_FAILED.step1 = Check the target triple and the first backend or linker note.
LINK_E_GENERIC_BOUND_FAILED.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_GENERIC_BOUND_FAILED.example = vitte build app.vit -o app

LINK_E_UNSUPPORTED_TARGET.summary = Ziel nicht unterstuetzt.
LINK_E_UNSUPPORTED_TARGET.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_UNSUPPORTED_TARGET.step1 = Check the target triple and the first backend or linker note.
LINK_E_UNSUPPORTED_TARGET.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_UNSUPPORTED_TARGET.example = vitte build app.vit -o app

LINK_E_ABI_MISMATCH.summary = abi Nichtuebereinstimmung.
LINK_E_ABI_MISMATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_ABI_MISMATCH.step1 = Check the target triple and the first backend or linker note.
LINK_E_ABI_MISMATCH.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_ABI_MISMATCH.example = vitte build app.vit -o app

LINK_E_LINK_FAILED.summary = link fehlgeschlagen.
LINK_E_LINK_FAILED.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_LINK_FAILED.step1 = Check the target triple and the first backend or linker note.
LINK_E_LINK_FAILED.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_LINK_FAILED.example = vitte build app.vit -o app

LINK_E_RUNTIME_PANIC.summary = runtime Panik.
LINK_E_RUNTIME_PANIC.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_RUNTIME_PANIC.step1 = Check the target triple and the first backend or linker note.
LINK_E_RUNTIME_PANIC.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_RUNTIME_PANIC.example = vitte build app.vit -o app

RUNTIME_E_EXPECTED_IDENTIFIER.summary = Bezeichner erwartet.
RUNTIME_E_EXPECTED_IDENTIFIER.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_EXPECTED_IDENTIFIER.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_EXPECTED_IDENTIFIER.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_EXPECTED_IDENTIFIER.example = vitte check path/to/file.vit

RUNTIME_E_EXPECTED_EXPRESSION.summary = Ausdruck erwartet.
RUNTIME_E_EXPECTED_EXPRESSION.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_EXPECTED_EXPRESSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_EXPECTED_EXPRESSION.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_EXPECTED_EXPRESSION.example = vitte check path/to/file.vit

RUNTIME_E_EXPECTED_TYPE.summary = Typ erwartet.
RUNTIME_E_EXPECTED_TYPE.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_EXPECTED_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_EXPECTED_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_EXPECTED_TYPE.example = vitte check path/to/file.vit

RUNTIME_E_EXPECTED_PATTERN.summary = Muster erwartet.
RUNTIME_E_EXPECTED_PATTERN.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_EXPECTED_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_EXPECTED_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_EXPECTED_PATTERN.example = vitte check path/to/file.vit

RUNTIME_E_EXPECTED_BLOCK.summary = Block erwartet.
RUNTIME_E_EXPECTED_BLOCK.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_EXPECTED_BLOCK.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_EXPECTED_BLOCK.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_EXPECTED_BLOCK.example = vitte check path/to/file.vit

RUNTIME_E_EXPECTED_DELIMITER.summary = Trennzeichen erwartet.
RUNTIME_E_EXPECTED_DELIMITER.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_EXPECTED_DELIMITER.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_EXPECTED_DELIMITER.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_EXPECTED_DELIMITER.example = vitte check path/to/file.vit

RUNTIME_E_UNEXPECTED_TOKEN.summary = Token unerwartet.
RUNTIME_E_UNEXPECTED_TOKEN.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_UNEXPECTED_TOKEN.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_UNEXPECTED_TOKEN.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_UNEXPECTED_TOKEN.example = vitte check path/to/file.vit

RUNTIME_E_UNBALANCED_DELIMITER.summary = unbalanced Trennzeichen.
RUNTIME_E_UNBALANCED_DELIMITER.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_UNBALANCED_DELIMITER.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_UNBALANCED_DELIMITER.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_UNBALANCED_DELIMITER.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_ATTRIBUTE.summary = attribute ungueltig.
RUNTIME_E_INVALID_ATTRIBUTE.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_ATTRIBUTE.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_ATTRIBUTE.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_ATTRIBUTE.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_DECLARATION.summary = Deklaration ungueltig.
RUNTIME_E_INVALID_DECLARATION.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_DECLARATION.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_DECLARATION.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_DECLARATION.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_STATEMENT.summary = Anweisung ungueltig.
RUNTIME_E_INVALID_STATEMENT.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_STATEMENT.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_STATEMENT.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_STATEMENT.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_EXPRESSION.summary = Ausdruck ungueltig.
RUNTIME_E_INVALID_EXPRESSION.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_EXPRESSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_EXPRESSION.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_EXPRESSION.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_PATTERN.summary = Muster ungueltig.
RUNTIME_E_INVALID_PATTERN.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_PATTERN.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_LITERAL.summary = Literal ungueltig.
RUNTIME_E_INVALID_LITERAL.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_LITERAL.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_LITERAL.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_LITERAL.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_OPERATOR.summary = Operator ungueltig.
RUNTIME_E_INVALID_OPERATOR.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_OPERATOR.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_OPERATOR.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_OPERATOR.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_MODIFIER.summary = modifier ungueltig.
RUNTIME_E_INVALID_MODIFIER.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_MODIFIER.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_MODIFIER.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_MODIFIER.example = vitte check path/to/file.vit

RUNTIME_E_MISSING_BODY.summary = body fehlt.
RUNTIME_E_MISSING_BODY.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_MISSING_BODY.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_MISSING_BODY.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_MISSING_BODY.example = vitte check path/to/file.vit

RUNTIME_E_MISSING_RETURN.summary = Rueckgabe fehlt.
RUNTIME_E_MISSING_RETURN.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_MISSING_RETURN.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_MISSING_RETURN.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_MISSING_RETURN.example = vitte check path/to/file.vit

RUNTIME_E_DUPLICATE_NAME.summary = Name doppelt.
RUNTIME_E_DUPLICATE_NAME.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_DUPLICATE_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_DUPLICATE_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_DUPLICATE_NAME.example = vitte check path/to/file.vit

RUNTIME_E_UNKNOWN_NAME.summary = Name unbekannt.
RUNTIME_E_UNKNOWN_NAME.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_UNKNOWN_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_UNKNOWN_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_UNKNOWN_NAME.example = vitte check path/to/file.vit

RUNTIME_E_UNKNOWN_TYPE.summary = Typ unbekannt.
RUNTIME_E_UNKNOWN_TYPE.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_UNKNOWN_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_UNKNOWN_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_UNKNOWN_TYPE.example = vitte check path/to/file.vit

RUNTIME_E_UNKNOWN_MODULE.summary = Modul unbekannt.
RUNTIME_E_UNKNOWN_MODULE.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_UNKNOWN_MODULE.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_UNKNOWN_MODULE.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_UNKNOWN_MODULE.example = vitte check path/to/file.vit

RUNTIME_E_UNKNOWN_MEMBER.summary = Member unbekannt.
RUNTIME_E_UNKNOWN_MEMBER.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_UNKNOWN_MEMBER.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_UNKNOWN_MEMBER.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_UNKNOWN_MEMBER.example = vitte check path/to/file.vit

RUNTIME_E_AMBIGUOUS_NAME.summary = ambiguous Name.
RUNTIME_E_AMBIGUOUS_NAME.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_AMBIGUOUS_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_AMBIGUOUS_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_AMBIGUOUS_NAME.example = vitte check path/to/file.vit

RUNTIME_E_PRIVATE_SYMBOL.summary = private Symbol.
RUNTIME_E_PRIVATE_SYMBOL.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_PRIVATE_SYMBOL.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_PRIVATE_SYMBOL.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_PRIVATE_SYMBOL.example = vitte check path/to/file.vit

RUNTIME_E_IMPORT_NOT_FOUND.summary = Import fehlend.
RUNTIME_E_IMPORT_NOT_FOUND.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_IMPORT_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_IMPORT_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_IMPORT_NOT_FOUND.example = vitte check path/to/file.vit

RUNTIME_E_IMPORT_CYCLE.summary = Import Zyklus.
RUNTIME_E_IMPORT_CYCLE.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_IMPORT_CYCLE.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_IMPORT_CYCLE.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_IMPORT_CYCLE.example = vitte check path/to/file.vit

RUNTIME_E_EXPORT_CONFLICT.summary = export Konflikt.
RUNTIME_E_EXPORT_CONFLICT.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_EXPORT_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_EXPORT_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_EXPORT_CONFLICT.example = vitte check path/to/file.vit

RUNTIME_E_ARITY_MISMATCH.summary = arity Nichtuebereinstimmung.
RUNTIME_E_ARITY_MISMATCH.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_ARITY_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_ARITY_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_ARITY_MISMATCH.example = vitte check path/to/file.vit

RUNTIME_E_ARGUMENT_MISMATCH.summary = Argument Nichtuebereinstimmung.
RUNTIME_E_ARGUMENT_MISMATCH.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_ARGUMENT_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_ARGUMENT_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_ARGUMENT_MISMATCH.example = vitte check path/to/file.vit

RUNTIME_E_ASSIGNMENT_MISMATCH.summary = Zuweisung Nichtuebereinstimmung.
RUNTIME_E_ASSIGNMENT_MISMATCH.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_ASSIGNMENT_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_ASSIGNMENT_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_ASSIGNMENT_MISMATCH.example = vitte check path/to/file.vit

RUNTIME_E_BRANCH_MISMATCH.summary = branch Nichtuebereinstimmung.
RUNTIME_E_BRANCH_MISMATCH.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_BRANCH_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_BRANCH_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_BRANCH_MISMATCH.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_CALL.summary = Aufruf ungueltig.
RUNTIME_E_INVALID_CALL.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_CALL.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_CALL.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_CALL.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_CAST.summary = Umwandlung ungueltig.
RUNTIME_E_INVALID_CAST.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_CAST.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_CAST.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_CAST.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_INDEX.summary = Index ungueltig.
RUNTIME_E_INVALID_INDEX.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_INDEX.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_INDEX.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_INDEX.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_DEREF.summary = deref ungueltig.
RUNTIME_E_INVALID_DEREF.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_DEREF.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_DEREF.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_DEREF.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_BORROW.summary = Ausleihe ungueltig.
RUNTIME_E_INVALID_BORROW.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_BORROW.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_BORROW.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_BORROW.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_MOVE.summary = Verschiebung ungueltig.
RUNTIME_E_INVALID_MOVE.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_MOVE.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_MOVE.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_MOVE.example = vitte check path/to/file.vit

RUNTIME_E_USE_AFTER_MOVE.summary = use after Verschiebung.
RUNTIME_E_USE_AFTER_MOVE.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_USE_AFTER_MOVE.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_USE_AFTER_MOVE.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_USE_AFTER_MOVE.example = vitte check path/to/file.vit

RUNTIME_E_USE_AFTER_DROP.summary = use after Freigabe.
RUNTIME_E_USE_AFTER_DROP.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_USE_AFTER_DROP.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_USE_AFTER_DROP.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_USE_AFTER_DROP.example = vitte check path/to/file.vit

RUNTIME_E_USE_BEFORE_INIT.summary = use before init.
RUNTIME_E_USE_BEFORE_INIT.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_USE_BEFORE_INIT.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_USE_BEFORE_INIT.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_USE_BEFORE_INIT.example = vitte check path/to/file.vit

RUNTIME_E_DOUBLE_DROP.summary = double Freigabe.
RUNTIME_E_DOUBLE_DROP.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_DOUBLE_DROP.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_DOUBLE_DROP.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_DOUBLE_DROP.example = vitte check path/to/file.vit

RUNTIME_E_BORROW_CONFLICT.summary = Ausleihe Konflikt.
RUNTIME_E_BORROW_CONFLICT.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_BORROW_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_BORROW_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_BORROW_CONFLICT.example = vitte check path/to/file.vit

RUNTIME_E_MUTABILITY_CONFLICT.summary = mutability Konflikt.
RUNTIME_E_MUTABILITY_CONFLICT.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_MUTABILITY_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_MUTABILITY_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_MUTABILITY_CONFLICT.example = vitte check path/to/file.vit

RUNTIME_E_LIFETIME_TOO_SHORT.summary = Lebensdauer zu short.
RUNTIME_E_LIFETIME_TOO_SHORT.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_LIFETIME_TOO_SHORT.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_LIFETIME_TOO_SHORT.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_LIFETIME_TOO_SHORT.example = vitte check path/to/file.vit

RUNTIME_E_DANGLING_REFERENCE.summary = dangling Referenz.
RUNTIME_E_DANGLING_REFERENCE.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_DANGLING_REFERENCE.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_DANGLING_REFERENCE.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_DANGLING_REFERENCE.example = vitte check path/to/file.vit

RUNTIME_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
RUNTIME_E_NON_EXHAUSTIVE_MATCH.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_NON_EXHAUSTIVE_MATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_NON_EXHAUSTIVE_MATCH.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_NON_EXHAUSTIVE_MATCH.example = vitte check path/to/file.vit

RUNTIME_E_UNREACHABLE_PATTERN.summary = unerreichbar Muster.
RUNTIME_E_UNREACHABLE_PATTERN.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_UNREACHABLE_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_UNREACHABLE_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_UNREACHABLE_PATTERN.example = vitte check path/to/file.vit

RUNTIME_E_CONST_REQUIRED.summary = const required.
RUNTIME_E_CONST_REQUIRED.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_CONST_REQUIRED.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_CONST_REQUIRED.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_CONST_REQUIRED.example = vitte check path/to/file.vit

RUNTIME_E_CONST_OVERFLOW.summary = const Ueberlauf.
RUNTIME_E_CONST_OVERFLOW.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_CONST_OVERFLOW.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_CONST_OVERFLOW.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_CONST_OVERFLOW.example = vitte check path/to/file.vit

RUNTIME_E_CONST_DIVISION_BY_ZERO.summary = const Division by Null.
RUNTIME_E_CONST_DIVISION_BY_ZERO.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_CONST_DIVISION_BY_ZERO.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_CONST_DIVISION_BY_ZERO.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_CONST_DIVISION_BY_ZERO.example = vitte check path/to/file.vit

RUNTIME_E_CONST_CYCLE.summary = const Zyklus.
RUNTIME_E_CONST_CYCLE.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_CONST_CYCLE.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_CONST_CYCLE.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_CONST_CYCLE.example = vitte check path/to/file.vit

RUNTIME_E_MACRO_NOT_FOUND.summary = macro fehlend.
RUNTIME_E_MACRO_NOT_FOUND.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_MACRO_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_MACRO_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_MACRO_NOT_FOUND.example = vitte check path/to/file.vit

RUNTIME_E_MACRO_RECURSION.summary = macro recursion.
RUNTIME_E_MACRO_RECURSION.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_MACRO_RECURSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_MACRO_RECURSION.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_MACRO_RECURSION.example = vitte check path/to/file.vit

RUNTIME_E_MACRO_EXPANSION_FAILED.summary = macro expansion fehlgeschlagen.
RUNTIME_E_MACRO_EXPANSION_FAILED.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_MACRO_EXPANSION_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_MACRO_EXPANSION_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_MACRO_EXPANSION_FAILED.example = vitte check path/to/file.vit

RUNTIME_E_TRAIT_NOT_IMPLEMENTED.summary = trait not implemented.
RUNTIME_E_TRAIT_NOT_IMPLEMENTED.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_TRAIT_NOT_IMPLEMENTED.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_TRAIT_NOT_IMPLEMENTED.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_TRAIT_NOT_IMPLEMENTED.example = vitte check path/to/file.vit

RUNTIME_E_TRAIT_AMBIGUOUS.summary = trait ambiguous.
RUNTIME_E_TRAIT_AMBIGUOUS.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_TRAIT_AMBIGUOUS.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_TRAIT_AMBIGUOUS.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_TRAIT_AMBIGUOUS.example = vitte check path/to/file.vit

RUNTIME_E_GENERIC_ARGUMENT_MISSING.summary = generisch Argument fehlend.
RUNTIME_E_GENERIC_ARGUMENT_MISSING.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_GENERIC_ARGUMENT_MISSING.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_GENERIC_ARGUMENT_MISSING.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_GENERIC_ARGUMENT_MISSING.example = vitte check path/to/file.vit

RUNTIME_E_GENERIC_BOUND_FAILED.summary = generisch bound fehlgeschlagen.
RUNTIME_E_GENERIC_BOUND_FAILED.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_GENERIC_BOUND_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_GENERIC_BOUND_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_GENERIC_BOUND_FAILED.example = vitte check path/to/file.vit

RUNTIME_E_UNSUPPORTED_TARGET.summary = Ziel nicht unterstuetzt.
RUNTIME_E_UNSUPPORTED_TARGET.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_UNSUPPORTED_TARGET.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_UNSUPPORTED_TARGET.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_UNSUPPORTED_TARGET.example = vitte check path/to/file.vit

RUNTIME_E_ABI_MISMATCH.summary = abi Nichtuebereinstimmung.
RUNTIME_E_ABI_MISMATCH.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_ABI_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_ABI_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_ABI_MISMATCH.example = vitte check path/to/file.vit

RUNTIME_E_LINK_FAILED.summary = link fehlgeschlagen.
RUNTIME_E_LINK_FAILED.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_LINK_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_LINK_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_LINK_FAILED.example = vitte check path/to/file.vit

RUNTIME_E_RUNTIME_PANIC.summary = runtime Panik.
RUNTIME_E_RUNTIME_PANIC.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_RUNTIME_PANIC.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_RUNTIME_PANIC.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_RUNTIME_PANIC.example = vitte check path/to/file.vit

DRIVER_E_EXPECTED_IDENTIFIER.summary = Bezeichner erwartet.
DRIVER_E_EXPECTED_IDENTIFIER.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_EXPECTED_IDENTIFIER.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_EXPECTED_IDENTIFIER.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_EXPECTED_IDENTIFIER.example = vitte check src/main.vit --lang en

DRIVER_E_EXPECTED_EXPRESSION.summary = Ausdruck erwartet.
DRIVER_E_EXPECTED_EXPRESSION.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_EXPECTED_EXPRESSION.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_EXPECTED_EXPRESSION.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_EXPECTED_EXPRESSION.example = vitte check src/main.vit --lang en

DRIVER_E_EXPECTED_TYPE.summary = Typ erwartet.
DRIVER_E_EXPECTED_TYPE.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_EXPECTED_TYPE.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_EXPECTED_TYPE.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_EXPECTED_TYPE.example = vitte check src/main.vit --lang en

DRIVER_E_EXPECTED_PATTERN.summary = Muster erwartet.
DRIVER_E_EXPECTED_PATTERN.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_EXPECTED_PATTERN.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_EXPECTED_PATTERN.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_EXPECTED_PATTERN.example = vitte check src/main.vit --lang en

DRIVER_E_EXPECTED_BLOCK.summary = Block erwartet.
DRIVER_E_EXPECTED_BLOCK.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_EXPECTED_BLOCK.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_EXPECTED_BLOCK.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_EXPECTED_BLOCK.example = vitte check src/main.vit --lang en

DRIVER_E_EXPECTED_DELIMITER.summary = Trennzeichen erwartet.
DRIVER_E_EXPECTED_DELIMITER.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_EXPECTED_DELIMITER.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_EXPECTED_DELIMITER.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_EXPECTED_DELIMITER.example = vitte check src/main.vit --lang en

DRIVER_E_UNEXPECTED_TOKEN.summary = Token unerwartet.
DRIVER_E_UNEXPECTED_TOKEN.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_UNEXPECTED_TOKEN.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_UNEXPECTED_TOKEN.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_UNEXPECTED_TOKEN.example = vitte check src/main.vit --lang en

DRIVER_E_UNBALANCED_DELIMITER.summary = unbalanced Trennzeichen.
DRIVER_E_UNBALANCED_DELIMITER.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_UNBALANCED_DELIMITER.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_UNBALANCED_DELIMITER.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_UNBALANCED_DELIMITER.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_ATTRIBUTE.summary = attribute ungueltig.
DRIVER_E_INVALID_ATTRIBUTE.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_ATTRIBUTE.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_ATTRIBUTE.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_ATTRIBUTE.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_DECLARATION.summary = Deklaration ungueltig.
DRIVER_E_INVALID_DECLARATION.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_DECLARATION.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_DECLARATION.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_DECLARATION.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_STATEMENT.summary = Anweisung ungueltig.
DRIVER_E_INVALID_STATEMENT.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_STATEMENT.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_STATEMENT.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_STATEMENT.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_EXPRESSION.summary = Ausdruck ungueltig.
DRIVER_E_INVALID_EXPRESSION.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_EXPRESSION.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_EXPRESSION.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_EXPRESSION.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_PATTERN.summary = Muster ungueltig.
DRIVER_E_INVALID_PATTERN.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_PATTERN.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_PATTERN.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_PATTERN.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_LITERAL.summary = Literal ungueltig.
DRIVER_E_INVALID_LITERAL.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_LITERAL.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_LITERAL.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_LITERAL.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_OPERATOR.summary = Operator ungueltig.
DRIVER_E_INVALID_OPERATOR.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_OPERATOR.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_OPERATOR.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_OPERATOR.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_MODIFIER.summary = modifier ungueltig.
DRIVER_E_INVALID_MODIFIER.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_MODIFIER.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_MODIFIER.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_MODIFIER.example = vitte check src/main.vit --lang en

DRIVER_E_MISSING_BODY.summary = body fehlt.
DRIVER_E_MISSING_BODY.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_MISSING_BODY.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_MISSING_BODY.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_MISSING_BODY.example = vitte check src/main.vit --lang en

DRIVER_E_MISSING_RETURN.summary = Rueckgabe fehlt.
DRIVER_E_MISSING_RETURN.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_MISSING_RETURN.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_MISSING_RETURN.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_MISSING_RETURN.example = vitte check src/main.vit --lang en

DRIVER_E_DUPLICATE_NAME.summary = Name doppelt.
DRIVER_E_DUPLICATE_NAME.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_DUPLICATE_NAME.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_DUPLICATE_NAME.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_DUPLICATE_NAME.example = vitte check src/main.vit --lang en

DRIVER_E_UNKNOWN_NAME.summary = Name unbekannt.
DRIVER_E_UNKNOWN_NAME.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_UNKNOWN_NAME.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_UNKNOWN_NAME.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_UNKNOWN_NAME.example = vitte check src/main.vit --lang en

DRIVER_E_UNKNOWN_TYPE.summary = Typ unbekannt.
DRIVER_E_UNKNOWN_TYPE.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_UNKNOWN_TYPE.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_UNKNOWN_TYPE.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_UNKNOWN_TYPE.example = vitte check src/main.vit --lang en

DRIVER_E_UNKNOWN_MODULE.summary = Modul unbekannt.
DRIVER_E_UNKNOWN_MODULE.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_UNKNOWN_MODULE.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_UNKNOWN_MODULE.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_UNKNOWN_MODULE.example = vitte check src/main.vit --lang en

DRIVER_E_UNKNOWN_MEMBER.summary = Member unbekannt.
DRIVER_E_UNKNOWN_MEMBER.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_UNKNOWN_MEMBER.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_UNKNOWN_MEMBER.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_UNKNOWN_MEMBER.example = vitte check src/main.vit --lang en

DRIVER_E_AMBIGUOUS_NAME.summary = ambiguous Name.
DRIVER_E_AMBIGUOUS_NAME.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_AMBIGUOUS_NAME.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_AMBIGUOUS_NAME.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_AMBIGUOUS_NAME.example = vitte check src/main.vit --lang en

DRIVER_E_PRIVATE_SYMBOL.summary = private Symbol.
DRIVER_E_PRIVATE_SYMBOL.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_PRIVATE_SYMBOL.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_PRIVATE_SYMBOL.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_PRIVATE_SYMBOL.example = vitte check src/main.vit --lang en

DRIVER_E_IMPORT_NOT_FOUND.summary = Import fehlend.
DRIVER_E_IMPORT_NOT_FOUND.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_IMPORT_NOT_FOUND.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_IMPORT_NOT_FOUND.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_IMPORT_NOT_FOUND.example = vitte check src/main.vit --lang en

DRIVER_E_IMPORT_CYCLE.summary = Import Zyklus.
DRIVER_E_IMPORT_CYCLE.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_IMPORT_CYCLE.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_IMPORT_CYCLE.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_IMPORT_CYCLE.example = vitte check src/main.vit --lang en

DRIVER_E_EXPORT_CONFLICT.summary = export Konflikt.
DRIVER_E_EXPORT_CONFLICT.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_EXPORT_CONFLICT.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_EXPORT_CONFLICT.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_EXPORT_CONFLICT.example = vitte check src/main.vit --lang en

DRIVER_E_ARITY_MISMATCH.summary = arity Nichtuebereinstimmung.
DRIVER_E_ARITY_MISMATCH.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_ARITY_MISMATCH.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_ARITY_MISMATCH.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_ARITY_MISMATCH.example = vitte check src/main.vit --lang en

DRIVER_E_ARGUMENT_MISMATCH.summary = Argument Nichtuebereinstimmung.
DRIVER_E_ARGUMENT_MISMATCH.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_ARGUMENT_MISMATCH.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_ARGUMENT_MISMATCH.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_ARGUMENT_MISMATCH.example = vitte check src/main.vit --lang en

DRIVER_E_ASSIGNMENT_MISMATCH.summary = Zuweisung Nichtuebereinstimmung.
DRIVER_E_ASSIGNMENT_MISMATCH.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_ASSIGNMENT_MISMATCH.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_ASSIGNMENT_MISMATCH.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_ASSIGNMENT_MISMATCH.example = vitte check src/main.vit --lang en

DRIVER_E_BRANCH_MISMATCH.summary = branch Nichtuebereinstimmung.
DRIVER_E_BRANCH_MISMATCH.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_BRANCH_MISMATCH.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_BRANCH_MISMATCH.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_BRANCH_MISMATCH.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_CALL.summary = Aufruf ungueltig.
DRIVER_E_INVALID_CALL.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_CALL.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_CALL.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_CALL.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_CAST.summary = Umwandlung ungueltig.
DRIVER_E_INVALID_CAST.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_CAST.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_CAST.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_CAST.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_INDEX.summary = Index ungueltig.
DRIVER_E_INVALID_INDEX.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_INDEX.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_INDEX.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_INDEX.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_DEREF.summary = deref ungueltig.
DRIVER_E_INVALID_DEREF.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_DEREF.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_DEREF.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_DEREF.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_BORROW.summary = Ausleihe ungueltig.
DRIVER_E_INVALID_BORROW.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_BORROW.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_BORROW.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_BORROW.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_MOVE.summary = Verschiebung ungueltig.
DRIVER_E_INVALID_MOVE.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_MOVE.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_MOVE.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_MOVE.example = vitte check src/main.vit --lang en

DRIVER_E_USE_AFTER_MOVE.summary = use after Verschiebung.
DRIVER_E_USE_AFTER_MOVE.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_USE_AFTER_MOVE.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_USE_AFTER_MOVE.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_USE_AFTER_MOVE.example = vitte check src/main.vit --lang en

DRIVER_E_USE_AFTER_DROP.summary = use after Freigabe.
DRIVER_E_USE_AFTER_DROP.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_USE_AFTER_DROP.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_USE_AFTER_DROP.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_USE_AFTER_DROP.example = vitte check src/main.vit --lang en

DRIVER_E_USE_BEFORE_INIT.summary = use before init.
DRIVER_E_USE_BEFORE_INIT.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_USE_BEFORE_INIT.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_USE_BEFORE_INIT.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_USE_BEFORE_INIT.example = vitte check src/main.vit --lang en

DRIVER_E_DOUBLE_DROP.summary = double Freigabe.
DRIVER_E_DOUBLE_DROP.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_DOUBLE_DROP.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_DOUBLE_DROP.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_DOUBLE_DROP.example = vitte check src/main.vit --lang en

DRIVER_E_BORROW_CONFLICT.summary = Ausleihe Konflikt.
DRIVER_E_BORROW_CONFLICT.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_BORROW_CONFLICT.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_BORROW_CONFLICT.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_BORROW_CONFLICT.example = vitte check src/main.vit --lang en

DRIVER_E_MUTABILITY_CONFLICT.summary = mutability Konflikt.
DRIVER_E_MUTABILITY_CONFLICT.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_MUTABILITY_CONFLICT.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_MUTABILITY_CONFLICT.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_MUTABILITY_CONFLICT.example = vitte check src/main.vit --lang en

DRIVER_E_LIFETIME_TOO_SHORT.summary = Lebensdauer zu short.
DRIVER_E_LIFETIME_TOO_SHORT.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_LIFETIME_TOO_SHORT.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_LIFETIME_TOO_SHORT.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_LIFETIME_TOO_SHORT.example = vitte check src/main.vit --lang en

DRIVER_E_DANGLING_REFERENCE.summary = dangling Referenz.
DRIVER_E_DANGLING_REFERENCE.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_DANGLING_REFERENCE.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_DANGLING_REFERENCE.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_DANGLING_REFERENCE.example = vitte check src/main.vit --lang en

DRIVER_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
DRIVER_E_NON_EXHAUSTIVE_MATCH.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_NON_EXHAUSTIVE_MATCH.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_NON_EXHAUSTIVE_MATCH.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_NON_EXHAUSTIVE_MATCH.example = vitte check src/main.vit --lang en

DRIVER_E_UNREACHABLE_PATTERN.summary = unerreichbar Muster.
DRIVER_E_UNREACHABLE_PATTERN.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_UNREACHABLE_PATTERN.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_UNREACHABLE_PATTERN.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_UNREACHABLE_PATTERN.example = vitte check src/main.vit --lang en

DRIVER_E_CONST_REQUIRED.summary = const required.
DRIVER_E_CONST_REQUIRED.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_CONST_REQUIRED.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_CONST_REQUIRED.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_CONST_REQUIRED.example = vitte check src/main.vit --lang en

DRIVER_E_CONST_OVERFLOW.summary = const Ueberlauf.
DRIVER_E_CONST_OVERFLOW.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_CONST_OVERFLOW.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_CONST_OVERFLOW.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_CONST_OVERFLOW.example = vitte check src/main.vit --lang en

DRIVER_E_CONST_DIVISION_BY_ZERO.summary = const Division by Null.
DRIVER_E_CONST_DIVISION_BY_ZERO.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_CONST_DIVISION_BY_ZERO.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_CONST_DIVISION_BY_ZERO.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_CONST_DIVISION_BY_ZERO.example = vitte check src/main.vit --lang en

DRIVER_E_CONST_CYCLE.summary = const Zyklus.
DRIVER_E_CONST_CYCLE.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_CONST_CYCLE.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_CONST_CYCLE.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_CONST_CYCLE.example = vitte check src/main.vit --lang en

DRIVER_E_MACRO_NOT_FOUND.summary = macro fehlend.
DRIVER_E_MACRO_NOT_FOUND.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_MACRO_NOT_FOUND.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_MACRO_NOT_FOUND.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_MACRO_NOT_FOUND.example = vitte check src/main.vit --lang en

DRIVER_E_MACRO_RECURSION.summary = macro recursion.
DRIVER_E_MACRO_RECURSION.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_MACRO_RECURSION.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_MACRO_RECURSION.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_MACRO_RECURSION.example = vitte check src/main.vit --lang en

DRIVER_E_MACRO_EXPANSION_FAILED.summary = macro expansion fehlgeschlagen.
DRIVER_E_MACRO_EXPANSION_FAILED.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_MACRO_EXPANSION_FAILED.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_MACRO_EXPANSION_FAILED.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_MACRO_EXPANSION_FAILED.example = vitte check src/main.vit --lang en

DRIVER_E_TRAIT_NOT_IMPLEMENTED.summary = trait not implemented.
DRIVER_E_TRAIT_NOT_IMPLEMENTED.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_TRAIT_NOT_IMPLEMENTED.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_TRAIT_NOT_IMPLEMENTED.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_TRAIT_NOT_IMPLEMENTED.example = vitte check src/main.vit --lang en

DRIVER_E_TRAIT_AMBIGUOUS.summary = trait ambiguous.
DRIVER_E_TRAIT_AMBIGUOUS.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_TRAIT_AMBIGUOUS.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_TRAIT_AMBIGUOUS.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_TRAIT_AMBIGUOUS.example = vitte check src/main.vit --lang en

DRIVER_E_GENERIC_ARGUMENT_MISSING.summary = generisch Argument fehlend.
DRIVER_E_GENERIC_ARGUMENT_MISSING.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_GENERIC_ARGUMENT_MISSING.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_GENERIC_ARGUMENT_MISSING.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_GENERIC_ARGUMENT_MISSING.example = vitte check src/main.vit --lang en

DRIVER_E_GENERIC_BOUND_FAILED.summary = generisch bound fehlgeschlagen.
DRIVER_E_GENERIC_BOUND_FAILED.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_GENERIC_BOUND_FAILED.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_GENERIC_BOUND_FAILED.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_GENERIC_BOUND_FAILED.example = vitte check src/main.vit --lang en

DRIVER_E_UNSUPPORTED_TARGET.summary = Ziel nicht unterstuetzt.
DRIVER_E_UNSUPPORTED_TARGET.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_UNSUPPORTED_TARGET.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_UNSUPPORTED_TARGET.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_UNSUPPORTED_TARGET.example = vitte check src/main.vit --lang en

DRIVER_E_ABI_MISMATCH.summary = abi Nichtuebereinstimmung.
DRIVER_E_ABI_MISMATCH.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_ABI_MISMATCH.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_ABI_MISMATCH.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_ABI_MISMATCH.example = vitte check src/main.vit --lang en

DRIVER_E_LINK_FAILED.summary = link fehlgeschlagen.
DRIVER_E_LINK_FAILED.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_LINK_FAILED.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_LINK_FAILED.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_LINK_FAILED.example = vitte check src/main.vit --lang en

DRIVER_E_RUNTIME_PANIC.summary = runtime Panik.
DRIVER_E_RUNTIME_PANIC.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_RUNTIME_PANIC.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_RUNTIME_PANIC.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_RUNTIME_PANIC.example = vitte check src/main.vit --lang en

LIMIT_E_EXPECTED_IDENTIFIER.summary = Bezeichner erwartet.
LIMIT_E_EXPECTED_IDENTIFIER.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_EXPECTED_IDENTIFIER.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_EXPECTED_IDENTIFIER.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_EXPECTED_IDENTIFIER.example = vitte check src/main.vit

LIMIT_E_EXPECTED_EXPRESSION.summary = Ausdruck erwartet.
LIMIT_E_EXPECTED_EXPRESSION.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_EXPECTED_EXPRESSION.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_EXPECTED_EXPRESSION.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_EXPECTED_EXPRESSION.example = vitte check src/main.vit

LIMIT_E_EXPECTED_TYPE.summary = Typ erwartet.
LIMIT_E_EXPECTED_TYPE.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_EXPECTED_TYPE.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_EXPECTED_TYPE.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_EXPECTED_TYPE.example = vitte check src/main.vit

LIMIT_E_EXPECTED_PATTERN.summary = Muster erwartet.
LIMIT_E_EXPECTED_PATTERN.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_EXPECTED_PATTERN.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_EXPECTED_PATTERN.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_EXPECTED_PATTERN.example = vitte check src/main.vit

LIMIT_E_EXPECTED_BLOCK.summary = Block erwartet.
LIMIT_E_EXPECTED_BLOCK.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_EXPECTED_BLOCK.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_EXPECTED_BLOCK.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_EXPECTED_BLOCK.example = vitte check src/main.vit

LIMIT_E_EXPECTED_DELIMITER.summary = Trennzeichen erwartet.
LIMIT_E_EXPECTED_DELIMITER.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_EXPECTED_DELIMITER.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_EXPECTED_DELIMITER.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_EXPECTED_DELIMITER.example = vitte check src/main.vit

LIMIT_E_UNEXPECTED_TOKEN.summary = Token unerwartet.
LIMIT_E_UNEXPECTED_TOKEN.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_UNEXPECTED_TOKEN.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_UNEXPECTED_TOKEN.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_UNEXPECTED_TOKEN.example = vitte check src/main.vit

LIMIT_E_UNBALANCED_DELIMITER.summary = unbalanced Trennzeichen.
LIMIT_E_UNBALANCED_DELIMITER.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_UNBALANCED_DELIMITER.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_UNBALANCED_DELIMITER.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_UNBALANCED_DELIMITER.example = vitte check src/main.vit

LIMIT_E_INVALID_ATTRIBUTE.summary = attribute ungueltig.
LIMIT_E_INVALID_ATTRIBUTE.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_ATTRIBUTE.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_ATTRIBUTE.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_ATTRIBUTE.example = vitte check src/main.vit

LIMIT_E_INVALID_DECLARATION.summary = Deklaration ungueltig.
LIMIT_E_INVALID_DECLARATION.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_DECLARATION.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_DECLARATION.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_DECLARATION.example = vitte check src/main.vit

LIMIT_E_INVALID_STATEMENT.summary = Anweisung ungueltig.
LIMIT_E_INVALID_STATEMENT.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_STATEMENT.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_STATEMENT.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_STATEMENT.example = vitte check src/main.vit

LIMIT_E_INVALID_EXPRESSION.summary = Ausdruck ungueltig.
LIMIT_E_INVALID_EXPRESSION.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_EXPRESSION.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_EXPRESSION.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_EXPRESSION.example = vitte check src/main.vit

LIMIT_E_INVALID_PATTERN.summary = Muster ungueltig.
LIMIT_E_INVALID_PATTERN.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_PATTERN.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_PATTERN.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_PATTERN.example = vitte check src/main.vit

LIMIT_E_INVALID_LITERAL.summary = Literal ungueltig.
LIMIT_E_INVALID_LITERAL.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_LITERAL.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_LITERAL.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_LITERAL.example = vitte check src/main.vit

LIMIT_E_INVALID_OPERATOR.summary = Operator ungueltig.
LIMIT_E_INVALID_OPERATOR.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_OPERATOR.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_OPERATOR.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_OPERATOR.example = vitte check src/main.vit

LIMIT_E_INVALID_MODIFIER.summary = modifier ungueltig.
LIMIT_E_INVALID_MODIFIER.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_MODIFIER.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_MODIFIER.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_MODIFIER.example = vitte check src/main.vit

LIMIT_E_MISSING_BODY.summary = body fehlt.
LIMIT_E_MISSING_BODY.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_MISSING_BODY.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_MISSING_BODY.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_MISSING_BODY.example = vitte check src/main.vit

LIMIT_E_MISSING_RETURN.summary = Rueckgabe fehlt.
LIMIT_E_MISSING_RETURN.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_MISSING_RETURN.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_MISSING_RETURN.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_MISSING_RETURN.example = vitte check src/main.vit

LIMIT_E_DUPLICATE_NAME.summary = Name doppelt.
LIMIT_E_DUPLICATE_NAME.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_DUPLICATE_NAME.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_DUPLICATE_NAME.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_DUPLICATE_NAME.example = vitte check src/main.vit

LIMIT_E_UNKNOWN_NAME.summary = Name unbekannt.
LIMIT_E_UNKNOWN_NAME.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_UNKNOWN_NAME.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_UNKNOWN_NAME.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_UNKNOWN_NAME.example = vitte check src/main.vit

LIMIT_E_UNKNOWN_TYPE.summary = Typ unbekannt.
LIMIT_E_UNKNOWN_TYPE.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_UNKNOWN_TYPE.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_UNKNOWN_TYPE.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_UNKNOWN_TYPE.example = vitte check src/main.vit

LIMIT_E_UNKNOWN_MODULE.summary = Modul unbekannt.
LIMIT_E_UNKNOWN_MODULE.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_UNKNOWN_MODULE.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_UNKNOWN_MODULE.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_UNKNOWN_MODULE.example = vitte check src/main.vit

LIMIT_E_UNKNOWN_MEMBER.summary = Member unbekannt.
LIMIT_E_UNKNOWN_MEMBER.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_UNKNOWN_MEMBER.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_UNKNOWN_MEMBER.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_UNKNOWN_MEMBER.example = vitte check src/main.vit

LIMIT_E_AMBIGUOUS_NAME.summary = ambiguous Name.
LIMIT_E_AMBIGUOUS_NAME.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_AMBIGUOUS_NAME.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_AMBIGUOUS_NAME.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_AMBIGUOUS_NAME.example = vitte check src/main.vit

LIMIT_E_PRIVATE_SYMBOL.summary = private Symbol.
LIMIT_E_PRIVATE_SYMBOL.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_PRIVATE_SYMBOL.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_PRIVATE_SYMBOL.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_PRIVATE_SYMBOL.example = vitte check src/main.vit

LIMIT_E_IMPORT_NOT_FOUND.summary = Import fehlend.
LIMIT_E_IMPORT_NOT_FOUND.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_IMPORT_NOT_FOUND.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_IMPORT_NOT_FOUND.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_IMPORT_NOT_FOUND.example = vitte check src/main.vit

LIMIT_E_IMPORT_CYCLE.summary = Import Zyklus.
LIMIT_E_IMPORT_CYCLE.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_IMPORT_CYCLE.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_IMPORT_CYCLE.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_IMPORT_CYCLE.example = vitte check src/main.vit

LIMIT_E_EXPORT_CONFLICT.summary = export Konflikt.
LIMIT_E_EXPORT_CONFLICT.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_EXPORT_CONFLICT.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_EXPORT_CONFLICT.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_EXPORT_CONFLICT.example = vitte check src/main.vit

LIMIT_E_ARITY_MISMATCH.summary = arity Nichtuebereinstimmung.
LIMIT_E_ARITY_MISMATCH.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_ARITY_MISMATCH.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_ARITY_MISMATCH.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_ARITY_MISMATCH.example = vitte check src/main.vit

LIMIT_E_ARGUMENT_MISMATCH.summary = Argument Nichtuebereinstimmung.
LIMIT_E_ARGUMENT_MISMATCH.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_ARGUMENT_MISMATCH.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_ARGUMENT_MISMATCH.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_ARGUMENT_MISMATCH.example = vitte check src/main.vit

LIMIT_E_ASSIGNMENT_MISMATCH.summary = Zuweisung Nichtuebereinstimmung.
LIMIT_E_ASSIGNMENT_MISMATCH.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_ASSIGNMENT_MISMATCH.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_ASSIGNMENT_MISMATCH.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_ASSIGNMENT_MISMATCH.example = vitte check src/main.vit

LIMIT_E_BRANCH_MISMATCH.summary = branch Nichtuebereinstimmung.
LIMIT_E_BRANCH_MISMATCH.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_BRANCH_MISMATCH.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_BRANCH_MISMATCH.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_BRANCH_MISMATCH.example = vitte check src/main.vit

LIMIT_E_INVALID_CALL.summary = Aufruf ungueltig.
LIMIT_E_INVALID_CALL.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_CALL.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_CALL.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_CALL.example = vitte check src/main.vit

LIMIT_E_INVALID_CAST.summary = Umwandlung ungueltig.
LIMIT_E_INVALID_CAST.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_CAST.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_CAST.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_CAST.example = vitte check src/main.vit

LIMIT_E_INVALID_INDEX.summary = Index ungueltig.
LIMIT_E_INVALID_INDEX.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_INDEX.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_INDEX.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_INDEX.example = vitte check src/main.vit

LIMIT_E_INVALID_DEREF.summary = deref ungueltig.
LIMIT_E_INVALID_DEREF.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_DEREF.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_DEREF.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_DEREF.example = vitte check src/main.vit

LIMIT_E_INVALID_BORROW.summary = Ausleihe ungueltig.
LIMIT_E_INVALID_BORROW.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_BORROW.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_BORROW.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_BORROW.example = vitte check src/main.vit

LIMIT_E_INVALID_MOVE.summary = Verschiebung ungueltig.
LIMIT_E_INVALID_MOVE.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_MOVE.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_MOVE.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_MOVE.example = vitte check src/main.vit

LIMIT_E_USE_AFTER_MOVE.summary = use after Verschiebung.
LIMIT_E_USE_AFTER_MOVE.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_USE_AFTER_MOVE.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_USE_AFTER_MOVE.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_USE_AFTER_MOVE.example = vitte check src/main.vit

LIMIT_E_USE_AFTER_DROP.summary = use after Freigabe.
LIMIT_E_USE_AFTER_DROP.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_USE_AFTER_DROP.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_USE_AFTER_DROP.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_USE_AFTER_DROP.example = vitte check src/main.vit

LIMIT_E_USE_BEFORE_INIT.summary = use before init.
LIMIT_E_USE_BEFORE_INIT.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_USE_BEFORE_INIT.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_USE_BEFORE_INIT.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_USE_BEFORE_INIT.example = vitte check src/main.vit

LIMIT_E_DOUBLE_DROP.summary = double Freigabe.
LIMIT_E_DOUBLE_DROP.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_DOUBLE_DROP.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_DOUBLE_DROP.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_DOUBLE_DROP.example = vitte check src/main.vit

LIMIT_E_BORROW_CONFLICT.summary = Ausleihe Konflikt.
LIMIT_E_BORROW_CONFLICT.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_BORROW_CONFLICT.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_BORROW_CONFLICT.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_BORROW_CONFLICT.example = vitte check src/main.vit

LIMIT_E_MUTABILITY_CONFLICT.summary = mutability Konflikt.
LIMIT_E_MUTABILITY_CONFLICT.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_MUTABILITY_CONFLICT.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_MUTABILITY_CONFLICT.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_MUTABILITY_CONFLICT.example = vitte check src/main.vit

LIMIT_E_LIFETIME_TOO_SHORT.summary = Lebensdauer zu short.
LIMIT_E_LIFETIME_TOO_SHORT.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_LIFETIME_TOO_SHORT.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_LIFETIME_TOO_SHORT.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_LIFETIME_TOO_SHORT.example = vitte check src/main.vit

LIMIT_E_DANGLING_REFERENCE.summary = dangling Referenz.
LIMIT_E_DANGLING_REFERENCE.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_DANGLING_REFERENCE.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_DANGLING_REFERENCE.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_DANGLING_REFERENCE.example = vitte check src/main.vit

LIMIT_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
LIMIT_E_NON_EXHAUSTIVE_MATCH.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_NON_EXHAUSTIVE_MATCH.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_NON_EXHAUSTIVE_MATCH.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_NON_EXHAUSTIVE_MATCH.example = vitte check src/main.vit

LIMIT_E_UNREACHABLE_PATTERN.summary = unerreichbar Muster.
LIMIT_E_UNREACHABLE_PATTERN.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_UNREACHABLE_PATTERN.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_UNREACHABLE_PATTERN.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_UNREACHABLE_PATTERN.example = vitte check src/main.vit

LIMIT_E_CONST_REQUIRED.summary = const required.
LIMIT_E_CONST_REQUIRED.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_CONST_REQUIRED.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_CONST_REQUIRED.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_CONST_REQUIRED.example = vitte check src/main.vit

LIMIT_E_CONST_OVERFLOW.summary = const Ueberlauf.
LIMIT_E_CONST_OVERFLOW.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_CONST_OVERFLOW.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_CONST_OVERFLOW.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_CONST_OVERFLOW.example = vitte check src/main.vit

LIMIT_E_CONST_DIVISION_BY_ZERO.summary = const Division by Null.
LIMIT_E_CONST_DIVISION_BY_ZERO.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_CONST_DIVISION_BY_ZERO.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_CONST_DIVISION_BY_ZERO.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_CONST_DIVISION_BY_ZERO.example = vitte check src/main.vit

LIMIT_E_CONST_CYCLE.summary = const Zyklus.
LIMIT_E_CONST_CYCLE.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_CONST_CYCLE.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_CONST_CYCLE.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_CONST_CYCLE.example = vitte check src/main.vit

LIMIT_E_MACRO_NOT_FOUND.summary = macro fehlend.
LIMIT_E_MACRO_NOT_FOUND.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_MACRO_NOT_FOUND.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_MACRO_NOT_FOUND.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_MACRO_NOT_FOUND.example = vitte check src/main.vit

LIMIT_E_MACRO_RECURSION.summary = macro recursion.
LIMIT_E_MACRO_RECURSION.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_MACRO_RECURSION.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_MACRO_RECURSION.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_MACRO_RECURSION.example = vitte check src/main.vit

LIMIT_E_MACRO_EXPANSION_FAILED.summary = macro expansion fehlgeschlagen.
LIMIT_E_MACRO_EXPANSION_FAILED.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_MACRO_EXPANSION_FAILED.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_MACRO_EXPANSION_FAILED.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_MACRO_EXPANSION_FAILED.example = vitte check src/main.vit

LIMIT_E_TRAIT_NOT_IMPLEMENTED.summary = trait not implemented.
LIMIT_E_TRAIT_NOT_IMPLEMENTED.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_TRAIT_NOT_IMPLEMENTED.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_TRAIT_NOT_IMPLEMENTED.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_TRAIT_NOT_IMPLEMENTED.example = vitte check src/main.vit

LIMIT_E_TRAIT_AMBIGUOUS.summary = trait ambiguous.
LIMIT_E_TRAIT_AMBIGUOUS.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_TRAIT_AMBIGUOUS.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_TRAIT_AMBIGUOUS.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_TRAIT_AMBIGUOUS.example = vitte check src/main.vit

LIMIT_E_GENERIC_ARGUMENT_MISSING.summary = generisch Argument fehlend.
LIMIT_E_GENERIC_ARGUMENT_MISSING.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_GENERIC_ARGUMENT_MISSING.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_GENERIC_ARGUMENT_MISSING.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_GENERIC_ARGUMENT_MISSING.example = vitte check src/main.vit

LIMIT_E_GENERIC_BOUND_FAILED.summary = generisch bound fehlgeschlagen.
LIMIT_E_GENERIC_BOUND_FAILED.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_GENERIC_BOUND_FAILED.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_GENERIC_BOUND_FAILED.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_GENERIC_BOUND_FAILED.example = vitte check src/main.vit

LIMIT_E_UNSUPPORTED_TARGET.summary = Ziel nicht unterstuetzt.
LIMIT_E_UNSUPPORTED_TARGET.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_UNSUPPORTED_TARGET.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_UNSUPPORTED_TARGET.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_UNSUPPORTED_TARGET.example = vitte check src/main.vit

LIMIT_E_ABI_MISMATCH.summary = abi Nichtuebereinstimmung.
LIMIT_E_ABI_MISMATCH.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_ABI_MISMATCH.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_ABI_MISMATCH.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_ABI_MISMATCH.example = vitte check src/main.vit

LIMIT_E_LINK_FAILED.summary = link fehlgeschlagen.
LIMIT_E_LINK_FAILED.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_LINK_FAILED.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_LINK_FAILED.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_LINK_FAILED.example = vitte check src/main.vit

LIMIT_E_RUNTIME_PANIC.summary = runtime Panik.
LIMIT_E_RUNTIME_PANIC.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_RUNTIME_PANIC.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_RUNTIME_PANIC.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_RUNTIME_PANIC.example = vitte check src/main.vit
