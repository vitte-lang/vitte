# Vitte diagnostic explanations (French)
#
# Cles : CODE.summary, CODE.cause, CODE.step1, CODE.fix, CODE.example

E0001.summary = The parseur expected a nom for something (variable, type, module, etc.).
E0001.cause = The parser or lexer could not form the next valid source construct.
E0001.step1 = Look at the highlighted token and complete or remove the construct around it.
E0001.fix = complete the syntax shape named by the parser label at the highlighted token
E0001.example = proc main() -> i32 { return 0 }

E0002.summary = The parseur expected an expression at this location.
E0002.cause = The parser or lexer could not form the next valid source construct.
E0002.step1 = Look at the highlighted token and complete or remove the construct around it.
E0002.fix = complete the syntax shape named by the parser label at the highlighted token
E0002.example = proc main() -> i32 { return 0 }

E0003.summary = The parseur expected a motif.
E0003.cause = The parser or lexer could not form the next valid source construct.
E0003.step1 = Look at the highlighted token and complete or remove the construct around it.
E0003.fix = complete the syntax shape named by the parser label at the highlighted token
E0003.example = when x is Option.Some { return 0 }

E0004.summary = The parseur expected a type nom.
E0004.cause = The parser or lexer could not form the next valid source construct.
E0004.step1 = Look at the highlighted token and complete or remove the construct around it.
E0004.fix = complete the syntax shape named by the parser label at the highlighted token
E0004.example = proc id(x: int) -> int { return x }

E0005.summary = A bloc was opened but not closed with 'end' or '.end'.
E0005.cause = The parser or lexer could not form the next valid source construct.
E0005.step1 = Look at the highlighted token and complete or remove the construct around it.
E0005.fix = complete the syntax shape named by the parser label at the highlighted token
E0005.example = form Point\n  field x as int\n.end

E0006.summary = An attribute doit etre followed by a proc declaration.
E0006.cause = The parser or lexer could not form the next valid source construct.
E0006.step1 = Look at the highlighted token and complete or remove the construct around it.
E0006.fix = complete the syntax shape named by the parser label at the highlighted token
E0006.example = #[inline]\nproc add(a: int, b: int) -> int { return a + b }

E0007.summary = The parseur expected a top-level declaration.
E0007.cause = The parser or lexer could not form the next valid source construct.
E0007.step1 = Look at the highlighted token and complete or remove the construct around it.
E0007.fix = complete the syntax shape named by the parser label at the highlighted token
E0007.example = space my/app\nproc main() -> int { return 0 }

E0008.summary = A motif bound the same nom more than once.
E0008.cause = The parser or lexer could not form the next valid source construct.
E0008.step1 = Look at the highlighted token and complete or remove the construct around it.
E0008.fix = complete the syntax shape named by the parser label at the highlighted token
E0008.example = when Pair(x, y) { return 0 }

E0009.summary = A referenced type nom was manquant.
E0009.cause = The parser or lexer could not form the next valid source construct.
E0009.step1 = Look at the highlighted token and complete or remove the construct around it.
E0009.fix = complete the syntax shape named by the parser label at the highlighted token
E0009.example = use std/core/option.Option\nproc f(x: Option[int]) -> int { return 0 }

E0010.summary = The base type of a generique was manquant.
E0010.cause = The parser or lexer could not form the next valid source construct.
E0010.step1 = Look at the highlighted token and complete or remove the construct around it.
E0010.fix = complete the syntax shape named by the parser label at the highlighted token
E0010.example = use std/core/option.Option\nlet x: Option[int] = Option.None

E0011.summary = A generique type must include at least one argument.
E0011.cause = The parser or lexer could not form the next valid source construct.
E0011.step1 = Look at the highlighted token and complete or remove the construct around it.
E0011.fix = complete the syntax shape named by the parser label at the highlighted token
E0011.example = let x: Option[int] = Option.None

E0012.summary = This type form is not supported yet.
E0012.cause = The parser or lexer could not form the next valid source construct.
E0012.step1 = Look at the highlighted token and complete or remove the construct around it.
E0012.fix = complete the syntax shape named by the parser label at the highlighted token
E0012.example = let p: *int = &value

E0013.summary = A procedure with an explicit retour type has a chemin that reaches the end without returning a valeur.
E0013.cause = The parser or lexer could not form the next valid source construct.
E0013.step1 = Look at the highlighted token and complete or remove the construct around it.
E0013.fix = complete the syntax shape named by the parser label at the highlighted token
E0013.example = proc to_code(ok: bool) -> int {\n  if ok { give 0 }\n  give 1\n}

E0014.summary = An invocation is manquant its appele.
E0014.cause = The parser or lexer could not form the next valid source construct.
E0014.step1 = Look at the highlighted token and complete or remove the construct around it.
E0014.fix = complete the syntax shape named by the parser label at the highlighted token
E0014.example = print(\"hi\")

E0015.summary = This expression is not supported by the HIR lowering yet.
E0015.cause = The parser or lexer could not form the next valid source construct.
E0015.step1 = Look at the highlighted token and complete or remove the construct around it.
E0015.fix = complete the syntax shape named by the parser label at the highlighted token
E0015.example = let x = value

E0016.summary = This motif is not supported by the HIR lowering yet.
E0016.cause = The parser or lexer could not form the next valid source construct.
E0016.step1 = Look at the highlighted token and complete or remove the construct around it.
E0016.fix = complete the syntax shape named by the parser label at the highlighted token
E0016.example = when x is Option.Some { return 0 }

E0017.summary = This instruction is not supported by the HIR lowering yet.
E0017.cause = The parser or lexer could not form the next valid source construct.
E0017.step1 = Look at the highlighted token and complete or remove the construct around it.
E0017.fix = complete the syntax shape named by the parser label at the highlighted token
E0017.example = return 0

E0018.summary = An extern procedure cannot define a body.
E0018.cause = The parser or lexer could not form the next valid source construct.
E0018.step1 = Look at the highlighted token and complete or remove the construct around it.
E0018.fix = complete the syntax shape named by the parser label at the highlighted token
E0018.example = #[extern]\nproc puts(s: string) -> int

E1001.summary = motif liaison duplique.
E1001.cause = The general phase found code that violates this diagnostic rule.
E1001.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1001.fix = repair the highlighted compiler contract before checking later diagnostics
E1001.example = vitte check path/to/file.vit

E1002.summary = type (did you mean a built-in like int/i32/i64/i128/u32/u64/u128/bool/string?) inconnu.
E1002.cause = The general phase found code that violates this diagnostic rule.
E1002.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1002.fix = repair the highlighted compiler contract before checking later diagnostics
E1002.example = vitte check path/to/file.vit

E1003.summary = generique base type inconnu.
E1003.cause = The general phase found code that violates this diagnostic rule.
E1003.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1003.fix = repair the highlighted compiler contract before checking later diagnostics
E1003.example = vitte check path/to/file.vit

E1004.summary = generique type necessite au moins un argument.
E1004.cause = The general phase found code that violates this diagnostic rule.
E1004.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1004.fix = repair the highlighted compiler contract before checking later diagnostics
E1004.example = vitte check path/to/file.vit

E1005.summary = identifiant inconnu.
E1005.cause = The general phase found code that violates this diagnostic rule.
E1005.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1005.fix = repair the highlighted compiler contract before checking later diagnostics
E1005.example = vitte check path/to/file.vit

E1006.summary = generique type necessite au moins un type argument.
E1006.cause = The general phase found code that violates this diagnostic rule.
E1006.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1006.fix = repair the highlighted compiler contract before checking later diagnostics
E1006.example = vitte check path/to/file.vit

E1007.summary = conversion between signed and unsigned values invalide.
E1007.cause = The general phase found code that violates this diagnostic rule.
E1007.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1007.fix = repair the highlighted compiler contract before checking later diagnostics
E1007.example = vitte check path/to/file.vit

E1010.summary = stdlib module refuse par active stdlib profil.
E1010.cause = The general phase found code that violates this diagnostic rule.
E1010.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1010.fix = repair the highlighted compiler contract before checking later diagnostics
E1010.example = vitte check path/to/file.vit

E1011.summary = strict-imports requires explicit alias.
E1011.cause = The general phase found code that violates this diagnostic rule.
E1011.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1011.fix = repair the highlighted compiler contract before checking later diagnostics
E1011.example = vitte check path/to/file.vit

E1012.summary = strict-imports interdit unused import aliases.
E1012.cause = The general phase found code that violates this diagnostic rule.
E1012.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1012.fix = repair the highlighted compiler contract before checking later diagnostics
E1012.example = vitte check path/to/file.vit

E1013.summary = strict-imports interdit non-canonical import paths.
E1013.cause = The general phase found code that violates this diagnostic rule.
E1013.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1013.fix = repair the highlighted compiler contract before checking later diagnostics
E1013.example = vitte check path/to/file.vit

E1014.summary = stdlib module manquant.
E1014.cause = The general phase found code that violates this diagnostic rule.
E1014.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1014.fix = repair the highlighted compiler contract before checking later diagnostics
E1014.example = vitte check path/to/file.vit

E1015.summary = experimental module import denied.
E1015.cause = The general phase found code that violates this diagnostic rule.
E1015.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1015.fix = repair the highlighted compiler contract before checking later diagnostics
E1015.example = vitte check path/to/file.vit

E1016.summary = interne module import denied.
E1016.cause = The general phase found code that violates this diagnostic rule.
E1016.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1016.fix = repair the highlighted compiler contract before checking later diagnostics
E1016.example = vitte check path/to/file.vit

E1017.summary = re-export symbole conflit.
E1017.cause = The general phase found code that violates this diagnostic rule.
E1017.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1017.fix = repair the highlighted compiler contract before checking later diagnostics
E1017.example = vitte check path/to/file.vit

E1018.summary = ambiguous import chemin.
E1018.cause = The general phase found code that violates this diagnostic rule.
E1018.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1018.fix = repair the highlighted compiler contract before checking later diagnostics
E1018.example = vitte check path/to/file.vit

E1019.summary = strict-modules interdit glob imports.
E1019.cause = The general phase found code that violates this diagnostic rule.
E1019.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1019.fix = repair the highlighted compiler contract before checking later diagnostics
E1019.example = vitte check path/to/file.vit

E1020.summary = legacy import chemin est obsolete.
E1020.cause = The general phase found code that violates this diagnostic rule.
E1020.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1020.fix = repair the highlighted compiler contract before checking later diagnostics
E1020.example = vitte check path/to/file.vit

E1021.summary = entry module chemin doit etre canonical.
E1021.cause = The general phase found code that violates this diagnostic rule.
E1021.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1021.fix = repair the highlighted compiler contract before checking later diagnostics
E1021.example = vitte check path/to/file.vit

E1022.summary = entry nom duplique.
E1022.cause = The general phase found code that violates this diagnostic rule.
E1022.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1022.fix = repair the highlighted compiler contract before checking later diagnostics
E1022.example = vitte check path/to/file.vit

E1023.summary = share reference symbole inconnu.
E1023.cause = The general phase found code that violates this diagnostic rule.
E1023.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1023.fix = repair the highlighted compiler contract before checking later diagnostics
E1023.example = vitte check path/to/file.vit

E1024.summary = symbole in share list duplique.
E1024.cause = The general phase found code that violates this diagnostic rule.
E1024.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1024.fix = repair the highlighted compiler contract before checking later diagnostics
E1024.example = vitte check path/to/file.vit

E1025.summary = symbole non exporte par module.
E1025.cause = The general phase found code that violates this diagnostic rule.
E1025.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1025.fix = repair the highlighted compiler contract before checking later diagnostics
E1025.example = vitte check path/to/file.vit

E1026.summary = share declaration duplique.
E1026.cause = The general phase found code that violates this diagnostic rule.
E1026.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1026.fix = repair the highlighted compiler contract before checking later diagnostics
E1026.example = vitte check path/to/file.vit

E1027.summary = import liaison duplique.
E1027.cause = The general phase found code that violates this diagnostic rule.
E1027.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1027.fix = repair the highlighted compiler contract before checking later diagnostics
E1027.example = vitte check path/to/file.vit

E1028.summary = import liaison en conflit avec local declaration.
E1028.cause = The general phase found code that violates this diagnostic rule.
E1028.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1028.fix = repair the highlighted compiler contract before checking later diagnostics
E1028.example = vitte check path/to/file.vit

E1029.summary = local declaration nom duplique.
E1029.cause = The general phase found code that violates this diagnostic rule.
E1029.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1029.fix = repair the highlighted compiler contract before checking later diagnostics
E1029.example = vitte check path/to/file.vit

E1030.summary = module alias membre not exported.
E1030.cause = The general phase found code that violates this diagnostic rule.
E1030.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1030.fix = repair the highlighted compiler contract before checking later diagnostics
E1030.example = vitte check path/to/file.vit

E1031.summary = qualified type membre manquant.
E1031.cause = The general phase found code that violates this diagnostic rule.
E1031.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1031.fix = repair the highlighted compiler contract before checking later diagnostics
E1031.example = vitte check path/to/file.vit

E1032.summary = l'expression n'est pas appelable.
E1032.cause = The general phase found code that violates this diagnostic rule.
E1032.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1032.fix = repair the highlighted compiler contract before checking later diagnostics
E1032.example = vitte check path/to/file.vit

E2001.summary = type non pris en charge.
E2001.cause = The general phase found code that violates this diagnostic rule.
E2001.step1 = Fix the first span reported for this diagnostic, then run the command again.
E2001.fix = repair the highlighted compiler contract before checking later diagnostics
E2001.example = vitte check path/to/file.vit

E2002.summary = invoke has no appele.
E2002.cause = The general phase found code that violates this diagnostic rule.
E2002.step1 = Fix the first span reported for this diagnostic, then run the command again.
E2002.fix = repair the highlighted compiler contract before checking later diagnostics
E2002.example = vitte check path/to/file.vit

E2003.summary = expression in HIR non pris en charge.
E2003.cause = The general phase found code that violates this diagnostic rule.
E2003.step1 = Fix the first span reported for this diagnostic, then run the command again.
E2003.fix = repair the highlighted compiler contract before checking later diagnostics
E2003.example = vitte check path/to/file.vit

E2004.summary = motif in HIR non pris en charge.
E2004.cause = The general phase found code that violates this diagnostic rule.
E2004.step1 = Fix the first span reported for this diagnostic, then run the command again.
E2004.fix = repair the highlighted compiler contract before checking later diagnostics
E2004.example = vitte check path/to/file.vit

E2005.summary = instruction in HIR non pris en charge.
E2005.cause = The general phase found code that violates this diagnostic rule.
E2005.step1 = Fix the first span reported for this diagnostic, then run the command again.
E2005.fix = repair the highlighted compiler contract before checking later diagnostics
E2005.example = vitte check path/to/file.vit

E2006.summary = HIR type kind inattendu.
E2006.cause = The general phase found code that violates this diagnostic rule.
E2006.step1 = Fix the first span reported for this diagnostic, then run the command again.
E2006.fix = repair the highlighted compiler contract before checking later diagnostics
E2006.example = vitte check path/to/file.vit

E2007.summary = HIR expr kind inattendu.
E2007.cause = The general phase found code that violates this diagnostic rule.
E2007.step1 = Fix the first span reported for this diagnostic, then run the command again.
E2007.fix = repair the highlighted compiler contract before checking later diagnostics
E2007.example = vitte check path/to/file.vit

E2008.summary = HIR stmt kind inattendu.
E2008.cause = The general phase found code that violates this diagnostic rule.
E2008.step1 = Fix the first span reported for this diagnostic, then run the command again.
E2008.fix = repair the highlighted compiler contract before checking later diagnostics
E2008.example = vitte check path/to/file.vit

E2009.summary = HIR motif kind inattendu.
E2009.cause = The general phase found code that violates this diagnostic rule.
E2009.step1 = Fix the first span reported for this diagnostic, then run the command again.
E2009.fix = repair the highlighted compiler contract before checking later diagnostics
E2009.example = vitte check path/to/file.vit

E2010.summary = HIR decl kind inattendu.
E2010.cause = The general phase found code that violates this diagnostic rule.
E2010.step1 = Fix the first span reported for this diagnostic, then run the command again.
E2010.fix = repair the highlighted compiler contract before checking later diagnostics
E2010.example = vitte check path/to/file.vit

LEX_E_INVALID_CHAR.summary = caractere invalide.
LEX_E_INVALID_CHAR.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_CHAR.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_CHAR.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_CHAR.example = proc main() -> int { give 0; }

LEX_E_INVALID_NUMBER.summary = numeric litteral invalide.
LEX_E_INVALID_NUMBER.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_NUMBER.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_NUMBER.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_NUMBER.example = proc main() -> int { give 0; }

LEX_E_INVALID_ESCAPE.summary = echappement sequence invalide.
LEX_E_INVALID_ESCAPE.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_ESCAPE.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_ESCAPE.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_ESCAPE.example = proc main() -> int { give 0; }

LEX_E_INVALID_UNICODE_ESCAPE.summary = unicode echappement sequence invalide.
LEX_E_INVALID_UNICODE_ESCAPE.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_UNICODE_ESCAPE.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_UNICODE_ESCAPE.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_UNICODE_ESCAPE.example = proc main() -> int { give 0; }

LEX_E_INVALID_UTF8.summary = UTF-8 byte sequence invalide.
LEX_E_INVALID_UTF8.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_UTF8.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_UTF8.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_UTF8.example = proc main() -> int { give 0; }

LEX_E_INVALID_CHAR_LITERAL.summary = caractere litteral invalide.
LEX_E_INVALID_CHAR_LITERAL.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_CHAR_LITERAL.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_CHAR_LITERAL.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_CHAR_LITERAL.example = proc main() -> int { give 0; }

LEX_E_INVALID_IDENTIFIER.summary = identifiant invalide.
LEX_E_INVALID_IDENTIFIER.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_IDENTIFIER.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_IDENTIFIER.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_IDENTIFIER.example = proc main() -> int { give 0; }

LEX_E_UNTERMINATED_STRING.summary = string litteral non termine.
LEX_E_UNTERMINATED_STRING.cause = The parser or lexer could not form the next valid source construct.
LEX_E_UNTERMINATED_STRING.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_UNTERMINATED_STRING.fix = add the closing `"` on the same line; do not close a string with a single quote
LEX_E_UNTERMINATED_STRING.example = proc main() -> int { give 0; }

LEX_E_UNTERMINATED_CHAR.summary = caractere litteral non termine.
LEX_E_UNTERMINATED_CHAR.cause = The parser or lexer could not form the next valid source construct.
LEX_E_UNTERMINATED_CHAR.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_UNTERMINATED_CHAR.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_UNTERMINATED_CHAR.example = proc main() -> int { give 0; }

LEX_E_UNTERMINATED_BLOCK_COMMENT.summary = bloc comment non termine.
LEX_E_UNTERMINATED_BLOCK_COMMENT.cause = The parser or lexer could not form the next valid source construct.
LEX_E_UNTERMINATED_BLOCK_COMMENT.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_UNTERMINATED_BLOCK_COMMENT.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_UNTERMINATED_BLOCK_COMMENT.example = proc main() -> int { give 0; }

LEX_E_UNTERMINATED_REGION_COMMENT.summary = region comment non termine.
LEX_E_UNTERMINATED_REGION_COMMENT.cause = The parser or lexer could not form the next valid source construct.
LEX_E_UNTERMINATED_REGION_COMMENT.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_UNTERMINATED_REGION_COMMENT.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_UNTERMINATED_REGION_COMMENT.example = proc main() -> int { give 0; }

LEX_E_UNEXPECTED_EOF.summary = end of fichier inattendu.
LEX_E_UNEXPECTED_EOF.cause = The parser or lexer could not form the next valid source construct.
LEX_E_UNEXPECTED_EOF.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_UNEXPECTED_EOF.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_UNEXPECTED_EOF.example = proc main() -> int { give 0; }

LEX_E_TOKEN_TOO_LARGE.summary = jeton trop.
LEX_E_TOKEN_TOO_LARGE.cause = The parser or lexer could not form the next valid source construct.
LEX_E_TOKEN_TOO_LARGE.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_TOKEN_TOO_LARGE.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_TOKEN_TOO_LARGE.example = proc main() -> int { give 0; }

PLOOP.summary = le parseur ne progresse plus.
PLOOP.cause = The parser or lexer could not form the next valid source construct.
PLOOP.step1 = Look at the highlighted token and complete or remove the construct around it.
PLOOP.fix = complete the syntax shape named by the parser label at the highlighted token
PLOOP.example = proc main() -> int { give 0; }

PPRIMARY999.summary = expression jeton inattendu.
PPRIMARY999.cause = The parser or lexer could not form the next valid source construct.
PPRIMARY999.step1 = Look at the highlighted token and complete or remove the construct around it.
PPRIMARY999.fix = complete the syntax shape named by the parser label at the highlighted token
PPRIMARY999.example = proc main() -> int { give 0; }

PSTMT007.summary = affectation operateur attendu.
PSTMT007.cause = The parser or lexer could not form the next valid source construct.
PSTMT007.step1 = Look at the highlighted token and complete or remove the construct around it.
PSTMT007.fix = complete the syntax shape named by the parser label at the highlighted token
PSTMT007.example = proc main() -> int { give 0; }

P0001.summary = top-level jeton inattendu.
P0001.cause = The parser or lexer could not form the next valid source construct.
P0001.step1 = Look at the highlighted token and complete or remove the construct around it.
P0001.fix = complete the syntax shape named by the parser label at the highlighted token
P0001.example = proc main() -> int { give 0; }

P000_UNBALANCED.summary = bloc non ferme.
P000_UNBALANCED.cause = The parser or lexer could not form the next valid source construct.
P000_UNBALANCED.step1 = Look at the highlighted token and complete or remove the construct around it.
P000_UNBALANCED.fix = complete the syntax shape named by the parser label at the highlighted token
P000_UNBALANCED.example = proc main() -> int { give 0; }

LEX_E_INVALID_FLOAT.summary = flottant invalide.
LEX_E_INVALID_FLOAT.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_FLOAT.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_FLOAT.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_FLOAT.example = proc main() -> int { give 0; }

LEX_E_INVALID_BINARY.summary = binaire invalide.
LEX_E_INVALID_BINARY.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_BINARY.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_BINARY.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_BINARY.example = proc main() -> int { give 0; }

LEX_E_INVALID_OCTAL.summary = octal invalide.
LEX_E_INVALID_OCTAL.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_OCTAL.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_OCTAL.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_OCTAL.example = proc main() -> int { give 0; }

LEX_E_INVALID_HEX.summary = hex invalide.
LEX_E_INVALID_HEX.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_HEX.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_HEX.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_HEX.example = proc main() -> int { give 0; }

LEX_E_INVALID_INDENTATION.summary = indentation invalide.
LEX_E_INVALID_INDENTATION.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_INDENTATION.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_INDENTATION.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_INDENTATION.example = proc main() -> int { give 0; }

LEX_E_INVALID_TOKEN.summary = jeton invalide.
LEX_E_INVALID_TOKEN.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_TOKEN.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_TOKEN.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_TOKEN.example = proc main() -> int { give 0; }

LEX_E_INVALID_UNICODE.summary = unicode invalide.
LEX_E_INVALID_UNICODE.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_UNICODE.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_UNICODE.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_UNICODE.example = proc main() -> int { give 0; }

LEX_E_UNTERMINATED_COMMENT.summary = comment non termine.
LEX_E_UNTERMINATED_COMMENT.cause = The parser or lexer could not form the next valid source construct.
LEX_E_UNTERMINATED_COMMENT.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_UNTERMINATED_COMMENT.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_UNTERMINATED_COMMENT.example = proc main() -> int { give 0; }

PATTR003.summary = pattr003.
PATTR003.cause = The parser or lexer could not form the next valid source construct.
PATTR003.step1 = Look at the highlighted token and complete or remove the construct around it.
PATTR003.fix = complete the syntax shape named by the parser label at the highlighted token
PATTR003.example = proc main() -> int { give 0; }

PARSE_E_TOPLEVEL_DECL_EXPECTED.summary = top-level declaration expected.
PARSE_E_TOPLEVEL_DECL_EXPECTED.cause = The parser or lexer could not form the next valid source construct.
PARSE_E_TOPLEVEL_DECL_EXPECTED.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_E_TOPLEVEL_DECL_EXPECTED.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_E_TOPLEVEL_DECL_EXPECTED.example = proc main() -> int { give 0; }

PARSE_E_INCOMPLETE_EXPR.summary = incomplete expression.
PARSE_E_INCOMPLETE_EXPR.cause = The parser or lexer could not form the next valid source construct.
PARSE_E_INCOMPLETE_EXPR.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_E_INCOMPLETE_EXPR.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_E_INCOMPLETE_EXPR.example = proc main() -> int { give 0; }

PARSE_E_MISSING_RPAREN.summary = closing parenthesis manquant.
PARSE_E_MISSING_RPAREN.cause = The parser or lexer could not form the next valid source construct.
PARSE_E_MISSING_RPAREN.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_E_MISSING_RPAREN.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_E_MISSING_RPAREN.example = proc main() -> int { give 0; }

PARSE_E_MISSING_COMMA.summary = comma manquant.
PARSE_E_MISSING_COMMA.cause = The parser or lexer could not form the next valid source construct.
PARSE_E_MISSING_COMMA.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_E_MISSING_COMMA.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_E_MISSING_COMMA.example = proc main() -> int { give 0; }

PARSE_E_TYPE_EXPECTED.summary = type expected.
PARSE_E_TYPE_EXPECTED.cause = The parser or lexer could not form the next valid source construct.
PARSE_E_TYPE_EXPECTED.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_E_TYPE_EXPECTED.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_E_TYPE_EXPECTED.example = proc main() -> int { give 0; }

PARSE_E_PATTERN_EXPECTED.summary = motif expected.
PARSE_E_PATTERN_EXPECTED.cause = The parser or lexer could not form the next valid source construct.
PARSE_E_PATTERN_EXPECTED.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_E_PATTERN_EXPECTED.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_E_PATTERN_EXPECTED.example = proc main() -> int { give 0; }

PARSE_E_BLOCK_EXPECTED.summary = bloc expected.
PARSE_E_BLOCK_EXPECTED.cause = The parser or lexer could not form the next valid source construct.
PARSE_E_BLOCK_EXPECTED.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_E_BLOCK_EXPECTED.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_E_BLOCK_EXPECTED.example = proc main() -> int { give 0; }

PARSE_E_UNCLOSED_BLOCK.summary = bloc non ferme.
PARSE_E_UNCLOSED_BLOCK.cause = The parser or lexer could not form the next valid source construct.
PARSE_E_UNCLOSED_BLOCK.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_E_UNCLOSED_BLOCK.fix = add `}` to close the highlighted block, then run the checker again for its parent block
PARSE_E_UNCLOSED_BLOCK.example = proc main() -> int { give 0; }

PARSE_E_EXPECTED_TOKEN.summary = jeton attendu.
PARSE_E_EXPECTED_TOKEN.cause = The parser or lexer could not form the next valid source construct.
PARSE_E_EXPECTED_TOKEN.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_E_EXPECTED_TOKEN.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_E_EXPECTED_TOKEN.example = proc main() -> int { give 0; }

PARSE_E_PARAMETER_COLON_EXPECTED.summary = deux-points manquant dans un parametre de procedure.
PARSE_E_PARAMETER_COLON_EXPECTED.cause = Le nom du parametre est suivi de son type sans le separateur deux-points obligatoire.
PARSE_E_PARAMETER_COLON_EXPECTED.step1 = Verifiez le parametre souligne dans la signature multi-ligne de la procedure.
PARSE_E_PARAMETER_COLON_EXPECTED.fix = insert `:` between the parameter name and its type, for example `right: f64`
PARSE_E_PARAMETER_COLON_EXPECTED.example = proc calculate(right: f64) -> f64 { give right; }

PARSE_E_UNEXPECTED_TOKEN.summary = jeton inattendu.
PARSE_E_UNEXPECTED_TOKEN.cause = The parser or lexer could not form the next valid source construct.
PARSE_E_UNEXPECTED_TOKEN.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_E_UNEXPECTED_TOKEN.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_E_UNEXPECTED_TOKEN.example = proc main() -> int { give 0; }

PARSE_EXPECTED_EXPR.summary = parse expected expr.
PARSE_EXPECTED_EXPR.cause = The parser or lexer could not form the next valid source construct.
PARSE_EXPECTED_EXPR.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_EXPECTED_EXPR.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_EXPECTED_EXPR.example = proc main() -> int { give 0; }

PARSE_EXPECTED_TYPE.summary = parse expected type.
PARSE_EXPECTED_TYPE.cause = The parser or lexer could not form the next valid source construct.
PARSE_EXPECTED_TYPE.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_EXPECTED_TYPE.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_EXPECTED_TYPE.example = proc main() -> int { give 0; }

PARSE_EXPECTED_PATTERN.summary = parse expected motif.
PARSE_EXPECTED_PATTERN.cause = The parser or lexer could not form the next valid source construct.
PARSE_EXPECTED_PATTERN.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_EXPECTED_PATTERN.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_EXPECTED_PATTERN.example = proc main() -> int { give 0; }

PARSE_EXPECTED_BLOCK.summary = parse expected bloc.
PARSE_EXPECTED_BLOCK.cause = The parser or lexer could not form the next valid source construct.
PARSE_EXPECTED_BLOCK.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_EXPECTED_BLOCK.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_EXPECTED_BLOCK.example = proc main() -> int { give 0; }

PARSE_EXPECTED_IDENTIFIER.summary = parse expected identifiant.
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

AST_E_INVALID_NODE.summary = noeud invalide.
AST_E_INVALID_NODE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
AST_E_INVALID_NODE.step1 = Fix the first span reported for this diagnostic, then run the command again.
AST_E_INVALID_NODE.fix = repair the highlighted compiler contract before checking later diagnostics
AST_E_INVALID_NODE.example = vitte check path/to/file.vit

AST_E_EMPTY_MODULE.summary = vide module.
AST_E_EMPTY_MODULE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
AST_E_EMPTY_MODULE.step1 = Fix the first span reported for this diagnostic, then run the command again.
AST_E_EMPTY_MODULE.fix = repair the highlighted compiler contract before checking later diagnostics
AST_E_EMPTY_MODULE.example = vitte check path/to/file.vit

AST_E_INVALID_DECL.summary = decl invalide.
AST_E_INVALID_DECL.cause = The compiler pipeline phase found code that violates this diagnostic rule.
AST_E_INVALID_DECL.step1 = Fix the first span reported for this diagnostic, then run the command again.
AST_E_INVALID_DECL.fix = repair the highlighted compiler contract before checking later diagnostics
AST_E_INVALID_DECL.example = vitte check path/to/file.vit

AST_E_INVALID_EXPR.summary = expr invalide.
AST_E_INVALID_EXPR.cause = The compiler pipeline phase found code that violates this diagnostic rule.
AST_E_INVALID_EXPR.step1 = Fix the first span reported for this diagnostic, then run the command again.
AST_E_INVALID_EXPR.fix = repair the highlighted compiler contract before checking later diagnostics
AST_E_INVALID_EXPR.example = vitte check path/to/file.vit

AST_E_INVALID_PATTERN.summary = motif invalide.
AST_E_INVALID_PATTERN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
AST_E_INVALID_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
AST_E_INVALID_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
AST_E_INVALID_PATTERN.example = vitte check path/to/file.vit

AST_E_INVALID_ATTRIBUTE.summary = attribute invalide.
AST_E_INVALID_ATTRIBUTE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
AST_E_INVALID_ATTRIBUTE.step1 = Fix the first span reported for this diagnostic, then run the command again.
AST_E_INVALID_ATTRIBUTE.fix = repair the highlighted compiler contract before checking later diagnostics
AST_E_INVALID_ATTRIBUTE.example = vitte check path/to/file.vit

AST_E_DUPLICATE_FIELD.summary = champ duplique.
AST_E_DUPLICATE_FIELD.cause = The compiler pipeline phase found code that violates this diagnostic rule.
AST_E_DUPLICATE_FIELD.step1 = Fix the first span reported for this diagnostic, then run the command again.
AST_E_DUPLICATE_FIELD.fix = repair the highlighted compiler contract before checking later diagnostics
AST_E_DUPLICATE_FIELD.example = vitte check path/to/file.vit

AST_E_INVALID_VISIBILITY.summary = visibilite invalide.
AST_E_INVALID_VISIBILITY.cause = The compiler pipeline phase found code that violates this diagnostic rule.
AST_E_INVALID_VISIBILITY.step1 = Fix the first span reported for this diagnostic, then run the command again.
AST_E_INVALID_VISIBILITY.fix = repair the highlighted compiler contract before checking later diagnostics
AST_E_INVALID_VISIBILITY.example = vitte check path/to/file.vit

AST_E_INVALID_ENTRY.summary = entry invalide.
AST_E_INVALID_ENTRY.cause = The compiler pipeline phase found code that violates this diagnostic rule.
AST_E_INVALID_ENTRY.step1 = Fix the first span reported for this diagnostic, then run the command again.
AST_E_INVALID_ENTRY.fix = repair the highlighted compiler contract before checking later diagnostics
AST_E_INVALID_ENTRY.example = vitte check path/to/file.vit

SEMA_E_DUPLICATE_SYMBOL.summary = symbole duplique.
SEMA_E_DUPLICATE_SYMBOL.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_DUPLICATE_SYMBOL.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_DUPLICATE_SYMBOL.fix = rename one declaration or remove the duplicate from the same scope
SEMA_E_DUPLICATE_SYMBOL.example = use vitte/core

SEMA_E_UNKNOWN_IDENTIFIER.summary = identifiant inconnu.
SEMA_E_UNKNOWN_IDENTIFIER.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_UNKNOWN_IDENTIFIER.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_UNKNOWN_IDENTIFIER.fix = declare the missing identifier in scope or import the module that exports it
SEMA_E_UNKNOWN_IDENTIFIER.example = use vitte/core

SEMA_E_AMBIGUOUS_SYMBOL.summary = ambiguous symbole.
SEMA_E_AMBIGUOUS_SYMBOL.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_AMBIGUOUS_SYMBOL.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_AMBIGUOUS_SYMBOL.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_AMBIGUOUS_SYMBOL.example = use vitte/core

SEMA_E_SHADOWING_FORBIDDEN.summary = shadowing is forbidden.
SEMA_E_SHADOWING_FORBIDDEN.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_SHADOWING_FORBIDDEN.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_SHADOWING_FORBIDDEN.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_SHADOWING_FORBIDDEN.example = use vitte/core

SEMA_E_UNKNOWN_FIELD.summary = champ does not exist.
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

SEMA_E_UNKNOWN_SYMBOL.summary = symbole inconnu.
SEMA_E_UNKNOWN_SYMBOL.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_UNKNOWN_SYMBOL.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_UNKNOWN_SYMBOL.fix = declare the missing identifier in scope or import the module that exports it
SEMA_E_UNKNOWN_SYMBOL.example = use vitte/core

SEMA_E_INVALID_IMPORT.summary = import invalide.
SEMA_E_INVALID_IMPORT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_INVALID_IMPORT.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_INVALID_IMPORT.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_INVALID_IMPORT.example = use vitte/core

SEMA_E_INVALID_EXPORT.summary = export invalide.
SEMA_E_INVALID_EXPORT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_INVALID_EXPORT.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_INVALID_EXPORT.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_INVALID_EXPORT.example = use vitte/core

SEMA_E_INVALID_ATTRIBUTE.summary = attribute invalide.
SEMA_E_INVALID_ATTRIBUTE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_INVALID_ATTRIBUTE.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_INVALID_ATTRIBUTE.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_INVALID_ATTRIBUTE.example = use vitte/core

SEMA_E_INVALID_VISIBILITY.summary = visibilite invalide.
SEMA_E_INVALID_VISIBILITY.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_INVALID_VISIBILITY.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_INVALID_VISIBILITY.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_INVALID_VISIBILITY.example = use vitte/core

SEMA_E_INVALID_CONTROL_FLOW.summary = control flux invalide.
SEMA_E_INVALID_CONTROL_FLOW.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_INVALID_CONTROL_FLOW.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_INVALID_CONTROL_FLOW.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_INVALID_CONTROL_FLOW.example = use vitte/core

SEMA_E_INVALID_MODULE.summary = module invalide.
SEMA_E_INVALID_MODULE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_INVALID_MODULE.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_INVALID_MODULE.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_INVALID_MODULE.example = use vitte/core

SEMA_E_INVALID_ASSIGN_TARGET.summary = assign cible invalide.
SEMA_E_INVALID_ASSIGN_TARGET.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_INVALID_ASSIGN_TARGET.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_INVALID_ASSIGN_TARGET.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_INVALID_ASSIGN_TARGET.example = use vitte/core

SEMA_E_UNDECLARED_TARGET.summary = undeclared cible.
SEMA_E_UNDECLARED_TARGET.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_UNDECLARED_TARGET.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_UNDECLARED_TARGET.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_UNDECLARED_TARGET.example = use vitte/core

SEMA_E_MISSING_BINDING.summary = liaison manquant.
SEMA_E_MISSING_BINDING.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_MISSING_BINDING.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_MISSING_BINDING.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_MISSING_BINDING.example = use vitte/core

SEMA_E_DUPLICATE_BINDING.summary = liaison duplique.
SEMA_E_DUPLICATE_BINDING.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_DUPLICATE_BINDING.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_DUPLICATE_BINDING.fix = rename one declaration or remove the duplicate from the same scope
SEMA_E_DUPLICATE_BINDING.example = use vitte/core

SEMA_E_DUPLICATE_ITEM.summary = element duplique.
SEMA_E_DUPLICATE_ITEM.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_DUPLICATE_ITEM.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_DUPLICATE_ITEM.fix = rename one declaration or remove the duplicate from the same scope
SEMA_E_DUPLICATE_ITEM.example = use vitte/core

SEMA_E_INVALID_HIR.summary = hir invalide.
SEMA_E_INVALID_HIR.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_INVALID_HIR.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_INVALID_HIR.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_INVALID_HIR.example = use vitte/core

SEMA_E_INTERNAL.summary = interne.
SEMA_E_INTERNAL.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_INTERNAL.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_INTERNAL.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_INTERNAL.example = use vitte/core

TYPECK_E_UNKNOWN_NAME.summary = nom inconnu.
TYPECK_E_UNKNOWN_NAME.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_UNKNOWN_NAME.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_UNKNOWN_NAME.fix = make the expression type match the type contract named by the type checker
TYPECK_E_UNKNOWN_NAME.example = let count: int = 1

TYPECK_E_UNKNOWN_TYPE.summary = type inconnu.
TYPECK_E_UNKNOWN_TYPE.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_UNKNOWN_TYPE.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_UNKNOWN_TYPE.fix = make the expression type match the type contract named by the type checker
TYPECK_E_UNKNOWN_TYPE.example = let count: int = 1

TYPECK_E_UNKNOWN_ITEM.summary = element inconnu.
TYPECK_E_UNKNOWN_ITEM.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_UNKNOWN_ITEM.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_UNKNOWN_ITEM.fix = make the expression type match the type contract named by the type checker
TYPECK_E_UNKNOWN_ITEM.example = let count: int = 1

TYPECK_W_UNRESOLVED_NAME.summary = unresolved nom.
TYPECK_W_UNRESOLVED_NAME.cause = The inferred type does not satisfy the type required at this location.
TYPECK_W_UNRESOLVED_NAME.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_W_UNRESOLVED_NAME.fix = make the expression type match the type contract named by the type checker
TYPECK_W_UNRESOLVED_NAME.example = let count: int = 1

TYPECK_E_INVALID_EXPR.summary = expr invalide.
TYPECK_E_INVALID_EXPR.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_INVALID_EXPR.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_INVALID_EXPR.fix = make the expression type match the type contract named by the type checker
TYPECK_E_INVALID_EXPR.example = let count: int = 1

TYPECK_E_INVALID_DEREF.summary = deref invalide.
TYPECK_E_INVALID_DEREF.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_INVALID_DEREF.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_INVALID_DEREF.fix = make the expression type match the type contract named by the type checker
TYPECK_E_INVALID_DEREF.example = let count: int = 1

TYPECK_E_BINARY_MISMATCH.summary = binaire incompatibilite.
TYPECK_E_BINARY_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_BINARY_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_BINARY_MISMATCH.fix = make both operands valid for the operator before MIR lowering
TYPECK_E_BINARY_MISMATCH.example = let count: int = 1

TYPECK_E_ASSIGN_MISMATCH.summary = affectation type incompatibilite.
TYPECK_E_ASSIGN_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_ASSIGN_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_ASSIGN_MISMATCH.fix = assign a value of the declared binding type, or change the binding annotation at its declaration
TYPECK_E_ASSIGN_MISMATCH.example = let count: int = 1

TYPECK_E_RETURN_MISMATCH.summary = retour type incompatibilite.
TYPECK_E_RETURN_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_RETURN_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_RETURN_MISMATCH.fix = make the expression type match the type contract named by the type checker
TYPECK_E_RETURN_MISMATCH.example = let count: int = 1

TYPECK_E_CONDITION_TYPE.summary = condition type.
TYPECK_E_CONDITION_TYPE.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_CONDITION_TYPE.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_CONDITION_TYPE.fix = make the condition produce bool, for example by adding an explicit comparison
TYPECK_E_CONDITION_TYPE.example = let count: int = 1

TYPECK_E_UNKNOWN_MEMBER.summary = membre inconnu.
TYPECK_E_UNKNOWN_MEMBER.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_UNKNOWN_MEMBER.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_UNKNOWN_MEMBER.fix = make the expression type match the type contract named by the type checker
TYPECK_E_UNKNOWN_MEMBER.example = let count: int = 1

TYPECK_E_INDEX_TYPE.summary = index type.
TYPECK_E_INDEX_TYPE.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_INDEX_TYPE.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_INDEX_TYPE.fix = make the expression type match the type contract named by the type checker
TYPECK_E_INDEX_TYPE.example = let count: int = 1

TYPECK_E_INVALID_INDEX_TARGET.summary = index cible invalide.
TYPECK_E_INVALID_INDEX_TARGET.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_INVALID_INDEX_TARGET.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_INVALID_INDEX_TARGET.fix = make the expression type match the type contract named by the type checker
TYPECK_E_INVALID_INDEX_TARGET.example = let count: int = 1

TYPECK_E_IF_BRANCH_MISMATCH.summary = if branch incompatibilite.
TYPECK_E_IF_BRANCH_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_IF_BRANCH_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_IF_BRANCH_MISMATCH.fix = make the expression type match the type contract named by the type checker
TYPECK_E_IF_BRANCH_MISMATCH.example = let count: int = 1

TYPECK_E_COMPARE_MISMATCH.summary = compare incompatibilite.
TYPECK_E_COMPARE_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_COMPARE_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_COMPARE_MISMATCH.fix = make the expression type match the type contract named by the type checker
TYPECK_E_COMPARE_MISMATCH.example = let count: int = 1

TYPECK_E_INVALID_CAST.summary = conversion invalide.
TYPECK_E_INVALID_CAST.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_INVALID_CAST.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_INVALID_CAST.fix = make the expression type match the type contract named by the type checker
TYPECK_E_INVALID_CAST.example = let count: int = 1

TYPECK_E_MATCH_NON_EXHAUSTIVE.summary = match non exhaustive.
TYPECK_E_MATCH_NON_EXHAUSTIVE.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_MATCH_NON_EXHAUSTIVE.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_MATCH_NON_EXHAUSTIVE.fix = make the expression type match the type contract named by the type checker
TYPECK_E_MATCH_NON_EXHAUSTIVE.example = let count: int = 1

TYPECK_E_INVALID_CALL_TARGET.summary = appel cible invalide.
TYPECK_E_INVALID_CALL_TARGET.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_INVALID_CALL_TARGET.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_INVALID_CALL_TARGET.fix = make the expression type match the type contract named by the type checker
TYPECK_E_INVALID_CALL_TARGET.example = let count: int = 1

TYPECK_E_ARGUMENT_MISMATCH.summary = appel argument type incompatibilite.
TYPECK_E_ARGUMENT_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_ARGUMENT_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_ARGUMENT_MISMATCH.fix = make the expression type match the type contract named by the type checker
TYPECK_E_ARGUMENT_MISMATCH.example = let count: int = 1

TYPECK_E_CALL_ARITY.summary = wrong nombre of appel arguments.
TYPECK_E_CALL_ARITY.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_CALL_ARITY.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_CALL_ARITY.fix = make the expression type match the type contract named by the type checker
TYPECK_E_CALL_ARITY.example = let count: int = 1

TYPECK_E_GENERIC_INFERENCE.summary = generique type could not be inferred.
TYPECK_E_GENERIC_INFERENCE.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_GENERIC_INFERENCE.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_GENERIC_INFERENCE.fix = make the expression type match the type contract named by the type checker
TYPECK_E_GENERIC_INFERENCE.example = let count: int = 1

TYPECK_E_TRAIT_BOUND.summary = trait constraint is not satisfied.
TYPECK_E_TRAIT_BOUND.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_TRAIT_BOUND.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_TRAIT_BOUND.fix = make the expression type match the type contract named by the type checker
TYPECK_E_TRAIT_BOUND.example = let count: int = 1

TYPECK_E_CAUSE_CHAIN_MISSING.summary = type diagnostic is manquant a cause chain.
TYPECK_E_CAUSE_CHAIN_MISSING.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_CAUSE_CHAIN_MISSING.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_CAUSE_CHAIN_MISSING.fix = make the expression type match the type contract named by the type checker
TYPECK_E_CAUSE_CHAIN_MISSING.example = let count: int = 1

TYPECK_E_CONFLICTING_IMPL.summary = conflicting trait implementation.
TYPECK_E_CONFLICTING_IMPL.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_CONFLICTING_IMPL.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_CONFLICTING_IMPL.fix = make the expression type match the type contract named by the type checker
TYPECK_E_CONFLICTING_IMPL.example = let count: int = 1

TYPECK_E_CONSTRAINT_CYCLE.summary = cyclic generique constraint.
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

TYPECK_E_USE_AFTER_MOVE.summary = use after deplacement.
TYPECK_E_USE_AFTER_MOVE.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_USE_AFTER_MOVE.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_USE_AFTER_MOVE.fix = make the expression type match the type contract named by the type checker
TYPECK_E_USE_AFTER_MOVE.example = let count: int = 1

TYPECK_E_MALFORMED_UNARY.summary = malformed unary.
TYPECK_E_MALFORMED_UNARY.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_MALFORMED_UNARY.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_MALFORMED_UNARY.fix = make the expression type match the type contract named by the type checker
TYPECK_E_MALFORMED_UNARY.example = let count: int = 1

TYPECK_E_UNARY_MISMATCH.summary = unary incompatibilite.
TYPECK_E_UNARY_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_UNARY_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_UNARY_MISMATCH.fix = make the expression type match the type contract named by the type checker
TYPECK_E_UNARY_MISMATCH.example = let count: int = 1

TYPECK_E_MALFORMED_BORROW.summary = malformed emprunt.
TYPECK_E_MALFORMED_BORROW.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_MALFORMED_BORROW.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_MALFORMED_BORROW.fix = make the expression type match the type contract named by the type checker
TYPECK_E_MALFORMED_BORROW.example = let count: int = 1

TYPECK_E_MALFORMED_MEMBER.summary = malformed membre.
TYPECK_E_MALFORMED_MEMBER.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_MALFORMED_MEMBER.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_MALFORMED_MEMBER.fix = make the expression type match the type contract named by the type checker
TYPECK_E_MALFORMED_MEMBER.example = let count: int = 1

TYPECK_E_MALFORMED_INDEX.summary = malformed index.
TYPECK_E_MALFORMED_INDEX.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_MALFORMED_INDEX.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_MALFORMED_INDEX.fix = make the expression type match the type contract named by the type checker
TYPECK_E_MALFORMED_INDEX.example = let count: int = 1

TYPECK_E_MALFORMED_BINARY.summary = malformed binaire.
TYPECK_E_MALFORMED_BINARY.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_MALFORMED_BINARY.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_MALFORMED_BINARY.fix = make the expression type match the type contract named by the type checker
TYPECK_E_MALFORMED_BINARY.example = let count: int = 1

TYPECK_E_MALFORMED_CAST.summary = malformed conversion.
TYPECK_E_MALFORMED_CAST.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_MALFORMED_CAST.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_MALFORMED_CAST.fix = make the expression type match the type contract named by the type checker
TYPECK_E_MALFORMED_CAST.example = let count: int = 1

TYPECK_E_MALFORMED_IF.summary = malformed if.
TYPECK_E_MALFORMED_IF.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_MALFORMED_IF.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_MALFORMED_IF.fix = make the expression type match the type contract named by the type checker
TYPECK_E_MALFORMED_IF.example = let count: int = 1

TYPECK_E_INVALID_HIR.summary = hir invalide.
TYPECK_E_INVALID_HIR.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_INVALID_HIR.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_INVALID_HIR.fix = make the expression type match the type contract named by the type checker
TYPECK_E_INVALID_HIR.example = let count: int = 1

TYPECK_E_INTERNAL.summary = interne.
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

BORROWCK_E_MOVE_AFTER_MOVE.summary = deplacement after deplacement.
BORROWCK_E_MOVE_AFTER_MOVE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_MOVE_AFTER_MOVE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_MOVE_AFTER_MOVE.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_MOVE_AFTER_MOVE.example = let view = &value

BORROWCK_E_USE_AFTER_MOVE.summary = valeur utilise after deplacement.
BORROWCK_E_USE_AFTER_MOVE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_USE_AFTER_MOVE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_USE_AFTER_MOVE.fix = borrow before the move, clone explicitly, or move the later use before ownership transfer
BORROWCK_E_USE_AFTER_MOVE.example = let view = &value

BORROWCK_E_BORROW_OF_MOVED_VALUE.summary = emprunt of deplace valeur.
BORROWCK_E_BORROW_OF_MOVED_VALUE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_BORROW_OF_MOVED_VALUE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_BORROW_OF_MOVED_VALUE.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_BORROW_OF_MOVED_VALUE.example = let view = &value

BORROWCK_E_MUTABLE_BORROW_CONFLICT.summary = mutable emprunt conflit.
BORROWCK_E_MUTABLE_BORROW_CONFLICT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_MUTABLE_BORROW_CONFLICT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_MUTABLE_BORROW_CONFLICT.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_MUTABLE_BORROW_CONFLICT.example = let view = &value

BORROWCK_E_SHARED_BORROW_CONFLICT.summary = partage emprunt conflit.
BORROWCK_E_SHARED_BORROW_CONFLICT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_SHARED_BORROW_CONFLICT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_SHARED_BORROW_CONFLICT.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_SHARED_BORROW_CONFLICT.example = let view = &value

BORROWCK_E_WRITE_WHILE_BORROWED.summary = ecriture pendant emprunte.
BORROWCK_E_WRITE_WHILE_BORROWED.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_WRITE_WHILE_BORROWED.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_WRITE_WHILE_BORROWED.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_WRITE_WHILE_BORROWED.example = let view = &value

BORROWCK_E_MOVE_WHILE_BORROWED.summary = deplacement pendant emprunte.
BORROWCK_E_MOVE_WHILE_BORROWED.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_MOVE_WHILE_BORROWED.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_MOVE_WHILE_BORROWED.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_MOVE_WHILE_BORROWED.example = let view = &value

BORROWCK_E_DROP_WHILE_BORROWED.summary = destruction pendant emprunte.
BORROWCK_E_DROP_WHILE_BORROWED.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_DROP_WHILE_BORROWED.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_DROP_WHILE_BORROWED.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_DROP_WHILE_BORROWED.example = let view = &value

BORROWCK_E_ASSIGN_WHILE_BORROWED.summary = assign pendant emprunte.
BORROWCK_E_ASSIGN_WHILE_BORROWED.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_ASSIGN_WHILE_BORROWED.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_ASSIGN_WHILE_BORROWED.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_ASSIGN_WHILE_BORROWED.example = let view = &value

BORROWCK_E_RETURN_REF_TO_LOCAL.summary = retour ref to local.
BORROWCK_E_RETURN_REF_TO_LOCAL.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_RETURN_REF_TO_LOCAL.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_RETURN_REF_TO_LOCAL.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_RETURN_REF_TO_LOCAL.example = let view = &value

BORROWCK_E_RETURN_BORROW_OF_LOCAL.summary = retour emprunt of local.
BORROWCK_E_RETURN_BORROW_OF_LOCAL.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_RETURN_BORROW_OF_LOCAL.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_RETURN_BORROW_OF_LOCAL.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_RETURN_BORROW_OF_LOCAL.example = let view = &value

BORROWCK_E_DANGLING_REFERENCE.summary = dangling reference.
BORROWCK_E_DANGLING_REFERENCE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_DANGLING_REFERENCE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_DANGLING_REFERENCE.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_DANGLING_REFERENCE.example = let view = &value

BORROWCK_E_LIFETIME_TOO_SHORT.summary = duree de vie trop short.
BORROWCK_E_LIFETIME_TOO_SHORT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_LIFETIME_TOO_SHORT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_LIFETIME_TOO_SHORT.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_LIFETIME_TOO_SHORT.example = let view = &value

BORROWCK_E_IMMUTABLE_ASSIGN.summary = immutable assign.
BORROWCK_E_IMMUTABLE_ASSIGN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_IMMUTABLE_ASSIGN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_IMMUTABLE_ASSIGN.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_IMMUTABLE_ASSIGN.example = let view = &value

BORROWCK_E_USE_AFTER_DROP.summary = use after destruction.
BORROWCK_E_USE_AFTER_DROP.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_USE_AFTER_DROP.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_USE_AFTER_DROP.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_USE_AFTER_DROP.example = let view = &value

BORROWCK_E_DOUBLE_DROP.summary = double destruction.
BORROWCK_E_DOUBLE_DROP.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_DOUBLE_DROP.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_DOUBLE_DROP.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_DOUBLE_DROP.example = let view = &value

BORROWCK_E_UNINITIALIZED_USE.summary = non initialise use.
BORROWCK_E_UNINITIALIZED_USE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_UNINITIALIZED_USE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_UNINITIALIZED_USE.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_UNINITIALIZED_USE.example = let view = &value

BORROWCK_E_MOVE_AFTER_BORROW.summary = deplacement after emprunt.
BORROWCK_E_MOVE_AFTER_BORROW.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_MOVE_AFTER_BORROW.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_MOVE_AFTER_BORROW.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_MOVE_AFTER_BORROW.example = let view = &value

BORROWCK_E_MUTABLE_ALIAS.summary = mutable alias.
BORROWCK_E_MUTABLE_ALIAS.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_MUTABLE_ALIAS.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_MUTABLE_ALIAS.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_MUTABLE_ALIAS.example = let view = &value

BORROWCK_E_INTERNAL.summary = interne.
BORROWCK_E_INTERNAL.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_INTERNAL.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_INTERNAL.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_INTERNAL.example = let view = &value

BORROWCK_E_UNKNOWN.summary = inconnu.
BORROWCK_E_UNKNOWN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_UNKNOWN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_UNKNOWN.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_UNKNOWN.example = let view = &value

CONST_EVAL_E_DIVISION_BY_ZERO.summary = division by zero in constante evaluation.
CONST_EVAL_E_DIVISION_BY_ZERO.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_EVAL_E_DIVISION_BY_ZERO.step1 = Reduce the constant expression at the reported span.
CONST_EVAL_E_DIVISION_BY_ZERO.fix = change the const divisor to a non-zero value or guard the expression before const evaluation
CONST_EVAL_E_DIVISION_BY_ZERO.example = const size: int = 4

CONST_EVAL_E_UNKNOWN_NAME.summary = nom inconnu.
CONST_EVAL_E_UNKNOWN_NAME.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_EVAL_E_UNKNOWN_NAME.step1 = Reduce the constant expression at the reported span.
CONST_EVAL_E_UNKNOWN_NAME.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_EVAL_E_UNKNOWN_NAME.example = const size: int = 4

CONST_EVAL_E_NON_CONST_CALL.summary = non const appel.
CONST_EVAL_E_NON_CONST_CALL.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_EVAL_E_NON_CONST_CALL.step1 = Reduce the constant expression at the reported span.
CONST_EVAL_E_NON_CONST_CALL.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_EVAL_E_NON_CONST_CALL.example = const size: int = 4

CONST_EVAL_E_MUTATION_IN_CONST.summary = mutation in const.
CONST_EVAL_E_MUTATION_IN_CONST.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_EVAL_E_MUTATION_IN_CONST.step1 = Reduce the constant expression at the reported span.
CONST_EVAL_E_MUTATION_IN_CONST.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_EVAL_E_MUTATION_IN_CONST.example = const size: int = 4

CONST_EVAL_E_UNSUPPORTED_EXPR.summary = expr non pris en charge.
CONST_EVAL_E_UNSUPPORTED_EXPR.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_EVAL_E_UNSUPPORTED_EXPR.step1 = Reduce the constant expression at the reported span.
CONST_EVAL_E_UNSUPPORTED_EXPR.fix = replace the const expression with literals, supported arithmetic, or another compile-time value
CONST_EVAL_E_UNSUPPORTED_EXPR.example = const size: int = 4

CONST_EVAL_E_OVERFLOW.summary = depassement.
CONST_EVAL_E_OVERFLOW.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_EVAL_E_OVERFLOW.step1 = Reduce the constant expression at the reported span.
CONST_EVAL_E_OVERFLOW.fix = use a wider const type or reduce the arithmetic result before overflow
CONST_EVAL_E_OVERFLOW.example = const size: int = 4

CONST_EVAL_E_STATIC_ASSERT_FAILED.summary = static assertion echec.
CONST_EVAL_E_STATIC_ASSERT_FAILED.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_EVAL_E_STATIC_ASSERT_FAILED.step1 = Reduce the constant expression at the reported span.
CONST_EVAL_E_STATIC_ASSERT_FAILED.fix = make the asserted const condition true or remove the invalid compile-time assumption
CONST_EVAL_E_STATIC_ASSERT_FAILED.example = const size: int = 4

CONST_EVAL_E_CYCLE.summary = cycle.
CONST_EVAL_E_CYCLE.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_EVAL_E_CYCLE.step1 = Reduce the constant expression at the reported span.
CONST_EVAL_E_CYCLE.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_EVAL_E_CYCLE.example = const size: int = 4

CONST_EVAL_E_PARSE.summary = parse.
CONST_EVAL_E_PARSE.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_EVAL_E_PARSE.step1 = Reduce the constant expression at the reported span.
CONST_EVAL_E_PARSE.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_EVAL_E_PARSE.example = const size: int = 4

CONST_EVAL_E_UNKNOWN.summary = inconnu.
CONST_EVAL_E_UNKNOWN.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_EVAL_E_UNKNOWN.step1 = Reduce the constant expression at the reported span.
CONST_EVAL_E_UNKNOWN.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_EVAL_E_UNKNOWN.example = const size: int = 4

MOD_E_MODULE_NOT_FOUND.summary = module manquant.
MOD_E_MODULE_NOT_FOUND.cause = The general phase found code that violates this diagnostic rule.
MOD_E_MODULE_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
MOD_E_MODULE_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
MOD_E_MODULE_NOT_FOUND.example = vitte check path/to/file.vit

MOD_E_IMPORT_CYCLE.summary = import cycle detected.
MOD_E_IMPORT_CYCLE.cause = The general phase found code that violates this diagnostic rule.
MOD_E_IMPORT_CYCLE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MOD_E_IMPORT_CYCLE.fix = repair the highlighted compiler contract before checking later diagnostics
MOD_E_IMPORT_CYCLE.example = vitte check path/to/file.vit

MOD_E_SYMBOL_NOT_EXPORTED.summary = symbole is non exporte par module.
MOD_E_SYMBOL_NOT_EXPORTED.cause = The general phase found code that violates this diagnostic rule.
MOD_E_SYMBOL_NOT_EXPORTED.step1 = Fix the first span reported for this diagnostic, then run the command again.
MOD_E_SYMBOL_NOT_EXPORTED.fix = repair the highlighted compiler contract before checking later diagnostics
MOD_E_SYMBOL_NOT_EXPORTED.example = vitte check path/to/file.vit

MOD_E_IMPORT_NOT_FOUND.summary = import manquant.
MOD_E_IMPORT_NOT_FOUND.cause = The general phase found code that violates this diagnostic rule.
MOD_E_IMPORT_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
MOD_E_IMPORT_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
MOD_E_IMPORT_NOT_FOUND.example = vitte check path/to/file.vit

MOD_E_PACKAGE_MISSING.summary = package manquant.
MOD_E_PACKAGE_MISSING.cause = The general phase found code that violates this diagnostic rule.
MOD_E_PACKAGE_MISSING.step1 = Fix the first span reported for this diagnostic, then run the command again.
MOD_E_PACKAGE_MISSING.fix = repair the highlighted compiler contract before checking later diagnostics
MOD_E_PACKAGE_MISSING.example = vitte check path/to/file.vit

MOD_E_STDLIB_MISSING.summary = stdlib manquant.
MOD_E_STDLIB_MISSING.cause = The general phase found code that violates this diagnostic rule.
MOD_E_STDLIB_MISSING.step1 = Fix the first span reported for this diagnostic, then run the command again.
MOD_E_STDLIB_MISSING.fix = repair the highlighted compiler contract before checking later diagnostics
MOD_E_STDLIB_MISSING.example = vitte check path/to/file.vit

MOD_E_AMBIGUOUS_MODULE.summary = ambiguous module chemin.
MOD_E_AMBIGUOUS_MODULE.cause = The general phase found code that violates this diagnostic rule.
MOD_E_AMBIGUOUS_MODULE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MOD_E_AMBIGUOUS_MODULE.fix = repair the highlighted compiler contract before checking later diagnostics
MOD_E_AMBIGUOUS_MODULE.example = vitte check path/to/file.vit

HIR_E_INVALID_EXPR.summary = expr invalide.
HIR_E_INVALID_EXPR.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_EXPR.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_EXPR.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_EXPR.example = vitte check path/to/file.vit

HIR_E_INVALID_STMT.summary = stmt invalide.
HIR_E_INVALID_STMT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_STMT.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_STMT.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_STMT.example = vitte check path/to/file.vit

HIR_E_INVALID_PATTERN.summary = motif invalide.
HIR_E_INVALID_PATTERN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_PATTERN.example = vitte check path/to/file.vit

HIR_E_INVALID_TYPE.summary = type invalide.
HIR_E_INVALID_TYPE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_TYPE.example = vitte check path/to/file.vit

HIR_E_MISSING_SYMBOL.summary = symbole manquant.
HIR_E_MISSING_SYMBOL.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_MISSING_SYMBOL.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_MISSING_SYMBOL.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_MISSING_SYMBOL.example = vitte check path/to/file.vit

HIR_E_INVALID_CONTROL_FLOW.summary = control flux invalide.
HIR_E_INVALID_CONTROL_FLOW.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_CONTROL_FLOW.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_CONTROL_FLOW.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_CONTROL_FLOW.example = vitte check path/to/file.vit

HIR_E_LOWERING_FAILED.summary = lowering echec.
HIR_E_LOWERING_FAILED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_LOWERING_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_LOWERING_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_LOWERING_FAILED.example = vitte check path/to/file.vit

MIR_E_INVALID_BLOCK.summary = bloc invalide.
MIR_E_INVALID_BLOCK.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_BLOCK.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_BLOCK.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_BLOCK.example = vitte check path/to/file.vit

MIR_E_INVALID_TERMINATOR.summary = terminateur invalide.
MIR_E_INVALID_TERMINATOR.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_TERMINATOR.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_TERMINATOR.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_TERMINATOR.example = vitte check path/to/file.vit

MIR_E_UNREACHABLE_BLOCK.summary = inatteignable bloc.
MIR_E_UNREACHABLE_BLOCK.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_UNREACHABLE_BLOCK.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_UNREACHABLE_BLOCK.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_UNREACHABLE_BLOCK.example = vitte check path/to/file.vit

MIR_E_INVALID_OPERAND.summary = operande invalide.
MIR_E_INVALID_OPERAND.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_OPERAND.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_OPERAND.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_OPERAND.example = vitte check path/to/file.vit

MIR_E_INVALID_PLACE.summary = place invalide.
MIR_E_INVALID_PLACE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_PLACE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_PLACE.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_PLACE.example = vitte check path/to/file.vit

MIR_E_DATAFLOW_CONFLICT.summary = dataflow conflit.
MIR_E_DATAFLOW_CONFLICT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_DATAFLOW_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_DATAFLOW_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_DATAFLOW_CONFLICT.example = vitte check path/to/file.vit

MIR_E_VERIFICATION_FAILED.summary = verification echec.
MIR_E_VERIFICATION_FAILED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_VERIFICATION_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_VERIFICATION_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_VERIFICATION_FAILED.example = vitte check path/to/file.vit

IR_E_INVALID_MODULE.summary = module invalide.
IR_E_INVALID_MODULE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_MODULE.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_MODULE.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_MODULE.example = vitte check path/to/file.vit

IR_E_INVALID_FUNCTION.summary = function invalide.
IR_E_INVALID_FUNCTION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_FUNCTION.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_FUNCTION.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_FUNCTION.example = vitte check path/to/file.vit

IR_E_INVALID_BLOCK.summary = bloc invalide.
IR_E_INVALID_BLOCK.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_BLOCK.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_BLOCK.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_BLOCK.example = vitte check path/to/file.vit

IR_E_INVALID_INSTRUCTION.summary = instruction invalide.
IR_E_INVALID_INSTRUCTION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_INSTRUCTION.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_INSTRUCTION.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_INSTRUCTION.example = vitte check path/to/file.vit

IR_E_TYPE_MISMATCH.summary = type incompatibilite.
IR_E_TYPE_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_TYPE_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_TYPE_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_TYPE_MISMATCH.example = vitte check path/to/file.vit

IR_E_VERIFY_FAILED.summary = verify echec.
IR_E_VERIFY_FAILED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_VERIFY_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_VERIFY_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_VERIFY_FAILED.example = vitte check path/to/file.vit

BACKEND_E_UNSUPPORTED_TARGET.summary = cible non pris en charge.
BACKEND_E_UNSUPPORTED_TARGET.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_UNSUPPORTED_TARGET.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_UNSUPPORTED_TARGET.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_UNSUPPORTED_TARGET.example = vitte build app.vit -o app

BACKEND_E_UNSUPPORTED_FEATURE.summary = fonctionnalite non pris en charge.
BACKEND_E_UNSUPPORTED_FEATURE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_UNSUPPORTED_FEATURE.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_UNSUPPORTED_FEATURE.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_UNSUPPORTED_FEATURE.example = vitte build app.vit -o app

BACKEND_E_CODEGEN_FAILED.summary = generation de code echec.
BACKEND_E_CODEGEN_FAILED.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_CODEGEN_FAILED.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_CODEGEN_FAILED.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_CODEGEN_FAILED.example = vitte build app.vit -o app

BACKEND_E_OBJECT_WRITE_FAILED.summary = objet ecriture echec.
BACKEND_E_OBJECT_WRITE_FAILED.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_OBJECT_WRITE_FAILED.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_OBJECT_WRITE_FAILED.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_OBJECT_WRITE_FAILED.example = vitte build app.vit -o app

BACKEND_E_ASSEMBLER_FAILED.summary = assembler echec.
BACKEND_E_ASSEMBLER_FAILED.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_ASSEMBLER_FAILED.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_ASSEMBLER_FAILED.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_ASSEMBLER_FAILED.example = vitte build app.vit -o app

BACKEND_E_ABI_MISMATCH.summary = abi incompatibilite.
BACKEND_E_ABI_MISMATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_ABI_MISMATCH.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_ABI_MISMATCH.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_ABI_MISMATCH.example = vitte build app.vit -o app

BACKEND_E_NATIVE_TOOL_MISSING.summary = native outil manquant.
BACKEND_E_NATIVE_TOOL_MISSING.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_NATIVE_TOOL_MISSING.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_NATIVE_TOOL_MISSING.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_NATIVE_TOOL_MISSING.example = vitte build app.vit -o app

LINK_E_UNDEFINED_SYMBOL.summary = indefini symbole.
LINK_E_UNDEFINED_SYMBOL.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_UNDEFINED_SYMBOL.step1 = Check the target triple and the first backend or linker note.
LINK_E_UNDEFINED_SYMBOL.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_UNDEFINED_SYMBOL.example = vitte build app.vit -o app

LINK_E_DUPLICATE_SYMBOL.summary = symbole duplique.
LINK_E_DUPLICATE_SYMBOL.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_DUPLICATE_SYMBOL.step1 = Check the target triple and the first backend or linker note.
LINK_E_DUPLICATE_SYMBOL.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_DUPLICATE_SYMBOL.example = vitte build app.vit -o app

LINK_E_LIBRARY_NOT_FOUND.summary = library manquant.
LINK_E_LIBRARY_NOT_FOUND.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_LIBRARY_NOT_FOUND.step1 = Check the target triple and the first backend or linker note.
LINK_E_LIBRARY_NOT_FOUND.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_LIBRARY_NOT_FOUND.example = vitte build app.vit -o app

LINK_E_OBJECT_NOT_FOUND.summary = objet manquant.
LINK_E_OBJECT_NOT_FOUND.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_OBJECT_NOT_FOUND.step1 = Check the target triple and the first backend or linker note.
LINK_E_OBJECT_NOT_FOUND.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_OBJECT_NOT_FOUND.example = vitte build app.vit -o app

LINK_E_UNSUPPORTED_FORMAT.summary = format non pris en charge.
LINK_E_UNSUPPORTED_FORMAT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_UNSUPPORTED_FORMAT.step1 = Check the target triple and the first backend or linker note.
LINK_E_UNSUPPORTED_FORMAT.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_UNSUPPORTED_FORMAT.example = vitte build app.vit -o app

LINK_E_SYSTEM_LINKER_FAILED.summary = system linker echec.
LINK_E_SYSTEM_LINKER_FAILED.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_SYSTEM_LINKER_FAILED.step1 = Check the target triple and the first backend or linker note.
LINK_E_SYSTEM_LINKER_FAILED.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_SYSTEM_LINKER_FAILED.example = vitte build app.vit -o app

RUNTIME_E_PANIC.summary = panique.
RUNTIME_E_PANIC.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_PANIC.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_PANIC.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_PANIC.example = vitte check path/to/file.vit

RUNTIME_E_ASSERT_FAILED.summary = assertion echec.
RUNTIME_E_ASSERT_FAILED.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_ASSERT_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_ASSERT_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_ASSERT_FAILED.example = vitte check path/to/file.vit

RUNTIME_E_STACK_OVERFLOW.summary = pile depassement.
RUNTIME_E_STACK_OVERFLOW.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_STACK_OVERFLOW.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_STACK_OVERFLOW.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_STACK_OVERFLOW.example = vitte check path/to/file.vit

RUNTIME_E_OUT_OF_MEMORY.summary = out of memory.
RUNTIME_E_OUT_OF_MEMORY.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_OUT_OF_MEMORY.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_OUT_OF_MEMORY.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_OUT_OF_MEMORY.example = vitte check path/to/file.vit

RUNTIME_E_DIVISION_BY_ZERO.summary = division by zero.
RUNTIME_E_DIVISION_BY_ZERO.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_DIVISION_BY_ZERO.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_DIVISION_BY_ZERO.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_DIVISION_BY_ZERO.example = vitte check path/to/file.vit

RUNTIME_E_BOUNDS_CHECK.summary = bounds check.
RUNTIME_E_BOUNDS_CHECK.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_BOUNDS_CHECK.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_BOUNDS_CHECK.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_BOUNDS_CHECK.example = vitte check path/to/file.vit

RUNTIME_E_NULL_DEREF.summary = nul deref.
RUNTIME_E_NULL_DEREF.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_NULL_DEREF.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_NULL_DEREF.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_NULL_DEREF.example = vitte check path/to/file.vit

DRIVER_E_INVALID_ARGUMENT.summary = argument invalide.
DRIVER_E_INVALID_ARGUMENT.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_ARGUMENT.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_ARGUMENT.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_ARGUMENT.example = vitte check src/main.vit --lang en

DRIVER_E_MISSING_INPUT.summary = entree manquant.
DRIVER_E_MISSING_INPUT.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_MISSING_INPUT.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_MISSING_INPUT.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_MISSING_INPUT.example = vitte check src/main.vit --lang en

DRIVER_E_INPUT_NOT_FOUND.summary = entree manquant.
DRIVER_E_INPUT_NOT_FOUND.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INPUT_NOT_FOUND.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INPUT_NOT_FOUND.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INPUT_NOT_FOUND.example = vitte check src/main.vit --lang en

DRIVER_E_OUTPUT_WRITE_FAILED.summary = sortie ecriture echec.
DRIVER_E_OUTPUT_WRITE_FAILED.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_OUTPUT_WRITE_FAILED.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_OUTPUT_WRITE_FAILED.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_OUTPUT_WRITE_FAILED.example = vitte check src/main.vit --lang en

DRIVER_E_CACHE_READ_FAILED.summary = cache read echec.
DRIVER_E_CACHE_READ_FAILED.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_CACHE_READ_FAILED.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_CACHE_READ_FAILED.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_CACHE_READ_FAILED.example = vitte check src/main.vit --lang en

DRIVER_E_CACHE_WRITE_FAILED.summary = cache ecriture echec.
DRIVER_E_CACHE_WRITE_FAILED.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_CACHE_WRITE_FAILED.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_CACHE_WRITE_FAILED.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_CACHE_WRITE_FAILED.example = vitte check src/main.vit --lang en

DRIVER_E_PROFILE_NOT_FOUND.summary = profil manquant.
DRIVER_E_PROFILE_NOT_FOUND.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_PROFILE_NOT_FOUND.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_PROFILE_NOT_FOUND.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_PROFILE_NOT_FOUND.example = vitte check src/main.vit --lang en

DRIVER_E_TARGET_NOT_FOUND.summary = cible manquant.
DRIVER_E_TARGET_NOT_FOUND.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_TARGET_NOT_FOUND.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_TARGET_NOT_FOUND.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_TARGET_NOT_FOUND.example = vitte check src/main.vit --lang en

BOOTSTRAP_E_STAGE_FAILURE.summary = stage failure.
BOOTSTRAP_E_STAGE_FAILURE.cause = The bootstrap phase found code that violates this diagnostic rule.
BOOTSTRAP_E_STAGE_FAILURE.step1 = Fix the first span reported for this diagnostic, then run the command again.
BOOTSTRAP_E_STAGE_FAILURE.fix = repair the highlighted compiler contract before checking later diagnostics
BOOTSTRAP_E_STAGE_FAILURE.example = vitte check path/to/file.vit

BOOTSTRAP_E_SEED_MISSING.summary = seed manquant.
BOOTSTRAP_E_SEED_MISSING.cause = The bootstrap phase found code that violates this diagnostic rule.
BOOTSTRAP_E_SEED_MISSING.step1 = Fix the first span reported for this diagnostic, then run the command again.
BOOTSTRAP_E_SEED_MISSING.fix = repair the highlighted compiler contract before checking later diagnostics
BOOTSTRAP_E_SEED_MISSING.example = vitte check path/to/file.vit

BOOTSTRAP_E_COMPILER_MISSING.summary = compiler manquant.
BOOTSTRAP_E_COMPILER_MISSING.cause = The bootstrap phase found code that violates this diagnostic rule.
BOOTSTRAP_E_COMPILER_MISSING.step1 = Fix the first span reported for this diagnostic, then run the command again.
BOOTSTRAP_E_COMPILER_MISSING.fix = repair the highlighted compiler contract before checking later diagnostics
BOOTSTRAP_E_COMPILER_MISSING.example = vitte check path/to/file.vit

BOOTSTRAP_E_SELF_CHECK_FAILED.summary = self check echec.
BOOTSTRAP_E_SELF_CHECK_FAILED.cause = The bootstrap phase found code that violates this diagnostic rule.
BOOTSTRAP_E_SELF_CHECK_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
BOOTSTRAP_E_SELF_CHECK_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
BOOTSTRAP_E_SELF_CHECK_FAILED.example = vitte check path/to/file.vit

BOOTSTRAP_E_ARTIFACT_INVALID.summary = artifact invalide.
BOOTSTRAP_E_ARTIFACT_INVALID.cause = The bootstrap phase found code that violates this diagnostic rule.
BOOTSTRAP_E_ARTIFACT_INVALID.step1 = Fix the first span reported for this diagnostic, then run the command again.
BOOTSTRAP_E_ARTIFACT_INVALID.fix = repair the highlighted compiler contract before checking later diagnostics
BOOTSTRAP_E_ARTIFACT_INVALID.example = vitte check path/to/file.vit

E_BOOTSTRAP_CONST_TYPE.summary = bootstrap constante has wrong type.
E_BOOTSTRAP_CONST_TYPE.cause = The general phase found code that violates this diagnostic rule.
E_BOOTSTRAP_CONST_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
E_BOOTSTRAP_CONST_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
E_BOOTSTRAP_CONST_TYPE.example = vitte check path/to/file.vit

E_BOOTSTRAP_DUP_PROC.summary = bootstrap procedure duplique.
E_BOOTSTRAP_DUP_PROC.cause = The general phase found code that violates this diagnostic rule.
E_BOOTSTRAP_DUP_PROC.step1 = Fix the first span reported for this diagnostic, then run the command again.
E_BOOTSTRAP_DUP_PROC.fix = repair the highlighted compiler contract before checking later diagnostics
E_BOOTSTRAP_DUP_PROC.example = vitte check path/to/file.vit

E_BOOTSTRAP_UNKNOWN_CONST.summary = bootstrap constante inconnu.
E_BOOTSTRAP_UNKNOWN_CONST.cause = The general phase found code that violates this diagnostic rule.
E_BOOTSTRAP_UNKNOWN_CONST.step1 = Fix the first span reported for this diagnostic, then run the command again.
E_BOOTSTRAP_UNKNOWN_CONST.fix = repair the highlighted compiler contract before checking later diagnostics
E_BOOTSTRAP_UNKNOWN_CONST.example = vitte check path/to/file.vit

E_BOOTSTRAP_UNKNOWN_PROC.summary = bootstrap procedure non pris en charge.
E_BOOTSTRAP_UNKNOWN_PROC.cause = The general phase found code that violates this diagnostic rule.
E_BOOTSTRAP_UNKNOWN_PROC.step1 = Fix the first span reported for this diagnostic, then run the command again.
E_BOOTSTRAP_UNKNOWN_PROC.fix = repair the highlighted compiler contract before checking later diagnostics
E_BOOTSTRAP_UNKNOWN_PROC.example = vitte check path/to/file.vit

LIMIT_FILE_SIZE_MAX.summary = fichier size max.
LIMIT_FILE_SIZE_MAX.cause = The input exceeded a configured compiler safety limit.
LIMIT_FILE_SIZE_MAX.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_FILE_SIZE_MAX.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_FILE_SIZE_MAX.example = vitte check src/main.vit

LIMIT_TOKEN_SIZE_MAX.summary = jeton size max.
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

LIMIT_IMPORT_DEPTH_MAX.summary = import depth max.
LIMIT_IMPORT_DEPTH_MAX.cause = The input exceeded a configured compiler safety limit.
LIMIT_IMPORT_DEPTH_MAX.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_IMPORT_DEPTH_MAX.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_IMPORT_DEPTH_MAX.example = vitte check src/main.vit

LIMIT_MODULE_COUNT_MAX.summary = module count max.
LIMIT_MODULE_COUNT_MAX.cause = The input exceeded a configured compiler safety limit.
LIMIT_MODULE_COUNT_MAX.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_MODULE_COUNT_MAX.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_MODULE_COUNT_MAX.example = vitte check src/main.vit

LIMIT_DIAGNOSTICS_MAX.summary = trop many diagnostics emitted.
LIMIT_DIAGNOSTICS_MAX.cause = The input exceeded a configured compiler safety limit.
LIMIT_DIAGNOSTICS_MAX.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_DIAGNOSTICS_MAX.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_DIAGNOSTICS_MAX.example = vitte check src/main.vit

LIMIT_SYMBOL_COUNT_MAX.summary = symbole count max.
LIMIT_SYMBOL_COUNT_MAX.cause = The input exceeded a configured compiler safety limit.
LIMIT_SYMBOL_COUNT_MAX.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_SYMBOL_COUNT_MAX.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_SYMBOL_COUNT_MAX.example = vitte check src/main.vit

LIMIT_PARSER_RECURSION_MAX.summary = parseur recursion max.
LIMIT_PARSER_RECURSION_MAX.cause = The input exceeded a configured compiler safety limit.
LIMIT_PARSER_RECURSION_MAX.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_PARSER_RECURSION_MAX.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_PARSER_RECURSION_MAX.example = vitte check src/main.vit

LIMIT_MACRO_EXPANSION_MAX.summary = macro expansion max.
LIMIT_MACRO_EXPANSION_MAX.cause = The input exceeded a configured compiler safety limit.
LIMIT_MACRO_EXPANSION_MAX.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_MACRO_EXPANSION_MAX.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_MACRO_EXPANSION_MAX.example = vitte check src/main.vit

MACRO_E_EXPANSION_FAILED.summary = expansion echec.
MACRO_E_EXPANSION_FAILED.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_EXPANSION_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_EXPANSION_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_EXPANSION_FAILED.example = vitte check path/to/file.vit

MACRO_E_RECURSION_LIMIT.summary = recursion limit.
MACRO_E_RECURSION_LIMIT.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_RECURSION_LIMIT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_RECURSION_LIMIT.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_RECURSION_LIMIT.example = vitte check path/to/file.vit

MACRO_E_INVALID_ARGUMENT.summary = argument invalide.
MACRO_E_INVALID_ARGUMENT.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_ARGUMENT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_ARGUMENT.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_ARGUMENT.example = vitte check path/to/file.vit

MACRO_E_UNKNOWN_MACRO.summary = macro inconnu.
MACRO_E_UNKNOWN_MACRO.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_UNKNOWN_MACRO.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_UNKNOWN_MACRO.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_UNKNOWN_MACRO.example = vitte check path/to/file.vit

MACRO_E_UNSTABLE_FEATURE.summary = unstable fonctionnalite.
MACRO_E_UNSTABLE_FEATURE.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_UNSTABLE_FEATURE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_UNSTABLE_FEATURE.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_UNSTABLE_FEATURE.example = vitte check path/to/file.vit

SYNTAX_E_EXPECTED_IDENTIFIER.summary = identifiant attendu.
SYNTAX_E_EXPECTED_IDENTIFIER.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_EXPECTED_IDENTIFIER.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_EXPECTED_IDENTIFIER.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_EXPECTED_IDENTIFIER.example = proc main() -> int { give 0; }

SYNTAX_E_EXPECTED_EXPRESSION.summary = expression attendu.
SYNTAX_E_EXPECTED_EXPRESSION.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_EXPECTED_EXPRESSION.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_EXPECTED_EXPRESSION.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_EXPECTED_EXPRESSION.example = proc main() -> int { give 0; }

SYNTAX_E_EXPECTED_TYPE.summary = type attendu.
SYNTAX_E_EXPECTED_TYPE.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_EXPECTED_TYPE.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_EXPECTED_TYPE.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_EXPECTED_TYPE.example = proc main() -> int { give 0; }

SYNTAX_E_EXPECTED_PATTERN.summary = motif attendu.
SYNTAX_E_EXPECTED_PATTERN.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_EXPECTED_PATTERN.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_EXPECTED_PATTERN.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_EXPECTED_PATTERN.example = proc main() -> int { give 0; }

SYNTAX_E_EXPECTED_BLOCK.summary = bloc attendu.
SYNTAX_E_EXPECTED_BLOCK.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_EXPECTED_BLOCK.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_EXPECTED_BLOCK.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_EXPECTED_BLOCK.example = proc main() -> int { give 0; }

SYNTAX_E_EXPECTED_DELIMITER.summary = delimiteur attendu.
SYNTAX_E_EXPECTED_DELIMITER.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_EXPECTED_DELIMITER.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_EXPECTED_DELIMITER.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_EXPECTED_DELIMITER.example = proc main() -> int { give 0; }

SYNTAX_E_UNEXPECTED_TOKEN.summary = jeton inattendu.
SYNTAX_E_UNEXPECTED_TOKEN.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_UNEXPECTED_TOKEN.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_UNEXPECTED_TOKEN.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_UNEXPECTED_TOKEN.example = proc main() -> int { give 0; }

SYNTAX_E_UNBALANCED_DELIMITER.summary = unbalanced delimiteur.
SYNTAX_E_UNBALANCED_DELIMITER.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_UNBALANCED_DELIMITER.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_UNBALANCED_DELIMITER.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_UNBALANCED_DELIMITER.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_ATTRIBUTE.summary = attribute invalide.
SYNTAX_E_INVALID_ATTRIBUTE.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_ATTRIBUTE.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_ATTRIBUTE.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_ATTRIBUTE.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_DECLARATION.summary = declaration invalide.
SYNTAX_E_INVALID_DECLARATION.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_DECLARATION.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_DECLARATION.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_DECLARATION.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_STATEMENT.summary = instruction invalide.
SYNTAX_E_INVALID_STATEMENT.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_STATEMENT.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_STATEMENT.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_STATEMENT.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_EXPRESSION.summary = expression invalide.
SYNTAX_E_INVALID_EXPRESSION.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_EXPRESSION.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_EXPRESSION.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_EXPRESSION.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_PATTERN.summary = motif invalide.
SYNTAX_E_INVALID_PATTERN.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_PATTERN.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_PATTERN.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_PATTERN.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_LITERAL.summary = litteral invalide.
SYNTAX_E_INVALID_LITERAL.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_LITERAL.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_LITERAL.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_LITERAL.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_OPERATOR.summary = operateur invalide.
SYNTAX_E_INVALID_OPERATOR.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_OPERATOR.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_OPERATOR.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_OPERATOR.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_MODIFIER.summary = modifier invalide.
SYNTAX_E_INVALID_MODIFIER.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_MODIFIER.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_MODIFIER.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_MODIFIER.example = proc main() -> int { give 0; }

SYNTAX_E_MISSING_BODY.summary = body manquant.
SYNTAX_E_MISSING_BODY.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_MISSING_BODY.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_MISSING_BODY.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_MISSING_BODY.example = proc main() -> int { give 0; }

SYNTAX_E_MISSING_RETURN.summary = retour manquant.
SYNTAX_E_MISSING_RETURN.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_MISSING_RETURN.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_MISSING_RETURN.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_MISSING_RETURN.example = proc main() -> int { give 0; }

SYNTAX_E_DUPLICATE_NAME.summary = nom duplique.
SYNTAX_E_DUPLICATE_NAME.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_DUPLICATE_NAME.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_DUPLICATE_NAME.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_DUPLICATE_NAME.example = proc main() -> int { give 0; }

SYNTAX_E_UNKNOWN_NAME.summary = nom inconnu.
SYNTAX_E_UNKNOWN_NAME.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_UNKNOWN_NAME.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_UNKNOWN_NAME.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_UNKNOWN_NAME.example = proc main() -> int { give 0; }

SYNTAX_E_UNKNOWN_TYPE.summary = type inconnu.
SYNTAX_E_UNKNOWN_TYPE.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_UNKNOWN_TYPE.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_UNKNOWN_TYPE.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_UNKNOWN_TYPE.example = proc main() -> int { give 0; }

SYNTAX_E_UNKNOWN_MODULE.summary = module inconnu.
SYNTAX_E_UNKNOWN_MODULE.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_UNKNOWN_MODULE.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_UNKNOWN_MODULE.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_UNKNOWN_MODULE.example = proc main() -> int { give 0; }

SYNTAX_E_UNKNOWN_MEMBER.summary = membre inconnu.
SYNTAX_E_UNKNOWN_MEMBER.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_UNKNOWN_MEMBER.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_UNKNOWN_MEMBER.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_UNKNOWN_MEMBER.example = proc main() -> int { give 0; }

SYNTAX_E_AMBIGUOUS_NAME.summary = ambiguous nom.
SYNTAX_E_AMBIGUOUS_NAME.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_AMBIGUOUS_NAME.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_AMBIGUOUS_NAME.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_AMBIGUOUS_NAME.example = proc main() -> int { give 0; }

SYNTAX_E_PRIVATE_SYMBOL.summary = private symbole.
SYNTAX_E_PRIVATE_SYMBOL.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_PRIVATE_SYMBOL.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_PRIVATE_SYMBOL.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_PRIVATE_SYMBOL.example = proc main() -> int { give 0; }

SYNTAX_E_IMPORT_NOT_FOUND.summary = import manquant.
SYNTAX_E_IMPORT_NOT_FOUND.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_IMPORT_NOT_FOUND.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_IMPORT_NOT_FOUND.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_IMPORT_NOT_FOUND.example = proc main() -> int { give 0; }

SYNTAX_E_IMPORT_CYCLE.summary = import cycle.
SYNTAX_E_IMPORT_CYCLE.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_IMPORT_CYCLE.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_IMPORT_CYCLE.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_IMPORT_CYCLE.example = proc main() -> int { give 0; }

SYNTAX_E_EXPORT_CONFLICT.summary = export conflit.
SYNTAX_E_EXPORT_CONFLICT.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_EXPORT_CONFLICT.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_EXPORT_CONFLICT.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_EXPORT_CONFLICT.example = proc main() -> int { give 0; }

SYNTAX_E_ARITY_MISMATCH.summary = arity incompatibilite.
SYNTAX_E_ARITY_MISMATCH.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_ARITY_MISMATCH.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_ARITY_MISMATCH.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_ARITY_MISMATCH.example = proc main() -> int { give 0; }

SYNTAX_E_ARGUMENT_MISMATCH.summary = argument incompatibilite.
SYNTAX_E_ARGUMENT_MISMATCH.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_ARGUMENT_MISMATCH.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_ARGUMENT_MISMATCH.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_ARGUMENT_MISMATCH.example = proc main() -> int { give 0; }

SYNTAX_E_ASSIGNMENT_MISMATCH.summary = affectation incompatibilite.
SYNTAX_E_ASSIGNMENT_MISMATCH.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_ASSIGNMENT_MISMATCH.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_ASSIGNMENT_MISMATCH.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_ASSIGNMENT_MISMATCH.example = proc main() -> int { give 0; }

SYNTAX_E_BRANCH_MISMATCH.summary = branch incompatibilite.
SYNTAX_E_BRANCH_MISMATCH.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_BRANCH_MISMATCH.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_BRANCH_MISMATCH.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_BRANCH_MISMATCH.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_CALL.summary = appel invalide.
SYNTAX_E_INVALID_CALL.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_CALL.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_CALL.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_CALL.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_CAST.summary = conversion invalide.
SYNTAX_E_INVALID_CAST.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_CAST.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_CAST.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_CAST.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_INDEX.summary = index invalide.
SYNTAX_E_INVALID_INDEX.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_INDEX.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_INDEX.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_INDEX.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_DEREF.summary = deref invalide.
SYNTAX_E_INVALID_DEREF.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_DEREF.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_DEREF.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_DEREF.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_BORROW.summary = emprunt invalide.
SYNTAX_E_INVALID_BORROW.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_BORROW.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_BORROW.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_BORROW.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_MOVE.summary = deplacement invalide.
SYNTAX_E_INVALID_MOVE.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_MOVE.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_MOVE.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_MOVE.example = proc main() -> int { give 0; }

SYNTAX_E_USE_AFTER_MOVE.summary = use after deplacement.
SYNTAX_E_USE_AFTER_MOVE.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_USE_AFTER_MOVE.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_USE_AFTER_MOVE.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_USE_AFTER_MOVE.example = proc main() -> int { give 0; }

SYNTAX_E_USE_AFTER_DROP.summary = use after destruction.
SYNTAX_E_USE_AFTER_DROP.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_USE_AFTER_DROP.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_USE_AFTER_DROP.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_USE_AFTER_DROP.example = proc main() -> int { give 0; }

SYNTAX_E_USE_BEFORE_INIT.summary = use before init.
SYNTAX_E_USE_BEFORE_INIT.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_USE_BEFORE_INIT.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_USE_BEFORE_INIT.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_USE_BEFORE_INIT.example = proc main() -> int { give 0; }

SYNTAX_E_DOUBLE_DROP.summary = double destruction.
SYNTAX_E_DOUBLE_DROP.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_DOUBLE_DROP.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_DOUBLE_DROP.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_DOUBLE_DROP.example = proc main() -> int { give 0; }

SYNTAX_E_BORROW_CONFLICT.summary = emprunt conflit.
SYNTAX_E_BORROW_CONFLICT.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_BORROW_CONFLICT.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_BORROW_CONFLICT.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_BORROW_CONFLICT.example = proc main() -> int { give 0; }

SYNTAX_E_MUTABILITY_CONFLICT.summary = mutability conflit.
SYNTAX_E_MUTABILITY_CONFLICT.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_MUTABILITY_CONFLICT.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_MUTABILITY_CONFLICT.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_MUTABILITY_CONFLICT.example = proc main() -> int { give 0; }

SYNTAX_E_LIFETIME_TOO_SHORT.summary = duree de vie trop short.
SYNTAX_E_LIFETIME_TOO_SHORT.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_LIFETIME_TOO_SHORT.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_LIFETIME_TOO_SHORT.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_LIFETIME_TOO_SHORT.example = proc main() -> int { give 0; }

SYNTAX_E_DANGLING_REFERENCE.summary = dangling reference.
SYNTAX_E_DANGLING_REFERENCE.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_DANGLING_REFERENCE.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_DANGLING_REFERENCE.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_DANGLING_REFERENCE.example = proc main() -> int { give 0; }

SYNTAX_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
SYNTAX_E_NON_EXHAUSTIVE_MATCH.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_NON_EXHAUSTIVE_MATCH.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_NON_EXHAUSTIVE_MATCH.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_NON_EXHAUSTIVE_MATCH.example = proc main() -> int { give 0; }

SYNTAX_E_UNREACHABLE_PATTERN.summary = inatteignable motif.
SYNTAX_E_UNREACHABLE_PATTERN.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_UNREACHABLE_PATTERN.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_UNREACHABLE_PATTERN.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_UNREACHABLE_PATTERN.example = proc main() -> int { give 0; }

SYNTAX_E_CONST_REQUIRED.summary = const required.
SYNTAX_E_CONST_REQUIRED.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_CONST_REQUIRED.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_CONST_REQUIRED.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_CONST_REQUIRED.example = proc main() -> int { give 0; }

SYNTAX_E_CONST_OVERFLOW.summary = const depassement.
SYNTAX_E_CONST_OVERFLOW.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_CONST_OVERFLOW.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_CONST_OVERFLOW.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_CONST_OVERFLOW.example = proc main() -> int { give 0; }

SYNTAX_E_CONST_DIVISION_BY_ZERO.summary = const division by zero.
SYNTAX_E_CONST_DIVISION_BY_ZERO.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_CONST_DIVISION_BY_ZERO.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_CONST_DIVISION_BY_ZERO.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_CONST_DIVISION_BY_ZERO.example = proc main() -> int { give 0; }

SYNTAX_E_CONST_CYCLE.summary = const cycle.
SYNTAX_E_CONST_CYCLE.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_CONST_CYCLE.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_CONST_CYCLE.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_CONST_CYCLE.example = proc main() -> int { give 0; }

SYNTAX_E_MACRO_NOT_FOUND.summary = macro manquant.
SYNTAX_E_MACRO_NOT_FOUND.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_MACRO_NOT_FOUND.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_MACRO_NOT_FOUND.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_MACRO_NOT_FOUND.example = proc main() -> int { give 0; }

SYNTAX_E_MACRO_RECURSION.summary = macro recursion.
SYNTAX_E_MACRO_RECURSION.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_MACRO_RECURSION.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_MACRO_RECURSION.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_MACRO_RECURSION.example = proc main() -> int { give 0; }

SYNTAX_E_MACRO_EXPANSION_FAILED.summary = macro expansion echec.
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

SYNTAX_E_GENERIC_ARGUMENT_MISSING.summary = generique argument manquant.
SYNTAX_E_GENERIC_ARGUMENT_MISSING.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_GENERIC_ARGUMENT_MISSING.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_GENERIC_ARGUMENT_MISSING.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_GENERIC_ARGUMENT_MISSING.example = proc main() -> int { give 0; }

SYNTAX_E_GENERIC_BOUND_FAILED.summary = generique bound echec.
SYNTAX_E_GENERIC_BOUND_FAILED.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_GENERIC_BOUND_FAILED.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_GENERIC_BOUND_FAILED.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_GENERIC_BOUND_FAILED.example = proc main() -> int { give 0; }

SYNTAX_E_UNSUPPORTED_TARGET.summary = cible non pris en charge.
SYNTAX_E_UNSUPPORTED_TARGET.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_UNSUPPORTED_TARGET.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_UNSUPPORTED_TARGET.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_UNSUPPORTED_TARGET.example = proc main() -> int { give 0; }

SYNTAX_E_ABI_MISMATCH.summary = abi incompatibilite.
SYNTAX_E_ABI_MISMATCH.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_ABI_MISMATCH.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_ABI_MISMATCH.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_ABI_MISMATCH.example = proc main() -> int { give 0; }

SYNTAX_E_LINK_FAILED.summary = link echec.
SYNTAX_E_LINK_FAILED.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_LINK_FAILED.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_LINK_FAILED.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_LINK_FAILED.example = proc main() -> int { give 0; }

SYNTAX_E_RUNTIME_PANIC.summary = runtime panique.
SYNTAX_E_RUNTIME_PANIC.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_RUNTIME_PANIC.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_RUNTIME_PANIC.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_RUNTIME_PANIC.example = proc main() -> int { give 0; }

NAME_E_EXPECTED_IDENTIFIER.summary = identifiant attendu.
NAME_E_EXPECTED_IDENTIFIER.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_EXPECTED_IDENTIFIER.step1 = Check the spelling and the nearest import or declaration.
NAME_E_EXPECTED_IDENTIFIER.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_EXPECTED_IDENTIFIER.example = use vitte/core

NAME_E_EXPECTED_EXPRESSION.summary = expression attendu.
NAME_E_EXPECTED_EXPRESSION.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_EXPECTED_EXPRESSION.step1 = Check the spelling and the nearest import or declaration.
NAME_E_EXPECTED_EXPRESSION.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_EXPECTED_EXPRESSION.example = use vitte/core

NAME_E_EXPECTED_TYPE.summary = type attendu.
NAME_E_EXPECTED_TYPE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_EXPECTED_TYPE.step1 = Check the spelling and the nearest import or declaration.
NAME_E_EXPECTED_TYPE.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_EXPECTED_TYPE.example = use vitte/core

NAME_E_EXPECTED_PATTERN.summary = motif attendu.
NAME_E_EXPECTED_PATTERN.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_EXPECTED_PATTERN.step1 = Check the spelling and the nearest import or declaration.
NAME_E_EXPECTED_PATTERN.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_EXPECTED_PATTERN.example = use vitte/core

NAME_E_EXPECTED_BLOCK.summary = bloc attendu.
NAME_E_EXPECTED_BLOCK.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_EXPECTED_BLOCK.step1 = Check the spelling and the nearest import or declaration.
NAME_E_EXPECTED_BLOCK.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_EXPECTED_BLOCK.example = use vitte/core

NAME_E_EXPECTED_DELIMITER.summary = delimiteur attendu.
NAME_E_EXPECTED_DELIMITER.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_EXPECTED_DELIMITER.step1 = Check the spelling and the nearest import or declaration.
NAME_E_EXPECTED_DELIMITER.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_EXPECTED_DELIMITER.example = use vitte/core

NAME_E_UNEXPECTED_TOKEN.summary = jeton inattendu.
NAME_E_UNEXPECTED_TOKEN.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_UNEXPECTED_TOKEN.step1 = Check the spelling and the nearest import or declaration.
NAME_E_UNEXPECTED_TOKEN.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_UNEXPECTED_TOKEN.example = use vitte/core

NAME_E_UNBALANCED_DELIMITER.summary = unbalanced delimiteur.
NAME_E_UNBALANCED_DELIMITER.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_UNBALANCED_DELIMITER.step1 = Check the spelling and the nearest import or declaration.
NAME_E_UNBALANCED_DELIMITER.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_UNBALANCED_DELIMITER.example = use vitte/core

NAME_E_INVALID_ATTRIBUTE.summary = attribute invalide.
NAME_E_INVALID_ATTRIBUTE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_ATTRIBUTE.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_ATTRIBUTE.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_ATTRIBUTE.example = use vitte/core

NAME_E_INVALID_DECLARATION.summary = declaration invalide.
NAME_E_INVALID_DECLARATION.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_DECLARATION.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_DECLARATION.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_DECLARATION.example = use vitte/core

NAME_E_INVALID_STATEMENT.summary = instruction invalide.
NAME_E_INVALID_STATEMENT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_STATEMENT.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_STATEMENT.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_STATEMENT.example = use vitte/core

NAME_E_INVALID_EXPRESSION.summary = expression invalide.
NAME_E_INVALID_EXPRESSION.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_EXPRESSION.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_EXPRESSION.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_EXPRESSION.example = use vitte/core

NAME_E_INVALID_PATTERN.summary = motif invalide.
NAME_E_INVALID_PATTERN.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_PATTERN.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_PATTERN.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_PATTERN.example = use vitte/core

NAME_E_INVALID_LITERAL.summary = litteral invalide.
NAME_E_INVALID_LITERAL.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_LITERAL.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_LITERAL.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_LITERAL.example = use vitte/core

NAME_E_INVALID_OPERATOR.summary = operateur invalide.
NAME_E_INVALID_OPERATOR.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_OPERATOR.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_OPERATOR.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_OPERATOR.example = use vitte/core

NAME_E_INVALID_MODIFIER.summary = modifier invalide.
NAME_E_INVALID_MODIFIER.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_MODIFIER.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_MODIFIER.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_MODIFIER.example = use vitte/core

NAME_E_MISSING_BODY.summary = body manquant.
NAME_E_MISSING_BODY.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_MISSING_BODY.step1 = Check the spelling and the nearest import or declaration.
NAME_E_MISSING_BODY.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_MISSING_BODY.example = use vitte/core

NAME_E_MISSING_RETURN.summary = retour manquant.
NAME_E_MISSING_RETURN.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_MISSING_RETURN.step1 = Check the spelling and the nearest import or declaration.
NAME_E_MISSING_RETURN.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_MISSING_RETURN.example = use vitte/core

NAME_E_DUPLICATE_NAME.summary = nom duplique.
NAME_E_DUPLICATE_NAME.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_DUPLICATE_NAME.step1 = Check the spelling and the nearest import or declaration.
NAME_E_DUPLICATE_NAME.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_DUPLICATE_NAME.example = use vitte/core

NAME_E_UNKNOWN_NAME.summary = nom inconnu.
NAME_E_UNKNOWN_NAME.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_UNKNOWN_NAME.step1 = Check the spelling and the nearest import or declaration.
NAME_E_UNKNOWN_NAME.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_UNKNOWN_NAME.example = use vitte/core

NAME_E_UNKNOWN_TYPE.summary = type inconnu.
NAME_E_UNKNOWN_TYPE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_UNKNOWN_TYPE.step1 = Check the spelling and the nearest import or declaration.
NAME_E_UNKNOWN_TYPE.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_UNKNOWN_TYPE.example = use vitte/core

NAME_E_UNKNOWN_MODULE.summary = module inconnu.
NAME_E_UNKNOWN_MODULE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_UNKNOWN_MODULE.step1 = Check the spelling and the nearest import or declaration.
NAME_E_UNKNOWN_MODULE.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_UNKNOWN_MODULE.example = use vitte/core

NAME_E_UNKNOWN_MEMBER.summary = membre inconnu.
NAME_E_UNKNOWN_MEMBER.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_UNKNOWN_MEMBER.step1 = Check the spelling and the nearest import or declaration.
NAME_E_UNKNOWN_MEMBER.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_UNKNOWN_MEMBER.example = use vitte/core

NAME_E_AMBIGUOUS_NAME.summary = ambiguous nom.
NAME_E_AMBIGUOUS_NAME.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_AMBIGUOUS_NAME.step1 = Check the spelling and the nearest import or declaration.
NAME_E_AMBIGUOUS_NAME.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_AMBIGUOUS_NAME.example = use vitte/core

NAME_E_PRIVATE_SYMBOL.summary = private symbole.
NAME_E_PRIVATE_SYMBOL.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_PRIVATE_SYMBOL.step1 = Check the spelling and the nearest import or declaration.
NAME_E_PRIVATE_SYMBOL.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_PRIVATE_SYMBOL.example = use vitte/core

NAME_E_IMPORT_NOT_FOUND.summary = import manquant.
NAME_E_IMPORT_NOT_FOUND.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_IMPORT_NOT_FOUND.step1 = Check the spelling and the nearest import or declaration.
NAME_E_IMPORT_NOT_FOUND.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_IMPORT_NOT_FOUND.example = use vitte/core

NAME_E_IMPORT_CYCLE.summary = import cycle.
NAME_E_IMPORT_CYCLE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_IMPORT_CYCLE.step1 = Check the spelling and the nearest import or declaration.
NAME_E_IMPORT_CYCLE.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_IMPORT_CYCLE.example = use vitte/core

NAME_E_EXPORT_CONFLICT.summary = export conflit.
NAME_E_EXPORT_CONFLICT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_EXPORT_CONFLICT.step1 = Check the spelling and the nearest import or declaration.
NAME_E_EXPORT_CONFLICT.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_EXPORT_CONFLICT.example = use vitte/core

NAME_E_ARITY_MISMATCH.summary = arity incompatibilite.
NAME_E_ARITY_MISMATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_ARITY_MISMATCH.step1 = Check the spelling and the nearest import or declaration.
NAME_E_ARITY_MISMATCH.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_ARITY_MISMATCH.example = use vitte/core

NAME_E_ARGUMENT_MISMATCH.summary = argument incompatibilite.
NAME_E_ARGUMENT_MISMATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_ARGUMENT_MISMATCH.step1 = Check the spelling and the nearest import or declaration.
NAME_E_ARGUMENT_MISMATCH.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_ARGUMENT_MISMATCH.example = use vitte/core

NAME_E_ASSIGNMENT_MISMATCH.summary = affectation incompatibilite.
NAME_E_ASSIGNMENT_MISMATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_ASSIGNMENT_MISMATCH.step1 = Check the spelling and the nearest import or declaration.
NAME_E_ASSIGNMENT_MISMATCH.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_ASSIGNMENT_MISMATCH.example = use vitte/core

NAME_E_BRANCH_MISMATCH.summary = branch incompatibilite.
NAME_E_BRANCH_MISMATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_BRANCH_MISMATCH.step1 = Check the spelling and the nearest import or declaration.
NAME_E_BRANCH_MISMATCH.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_BRANCH_MISMATCH.example = use vitte/core

NAME_E_INVALID_CALL.summary = appel invalide.
NAME_E_INVALID_CALL.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_CALL.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_CALL.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_CALL.example = use vitte/core

NAME_E_INVALID_CAST.summary = conversion invalide.
NAME_E_INVALID_CAST.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_CAST.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_CAST.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_CAST.example = use vitte/core

NAME_E_INVALID_INDEX.summary = index invalide.
NAME_E_INVALID_INDEX.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_INDEX.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_INDEX.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_INDEX.example = use vitte/core

NAME_E_INVALID_DEREF.summary = deref invalide.
NAME_E_INVALID_DEREF.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_DEREF.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_DEREF.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_DEREF.example = use vitte/core

NAME_E_INVALID_BORROW.summary = emprunt invalide.
NAME_E_INVALID_BORROW.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_BORROW.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_BORROW.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_BORROW.example = use vitte/core

NAME_E_INVALID_MOVE.summary = deplacement invalide.
NAME_E_INVALID_MOVE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_MOVE.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_MOVE.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_MOVE.example = use vitte/core

NAME_E_USE_AFTER_MOVE.summary = use after deplacement.
NAME_E_USE_AFTER_MOVE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_USE_AFTER_MOVE.step1 = Check the spelling and the nearest import or declaration.
NAME_E_USE_AFTER_MOVE.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_USE_AFTER_MOVE.example = use vitte/core

NAME_E_USE_AFTER_DROP.summary = use after destruction.
NAME_E_USE_AFTER_DROP.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_USE_AFTER_DROP.step1 = Check the spelling and the nearest import or declaration.
NAME_E_USE_AFTER_DROP.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_USE_AFTER_DROP.example = use vitte/core

NAME_E_USE_BEFORE_INIT.summary = use before init.
NAME_E_USE_BEFORE_INIT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_USE_BEFORE_INIT.step1 = Check the spelling and the nearest import or declaration.
NAME_E_USE_BEFORE_INIT.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_USE_BEFORE_INIT.example = use vitte/core

NAME_E_DOUBLE_DROP.summary = double destruction.
NAME_E_DOUBLE_DROP.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_DOUBLE_DROP.step1 = Check the spelling and the nearest import or declaration.
NAME_E_DOUBLE_DROP.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_DOUBLE_DROP.example = use vitte/core

NAME_E_BORROW_CONFLICT.summary = emprunt conflit.
NAME_E_BORROW_CONFLICT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_BORROW_CONFLICT.step1 = Check the spelling and the nearest import or declaration.
NAME_E_BORROW_CONFLICT.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_BORROW_CONFLICT.example = use vitte/core

NAME_E_MUTABILITY_CONFLICT.summary = mutability conflit.
NAME_E_MUTABILITY_CONFLICT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_MUTABILITY_CONFLICT.step1 = Check the spelling and the nearest import or declaration.
NAME_E_MUTABILITY_CONFLICT.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_MUTABILITY_CONFLICT.example = use vitte/core

NAME_E_LIFETIME_TOO_SHORT.summary = duree de vie trop short.
NAME_E_LIFETIME_TOO_SHORT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_LIFETIME_TOO_SHORT.step1 = Check the spelling and the nearest import or declaration.
NAME_E_LIFETIME_TOO_SHORT.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_LIFETIME_TOO_SHORT.example = use vitte/core

NAME_E_DANGLING_REFERENCE.summary = dangling reference.
NAME_E_DANGLING_REFERENCE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_DANGLING_REFERENCE.step1 = Check the spelling and the nearest import or declaration.
NAME_E_DANGLING_REFERENCE.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_DANGLING_REFERENCE.example = use vitte/core

NAME_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
NAME_E_NON_EXHAUSTIVE_MATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_NON_EXHAUSTIVE_MATCH.step1 = Check the spelling and the nearest import or declaration.
NAME_E_NON_EXHAUSTIVE_MATCH.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_NON_EXHAUSTIVE_MATCH.example = use vitte/core

NAME_E_UNREACHABLE_PATTERN.summary = inatteignable motif.
NAME_E_UNREACHABLE_PATTERN.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_UNREACHABLE_PATTERN.step1 = Check the spelling and the nearest import or declaration.
NAME_E_UNREACHABLE_PATTERN.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_UNREACHABLE_PATTERN.example = use vitte/core

NAME_E_CONST_REQUIRED.summary = const required.
NAME_E_CONST_REQUIRED.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_CONST_REQUIRED.step1 = Check the spelling and the nearest import or declaration.
NAME_E_CONST_REQUIRED.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_CONST_REQUIRED.example = use vitte/core

NAME_E_CONST_OVERFLOW.summary = const depassement.
NAME_E_CONST_OVERFLOW.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_CONST_OVERFLOW.step1 = Check the spelling and the nearest import or declaration.
NAME_E_CONST_OVERFLOW.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_CONST_OVERFLOW.example = use vitte/core

NAME_E_CONST_DIVISION_BY_ZERO.summary = const division by zero.
NAME_E_CONST_DIVISION_BY_ZERO.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_CONST_DIVISION_BY_ZERO.step1 = Check the spelling and the nearest import or declaration.
NAME_E_CONST_DIVISION_BY_ZERO.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_CONST_DIVISION_BY_ZERO.example = use vitte/core

NAME_E_CONST_CYCLE.summary = const cycle.
NAME_E_CONST_CYCLE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_CONST_CYCLE.step1 = Check the spelling and the nearest import or declaration.
NAME_E_CONST_CYCLE.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_CONST_CYCLE.example = use vitte/core

NAME_E_MACRO_NOT_FOUND.summary = macro manquant.
NAME_E_MACRO_NOT_FOUND.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_MACRO_NOT_FOUND.step1 = Check the spelling and the nearest import or declaration.
NAME_E_MACRO_NOT_FOUND.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_MACRO_NOT_FOUND.example = use vitte/core

NAME_E_MACRO_RECURSION.summary = macro recursion.
NAME_E_MACRO_RECURSION.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_MACRO_RECURSION.step1 = Check the spelling and the nearest import or declaration.
NAME_E_MACRO_RECURSION.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_MACRO_RECURSION.example = use vitte/core

NAME_E_MACRO_EXPANSION_FAILED.summary = macro expansion echec.
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

NAME_E_GENERIC_ARGUMENT_MISSING.summary = generique argument manquant.
NAME_E_GENERIC_ARGUMENT_MISSING.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_GENERIC_ARGUMENT_MISSING.step1 = Check the spelling and the nearest import or declaration.
NAME_E_GENERIC_ARGUMENT_MISSING.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_GENERIC_ARGUMENT_MISSING.example = use vitte/core

NAME_E_GENERIC_BOUND_FAILED.summary = generique bound echec.
NAME_E_GENERIC_BOUND_FAILED.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_GENERIC_BOUND_FAILED.step1 = Check the spelling and the nearest import or declaration.
NAME_E_GENERIC_BOUND_FAILED.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_GENERIC_BOUND_FAILED.example = use vitte/core

NAME_E_UNSUPPORTED_TARGET.summary = cible non pris en charge.
NAME_E_UNSUPPORTED_TARGET.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_UNSUPPORTED_TARGET.step1 = Check the spelling and the nearest import or declaration.
NAME_E_UNSUPPORTED_TARGET.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_UNSUPPORTED_TARGET.example = use vitte/core

NAME_E_ABI_MISMATCH.summary = abi incompatibilite.
NAME_E_ABI_MISMATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_ABI_MISMATCH.step1 = Check the spelling and the nearest import or declaration.
NAME_E_ABI_MISMATCH.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_ABI_MISMATCH.example = use vitte/core

NAME_E_LINK_FAILED.summary = link echec.
NAME_E_LINK_FAILED.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_LINK_FAILED.step1 = Check the spelling and the nearest import or declaration.
NAME_E_LINK_FAILED.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_LINK_FAILED.example = use vitte/core

NAME_E_RUNTIME_PANIC.summary = runtime panique.
NAME_E_RUNTIME_PANIC.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_RUNTIME_PANIC.step1 = Check the spelling and the nearest import or declaration.
NAME_E_RUNTIME_PANIC.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_RUNTIME_PANIC.example = use vitte/core

MODULE_E_EXPECTED_IDENTIFIER.summary = identifiant attendu.
MODULE_E_EXPECTED_IDENTIFIER.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_EXPECTED_IDENTIFIER.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_EXPECTED_IDENTIFIER.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_EXPECTED_IDENTIFIER.example = use vitte/core

MODULE_E_EXPECTED_EXPRESSION.summary = expression attendu.
MODULE_E_EXPECTED_EXPRESSION.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_EXPECTED_EXPRESSION.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_EXPECTED_EXPRESSION.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_EXPECTED_EXPRESSION.example = use vitte/core

MODULE_E_EXPECTED_TYPE.summary = type attendu.
MODULE_E_EXPECTED_TYPE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_EXPECTED_TYPE.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_EXPECTED_TYPE.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_EXPECTED_TYPE.example = use vitte/core

MODULE_E_EXPECTED_PATTERN.summary = motif attendu.
MODULE_E_EXPECTED_PATTERN.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_EXPECTED_PATTERN.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_EXPECTED_PATTERN.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_EXPECTED_PATTERN.example = use vitte/core

MODULE_E_EXPECTED_BLOCK.summary = bloc attendu.
MODULE_E_EXPECTED_BLOCK.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_EXPECTED_BLOCK.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_EXPECTED_BLOCK.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_EXPECTED_BLOCK.example = use vitte/core

MODULE_E_EXPECTED_DELIMITER.summary = delimiteur attendu.
MODULE_E_EXPECTED_DELIMITER.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_EXPECTED_DELIMITER.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_EXPECTED_DELIMITER.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_EXPECTED_DELIMITER.example = use vitte/core

MODULE_E_UNEXPECTED_TOKEN.summary = jeton inattendu.
MODULE_E_UNEXPECTED_TOKEN.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_UNEXPECTED_TOKEN.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_UNEXPECTED_TOKEN.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_UNEXPECTED_TOKEN.example = use vitte/core

MODULE_E_UNBALANCED_DELIMITER.summary = unbalanced delimiteur.
MODULE_E_UNBALANCED_DELIMITER.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_UNBALANCED_DELIMITER.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_UNBALANCED_DELIMITER.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_UNBALANCED_DELIMITER.example = use vitte/core

MODULE_E_INVALID_ATTRIBUTE.summary = attribute invalide.
MODULE_E_INVALID_ATTRIBUTE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_ATTRIBUTE.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_ATTRIBUTE.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_ATTRIBUTE.example = use vitte/core

MODULE_E_INVALID_DECLARATION.summary = declaration invalide.
MODULE_E_INVALID_DECLARATION.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_DECLARATION.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_DECLARATION.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_DECLARATION.example = use vitte/core

MODULE_E_INVALID_STATEMENT.summary = instruction invalide.
MODULE_E_INVALID_STATEMENT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_STATEMENT.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_STATEMENT.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_STATEMENT.example = use vitte/core

MODULE_E_INVALID_EXPRESSION.summary = expression invalide.
MODULE_E_INVALID_EXPRESSION.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_EXPRESSION.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_EXPRESSION.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_EXPRESSION.example = use vitte/core

MODULE_E_INVALID_PATTERN.summary = motif invalide.
MODULE_E_INVALID_PATTERN.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_PATTERN.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_PATTERN.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_PATTERN.example = use vitte/core

MODULE_E_INVALID_LITERAL.summary = litteral invalide.
MODULE_E_INVALID_LITERAL.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_LITERAL.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_LITERAL.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_LITERAL.example = use vitte/core

MODULE_E_INVALID_OPERATOR.summary = operateur invalide.
MODULE_E_INVALID_OPERATOR.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_OPERATOR.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_OPERATOR.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_OPERATOR.example = use vitte/core

MODULE_E_INVALID_MODIFIER.summary = modifier invalide.
MODULE_E_INVALID_MODIFIER.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_MODIFIER.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_MODIFIER.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_MODIFIER.example = use vitte/core

MODULE_E_MISSING_BODY.summary = body manquant.
MODULE_E_MISSING_BODY.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_MISSING_BODY.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_MISSING_BODY.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_MISSING_BODY.example = use vitte/core

MODULE_E_MISSING_RETURN.summary = retour manquant.
MODULE_E_MISSING_RETURN.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_MISSING_RETURN.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_MISSING_RETURN.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_MISSING_RETURN.example = use vitte/core

MODULE_E_DUPLICATE_NAME.summary = nom duplique.
MODULE_E_DUPLICATE_NAME.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_DUPLICATE_NAME.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_DUPLICATE_NAME.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_DUPLICATE_NAME.example = use vitte/core

MODULE_E_UNKNOWN_NAME.summary = nom inconnu.
MODULE_E_UNKNOWN_NAME.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_UNKNOWN_NAME.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_UNKNOWN_NAME.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_UNKNOWN_NAME.example = use vitte/core

MODULE_E_UNKNOWN_TYPE.summary = type inconnu.
MODULE_E_UNKNOWN_TYPE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_UNKNOWN_TYPE.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_UNKNOWN_TYPE.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_UNKNOWN_TYPE.example = use vitte/core

MODULE_E_UNKNOWN_MODULE.summary = module inconnu.
MODULE_E_UNKNOWN_MODULE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_UNKNOWN_MODULE.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_UNKNOWN_MODULE.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_UNKNOWN_MODULE.example = use vitte/core

MODULE_E_UNKNOWN_MEMBER.summary = membre inconnu.
MODULE_E_UNKNOWN_MEMBER.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_UNKNOWN_MEMBER.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_UNKNOWN_MEMBER.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_UNKNOWN_MEMBER.example = use vitte/core

MODULE_E_AMBIGUOUS_NAME.summary = ambiguous nom.
MODULE_E_AMBIGUOUS_NAME.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_AMBIGUOUS_NAME.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_AMBIGUOUS_NAME.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_AMBIGUOUS_NAME.example = use vitte/core

MODULE_E_PRIVATE_SYMBOL.summary = private symbole.
MODULE_E_PRIVATE_SYMBOL.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_PRIVATE_SYMBOL.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_PRIVATE_SYMBOL.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_PRIVATE_SYMBOL.example = use vitte/core

MODULE_E_IMPORT_NOT_FOUND.summary = import manquant.
MODULE_E_IMPORT_NOT_FOUND.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_IMPORT_NOT_FOUND.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_IMPORT_NOT_FOUND.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_IMPORT_NOT_FOUND.example = use vitte/core

MODULE_E_IMPORT_CYCLE.summary = import cycle.
MODULE_E_IMPORT_CYCLE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_IMPORT_CYCLE.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_IMPORT_CYCLE.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_IMPORT_CYCLE.example = use vitte/core

MODULE_E_EXPORT_CONFLICT.summary = export conflit.
MODULE_E_EXPORT_CONFLICT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_EXPORT_CONFLICT.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_EXPORT_CONFLICT.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_EXPORT_CONFLICT.example = use vitte/core

MODULE_E_ARITY_MISMATCH.summary = arity incompatibilite.
MODULE_E_ARITY_MISMATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_ARITY_MISMATCH.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_ARITY_MISMATCH.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_ARITY_MISMATCH.example = use vitte/core

MODULE_E_ARGUMENT_MISMATCH.summary = argument incompatibilite.
MODULE_E_ARGUMENT_MISMATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_ARGUMENT_MISMATCH.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_ARGUMENT_MISMATCH.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_ARGUMENT_MISMATCH.example = use vitte/core

MODULE_E_ASSIGNMENT_MISMATCH.summary = affectation incompatibilite.
MODULE_E_ASSIGNMENT_MISMATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_ASSIGNMENT_MISMATCH.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_ASSIGNMENT_MISMATCH.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_ASSIGNMENT_MISMATCH.example = use vitte/core

MODULE_E_BRANCH_MISMATCH.summary = branch incompatibilite.
MODULE_E_BRANCH_MISMATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_BRANCH_MISMATCH.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_BRANCH_MISMATCH.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_BRANCH_MISMATCH.example = use vitte/core

MODULE_E_INVALID_CALL.summary = appel invalide.
MODULE_E_INVALID_CALL.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_CALL.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_CALL.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_CALL.example = use vitte/core

MODULE_E_INVALID_CAST.summary = conversion invalide.
MODULE_E_INVALID_CAST.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_CAST.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_CAST.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_CAST.example = use vitte/core

MODULE_E_INVALID_INDEX.summary = index invalide.
MODULE_E_INVALID_INDEX.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_INDEX.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_INDEX.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_INDEX.example = use vitte/core

MODULE_E_INVALID_DEREF.summary = deref invalide.
MODULE_E_INVALID_DEREF.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_DEREF.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_DEREF.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_DEREF.example = use vitte/core

MODULE_E_INVALID_BORROW.summary = emprunt invalide.
MODULE_E_INVALID_BORROW.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_BORROW.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_BORROW.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_BORROW.example = use vitte/core

MODULE_E_INVALID_MOVE.summary = deplacement invalide.
MODULE_E_INVALID_MOVE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_MOVE.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_MOVE.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_MOVE.example = use vitte/core

MODULE_E_USE_AFTER_MOVE.summary = use after deplacement.
MODULE_E_USE_AFTER_MOVE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_USE_AFTER_MOVE.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_USE_AFTER_MOVE.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_USE_AFTER_MOVE.example = use vitte/core

MODULE_E_USE_AFTER_DROP.summary = use after destruction.
MODULE_E_USE_AFTER_DROP.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_USE_AFTER_DROP.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_USE_AFTER_DROP.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_USE_AFTER_DROP.example = use vitte/core

MODULE_E_USE_BEFORE_INIT.summary = use before init.
MODULE_E_USE_BEFORE_INIT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_USE_BEFORE_INIT.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_USE_BEFORE_INIT.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_USE_BEFORE_INIT.example = use vitte/core

MODULE_E_DOUBLE_DROP.summary = double destruction.
MODULE_E_DOUBLE_DROP.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_DOUBLE_DROP.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_DOUBLE_DROP.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_DOUBLE_DROP.example = use vitte/core

MODULE_E_BORROW_CONFLICT.summary = emprunt conflit.
MODULE_E_BORROW_CONFLICT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_BORROW_CONFLICT.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_BORROW_CONFLICT.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_BORROW_CONFLICT.example = use vitte/core

MODULE_E_MUTABILITY_CONFLICT.summary = mutability conflit.
MODULE_E_MUTABILITY_CONFLICT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_MUTABILITY_CONFLICT.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_MUTABILITY_CONFLICT.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_MUTABILITY_CONFLICT.example = use vitte/core

MODULE_E_LIFETIME_TOO_SHORT.summary = duree de vie trop short.
MODULE_E_LIFETIME_TOO_SHORT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_LIFETIME_TOO_SHORT.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_LIFETIME_TOO_SHORT.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_LIFETIME_TOO_SHORT.example = use vitte/core

MODULE_E_DANGLING_REFERENCE.summary = dangling reference.
MODULE_E_DANGLING_REFERENCE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_DANGLING_REFERENCE.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_DANGLING_REFERENCE.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_DANGLING_REFERENCE.example = use vitte/core

MODULE_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
MODULE_E_NON_EXHAUSTIVE_MATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_NON_EXHAUSTIVE_MATCH.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_NON_EXHAUSTIVE_MATCH.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_NON_EXHAUSTIVE_MATCH.example = use vitte/core

MODULE_E_UNREACHABLE_PATTERN.summary = inatteignable motif.
MODULE_E_UNREACHABLE_PATTERN.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_UNREACHABLE_PATTERN.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_UNREACHABLE_PATTERN.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_UNREACHABLE_PATTERN.example = use vitte/core

MODULE_E_CONST_REQUIRED.summary = const required.
MODULE_E_CONST_REQUIRED.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_CONST_REQUIRED.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_CONST_REQUIRED.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_CONST_REQUIRED.example = use vitte/core

MODULE_E_CONST_OVERFLOW.summary = const depassement.
MODULE_E_CONST_OVERFLOW.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_CONST_OVERFLOW.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_CONST_OVERFLOW.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_CONST_OVERFLOW.example = use vitte/core

MODULE_E_CONST_DIVISION_BY_ZERO.summary = const division by zero.
MODULE_E_CONST_DIVISION_BY_ZERO.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_CONST_DIVISION_BY_ZERO.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_CONST_DIVISION_BY_ZERO.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_CONST_DIVISION_BY_ZERO.example = use vitte/core

MODULE_E_CONST_CYCLE.summary = const cycle.
MODULE_E_CONST_CYCLE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_CONST_CYCLE.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_CONST_CYCLE.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_CONST_CYCLE.example = use vitte/core

MODULE_E_MACRO_NOT_FOUND.summary = macro manquant.
MODULE_E_MACRO_NOT_FOUND.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_MACRO_NOT_FOUND.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_MACRO_NOT_FOUND.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_MACRO_NOT_FOUND.example = use vitte/core

MODULE_E_MACRO_RECURSION.summary = macro recursion.
MODULE_E_MACRO_RECURSION.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_MACRO_RECURSION.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_MACRO_RECURSION.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_MACRO_RECURSION.example = use vitte/core

MODULE_E_MACRO_EXPANSION_FAILED.summary = macro expansion echec.
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

MODULE_E_GENERIC_ARGUMENT_MISSING.summary = generique argument manquant.
MODULE_E_GENERIC_ARGUMENT_MISSING.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_GENERIC_ARGUMENT_MISSING.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_GENERIC_ARGUMENT_MISSING.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_GENERIC_ARGUMENT_MISSING.example = use vitte/core

MODULE_E_GENERIC_BOUND_FAILED.summary = generique bound echec.
MODULE_E_GENERIC_BOUND_FAILED.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_GENERIC_BOUND_FAILED.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_GENERIC_BOUND_FAILED.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_GENERIC_BOUND_FAILED.example = use vitte/core

MODULE_E_UNSUPPORTED_TARGET.summary = cible non pris en charge.
MODULE_E_UNSUPPORTED_TARGET.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_UNSUPPORTED_TARGET.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_UNSUPPORTED_TARGET.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_UNSUPPORTED_TARGET.example = use vitte/core

MODULE_E_ABI_MISMATCH.summary = abi incompatibilite.
MODULE_E_ABI_MISMATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_ABI_MISMATCH.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_ABI_MISMATCH.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_ABI_MISMATCH.example = use vitte/core

MODULE_E_LINK_FAILED.summary = link echec.
MODULE_E_LINK_FAILED.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_LINK_FAILED.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_LINK_FAILED.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_LINK_FAILED.example = use vitte/core

MODULE_E_RUNTIME_PANIC.summary = runtime panique.
MODULE_E_RUNTIME_PANIC.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_RUNTIME_PANIC.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_RUNTIME_PANIC.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_RUNTIME_PANIC.example = use vitte/core

TYPE_E_EXPECTED_IDENTIFIER.summary = identifiant attendu.
TYPE_E_EXPECTED_IDENTIFIER.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_EXPECTED_IDENTIFIER.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_EXPECTED_IDENTIFIER.fix = make the expression type match the type contract named by the type checker
TYPE_E_EXPECTED_IDENTIFIER.example = let count: int = 1

TYPE_E_EXPECTED_EXPRESSION.summary = expression attendu.
TYPE_E_EXPECTED_EXPRESSION.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_EXPECTED_EXPRESSION.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_EXPECTED_EXPRESSION.fix = make the expression type match the type contract named by the type checker
TYPE_E_EXPECTED_EXPRESSION.example = let count: int = 1

TYPE_E_EXPECTED_TYPE.summary = type attendu.
TYPE_E_EXPECTED_TYPE.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_EXPECTED_TYPE.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_EXPECTED_TYPE.fix = make the expression type match the type contract named by the type checker
TYPE_E_EXPECTED_TYPE.example = let count: int = 1

TYPE_E_EXPECTED_PATTERN.summary = motif attendu.
TYPE_E_EXPECTED_PATTERN.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_EXPECTED_PATTERN.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_EXPECTED_PATTERN.fix = make the expression type match the type contract named by the type checker
TYPE_E_EXPECTED_PATTERN.example = let count: int = 1

TYPE_E_EXPECTED_BLOCK.summary = bloc attendu.
TYPE_E_EXPECTED_BLOCK.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_EXPECTED_BLOCK.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_EXPECTED_BLOCK.fix = make the expression type match the type contract named by the type checker
TYPE_E_EXPECTED_BLOCK.example = let count: int = 1

TYPE_E_EXPECTED_DELIMITER.summary = delimiteur attendu.
TYPE_E_EXPECTED_DELIMITER.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_EXPECTED_DELIMITER.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_EXPECTED_DELIMITER.fix = make the expression type match the type contract named by the type checker
TYPE_E_EXPECTED_DELIMITER.example = let count: int = 1

TYPE_E_UNEXPECTED_TOKEN.summary = jeton inattendu.
TYPE_E_UNEXPECTED_TOKEN.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_UNEXPECTED_TOKEN.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_UNEXPECTED_TOKEN.fix = make the expression type match the type contract named by the type checker
TYPE_E_UNEXPECTED_TOKEN.example = let count: int = 1

TYPE_E_UNBALANCED_DELIMITER.summary = unbalanced delimiteur.
TYPE_E_UNBALANCED_DELIMITER.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_UNBALANCED_DELIMITER.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_UNBALANCED_DELIMITER.fix = make the expression type match the type contract named by the type checker
TYPE_E_UNBALANCED_DELIMITER.example = let count: int = 1

TYPE_E_INVALID_ATTRIBUTE.summary = attribute invalide.
TYPE_E_INVALID_ATTRIBUTE.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_ATTRIBUTE.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_ATTRIBUTE.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_ATTRIBUTE.example = let count: int = 1

TYPE_E_INVALID_DECLARATION.summary = declaration invalide.
TYPE_E_INVALID_DECLARATION.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_DECLARATION.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_DECLARATION.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_DECLARATION.example = let count: int = 1

TYPE_E_INVALID_STATEMENT.summary = instruction invalide.
TYPE_E_INVALID_STATEMENT.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_STATEMENT.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_STATEMENT.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_STATEMENT.example = let count: int = 1

TYPE_E_INVALID_EXPRESSION.summary = expression invalide.
TYPE_E_INVALID_EXPRESSION.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_EXPRESSION.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_EXPRESSION.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_EXPRESSION.example = let count: int = 1

TYPE_E_INVALID_PATTERN.summary = motif invalide.
TYPE_E_INVALID_PATTERN.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_PATTERN.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_PATTERN.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_PATTERN.example = let count: int = 1

TYPE_E_INVALID_LITERAL.summary = litteral invalide.
TYPE_E_INVALID_LITERAL.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_LITERAL.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_LITERAL.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_LITERAL.example = let count: int = 1

TYPE_E_INVALID_OPERATOR.summary = operateur invalide.
TYPE_E_INVALID_OPERATOR.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_OPERATOR.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_OPERATOR.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_OPERATOR.example = let count: int = 1

TYPE_E_INVALID_MODIFIER.summary = modifier invalide.
TYPE_E_INVALID_MODIFIER.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_MODIFIER.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_MODIFIER.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_MODIFIER.example = let count: int = 1

TYPE_E_MISSING_BODY.summary = body manquant.
TYPE_E_MISSING_BODY.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_MISSING_BODY.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_MISSING_BODY.fix = make the expression type match the type contract named by the type checker
TYPE_E_MISSING_BODY.example = let count: int = 1

TYPE_E_MISSING_RETURN.summary = retour manquant.
TYPE_E_MISSING_RETURN.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_MISSING_RETURN.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_MISSING_RETURN.fix = make the expression type match the type contract named by the type checker
TYPE_E_MISSING_RETURN.example = let count: int = 1

TYPE_E_DUPLICATE_NAME.summary = nom duplique.
TYPE_E_DUPLICATE_NAME.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_DUPLICATE_NAME.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_DUPLICATE_NAME.fix = make the expression type match the type contract named by the type checker
TYPE_E_DUPLICATE_NAME.example = let count: int = 1

TYPE_E_UNKNOWN_NAME.summary = nom inconnu.
TYPE_E_UNKNOWN_NAME.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_UNKNOWN_NAME.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_UNKNOWN_NAME.fix = make the expression type match the type contract named by the type checker
TYPE_E_UNKNOWN_NAME.example = let count: int = 1

TYPE_E_UNKNOWN_TYPE.summary = type inconnu.
TYPE_E_UNKNOWN_TYPE.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_UNKNOWN_TYPE.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_UNKNOWN_TYPE.fix = make the expression type match the type contract named by the type checker
TYPE_E_UNKNOWN_TYPE.example = let count: int = 1

TYPE_E_UNKNOWN_MODULE.summary = module inconnu.
TYPE_E_UNKNOWN_MODULE.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_UNKNOWN_MODULE.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_UNKNOWN_MODULE.fix = make the expression type match the type contract named by the type checker
TYPE_E_UNKNOWN_MODULE.example = let count: int = 1

TYPE_E_UNKNOWN_MEMBER.summary = membre inconnu.
TYPE_E_UNKNOWN_MEMBER.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_UNKNOWN_MEMBER.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_UNKNOWN_MEMBER.fix = make the expression type match the type contract named by the type checker
TYPE_E_UNKNOWN_MEMBER.example = let count: int = 1

TYPE_E_AMBIGUOUS_NAME.summary = ambiguous nom.
TYPE_E_AMBIGUOUS_NAME.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_AMBIGUOUS_NAME.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_AMBIGUOUS_NAME.fix = make the expression type match the type contract named by the type checker
TYPE_E_AMBIGUOUS_NAME.example = let count: int = 1

TYPE_E_PRIVATE_SYMBOL.summary = private symbole.
TYPE_E_PRIVATE_SYMBOL.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_PRIVATE_SYMBOL.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_PRIVATE_SYMBOL.fix = make the expression type match the type contract named by the type checker
TYPE_E_PRIVATE_SYMBOL.example = let count: int = 1

TYPE_E_IMPORT_NOT_FOUND.summary = import manquant.
TYPE_E_IMPORT_NOT_FOUND.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_IMPORT_NOT_FOUND.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_IMPORT_NOT_FOUND.fix = make the expression type match the type contract named by the type checker
TYPE_E_IMPORT_NOT_FOUND.example = let count: int = 1

TYPE_E_IMPORT_CYCLE.summary = import cycle.
TYPE_E_IMPORT_CYCLE.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_IMPORT_CYCLE.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_IMPORT_CYCLE.fix = make the expression type match the type contract named by the type checker
TYPE_E_IMPORT_CYCLE.example = let count: int = 1

TYPE_E_EXPORT_CONFLICT.summary = export conflit.
TYPE_E_EXPORT_CONFLICT.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_EXPORT_CONFLICT.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_EXPORT_CONFLICT.fix = make the expression type match the type contract named by the type checker
TYPE_E_EXPORT_CONFLICT.example = let count: int = 1

TYPE_E_ARITY_MISMATCH.summary = arity incompatibilite.
TYPE_E_ARITY_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_ARITY_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_ARITY_MISMATCH.fix = make the expression type match the type contract named by the type checker
TYPE_E_ARITY_MISMATCH.example = let count: int = 1

TYPE_E_ARGUMENT_MISMATCH.summary = argument incompatibilite.
TYPE_E_ARGUMENT_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_ARGUMENT_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_ARGUMENT_MISMATCH.fix = make the expression type match the type contract named by the type checker
TYPE_E_ARGUMENT_MISMATCH.example = let count: int = 1

TYPE_E_ASSIGNMENT_MISMATCH.summary = affectation incompatibilite.
TYPE_E_ASSIGNMENT_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_ASSIGNMENT_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_ASSIGNMENT_MISMATCH.fix = make the expression type match the type contract named by the type checker
TYPE_E_ASSIGNMENT_MISMATCH.example = let count: int = 1

TYPE_E_BRANCH_MISMATCH.summary = branch incompatibilite.
TYPE_E_BRANCH_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_BRANCH_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_BRANCH_MISMATCH.fix = make the expression type match the type contract named by the type checker
TYPE_E_BRANCH_MISMATCH.example = let count: int = 1

TYPE_E_INVALID_CALL.summary = appel invalide.
TYPE_E_INVALID_CALL.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_CALL.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_CALL.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_CALL.example = let count: int = 1

TYPE_E_INVALID_CAST.summary = conversion invalide.
TYPE_E_INVALID_CAST.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_CAST.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_CAST.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_CAST.example = let count: int = 1

TYPE_E_INVALID_INDEX.summary = index invalide.
TYPE_E_INVALID_INDEX.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_INDEX.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_INDEX.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_INDEX.example = let count: int = 1

TYPE_E_INVALID_DEREF.summary = deref invalide.
TYPE_E_INVALID_DEREF.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_DEREF.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_DEREF.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_DEREF.example = let count: int = 1

TYPE_E_INVALID_BORROW.summary = emprunt invalide.
TYPE_E_INVALID_BORROW.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_BORROW.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_BORROW.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_BORROW.example = let count: int = 1

TYPE_E_INVALID_MOVE.summary = deplacement invalide.
TYPE_E_INVALID_MOVE.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_MOVE.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_MOVE.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_MOVE.example = let count: int = 1

TYPE_E_USE_AFTER_MOVE.summary = use after deplacement.
TYPE_E_USE_AFTER_MOVE.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_USE_AFTER_MOVE.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_USE_AFTER_MOVE.fix = make the expression type match the type contract named by the type checker
TYPE_E_USE_AFTER_MOVE.example = let count: int = 1

TYPE_E_USE_AFTER_DROP.summary = use after destruction.
TYPE_E_USE_AFTER_DROP.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_USE_AFTER_DROP.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_USE_AFTER_DROP.fix = make the expression type match the type contract named by the type checker
TYPE_E_USE_AFTER_DROP.example = let count: int = 1

TYPE_E_USE_BEFORE_INIT.summary = use before init.
TYPE_E_USE_BEFORE_INIT.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_USE_BEFORE_INIT.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_USE_BEFORE_INIT.fix = make the expression type match the type contract named by the type checker
TYPE_E_USE_BEFORE_INIT.example = let count: int = 1

TYPE_E_DOUBLE_DROP.summary = double destruction.
TYPE_E_DOUBLE_DROP.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_DOUBLE_DROP.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_DOUBLE_DROP.fix = make the expression type match the type contract named by the type checker
TYPE_E_DOUBLE_DROP.example = let count: int = 1

TYPE_E_BORROW_CONFLICT.summary = emprunt conflit.
TYPE_E_BORROW_CONFLICT.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_BORROW_CONFLICT.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_BORROW_CONFLICT.fix = make the expression type match the type contract named by the type checker
TYPE_E_BORROW_CONFLICT.example = let count: int = 1

TYPE_E_MUTABILITY_CONFLICT.summary = mutability conflit.
TYPE_E_MUTABILITY_CONFLICT.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_MUTABILITY_CONFLICT.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_MUTABILITY_CONFLICT.fix = make the expression type match the type contract named by the type checker
TYPE_E_MUTABILITY_CONFLICT.example = let count: int = 1

TYPE_E_LIFETIME_TOO_SHORT.summary = duree de vie trop short.
TYPE_E_LIFETIME_TOO_SHORT.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_LIFETIME_TOO_SHORT.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_LIFETIME_TOO_SHORT.fix = make the expression type match the type contract named by the type checker
TYPE_E_LIFETIME_TOO_SHORT.example = let count: int = 1

TYPE_E_DANGLING_REFERENCE.summary = dangling reference.
TYPE_E_DANGLING_REFERENCE.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_DANGLING_REFERENCE.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_DANGLING_REFERENCE.fix = make the expression type match the type contract named by the type checker
TYPE_E_DANGLING_REFERENCE.example = let count: int = 1

TYPE_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
TYPE_E_NON_EXHAUSTIVE_MATCH.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_NON_EXHAUSTIVE_MATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_NON_EXHAUSTIVE_MATCH.fix = make the expression type match the type contract named by the type checker
TYPE_E_NON_EXHAUSTIVE_MATCH.example = let count: int = 1

TYPE_E_UNREACHABLE_PATTERN.summary = inatteignable motif.
TYPE_E_UNREACHABLE_PATTERN.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_UNREACHABLE_PATTERN.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_UNREACHABLE_PATTERN.fix = make the expression type match the type contract named by the type checker
TYPE_E_UNREACHABLE_PATTERN.example = let count: int = 1

TYPE_E_CONST_REQUIRED.summary = const required.
TYPE_E_CONST_REQUIRED.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_CONST_REQUIRED.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_CONST_REQUIRED.fix = make the expression type match the type contract named by the type checker
TYPE_E_CONST_REQUIRED.example = let count: int = 1

TYPE_E_CONST_OVERFLOW.summary = const depassement.
TYPE_E_CONST_OVERFLOW.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_CONST_OVERFLOW.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_CONST_OVERFLOW.fix = make the expression type match the type contract named by the type checker
TYPE_E_CONST_OVERFLOW.example = let count: int = 1

TYPE_E_CONST_DIVISION_BY_ZERO.summary = const division by zero.
TYPE_E_CONST_DIVISION_BY_ZERO.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_CONST_DIVISION_BY_ZERO.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_CONST_DIVISION_BY_ZERO.fix = make the expression type match the type contract named by the type checker
TYPE_E_CONST_DIVISION_BY_ZERO.example = let count: int = 1

TYPE_E_CONST_CYCLE.summary = const cycle.
TYPE_E_CONST_CYCLE.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_CONST_CYCLE.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_CONST_CYCLE.fix = make the expression type match the type contract named by the type checker
TYPE_E_CONST_CYCLE.example = let count: int = 1

TYPE_E_MACRO_NOT_FOUND.summary = macro manquant.
TYPE_E_MACRO_NOT_FOUND.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_MACRO_NOT_FOUND.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_MACRO_NOT_FOUND.fix = make the expression type match the type contract named by the type checker
TYPE_E_MACRO_NOT_FOUND.example = let count: int = 1

TYPE_E_MACRO_RECURSION.summary = macro recursion.
TYPE_E_MACRO_RECURSION.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_MACRO_RECURSION.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_MACRO_RECURSION.fix = make the expression type match the type contract named by the type checker
TYPE_E_MACRO_RECURSION.example = let count: int = 1

TYPE_E_MACRO_EXPANSION_FAILED.summary = macro expansion echec.
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

TYPE_E_GENERIC_ARGUMENT_MISSING.summary = generique argument manquant.
TYPE_E_GENERIC_ARGUMENT_MISSING.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_GENERIC_ARGUMENT_MISSING.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_GENERIC_ARGUMENT_MISSING.fix = make the expression type match the type contract named by the type checker
TYPE_E_GENERIC_ARGUMENT_MISSING.example = let count: int = 1

TYPE_E_GENERIC_BOUND_FAILED.summary = generique bound echec.
TYPE_E_GENERIC_BOUND_FAILED.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_GENERIC_BOUND_FAILED.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_GENERIC_BOUND_FAILED.fix = make the expression type match the type contract named by the type checker
TYPE_E_GENERIC_BOUND_FAILED.example = let count: int = 1

TYPE_E_UNSUPPORTED_TARGET.summary = cible non pris en charge.
TYPE_E_UNSUPPORTED_TARGET.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_UNSUPPORTED_TARGET.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_UNSUPPORTED_TARGET.fix = make the expression type match the type contract named by the type checker
TYPE_E_UNSUPPORTED_TARGET.example = let count: int = 1

TYPE_E_ABI_MISMATCH.summary = abi incompatibilite.
TYPE_E_ABI_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_ABI_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_ABI_MISMATCH.fix = make the expression type match the type contract named by the type checker
TYPE_E_ABI_MISMATCH.example = let count: int = 1

TYPE_E_LINK_FAILED.summary = link echec.
TYPE_E_LINK_FAILED.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_LINK_FAILED.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_LINK_FAILED.fix = make the expression type match the type contract named by the type checker
TYPE_E_LINK_FAILED.example = let count: int = 1

TYPE_E_RUNTIME_PANIC.summary = runtime panique.
TYPE_E_RUNTIME_PANIC.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_RUNTIME_PANIC.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_RUNTIME_PANIC.fix = make the expression type match the type contract named by the type checker
TYPE_E_RUNTIME_PANIC.example = let count: int = 1

GENERIC_E_EXPECTED_IDENTIFIER.summary = identifiant attendu.
GENERIC_E_EXPECTED_IDENTIFIER.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_EXPECTED_IDENTIFIER.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_EXPECTED_IDENTIFIER.fix = make the expression type match the type contract named by the type checker
GENERIC_E_EXPECTED_IDENTIFIER.example = let count: int = 1

GENERIC_E_EXPECTED_EXPRESSION.summary = expression attendu.
GENERIC_E_EXPECTED_EXPRESSION.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_EXPECTED_EXPRESSION.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_EXPECTED_EXPRESSION.fix = make the expression type match the type contract named by the type checker
GENERIC_E_EXPECTED_EXPRESSION.example = let count: int = 1

GENERIC_E_EXPECTED_TYPE.summary = type attendu.
GENERIC_E_EXPECTED_TYPE.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_EXPECTED_TYPE.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_EXPECTED_TYPE.fix = make the expression type match the type contract named by the type checker
GENERIC_E_EXPECTED_TYPE.example = let count: int = 1

GENERIC_E_EXPECTED_PATTERN.summary = motif attendu.
GENERIC_E_EXPECTED_PATTERN.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_EXPECTED_PATTERN.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_EXPECTED_PATTERN.fix = make the expression type match the type contract named by the type checker
GENERIC_E_EXPECTED_PATTERN.example = let count: int = 1

GENERIC_E_EXPECTED_BLOCK.summary = bloc attendu.
GENERIC_E_EXPECTED_BLOCK.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_EXPECTED_BLOCK.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_EXPECTED_BLOCK.fix = make the expression type match the type contract named by the type checker
GENERIC_E_EXPECTED_BLOCK.example = let count: int = 1

GENERIC_E_EXPECTED_DELIMITER.summary = delimiteur attendu.
GENERIC_E_EXPECTED_DELIMITER.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_EXPECTED_DELIMITER.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_EXPECTED_DELIMITER.fix = make the expression type match the type contract named by the type checker
GENERIC_E_EXPECTED_DELIMITER.example = let count: int = 1

GENERIC_E_UNEXPECTED_TOKEN.summary = jeton inattendu.
GENERIC_E_UNEXPECTED_TOKEN.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_UNEXPECTED_TOKEN.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_UNEXPECTED_TOKEN.fix = make the expression type match the type contract named by the type checker
GENERIC_E_UNEXPECTED_TOKEN.example = let count: int = 1

GENERIC_E_UNBALANCED_DELIMITER.summary = unbalanced delimiteur.
GENERIC_E_UNBALANCED_DELIMITER.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_UNBALANCED_DELIMITER.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_UNBALANCED_DELIMITER.fix = make the expression type match the type contract named by the type checker
GENERIC_E_UNBALANCED_DELIMITER.example = let count: int = 1

GENERIC_E_INVALID_ATTRIBUTE.summary = attribute invalide.
GENERIC_E_INVALID_ATTRIBUTE.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_ATTRIBUTE.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_ATTRIBUTE.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_ATTRIBUTE.example = let count: int = 1

GENERIC_E_INVALID_DECLARATION.summary = declaration invalide.
GENERIC_E_INVALID_DECLARATION.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_DECLARATION.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_DECLARATION.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_DECLARATION.example = let count: int = 1

GENERIC_E_INVALID_STATEMENT.summary = instruction invalide.
GENERIC_E_INVALID_STATEMENT.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_STATEMENT.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_STATEMENT.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_STATEMENT.example = let count: int = 1

GENERIC_E_INVALID_EXPRESSION.summary = expression invalide.
GENERIC_E_INVALID_EXPRESSION.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_EXPRESSION.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_EXPRESSION.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_EXPRESSION.example = let count: int = 1

GENERIC_E_INVALID_PATTERN.summary = motif invalide.
GENERIC_E_INVALID_PATTERN.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_PATTERN.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_PATTERN.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_PATTERN.example = let count: int = 1

GENERIC_E_INVALID_LITERAL.summary = litteral invalide.
GENERIC_E_INVALID_LITERAL.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_LITERAL.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_LITERAL.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_LITERAL.example = let count: int = 1

GENERIC_E_INVALID_OPERATOR.summary = operateur invalide.
GENERIC_E_INVALID_OPERATOR.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_OPERATOR.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_OPERATOR.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_OPERATOR.example = let count: int = 1

GENERIC_E_INVALID_MODIFIER.summary = modifier invalide.
GENERIC_E_INVALID_MODIFIER.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_MODIFIER.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_MODIFIER.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_MODIFIER.example = let count: int = 1

GENERIC_E_MISSING_BODY.summary = body manquant.
GENERIC_E_MISSING_BODY.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_MISSING_BODY.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_MISSING_BODY.fix = make the expression type match the type contract named by the type checker
GENERIC_E_MISSING_BODY.example = let count: int = 1

GENERIC_E_MISSING_RETURN.summary = retour manquant.
GENERIC_E_MISSING_RETURN.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_MISSING_RETURN.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_MISSING_RETURN.fix = make the expression type match the type contract named by the type checker
GENERIC_E_MISSING_RETURN.example = let count: int = 1

GENERIC_E_DUPLICATE_NAME.summary = nom duplique.
GENERIC_E_DUPLICATE_NAME.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_DUPLICATE_NAME.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_DUPLICATE_NAME.fix = make the expression type match the type contract named by the type checker
GENERIC_E_DUPLICATE_NAME.example = let count: int = 1

GENERIC_E_UNKNOWN_NAME.summary = nom inconnu.
GENERIC_E_UNKNOWN_NAME.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_UNKNOWN_NAME.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_UNKNOWN_NAME.fix = make the expression type match the type contract named by the type checker
GENERIC_E_UNKNOWN_NAME.example = let count: int = 1

GENERIC_E_UNKNOWN_TYPE.summary = type inconnu.
GENERIC_E_UNKNOWN_TYPE.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_UNKNOWN_TYPE.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_UNKNOWN_TYPE.fix = make the expression type match the type contract named by the type checker
GENERIC_E_UNKNOWN_TYPE.example = let count: int = 1

GENERIC_E_UNKNOWN_MODULE.summary = module inconnu.
GENERIC_E_UNKNOWN_MODULE.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_UNKNOWN_MODULE.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_UNKNOWN_MODULE.fix = make the expression type match the type contract named by the type checker
GENERIC_E_UNKNOWN_MODULE.example = let count: int = 1

GENERIC_E_UNKNOWN_MEMBER.summary = membre inconnu.
GENERIC_E_UNKNOWN_MEMBER.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_UNKNOWN_MEMBER.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_UNKNOWN_MEMBER.fix = make the expression type match the type contract named by the type checker
GENERIC_E_UNKNOWN_MEMBER.example = let count: int = 1

GENERIC_E_AMBIGUOUS_NAME.summary = ambiguous nom.
GENERIC_E_AMBIGUOUS_NAME.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_AMBIGUOUS_NAME.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_AMBIGUOUS_NAME.fix = make the expression type match the type contract named by the type checker
GENERIC_E_AMBIGUOUS_NAME.example = let count: int = 1

GENERIC_E_PRIVATE_SYMBOL.summary = private symbole.
GENERIC_E_PRIVATE_SYMBOL.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_PRIVATE_SYMBOL.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_PRIVATE_SYMBOL.fix = make the expression type match the type contract named by the type checker
GENERIC_E_PRIVATE_SYMBOL.example = let count: int = 1

GENERIC_E_IMPORT_NOT_FOUND.summary = import manquant.
GENERIC_E_IMPORT_NOT_FOUND.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_IMPORT_NOT_FOUND.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_IMPORT_NOT_FOUND.fix = make the expression type match the type contract named by the type checker
GENERIC_E_IMPORT_NOT_FOUND.example = let count: int = 1

GENERIC_E_IMPORT_CYCLE.summary = import cycle.
GENERIC_E_IMPORT_CYCLE.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_IMPORT_CYCLE.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_IMPORT_CYCLE.fix = make the expression type match the type contract named by the type checker
GENERIC_E_IMPORT_CYCLE.example = let count: int = 1

GENERIC_E_EXPORT_CONFLICT.summary = export conflit.
GENERIC_E_EXPORT_CONFLICT.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_EXPORT_CONFLICT.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_EXPORT_CONFLICT.fix = make the expression type match the type contract named by the type checker
GENERIC_E_EXPORT_CONFLICT.example = let count: int = 1

GENERIC_E_ARITY_MISMATCH.summary = arity incompatibilite.
GENERIC_E_ARITY_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_ARITY_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_ARITY_MISMATCH.fix = make the expression type match the type contract named by the type checker
GENERIC_E_ARITY_MISMATCH.example = let count: int = 1

GENERIC_E_ARGUMENT_MISMATCH.summary = argument incompatibilite.
GENERIC_E_ARGUMENT_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_ARGUMENT_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_ARGUMENT_MISMATCH.fix = make the expression type match the type contract named by the type checker
GENERIC_E_ARGUMENT_MISMATCH.example = let count: int = 1

GENERIC_E_ASSIGNMENT_MISMATCH.summary = affectation incompatibilite.
GENERIC_E_ASSIGNMENT_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_ASSIGNMENT_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_ASSIGNMENT_MISMATCH.fix = make the expression type match the type contract named by the type checker
GENERIC_E_ASSIGNMENT_MISMATCH.example = let count: int = 1

GENERIC_E_BRANCH_MISMATCH.summary = branch incompatibilite.
GENERIC_E_BRANCH_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_BRANCH_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_BRANCH_MISMATCH.fix = make the expression type match the type contract named by the type checker
GENERIC_E_BRANCH_MISMATCH.example = let count: int = 1

GENERIC_E_INVALID_CALL.summary = appel invalide.
GENERIC_E_INVALID_CALL.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_CALL.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_CALL.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_CALL.example = let count: int = 1

GENERIC_E_INVALID_CAST.summary = conversion invalide.
GENERIC_E_INVALID_CAST.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_CAST.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_CAST.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_CAST.example = let count: int = 1

GENERIC_E_INVALID_INDEX.summary = index invalide.
GENERIC_E_INVALID_INDEX.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_INDEX.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_INDEX.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_INDEX.example = let count: int = 1

GENERIC_E_INVALID_DEREF.summary = deref invalide.
GENERIC_E_INVALID_DEREF.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_DEREF.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_DEREF.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_DEREF.example = let count: int = 1

GENERIC_E_INVALID_BORROW.summary = emprunt invalide.
GENERIC_E_INVALID_BORROW.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_BORROW.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_BORROW.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_BORROW.example = let count: int = 1

GENERIC_E_INVALID_MOVE.summary = deplacement invalide.
GENERIC_E_INVALID_MOVE.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_MOVE.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_MOVE.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_MOVE.example = let count: int = 1

GENERIC_E_USE_AFTER_MOVE.summary = use after deplacement.
GENERIC_E_USE_AFTER_MOVE.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_USE_AFTER_MOVE.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_USE_AFTER_MOVE.fix = make the expression type match the type contract named by the type checker
GENERIC_E_USE_AFTER_MOVE.example = let count: int = 1

GENERIC_E_USE_AFTER_DROP.summary = use after destruction.
GENERIC_E_USE_AFTER_DROP.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_USE_AFTER_DROP.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_USE_AFTER_DROP.fix = make the expression type match the type contract named by the type checker
GENERIC_E_USE_AFTER_DROP.example = let count: int = 1

GENERIC_E_USE_BEFORE_INIT.summary = use before init.
GENERIC_E_USE_BEFORE_INIT.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_USE_BEFORE_INIT.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_USE_BEFORE_INIT.fix = make the expression type match the type contract named by the type checker
GENERIC_E_USE_BEFORE_INIT.example = let count: int = 1

GENERIC_E_DOUBLE_DROP.summary = double destruction.
GENERIC_E_DOUBLE_DROP.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_DOUBLE_DROP.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_DOUBLE_DROP.fix = make the expression type match the type contract named by the type checker
GENERIC_E_DOUBLE_DROP.example = let count: int = 1

GENERIC_E_BORROW_CONFLICT.summary = emprunt conflit.
GENERIC_E_BORROW_CONFLICT.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_BORROW_CONFLICT.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_BORROW_CONFLICT.fix = make the expression type match the type contract named by the type checker
GENERIC_E_BORROW_CONFLICT.example = let count: int = 1

GENERIC_E_MUTABILITY_CONFLICT.summary = mutability conflit.
GENERIC_E_MUTABILITY_CONFLICT.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_MUTABILITY_CONFLICT.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_MUTABILITY_CONFLICT.fix = make the expression type match the type contract named by the type checker
GENERIC_E_MUTABILITY_CONFLICT.example = let count: int = 1

GENERIC_E_LIFETIME_TOO_SHORT.summary = duree de vie trop short.
GENERIC_E_LIFETIME_TOO_SHORT.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_LIFETIME_TOO_SHORT.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_LIFETIME_TOO_SHORT.fix = make the expression type match the type contract named by the type checker
GENERIC_E_LIFETIME_TOO_SHORT.example = let count: int = 1

GENERIC_E_DANGLING_REFERENCE.summary = dangling reference.
GENERIC_E_DANGLING_REFERENCE.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_DANGLING_REFERENCE.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_DANGLING_REFERENCE.fix = make the expression type match the type contract named by the type checker
GENERIC_E_DANGLING_REFERENCE.example = let count: int = 1

GENERIC_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
GENERIC_E_NON_EXHAUSTIVE_MATCH.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_NON_EXHAUSTIVE_MATCH.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_NON_EXHAUSTIVE_MATCH.fix = make the expression type match the type contract named by the type checker
GENERIC_E_NON_EXHAUSTIVE_MATCH.example = let count: int = 1

GENERIC_E_UNREACHABLE_PATTERN.summary = inatteignable motif.
GENERIC_E_UNREACHABLE_PATTERN.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_UNREACHABLE_PATTERN.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_UNREACHABLE_PATTERN.fix = make the expression type match the type contract named by the type checker
GENERIC_E_UNREACHABLE_PATTERN.example = let count: int = 1

GENERIC_E_CONST_REQUIRED.summary = const required.
GENERIC_E_CONST_REQUIRED.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_CONST_REQUIRED.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_CONST_REQUIRED.fix = make the expression type match the type contract named by the type checker
GENERIC_E_CONST_REQUIRED.example = let count: int = 1

GENERIC_E_CONST_OVERFLOW.summary = const depassement.
GENERIC_E_CONST_OVERFLOW.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_CONST_OVERFLOW.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_CONST_OVERFLOW.fix = make the expression type match the type contract named by the type checker
GENERIC_E_CONST_OVERFLOW.example = let count: int = 1

GENERIC_E_CONST_DIVISION_BY_ZERO.summary = const division by zero.
GENERIC_E_CONST_DIVISION_BY_ZERO.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_CONST_DIVISION_BY_ZERO.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_CONST_DIVISION_BY_ZERO.fix = make the expression type match the type contract named by the type checker
GENERIC_E_CONST_DIVISION_BY_ZERO.example = let count: int = 1

GENERIC_E_CONST_CYCLE.summary = const cycle.
GENERIC_E_CONST_CYCLE.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_CONST_CYCLE.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_CONST_CYCLE.fix = make the expression type match the type contract named by the type checker
GENERIC_E_CONST_CYCLE.example = let count: int = 1

GENERIC_E_MACRO_NOT_FOUND.summary = macro manquant.
GENERIC_E_MACRO_NOT_FOUND.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_MACRO_NOT_FOUND.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_MACRO_NOT_FOUND.fix = make the expression type match the type contract named by the type checker
GENERIC_E_MACRO_NOT_FOUND.example = let count: int = 1

GENERIC_E_MACRO_RECURSION.summary = macro recursion.
GENERIC_E_MACRO_RECURSION.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_MACRO_RECURSION.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_MACRO_RECURSION.fix = make the expression type match the type contract named by the type checker
GENERIC_E_MACRO_RECURSION.example = let count: int = 1

GENERIC_E_MACRO_EXPANSION_FAILED.summary = macro expansion echec.
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

GENERIC_E_GENERIC_ARGUMENT_MISSING.summary = generique argument manquant.
GENERIC_E_GENERIC_ARGUMENT_MISSING.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_GENERIC_ARGUMENT_MISSING.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_GENERIC_ARGUMENT_MISSING.fix = make the expression type match the type contract named by the type checker
GENERIC_E_GENERIC_ARGUMENT_MISSING.example = let count: int = 1

GENERIC_E_GENERIC_BOUND_FAILED.summary = generique bound echec.
GENERIC_E_GENERIC_BOUND_FAILED.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_GENERIC_BOUND_FAILED.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_GENERIC_BOUND_FAILED.fix = make the expression type match the type contract named by the type checker
GENERIC_E_GENERIC_BOUND_FAILED.example = let count: int = 1

GENERIC_E_UNSUPPORTED_TARGET.summary = cible non pris en charge.
GENERIC_E_UNSUPPORTED_TARGET.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_UNSUPPORTED_TARGET.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_UNSUPPORTED_TARGET.fix = make the expression type match the type contract named by the type checker
GENERIC_E_UNSUPPORTED_TARGET.example = let count: int = 1

GENERIC_E_ABI_MISMATCH.summary = abi incompatibilite.
GENERIC_E_ABI_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_ABI_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_ABI_MISMATCH.fix = make the expression type match the type contract named by the type checker
GENERIC_E_ABI_MISMATCH.example = let count: int = 1

GENERIC_E_LINK_FAILED.summary = link echec.
GENERIC_E_LINK_FAILED.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_LINK_FAILED.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_LINK_FAILED.fix = make the expression type match the type contract named by the type checker
GENERIC_E_LINK_FAILED.example = let count: int = 1

GENERIC_E_RUNTIME_PANIC.summary = runtime panique.
GENERIC_E_RUNTIME_PANIC.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_RUNTIME_PANIC.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_RUNTIME_PANIC.fix = make the expression type match the type contract named by the type checker
GENERIC_E_RUNTIME_PANIC.example = let count: int = 1

TRAIT_E_EXPECTED_IDENTIFIER.summary = identifiant attendu.
TRAIT_E_EXPECTED_IDENTIFIER.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_EXPECTED_IDENTIFIER.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_EXPECTED_IDENTIFIER.fix = make the expression type match the type contract named by the type checker
TRAIT_E_EXPECTED_IDENTIFIER.example = let count: int = 1

TRAIT_E_EXPECTED_EXPRESSION.summary = expression attendu.
TRAIT_E_EXPECTED_EXPRESSION.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_EXPECTED_EXPRESSION.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_EXPECTED_EXPRESSION.fix = make the expression type match the type contract named by the type checker
TRAIT_E_EXPECTED_EXPRESSION.example = let count: int = 1

TRAIT_E_EXPECTED_TYPE.summary = type attendu.
TRAIT_E_EXPECTED_TYPE.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_EXPECTED_TYPE.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_EXPECTED_TYPE.fix = make the expression type match the type contract named by the type checker
TRAIT_E_EXPECTED_TYPE.example = let count: int = 1

TRAIT_E_EXPECTED_PATTERN.summary = motif attendu.
TRAIT_E_EXPECTED_PATTERN.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_EXPECTED_PATTERN.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_EXPECTED_PATTERN.fix = make the expression type match the type contract named by the type checker
TRAIT_E_EXPECTED_PATTERN.example = let count: int = 1

TRAIT_E_EXPECTED_BLOCK.summary = bloc attendu.
TRAIT_E_EXPECTED_BLOCK.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_EXPECTED_BLOCK.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_EXPECTED_BLOCK.fix = make the expression type match the type contract named by the type checker
TRAIT_E_EXPECTED_BLOCK.example = let count: int = 1

TRAIT_E_EXPECTED_DELIMITER.summary = delimiteur attendu.
TRAIT_E_EXPECTED_DELIMITER.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_EXPECTED_DELIMITER.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_EXPECTED_DELIMITER.fix = make the expression type match the type contract named by the type checker
TRAIT_E_EXPECTED_DELIMITER.example = let count: int = 1

TRAIT_E_UNEXPECTED_TOKEN.summary = jeton inattendu.
TRAIT_E_UNEXPECTED_TOKEN.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_UNEXPECTED_TOKEN.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_UNEXPECTED_TOKEN.fix = make the expression type match the type contract named by the type checker
TRAIT_E_UNEXPECTED_TOKEN.example = let count: int = 1

TRAIT_E_UNBALANCED_DELIMITER.summary = unbalanced delimiteur.
TRAIT_E_UNBALANCED_DELIMITER.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_UNBALANCED_DELIMITER.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_UNBALANCED_DELIMITER.fix = make the expression type match the type contract named by the type checker
TRAIT_E_UNBALANCED_DELIMITER.example = let count: int = 1

TRAIT_E_INVALID_ATTRIBUTE.summary = attribute invalide.
TRAIT_E_INVALID_ATTRIBUTE.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_ATTRIBUTE.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_ATTRIBUTE.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_ATTRIBUTE.example = let count: int = 1

TRAIT_E_INVALID_DECLARATION.summary = declaration invalide.
TRAIT_E_INVALID_DECLARATION.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_DECLARATION.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_DECLARATION.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_DECLARATION.example = let count: int = 1

TRAIT_E_INVALID_STATEMENT.summary = instruction invalide.
TRAIT_E_INVALID_STATEMENT.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_STATEMENT.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_STATEMENT.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_STATEMENT.example = let count: int = 1

TRAIT_E_INVALID_EXPRESSION.summary = expression invalide.
TRAIT_E_INVALID_EXPRESSION.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_EXPRESSION.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_EXPRESSION.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_EXPRESSION.example = let count: int = 1

TRAIT_E_INVALID_PATTERN.summary = motif invalide.
TRAIT_E_INVALID_PATTERN.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_PATTERN.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_PATTERN.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_PATTERN.example = let count: int = 1

TRAIT_E_INVALID_LITERAL.summary = litteral invalide.
TRAIT_E_INVALID_LITERAL.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_LITERAL.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_LITERAL.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_LITERAL.example = let count: int = 1

TRAIT_E_INVALID_OPERATOR.summary = operateur invalide.
TRAIT_E_INVALID_OPERATOR.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_OPERATOR.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_OPERATOR.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_OPERATOR.example = let count: int = 1

TRAIT_E_INVALID_MODIFIER.summary = modifier invalide.
TRAIT_E_INVALID_MODIFIER.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_MODIFIER.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_MODIFIER.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_MODIFIER.example = let count: int = 1

TRAIT_E_MISSING_BODY.summary = body manquant.
TRAIT_E_MISSING_BODY.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_MISSING_BODY.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_MISSING_BODY.fix = make the expression type match the type contract named by the type checker
TRAIT_E_MISSING_BODY.example = let count: int = 1

TRAIT_E_MISSING_RETURN.summary = retour manquant.
TRAIT_E_MISSING_RETURN.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_MISSING_RETURN.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_MISSING_RETURN.fix = make the expression type match the type contract named by the type checker
TRAIT_E_MISSING_RETURN.example = let count: int = 1

TRAIT_E_DUPLICATE_NAME.summary = nom duplique.
TRAIT_E_DUPLICATE_NAME.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_DUPLICATE_NAME.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_DUPLICATE_NAME.fix = make the expression type match the type contract named by the type checker
TRAIT_E_DUPLICATE_NAME.example = let count: int = 1

TRAIT_E_UNKNOWN_NAME.summary = nom inconnu.
TRAIT_E_UNKNOWN_NAME.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_UNKNOWN_NAME.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_UNKNOWN_NAME.fix = make the expression type match the type contract named by the type checker
TRAIT_E_UNKNOWN_NAME.example = let count: int = 1

TRAIT_E_UNKNOWN_TYPE.summary = type inconnu.
TRAIT_E_UNKNOWN_TYPE.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_UNKNOWN_TYPE.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_UNKNOWN_TYPE.fix = make the expression type match the type contract named by the type checker
TRAIT_E_UNKNOWN_TYPE.example = let count: int = 1

TRAIT_E_UNKNOWN_MODULE.summary = module inconnu.
TRAIT_E_UNKNOWN_MODULE.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_UNKNOWN_MODULE.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_UNKNOWN_MODULE.fix = make the expression type match the type contract named by the type checker
TRAIT_E_UNKNOWN_MODULE.example = let count: int = 1

TRAIT_E_UNKNOWN_MEMBER.summary = membre inconnu.
TRAIT_E_UNKNOWN_MEMBER.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_UNKNOWN_MEMBER.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_UNKNOWN_MEMBER.fix = make the expression type match the type contract named by the type checker
TRAIT_E_UNKNOWN_MEMBER.example = let count: int = 1

TRAIT_E_AMBIGUOUS_NAME.summary = ambiguous nom.
TRAIT_E_AMBIGUOUS_NAME.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_AMBIGUOUS_NAME.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_AMBIGUOUS_NAME.fix = make the expression type match the type contract named by the type checker
TRAIT_E_AMBIGUOUS_NAME.example = let count: int = 1

TRAIT_E_PRIVATE_SYMBOL.summary = private symbole.
TRAIT_E_PRIVATE_SYMBOL.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_PRIVATE_SYMBOL.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_PRIVATE_SYMBOL.fix = make the expression type match the type contract named by the type checker
TRAIT_E_PRIVATE_SYMBOL.example = let count: int = 1

TRAIT_E_IMPORT_NOT_FOUND.summary = import manquant.
TRAIT_E_IMPORT_NOT_FOUND.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_IMPORT_NOT_FOUND.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_IMPORT_NOT_FOUND.fix = make the expression type match the type contract named by the type checker
TRAIT_E_IMPORT_NOT_FOUND.example = let count: int = 1

TRAIT_E_IMPORT_CYCLE.summary = import cycle.
TRAIT_E_IMPORT_CYCLE.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_IMPORT_CYCLE.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_IMPORT_CYCLE.fix = make the expression type match the type contract named by the type checker
TRAIT_E_IMPORT_CYCLE.example = let count: int = 1

TRAIT_E_EXPORT_CONFLICT.summary = export conflit.
TRAIT_E_EXPORT_CONFLICT.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_EXPORT_CONFLICT.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_EXPORT_CONFLICT.fix = make the expression type match the type contract named by the type checker
TRAIT_E_EXPORT_CONFLICT.example = let count: int = 1

TRAIT_E_ARITY_MISMATCH.summary = arity incompatibilite.
TRAIT_E_ARITY_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_ARITY_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_ARITY_MISMATCH.fix = make the expression type match the type contract named by the type checker
TRAIT_E_ARITY_MISMATCH.example = let count: int = 1

TRAIT_E_ARGUMENT_MISMATCH.summary = argument incompatibilite.
TRAIT_E_ARGUMENT_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_ARGUMENT_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_ARGUMENT_MISMATCH.fix = make the expression type match the type contract named by the type checker
TRAIT_E_ARGUMENT_MISMATCH.example = let count: int = 1

TRAIT_E_ASSIGNMENT_MISMATCH.summary = affectation incompatibilite.
TRAIT_E_ASSIGNMENT_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_ASSIGNMENT_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_ASSIGNMENT_MISMATCH.fix = make the expression type match the type contract named by the type checker
TRAIT_E_ASSIGNMENT_MISMATCH.example = let count: int = 1

TRAIT_E_BRANCH_MISMATCH.summary = branch incompatibilite.
TRAIT_E_BRANCH_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_BRANCH_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_BRANCH_MISMATCH.fix = make the expression type match the type contract named by the type checker
TRAIT_E_BRANCH_MISMATCH.example = let count: int = 1

TRAIT_E_INVALID_CALL.summary = appel invalide.
TRAIT_E_INVALID_CALL.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_CALL.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_CALL.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_CALL.example = let count: int = 1

TRAIT_E_INVALID_CAST.summary = conversion invalide.
TRAIT_E_INVALID_CAST.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_CAST.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_CAST.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_CAST.example = let count: int = 1

TRAIT_E_INVALID_INDEX.summary = index invalide.
TRAIT_E_INVALID_INDEX.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_INDEX.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_INDEX.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_INDEX.example = let count: int = 1

TRAIT_E_INVALID_DEREF.summary = deref invalide.
TRAIT_E_INVALID_DEREF.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_DEREF.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_DEREF.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_DEREF.example = let count: int = 1

TRAIT_E_INVALID_BORROW.summary = emprunt invalide.
TRAIT_E_INVALID_BORROW.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_BORROW.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_BORROW.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_BORROW.example = let count: int = 1

TRAIT_E_INVALID_MOVE.summary = deplacement invalide.
TRAIT_E_INVALID_MOVE.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_MOVE.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_MOVE.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_MOVE.example = let count: int = 1

TRAIT_E_USE_AFTER_MOVE.summary = use after deplacement.
TRAIT_E_USE_AFTER_MOVE.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_USE_AFTER_MOVE.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_USE_AFTER_MOVE.fix = make the expression type match the type contract named by the type checker
TRAIT_E_USE_AFTER_MOVE.example = let count: int = 1

TRAIT_E_USE_AFTER_DROP.summary = use after destruction.
TRAIT_E_USE_AFTER_DROP.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_USE_AFTER_DROP.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_USE_AFTER_DROP.fix = make the expression type match the type contract named by the type checker
TRAIT_E_USE_AFTER_DROP.example = let count: int = 1

TRAIT_E_USE_BEFORE_INIT.summary = use before init.
TRAIT_E_USE_BEFORE_INIT.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_USE_BEFORE_INIT.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_USE_BEFORE_INIT.fix = make the expression type match the type contract named by the type checker
TRAIT_E_USE_BEFORE_INIT.example = let count: int = 1

TRAIT_E_DOUBLE_DROP.summary = double destruction.
TRAIT_E_DOUBLE_DROP.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_DOUBLE_DROP.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_DOUBLE_DROP.fix = make the expression type match the type contract named by the type checker
TRAIT_E_DOUBLE_DROP.example = let count: int = 1

TRAIT_E_BORROW_CONFLICT.summary = emprunt conflit.
TRAIT_E_BORROW_CONFLICT.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_BORROW_CONFLICT.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_BORROW_CONFLICT.fix = make the expression type match the type contract named by the type checker
TRAIT_E_BORROW_CONFLICT.example = let count: int = 1

TRAIT_E_MUTABILITY_CONFLICT.summary = mutability conflit.
TRAIT_E_MUTABILITY_CONFLICT.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_MUTABILITY_CONFLICT.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_MUTABILITY_CONFLICT.fix = make the expression type match the type contract named by the type checker
TRAIT_E_MUTABILITY_CONFLICT.example = let count: int = 1

TRAIT_E_LIFETIME_TOO_SHORT.summary = duree de vie trop short.
TRAIT_E_LIFETIME_TOO_SHORT.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_LIFETIME_TOO_SHORT.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_LIFETIME_TOO_SHORT.fix = make the expression type match the type contract named by the type checker
TRAIT_E_LIFETIME_TOO_SHORT.example = let count: int = 1

TRAIT_E_DANGLING_REFERENCE.summary = dangling reference.
TRAIT_E_DANGLING_REFERENCE.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_DANGLING_REFERENCE.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_DANGLING_REFERENCE.fix = make the expression type match the type contract named by the type checker
TRAIT_E_DANGLING_REFERENCE.example = let count: int = 1

TRAIT_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
TRAIT_E_NON_EXHAUSTIVE_MATCH.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_NON_EXHAUSTIVE_MATCH.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_NON_EXHAUSTIVE_MATCH.fix = make the expression type match the type contract named by the type checker
TRAIT_E_NON_EXHAUSTIVE_MATCH.example = let count: int = 1

TRAIT_E_UNREACHABLE_PATTERN.summary = inatteignable motif.
TRAIT_E_UNREACHABLE_PATTERN.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_UNREACHABLE_PATTERN.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_UNREACHABLE_PATTERN.fix = make the expression type match the type contract named by the type checker
TRAIT_E_UNREACHABLE_PATTERN.example = let count: int = 1

TRAIT_E_CONST_REQUIRED.summary = const required.
TRAIT_E_CONST_REQUIRED.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_CONST_REQUIRED.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_CONST_REQUIRED.fix = make the expression type match the type contract named by the type checker
TRAIT_E_CONST_REQUIRED.example = let count: int = 1

TRAIT_E_CONST_OVERFLOW.summary = const depassement.
TRAIT_E_CONST_OVERFLOW.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_CONST_OVERFLOW.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_CONST_OVERFLOW.fix = make the expression type match the type contract named by the type checker
TRAIT_E_CONST_OVERFLOW.example = let count: int = 1

TRAIT_E_CONST_DIVISION_BY_ZERO.summary = const division by zero.
TRAIT_E_CONST_DIVISION_BY_ZERO.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_CONST_DIVISION_BY_ZERO.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_CONST_DIVISION_BY_ZERO.fix = make the expression type match the type contract named by the type checker
TRAIT_E_CONST_DIVISION_BY_ZERO.example = let count: int = 1

TRAIT_E_CONST_CYCLE.summary = const cycle.
TRAIT_E_CONST_CYCLE.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_CONST_CYCLE.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_CONST_CYCLE.fix = make the expression type match the type contract named by the type checker
TRAIT_E_CONST_CYCLE.example = let count: int = 1

TRAIT_E_MACRO_NOT_FOUND.summary = macro manquant.
TRAIT_E_MACRO_NOT_FOUND.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_MACRO_NOT_FOUND.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_MACRO_NOT_FOUND.fix = make the expression type match the type contract named by the type checker
TRAIT_E_MACRO_NOT_FOUND.example = let count: int = 1

TRAIT_E_MACRO_RECURSION.summary = macro recursion.
TRAIT_E_MACRO_RECURSION.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_MACRO_RECURSION.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_MACRO_RECURSION.fix = make the expression type match the type contract named by the type checker
TRAIT_E_MACRO_RECURSION.example = let count: int = 1

TRAIT_E_MACRO_EXPANSION_FAILED.summary = macro expansion echec.
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

TRAIT_E_GENERIC_ARGUMENT_MISSING.summary = generique argument manquant.
TRAIT_E_GENERIC_ARGUMENT_MISSING.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_GENERIC_ARGUMENT_MISSING.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_GENERIC_ARGUMENT_MISSING.fix = make the expression type match the type contract named by the type checker
TRAIT_E_GENERIC_ARGUMENT_MISSING.example = let count: int = 1

TRAIT_E_GENERIC_BOUND_FAILED.summary = generique bound echec.
TRAIT_E_GENERIC_BOUND_FAILED.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_GENERIC_BOUND_FAILED.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_GENERIC_BOUND_FAILED.fix = make the expression type match the type contract named by the type checker
TRAIT_E_GENERIC_BOUND_FAILED.example = let count: int = 1

TRAIT_E_UNSUPPORTED_TARGET.summary = cible non pris en charge.
TRAIT_E_UNSUPPORTED_TARGET.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_UNSUPPORTED_TARGET.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_UNSUPPORTED_TARGET.fix = make the expression type match the type contract named by the type checker
TRAIT_E_UNSUPPORTED_TARGET.example = let count: int = 1

TRAIT_E_ABI_MISMATCH.summary = abi incompatibilite.
TRAIT_E_ABI_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_ABI_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_ABI_MISMATCH.fix = make the expression type match the type contract named by the type checker
TRAIT_E_ABI_MISMATCH.example = let count: int = 1

TRAIT_E_LINK_FAILED.summary = link echec.
TRAIT_E_LINK_FAILED.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_LINK_FAILED.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_LINK_FAILED.fix = make the expression type match the type contract named by the type checker
TRAIT_E_LINK_FAILED.example = let count: int = 1

TRAIT_E_RUNTIME_PANIC.summary = runtime panique.
TRAIT_E_RUNTIME_PANIC.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_RUNTIME_PANIC.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_RUNTIME_PANIC.fix = make the expression type match the type contract named by the type checker
TRAIT_E_RUNTIME_PANIC.example = let count: int = 1

OWNERSHIP_E_EXPECTED_IDENTIFIER.summary = identifiant attendu.
OWNERSHIP_E_EXPECTED_IDENTIFIER.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_EXPECTED_IDENTIFIER.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_EXPECTED_IDENTIFIER.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_EXPECTED_IDENTIFIER.example = let view = &value

OWNERSHIP_E_EXPECTED_EXPRESSION.summary = expression attendu.
OWNERSHIP_E_EXPECTED_EXPRESSION.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_EXPECTED_EXPRESSION.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_EXPECTED_EXPRESSION.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_EXPECTED_EXPRESSION.example = let view = &value

OWNERSHIP_E_EXPECTED_TYPE.summary = type attendu.
OWNERSHIP_E_EXPECTED_TYPE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_EXPECTED_TYPE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_EXPECTED_TYPE.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_EXPECTED_TYPE.example = let view = &value

OWNERSHIP_E_EXPECTED_PATTERN.summary = motif attendu.
OWNERSHIP_E_EXPECTED_PATTERN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_EXPECTED_PATTERN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_EXPECTED_PATTERN.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_EXPECTED_PATTERN.example = let view = &value

OWNERSHIP_E_EXPECTED_BLOCK.summary = bloc attendu.
OWNERSHIP_E_EXPECTED_BLOCK.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_EXPECTED_BLOCK.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_EXPECTED_BLOCK.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_EXPECTED_BLOCK.example = let view = &value

OWNERSHIP_E_EXPECTED_DELIMITER.summary = delimiteur attendu.
OWNERSHIP_E_EXPECTED_DELIMITER.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_EXPECTED_DELIMITER.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_EXPECTED_DELIMITER.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_EXPECTED_DELIMITER.example = let view = &value

OWNERSHIP_E_UNEXPECTED_TOKEN.summary = jeton inattendu.
OWNERSHIP_E_UNEXPECTED_TOKEN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_UNEXPECTED_TOKEN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_UNEXPECTED_TOKEN.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_UNEXPECTED_TOKEN.example = let view = &value

OWNERSHIP_E_UNBALANCED_DELIMITER.summary = unbalanced delimiteur.
OWNERSHIP_E_UNBALANCED_DELIMITER.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_UNBALANCED_DELIMITER.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_UNBALANCED_DELIMITER.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_UNBALANCED_DELIMITER.example = let view = &value

OWNERSHIP_E_INVALID_ATTRIBUTE.summary = attribute invalide.
OWNERSHIP_E_INVALID_ATTRIBUTE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_ATTRIBUTE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_ATTRIBUTE.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_ATTRIBUTE.example = let view = &value

OWNERSHIP_E_INVALID_DECLARATION.summary = declaration invalide.
OWNERSHIP_E_INVALID_DECLARATION.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_DECLARATION.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_DECLARATION.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_DECLARATION.example = let view = &value

OWNERSHIP_E_INVALID_STATEMENT.summary = instruction invalide.
OWNERSHIP_E_INVALID_STATEMENT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_STATEMENT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_STATEMENT.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_STATEMENT.example = let view = &value

OWNERSHIP_E_INVALID_EXPRESSION.summary = expression invalide.
OWNERSHIP_E_INVALID_EXPRESSION.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_EXPRESSION.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_EXPRESSION.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_EXPRESSION.example = let view = &value

OWNERSHIP_E_INVALID_PATTERN.summary = motif invalide.
OWNERSHIP_E_INVALID_PATTERN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_PATTERN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_PATTERN.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_PATTERN.example = let view = &value

OWNERSHIP_E_INVALID_LITERAL.summary = litteral invalide.
OWNERSHIP_E_INVALID_LITERAL.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_LITERAL.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_LITERAL.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_LITERAL.example = let view = &value

OWNERSHIP_E_INVALID_OPERATOR.summary = operateur invalide.
OWNERSHIP_E_INVALID_OPERATOR.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_OPERATOR.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_OPERATOR.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_OPERATOR.example = let view = &value

OWNERSHIP_E_INVALID_MODIFIER.summary = modifier invalide.
OWNERSHIP_E_INVALID_MODIFIER.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_MODIFIER.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_MODIFIER.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_MODIFIER.example = let view = &value

OWNERSHIP_E_MISSING_BODY.summary = body manquant.
OWNERSHIP_E_MISSING_BODY.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_MISSING_BODY.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_MISSING_BODY.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_MISSING_BODY.example = let view = &value

OWNERSHIP_E_MISSING_RETURN.summary = retour manquant.
OWNERSHIP_E_MISSING_RETURN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_MISSING_RETURN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_MISSING_RETURN.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_MISSING_RETURN.example = let view = &value

OWNERSHIP_E_DUPLICATE_NAME.summary = nom duplique.
OWNERSHIP_E_DUPLICATE_NAME.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_DUPLICATE_NAME.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_DUPLICATE_NAME.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_DUPLICATE_NAME.example = let view = &value

OWNERSHIP_E_UNKNOWN_NAME.summary = nom inconnu.
OWNERSHIP_E_UNKNOWN_NAME.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_UNKNOWN_NAME.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_UNKNOWN_NAME.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_UNKNOWN_NAME.example = let view = &value

OWNERSHIP_E_UNKNOWN_TYPE.summary = type inconnu.
OWNERSHIP_E_UNKNOWN_TYPE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_UNKNOWN_TYPE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_UNKNOWN_TYPE.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_UNKNOWN_TYPE.example = let view = &value

OWNERSHIP_E_UNKNOWN_MODULE.summary = module inconnu.
OWNERSHIP_E_UNKNOWN_MODULE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_UNKNOWN_MODULE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_UNKNOWN_MODULE.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_UNKNOWN_MODULE.example = let view = &value

OWNERSHIP_E_UNKNOWN_MEMBER.summary = membre inconnu.
OWNERSHIP_E_UNKNOWN_MEMBER.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_UNKNOWN_MEMBER.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_UNKNOWN_MEMBER.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_UNKNOWN_MEMBER.example = let view = &value

OWNERSHIP_E_AMBIGUOUS_NAME.summary = ambiguous nom.
OWNERSHIP_E_AMBIGUOUS_NAME.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_AMBIGUOUS_NAME.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_AMBIGUOUS_NAME.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_AMBIGUOUS_NAME.example = let view = &value

OWNERSHIP_E_PRIVATE_SYMBOL.summary = private symbole.
OWNERSHIP_E_PRIVATE_SYMBOL.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_PRIVATE_SYMBOL.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_PRIVATE_SYMBOL.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_PRIVATE_SYMBOL.example = let view = &value

OWNERSHIP_E_IMPORT_NOT_FOUND.summary = import manquant.
OWNERSHIP_E_IMPORT_NOT_FOUND.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_IMPORT_NOT_FOUND.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_IMPORT_NOT_FOUND.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_IMPORT_NOT_FOUND.example = let view = &value

OWNERSHIP_E_IMPORT_CYCLE.summary = import cycle.
OWNERSHIP_E_IMPORT_CYCLE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_IMPORT_CYCLE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_IMPORT_CYCLE.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_IMPORT_CYCLE.example = let view = &value

OWNERSHIP_E_EXPORT_CONFLICT.summary = export conflit.
OWNERSHIP_E_EXPORT_CONFLICT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_EXPORT_CONFLICT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_EXPORT_CONFLICT.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_EXPORT_CONFLICT.example = let view = &value

OWNERSHIP_E_ARITY_MISMATCH.summary = arity incompatibilite.
OWNERSHIP_E_ARITY_MISMATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_ARITY_MISMATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_ARITY_MISMATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_ARITY_MISMATCH.example = let view = &value

OWNERSHIP_E_ARGUMENT_MISMATCH.summary = argument incompatibilite.
OWNERSHIP_E_ARGUMENT_MISMATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_ARGUMENT_MISMATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_ARGUMENT_MISMATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_ARGUMENT_MISMATCH.example = let view = &value

OWNERSHIP_E_ASSIGNMENT_MISMATCH.summary = affectation incompatibilite.
OWNERSHIP_E_ASSIGNMENT_MISMATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_ASSIGNMENT_MISMATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_ASSIGNMENT_MISMATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_ASSIGNMENT_MISMATCH.example = let view = &value

OWNERSHIP_E_BRANCH_MISMATCH.summary = branch incompatibilite.
OWNERSHIP_E_BRANCH_MISMATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_BRANCH_MISMATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_BRANCH_MISMATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_BRANCH_MISMATCH.example = let view = &value

OWNERSHIP_E_INVALID_CALL.summary = appel invalide.
OWNERSHIP_E_INVALID_CALL.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_CALL.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_CALL.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_CALL.example = let view = &value

OWNERSHIP_E_INVALID_CAST.summary = conversion invalide.
OWNERSHIP_E_INVALID_CAST.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_CAST.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_CAST.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_CAST.example = let view = &value

OWNERSHIP_E_INVALID_INDEX.summary = index invalide.
OWNERSHIP_E_INVALID_INDEX.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_INDEX.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_INDEX.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_INDEX.example = let view = &value

OWNERSHIP_E_INVALID_DEREF.summary = deref invalide.
OWNERSHIP_E_INVALID_DEREF.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_DEREF.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_DEREF.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_DEREF.example = let view = &value

OWNERSHIP_E_INVALID_BORROW.summary = emprunt invalide.
OWNERSHIP_E_INVALID_BORROW.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_BORROW.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_BORROW.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_BORROW.example = let view = &value

OWNERSHIP_E_INVALID_MOVE.summary = deplacement invalide.
OWNERSHIP_E_INVALID_MOVE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_MOVE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_MOVE.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_MOVE.example = let view = &value

OWNERSHIP_E_USE_AFTER_MOVE.summary = use after deplacement.
OWNERSHIP_E_USE_AFTER_MOVE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_USE_AFTER_MOVE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_USE_AFTER_MOVE.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_USE_AFTER_MOVE.example = let view = &value

OWNERSHIP_E_USE_AFTER_DROP.summary = use after destruction.
OWNERSHIP_E_USE_AFTER_DROP.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_USE_AFTER_DROP.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_USE_AFTER_DROP.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_USE_AFTER_DROP.example = let view = &value

OWNERSHIP_E_USE_BEFORE_INIT.summary = use before init.
OWNERSHIP_E_USE_BEFORE_INIT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_USE_BEFORE_INIT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_USE_BEFORE_INIT.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_USE_BEFORE_INIT.example = let view = &value

OWNERSHIP_E_DOUBLE_DROP.summary = double destruction.
OWNERSHIP_E_DOUBLE_DROP.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_DOUBLE_DROP.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_DOUBLE_DROP.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_DOUBLE_DROP.example = let view = &value

OWNERSHIP_E_BORROW_CONFLICT.summary = emprunt conflit.
OWNERSHIP_E_BORROW_CONFLICT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_BORROW_CONFLICT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_BORROW_CONFLICT.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_BORROW_CONFLICT.example = let view = &value

OWNERSHIP_E_MUTABILITY_CONFLICT.summary = mutability conflit.
OWNERSHIP_E_MUTABILITY_CONFLICT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_MUTABILITY_CONFLICT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_MUTABILITY_CONFLICT.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_MUTABILITY_CONFLICT.example = let view = &value

OWNERSHIP_E_LIFETIME_TOO_SHORT.summary = duree de vie trop short.
OWNERSHIP_E_LIFETIME_TOO_SHORT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_LIFETIME_TOO_SHORT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_LIFETIME_TOO_SHORT.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_LIFETIME_TOO_SHORT.example = let view = &value

OWNERSHIP_E_DANGLING_REFERENCE.summary = dangling reference.
OWNERSHIP_E_DANGLING_REFERENCE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_DANGLING_REFERENCE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_DANGLING_REFERENCE.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_DANGLING_REFERENCE.example = let view = &value

OWNERSHIP_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
OWNERSHIP_E_NON_EXHAUSTIVE_MATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_NON_EXHAUSTIVE_MATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_NON_EXHAUSTIVE_MATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_NON_EXHAUSTIVE_MATCH.example = let view = &value

OWNERSHIP_E_UNREACHABLE_PATTERN.summary = inatteignable motif.
OWNERSHIP_E_UNREACHABLE_PATTERN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_UNREACHABLE_PATTERN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_UNREACHABLE_PATTERN.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_UNREACHABLE_PATTERN.example = let view = &value

OWNERSHIP_E_CONST_REQUIRED.summary = const required.
OWNERSHIP_E_CONST_REQUIRED.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_CONST_REQUIRED.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_CONST_REQUIRED.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_CONST_REQUIRED.example = let view = &value

OWNERSHIP_E_CONST_OVERFLOW.summary = const depassement.
OWNERSHIP_E_CONST_OVERFLOW.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_CONST_OVERFLOW.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_CONST_OVERFLOW.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_CONST_OVERFLOW.example = let view = &value

OWNERSHIP_E_CONST_DIVISION_BY_ZERO.summary = const division by zero.
OWNERSHIP_E_CONST_DIVISION_BY_ZERO.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_CONST_DIVISION_BY_ZERO.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_CONST_DIVISION_BY_ZERO.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_CONST_DIVISION_BY_ZERO.example = let view = &value

OWNERSHIP_E_CONST_CYCLE.summary = const cycle.
OWNERSHIP_E_CONST_CYCLE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_CONST_CYCLE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_CONST_CYCLE.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_CONST_CYCLE.example = let view = &value

OWNERSHIP_E_MACRO_NOT_FOUND.summary = macro manquant.
OWNERSHIP_E_MACRO_NOT_FOUND.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_MACRO_NOT_FOUND.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_MACRO_NOT_FOUND.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_MACRO_NOT_FOUND.example = let view = &value

OWNERSHIP_E_MACRO_RECURSION.summary = macro recursion.
OWNERSHIP_E_MACRO_RECURSION.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_MACRO_RECURSION.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_MACRO_RECURSION.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_MACRO_RECURSION.example = let view = &value

OWNERSHIP_E_MACRO_EXPANSION_FAILED.summary = macro expansion echec.
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

OWNERSHIP_E_GENERIC_ARGUMENT_MISSING.summary = generique argument manquant.
OWNERSHIP_E_GENERIC_ARGUMENT_MISSING.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_GENERIC_ARGUMENT_MISSING.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_GENERIC_ARGUMENT_MISSING.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_GENERIC_ARGUMENT_MISSING.example = let view = &value

OWNERSHIP_E_GENERIC_BOUND_FAILED.summary = generique bound echec.
OWNERSHIP_E_GENERIC_BOUND_FAILED.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_GENERIC_BOUND_FAILED.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_GENERIC_BOUND_FAILED.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_GENERIC_BOUND_FAILED.example = let view = &value

OWNERSHIP_E_UNSUPPORTED_TARGET.summary = cible non pris en charge.
OWNERSHIP_E_UNSUPPORTED_TARGET.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_UNSUPPORTED_TARGET.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_UNSUPPORTED_TARGET.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_UNSUPPORTED_TARGET.example = let view = &value

OWNERSHIP_E_ABI_MISMATCH.summary = abi incompatibilite.
OWNERSHIP_E_ABI_MISMATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_ABI_MISMATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_ABI_MISMATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_ABI_MISMATCH.example = let view = &value

OWNERSHIP_E_LINK_FAILED.summary = link echec.
OWNERSHIP_E_LINK_FAILED.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_LINK_FAILED.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_LINK_FAILED.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_LINK_FAILED.example = let view = &value

OWNERSHIP_E_RUNTIME_PANIC.summary = runtime panique.
OWNERSHIP_E_RUNTIME_PANIC.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_RUNTIME_PANIC.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_RUNTIME_PANIC.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_RUNTIME_PANIC.example = let view = &value

LIFETIME_E_EXPECTED_IDENTIFIER.summary = identifiant attendu.
LIFETIME_E_EXPECTED_IDENTIFIER.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_EXPECTED_IDENTIFIER.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_EXPECTED_IDENTIFIER.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_EXPECTED_IDENTIFIER.example = let view = &value

LIFETIME_E_EXPECTED_EXPRESSION.summary = expression attendu.
LIFETIME_E_EXPECTED_EXPRESSION.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_EXPECTED_EXPRESSION.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_EXPECTED_EXPRESSION.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_EXPECTED_EXPRESSION.example = let view = &value

LIFETIME_E_EXPECTED_TYPE.summary = type attendu.
LIFETIME_E_EXPECTED_TYPE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_EXPECTED_TYPE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_EXPECTED_TYPE.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_EXPECTED_TYPE.example = let view = &value

LIFETIME_E_EXPECTED_PATTERN.summary = motif attendu.
LIFETIME_E_EXPECTED_PATTERN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_EXPECTED_PATTERN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_EXPECTED_PATTERN.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_EXPECTED_PATTERN.example = let view = &value

LIFETIME_E_EXPECTED_BLOCK.summary = bloc attendu.
LIFETIME_E_EXPECTED_BLOCK.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_EXPECTED_BLOCK.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_EXPECTED_BLOCK.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_EXPECTED_BLOCK.example = let view = &value

LIFETIME_E_EXPECTED_DELIMITER.summary = delimiteur attendu.
LIFETIME_E_EXPECTED_DELIMITER.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_EXPECTED_DELIMITER.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_EXPECTED_DELIMITER.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_EXPECTED_DELIMITER.example = let view = &value

LIFETIME_E_UNEXPECTED_TOKEN.summary = jeton inattendu.
LIFETIME_E_UNEXPECTED_TOKEN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_UNEXPECTED_TOKEN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_UNEXPECTED_TOKEN.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_UNEXPECTED_TOKEN.example = let view = &value

LIFETIME_E_UNBALANCED_DELIMITER.summary = unbalanced delimiteur.
LIFETIME_E_UNBALANCED_DELIMITER.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_UNBALANCED_DELIMITER.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_UNBALANCED_DELIMITER.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_UNBALANCED_DELIMITER.example = let view = &value

LIFETIME_E_INVALID_ATTRIBUTE.summary = attribute invalide.
LIFETIME_E_INVALID_ATTRIBUTE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_ATTRIBUTE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_ATTRIBUTE.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_ATTRIBUTE.example = let view = &value

LIFETIME_E_INVALID_DECLARATION.summary = declaration invalide.
LIFETIME_E_INVALID_DECLARATION.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_DECLARATION.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_DECLARATION.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_DECLARATION.example = let view = &value

LIFETIME_E_INVALID_STATEMENT.summary = instruction invalide.
LIFETIME_E_INVALID_STATEMENT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_STATEMENT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_STATEMENT.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_STATEMENT.example = let view = &value

LIFETIME_E_INVALID_EXPRESSION.summary = expression invalide.
LIFETIME_E_INVALID_EXPRESSION.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_EXPRESSION.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_EXPRESSION.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_EXPRESSION.example = let view = &value

LIFETIME_E_INVALID_PATTERN.summary = motif invalide.
LIFETIME_E_INVALID_PATTERN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_PATTERN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_PATTERN.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_PATTERN.example = let view = &value

LIFETIME_E_INVALID_LITERAL.summary = litteral invalide.
LIFETIME_E_INVALID_LITERAL.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_LITERAL.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_LITERAL.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_LITERAL.example = let view = &value

LIFETIME_E_INVALID_OPERATOR.summary = operateur invalide.
LIFETIME_E_INVALID_OPERATOR.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_OPERATOR.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_OPERATOR.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_OPERATOR.example = let view = &value

LIFETIME_E_INVALID_MODIFIER.summary = modifier invalide.
LIFETIME_E_INVALID_MODIFIER.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_MODIFIER.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_MODIFIER.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_MODIFIER.example = let view = &value

LIFETIME_E_MISSING_BODY.summary = body manquant.
LIFETIME_E_MISSING_BODY.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_MISSING_BODY.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_MISSING_BODY.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_MISSING_BODY.example = let view = &value

LIFETIME_E_MISSING_RETURN.summary = retour manquant.
LIFETIME_E_MISSING_RETURN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_MISSING_RETURN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_MISSING_RETURN.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_MISSING_RETURN.example = let view = &value

LIFETIME_E_DUPLICATE_NAME.summary = nom duplique.
LIFETIME_E_DUPLICATE_NAME.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_DUPLICATE_NAME.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_DUPLICATE_NAME.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_DUPLICATE_NAME.example = let view = &value

LIFETIME_E_UNKNOWN_NAME.summary = nom inconnu.
LIFETIME_E_UNKNOWN_NAME.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_UNKNOWN_NAME.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_UNKNOWN_NAME.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_UNKNOWN_NAME.example = let view = &value

LIFETIME_E_UNKNOWN_TYPE.summary = type inconnu.
LIFETIME_E_UNKNOWN_TYPE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_UNKNOWN_TYPE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_UNKNOWN_TYPE.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_UNKNOWN_TYPE.example = let view = &value

LIFETIME_E_UNKNOWN_MODULE.summary = module inconnu.
LIFETIME_E_UNKNOWN_MODULE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_UNKNOWN_MODULE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_UNKNOWN_MODULE.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_UNKNOWN_MODULE.example = let view = &value

LIFETIME_E_UNKNOWN_MEMBER.summary = membre inconnu.
LIFETIME_E_UNKNOWN_MEMBER.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_UNKNOWN_MEMBER.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_UNKNOWN_MEMBER.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_UNKNOWN_MEMBER.example = let view = &value

LIFETIME_E_AMBIGUOUS_NAME.summary = ambiguous nom.
LIFETIME_E_AMBIGUOUS_NAME.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_AMBIGUOUS_NAME.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_AMBIGUOUS_NAME.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_AMBIGUOUS_NAME.example = let view = &value

LIFETIME_E_PRIVATE_SYMBOL.summary = private symbole.
LIFETIME_E_PRIVATE_SYMBOL.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_PRIVATE_SYMBOL.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_PRIVATE_SYMBOL.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_PRIVATE_SYMBOL.example = let view = &value

LIFETIME_E_IMPORT_NOT_FOUND.summary = import manquant.
LIFETIME_E_IMPORT_NOT_FOUND.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_IMPORT_NOT_FOUND.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_IMPORT_NOT_FOUND.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_IMPORT_NOT_FOUND.example = let view = &value

LIFETIME_E_IMPORT_CYCLE.summary = import cycle.
LIFETIME_E_IMPORT_CYCLE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_IMPORT_CYCLE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_IMPORT_CYCLE.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_IMPORT_CYCLE.example = let view = &value

LIFETIME_E_EXPORT_CONFLICT.summary = export conflit.
LIFETIME_E_EXPORT_CONFLICT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_EXPORT_CONFLICT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_EXPORT_CONFLICT.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_EXPORT_CONFLICT.example = let view = &value

LIFETIME_E_ARITY_MISMATCH.summary = arity incompatibilite.
LIFETIME_E_ARITY_MISMATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_ARITY_MISMATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_ARITY_MISMATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_ARITY_MISMATCH.example = let view = &value

LIFETIME_E_ARGUMENT_MISMATCH.summary = argument incompatibilite.
LIFETIME_E_ARGUMENT_MISMATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_ARGUMENT_MISMATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_ARGUMENT_MISMATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_ARGUMENT_MISMATCH.example = let view = &value

LIFETIME_E_ASSIGNMENT_MISMATCH.summary = affectation incompatibilite.
LIFETIME_E_ASSIGNMENT_MISMATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_ASSIGNMENT_MISMATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_ASSIGNMENT_MISMATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_ASSIGNMENT_MISMATCH.example = let view = &value

LIFETIME_E_BRANCH_MISMATCH.summary = branch incompatibilite.
LIFETIME_E_BRANCH_MISMATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_BRANCH_MISMATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_BRANCH_MISMATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_BRANCH_MISMATCH.example = let view = &value

LIFETIME_E_INVALID_CALL.summary = appel invalide.
LIFETIME_E_INVALID_CALL.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_CALL.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_CALL.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_CALL.example = let view = &value

LIFETIME_E_INVALID_CAST.summary = conversion invalide.
LIFETIME_E_INVALID_CAST.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_CAST.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_CAST.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_CAST.example = let view = &value

LIFETIME_E_INVALID_INDEX.summary = index invalide.
LIFETIME_E_INVALID_INDEX.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_INDEX.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_INDEX.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_INDEX.example = let view = &value

LIFETIME_E_INVALID_DEREF.summary = deref invalide.
LIFETIME_E_INVALID_DEREF.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_DEREF.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_DEREF.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_DEREF.example = let view = &value

LIFETIME_E_INVALID_BORROW.summary = emprunt invalide.
LIFETIME_E_INVALID_BORROW.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_BORROW.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_BORROW.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_BORROW.example = let view = &value

LIFETIME_E_INVALID_MOVE.summary = deplacement invalide.
LIFETIME_E_INVALID_MOVE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_MOVE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_MOVE.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_MOVE.example = let view = &value

LIFETIME_E_USE_AFTER_MOVE.summary = use after deplacement.
LIFETIME_E_USE_AFTER_MOVE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_USE_AFTER_MOVE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_USE_AFTER_MOVE.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_USE_AFTER_MOVE.example = let view = &value

LIFETIME_E_USE_AFTER_DROP.summary = use after destruction.
LIFETIME_E_USE_AFTER_DROP.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_USE_AFTER_DROP.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_USE_AFTER_DROP.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_USE_AFTER_DROP.example = let view = &value

LIFETIME_E_USE_BEFORE_INIT.summary = use before init.
LIFETIME_E_USE_BEFORE_INIT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_USE_BEFORE_INIT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_USE_BEFORE_INIT.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_USE_BEFORE_INIT.example = let view = &value

LIFETIME_E_DOUBLE_DROP.summary = double destruction.
LIFETIME_E_DOUBLE_DROP.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_DOUBLE_DROP.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_DOUBLE_DROP.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_DOUBLE_DROP.example = let view = &value

LIFETIME_E_BORROW_CONFLICT.summary = emprunt conflit.
LIFETIME_E_BORROW_CONFLICT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_BORROW_CONFLICT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_BORROW_CONFLICT.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_BORROW_CONFLICT.example = let view = &value

LIFETIME_E_MUTABILITY_CONFLICT.summary = mutability conflit.
LIFETIME_E_MUTABILITY_CONFLICT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_MUTABILITY_CONFLICT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_MUTABILITY_CONFLICT.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_MUTABILITY_CONFLICT.example = let view = &value

LIFETIME_E_LIFETIME_TOO_SHORT.summary = duree de vie trop short.
LIFETIME_E_LIFETIME_TOO_SHORT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_LIFETIME_TOO_SHORT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_LIFETIME_TOO_SHORT.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_LIFETIME_TOO_SHORT.example = let view = &value

LIFETIME_E_DANGLING_REFERENCE.summary = dangling reference.
LIFETIME_E_DANGLING_REFERENCE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_DANGLING_REFERENCE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_DANGLING_REFERENCE.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_DANGLING_REFERENCE.example = let view = &value

LIFETIME_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
LIFETIME_E_NON_EXHAUSTIVE_MATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_NON_EXHAUSTIVE_MATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_NON_EXHAUSTIVE_MATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_NON_EXHAUSTIVE_MATCH.example = let view = &value

LIFETIME_E_UNREACHABLE_PATTERN.summary = inatteignable motif.
LIFETIME_E_UNREACHABLE_PATTERN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_UNREACHABLE_PATTERN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_UNREACHABLE_PATTERN.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_UNREACHABLE_PATTERN.example = let view = &value

LIFETIME_E_CONST_REQUIRED.summary = const required.
LIFETIME_E_CONST_REQUIRED.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_CONST_REQUIRED.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_CONST_REQUIRED.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_CONST_REQUIRED.example = let view = &value

LIFETIME_E_CONST_OVERFLOW.summary = const depassement.
LIFETIME_E_CONST_OVERFLOW.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_CONST_OVERFLOW.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_CONST_OVERFLOW.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_CONST_OVERFLOW.example = let view = &value

LIFETIME_E_CONST_DIVISION_BY_ZERO.summary = const division by zero.
LIFETIME_E_CONST_DIVISION_BY_ZERO.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_CONST_DIVISION_BY_ZERO.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_CONST_DIVISION_BY_ZERO.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_CONST_DIVISION_BY_ZERO.example = let view = &value

LIFETIME_E_CONST_CYCLE.summary = const cycle.
LIFETIME_E_CONST_CYCLE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_CONST_CYCLE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_CONST_CYCLE.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_CONST_CYCLE.example = let view = &value

LIFETIME_E_MACRO_NOT_FOUND.summary = macro manquant.
LIFETIME_E_MACRO_NOT_FOUND.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_MACRO_NOT_FOUND.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_MACRO_NOT_FOUND.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_MACRO_NOT_FOUND.example = let view = &value

LIFETIME_E_MACRO_RECURSION.summary = macro recursion.
LIFETIME_E_MACRO_RECURSION.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_MACRO_RECURSION.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_MACRO_RECURSION.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_MACRO_RECURSION.example = let view = &value

LIFETIME_E_MACRO_EXPANSION_FAILED.summary = macro expansion echec.
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

LIFETIME_E_GENERIC_ARGUMENT_MISSING.summary = generique argument manquant.
LIFETIME_E_GENERIC_ARGUMENT_MISSING.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_GENERIC_ARGUMENT_MISSING.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_GENERIC_ARGUMENT_MISSING.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_GENERIC_ARGUMENT_MISSING.example = let view = &value

LIFETIME_E_GENERIC_BOUND_FAILED.summary = generique bound echec.
LIFETIME_E_GENERIC_BOUND_FAILED.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_GENERIC_BOUND_FAILED.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_GENERIC_BOUND_FAILED.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_GENERIC_BOUND_FAILED.example = let view = &value

LIFETIME_E_UNSUPPORTED_TARGET.summary = cible non pris en charge.
LIFETIME_E_UNSUPPORTED_TARGET.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_UNSUPPORTED_TARGET.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_UNSUPPORTED_TARGET.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_UNSUPPORTED_TARGET.example = let view = &value

LIFETIME_E_ABI_MISMATCH.summary = abi incompatibilite.
LIFETIME_E_ABI_MISMATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_ABI_MISMATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_ABI_MISMATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_ABI_MISMATCH.example = let view = &value

LIFETIME_E_LINK_FAILED.summary = link echec.
LIFETIME_E_LINK_FAILED.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_LINK_FAILED.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_LINK_FAILED.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_LINK_FAILED.example = let view = &value

LIFETIME_E_RUNTIME_PANIC.summary = runtime panique.
LIFETIME_E_RUNTIME_PANIC.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_RUNTIME_PANIC.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_RUNTIME_PANIC.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_RUNTIME_PANIC.example = let view = &value

CONST_E_EXPECTED_IDENTIFIER.summary = identifiant attendu.
CONST_E_EXPECTED_IDENTIFIER.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_EXPECTED_IDENTIFIER.step1 = Reduce the constant expression at the reported span.
CONST_E_EXPECTED_IDENTIFIER.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_EXPECTED_IDENTIFIER.example = const size: int = 4

CONST_E_EXPECTED_EXPRESSION.summary = expression attendu.
CONST_E_EXPECTED_EXPRESSION.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_EXPECTED_EXPRESSION.step1 = Reduce the constant expression at the reported span.
CONST_E_EXPECTED_EXPRESSION.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_EXPECTED_EXPRESSION.example = const size: int = 4

CONST_E_EXPECTED_TYPE.summary = type attendu.
CONST_E_EXPECTED_TYPE.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_EXPECTED_TYPE.step1 = Reduce the constant expression at the reported span.
CONST_E_EXPECTED_TYPE.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_EXPECTED_TYPE.example = const size: int = 4

CONST_E_EXPECTED_PATTERN.summary = motif attendu.
CONST_E_EXPECTED_PATTERN.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_EXPECTED_PATTERN.step1 = Reduce the constant expression at the reported span.
CONST_E_EXPECTED_PATTERN.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_EXPECTED_PATTERN.example = const size: int = 4

CONST_E_EXPECTED_BLOCK.summary = bloc attendu.
CONST_E_EXPECTED_BLOCK.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_EXPECTED_BLOCK.step1 = Reduce the constant expression at the reported span.
CONST_E_EXPECTED_BLOCK.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_EXPECTED_BLOCK.example = const size: int = 4

CONST_E_EXPECTED_DELIMITER.summary = delimiteur attendu.
CONST_E_EXPECTED_DELIMITER.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_EXPECTED_DELIMITER.step1 = Reduce the constant expression at the reported span.
CONST_E_EXPECTED_DELIMITER.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_EXPECTED_DELIMITER.example = const size: int = 4

CONST_E_UNEXPECTED_TOKEN.summary = jeton inattendu.
CONST_E_UNEXPECTED_TOKEN.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_UNEXPECTED_TOKEN.step1 = Reduce the constant expression at the reported span.
CONST_E_UNEXPECTED_TOKEN.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_UNEXPECTED_TOKEN.example = const size: int = 4

CONST_E_UNBALANCED_DELIMITER.summary = unbalanced delimiteur.
CONST_E_UNBALANCED_DELIMITER.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_UNBALANCED_DELIMITER.step1 = Reduce the constant expression at the reported span.
CONST_E_UNBALANCED_DELIMITER.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_UNBALANCED_DELIMITER.example = const size: int = 4

CONST_E_INVALID_ATTRIBUTE.summary = attribute invalide.
CONST_E_INVALID_ATTRIBUTE.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_ATTRIBUTE.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_ATTRIBUTE.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_ATTRIBUTE.example = const size: int = 4

CONST_E_INVALID_DECLARATION.summary = declaration invalide.
CONST_E_INVALID_DECLARATION.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_DECLARATION.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_DECLARATION.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_DECLARATION.example = const size: int = 4

CONST_E_INVALID_STATEMENT.summary = instruction invalide.
CONST_E_INVALID_STATEMENT.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_STATEMENT.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_STATEMENT.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_STATEMENT.example = const size: int = 4

CONST_E_INVALID_EXPRESSION.summary = expression invalide.
CONST_E_INVALID_EXPRESSION.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_EXPRESSION.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_EXPRESSION.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_EXPRESSION.example = const size: int = 4

CONST_E_INVALID_PATTERN.summary = motif invalide.
CONST_E_INVALID_PATTERN.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_PATTERN.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_PATTERN.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_PATTERN.example = const size: int = 4

CONST_E_INVALID_LITERAL.summary = litteral invalide.
CONST_E_INVALID_LITERAL.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_LITERAL.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_LITERAL.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_LITERAL.example = const size: int = 4

CONST_E_INVALID_OPERATOR.summary = operateur invalide.
CONST_E_INVALID_OPERATOR.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_OPERATOR.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_OPERATOR.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_OPERATOR.example = const size: int = 4

CONST_E_INVALID_MODIFIER.summary = modifier invalide.
CONST_E_INVALID_MODIFIER.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_MODIFIER.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_MODIFIER.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_MODIFIER.example = const size: int = 4

CONST_E_MISSING_BODY.summary = body manquant.
CONST_E_MISSING_BODY.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_MISSING_BODY.step1 = Reduce the constant expression at the reported span.
CONST_E_MISSING_BODY.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_MISSING_BODY.example = const size: int = 4

CONST_E_MISSING_RETURN.summary = retour manquant.
CONST_E_MISSING_RETURN.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_MISSING_RETURN.step1 = Reduce the constant expression at the reported span.
CONST_E_MISSING_RETURN.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_MISSING_RETURN.example = const size: int = 4

CONST_E_DUPLICATE_NAME.summary = nom duplique.
CONST_E_DUPLICATE_NAME.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_DUPLICATE_NAME.step1 = Reduce the constant expression at the reported span.
CONST_E_DUPLICATE_NAME.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_DUPLICATE_NAME.example = const size: int = 4

CONST_E_UNKNOWN_NAME.summary = nom inconnu.
CONST_E_UNKNOWN_NAME.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_UNKNOWN_NAME.step1 = Reduce the constant expression at the reported span.
CONST_E_UNKNOWN_NAME.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_UNKNOWN_NAME.example = const size: int = 4

CONST_E_UNKNOWN_TYPE.summary = type inconnu.
CONST_E_UNKNOWN_TYPE.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_UNKNOWN_TYPE.step1 = Reduce the constant expression at the reported span.
CONST_E_UNKNOWN_TYPE.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_UNKNOWN_TYPE.example = const size: int = 4

CONST_E_UNKNOWN_MODULE.summary = module inconnu.
CONST_E_UNKNOWN_MODULE.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_UNKNOWN_MODULE.step1 = Reduce the constant expression at the reported span.
CONST_E_UNKNOWN_MODULE.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_UNKNOWN_MODULE.example = const size: int = 4

CONST_E_UNKNOWN_MEMBER.summary = membre inconnu.
CONST_E_UNKNOWN_MEMBER.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_UNKNOWN_MEMBER.step1 = Reduce the constant expression at the reported span.
CONST_E_UNKNOWN_MEMBER.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_UNKNOWN_MEMBER.example = const size: int = 4

CONST_E_AMBIGUOUS_NAME.summary = ambiguous nom.
CONST_E_AMBIGUOUS_NAME.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_AMBIGUOUS_NAME.step1 = Reduce the constant expression at the reported span.
CONST_E_AMBIGUOUS_NAME.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_AMBIGUOUS_NAME.example = const size: int = 4

CONST_E_PRIVATE_SYMBOL.summary = private symbole.
CONST_E_PRIVATE_SYMBOL.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_PRIVATE_SYMBOL.step1 = Reduce the constant expression at the reported span.
CONST_E_PRIVATE_SYMBOL.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_PRIVATE_SYMBOL.example = const size: int = 4

CONST_E_IMPORT_NOT_FOUND.summary = import manquant.
CONST_E_IMPORT_NOT_FOUND.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_IMPORT_NOT_FOUND.step1 = Reduce the constant expression at the reported span.
CONST_E_IMPORT_NOT_FOUND.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_IMPORT_NOT_FOUND.example = const size: int = 4

CONST_E_IMPORT_CYCLE.summary = import cycle.
CONST_E_IMPORT_CYCLE.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_IMPORT_CYCLE.step1 = Reduce the constant expression at the reported span.
CONST_E_IMPORT_CYCLE.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_IMPORT_CYCLE.example = const size: int = 4

CONST_E_EXPORT_CONFLICT.summary = export conflit.
CONST_E_EXPORT_CONFLICT.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_EXPORT_CONFLICT.step1 = Reduce the constant expression at the reported span.
CONST_E_EXPORT_CONFLICT.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_EXPORT_CONFLICT.example = const size: int = 4

CONST_E_ARITY_MISMATCH.summary = arity incompatibilite.
CONST_E_ARITY_MISMATCH.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_ARITY_MISMATCH.step1 = Reduce the constant expression at the reported span.
CONST_E_ARITY_MISMATCH.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_ARITY_MISMATCH.example = const size: int = 4

CONST_E_ARGUMENT_MISMATCH.summary = argument incompatibilite.
CONST_E_ARGUMENT_MISMATCH.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_ARGUMENT_MISMATCH.step1 = Reduce the constant expression at the reported span.
CONST_E_ARGUMENT_MISMATCH.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_ARGUMENT_MISMATCH.example = const size: int = 4

CONST_E_ASSIGNMENT_MISMATCH.summary = affectation incompatibilite.
CONST_E_ASSIGNMENT_MISMATCH.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_ASSIGNMENT_MISMATCH.step1 = Reduce the constant expression at the reported span.
CONST_E_ASSIGNMENT_MISMATCH.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_ASSIGNMENT_MISMATCH.example = const size: int = 4

CONST_E_BRANCH_MISMATCH.summary = branch incompatibilite.
CONST_E_BRANCH_MISMATCH.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_BRANCH_MISMATCH.step1 = Reduce the constant expression at the reported span.
CONST_E_BRANCH_MISMATCH.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_BRANCH_MISMATCH.example = const size: int = 4

CONST_E_INVALID_CALL.summary = appel invalide.
CONST_E_INVALID_CALL.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_CALL.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_CALL.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_CALL.example = const size: int = 4

CONST_E_INVALID_CAST.summary = conversion invalide.
CONST_E_INVALID_CAST.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_CAST.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_CAST.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_CAST.example = const size: int = 4

CONST_E_INVALID_INDEX.summary = index invalide.
CONST_E_INVALID_INDEX.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_INDEX.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_INDEX.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_INDEX.example = const size: int = 4

CONST_E_INVALID_DEREF.summary = deref invalide.
CONST_E_INVALID_DEREF.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_DEREF.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_DEREF.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_DEREF.example = const size: int = 4

CONST_E_INVALID_BORROW.summary = emprunt invalide.
CONST_E_INVALID_BORROW.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_BORROW.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_BORROW.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_BORROW.example = const size: int = 4

CONST_E_INVALID_MOVE.summary = deplacement invalide.
CONST_E_INVALID_MOVE.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_MOVE.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_MOVE.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_MOVE.example = const size: int = 4

CONST_E_USE_AFTER_MOVE.summary = use after deplacement.
CONST_E_USE_AFTER_MOVE.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_USE_AFTER_MOVE.step1 = Reduce the constant expression at the reported span.
CONST_E_USE_AFTER_MOVE.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_USE_AFTER_MOVE.example = const size: int = 4

CONST_E_USE_AFTER_DROP.summary = use after destruction.
CONST_E_USE_AFTER_DROP.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_USE_AFTER_DROP.step1 = Reduce the constant expression at the reported span.
CONST_E_USE_AFTER_DROP.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_USE_AFTER_DROP.example = const size: int = 4

CONST_E_USE_BEFORE_INIT.summary = use before init.
CONST_E_USE_BEFORE_INIT.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_USE_BEFORE_INIT.step1 = Reduce the constant expression at the reported span.
CONST_E_USE_BEFORE_INIT.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_USE_BEFORE_INIT.example = const size: int = 4

CONST_E_DOUBLE_DROP.summary = double destruction.
CONST_E_DOUBLE_DROP.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_DOUBLE_DROP.step1 = Reduce the constant expression at the reported span.
CONST_E_DOUBLE_DROP.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_DOUBLE_DROP.example = const size: int = 4

CONST_E_BORROW_CONFLICT.summary = emprunt conflit.
CONST_E_BORROW_CONFLICT.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_BORROW_CONFLICT.step1 = Reduce the constant expression at the reported span.
CONST_E_BORROW_CONFLICT.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_BORROW_CONFLICT.example = const size: int = 4

CONST_E_MUTABILITY_CONFLICT.summary = mutability conflit.
CONST_E_MUTABILITY_CONFLICT.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_MUTABILITY_CONFLICT.step1 = Reduce the constant expression at the reported span.
CONST_E_MUTABILITY_CONFLICT.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_MUTABILITY_CONFLICT.example = const size: int = 4

CONST_E_LIFETIME_TOO_SHORT.summary = duree de vie trop short.
CONST_E_LIFETIME_TOO_SHORT.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_LIFETIME_TOO_SHORT.step1 = Reduce the constant expression at the reported span.
CONST_E_LIFETIME_TOO_SHORT.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_LIFETIME_TOO_SHORT.example = const size: int = 4

CONST_E_DANGLING_REFERENCE.summary = dangling reference.
CONST_E_DANGLING_REFERENCE.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_DANGLING_REFERENCE.step1 = Reduce the constant expression at the reported span.
CONST_E_DANGLING_REFERENCE.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_DANGLING_REFERENCE.example = const size: int = 4

CONST_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
CONST_E_NON_EXHAUSTIVE_MATCH.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_NON_EXHAUSTIVE_MATCH.step1 = Reduce the constant expression at the reported span.
CONST_E_NON_EXHAUSTIVE_MATCH.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_NON_EXHAUSTIVE_MATCH.example = const size: int = 4

CONST_E_UNREACHABLE_PATTERN.summary = inatteignable motif.
CONST_E_UNREACHABLE_PATTERN.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_UNREACHABLE_PATTERN.step1 = Reduce the constant expression at the reported span.
CONST_E_UNREACHABLE_PATTERN.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_UNREACHABLE_PATTERN.example = const size: int = 4

CONST_E_CONST_REQUIRED.summary = const required.
CONST_E_CONST_REQUIRED.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_CONST_REQUIRED.step1 = Reduce the constant expression at the reported span.
CONST_E_CONST_REQUIRED.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_CONST_REQUIRED.example = const size: int = 4

CONST_E_CONST_OVERFLOW.summary = const depassement.
CONST_E_CONST_OVERFLOW.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_CONST_OVERFLOW.step1 = Reduce the constant expression at the reported span.
CONST_E_CONST_OVERFLOW.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_CONST_OVERFLOW.example = const size: int = 4

CONST_E_CONST_DIVISION_BY_ZERO.summary = const division by zero.
CONST_E_CONST_DIVISION_BY_ZERO.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_CONST_DIVISION_BY_ZERO.step1 = Reduce the constant expression at the reported span.
CONST_E_CONST_DIVISION_BY_ZERO.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_CONST_DIVISION_BY_ZERO.example = const size: int = 4

CONST_E_CONST_CYCLE.summary = const cycle.
CONST_E_CONST_CYCLE.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_CONST_CYCLE.step1 = Reduce the constant expression at the reported span.
CONST_E_CONST_CYCLE.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_CONST_CYCLE.example = const size: int = 4

CONST_E_MACRO_NOT_FOUND.summary = macro manquant.
CONST_E_MACRO_NOT_FOUND.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_MACRO_NOT_FOUND.step1 = Reduce the constant expression at the reported span.
CONST_E_MACRO_NOT_FOUND.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_MACRO_NOT_FOUND.example = const size: int = 4

CONST_E_MACRO_RECURSION.summary = macro recursion.
CONST_E_MACRO_RECURSION.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_MACRO_RECURSION.step1 = Reduce the constant expression at the reported span.
CONST_E_MACRO_RECURSION.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_MACRO_RECURSION.example = const size: int = 4

CONST_E_MACRO_EXPANSION_FAILED.summary = macro expansion echec.
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

CONST_E_GENERIC_ARGUMENT_MISSING.summary = generique argument manquant.
CONST_E_GENERIC_ARGUMENT_MISSING.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_GENERIC_ARGUMENT_MISSING.step1 = Reduce the constant expression at the reported span.
CONST_E_GENERIC_ARGUMENT_MISSING.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_GENERIC_ARGUMENT_MISSING.example = const size: int = 4

CONST_E_GENERIC_BOUND_FAILED.summary = generique bound echec.
CONST_E_GENERIC_BOUND_FAILED.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_GENERIC_BOUND_FAILED.step1 = Reduce the constant expression at the reported span.
CONST_E_GENERIC_BOUND_FAILED.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_GENERIC_BOUND_FAILED.example = const size: int = 4

CONST_E_UNSUPPORTED_TARGET.summary = cible non pris en charge.
CONST_E_UNSUPPORTED_TARGET.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_UNSUPPORTED_TARGET.step1 = Reduce the constant expression at the reported span.
CONST_E_UNSUPPORTED_TARGET.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_UNSUPPORTED_TARGET.example = const size: int = 4

CONST_E_ABI_MISMATCH.summary = abi incompatibilite.
CONST_E_ABI_MISMATCH.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_ABI_MISMATCH.step1 = Reduce the constant expression at the reported span.
CONST_E_ABI_MISMATCH.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_ABI_MISMATCH.example = const size: int = 4

CONST_E_LINK_FAILED.summary = link echec.
CONST_E_LINK_FAILED.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_LINK_FAILED.step1 = Reduce the constant expression at the reported span.
CONST_E_LINK_FAILED.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_LINK_FAILED.example = const size: int = 4

CONST_E_RUNTIME_PANIC.summary = runtime panique.
CONST_E_RUNTIME_PANIC.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_RUNTIME_PANIC.step1 = Reduce the constant expression at the reported span.
CONST_E_RUNTIME_PANIC.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_RUNTIME_PANIC.example = const size: int = 4

MACRO_E_EXPECTED_IDENTIFIER.summary = identifiant attendu.
MACRO_E_EXPECTED_IDENTIFIER.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_EXPECTED_IDENTIFIER.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_EXPECTED_IDENTIFIER.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_EXPECTED_IDENTIFIER.example = vitte check path/to/file.vit

MACRO_E_EXPECTED_EXPRESSION.summary = expression attendu.
MACRO_E_EXPECTED_EXPRESSION.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_EXPECTED_EXPRESSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_EXPECTED_EXPRESSION.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_EXPECTED_EXPRESSION.example = vitte check path/to/file.vit

MACRO_E_EXPECTED_TYPE.summary = type attendu.
MACRO_E_EXPECTED_TYPE.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_EXPECTED_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_EXPECTED_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_EXPECTED_TYPE.example = vitte check path/to/file.vit

MACRO_E_EXPECTED_PATTERN.summary = motif attendu.
MACRO_E_EXPECTED_PATTERN.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_EXPECTED_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_EXPECTED_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_EXPECTED_PATTERN.example = vitte check path/to/file.vit

MACRO_E_EXPECTED_BLOCK.summary = bloc attendu.
MACRO_E_EXPECTED_BLOCK.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_EXPECTED_BLOCK.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_EXPECTED_BLOCK.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_EXPECTED_BLOCK.example = vitte check path/to/file.vit

MACRO_E_EXPECTED_DELIMITER.summary = delimiteur attendu.
MACRO_E_EXPECTED_DELIMITER.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_EXPECTED_DELIMITER.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_EXPECTED_DELIMITER.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_EXPECTED_DELIMITER.example = vitte check path/to/file.vit

MACRO_E_UNEXPECTED_TOKEN.summary = jeton inattendu.
MACRO_E_UNEXPECTED_TOKEN.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_UNEXPECTED_TOKEN.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_UNEXPECTED_TOKEN.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_UNEXPECTED_TOKEN.example = vitte check path/to/file.vit

MACRO_E_UNBALANCED_DELIMITER.summary = unbalanced delimiteur.
MACRO_E_UNBALANCED_DELIMITER.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_UNBALANCED_DELIMITER.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_UNBALANCED_DELIMITER.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_UNBALANCED_DELIMITER.example = vitte check path/to/file.vit

MACRO_E_INVALID_ATTRIBUTE.summary = attribute invalide.
MACRO_E_INVALID_ATTRIBUTE.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_ATTRIBUTE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_ATTRIBUTE.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_ATTRIBUTE.example = vitte check path/to/file.vit

MACRO_E_INVALID_DECLARATION.summary = declaration invalide.
MACRO_E_INVALID_DECLARATION.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_DECLARATION.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_DECLARATION.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_DECLARATION.example = vitte check path/to/file.vit

MACRO_E_INVALID_STATEMENT.summary = instruction invalide.
MACRO_E_INVALID_STATEMENT.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_STATEMENT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_STATEMENT.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_STATEMENT.example = vitte check path/to/file.vit

MACRO_E_INVALID_EXPRESSION.summary = expression invalide.
MACRO_E_INVALID_EXPRESSION.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_EXPRESSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_EXPRESSION.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_EXPRESSION.example = vitte check path/to/file.vit

MACRO_E_INVALID_PATTERN.summary = motif invalide.
MACRO_E_INVALID_PATTERN.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_PATTERN.example = vitte check path/to/file.vit

MACRO_E_INVALID_LITERAL.summary = litteral invalide.
MACRO_E_INVALID_LITERAL.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_LITERAL.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_LITERAL.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_LITERAL.example = vitte check path/to/file.vit

MACRO_E_INVALID_OPERATOR.summary = operateur invalide.
MACRO_E_INVALID_OPERATOR.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_OPERATOR.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_OPERATOR.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_OPERATOR.example = vitte check path/to/file.vit

MACRO_E_INVALID_MODIFIER.summary = modifier invalide.
MACRO_E_INVALID_MODIFIER.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_MODIFIER.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_MODIFIER.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_MODIFIER.example = vitte check path/to/file.vit

MACRO_E_MISSING_BODY.summary = body manquant.
MACRO_E_MISSING_BODY.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_MISSING_BODY.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_MISSING_BODY.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_MISSING_BODY.example = vitte check path/to/file.vit

MACRO_E_MISSING_RETURN.summary = retour manquant.
MACRO_E_MISSING_RETURN.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_MISSING_RETURN.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_MISSING_RETURN.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_MISSING_RETURN.example = vitte check path/to/file.vit

MACRO_E_DUPLICATE_NAME.summary = nom duplique.
MACRO_E_DUPLICATE_NAME.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_DUPLICATE_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_DUPLICATE_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_DUPLICATE_NAME.example = vitte check path/to/file.vit

MACRO_E_UNKNOWN_NAME.summary = nom inconnu.
MACRO_E_UNKNOWN_NAME.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_UNKNOWN_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_UNKNOWN_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_UNKNOWN_NAME.example = vitte check path/to/file.vit

MACRO_E_UNKNOWN_TYPE.summary = type inconnu.
MACRO_E_UNKNOWN_TYPE.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_UNKNOWN_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_UNKNOWN_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_UNKNOWN_TYPE.example = vitte check path/to/file.vit

MACRO_E_UNKNOWN_MODULE.summary = module inconnu.
MACRO_E_UNKNOWN_MODULE.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_UNKNOWN_MODULE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_UNKNOWN_MODULE.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_UNKNOWN_MODULE.example = vitte check path/to/file.vit

MACRO_E_UNKNOWN_MEMBER.summary = membre inconnu.
MACRO_E_UNKNOWN_MEMBER.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_UNKNOWN_MEMBER.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_UNKNOWN_MEMBER.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_UNKNOWN_MEMBER.example = vitte check path/to/file.vit

MACRO_E_AMBIGUOUS_NAME.summary = ambiguous nom.
MACRO_E_AMBIGUOUS_NAME.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_AMBIGUOUS_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_AMBIGUOUS_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_AMBIGUOUS_NAME.example = vitte check path/to/file.vit

MACRO_E_PRIVATE_SYMBOL.summary = private symbole.
MACRO_E_PRIVATE_SYMBOL.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_PRIVATE_SYMBOL.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_PRIVATE_SYMBOL.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_PRIVATE_SYMBOL.example = vitte check path/to/file.vit

MACRO_E_IMPORT_NOT_FOUND.summary = import manquant.
MACRO_E_IMPORT_NOT_FOUND.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_IMPORT_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_IMPORT_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_IMPORT_NOT_FOUND.example = vitte check path/to/file.vit

MACRO_E_IMPORT_CYCLE.summary = import cycle.
MACRO_E_IMPORT_CYCLE.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_IMPORT_CYCLE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_IMPORT_CYCLE.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_IMPORT_CYCLE.example = vitte check path/to/file.vit

MACRO_E_EXPORT_CONFLICT.summary = export conflit.
MACRO_E_EXPORT_CONFLICT.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_EXPORT_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_EXPORT_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_EXPORT_CONFLICT.example = vitte check path/to/file.vit

MACRO_E_ARITY_MISMATCH.summary = arity incompatibilite.
MACRO_E_ARITY_MISMATCH.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_ARITY_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_ARITY_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_ARITY_MISMATCH.example = vitte check path/to/file.vit

MACRO_E_ARGUMENT_MISMATCH.summary = argument incompatibilite.
MACRO_E_ARGUMENT_MISMATCH.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_ARGUMENT_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_ARGUMENT_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_ARGUMENT_MISMATCH.example = vitte check path/to/file.vit

MACRO_E_ASSIGNMENT_MISMATCH.summary = affectation incompatibilite.
MACRO_E_ASSIGNMENT_MISMATCH.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_ASSIGNMENT_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_ASSIGNMENT_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_ASSIGNMENT_MISMATCH.example = vitte check path/to/file.vit

MACRO_E_BRANCH_MISMATCH.summary = branch incompatibilite.
MACRO_E_BRANCH_MISMATCH.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_BRANCH_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_BRANCH_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_BRANCH_MISMATCH.example = vitte check path/to/file.vit

MACRO_E_INVALID_CALL.summary = appel invalide.
MACRO_E_INVALID_CALL.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_CALL.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_CALL.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_CALL.example = vitte check path/to/file.vit

MACRO_E_INVALID_CAST.summary = conversion invalide.
MACRO_E_INVALID_CAST.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_CAST.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_CAST.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_CAST.example = vitte check path/to/file.vit

MACRO_E_INVALID_INDEX.summary = index invalide.
MACRO_E_INVALID_INDEX.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_INDEX.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_INDEX.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_INDEX.example = vitte check path/to/file.vit

MACRO_E_INVALID_DEREF.summary = deref invalide.
MACRO_E_INVALID_DEREF.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_DEREF.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_DEREF.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_DEREF.example = vitte check path/to/file.vit

MACRO_E_INVALID_BORROW.summary = emprunt invalide.
MACRO_E_INVALID_BORROW.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_BORROW.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_BORROW.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_BORROW.example = vitte check path/to/file.vit

MACRO_E_INVALID_MOVE.summary = deplacement invalide.
MACRO_E_INVALID_MOVE.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_MOVE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_MOVE.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_MOVE.example = vitte check path/to/file.vit

MACRO_E_USE_AFTER_MOVE.summary = use after deplacement.
MACRO_E_USE_AFTER_MOVE.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_USE_AFTER_MOVE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_USE_AFTER_MOVE.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_USE_AFTER_MOVE.example = vitte check path/to/file.vit

MACRO_E_USE_AFTER_DROP.summary = use after destruction.
MACRO_E_USE_AFTER_DROP.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_USE_AFTER_DROP.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_USE_AFTER_DROP.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_USE_AFTER_DROP.example = vitte check path/to/file.vit

MACRO_E_USE_BEFORE_INIT.summary = use before init.
MACRO_E_USE_BEFORE_INIT.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_USE_BEFORE_INIT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_USE_BEFORE_INIT.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_USE_BEFORE_INIT.example = vitte check path/to/file.vit

MACRO_E_DOUBLE_DROP.summary = double destruction.
MACRO_E_DOUBLE_DROP.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_DOUBLE_DROP.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_DOUBLE_DROP.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_DOUBLE_DROP.example = vitte check path/to/file.vit

MACRO_E_BORROW_CONFLICT.summary = emprunt conflit.
MACRO_E_BORROW_CONFLICT.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_BORROW_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_BORROW_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_BORROW_CONFLICT.example = vitte check path/to/file.vit

MACRO_E_MUTABILITY_CONFLICT.summary = mutability conflit.
MACRO_E_MUTABILITY_CONFLICT.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_MUTABILITY_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_MUTABILITY_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_MUTABILITY_CONFLICT.example = vitte check path/to/file.vit

MACRO_E_LIFETIME_TOO_SHORT.summary = duree de vie trop short.
MACRO_E_LIFETIME_TOO_SHORT.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_LIFETIME_TOO_SHORT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_LIFETIME_TOO_SHORT.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_LIFETIME_TOO_SHORT.example = vitte check path/to/file.vit

MACRO_E_DANGLING_REFERENCE.summary = dangling reference.
MACRO_E_DANGLING_REFERENCE.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_DANGLING_REFERENCE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_DANGLING_REFERENCE.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_DANGLING_REFERENCE.example = vitte check path/to/file.vit

MACRO_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
MACRO_E_NON_EXHAUSTIVE_MATCH.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_NON_EXHAUSTIVE_MATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_NON_EXHAUSTIVE_MATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_NON_EXHAUSTIVE_MATCH.example = vitte check path/to/file.vit

MACRO_E_UNREACHABLE_PATTERN.summary = inatteignable motif.
MACRO_E_UNREACHABLE_PATTERN.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_UNREACHABLE_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_UNREACHABLE_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_UNREACHABLE_PATTERN.example = vitte check path/to/file.vit

MACRO_E_CONST_REQUIRED.summary = const required.
MACRO_E_CONST_REQUIRED.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_CONST_REQUIRED.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_CONST_REQUIRED.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_CONST_REQUIRED.example = vitte check path/to/file.vit

MACRO_E_CONST_OVERFLOW.summary = const depassement.
MACRO_E_CONST_OVERFLOW.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_CONST_OVERFLOW.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_CONST_OVERFLOW.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_CONST_OVERFLOW.example = vitte check path/to/file.vit

MACRO_E_CONST_DIVISION_BY_ZERO.summary = const division by zero.
MACRO_E_CONST_DIVISION_BY_ZERO.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_CONST_DIVISION_BY_ZERO.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_CONST_DIVISION_BY_ZERO.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_CONST_DIVISION_BY_ZERO.example = vitte check path/to/file.vit

MACRO_E_CONST_CYCLE.summary = const cycle.
MACRO_E_CONST_CYCLE.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_CONST_CYCLE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_CONST_CYCLE.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_CONST_CYCLE.example = vitte check path/to/file.vit

MACRO_E_MACRO_NOT_FOUND.summary = macro manquant.
MACRO_E_MACRO_NOT_FOUND.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_MACRO_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_MACRO_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_MACRO_NOT_FOUND.example = vitte check path/to/file.vit

MACRO_E_MACRO_RECURSION.summary = macro recursion.
MACRO_E_MACRO_RECURSION.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_MACRO_RECURSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_MACRO_RECURSION.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_MACRO_RECURSION.example = vitte check path/to/file.vit

MACRO_E_MACRO_EXPANSION_FAILED.summary = macro expansion echec.
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

MACRO_E_GENERIC_ARGUMENT_MISSING.summary = generique argument manquant.
MACRO_E_GENERIC_ARGUMENT_MISSING.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_GENERIC_ARGUMENT_MISSING.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_GENERIC_ARGUMENT_MISSING.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_GENERIC_ARGUMENT_MISSING.example = vitte check path/to/file.vit

MACRO_E_GENERIC_BOUND_FAILED.summary = generique bound echec.
MACRO_E_GENERIC_BOUND_FAILED.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_GENERIC_BOUND_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_GENERIC_BOUND_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_GENERIC_BOUND_FAILED.example = vitte check path/to/file.vit

MACRO_E_UNSUPPORTED_TARGET.summary = cible non pris en charge.
MACRO_E_UNSUPPORTED_TARGET.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_UNSUPPORTED_TARGET.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_UNSUPPORTED_TARGET.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_UNSUPPORTED_TARGET.example = vitte check path/to/file.vit

MACRO_E_ABI_MISMATCH.summary = abi incompatibilite.
MACRO_E_ABI_MISMATCH.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_ABI_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_ABI_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_ABI_MISMATCH.example = vitte check path/to/file.vit

MACRO_E_LINK_FAILED.summary = link echec.
MACRO_E_LINK_FAILED.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_LINK_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_LINK_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_LINK_FAILED.example = vitte check path/to/file.vit

MACRO_E_RUNTIME_PANIC.summary = runtime panique.
MACRO_E_RUNTIME_PANIC.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_RUNTIME_PANIC.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_RUNTIME_PANIC.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_RUNTIME_PANIC.example = vitte check path/to/file.vit

HIR_E_EXPECTED_IDENTIFIER.summary = identifiant attendu.
HIR_E_EXPECTED_IDENTIFIER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_EXPECTED_IDENTIFIER.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_EXPECTED_IDENTIFIER.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_EXPECTED_IDENTIFIER.example = vitte check path/to/file.vit

HIR_E_EXPECTED_EXPRESSION.summary = expression attendu.
HIR_E_EXPECTED_EXPRESSION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_EXPECTED_EXPRESSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_EXPECTED_EXPRESSION.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_EXPECTED_EXPRESSION.example = vitte check path/to/file.vit

HIR_E_EXPECTED_TYPE.summary = type attendu.
HIR_E_EXPECTED_TYPE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_EXPECTED_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_EXPECTED_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_EXPECTED_TYPE.example = vitte check path/to/file.vit

HIR_E_EXPECTED_PATTERN.summary = motif attendu.
HIR_E_EXPECTED_PATTERN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_EXPECTED_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_EXPECTED_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_EXPECTED_PATTERN.example = vitte check path/to/file.vit

HIR_E_EXPECTED_BLOCK.summary = bloc attendu.
HIR_E_EXPECTED_BLOCK.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_EXPECTED_BLOCK.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_EXPECTED_BLOCK.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_EXPECTED_BLOCK.example = vitte check path/to/file.vit

HIR_E_EXPECTED_DELIMITER.summary = delimiteur attendu.
HIR_E_EXPECTED_DELIMITER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_EXPECTED_DELIMITER.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_EXPECTED_DELIMITER.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_EXPECTED_DELIMITER.example = vitte check path/to/file.vit

HIR_E_UNEXPECTED_TOKEN.summary = jeton inattendu.
HIR_E_UNEXPECTED_TOKEN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_UNEXPECTED_TOKEN.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_UNEXPECTED_TOKEN.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_UNEXPECTED_TOKEN.example = vitte check path/to/file.vit

HIR_E_UNBALANCED_DELIMITER.summary = unbalanced delimiteur.
HIR_E_UNBALANCED_DELIMITER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_UNBALANCED_DELIMITER.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_UNBALANCED_DELIMITER.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_UNBALANCED_DELIMITER.example = vitte check path/to/file.vit

HIR_E_INVALID_ATTRIBUTE.summary = attribute invalide.
HIR_E_INVALID_ATTRIBUTE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_ATTRIBUTE.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_ATTRIBUTE.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_ATTRIBUTE.example = vitte check path/to/file.vit

HIR_E_INVALID_DECLARATION.summary = declaration invalide.
HIR_E_INVALID_DECLARATION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_DECLARATION.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_DECLARATION.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_DECLARATION.example = vitte check path/to/file.vit

HIR_E_INVALID_STATEMENT.summary = instruction invalide.
HIR_E_INVALID_STATEMENT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_STATEMENT.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_STATEMENT.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_STATEMENT.example = vitte check path/to/file.vit

HIR_E_INVALID_EXPRESSION.summary = expression invalide.
HIR_E_INVALID_EXPRESSION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_EXPRESSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_EXPRESSION.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_EXPRESSION.example = vitte check path/to/file.vit

HIR_E_INVALID_LITERAL.summary = litteral invalide.
HIR_E_INVALID_LITERAL.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_LITERAL.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_LITERAL.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_LITERAL.example = vitte check path/to/file.vit

HIR_E_INVALID_OPERATOR.summary = operateur invalide.
HIR_E_INVALID_OPERATOR.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_OPERATOR.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_OPERATOR.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_OPERATOR.example = vitte check path/to/file.vit

HIR_E_INVALID_MODIFIER.summary = modifier invalide.
HIR_E_INVALID_MODIFIER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_MODIFIER.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_MODIFIER.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_MODIFIER.example = vitte check path/to/file.vit

HIR_E_MISSING_BODY.summary = body manquant.
HIR_E_MISSING_BODY.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_MISSING_BODY.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_MISSING_BODY.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_MISSING_BODY.example = vitte check path/to/file.vit

HIR_E_MISSING_RETURN.summary = retour manquant.
HIR_E_MISSING_RETURN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_MISSING_RETURN.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_MISSING_RETURN.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_MISSING_RETURN.example = vitte check path/to/file.vit

HIR_E_DUPLICATE_NAME.summary = nom duplique.
HIR_E_DUPLICATE_NAME.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_DUPLICATE_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_DUPLICATE_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_DUPLICATE_NAME.example = vitte check path/to/file.vit

HIR_E_UNKNOWN_NAME.summary = nom inconnu.
HIR_E_UNKNOWN_NAME.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_UNKNOWN_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_UNKNOWN_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_UNKNOWN_NAME.example = vitte check path/to/file.vit

HIR_E_UNKNOWN_TYPE.summary = type inconnu.
HIR_E_UNKNOWN_TYPE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_UNKNOWN_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_UNKNOWN_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_UNKNOWN_TYPE.example = vitte check path/to/file.vit

HIR_E_UNKNOWN_MODULE.summary = module inconnu.
HIR_E_UNKNOWN_MODULE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_UNKNOWN_MODULE.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_UNKNOWN_MODULE.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_UNKNOWN_MODULE.example = vitte check path/to/file.vit

HIR_E_UNKNOWN_MEMBER.summary = membre inconnu.
HIR_E_UNKNOWN_MEMBER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_UNKNOWN_MEMBER.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_UNKNOWN_MEMBER.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_UNKNOWN_MEMBER.example = vitte check path/to/file.vit

HIR_E_AMBIGUOUS_NAME.summary = ambiguous nom.
HIR_E_AMBIGUOUS_NAME.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_AMBIGUOUS_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_AMBIGUOUS_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_AMBIGUOUS_NAME.example = vitte check path/to/file.vit

HIR_E_PRIVATE_SYMBOL.summary = private symbole.
HIR_E_PRIVATE_SYMBOL.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_PRIVATE_SYMBOL.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_PRIVATE_SYMBOL.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_PRIVATE_SYMBOL.example = vitte check path/to/file.vit

HIR_E_IMPORT_NOT_FOUND.summary = import manquant.
HIR_E_IMPORT_NOT_FOUND.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_IMPORT_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_IMPORT_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_IMPORT_NOT_FOUND.example = vitte check path/to/file.vit

HIR_E_IMPORT_CYCLE.summary = import cycle.
HIR_E_IMPORT_CYCLE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_IMPORT_CYCLE.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_IMPORT_CYCLE.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_IMPORT_CYCLE.example = vitte check path/to/file.vit

HIR_E_EXPORT_CONFLICT.summary = export conflit.
HIR_E_EXPORT_CONFLICT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_EXPORT_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_EXPORT_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_EXPORT_CONFLICT.example = vitte check path/to/file.vit

HIR_E_ARITY_MISMATCH.summary = arity incompatibilite.
HIR_E_ARITY_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_ARITY_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_ARITY_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_ARITY_MISMATCH.example = vitte check path/to/file.vit

HIR_E_ARGUMENT_MISMATCH.summary = argument incompatibilite.
HIR_E_ARGUMENT_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_ARGUMENT_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_ARGUMENT_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_ARGUMENT_MISMATCH.example = vitte check path/to/file.vit

HIR_E_ASSIGNMENT_MISMATCH.summary = affectation incompatibilite.
HIR_E_ASSIGNMENT_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_ASSIGNMENT_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_ASSIGNMENT_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_ASSIGNMENT_MISMATCH.example = vitte check path/to/file.vit

HIR_E_BRANCH_MISMATCH.summary = branch incompatibilite.
HIR_E_BRANCH_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_BRANCH_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_BRANCH_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_BRANCH_MISMATCH.example = vitte check path/to/file.vit

HIR_E_INVALID_CALL.summary = appel invalide.
HIR_E_INVALID_CALL.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_CALL.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_CALL.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_CALL.example = vitte check path/to/file.vit

HIR_E_INVALID_CAST.summary = conversion invalide.
HIR_E_INVALID_CAST.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_CAST.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_CAST.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_CAST.example = vitte check path/to/file.vit

HIR_E_INVALID_INDEX.summary = index invalide.
HIR_E_INVALID_INDEX.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_INDEX.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_INDEX.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_INDEX.example = vitte check path/to/file.vit

HIR_E_INVALID_DEREF.summary = deref invalide.
HIR_E_INVALID_DEREF.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_DEREF.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_DEREF.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_DEREF.example = vitte check path/to/file.vit

HIR_E_INVALID_BORROW.summary = emprunt invalide.
HIR_E_INVALID_BORROW.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_BORROW.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_BORROW.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_BORROW.example = vitte check path/to/file.vit

HIR_E_INVALID_MOVE.summary = deplacement invalide.
HIR_E_INVALID_MOVE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_MOVE.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_MOVE.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_MOVE.example = vitte check path/to/file.vit

HIR_E_USE_AFTER_MOVE.summary = use after deplacement.
HIR_E_USE_AFTER_MOVE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_USE_AFTER_MOVE.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_USE_AFTER_MOVE.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_USE_AFTER_MOVE.example = vitte check path/to/file.vit

HIR_E_USE_AFTER_DROP.summary = use after destruction.
HIR_E_USE_AFTER_DROP.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_USE_AFTER_DROP.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_USE_AFTER_DROP.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_USE_AFTER_DROP.example = vitte check path/to/file.vit

HIR_E_USE_BEFORE_INIT.summary = use before init.
HIR_E_USE_BEFORE_INIT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_USE_BEFORE_INIT.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_USE_BEFORE_INIT.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_USE_BEFORE_INIT.example = vitte check path/to/file.vit

HIR_E_DOUBLE_DROP.summary = double destruction.
HIR_E_DOUBLE_DROP.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_DOUBLE_DROP.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_DOUBLE_DROP.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_DOUBLE_DROP.example = vitte check path/to/file.vit

HIR_E_BORROW_CONFLICT.summary = emprunt conflit.
HIR_E_BORROW_CONFLICT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_BORROW_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_BORROW_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_BORROW_CONFLICT.example = vitte check path/to/file.vit

HIR_E_MUTABILITY_CONFLICT.summary = mutability conflit.
HIR_E_MUTABILITY_CONFLICT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_MUTABILITY_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_MUTABILITY_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_MUTABILITY_CONFLICT.example = vitte check path/to/file.vit

HIR_E_LIFETIME_TOO_SHORT.summary = duree de vie trop short.
HIR_E_LIFETIME_TOO_SHORT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_LIFETIME_TOO_SHORT.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_LIFETIME_TOO_SHORT.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_LIFETIME_TOO_SHORT.example = vitte check path/to/file.vit

HIR_E_DANGLING_REFERENCE.summary = dangling reference.
HIR_E_DANGLING_REFERENCE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_DANGLING_REFERENCE.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_DANGLING_REFERENCE.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_DANGLING_REFERENCE.example = vitte check path/to/file.vit

HIR_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
HIR_E_NON_EXHAUSTIVE_MATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_NON_EXHAUSTIVE_MATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_NON_EXHAUSTIVE_MATCH.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_NON_EXHAUSTIVE_MATCH.example = vitte check path/to/file.vit

HIR_E_UNREACHABLE_PATTERN.summary = inatteignable motif.
HIR_E_UNREACHABLE_PATTERN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_UNREACHABLE_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_UNREACHABLE_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_UNREACHABLE_PATTERN.example = vitte check path/to/file.vit

HIR_E_CONST_REQUIRED.summary = const required.
HIR_E_CONST_REQUIRED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_CONST_REQUIRED.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_CONST_REQUIRED.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_CONST_REQUIRED.example = vitte check path/to/file.vit

HIR_E_CONST_OVERFLOW.summary = const depassement.
HIR_E_CONST_OVERFLOW.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_CONST_OVERFLOW.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_CONST_OVERFLOW.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_CONST_OVERFLOW.example = vitte check path/to/file.vit

HIR_E_CONST_DIVISION_BY_ZERO.summary = const division by zero.
HIR_E_CONST_DIVISION_BY_ZERO.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_CONST_DIVISION_BY_ZERO.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_CONST_DIVISION_BY_ZERO.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_CONST_DIVISION_BY_ZERO.example = vitte check path/to/file.vit

HIR_E_CONST_CYCLE.summary = const cycle.
HIR_E_CONST_CYCLE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_CONST_CYCLE.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_CONST_CYCLE.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_CONST_CYCLE.example = vitte check path/to/file.vit

HIR_E_MACRO_NOT_FOUND.summary = macro manquant.
HIR_E_MACRO_NOT_FOUND.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_MACRO_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_MACRO_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_MACRO_NOT_FOUND.example = vitte check path/to/file.vit

HIR_E_MACRO_RECURSION.summary = macro recursion.
HIR_E_MACRO_RECURSION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_MACRO_RECURSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_MACRO_RECURSION.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_MACRO_RECURSION.example = vitte check path/to/file.vit

HIR_E_MACRO_EXPANSION_FAILED.summary = macro expansion echec.
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

HIR_E_GENERIC_ARGUMENT_MISSING.summary = generique argument manquant.
HIR_E_GENERIC_ARGUMENT_MISSING.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_GENERIC_ARGUMENT_MISSING.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_GENERIC_ARGUMENT_MISSING.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_GENERIC_ARGUMENT_MISSING.example = vitte check path/to/file.vit

HIR_E_GENERIC_BOUND_FAILED.summary = generique bound echec.
HIR_E_GENERIC_BOUND_FAILED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_GENERIC_BOUND_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_GENERIC_BOUND_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_GENERIC_BOUND_FAILED.example = vitte check path/to/file.vit

HIR_E_UNSUPPORTED_TARGET.summary = cible non pris en charge.
HIR_E_UNSUPPORTED_TARGET.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_UNSUPPORTED_TARGET.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_UNSUPPORTED_TARGET.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_UNSUPPORTED_TARGET.example = vitte check path/to/file.vit

HIR_E_ABI_MISMATCH.summary = abi incompatibilite.
HIR_E_ABI_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_ABI_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_ABI_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_ABI_MISMATCH.example = vitte check path/to/file.vit

HIR_E_LINK_FAILED.summary = link echec.
HIR_E_LINK_FAILED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_LINK_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_LINK_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_LINK_FAILED.example = vitte check path/to/file.vit

HIR_E_RUNTIME_PANIC.summary = runtime panique.
HIR_E_RUNTIME_PANIC.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_RUNTIME_PANIC.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_RUNTIME_PANIC.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_RUNTIME_PANIC.example = vitte check path/to/file.vit

MIR_E_EXPECTED_IDENTIFIER.summary = identifiant attendu.
MIR_E_EXPECTED_IDENTIFIER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_EXPECTED_IDENTIFIER.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_EXPECTED_IDENTIFIER.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_EXPECTED_IDENTIFIER.example = vitte check path/to/file.vit

MIR_E_EXPECTED_EXPRESSION.summary = expression attendu.
MIR_E_EXPECTED_EXPRESSION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_EXPECTED_EXPRESSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_EXPECTED_EXPRESSION.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_EXPECTED_EXPRESSION.example = vitte check path/to/file.vit

MIR_E_EXPECTED_TYPE.summary = type attendu.
MIR_E_EXPECTED_TYPE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_EXPECTED_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_EXPECTED_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_EXPECTED_TYPE.example = vitte check path/to/file.vit

MIR_E_EXPECTED_PATTERN.summary = motif attendu.
MIR_E_EXPECTED_PATTERN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_EXPECTED_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_EXPECTED_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_EXPECTED_PATTERN.example = vitte check path/to/file.vit

MIR_E_EXPECTED_BLOCK.summary = bloc attendu.
MIR_E_EXPECTED_BLOCK.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_EXPECTED_BLOCK.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_EXPECTED_BLOCK.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_EXPECTED_BLOCK.example = vitte check path/to/file.vit

MIR_E_EXPECTED_DELIMITER.summary = delimiteur attendu.
MIR_E_EXPECTED_DELIMITER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_EXPECTED_DELIMITER.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_EXPECTED_DELIMITER.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_EXPECTED_DELIMITER.example = vitte check path/to/file.vit

MIR_E_UNEXPECTED_TOKEN.summary = jeton inattendu.
MIR_E_UNEXPECTED_TOKEN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_UNEXPECTED_TOKEN.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_UNEXPECTED_TOKEN.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_UNEXPECTED_TOKEN.example = vitte check path/to/file.vit

MIR_E_UNBALANCED_DELIMITER.summary = unbalanced delimiteur.
MIR_E_UNBALANCED_DELIMITER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_UNBALANCED_DELIMITER.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_UNBALANCED_DELIMITER.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_UNBALANCED_DELIMITER.example = vitte check path/to/file.vit

MIR_E_INVALID_ATTRIBUTE.summary = attribute invalide.
MIR_E_INVALID_ATTRIBUTE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_ATTRIBUTE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_ATTRIBUTE.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_ATTRIBUTE.example = vitte check path/to/file.vit

MIR_E_INVALID_DECLARATION.summary = declaration invalide.
MIR_E_INVALID_DECLARATION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_DECLARATION.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_DECLARATION.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_DECLARATION.example = vitte check path/to/file.vit

MIR_E_INVALID_STATEMENT.summary = instruction invalide.
MIR_E_INVALID_STATEMENT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_STATEMENT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_STATEMENT.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_STATEMENT.example = vitte check path/to/file.vit

MIR_E_INVALID_EXPRESSION.summary = expression invalide.
MIR_E_INVALID_EXPRESSION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_EXPRESSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_EXPRESSION.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_EXPRESSION.example = vitte check path/to/file.vit

MIR_E_INVALID_PATTERN.summary = motif invalide.
MIR_E_INVALID_PATTERN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_PATTERN.example = vitte check path/to/file.vit

MIR_E_INVALID_LITERAL.summary = litteral invalide.
MIR_E_INVALID_LITERAL.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_LITERAL.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_LITERAL.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_LITERAL.example = vitte check path/to/file.vit

MIR_E_INVALID_OPERATOR.summary = operateur invalide.
MIR_E_INVALID_OPERATOR.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_OPERATOR.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_OPERATOR.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_OPERATOR.example = vitte check path/to/file.vit

MIR_E_INVALID_MODIFIER.summary = modifier invalide.
MIR_E_INVALID_MODIFIER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_MODIFIER.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_MODIFIER.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_MODIFIER.example = vitte check path/to/file.vit

MIR_E_MISSING_BODY.summary = body manquant.
MIR_E_MISSING_BODY.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_MISSING_BODY.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_MISSING_BODY.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_MISSING_BODY.example = vitte check path/to/file.vit

MIR_E_MISSING_RETURN.summary = retour manquant.
MIR_E_MISSING_RETURN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_MISSING_RETURN.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_MISSING_RETURN.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_MISSING_RETURN.example = vitte check path/to/file.vit

MIR_E_DUPLICATE_NAME.summary = nom duplique.
MIR_E_DUPLICATE_NAME.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_DUPLICATE_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_DUPLICATE_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_DUPLICATE_NAME.example = vitte check path/to/file.vit

MIR_E_UNKNOWN_NAME.summary = nom inconnu.
MIR_E_UNKNOWN_NAME.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_UNKNOWN_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_UNKNOWN_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_UNKNOWN_NAME.example = vitte check path/to/file.vit

MIR_E_UNKNOWN_TYPE.summary = type inconnu.
MIR_E_UNKNOWN_TYPE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_UNKNOWN_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_UNKNOWN_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_UNKNOWN_TYPE.example = vitte check path/to/file.vit

MIR_E_UNKNOWN_MODULE.summary = module inconnu.
MIR_E_UNKNOWN_MODULE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_UNKNOWN_MODULE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_UNKNOWN_MODULE.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_UNKNOWN_MODULE.example = vitte check path/to/file.vit

MIR_E_UNKNOWN_MEMBER.summary = membre inconnu.
MIR_E_UNKNOWN_MEMBER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_UNKNOWN_MEMBER.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_UNKNOWN_MEMBER.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_UNKNOWN_MEMBER.example = vitte check path/to/file.vit

MIR_E_AMBIGUOUS_NAME.summary = ambiguous nom.
MIR_E_AMBIGUOUS_NAME.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_AMBIGUOUS_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_AMBIGUOUS_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_AMBIGUOUS_NAME.example = vitte check path/to/file.vit

MIR_E_PRIVATE_SYMBOL.summary = private symbole.
MIR_E_PRIVATE_SYMBOL.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_PRIVATE_SYMBOL.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_PRIVATE_SYMBOL.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_PRIVATE_SYMBOL.example = vitte check path/to/file.vit

MIR_E_IMPORT_NOT_FOUND.summary = import manquant.
MIR_E_IMPORT_NOT_FOUND.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_IMPORT_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_IMPORT_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_IMPORT_NOT_FOUND.example = vitte check path/to/file.vit

MIR_E_IMPORT_CYCLE.summary = import cycle.
MIR_E_IMPORT_CYCLE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_IMPORT_CYCLE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_IMPORT_CYCLE.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_IMPORT_CYCLE.example = vitte check path/to/file.vit

MIR_E_EXPORT_CONFLICT.summary = export conflit.
MIR_E_EXPORT_CONFLICT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_EXPORT_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_EXPORT_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_EXPORT_CONFLICT.example = vitte check path/to/file.vit

MIR_E_ARITY_MISMATCH.summary = arity incompatibilite.
MIR_E_ARITY_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_ARITY_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_ARITY_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_ARITY_MISMATCH.example = vitte check path/to/file.vit

MIR_E_ARGUMENT_MISMATCH.summary = argument incompatibilite.
MIR_E_ARGUMENT_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_ARGUMENT_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_ARGUMENT_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_ARGUMENT_MISMATCH.example = vitte check path/to/file.vit

MIR_E_ASSIGNMENT_MISMATCH.summary = affectation incompatibilite.
MIR_E_ASSIGNMENT_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_ASSIGNMENT_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_ASSIGNMENT_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_ASSIGNMENT_MISMATCH.example = vitte check path/to/file.vit

MIR_E_BRANCH_MISMATCH.summary = branch incompatibilite.
MIR_E_BRANCH_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_BRANCH_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_BRANCH_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_BRANCH_MISMATCH.example = vitte check path/to/file.vit

MIR_E_INVALID_CALL.summary = appel invalide.
MIR_E_INVALID_CALL.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_CALL.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_CALL.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_CALL.example = vitte check path/to/file.vit

MIR_E_INVALID_CAST.summary = conversion invalide.
MIR_E_INVALID_CAST.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_CAST.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_CAST.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_CAST.example = vitte check path/to/file.vit

MIR_E_INVALID_INDEX.summary = index invalide.
MIR_E_INVALID_INDEX.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_INDEX.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_INDEX.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_INDEX.example = vitte check path/to/file.vit

MIR_E_INVALID_DEREF.summary = deref invalide.
MIR_E_INVALID_DEREF.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_DEREF.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_DEREF.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_DEREF.example = vitte check path/to/file.vit

MIR_E_INVALID_BORROW.summary = emprunt invalide.
MIR_E_INVALID_BORROW.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_BORROW.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_BORROW.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_BORROW.example = vitte check path/to/file.vit

MIR_E_INVALID_MOVE.summary = deplacement invalide.
MIR_E_INVALID_MOVE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_MOVE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_MOVE.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_MOVE.example = vitte check path/to/file.vit

MIR_E_USE_AFTER_MOVE.summary = use after deplacement.
MIR_E_USE_AFTER_MOVE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_USE_AFTER_MOVE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_USE_AFTER_MOVE.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_USE_AFTER_MOVE.example = vitte check path/to/file.vit

MIR_E_USE_AFTER_DROP.summary = use after destruction.
MIR_E_USE_AFTER_DROP.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_USE_AFTER_DROP.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_USE_AFTER_DROP.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_USE_AFTER_DROP.example = vitte check path/to/file.vit

MIR_E_USE_BEFORE_INIT.summary = use before init.
MIR_E_USE_BEFORE_INIT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_USE_BEFORE_INIT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_USE_BEFORE_INIT.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_USE_BEFORE_INIT.example = vitte check path/to/file.vit

MIR_E_DOUBLE_DROP.summary = double destruction.
MIR_E_DOUBLE_DROP.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_DOUBLE_DROP.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_DOUBLE_DROP.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_DOUBLE_DROP.example = vitte check path/to/file.vit

MIR_E_BORROW_CONFLICT.summary = emprunt conflit.
MIR_E_BORROW_CONFLICT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_BORROW_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_BORROW_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_BORROW_CONFLICT.example = vitte check path/to/file.vit

MIR_E_MUTABILITY_CONFLICT.summary = mutability conflit.
MIR_E_MUTABILITY_CONFLICT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_MUTABILITY_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_MUTABILITY_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_MUTABILITY_CONFLICT.example = vitte check path/to/file.vit

MIR_E_LIFETIME_TOO_SHORT.summary = duree de vie trop short.
MIR_E_LIFETIME_TOO_SHORT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_LIFETIME_TOO_SHORT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_LIFETIME_TOO_SHORT.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_LIFETIME_TOO_SHORT.example = vitte check path/to/file.vit

MIR_E_DANGLING_REFERENCE.summary = dangling reference.
MIR_E_DANGLING_REFERENCE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_DANGLING_REFERENCE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_DANGLING_REFERENCE.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_DANGLING_REFERENCE.example = vitte check path/to/file.vit

MIR_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
MIR_E_NON_EXHAUSTIVE_MATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_NON_EXHAUSTIVE_MATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_NON_EXHAUSTIVE_MATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_NON_EXHAUSTIVE_MATCH.example = vitte check path/to/file.vit

MIR_E_UNREACHABLE_PATTERN.summary = inatteignable motif.
MIR_E_UNREACHABLE_PATTERN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_UNREACHABLE_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_UNREACHABLE_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_UNREACHABLE_PATTERN.example = vitte check path/to/file.vit

MIR_E_CONST_REQUIRED.summary = const required.
MIR_E_CONST_REQUIRED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_CONST_REQUIRED.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_CONST_REQUIRED.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_CONST_REQUIRED.example = vitte check path/to/file.vit

MIR_E_CONST_OVERFLOW.summary = const depassement.
MIR_E_CONST_OVERFLOW.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_CONST_OVERFLOW.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_CONST_OVERFLOW.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_CONST_OVERFLOW.example = vitte check path/to/file.vit

MIR_E_CONST_DIVISION_BY_ZERO.summary = const division by zero.
MIR_E_CONST_DIVISION_BY_ZERO.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_CONST_DIVISION_BY_ZERO.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_CONST_DIVISION_BY_ZERO.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_CONST_DIVISION_BY_ZERO.example = vitte check path/to/file.vit

MIR_E_CONST_CYCLE.summary = const cycle.
MIR_E_CONST_CYCLE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_CONST_CYCLE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_CONST_CYCLE.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_CONST_CYCLE.example = vitte check path/to/file.vit

MIR_E_MACRO_NOT_FOUND.summary = macro manquant.
MIR_E_MACRO_NOT_FOUND.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_MACRO_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_MACRO_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_MACRO_NOT_FOUND.example = vitte check path/to/file.vit

MIR_E_MACRO_RECURSION.summary = macro recursion.
MIR_E_MACRO_RECURSION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_MACRO_RECURSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_MACRO_RECURSION.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_MACRO_RECURSION.example = vitte check path/to/file.vit

MIR_E_MACRO_EXPANSION_FAILED.summary = macro expansion echec.
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

MIR_E_GENERIC_ARGUMENT_MISSING.summary = generique argument manquant.
MIR_E_GENERIC_ARGUMENT_MISSING.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_GENERIC_ARGUMENT_MISSING.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_GENERIC_ARGUMENT_MISSING.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_GENERIC_ARGUMENT_MISSING.example = vitte check path/to/file.vit

MIR_E_GENERIC_BOUND_FAILED.summary = generique bound echec.
MIR_E_GENERIC_BOUND_FAILED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_GENERIC_BOUND_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_GENERIC_BOUND_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_GENERIC_BOUND_FAILED.example = vitte check path/to/file.vit

MIR_E_UNSUPPORTED_TARGET.summary = cible non pris en charge.
MIR_E_UNSUPPORTED_TARGET.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_UNSUPPORTED_TARGET.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_UNSUPPORTED_TARGET.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_UNSUPPORTED_TARGET.example = vitte check path/to/file.vit

MIR_E_ABI_MISMATCH.summary = abi incompatibilite.
MIR_E_ABI_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_ABI_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_ABI_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_ABI_MISMATCH.example = vitte check path/to/file.vit

MIR_E_LINK_FAILED.summary = link echec.
MIR_E_LINK_FAILED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_LINK_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_LINK_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_LINK_FAILED.example = vitte check path/to/file.vit

MIR_E_RUNTIME_PANIC.summary = runtime panique.
MIR_E_RUNTIME_PANIC.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_RUNTIME_PANIC.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_RUNTIME_PANIC.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_RUNTIME_PANIC.example = vitte check path/to/file.vit

IR_E_EXPECTED_IDENTIFIER.summary = identifiant attendu.
IR_E_EXPECTED_IDENTIFIER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_EXPECTED_IDENTIFIER.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_EXPECTED_IDENTIFIER.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_EXPECTED_IDENTIFIER.example = vitte check path/to/file.vit

IR_E_EXPECTED_EXPRESSION.summary = expression attendu.
IR_E_EXPECTED_EXPRESSION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_EXPECTED_EXPRESSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_EXPECTED_EXPRESSION.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_EXPECTED_EXPRESSION.example = vitte check path/to/file.vit

IR_E_EXPECTED_TYPE.summary = type attendu.
IR_E_EXPECTED_TYPE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_EXPECTED_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_EXPECTED_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_EXPECTED_TYPE.example = vitte check path/to/file.vit

IR_E_EXPECTED_PATTERN.summary = motif attendu.
IR_E_EXPECTED_PATTERN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_EXPECTED_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_EXPECTED_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_EXPECTED_PATTERN.example = vitte check path/to/file.vit

IR_E_EXPECTED_BLOCK.summary = bloc attendu.
IR_E_EXPECTED_BLOCK.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_EXPECTED_BLOCK.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_EXPECTED_BLOCK.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_EXPECTED_BLOCK.example = vitte check path/to/file.vit

IR_E_EXPECTED_DELIMITER.summary = delimiteur attendu.
IR_E_EXPECTED_DELIMITER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_EXPECTED_DELIMITER.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_EXPECTED_DELIMITER.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_EXPECTED_DELIMITER.example = vitte check path/to/file.vit

IR_E_UNEXPECTED_TOKEN.summary = jeton inattendu.
IR_E_UNEXPECTED_TOKEN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_UNEXPECTED_TOKEN.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_UNEXPECTED_TOKEN.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_UNEXPECTED_TOKEN.example = vitte check path/to/file.vit

IR_E_UNBALANCED_DELIMITER.summary = unbalanced delimiteur.
IR_E_UNBALANCED_DELIMITER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_UNBALANCED_DELIMITER.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_UNBALANCED_DELIMITER.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_UNBALANCED_DELIMITER.example = vitte check path/to/file.vit

IR_E_INVALID_ATTRIBUTE.summary = attribute invalide.
IR_E_INVALID_ATTRIBUTE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_ATTRIBUTE.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_ATTRIBUTE.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_ATTRIBUTE.example = vitte check path/to/file.vit

IR_E_INVALID_DECLARATION.summary = declaration invalide.
IR_E_INVALID_DECLARATION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_DECLARATION.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_DECLARATION.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_DECLARATION.example = vitte check path/to/file.vit

IR_E_INVALID_STATEMENT.summary = instruction invalide.
IR_E_INVALID_STATEMENT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_STATEMENT.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_STATEMENT.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_STATEMENT.example = vitte check path/to/file.vit

IR_E_INVALID_EXPRESSION.summary = expression invalide.
IR_E_INVALID_EXPRESSION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_EXPRESSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_EXPRESSION.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_EXPRESSION.example = vitte check path/to/file.vit

IR_E_INVALID_PATTERN.summary = motif invalide.
IR_E_INVALID_PATTERN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_PATTERN.example = vitte check path/to/file.vit

IR_E_INVALID_LITERAL.summary = litteral invalide.
IR_E_INVALID_LITERAL.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_LITERAL.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_LITERAL.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_LITERAL.example = vitte check path/to/file.vit

IR_E_INVALID_OPERATOR.summary = operateur invalide.
IR_E_INVALID_OPERATOR.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_OPERATOR.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_OPERATOR.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_OPERATOR.example = vitte check path/to/file.vit

IR_E_INVALID_MODIFIER.summary = modifier invalide.
IR_E_INVALID_MODIFIER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_MODIFIER.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_MODIFIER.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_MODIFIER.example = vitte check path/to/file.vit

IR_E_MISSING_BODY.summary = body manquant.
IR_E_MISSING_BODY.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_MISSING_BODY.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_MISSING_BODY.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_MISSING_BODY.example = vitte check path/to/file.vit

IR_E_MISSING_RETURN.summary = retour manquant.
IR_E_MISSING_RETURN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_MISSING_RETURN.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_MISSING_RETURN.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_MISSING_RETURN.example = vitte check path/to/file.vit

IR_E_DUPLICATE_NAME.summary = nom duplique.
IR_E_DUPLICATE_NAME.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_DUPLICATE_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_DUPLICATE_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_DUPLICATE_NAME.example = vitte check path/to/file.vit

IR_E_UNKNOWN_NAME.summary = nom inconnu.
IR_E_UNKNOWN_NAME.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_UNKNOWN_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_UNKNOWN_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_UNKNOWN_NAME.example = vitte check path/to/file.vit

IR_E_UNKNOWN_TYPE.summary = type inconnu.
IR_E_UNKNOWN_TYPE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_UNKNOWN_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_UNKNOWN_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_UNKNOWN_TYPE.example = vitte check path/to/file.vit

IR_E_UNKNOWN_MODULE.summary = module inconnu.
IR_E_UNKNOWN_MODULE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_UNKNOWN_MODULE.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_UNKNOWN_MODULE.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_UNKNOWN_MODULE.example = vitte check path/to/file.vit

IR_E_UNKNOWN_MEMBER.summary = membre inconnu.
IR_E_UNKNOWN_MEMBER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_UNKNOWN_MEMBER.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_UNKNOWN_MEMBER.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_UNKNOWN_MEMBER.example = vitte check path/to/file.vit

IR_E_AMBIGUOUS_NAME.summary = ambiguous nom.
IR_E_AMBIGUOUS_NAME.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_AMBIGUOUS_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_AMBIGUOUS_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_AMBIGUOUS_NAME.example = vitte check path/to/file.vit

IR_E_PRIVATE_SYMBOL.summary = private symbole.
IR_E_PRIVATE_SYMBOL.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_PRIVATE_SYMBOL.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_PRIVATE_SYMBOL.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_PRIVATE_SYMBOL.example = vitte check path/to/file.vit

IR_E_IMPORT_NOT_FOUND.summary = import manquant.
IR_E_IMPORT_NOT_FOUND.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_IMPORT_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_IMPORT_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_IMPORT_NOT_FOUND.example = vitte check path/to/file.vit

IR_E_IMPORT_CYCLE.summary = import cycle.
IR_E_IMPORT_CYCLE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_IMPORT_CYCLE.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_IMPORT_CYCLE.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_IMPORT_CYCLE.example = vitte check path/to/file.vit

IR_E_EXPORT_CONFLICT.summary = export conflit.
IR_E_EXPORT_CONFLICT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_EXPORT_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_EXPORT_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_EXPORT_CONFLICT.example = vitte check path/to/file.vit

IR_E_ARITY_MISMATCH.summary = arity incompatibilite.
IR_E_ARITY_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_ARITY_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_ARITY_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_ARITY_MISMATCH.example = vitte check path/to/file.vit

IR_E_ARGUMENT_MISMATCH.summary = argument incompatibilite.
IR_E_ARGUMENT_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_ARGUMENT_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_ARGUMENT_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_ARGUMENT_MISMATCH.example = vitte check path/to/file.vit

IR_E_ASSIGNMENT_MISMATCH.summary = affectation incompatibilite.
IR_E_ASSIGNMENT_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_ASSIGNMENT_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_ASSIGNMENT_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_ASSIGNMENT_MISMATCH.example = vitte check path/to/file.vit

IR_E_BRANCH_MISMATCH.summary = branch incompatibilite.
IR_E_BRANCH_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_BRANCH_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_BRANCH_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_BRANCH_MISMATCH.example = vitte check path/to/file.vit

IR_E_INVALID_CALL.summary = appel invalide.
IR_E_INVALID_CALL.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_CALL.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_CALL.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_CALL.example = vitte check path/to/file.vit

IR_E_INVALID_CAST.summary = conversion invalide.
IR_E_INVALID_CAST.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_CAST.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_CAST.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_CAST.example = vitte check path/to/file.vit

IR_E_INVALID_INDEX.summary = index invalide.
IR_E_INVALID_INDEX.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_INDEX.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_INDEX.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_INDEX.example = vitte check path/to/file.vit

IR_E_INVALID_DEREF.summary = deref invalide.
IR_E_INVALID_DEREF.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_DEREF.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_DEREF.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_DEREF.example = vitte check path/to/file.vit

IR_E_INVALID_BORROW.summary = emprunt invalide.
IR_E_INVALID_BORROW.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_BORROW.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_BORROW.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_BORROW.example = vitte check path/to/file.vit

IR_E_INVALID_MOVE.summary = deplacement invalide.
IR_E_INVALID_MOVE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_MOVE.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_MOVE.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_MOVE.example = vitte check path/to/file.vit

IR_E_USE_AFTER_MOVE.summary = use after deplacement.
IR_E_USE_AFTER_MOVE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_USE_AFTER_MOVE.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_USE_AFTER_MOVE.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_USE_AFTER_MOVE.example = vitte check path/to/file.vit

IR_E_USE_AFTER_DROP.summary = use after destruction.
IR_E_USE_AFTER_DROP.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_USE_AFTER_DROP.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_USE_AFTER_DROP.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_USE_AFTER_DROP.example = vitte check path/to/file.vit

IR_E_USE_BEFORE_INIT.summary = use before init.
IR_E_USE_BEFORE_INIT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_USE_BEFORE_INIT.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_USE_BEFORE_INIT.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_USE_BEFORE_INIT.example = vitte check path/to/file.vit

IR_E_DOUBLE_DROP.summary = double destruction.
IR_E_DOUBLE_DROP.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_DOUBLE_DROP.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_DOUBLE_DROP.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_DOUBLE_DROP.example = vitte check path/to/file.vit

IR_E_BORROW_CONFLICT.summary = emprunt conflit.
IR_E_BORROW_CONFLICT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_BORROW_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_BORROW_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_BORROW_CONFLICT.example = vitte check path/to/file.vit

IR_E_MUTABILITY_CONFLICT.summary = mutability conflit.
IR_E_MUTABILITY_CONFLICT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_MUTABILITY_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_MUTABILITY_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_MUTABILITY_CONFLICT.example = vitte check path/to/file.vit

IR_E_LIFETIME_TOO_SHORT.summary = duree de vie trop short.
IR_E_LIFETIME_TOO_SHORT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_LIFETIME_TOO_SHORT.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_LIFETIME_TOO_SHORT.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_LIFETIME_TOO_SHORT.example = vitte check path/to/file.vit

IR_E_DANGLING_REFERENCE.summary = dangling reference.
IR_E_DANGLING_REFERENCE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_DANGLING_REFERENCE.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_DANGLING_REFERENCE.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_DANGLING_REFERENCE.example = vitte check path/to/file.vit

IR_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
IR_E_NON_EXHAUSTIVE_MATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_NON_EXHAUSTIVE_MATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_NON_EXHAUSTIVE_MATCH.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_NON_EXHAUSTIVE_MATCH.example = vitte check path/to/file.vit

IR_E_UNREACHABLE_PATTERN.summary = inatteignable motif.
IR_E_UNREACHABLE_PATTERN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_UNREACHABLE_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_UNREACHABLE_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_UNREACHABLE_PATTERN.example = vitte check path/to/file.vit

IR_E_CONST_REQUIRED.summary = const required.
IR_E_CONST_REQUIRED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_CONST_REQUIRED.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_CONST_REQUIRED.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_CONST_REQUIRED.example = vitte check path/to/file.vit

IR_E_CONST_OVERFLOW.summary = const depassement.
IR_E_CONST_OVERFLOW.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_CONST_OVERFLOW.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_CONST_OVERFLOW.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_CONST_OVERFLOW.example = vitte check path/to/file.vit

IR_E_CONST_DIVISION_BY_ZERO.summary = const division by zero.
IR_E_CONST_DIVISION_BY_ZERO.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_CONST_DIVISION_BY_ZERO.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_CONST_DIVISION_BY_ZERO.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_CONST_DIVISION_BY_ZERO.example = vitte check path/to/file.vit

IR_E_CONST_CYCLE.summary = const cycle.
IR_E_CONST_CYCLE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_CONST_CYCLE.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_CONST_CYCLE.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_CONST_CYCLE.example = vitte check path/to/file.vit

IR_E_MACRO_NOT_FOUND.summary = macro manquant.
IR_E_MACRO_NOT_FOUND.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_MACRO_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_MACRO_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_MACRO_NOT_FOUND.example = vitte check path/to/file.vit

IR_E_MACRO_RECURSION.summary = macro recursion.
IR_E_MACRO_RECURSION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_MACRO_RECURSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_MACRO_RECURSION.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_MACRO_RECURSION.example = vitte check path/to/file.vit

IR_E_MACRO_EXPANSION_FAILED.summary = macro expansion echec.
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

IR_E_GENERIC_ARGUMENT_MISSING.summary = generique argument manquant.
IR_E_GENERIC_ARGUMENT_MISSING.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_GENERIC_ARGUMENT_MISSING.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_GENERIC_ARGUMENT_MISSING.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_GENERIC_ARGUMENT_MISSING.example = vitte check path/to/file.vit

IR_E_GENERIC_BOUND_FAILED.summary = generique bound echec.
IR_E_GENERIC_BOUND_FAILED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_GENERIC_BOUND_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_GENERIC_BOUND_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_GENERIC_BOUND_FAILED.example = vitte check path/to/file.vit

IR_E_UNSUPPORTED_TARGET.summary = cible non pris en charge.
IR_E_UNSUPPORTED_TARGET.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_UNSUPPORTED_TARGET.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_UNSUPPORTED_TARGET.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_UNSUPPORTED_TARGET.example = vitte check path/to/file.vit

IR_E_ABI_MISMATCH.summary = abi incompatibilite.
IR_E_ABI_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_ABI_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_ABI_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_ABI_MISMATCH.example = vitte check path/to/file.vit

IR_E_LINK_FAILED.summary = link echec.
IR_E_LINK_FAILED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_LINK_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_LINK_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_LINK_FAILED.example = vitte check path/to/file.vit

IR_E_RUNTIME_PANIC.summary = runtime panique.
IR_E_RUNTIME_PANIC.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_RUNTIME_PANIC.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_RUNTIME_PANIC.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_RUNTIME_PANIC.example = vitte check path/to/file.vit

BACKEND_E_EXPECTED_IDENTIFIER.summary = identifiant attendu.
BACKEND_E_EXPECTED_IDENTIFIER.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_EXPECTED_IDENTIFIER.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_EXPECTED_IDENTIFIER.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_EXPECTED_IDENTIFIER.example = vitte build app.vit -o app

BACKEND_E_EXPECTED_EXPRESSION.summary = expression attendu.
BACKEND_E_EXPECTED_EXPRESSION.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_EXPECTED_EXPRESSION.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_EXPECTED_EXPRESSION.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_EXPECTED_EXPRESSION.example = vitte build app.vit -o app

BACKEND_E_EXPECTED_TYPE.summary = type attendu.
BACKEND_E_EXPECTED_TYPE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_EXPECTED_TYPE.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_EXPECTED_TYPE.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_EXPECTED_TYPE.example = vitte build app.vit -o app

BACKEND_E_EXPECTED_PATTERN.summary = motif attendu.
BACKEND_E_EXPECTED_PATTERN.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_EXPECTED_PATTERN.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_EXPECTED_PATTERN.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_EXPECTED_PATTERN.example = vitte build app.vit -o app

BACKEND_E_EXPECTED_BLOCK.summary = bloc attendu.
BACKEND_E_EXPECTED_BLOCK.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_EXPECTED_BLOCK.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_EXPECTED_BLOCK.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_EXPECTED_BLOCK.example = vitte build app.vit -o app

BACKEND_E_EXPECTED_DELIMITER.summary = delimiteur attendu.
BACKEND_E_EXPECTED_DELIMITER.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_EXPECTED_DELIMITER.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_EXPECTED_DELIMITER.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_EXPECTED_DELIMITER.example = vitte build app.vit -o app

BACKEND_E_UNEXPECTED_TOKEN.summary = jeton inattendu.
BACKEND_E_UNEXPECTED_TOKEN.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_UNEXPECTED_TOKEN.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_UNEXPECTED_TOKEN.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_UNEXPECTED_TOKEN.example = vitte build app.vit -o app

BACKEND_E_UNBALANCED_DELIMITER.summary = unbalanced delimiteur.
BACKEND_E_UNBALANCED_DELIMITER.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_UNBALANCED_DELIMITER.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_UNBALANCED_DELIMITER.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_UNBALANCED_DELIMITER.example = vitte build app.vit -o app

BACKEND_E_INVALID_ATTRIBUTE.summary = attribute invalide.
BACKEND_E_INVALID_ATTRIBUTE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_ATTRIBUTE.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_ATTRIBUTE.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_ATTRIBUTE.example = vitte build app.vit -o app

BACKEND_E_INVALID_DECLARATION.summary = declaration invalide.
BACKEND_E_INVALID_DECLARATION.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_DECLARATION.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_DECLARATION.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_DECLARATION.example = vitte build app.vit -o app

BACKEND_E_INVALID_STATEMENT.summary = instruction invalide.
BACKEND_E_INVALID_STATEMENT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_STATEMENT.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_STATEMENT.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_STATEMENT.example = vitte build app.vit -o app

BACKEND_E_INVALID_EXPRESSION.summary = expression invalide.
BACKEND_E_INVALID_EXPRESSION.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_EXPRESSION.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_EXPRESSION.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_EXPRESSION.example = vitte build app.vit -o app

BACKEND_E_INVALID_PATTERN.summary = motif invalide.
BACKEND_E_INVALID_PATTERN.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_PATTERN.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_PATTERN.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_PATTERN.example = vitte build app.vit -o app

BACKEND_E_INVALID_LITERAL.summary = litteral invalide.
BACKEND_E_INVALID_LITERAL.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_LITERAL.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_LITERAL.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_LITERAL.example = vitte build app.vit -o app

BACKEND_E_INVALID_OPERATOR.summary = operateur invalide.
BACKEND_E_INVALID_OPERATOR.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_OPERATOR.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_OPERATOR.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_OPERATOR.example = vitte build app.vit -o app

BACKEND_E_INVALID_MODIFIER.summary = modifier invalide.
BACKEND_E_INVALID_MODIFIER.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_MODIFIER.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_MODIFIER.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_MODIFIER.example = vitte build app.vit -o app

BACKEND_E_MISSING_BODY.summary = body manquant.
BACKEND_E_MISSING_BODY.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_MISSING_BODY.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_MISSING_BODY.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_MISSING_BODY.example = vitte build app.vit -o app

BACKEND_E_MISSING_RETURN.summary = retour manquant.
BACKEND_E_MISSING_RETURN.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_MISSING_RETURN.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_MISSING_RETURN.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_MISSING_RETURN.example = vitte build app.vit -o app

BACKEND_E_DUPLICATE_NAME.summary = nom duplique.
BACKEND_E_DUPLICATE_NAME.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_DUPLICATE_NAME.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_DUPLICATE_NAME.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_DUPLICATE_NAME.example = vitte build app.vit -o app

BACKEND_E_UNKNOWN_NAME.summary = nom inconnu.
BACKEND_E_UNKNOWN_NAME.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_UNKNOWN_NAME.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_UNKNOWN_NAME.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_UNKNOWN_NAME.example = vitte build app.vit -o app

BACKEND_E_UNKNOWN_TYPE.summary = type inconnu.
BACKEND_E_UNKNOWN_TYPE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_UNKNOWN_TYPE.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_UNKNOWN_TYPE.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_UNKNOWN_TYPE.example = vitte build app.vit -o app

BACKEND_E_UNKNOWN_MODULE.summary = module inconnu.
BACKEND_E_UNKNOWN_MODULE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_UNKNOWN_MODULE.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_UNKNOWN_MODULE.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_UNKNOWN_MODULE.example = vitte build app.vit -o app

BACKEND_E_UNKNOWN_MEMBER.summary = membre inconnu.
BACKEND_E_UNKNOWN_MEMBER.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_UNKNOWN_MEMBER.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_UNKNOWN_MEMBER.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_UNKNOWN_MEMBER.example = vitte build app.vit -o app

BACKEND_E_AMBIGUOUS_NAME.summary = ambiguous nom.
BACKEND_E_AMBIGUOUS_NAME.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_AMBIGUOUS_NAME.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_AMBIGUOUS_NAME.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_AMBIGUOUS_NAME.example = vitte build app.vit -o app

BACKEND_E_PRIVATE_SYMBOL.summary = private symbole.
BACKEND_E_PRIVATE_SYMBOL.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_PRIVATE_SYMBOL.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_PRIVATE_SYMBOL.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_PRIVATE_SYMBOL.example = vitte build app.vit -o app

BACKEND_E_IMPORT_NOT_FOUND.summary = import manquant.
BACKEND_E_IMPORT_NOT_FOUND.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_IMPORT_NOT_FOUND.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_IMPORT_NOT_FOUND.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_IMPORT_NOT_FOUND.example = vitte build app.vit -o app

BACKEND_E_IMPORT_CYCLE.summary = import cycle.
BACKEND_E_IMPORT_CYCLE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_IMPORT_CYCLE.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_IMPORT_CYCLE.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_IMPORT_CYCLE.example = vitte build app.vit -o app

BACKEND_E_EXPORT_CONFLICT.summary = export conflit.
BACKEND_E_EXPORT_CONFLICT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_EXPORT_CONFLICT.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_EXPORT_CONFLICT.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_EXPORT_CONFLICT.example = vitte build app.vit -o app

BACKEND_E_ARITY_MISMATCH.summary = arity incompatibilite.
BACKEND_E_ARITY_MISMATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_ARITY_MISMATCH.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_ARITY_MISMATCH.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_ARITY_MISMATCH.example = vitte build app.vit -o app

BACKEND_E_ARGUMENT_MISMATCH.summary = argument incompatibilite.
BACKEND_E_ARGUMENT_MISMATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_ARGUMENT_MISMATCH.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_ARGUMENT_MISMATCH.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_ARGUMENT_MISMATCH.example = vitte build app.vit -o app

BACKEND_E_ASSIGNMENT_MISMATCH.summary = affectation incompatibilite.
BACKEND_E_ASSIGNMENT_MISMATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_ASSIGNMENT_MISMATCH.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_ASSIGNMENT_MISMATCH.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_ASSIGNMENT_MISMATCH.example = vitte build app.vit -o app

BACKEND_E_BRANCH_MISMATCH.summary = branch incompatibilite.
BACKEND_E_BRANCH_MISMATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_BRANCH_MISMATCH.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_BRANCH_MISMATCH.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_BRANCH_MISMATCH.example = vitte build app.vit -o app

BACKEND_E_INVALID_CALL.summary = appel invalide.
BACKEND_E_INVALID_CALL.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_CALL.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_CALL.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_CALL.example = vitte build app.vit -o app

BACKEND_E_INVALID_CAST.summary = conversion invalide.
BACKEND_E_INVALID_CAST.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_CAST.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_CAST.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_CAST.example = vitte build app.vit -o app

BACKEND_E_INVALID_INDEX.summary = index invalide.
BACKEND_E_INVALID_INDEX.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_INDEX.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_INDEX.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_INDEX.example = vitte build app.vit -o app

BACKEND_E_INVALID_DEREF.summary = deref invalide.
BACKEND_E_INVALID_DEREF.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_DEREF.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_DEREF.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_DEREF.example = vitte build app.vit -o app

BACKEND_E_INVALID_BORROW.summary = emprunt invalide.
BACKEND_E_INVALID_BORROW.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_BORROW.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_BORROW.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_BORROW.example = vitte build app.vit -o app

BACKEND_E_INVALID_MOVE.summary = deplacement invalide.
BACKEND_E_INVALID_MOVE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_MOVE.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_MOVE.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_MOVE.example = vitte build app.vit -o app

BACKEND_E_USE_AFTER_MOVE.summary = use after deplacement.
BACKEND_E_USE_AFTER_MOVE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_USE_AFTER_MOVE.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_USE_AFTER_MOVE.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_USE_AFTER_MOVE.example = vitte build app.vit -o app

BACKEND_E_USE_AFTER_DROP.summary = use after destruction.
BACKEND_E_USE_AFTER_DROP.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_USE_AFTER_DROP.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_USE_AFTER_DROP.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_USE_AFTER_DROP.example = vitte build app.vit -o app

BACKEND_E_USE_BEFORE_INIT.summary = use before init.
BACKEND_E_USE_BEFORE_INIT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_USE_BEFORE_INIT.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_USE_BEFORE_INIT.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_USE_BEFORE_INIT.example = vitte build app.vit -o app

BACKEND_E_DOUBLE_DROP.summary = double destruction.
BACKEND_E_DOUBLE_DROP.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_DOUBLE_DROP.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_DOUBLE_DROP.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_DOUBLE_DROP.example = vitte build app.vit -o app

BACKEND_E_BORROW_CONFLICT.summary = emprunt conflit.
BACKEND_E_BORROW_CONFLICT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_BORROW_CONFLICT.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_BORROW_CONFLICT.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_BORROW_CONFLICT.example = vitte build app.vit -o app

BACKEND_E_MUTABILITY_CONFLICT.summary = mutability conflit.
BACKEND_E_MUTABILITY_CONFLICT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_MUTABILITY_CONFLICT.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_MUTABILITY_CONFLICT.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_MUTABILITY_CONFLICT.example = vitte build app.vit -o app

BACKEND_E_LIFETIME_TOO_SHORT.summary = duree de vie trop short.
BACKEND_E_LIFETIME_TOO_SHORT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_LIFETIME_TOO_SHORT.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_LIFETIME_TOO_SHORT.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_LIFETIME_TOO_SHORT.example = vitte build app.vit -o app

BACKEND_E_DANGLING_REFERENCE.summary = dangling reference.
BACKEND_E_DANGLING_REFERENCE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_DANGLING_REFERENCE.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_DANGLING_REFERENCE.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_DANGLING_REFERENCE.example = vitte build app.vit -o app

BACKEND_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
BACKEND_E_NON_EXHAUSTIVE_MATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_NON_EXHAUSTIVE_MATCH.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_NON_EXHAUSTIVE_MATCH.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_NON_EXHAUSTIVE_MATCH.example = vitte build app.vit -o app

BACKEND_E_UNREACHABLE_PATTERN.summary = inatteignable motif.
BACKEND_E_UNREACHABLE_PATTERN.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_UNREACHABLE_PATTERN.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_UNREACHABLE_PATTERN.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_UNREACHABLE_PATTERN.example = vitte build app.vit -o app

BACKEND_E_CONST_REQUIRED.summary = const required.
BACKEND_E_CONST_REQUIRED.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_CONST_REQUIRED.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_CONST_REQUIRED.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_CONST_REQUIRED.example = vitte build app.vit -o app

BACKEND_E_CONST_OVERFLOW.summary = const depassement.
BACKEND_E_CONST_OVERFLOW.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_CONST_OVERFLOW.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_CONST_OVERFLOW.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_CONST_OVERFLOW.example = vitte build app.vit -o app

BACKEND_E_CONST_DIVISION_BY_ZERO.summary = const division by zero.
BACKEND_E_CONST_DIVISION_BY_ZERO.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_CONST_DIVISION_BY_ZERO.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_CONST_DIVISION_BY_ZERO.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_CONST_DIVISION_BY_ZERO.example = vitte build app.vit -o app

BACKEND_E_CONST_CYCLE.summary = const cycle.
BACKEND_E_CONST_CYCLE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_CONST_CYCLE.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_CONST_CYCLE.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_CONST_CYCLE.example = vitte build app.vit -o app

BACKEND_E_MACRO_NOT_FOUND.summary = macro manquant.
BACKEND_E_MACRO_NOT_FOUND.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_MACRO_NOT_FOUND.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_MACRO_NOT_FOUND.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_MACRO_NOT_FOUND.example = vitte build app.vit -o app

BACKEND_E_MACRO_RECURSION.summary = macro recursion.
BACKEND_E_MACRO_RECURSION.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_MACRO_RECURSION.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_MACRO_RECURSION.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_MACRO_RECURSION.example = vitte build app.vit -o app

BACKEND_E_MACRO_EXPANSION_FAILED.summary = macro expansion echec.
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

BACKEND_E_GENERIC_ARGUMENT_MISSING.summary = generique argument manquant.
BACKEND_E_GENERIC_ARGUMENT_MISSING.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_GENERIC_ARGUMENT_MISSING.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_GENERIC_ARGUMENT_MISSING.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_GENERIC_ARGUMENT_MISSING.example = vitte build app.vit -o app

BACKEND_E_GENERIC_BOUND_FAILED.summary = generique bound echec.
BACKEND_E_GENERIC_BOUND_FAILED.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_GENERIC_BOUND_FAILED.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_GENERIC_BOUND_FAILED.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_GENERIC_BOUND_FAILED.example = vitte build app.vit -o app

BACKEND_E_LINK_FAILED.summary = link echec.
BACKEND_E_LINK_FAILED.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_LINK_FAILED.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_LINK_FAILED.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_LINK_FAILED.example = vitte build app.vit -o app

BACKEND_E_RUNTIME_PANIC.summary = runtime panique.
BACKEND_E_RUNTIME_PANIC.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_RUNTIME_PANIC.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_RUNTIME_PANIC.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_RUNTIME_PANIC.example = vitte build app.vit -o app

LINK_E_EXPECTED_IDENTIFIER.summary = identifiant attendu.
LINK_E_EXPECTED_IDENTIFIER.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_EXPECTED_IDENTIFIER.step1 = Check the target triple and the first backend or linker note.
LINK_E_EXPECTED_IDENTIFIER.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_EXPECTED_IDENTIFIER.example = vitte build app.vit -o app

LINK_E_EXPECTED_EXPRESSION.summary = expression attendu.
LINK_E_EXPECTED_EXPRESSION.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_EXPECTED_EXPRESSION.step1 = Check the target triple and the first backend or linker note.
LINK_E_EXPECTED_EXPRESSION.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_EXPECTED_EXPRESSION.example = vitte build app.vit -o app

LINK_E_EXPECTED_TYPE.summary = type attendu.
LINK_E_EXPECTED_TYPE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_EXPECTED_TYPE.step1 = Check the target triple and the first backend or linker note.
LINK_E_EXPECTED_TYPE.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_EXPECTED_TYPE.example = vitte build app.vit -o app

LINK_E_EXPECTED_PATTERN.summary = motif attendu.
LINK_E_EXPECTED_PATTERN.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_EXPECTED_PATTERN.step1 = Check the target triple and the first backend or linker note.
LINK_E_EXPECTED_PATTERN.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_EXPECTED_PATTERN.example = vitte build app.vit -o app

LINK_E_EXPECTED_BLOCK.summary = bloc attendu.
LINK_E_EXPECTED_BLOCK.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_EXPECTED_BLOCK.step1 = Check the target triple and the first backend or linker note.
LINK_E_EXPECTED_BLOCK.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_EXPECTED_BLOCK.example = vitte build app.vit -o app

LINK_E_EXPECTED_DELIMITER.summary = delimiteur attendu.
LINK_E_EXPECTED_DELIMITER.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_EXPECTED_DELIMITER.step1 = Check the target triple and the first backend or linker note.
LINK_E_EXPECTED_DELIMITER.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_EXPECTED_DELIMITER.example = vitte build app.vit -o app

LINK_E_UNEXPECTED_TOKEN.summary = jeton inattendu.
LINK_E_UNEXPECTED_TOKEN.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_UNEXPECTED_TOKEN.step1 = Check the target triple and the first backend or linker note.
LINK_E_UNEXPECTED_TOKEN.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_UNEXPECTED_TOKEN.example = vitte build app.vit -o app

LINK_E_UNBALANCED_DELIMITER.summary = unbalanced delimiteur.
LINK_E_UNBALANCED_DELIMITER.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_UNBALANCED_DELIMITER.step1 = Check the target triple and the first backend or linker note.
LINK_E_UNBALANCED_DELIMITER.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_UNBALANCED_DELIMITER.example = vitte build app.vit -o app

LINK_E_INVALID_ATTRIBUTE.summary = attribute invalide.
LINK_E_INVALID_ATTRIBUTE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_ATTRIBUTE.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_ATTRIBUTE.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_ATTRIBUTE.example = vitte build app.vit -o app

LINK_E_INVALID_DECLARATION.summary = declaration invalide.
LINK_E_INVALID_DECLARATION.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_DECLARATION.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_DECLARATION.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_DECLARATION.example = vitte build app.vit -o app

LINK_E_INVALID_STATEMENT.summary = instruction invalide.
LINK_E_INVALID_STATEMENT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_STATEMENT.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_STATEMENT.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_STATEMENT.example = vitte build app.vit -o app

LINK_E_INVALID_EXPRESSION.summary = expression invalide.
LINK_E_INVALID_EXPRESSION.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_EXPRESSION.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_EXPRESSION.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_EXPRESSION.example = vitte build app.vit -o app

LINK_E_INVALID_PATTERN.summary = motif invalide.
LINK_E_INVALID_PATTERN.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_PATTERN.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_PATTERN.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_PATTERN.example = vitte build app.vit -o app

LINK_E_INVALID_LITERAL.summary = litteral invalide.
LINK_E_INVALID_LITERAL.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_LITERAL.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_LITERAL.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_LITERAL.example = vitte build app.vit -o app

LINK_E_INVALID_OPERATOR.summary = operateur invalide.
LINK_E_INVALID_OPERATOR.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_OPERATOR.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_OPERATOR.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_OPERATOR.example = vitte build app.vit -o app

LINK_E_INVALID_MODIFIER.summary = modifier invalide.
LINK_E_INVALID_MODIFIER.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_MODIFIER.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_MODIFIER.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_MODIFIER.example = vitte build app.vit -o app

LINK_E_MISSING_BODY.summary = body manquant.
LINK_E_MISSING_BODY.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_MISSING_BODY.step1 = Check the target triple and the first backend or linker note.
LINK_E_MISSING_BODY.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_MISSING_BODY.example = vitte build app.vit -o app

LINK_E_MISSING_RETURN.summary = retour manquant.
LINK_E_MISSING_RETURN.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_MISSING_RETURN.step1 = Check the target triple and the first backend or linker note.
LINK_E_MISSING_RETURN.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_MISSING_RETURN.example = vitte build app.vit -o app

LINK_E_DUPLICATE_NAME.summary = nom duplique.
LINK_E_DUPLICATE_NAME.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_DUPLICATE_NAME.step1 = Check the target triple and the first backend or linker note.
LINK_E_DUPLICATE_NAME.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_DUPLICATE_NAME.example = vitte build app.vit -o app

LINK_E_UNKNOWN_NAME.summary = nom inconnu.
LINK_E_UNKNOWN_NAME.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_UNKNOWN_NAME.step1 = Check the target triple and the first backend or linker note.
LINK_E_UNKNOWN_NAME.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_UNKNOWN_NAME.example = vitte build app.vit -o app

LINK_E_UNKNOWN_TYPE.summary = type inconnu.
LINK_E_UNKNOWN_TYPE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_UNKNOWN_TYPE.step1 = Check the target triple and the first backend or linker note.
LINK_E_UNKNOWN_TYPE.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_UNKNOWN_TYPE.example = vitte build app.vit -o app

LINK_E_UNKNOWN_MODULE.summary = module inconnu.
LINK_E_UNKNOWN_MODULE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_UNKNOWN_MODULE.step1 = Check the target triple and the first backend or linker note.
LINK_E_UNKNOWN_MODULE.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_UNKNOWN_MODULE.example = vitte build app.vit -o app

LINK_E_UNKNOWN_MEMBER.summary = membre inconnu.
LINK_E_UNKNOWN_MEMBER.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_UNKNOWN_MEMBER.step1 = Check the target triple and the first backend or linker note.
LINK_E_UNKNOWN_MEMBER.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_UNKNOWN_MEMBER.example = vitte build app.vit -o app

LINK_E_AMBIGUOUS_NAME.summary = ambiguous nom.
LINK_E_AMBIGUOUS_NAME.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_AMBIGUOUS_NAME.step1 = Check the target triple and the first backend or linker note.
LINK_E_AMBIGUOUS_NAME.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_AMBIGUOUS_NAME.example = vitte build app.vit -o app

LINK_E_PRIVATE_SYMBOL.summary = private symbole.
LINK_E_PRIVATE_SYMBOL.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_PRIVATE_SYMBOL.step1 = Check the target triple and the first backend or linker note.
LINK_E_PRIVATE_SYMBOL.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_PRIVATE_SYMBOL.example = vitte build app.vit -o app

LINK_E_IMPORT_NOT_FOUND.summary = import manquant.
LINK_E_IMPORT_NOT_FOUND.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_IMPORT_NOT_FOUND.step1 = Check the target triple and the first backend or linker note.
LINK_E_IMPORT_NOT_FOUND.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_IMPORT_NOT_FOUND.example = vitte build app.vit -o app

LINK_E_IMPORT_CYCLE.summary = import cycle.
LINK_E_IMPORT_CYCLE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_IMPORT_CYCLE.step1 = Check the target triple and the first backend or linker note.
LINK_E_IMPORT_CYCLE.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_IMPORT_CYCLE.example = vitte build app.vit -o app

LINK_E_EXPORT_CONFLICT.summary = export conflit.
LINK_E_EXPORT_CONFLICT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_EXPORT_CONFLICT.step1 = Check the target triple and the first backend or linker note.
LINK_E_EXPORT_CONFLICT.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_EXPORT_CONFLICT.example = vitte build app.vit -o app

LINK_E_ARITY_MISMATCH.summary = arity incompatibilite.
LINK_E_ARITY_MISMATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_ARITY_MISMATCH.step1 = Check the target triple and the first backend or linker note.
LINK_E_ARITY_MISMATCH.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_ARITY_MISMATCH.example = vitte build app.vit -o app

LINK_E_ARGUMENT_MISMATCH.summary = argument incompatibilite.
LINK_E_ARGUMENT_MISMATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_ARGUMENT_MISMATCH.step1 = Check the target triple and the first backend or linker note.
LINK_E_ARGUMENT_MISMATCH.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_ARGUMENT_MISMATCH.example = vitte build app.vit -o app

LINK_E_ASSIGNMENT_MISMATCH.summary = affectation incompatibilite.
LINK_E_ASSIGNMENT_MISMATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_ASSIGNMENT_MISMATCH.step1 = Check the target triple and the first backend or linker note.
LINK_E_ASSIGNMENT_MISMATCH.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_ASSIGNMENT_MISMATCH.example = vitte build app.vit -o app

LINK_E_BRANCH_MISMATCH.summary = branch incompatibilite.
LINK_E_BRANCH_MISMATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_BRANCH_MISMATCH.step1 = Check the target triple and the first backend or linker note.
LINK_E_BRANCH_MISMATCH.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_BRANCH_MISMATCH.example = vitte build app.vit -o app

LINK_E_INVALID_CALL.summary = appel invalide.
LINK_E_INVALID_CALL.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_CALL.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_CALL.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_CALL.example = vitte build app.vit -o app

LINK_E_INVALID_CAST.summary = conversion invalide.
LINK_E_INVALID_CAST.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_CAST.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_CAST.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_CAST.example = vitte build app.vit -o app

LINK_E_INVALID_INDEX.summary = index invalide.
LINK_E_INVALID_INDEX.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_INDEX.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_INDEX.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_INDEX.example = vitte build app.vit -o app

LINK_E_INVALID_DEREF.summary = deref invalide.
LINK_E_INVALID_DEREF.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_DEREF.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_DEREF.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_DEREF.example = vitte build app.vit -o app

LINK_E_INVALID_BORROW.summary = emprunt invalide.
LINK_E_INVALID_BORROW.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_BORROW.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_BORROW.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_BORROW.example = vitte build app.vit -o app

LINK_E_INVALID_MOVE.summary = deplacement invalide.
LINK_E_INVALID_MOVE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_MOVE.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_MOVE.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_MOVE.example = vitte build app.vit -o app

LINK_E_USE_AFTER_MOVE.summary = use after deplacement.
LINK_E_USE_AFTER_MOVE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_USE_AFTER_MOVE.step1 = Check the target triple and the first backend or linker note.
LINK_E_USE_AFTER_MOVE.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_USE_AFTER_MOVE.example = vitte build app.vit -o app

LINK_E_USE_AFTER_DROP.summary = use after destruction.
LINK_E_USE_AFTER_DROP.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_USE_AFTER_DROP.step1 = Check the target triple and the first backend or linker note.
LINK_E_USE_AFTER_DROP.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_USE_AFTER_DROP.example = vitte build app.vit -o app

LINK_E_USE_BEFORE_INIT.summary = use before init.
LINK_E_USE_BEFORE_INIT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_USE_BEFORE_INIT.step1 = Check the target triple and the first backend or linker note.
LINK_E_USE_BEFORE_INIT.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_USE_BEFORE_INIT.example = vitte build app.vit -o app

LINK_E_DOUBLE_DROP.summary = double destruction.
LINK_E_DOUBLE_DROP.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_DOUBLE_DROP.step1 = Check the target triple and the first backend or linker note.
LINK_E_DOUBLE_DROP.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_DOUBLE_DROP.example = vitte build app.vit -o app

LINK_E_BORROW_CONFLICT.summary = emprunt conflit.
LINK_E_BORROW_CONFLICT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_BORROW_CONFLICT.step1 = Check the target triple and the first backend or linker note.
LINK_E_BORROW_CONFLICT.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_BORROW_CONFLICT.example = vitte build app.vit -o app

LINK_E_MUTABILITY_CONFLICT.summary = mutability conflit.
LINK_E_MUTABILITY_CONFLICT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_MUTABILITY_CONFLICT.step1 = Check the target triple and the first backend or linker note.
LINK_E_MUTABILITY_CONFLICT.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_MUTABILITY_CONFLICT.example = vitte build app.vit -o app

LINK_E_LIFETIME_TOO_SHORT.summary = duree de vie trop short.
LINK_E_LIFETIME_TOO_SHORT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_LIFETIME_TOO_SHORT.step1 = Check the target triple and the first backend or linker note.
LINK_E_LIFETIME_TOO_SHORT.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_LIFETIME_TOO_SHORT.example = vitte build app.vit -o app

LINK_E_DANGLING_REFERENCE.summary = dangling reference.
LINK_E_DANGLING_REFERENCE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_DANGLING_REFERENCE.step1 = Check the target triple and the first backend or linker note.
LINK_E_DANGLING_REFERENCE.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_DANGLING_REFERENCE.example = vitte build app.vit -o app

LINK_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
LINK_E_NON_EXHAUSTIVE_MATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_NON_EXHAUSTIVE_MATCH.step1 = Check the target triple and the first backend or linker note.
LINK_E_NON_EXHAUSTIVE_MATCH.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_NON_EXHAUSTIVE_MATCH.example = vitte build app.vit -o app

LINK_E_UNREACHABLE_PATTERN.summary = inatteignable motif.
LINK_E_UNREACHABLE_PATTERN.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_UNREACHABLE_PATTERN.step1 = Check the target triple and the first backend or linker note.
LINK_E_UNREACHABLE_PATTERN.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_UNREACHABLE_PATTERN.example = vitte build app.vit -o app

LINK_E_CONST_REQUIRED.summary = const required.
LINK_E_CONST_REQUIRED.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_CONST_REQUIRED.step1 = Check the target triple and the first backend or linker note.
LINK_E_CONST_REQUIRED.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_CONST_REQUIRED.example = vitte build app.vit -o app

LINK_E_CONST_OVERFLOW.summary = const depassement.
LINK_E_CONST_OVERFLOW.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_CONST_OVERFLOW.step1 = Check the target triple and the first backend or linker note.
LINK_E_CONST_OVERFLOW.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_CONST_OVERFLOW.example = vitte build app.vit -o app

LINK_E_CONST_DIVISION_BY_ZERO.summary = const division by zero.
LINK_E_CONST_DIVISION_BY_ZERO.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_CONST_DIVISION_BY_ZERO.step1 = Check the target triple and the first backend or linker note.
LINK_E_CONST_DIVISION_BY_ZERO.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_CONST_DIVISION_BY_ZERO.example = vitte build app.vit -o app

LINK_E_CONST_CYCLE.summary = const cycle.
LINK_E_CONST_CYCLE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_CONST_CYCLE.step1 = Check the target triple and the first backend or linker note.
LINK_E_CONST_CYCLE.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_CONST_CYCLE.example = vitte build app.vit -o app

LINK_E_MACRO_NOT_FOUND.summary = macro manquant.
LINK_E_MACRO_NOT_FOUND.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_MACRO_NOT_FOUND.step1 = Check the target triple and the first backend or linker note.
LINK_E_MACRO_NOT_FOUND.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_MACRO_NOT_FOUND.example = vitte build app.vit -o app

LINK_E_MACRO_RECURSION.summary = macro recursion.
LINK_E_MACRO_RECURSION.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_MACRO_RECURSION.step1 = Check the target triple and the first backend or linker note.
LINK_E_MACRO_RECURSION.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_MACRO_RECURSION.example = vitte build app.vit -o app

LINK_E_MACRO_EXPANSION_FAILED.summary = macro expansion echec.
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

LINK_E_GENERIC_ARGUMENT_MISSING.summary = generique argument manquant.
LINK_E_GENERIC_ARGUMENT_MISSING.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_GENERIC_ARGUMENT_MISSING.step1 = Check the target triple and the first backend or linker note.
LINK_E_GENERIC_ARGUMENT_MISSING.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_GENERIC_ARGUMENT_MISSING.example = vitte build app.vit -o app

LINK_E_GENERIC_BOUND_FAILED.summary = generique bound echec.
LINK_E_GENERIC_BOUND_FAILED.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_GENERIC_BOUND_FAILED.step1 = Check the target triple and the first backend or linker note.
LINK_E_GENERIC_BOUND_FAILED.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_GENERIC_BOUND_FAILED.example = vitte build app.vit -o app

LINK_E_UNSUPPORTED_TARGET.summary = cible non pris en charge.
LINK_E_UNSUPPORTED_TARGET.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_UNSUPPORTED_TARGET.step1 = Check the target triple and the first backend or linker note.
LINK_E_UNSUPPORTED_TARGET.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_UNSUPPORTED_TARGET.example = vitte build app.vit -o app

LINK_E_ABI_MISMATCH.summary = abi incompatibilite.
LINK_E_ABI_MISMATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_ABI_MISMATCH.step1 = Check the target triple and the first backend or linker note.
LINK_E_ABI_MISMATCH.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_ABI_MISMATCH.example = vitte build app.vit -o app

LINK_E_LINK_FAILED.summary = link echec.
LINK_E_LINK_FAILED.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_LINK_FAILED.step1 = Check the target triple and the first backend or linker note.
LINK_E_LINK_FAILED.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_LINK_FAILED.example = vitte build app.vit -o app

LINK_E_RUNTIME_PANIC.summary = runtime panique.
LINK_E_RUNTIME_PANIC.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_RUNTIME_PANIC.step1 = Check the target triple and the first backend or linker note.
LINK_E_RUNTIME_PANIC.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_RUNTIME_PANIC.example = vitte build app.vit -o app

RUNTIME_E_EXPECTED_IDENTIFIER.summary = identifiant attendu.
RUNTIME_E_EXPECTED_IDENTIFIER.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_EXPECTED_IDENTIFIER.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_EXPECTED_IDENTIFIER.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_EXPECTED_IDENTIFIER.example = vitte check path/to/file.vit

RUNTIME_E_EXPECTED_EXPRESSION.summary = expression attendu.
RUNTIME_E_EXPECTED_EXPRESSION.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_EXPECTED_EXPRESSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_EXPECTED_EXPRESSION.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_EXPECTED_EXPRESSION.example = vitte check path/to/file.vit

RUNTIME_E_EXPECTED_TYPE.summary = type attendu.
RUNTIME_E_EXPECTED_TYPE.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_EXPECTED_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_EXPECTED_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_EXPECTED_TYPE.example = vitte check path/to/file.vit

RUNTIME_E_EXPECTED_PATTERN.summary = motif attendu.
RUNTIME_E_EXPECTED_PATTERN.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_EXPECTED_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_EXPECTED_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_EXPECTED_PATTERN.example = vitte check path/to/file.vit

RUNTIME_E_EXPECTED_BLOCK.summary = bloc attendu.
RUNTIME_E_EXPECTED_BLOCK.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_EXPECTED_BLOCK.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_EXPECTED_BLOCK.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_EXPECTED_BLOCK.example = vitte check path/to/file.vit

RUNTIME_E_EXPECTED_DELIMITER.summary = delimiteur attendu.
RUNTIME_E_EXPECTED_DELIMITER.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_EXPECTED_DELIMITER.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_EXPECTED_DELIMITER.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_EXPECTED_DELIMITER.example = vitte check path/to/file.vit

RUNTIME_E_UNEXPECTED_TOKEN.summary = jeton inattendu.
RUNTIME_E_UNEXPECTED_TOKEN.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_UNEXPECTED_TOKEN.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_UNEXPECTED_TOKEN.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_UNEXPECTED_TOKEN.example = vitte check path/to/file.vit

RUNTIME_E_UNBALANCED_DELIMITER.summary = unbalanced delimiteur.
RUNTIME_E_UNBALANCED_DELIMITER.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_UNBALANCED_DELIMITER.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_UNBALANCED_DELIMITER.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_UNBALANCED_DELIMITER.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_ATTRIBUTE.summary = attribute invalide.
RUNTIME_E_INVALID_ATTRIBUTE.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_ATTRIBUTE.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_ATTRIBUTE.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_ATTRIBUTE.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_DECLARATION.summary = declaration invalide.
RUNTIME_E_INVALID_DECLARATION.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_DECLARATION.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_DECLARATION.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_DECLARATION.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_STATEMENT.summary = instruction invalide.
RUNTIME_E_INVALID_STATEMENT.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_STATEMENT.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_STATEMENT.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_STATEMENT.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_EXPRESSION.summary = expression invalide.
RUNTIME_E_INVALID_EXPRESSION.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_EXPRESSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_EXPRESSION.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_EXPRESSION.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_PATTERN.summary = motif invalide.
RUNTIME_E_INVALID_PATTERN.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_PATTERN.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_LITERAL.summary = litteral invalide.
RUNTIME_E_INVALID_LITERAL.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_LITERAL.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_LITERAL.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_LITERAL.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_OPERATOR.summary = operateur invalide.
RUNTIME_E_INVALID_OPERATOR.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_OPERATOR.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_OPERATOR.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_OPERATOR.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_MODIFIER.summary = modifier invalide.
RUNTIME_E_INVALID_MODIFIER.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_MODIFIER.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_MODIFIER.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_MODIFIER.example = vitte check path/to/file.vit

RUNTIME_E_MISSING_BODY.summary = body manquant.
RUNTIME_E_MISSING_BODY.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_MISSING_BODY.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_MISSING_BODY.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_MISSING_BODY.example = vitte check path/to/file.vit

RUNTIME_E_MISSING_RETURN.summary = retour manquant.
RUNTIME_E_MISSING_RETURN.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_MISSING_RETURN.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_MISSING_RETURN.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_MISSING_RETURN.example = vitte check path/to/file.vit

RUNTIME_E_DUPLICATE_NAME.summary = nom duplique.
RUNTIME_E_DUPLICATE_NAME.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_DUPLICATE_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_DUPLICATE_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_DUPLICATE_NAME.example = vitte check path/to/file.vit

RUNTIME_E_UNKNOWN_NAME.summary = nom inconnu.
RUNTIME_E_UNKNOWN_NAME.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_UNKNOWN_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_UNKNOWN_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_UNKNOWN_NAME.example = vitte check path/to/file.vit

RUNTIME_E_UNKNOWN_TYPE.summary = type inconnu.
RUNTIME_E_UNKNOWN_TYPE.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_UNKNOWN_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_UNKNOWN_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_UNKNOWN_TYPE.example = vitte check path/to/file.vit

RUNTIME_E_UNKNOWN_MODULE.summary = module inconnu.
RUNTIME_E_UNKNOWN_MODULE.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_UNKNOWN_MODULE.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_UNKNOWN_MODULE.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_UNKNOWN_MODULE.example = vitte check path/to/file.vit

RUNTIME_E_UNKNOWN_MEMBER.summary = membre inconnu.
RUNTIME_E_UNKNOWN_MEMBER.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_UNKNOWN_MEMBER.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_UNKNOWN_MEMBER.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_UNKNOWN_MEMBER.example = vitte check path/to/file.vit

RUNTIME_E_AMBIGUOUS_NAME.summary = ambiguous nom.
RUNTIME_E_AMBIGUOUS_NAME.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_AMBIGUOUS_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_AMBIGUOUS_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_AMBIGUOUS_NAME.example = vitte check path/to/file.vit

RUNTIME_E_PRIVATE_SYMBOL.summary = private symbole.
RUNTIME_E_PRIVATE_SYMBOL.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_PRIVATE_SYMBOL.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_PRIVATE_SYMBOL.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_PRIVATE_SYMBOL.example = vitte check path/to/file.vit

RUNTIME_E_IMPORT_NOT_FOUND.summary = import manquant.
RUNTIME_E_IMPORT_NOT_FOUND.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_IMPORT_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_IMPORT_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_IMPORT_NOT_FOUND.example = vitte check path/to/file.vit

RUNTIME_E_IMPORT_CYCLE.summary = import cycle.
RUNTIME_E_IMPORT_CYCLE.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_IMPORT_CYCLE.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_IMPORT_CYCLE.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_IMPORT_CYCLE.example = vitte check path/to/file.vit

RUNTIME_E_EXPORT_CONFLICT.summary = export conflit.
RUNTIME_E_EXPORT_CONFLICT.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_EXPORT_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_EXPORT_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_EXPORT_CONFLICT.example = vitte check path/to/file.vit

RUNTIME_E_ARITY_MISMATCH.summary = arity incompatibilite.
RUNTIME_E_ARITY_MISMATCH.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_ARITY_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_ARITY_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_ARITY_MISMATCH.example = vitte check path/to/file.vit

RUNTIME_E_ARGUMENT_MISMATCH.summary = argument incompatibilite.
RUNTIME_E_ARGUMENT_MISMATCH.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_ARGUMENT_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_ARGUMENT_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_ARGUMENT_MISMATCH.example = vitte check path/to/file.vit

RUNTIME_E_ASSIGNMENT_MISMATCH.summary = affectation incompatibilite.
RUNTIME_E_ASSIGNMENT_MISMATCH.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_ASSIGNMENT_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_ASSIGNMENT_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_ASSIGNMENT_MISMATCH.example = vitte check path/to/file.vit

RUNTIME_E_BRANCH_MISMATCH.summary = branch incompatibilite.
RUNTIME_E_BRANCH_MISMATCH.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_BRANCH_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_BRANCH_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_BRANCH_MISMATCH.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_CALL.summary = appel invalide.
RUNTIME_E_INVALID_CALL.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_CALL.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_CALL.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_CALL.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_CAST.summary = conversion invalide.
RUNTIME_E_INVALID_CAST.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_CAST.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_CAST.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_CAST.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_INDEX.summary = index invalide.
RUNTIME_E_INVALID_INDEX.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_INDEX.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_INDEX.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_INDEX.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_DEREF.summary = deref invalide.
RUNTIME_E_INVALID_DEREF.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_DEREF.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_DEREF.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_DEREF.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_BORROW.summary = emprunt invalide.
RUNTIME_E_INVALID_BORROW.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_BORROW.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_BORROW.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_BORROW.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_MOVE.summary = deplacement invalide.
RUNTIME_E_INVALID_MOVE.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_MOVE.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_MOVE.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_MOVE.example = vitte check path/to/file.vit

RUNTIME_E_USE_AFTER_MOVE.summary = use after deplacement.
RUNTIME_E_USE_AFTER_MOVE.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_USE_AFTER_MOVE.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_USE_AFTER_MOVE.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_USE_AFTER_MOVE.example = vitte check path/to/file.vit

RUNTIME_E_USE_AFTER_DROP.summary = use after destruction.
RUNTIME_E_USE_AFTER_DROP.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_USE_AFTER_DROP.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_USE_AFTER_DROP.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_USE_AFTER_DROP.example = vitte check path/to/file.vit

RUNTIME_E_USE_BEFORE_INIT.summary = use before init.
RUNTIME_E_USE_BEFORE_INIT.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_USE_BEFORE_INIT.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_USE_BEFORE_INIT.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_USE_BEFORE_INIT.example = vitte check path/to/file.vit

RUNTIME_E_DOUBLE_DROP.summary = double destruction.
RUNTIME_E_DOUBLE_DROP.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_DOUBLE_DROP.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_DOUBLE_DROP.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_DOUBLE_DROP.example = vitte check path/to/file.vit

RUNTIME_E_BORROW_CONFLICT.summary = emprunt conflit.
RUNTIME_E_BORROW_CONFLICT.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_BORROW_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_BORROW_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_BORROW_CONFLICT.example = vitte check path/to/file.vit

RUNTIME_E_MUTABILITY_CONFLICT.summary = mutability conflit.
RUNTIME_E_MUTABILITY_CONFLICT.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_MUTABILITY_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_MUTABILITY_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_MUTABILITY_CONFLICT.example = vitte check path/to/file.vit

RUNTIME_E_LIFETIME_TOO_SHORT.summary = duree de vie trop short.
RUNTIME_E_LIFETIME_TOO_SHORT.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_LIFETIME_TOO_SHORT.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_LIFETIME_TOO_SHORT.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_LIFETIME_TOO_SHORT.example = vitte check path/to/file.vit

RUNTIME_E_DANGLING_REFERENCE.summary = dangling reference.
RUNTIME_E_DANGLING_REFERENCE.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_DANGLING_REFERENCE.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_DANGLING_REFERENCE.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_DANGLING_REFERENCE.example = vitte check path/to/file.vit

RUNTIME_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
RUNTIME_E_NON_EXHAUSTIVE_MATCH.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_NON_EXHAUSTIVE_MATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_NON_EXHAUSTIVE_MATCH.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_NON_EXHAUSTIVE_MATCH.example = vitte check path/to/file.vit

RUNTIME_E_UNREACHABLE_PATTERN.summary = inatteignable motif.
RUNTIME_E_UNREACHABLE_PATTERN.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_UNREACHABLE_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_UNREACHABLE_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_UNREACHABLE_PATTERN.example = vitte check path/to/file.vit

RUNTIME_E_CONST_REQUIRED.summary = const required.
RUNTIME_E_CONST_REQUIRED.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_CONST_REQUIRED.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_CONST_REQUIRED.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_CONST_REQUIRED.example = vitte check path/to/file.vit

RUNTIME_E_CONST_OVERFLOW.summary = const depassement.
RUNTIME_E_CONST_OVERFLOW.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_CONST_OVERFLOW.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_CONST_OVERFLOW.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_CONST_OVERFLOW.example = vitte check path/to/file.vit

RUNTIME_E_CONST_DIVISION_BY_ZERO.summary = const division by zero.
RUNTIME_E_CONST_DIVISION_BY_ZERO.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_CONST_DIVISION_BY_ZERO.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_CONST_DIVISION_BY_ZERO.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_CONST_DIVISION_BY_ZERO.example = vitte check path/to/file.vit

RUNTIME_E_CONST_CYCLE.summary = const cycle.
RUNTIME_E_CONST_CYCLE.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_CONST_CYCLE.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_CONST_CYCLE.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_CONST_CYCLE.example = vitte check path/to/file.vit

RUNTIME_E_MACRO_NOT_FOUND.summary = macro manquant.
RUNTIME_E_MACRO_NOT_FOUND.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_MACRO_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_MACRO_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_MACRO_NOT_FOUND.example = vitte check path/to/file.vit

RUNTIME_E_MACRO_RECURSION.summary = macro recursion.
RUNTIME_E_MACRO_RECURSION.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_MACRO_RECURSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_MACRO_RECURSION.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_MACRO_RECURSION.example = vitte check path/to/file.vit

RUNTIME_E_MACRO_EXPANSION_FAILED.summary = macro expansion echec.
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

RUNTIME_E_GENERIC_ARGUMENT_MISSING.summary = generique argument manquant.
RUNTIME_E_GENERIC_ARGUMENT_MISSING.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_GENERIC_ARGUMENT_MISSING.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_GENERIC_ARGUMENT_MISSING.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_GENERIC_ARGUMENT_MISSING.example = vitte check path/to/file.vit

RUNTIME_E_GENERIC_BOUND_FAILED.summary = generique bound echec.
RUNTIME_E_GENERIC_BOUND_FAILED.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_GENERIC_BOUND_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_GENERIC_BOUND_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_GENERIC_BOUND_FAILED.example = vitte check path/to/file.vit

RUNTIME_E_UNSUPPORTED_TARGET.summary = cible non pris en charge.
RUNTIME_E_UNSUPPORTED_TARGET.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_UNSUPPORTED_TARGET.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_UNSUPPORTED_TARGET.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_UNSUPPORTED_TARGET.example = vitte check path/to/file.vit

RUNTIME_E_ABI_MISMATCH.summary = abi incompatibilite.
RUNTIME_E_ABI_MISMATCH.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_ABI_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_ABI_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_ABI_MISMATCH.example = vitte check path/to/file.vit

RUNTIME_E_LINK_FAILED.summary = link echec.
RUNTIME_E_LINK_FAILED.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_LINK_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_LINK_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_LINK_FAILED.example = vitte check path/to/file.vit

RUNTIME_E_RUNTIME_PANIC.summary = runtime panique.
RUNTIME_E_RUNTIME_PANIC.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_RUNTIME_PANIC.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_RUNTIME_PANIC.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_RUNTIME_PANIC.example = vitte check path/to/file.vit

DRIVER_E_EXPECTED_IDENTIFIER.summary = identifiant attendu.
DRIVER_E_EXPECTED_IDENTIFIER.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_EXPECTED_IDENTIFIER.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_EXPECTED_IDENTIFIER.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_EXPECTED_IDENTIFIER.example = vitte check src/main.vit --lang en

DRIVER_E_EXPECTED_EXPRESSION.summary = expression attendu.
DRIVER_E_EXPECTED_EXPRESSION.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_EXPECTED_EXPRESSION.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_EXPECTED_EXPRESSION.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_EXPECTED_EXPRESSION.example = vitte check src/main.vit --lang en

DRIVER_E_EXPECTED_TYPE.summary = type attendu.
DRIVER_E_EXPECTED_TYPE.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_EXPECTED_TYPE.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_EXPECTED_TYPE.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_EXPECTED_TYPE.example = vitte check src/main.vit --lang en

DRIVER_E_EXPECTED_PATTERN.summary = motif attendu.
DRIVER_E_EXPECTED_PATTERN.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_EXPECTED_PATTERN.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_EXPECTED_PATTERN.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_EXPECTED_PATTERN.example = vitte check src/main.vit --lang en

DRIVER_E_EXPECTED_BLOCK.summary = bloc attendu.
DRIVER_E_EXPECTED_BLOCK.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_EXPECTED_BLOCK.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_EXPECTED_BLOCK.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_EXPECTED_BLOCK.example = vitte check src/main.vit --lang en

DRIVER_E_EXPECTED_DELIMITER.summary = delimiteur attendu.
DRIVER_E_EXPECTED_DELIMITER.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_EXPECTED_DELIMITER.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_EXPECTED_DELIMITER.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_EXPECTED_DELIMITER.example = vitte check src/main.vit --lang en

DRIVER_E_UNEXPECTED_TOKEN.summary = jeton inattendu.
DRIVER_E_UNEXPECTED_TOKEN.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_UNEXPECTED_TOKEN.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_UNEXPECTED_TOKEN.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_UNEXPECTED_TOKEN.example = vitte check src/main.vit --lang en

DRIVER_E_UNBALANCED_DELIMITER.summary = unbalanced delimiteur.
DRIVER_E_UNBALANCED_DELIMITER.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_UNBALANCED_DELIMITER.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_UNBALANCED_DELIMITER.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_UNBALANCED_DELIMITER.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_ATTRIBUTE.summary = attribute invalide.
DRIVER_E_INVALID_ATTRIBUTE.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_ATTRIBUTE.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_ATTRIBUTE.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_ATTRIBUTE.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_DECLARATION.summary = declaration invalide.
DRIVER_E_INVALID_DECLARATION.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_DECLARATION.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_DECLARATION.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_DECLARATION.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_STATEMENT.summary = instruction invalide.
DRIVER_E_INVALID_STATEMENT.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_STATEMENT.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_STATEMENT.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_STATEMENT.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_EXPRESSION.summary = expression invalide.
DRIVER_E_INVALID_EXPRESSION.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_EXPRESSION.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_EXPRESSION.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_EXPRESSION.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_PATTERN.summary = motif invalide.
DRIVER_E_INVALID_PATTERN.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_PATTERN.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_PATTERN.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_PATTERN.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_LITERAL.summary = litteral invalide.
DRIVER_E_INVALID_LITERAL.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_LITERAL.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_LITERAL.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_LITERAL.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_OPERATOR.summary = operateur invalide.
DRIVER_E_INVALID_OPERATOR.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_OPERATOR.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_OPERATOR.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_OPERATOR.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_MODIFIER.summary = modifier invalide.
DRIVER_E_INVALID_MODIFIER.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_MODIFIER.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_MODIFIER.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_MODIFIER.example = vitte check src/main.vit --lang en

DRIVER_E_MISSING_BODY.summary = body manquant.
DRIVER_E_MISSING_BODY.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_MISSING_BODY.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_MISSING_BODY.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_MISSING_BODY.example = vitte check src/main.vit --lang en

DRIVER_E_MISSING_RETURN.summary = retour manquant.
DRIVER_E_MISSING_RETURN.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_MISSING_RETURN.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_MISSING_RETURN.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_MISSING_RETURN.example = vitte check src/main.vit --lang en

DRIVER_E_DUPLICATE_NAME.summary = nom duplique.
DRIVER_E_DUPLICATE_NAME.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_DUPLICATE_NAME.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_DUPLICATE_NAME.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_DUPLICATE_NAME.example = vitte check src/main.vit --lang en

DRIVER_E_UNKNOWN_NAME.summary = nom inconnu.
DRIVER_E_UNKNOWN_NAME.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_UNKNOWN_NAME.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_UNKNOWN_NAME.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_UNKNOWN_NAME.example = vitte check src/main.vit --lang en

DRIVER_E_UNKNOWN_TYPE.summary = type inconnu.
DRIVER_E_UNKNOWN_TYPE.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_UNKNOWN_TYPE.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_UNKNOWN_TYPE.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_UNKNOWN_TYPE.example = vitte check src/main.vit --lang en

DRIVER_E_UNKNOWN_MODULE.summary = module inconnu.
DRIVER_E_UNKNOWN_MODULE.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_UNKNOWN_MODULE.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_UNKNOWN_MODULE.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_UNKNOWN_MODULE.example = vitte check src/main.vit --lang en

DRIVER_E_UNKNOWN_MEMBER.summary = membre inconnu.
DRIVER_E_UNKNOWN_MEMBER.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_UNKNOWN_MEMBER.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_UNKNOWN_MEMBER.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_UNKNOWN_MEMBER.example = vitte check src/main.vit --lang en

DRIVER_E_AMBIGUOUS_NAME.summary = ambiguous nom.
DRIVER_E_AMBIGUOUS_NAME.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_AMBIGUOUS_NAME.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_AMBIGUOUS_NAME.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_AMBIGUOUS_NAME.example = vitte check src/main.vit --lang en

DRIVER_E_PRIVATE_SYMBOL.summary = private symbole.
DRIVER_E_PRIVATE_SYMBOL.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_PRIVATE_SYMBOL.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_PRIVATE_SYMBOL.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_PRIVATE_SYMBOL.example = vitte check src/main.vit --lang en

DRIVER_E_IMPORT_NOT_FOUND.summary = import manquant.
DRIVER_E_IMPORT_NOT_FOUND.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_IMPORT_NOT_FOUND.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_IMPORT_NOT_FOUND.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_IMPORT_NOT_FOUND.example = vitte check src/main.vit --lang en

DRIVER_E_IMPORT_CYCLE.summary = import cycle.
DRIVER_E_IMPORT_CYCLE.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_IMPORT_CYCLE.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_IMPORT_CYCLE.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_IMPORT_CYCLE.example = vitte check src/main.vit --lang en

DRIVER_E_EXPORT_CONFLICT.summary = export conflit.
DRIVER_E_EXPORT_CONFLICT.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_EXPORT_CONFLICT.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_EXPORT_CONFLICT.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_EXPORT_CONFLICT.example = vitte check src/main.vit --lang en

DRIVER_E_ARITY_MISMATCH.summary = arity incompatibilite.
DRIVER_E_ARITY_MISMATCH.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_ARITY_MISMATCH.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_ARITY_MISMATCH.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_ARITY_MISMATCH.example = vitte check src/main.vit --lang en

DRIVER_E_ARGUMENT_MISMATCH.summary = argument incompatibilite.
DRIVER_E_ARGUMENT_MISMATCH.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_ARGUMENT_MISMATCH.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_ARGUMENT_MISMATCH.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_ARGUMENT_MISMATCH.example = vitte check src/main.vit --lang en

DRIVER_E_ASSIGNMENT_MISMATCH.summary = affectation incompatibilite.
DRIVER_E_ASSIGNMENT_MISMATCH.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_ASSIGNMENT_MISMATCH.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_ASSIGNMENT_MISMATCH.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_ASSIGNMENT_MISMATCH.example = vitte check src/main.vit --lang en

DRIVER_E_BRANCH_MISMATCH.summary = branch incompatibilite.
DRIVER_E_BRANCH_MISMATCH.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_BRANCH_MISMATCH.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_BRANCH_MISMATCH.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_BRANCH_MISMATCH.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_CALL.summary = appel invalide.
DRIVER_E_INVALID_CALL.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_CALL.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_CALL.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_CALL.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_CAST.summary = conversion invalide.
DRIVER_E_INVALID_CAST.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_CAST.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_CAST.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_CAST.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_INDEX.summary = index invalide.
DRIVER_E_INVALID_INDEX.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_INDEX.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_INDEX.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_INDEX.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_DEREF.summary = deref invalide.
DRIVER_E_INVALID_DEREF.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_DEREF.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_DEREF.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_DEREF.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_BORROW.summary = emprunt invalide.
DRIVER_E_INVALID_BORROW.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_BORROW.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_BORROW.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_BORROW.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_MOVE.summary = deplacement invalide.
DRIVER_E_INVALID_MOVE.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_MOVE.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_MOVE.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_MOVE.example = vitte check src/main.vit --lang en

DRIVER_E_USE_AFTER_MOVE.summary = use after deplacement.
DRIVER_E_USE_AFTER_MOVE.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_USE_AFTER_MOVE.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_USE_AFTER_MOVE.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_USE_AFTER_MOVE.example = vitte check src/main.vit --lang en

DRIVER_E_USE_AFTER_DROP.summary = use after destruction.
DRIVER_E_USE_AFTER_DROP.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_USE_AFTER_DROP.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_USE_AFTER_DROP.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_USE_AFTER_DROP.example = vitte check src/main.vit --lang en

DRIVER_E_USE_BEFORE_INIT.summary = use before init.
DRIVER_E_USE_BEFORE_INIT.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_USE_BEFORE_INIT.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_USE_BEFORE_INIT.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_USE_BEFORE_INIT.example = vitte check src/main.vit --lang en

DRIVER_E_DOUBLE_DROP.summary = double destruction.
DRIVER_E_DOUBLE_DROP.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_DOUBLE_DROP.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_DOUBLE_DROP.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_DOUBLE_DROP.example = vitte check src/main.vit --lang en

DRIVER_E_BORROW_CONFLICT.summary = emprunt conflit.
DRIVER_E_BORROW_CONFLICT.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_BORROW_CONFLICT.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_BORROW_CONFLICT.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_BORROW_CONFLICT.example = vitte check src/main.vit --lang en

DRIVER_E_MUTABILITY_CONFLICT.summary = mutability conflit.
DRIVER_E_MUTABILITY_CONFLICT.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_MUTABILITY_CONFLICT.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_MUTABILITY_CONFLICT.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_MUTABILITY_CONFLICT.example = vitte check src/main.vit --lang en

DRIVER_E_LIFETIME_TOO_SHORT.summary = duree de vie trop short.
DRIVER_E_LIFETIME_TOO_SHORT.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_LIFETIME_TOO_SHORT.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_LIFETIME_TOO_SHORT.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_LIFETIME_TOO_SHORT.example = vitte check src/main.vit --lang en

DRIVER_E_DANGLING_REFERENCE.summary = dangling reference.
DRIVER_E_DANGLING_REFERENCE.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_DANGLING_REFERENCE.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_DANGLING_REFERENCE.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_DANGLING_REFERENCE.example = vitte check src/main.vit --lang en

DRIVER_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
DRIVER_E_NON_EXHAUSTIVE_MATCH.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_NON_EXHAUSTIVE_MATCH.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_NON_EXHAUSTIVE_MATCH.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_NON_EXHAUSTIVE_MATCH.example = vitte check src/main.vit --lang en

DRIVER_E_UNREACHABLE_PATTERN.summary = inatteignable motif.
DRIVER_E_UNREACHABLE_PATTERN.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_UNREACHABLE_PATTERN.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_UNREACHABLE_PATTERN.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_UNREACHABLE_PATTERN.example = vitte check src/main.vit --lang en

DRIVER_E_CONST_REQUIRED.summary = const required.
DRIVER_E_CONST_REQUIRED.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_CONST_REQUIRED.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_CONST_REQUIRED.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_CONST_REQUIRED.example = vitte check src/main.vit --lang en

DRIVER_E_CONST_OVERFLOW.summary = const depassement.
DRIVER_E_CONST_OVERFLOW.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_CONST_OVERFLOW.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_CONST_OVERFLOW.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_CONST_OVERFLOW.example = vitte check src/main.vit --lang en

DRIVER_E_CONST_DIVISION_BY_ZERO.summary = const division by zero.
DRIVER_E_CONST_DIVISION_BY_ZERO.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_CONST_DIVISION_BY_ZERO.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_CONST_DIVISION_BY_ZERO.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_CONST_DIVISION_BY_ZERO.example = vitte check src/main.vit --lang en

DRIVER_E_CONST_CYCLE.summary = const cycle.
DRIVER_E_CONST_CYCLE.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_CONST_CYCLE.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_CONST_CYCLE.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_CONST_CYCLE.example = vitte check src/main.vit --lang en

DRIVER_E_MACRO_NOT_FOUND.summary = macro manquant.
DRIVER_E_MACRO_NOT_FOUND.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_MACRO_NOT_FOUND.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_MACRO_NOT_FOUND.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_MACRO_NOT_FOUND.example = vitte check src/main.vit --lang en

DRIVER_E_MACRO_RECURSION.summary = macro recursion.
DRIVER_E_MACRO_RECURSION.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_MACRO_RECURSION.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_MACRO_RECURSION.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_MACRO_RECURSION.example = vitte check src/main.vit --lang en

DRIVER_E_MACRO_EXPANSION_FAILED.summary = macro expansion echec.
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

DRIVER_E_GENERIC_ARGUMENT_MISSING.summary = generique argument manquant.
DRIVER_E_GENERIC_ARGUMENT_MISSING.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_GENERIC_ARGUMENT_MISSING.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_GENERIC_ARGUMENT_MISSING.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_GENERIC_ARGUMENT_MISSING.example = vitte check src/main.vit --lang en

DRIVER_E_GENERIC_BOUND_FAILED.summary = generique bound echec.
DRIVER_E_GENERIC_BOUND_FAILED.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_GENERIC_BOUND_FAILED.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_GENERIC_BOUND_FAILED.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_GENERIC_BOUND_FAILED.example = vitte check src/main.vit --lang en

DRIVER_E_UNSUPPORTED_TARGET.summary = cible non pris en charge.
DRIVER_E_UNSUPPORTED_TARGET.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_UNSUPPORTED_TARGET.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_UNSUPPORTED_TARGET.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_UNSUPPORTED_TARGET.example = vitte check src/main.vit --lang en

DRIVER_E_ABI_MISMATCH.summary = abi incompatibilite.
DRIVER_E_ABI_MISMATCH.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_ABI_MISMATCH.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_ABI_MISMATCH.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_ABI_MISMATCH.example = vitte check src/main.vit --lang en

DRIVER_E_LINK_FAILED.summary = link echec.
DRIVER_E_LINK_FAILED.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_LINK_FAILED.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_LINK_FAILED.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_LINK_FAILED.example = vitte check src/main.vit --lang en

DRIVER_E_RUNTIME_PANIC.summary = runtime panique.
DRIVER_E_RUNTIME_PANIC.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_RUNTIME_PANIC.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_RUNTIME_PANIC.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_RUNTIME_PANIC.example = vitte check src/main.vit --lang en

LIMIT_E_EXPECTED_IDENTIFIER.summary = identifiant attendu.
LIMIT_E_EXPECTED_IDENTIFIER.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_EXPECTED_IDENTIFIER.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_EXPECTED_IDENTIFIER.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_EXPECTED_IDENTIFIER.example = vitte check src/main.vit

LIMIT_E_EXPECTED_EXPRESSION.summary = expression attendu.
LIMIT_E_EXPECTED_EXPRESSION.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_EXPECTED_EXPRESSION.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_EXPECTED_EXPRESSION.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_EXPECTED_EXPRESSION.example = vitte check src/main.vit

LIMIT_E_EXPECTED_TYPE.summary = type attendu.
LIMIT_E_EXPECTED_TYPE.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_EXPECTED_TYPE.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_EXPECTED_TYPE.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_EXPECTED_TYPE.example = vitte check src/main.vit

LIMIT_E_EXPECTED_PATTERN.summary = motif attendu.
LIMIT_E_EXPECTED_PATTERN.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_EXPECTED_PATTERN.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_EXPECTED_PATTERN.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_EXPECTED_PATTERN.example = vitte check src/main.vit

LIMIT_E_EXPECTED_BLOCK.summary = bloc attendu.
LIMIT_E_EXPECTED_BLOCK.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_EXPECTED_BLOCK.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_EXPECTED_BLOCK.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_EXPECTED_BLOCK.example = vitte check src/main.vit

LIMIT_E_EXPECTED_DELIMITER.summary = delimiteur attendu.
LIMIT_E_EXPECTED_DELIMITER.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_EXPECTED_DELIMITER.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_EXPECTED_DELIMITER.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_EXPECTED_DELIMITER.example = vitte check src/main.vit

LIMIT_E_UNEXPECTED_TOKEN.summary = jeton inattendu.
LIMIT_E_UNEXPECTED_TOKEN.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_UNEXPECTED_TOKEN.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_UNEXPECTED_TOKEN.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_UNEXPECTED_TOKEN.example = vitte check src/main.vit

LIMIT_E_UNBALANCED_DELIMITER.summary = unbalanced delimiteur.
LIMIT_E_UNBALANCED_DELIMITER.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_UNBALANCED_DELIMITER.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_UNBALANCED_DELIMITER.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_UNBALANCED_DELIMITER.example = vitte check src/main.vit

LIMIT_E_INVALID_ATTRIBUTE.summary = attribute invalide.
LIMIT_E_INVALID_ATTRIBUTE.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_ATTRIBUTE.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_ATTRIBUTE.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_ATTRIBUTE.example = vitte check src/main.vit

LIMIT_E_INVALID_DECLARATION.summary = declaration invalide.
LIMIT_E_INVALID_DECLARATION.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_DECLARATION.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_DECLARATION.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_DECLARATION.example = vitte check src/main.vit

LIMIT_E_INVALID_STATEMENT.summary = instruction invalide.
LIMIT_E_INVALID_STATEMENT.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_STATEMENT.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_STATEMENT.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_STATEMENT.example = vitte check src/main.vit

LIMIT_E_INVALID_EXPRESSION.summary = expression invalide.
LIMIT_E_INVALID_EXPRESSION.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_EXPRESSION.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_EXPRESSION.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_EXPRESSION.example = vitte check src/main.vit

LIMIT_E_INVALID_PATTERN.summary = motif invalide.
LIMIT_E_INVALID_PATTERN.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_PATTERN.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_PATTERN.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_PATTERN.example = vitte check src/main.vit

LIMIT_E_INVALID_LITERAL.summary = litteral invalide.
LIMIT_E_INVALID_LITERAL.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_LITERAL.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_LITERAL.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_LITERAL.example = vitte check src/main.vit

LIMIT_E_INVALID_OPERATOR.summary = operateur invalide.
LIMIT_E_INVALID_OPERATOR.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_OPERATOR.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_OPERATOR.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_OPERATOR.example = vitte check src/main.vit

LIMIT_E_INVALID_MODIFIER.summary = modifier invalide.
LIMIT_E_INVALID_MODIFIER.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_MODIFIER.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_MODIFIER.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_MODIFIER.example = vitte check src/main.vit

LIMIT_E_MISSING_BODY.summary = body manquant.
LIMIT_E_MISSING_BODY.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_MISSING_BODY.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_MISSING_BODY.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_MISSING_BODY.example = vitte check src/main.vit

LIMIT_E_MISSING_RETURN.summary = retour manquant.
LIMIT_E_MISSING_RETURN.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_MISSING_RETURN.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_MISSING_RETURN.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_MISSING_RETURN.example = vitte check src/main.vit

LIMIT_E_DUPLICATE_NAME.summary = nom duplique.
LIMIT_E_DUPLICATE_NAME.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_DUPLICATE_NAME.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_DUPLICATE_NAME.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_DUPLICATE_NAME.example = vitte check src/main.vit

LIMIT_E_UNKNOWN_NAME.summary = nom inconnu.
LIMIT_E_UNKNOWN_NAME.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_UNKNOWN_NAME.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_UNKNOWN_NAME.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_UNKNOWN_NAME.example = vitte check src/main.vit

LIMIT_E_UNKNOWN_TYPE.summary = type inconnu.
LIMIT_E_UNKNOWN_TYPE.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_UNKNOWN_TYPE.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_UNKNOWN_TYPE.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_UNKNOWN_TYPE.example = vitte check src/main.vit

LIMIT_E_UNKNOWN_MODULE.summary = module inconnu.
LIMIT_E_UNKNOWN_MODULE.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_UNKNOWN_MODULE.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_UNKNOWN_MODULE.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_UNKNOWN_MODULE.example = vitte check src/main.vit

LIMIT_E_UNKNOWN_MEMBER.summary = membre inconnu.
LIMIT_E_UNKNOWN_MEMBER.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_UNKNOWN_MEMBER.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_UNKNOWN_MEMBER.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_UNKNOWN_MEMBER.example = vitte check src/main.vit

LIMIT_E_AMBIGUOUS_NAME.summary = ambiguous nom.
LIMIT_E_AMBIGUOUS_NAME.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_AMBIGUOUS_NAME.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_AMBIGUOUS_NAME.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_AMBIGUOUS_NAME.example = vitte check src/main.vit

LIMIT_E_PRIVATE_SYMBOL.summary = private symbole.
LIMIT_E_PRIVATE_SYMBOL.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_PRIVATE_SYMBOL.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_PRIVATE_SYMBOL.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_PRIVATE_SYMBOL.example = vitte check src/main.vit

LIMIT_E_IMPORT_NOT_FOUND.summary = import manquant.
LIMIT_E_IMPORT_NOT_FOUND.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_IMPORT_NOT_FOUND.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_IMPORT_NOT_FOUND.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_IMPORT_NOT_FOUND.example = vitte check src/main.vit

LIMIT_E_IMPORT_CYCLE.summary = import cycle.
LIMIT_E_IMPORT_CYCLE.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_IMPORT_CYCLE.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_IMPORT_CYCLE.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_IMPORT_CYCLE.example = vitte check src/main.vit

LIMIT_E_EXPORT_CONFLICT.summary = export conflit.
LIMIT_E_EXPORT_CONFLICT.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_EXPORT_CONFLICT.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_EXPORT_CONFLICT.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_EXPORT_CONFLICT.example = vitte check src/main.vit

LIMIT_E_ARITY_MISMATCH.summary = arity incompatibilite.
LIMIT_E_ARITY_MISMATCH.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_ARITY_MISMATCH.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_ARITY_MISMATCH.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_ARITY_MISMATCH.example = vitte check src/main.vit

LIMIT_E_ARGUMENT_MISMATCH.summary = argument incompatibilite.
LIMIT_E_ARGUMENT_MISMATCH.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_ARGUMENT_MISMATCH.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_ARGUMENT_MISMATCH.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_ARGUMENT_MISMATCH.example = vitte check src/main.vit

LIMIT_E_ASSIGNMENT_MISMATCH.summary = affectation incompatibilite.
LIMIT_E_ASSIGNMENT_MISMATCH.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_ASSIGNMENT_MISMATCH.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_ASSIGNMENT_MISMATCH.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_ASSIGNMENT_MISMATCH.example = vitte check src/main.vit

LIMIT_E_BRANCH_MISMATCH.summary = branch incompatibilite.
LIMIT_E_BRANCH_MISMATCH.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_BRANCH_MISMATCH.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_BRANCH_MISMATCH.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_BRANCH_MISMATCH.example = vitte check src/main.vit

LIMIT_E_INVALID_CALL.summary = appel invalide.
LIMIT_E_INVALID_CALL.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_CALL.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_CALL.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_CALL.example = vitte check src/main.vit

LIMIT_E_INVALID_CAST.summary = conversion invalide.
LIMIT_E_INVALID_CAST.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_CAST.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_CAST.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_CAST.example = vitte check src/main.vit

LIMIT_E_INVALID_INDEX.summary = index invalide.
LIMIT_E_INVALID_INDEX.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_INDEX.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_INDEX.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_INDEX.example = vitte check src/main.vit

LIMIT_E_INVALID_DEREF.summary = deref invalide.
LIMIT_E_INVALID_DEREF.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_DEREF.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_DEREF.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_DEREF.example = vitte check src/main.vit

LIMIT_E_INVALID_BORROW.summary = emprunt invalide.
LIMIT_E_INVALID_BORROW.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_BORROW.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_BORROW.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_BORROW.example = vitte check src/main.vit

LIMIT_E_INVALID_MOVE.summary = deplacement invalide.
LIMIT_E_INVALID_MOVE.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_MOVE.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_MOVE.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_MOVE.example = vitte check src/main.vit

LIMIT_E_USE_AFTER_MOVE.summary = use after deplacement.
LIMIT_E_USE_AFTER_MOVE.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_USE_AFTER_MOVE.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_USE_AFTER_MOVE.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_USE_AFTER_MOVE.example = vitte check src/main.vit

LIMIT_E_USE_AFTER_DROP.summary = use after destruction.
LIMIT_E_USE_AFTER_DROP.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_USE_AFTER_DROP.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_USE_AFTER_DROP.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_USE_AFTER_DROP.example = vitte check src/main.vit

LIMIT_E_USE_BEFORE_INIT.summary = use before init.
LIMIT_E_USE_BEFORE_INIT.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_USE_BEFORE_INIT.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_USE_BEFORE_INIT.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_USE_BEFORE_INIT.example = vitte check src/main.vit

LIMIT_E_DOUBLE_DROP.summary = double destruction.
LIMIT_E_DOUBLE_DROP.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_DOUBLE_DROP.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_DOUBLE_DROP.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_DOUBLE_DROP.example = vitte check src/main.vit

LIMIT_E_BORROW_CONFLICT.summary = emprunt conflit.
LIMIT_E_BORROW_CONFLICT.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_BORROW_CONFLICT.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_BORROW_CONFLICT.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_BORROW_CONFLICT.example = vitte check src/main.vit

LIMIT_E_MUTABILITY_CONFLICT.summary = mutability conflit.
LIMIT_E_MUTABILITY_CONFLICT.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_MUTABILITY_CONFLICT.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_MUTABILITY_CONFLICT.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_MUTABILITY_CONFLICT.example = vitte check src/main.vit

LIMIT_E_LIFETIME_TOO_SHORT.summary = duree de vie trop short.
LIMIT_E_LIFETIME_TOO_SHORT.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_LIFETIME_TOO_SHORT.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_LIFETIME_TOO_SHORT.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_LIFETIME_TOO_SHORT.example = vitte check src/main.vit

LIMIT_E_DANGLING_REFERENCE.summary = dangling reference.
LIMIT_E_DANGLING_REFERENCE.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_DANGLING_REFERENCE.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_DANGLING_REFERENCE.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_DANGLING_REFERENCE.example = vitte check src/main.vit

LIMIT_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
LIMIT_E_NON_EXHAUSTIVE_MATCH.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_NON_EXHAUSTIVE_MATCH.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_NON_EXHAUSTIVE_MATCH.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_NON_EXHAUSTIVE_MATCH.example = vitte check src/main.vit

LIMIT_E_UNREACHABLE_PATTERN.summary = inatteignable motif.
LIMIT_E_UNREACHABLE_PATTERN.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_UNREACHABLE_PATTERN.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_UNREACHABLE_PATTERN.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_UNREACHABLE_PATTERN.example = vitte check src/main.vit

LIMIT_E_CONST_REQUIRED.summary = const required.
LIMIT_E_CONST_REQUIRED.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_CONST_REQUIRED.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_CONST_REQUIRED.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_CONST_REQUIRED.example = vitte check src/main.vit

LIMIT_E_CONST_OVERFLOW.summary = const depassement.
LIMIT_E_CONST_OVERFLOW.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_CONST_OVERFLOW.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_CONST_OVERFLOW.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_CONST_OVERFLOW.example = vitte check src/main.vit

LIMIT_E_CONST_DIVISION_BY_ZERO.summary = const division by zero.
LIMIT_E_CONST_DIVISION_BY_ZERO.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_CONST_DIVISION_BY_ZERO.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_CONST_DIVISION_BY_ZERO.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_CONST_DIVISION_BY_ZERO.example = vitte check src/main.vit

LIMIT_E_CONST_CYCLE.summary = const cycle.
LIMIT_E_CONST_CYCLE.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_CONST_CYCLE.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_CONST_CYCLE.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_CONST_CYCLE.example = vitte check src/main.vit

LIMIT_E_MACRO_NOT_FOUND.summary = macro manquant.
LIMIT_E_MACRO_NOT_FOUND.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_MACRO_NOT_FOUND.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_MACRO_NOT_FOUND.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_MACRO_NOT_FOUND.example = vitte check src/main.vit

LIMIT_E_MACRO_RECURSION.summary = macro recursion.
LIMIT_E_MACRO_RECURSION.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_MACRO_RECURSION.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_MACRO_RECURSION.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_MACRO_RECURSION.example = vitte check src/main.vit

LIMIT_E_MACRO_EXPANSION_FAILED.summary = macro expansion echec.
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

LIMIT_E_GENERIC_ARGUMENT_MISSING.summary = generique argument manquant.
LIMIT_E_GENERIC_ARGUMENT_MISSING.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_GENERIC_ARGUMENT_MISSING.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_GENERIC_ARGUMENT_MISSING.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_GENERIC_ARGUMENT_MISSING.example = vitte check src/main.vit

LIMIT_E_GENERIC_BOUND_FAILED.summary = generique bound echec.
LIMIT_E_GENERIC_BOUND_FAILED.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_GENERIC_BOUND_FAILED.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_GENERIC_BOUND_FAILED.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_GENERIC_BOUND_FAILED.example = vitte check src/main.vit

LIMIT_E_UNSUPPORTED_TARGET.summary = cible non pris en charge.
LIMIT_E_UNSUPPORTED_TARGET.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_UNSUPPORTED_TARGET.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_UNSUPPORTED_TARGET.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_UNSUPPORTED_TARGET.example = vitte check src/main.vit

LIMIT_E_ABI_MISMATCH.summary = abi incompatibilite.
LIMIT_E_ABI_MISMATCH.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_ABI_MISMATCH.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_ABI_MISMATCH.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_ABI_MISMATCH.example = vitte check src/main.vit

LIMIT_E_LINK_FAILED.summary = link echec.
LIMIT_E_LINK_FAILED.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_LINK_FAILED.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_LINK_FAILED.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_LINK_FAILED.example = vitte check src/main.vit

LIMIT_E_RUNTIME_PANIC.summary = runtime panique.
LIMIT_E_RUNTIME_PANIC.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_RUNTIME_PANIC.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_RUNTIME_PANIC.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_RUNTIME_PANIC.example = vitte check src/main.vit
