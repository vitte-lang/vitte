# Vitte diagnostic explanations (Portuguese (Brazil))
#
# Chaves: CODE.summary, CODE.cause, CODE.step1, CODE.fix, CODE.example

E0001.summary = The parser expected a nome for something (variable, tipo, modulo, etc.).
E0001.cause = The parser or lexer could not form the next valid source construct.
E0001.step1 = Look at the highlighted token and complete or remove the construct around it.
E0001.fix = complete the syntax shape named by the parser label at the highlighted token
E0001.example = proc main() -> i32 { return 0 }

E0002.summary = The parser expected an expressao at this location.
E0002.cause = The parser or lexer could not form the next valid source construct.
E0002.step1 = Look at the highlighted token and complete or remove the construct around it.
E0002.fix = complete the syntax shape named by the parser label at the highlighted token
E0002.example = proc main() -> i32 { return 0 }

E0003.summary = The parser expected a padrao.
E0003.cause = The parser or lexer could not form the next valid source construct.
E0003.step1 = Look at the highlighted token and complete or remove the construct around it.
E0003.fix = complete the syntax shape named by the parser label at the highlighted token
E0003.example = when x is Option.Some { return 0 }

E0004.summary = The parser expected a tipo nome.
E0004.cause = The parser or lexer could not form the next valid source construct.
E0004.step1 = Look at the highlighted token and complete or remove the construct around it.
E0004.fix = complete the syntax shape named by the parser label at the highlighted token
E0004.example = proc id(x: int) -> int { return x }

E0005.summary = A bloco was opened but not closed with 'end' or '.end'.
E0005.cause = The parser or lexer could not form the next valid source construct.
E0005.step1 = Look at the highlighted token and complete or remove the construct around it.
E0005.fix = complete the syntax shape named by the parser label at the highlighted token
E0005.example = form Point\n  field x as int\n.end

E0006.summary = An attribute deve ser followed by a proc declaracao.
E0006.cause = The parser or lexer could not form the next valid source construct.
E0006.step1 = Look at the highlighted token and complete or remove the construct around it.
E0006.fix = complete the syntax shape named by the parser label at the highlighted token
E0006.example = #[inline]\nproc add(a: int, b: int) -> int { return a + b }

E0007.summary = The parser expected a top-level declaracao.
E0007.cause = The parser or lexer could not form the next valid source construct.
E0007.step1 = Look at the highlighted token and complete or remove the construct around it.
E0007.fix = complete the syntax shape named by the parser label at the highlighted token
E0007.example = space my/app\nproc main() -> int { return 0 }

E0008.summary = A padrao bound the same nome more than once.
E0008.cause = The parser or lexer could not form the next valid source construct.
E0008.step1 = Look at the highlighted token and complete or remove the construct around it.
E0008.fix = complete the syntax shape named by the parser label at the highlighted token
E0008.example = when Pair(x, y) { return 0 }

E0009.summary = A referenced tipo nome was ausente.
E0009.cause = The parser or lexer could not form the next valid source construct.
E0009.step1 = Look at the highlighted token and complete or remove the construct around it.
E0009.fix = complete the syntax shape named by the parser label at the highlighted token
E0009.example = use std/core/option.Option\nproc f(x: Option[int]) -> int { return 0 }

E0010.summary = The base tipo of a generico was ausente.
E0010.cause = The parser or lexer could not form the next valid source construct.
E0010.step1 = Look at the highlighted token and complete or remove the construct around it.
E0010.fix = complete the syntax shape named by the parser label at the highlighted token
E0010.example = use std/core/option.Option\nlet x: Option[int] = Option.None

E0011.summary = A generico tipo must include at least one argumento.
E0011.cause = The parser or lexer could not form the next valid source construct.
E0011.step1 = Look at the highlighted token and complete or remove the construct around it.
E0011.fix = complete the syntax shape named by the parser label at the highlighted token
E0011.example = let x: Option[int] = Option.None

E0012.summary = This tipo form is not supported yet.
E0012.cause = The parser or lexer could not form the next valid source construct.
E0012.step1 = Look at the highlighted token and complete or remove the construct around it.
E0012.fix = complete the syntax shape named by the parser label at the highlighted token
E0012.example = let p: *int = &value

E0013.summary = A procedure with an explicit retorno tipo has a caminho that reaches the end without returning a valor.
E0013.cause = The parser or lexer could not form the next valid source construct.
E0013.step1 = Look at the highlighted token and complete or remove the construct around it.
E0013.fix = complete the syntax shape named by the parser label at the highlighted token
E0013.example = proc to_code(ok: bool) -> int {\n  if ok { give 0 }\n  give 1\n}

E0014.summary = An invocation is ausente its destino da chamada.
E0014.cause = The parser or lexer could not form the next valid source construct.
E0014.step1 = Look at the highlighted token and complete or remove the construct around it.
E0014.fix = complete the syntax shape named by the parser label at the highlighted token
E0014.example = print(\"hi\")

E0015.summary = This expressao is not supported by the HIR lowering yet.
E0015.cause = The parser or lexer could not form the next valid source construct.
E0015.step1 = Look at the highlighted token and complete or remove the construct around it.
E0015.fix = complete the syntax shape named by the parser label at the highlighted token
E0015.example = let x = value

E0016.summary = This padrao is not supported by the HIR lowering yet.
E0016.cause = The parser or lexer could not form the next valid source construct.
E0016.step1 = Look at the highlighted token and complete or remove the construct around it.
E0016.fix = complete the syntax shape named by the parser label at the highlighted token
E0016.example = when x is Option.Some { return 0 }

E0017.summary = This instrucao is not supported by the HIR lowering yet.
E0017.cause = The parser or lexer could not form the next valid source construct.
E0017.step1 = Look at the highlighted token and complete or remove the construct around it.
E0017.fix = complete the syntax shape named by the parser label at the highlighted token
E0017.example = return 0

E0018.summary = An extern procedure cannot define a body.
E0018.cause = The parser or lexer could not form the next valid source construct.
E0018.step1 = Look at the highlighted token and complete or remove the construct around it.
E0018.fix = complete the syntax shape named by the parser label at the highlighted token
E0018.example = #[extern]\nproc puts(s: string) -> int

E1001.summary = padrao vinculo duplicado.
E1001.cause = The general phase found code that violates this diagnostic rule.
E1001.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1001.fix = repair the highlighted compiler contract before checking later diagnostics
E1001.example = vitte check path/to/file.vit

E1002.summary = tipo (did you mean a built-in like int/i32/i64/i128/u32/u64/u128/bool/string?) desconhecido.
E1002.cause = The general phase found code that violates this diagnostic rule.
E1002.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1002.fix = repair the highlighted compiler contract before checking later diagnostics
E1002.example = vitte check path/to/file.vit

E1003.summary = generico base tipo desconhecido.
E1003.cause = The general phase found code that violates this diagnostic rule.
E1003.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1003.fix = repair the highlighted compiler contract before checking later diagnostics
E1003.example = vitte check path/to/file.vit

E1004.summary = generico tipo requer pelo menos um argumento.
E1004.cause = The general phase found code that violates this diagnostic rule.
E1004.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1004.fix = repair the highlighted compiler contract before checking later diagnostics
E1004.example = vitte check path/to/file.vit

E1005.summary = identificador desconhecido.
E1005.cause = The general phase found code that violates this diagnostic rule.
E1005.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1005.fix = repair the highlighted compiler contract before checking later diagnostics
E1005.example = vitte check path/to/file.vit

E1006.summary = generico tipo requer pelo menos um tipo argumento.
E1006.cause = The general phase found code that violates this diagnostic rule.
E1006.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1006.fix = repair the highlighted compiler contract before checking later diagnostics
E1006.example = vitte check path/to/file.vit

E1007.summary = conversao between signed and unsigned values invalido.
E1007.cause = The general phase found code that violates this diagnostic rule.
E1007.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1007.fix = repair the highlighted compiler contract before checking later diagnostics
E1007.example = vitte check path/to/file.vit

E1010.summary = stdlib modulo negado por active stdlib perfil.
E1010.cause = The general phase found code that violates this diagnostic rule.
E1010.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1010.fix = repair the highlighted compiler contract before checking later diagnostics
E1010.example = vitte check path/to/file.vit

E1011.summary = strict-imports requires explicit alias.
E1011.cause = The general phase found code that violates this diagnostic rule.
E1011.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1011.fix = repair the highlighted compiler contract before checking later diagnostics
E1011.example = vitte check path/to/file.vit

E1012.summary = strict-imports proibe unused importacao aliases.
E1012.cause = The general phase found code that violates this diagnostic rule.
E1012.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1012.fix = repair the highlighted compiler contract before checking later diagnostics
E1012.example = vitte check path/to/file.vit

E1013.summary = strict-imports proibe non-canonical importacao paths.
E1013.cause = The general phase found code that violates this diagnostic rule.
E1013.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1013.fix = repair the highlighted compiler contract before checking later diagnostics
E1013.example = vitte check path/to/file.vit

E1014.summary = stdlib modulo ausente.
E1014.cause = The general phase found code that violates this diagnostic rule.
E1014.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1014.fix = repair the highlighted compiler contract before checking later diagnostics
E1014.example = vitte check path/to/file.vit

E1015.summary = experimental modulo importacao denied.
E1015.cause = The general phase found code that violates this diagnostic rule.
E1015.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1015.fix = repair the highlighted compiler contract before checking later diagnostics
E1015.example = vitte check path/to/file.vit

E1016.summary = interno modulo importacao denied.
E1016.cause = The general phase found code that violates this diagnostic rule.
E1016.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1016.fix = repair the highlighted compiler contract before checking later diagnostics
E1016.example = vitte check path/to/file.vit

E1017.summary = re-export simbolo conflito.
E1017.cause = The general phase found code that violates this diagnostic rule.
E1017.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1017.fix = repair the highlighted compiler contract before checking later diagnostics
E1017.example = vitte check path/to/file.vit

E1018.summary = ambiguous importacao caminho.
E1018.cause = The general phase found code that violates this diagnostic rule.
E1018.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1018.fix = repair the highlighted compiler contract before checking later diagnostics
E1018.example = vitte check path/to/file.vit

E1019.summary = strict-modules proibe glob imports.
E1019.cause = The general phase found code that violates this diagnostic rule.
E1019.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1019.fix = repair the highlighted compiler contract before checking later diagnostics
E1019.example = vitte check path/to/file.vit

E1020.summary = legacy importacao caminho esta obsoleto.
E1020.cause = The general phase found code that violates this diagnostic rule.
E1020.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1020.fix = repair the highlighted compiler contract before checking later diagnostics
E1020.example = vitte check path/to/file.vit

E1021.summary = entry modulo caminho deve ser canonical.
E1021.cause = The general phase found code that violates this diagnostic rule.
E1021.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1021.fix = repair the highlighted compiler contract before checking later diagnostics
E1021.example = vitte check path/to/file.vit

E1022.summary = entry nome duplicado.
E1022.cause = The general phase found code that violates this diagnostic rule.
E1022.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1022.fix = repair the highlighted compiler contract before checking later diagnostics
E1022.example = vitte check path/to/file.vit

E1023.summary = share referencia simbolo desconhecido.
E1023.cause = The general phase found code that violates this diagnostic rule.
E1023.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1023.fix = repair the highlighted compiler contract before checking later diagnostics
E1023.example = vitte check path/to/file.vit

E1024.summary = simbolo in share list duplicado.
E1024.cause = The general phase found code that violates this diagnostic rule.
E1024.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1024.fix = repair the highlighted compiler contract before checking later diagnostics
E1024.example = vitte check path/to/file.vit

E1025.summary = simbolo nao exportado por modulo.
E1025.cause = The general phase found code that violates this diagnostic rule.
E1025.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1025.fix = repair the highlighted compiler contract before checking later diagnostics
E1025.example = vitte check path/to/file.vit

E1026.summary = share declaracao duplicado.
E1026.cause = The general phase found code that violates this diagnostic rule.
E1026.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1026.fix = repair the highlighted compiler contract before checking later diagnostics
E1026.example = vitte check path/to/file.vit

E1027.summary = importacao vinculo duplicado.
E1027.cause = The general phase found code that violates this diagnostic rule.
E1027.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1027.fix = repair the highlighted compiler contract before checking later diagnostics
E1027.example = vitte check path/to/file.vit

E1028.summary = importacao vinculo conflita com local declaracao.
E1028.cause = The general phase found code that violates this diagnostic rule.
E1028.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1028.fix = repair the highlighted compiler contract before checking later diagnostics
E1028.example = vitte check path/to/file.vit

E1029.summary = local declaracao nome duplicado.
E1029.cause = The general phase found code that violates this diagnostic rule.
E1029.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1029.fix = repair the highlighted compiler contract before checking later diagnostics
E1029.example = vitte check path/to/file.vit

E1030.summary = modulo alias membro not exported.
E1030.cause = The general phase found code that violates this diagnostic rule.
E1030.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1030.fix = repair the highlighted compiler contract before checking later diagnostics
E1030.example = vitte check path/to/file.vit

E1031.summary = qualified tipo membro ausente.
E1031.cause = The general phase found code that violates this diagnostic rule.
E1031.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1031.fix = repair the highlighted compiler contract before checking later diagnostics
E1031.example = vitte check path/to/file.vit

E1032.summary = a expressao nao pode ser chamada.
E1032.cause = The general phase found code that violates this diagnostic rule.
E1032.step1 = Fix the first span reported for this diagnostic, then run the command again.
E1032.fix = repair the highlighted compiler contract before checking later diagnostics
E1032.example = vitte check path/to/file.vit

E2001.summary = tipo nao suportado.
E2001.cause = The general phase found code that violates this diagnostic rule.
E2001.step1 = Fix the first span reported for this diagnostic, then run the command again.
E2001.fix = repair the highlighted compiler contract before checking later diagnostics
E2001.example = vitte check path/to/file.vit

E2002.summary = invoke has no destino da chamada.
E2002.cause = The general phase found code that violates this diagnostic rule.
E2002.step1 = Fix the first span reported for this diagnostic, then run the command again.
E2002.fix = repair the highlighted compiler contract before checking later diagnostics
E2002.example = vitte check path/to/file.vit

E2003.summary = expressao in HIR nao suportado.
E2003.cause = The general phase found code that violates this diagnostic rule.
E2003.step1 = Fix the first span reported for this diagnostic, then run the command again.
E2003.fix = repair the highlighted compiler contract before checking later diagnostics
E2003.example = vitte check path/to/file.vit

E2004.summary = padrao in HIR nao suportado.
E2004.cause = The general phase found code that violates this diagnostic rule.
E2004.step1 = Fix the first span reported for this diagnostic, then run the command again.
E2004.fix = repair the highlighted compiler contract before checking later diagnostics
E2004.example = vitte check path/to/file.vit

E2005.summary = instrucao in HIR nao suportado.
E2005.cause = The general phase found code that violates this diagnostic rule.
E2005.step1 = Fix the first span reported for this diagnostic, then run the command again.
E2005.fix = repair the highlighted compiler contract before checking later diagnostics
E2005.example = vitte check path/to/file.vit

E2006.summary = HIR tipo kind inesperado.
E2006.cause = The general phase found code that violates this diagnostic rule.
E2006.step1 = Fix the first span reported for this diagnostic, then run the command again.
E2006.fix = repair the highlighted compiler contract before checking later diagnostics
E2006.example = vitte check path/to/file.vit

E2007.summary = HIR expr kind inesperado.
E2007.cause = The general phase found code that violates this diagnostic rule.
E2007.step1 = Fix the first span reported for this diagnostic, then run the command again.
E2007.fix = repair the highlighted compiler contract before checking later diagnostics
E2007.example = vitte check path/to/file.vit

E2008.summary = HIR stmt kind inesperado.
E2008.cause = The general phase found code that violates this diagnostic rule.
E2008.step1 = Fix the first span reported for this diagnostic, then run the command again.
E2008.fix = repair the highlighted compiler contract before checking later diagnostics
E2008.example = vitte check path/to/file.vit

E2009.summary = HIR padrao kind inesperado.
E2009.cause = The general phase found code that violates this diagnostic rule.
E2009.step1 = Fix the first span reported for this diagnostic, then run the command again.
E2009.fix = repair the highlighted compiler contract before checking later diagnostics
E2009.example = vitte check path/to/file.vit

E2010.summary = HIR decl kind inesperado.
E2010.cause = The general phase found code that violates this diagnostic rule.
E2010.step1 = Fix the first span reported for this diagnostic, then run the command again.
E2010.fix = repair the highlighted compiler contract before checking later diagnostics
E2010.example = vitte check path/to/file.vit

LEX_E_INVALID_CHAR.summary = caractere invalido.
LEX_E_INVALID_CHAR.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_CHAR.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_CHAR.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_CHAR.example = proc main() -> int { give 0; }

LEX_E_INVALID_NUMBER.summary = numeric literal invalido.
LEX_E_INVALID_NUMBER.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_NUMBER.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_NUMBER.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_NUMBER.example = proc main() -> int { give 0; }

LEX_E_INVALID_ESCAPE.summary = escape sequence invalido.
LEX_E_INVALID_ESCAPE.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_ESCAPE.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_ESCAPE.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_ESCAPE.example = proc main() -> int { give 0; }

LEX_E_INVALID_UNICODE_ESCAPE.summary = unicode escape sequence invalido.
LEX_E_INVALID_UNICODE_ESCAPE.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_UNICODE_ESCAPE.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_UNICODE_ESCAPE.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_UNICODE_ESCAPE.example = proc main() -> int { give 0; }

LEX_E_INVALID_UTF8.summary = UTF-8 byte sequence invalido.
LEX_E_INVALID_UTF8.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_UTF8.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_UTF8.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_UTF8.example = proc main() -> int { give 0; }

LEX_E_INVALID_CHAR_LITERAL.summary = caractere literal invalido.
LEX_E_INVALID_CHAR_LITERAL.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_CHAR_LITERAL.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_CHAR_LITERAL.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_CHAR_LITERAL.example = proc main() -> int { give 0; }

LEX_E_INVALID_IDENTIFIER.summary = identificador invalido.
LEX_E_INVALID_IDENTIFIER.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_IDENTIFIER.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_IDENTIFIER.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_IDENTIFIER.example = proc main() -> int { give 0; }

LEX_E_UNTERMINATED_STRING.summary = string literal nao terminado.
LEX_E_UNTERMINATED_STRING.cause = The parser or lexer could not form the next valid source construct.
LEX_E_UNTERMINATED_STRING.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_UNTERMINATED_STRING.fix = add the closing `"` on the same line; do not close a string with a single quote
LEX_E_UNTERMINATED_STRING.example = proc main() -> int { give 0; }

LEX_E_UNTERMINATED_CHAR.summary = caractere literal nao terminado.
LEX_E_UNTERMINATED_CHAR.cause = The parser or lexer could not form the next valid source construct.
LEX_E_UNTERMINATED_CHAR.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_UNTERMINATED_CHAR.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_UNTERMINATED_CHAR.example = proc main() -> int { give 0; }

LEX_E_UNTERMINATED_BLOCK_COMMENT.summary = bloco comment nao terminado.
LEX_E_UNTERMINATED_BLOCK_COMMENT.cause = The parser or lexer could not form the next valid source construct.
LEX_E_UNTERMINATED_BLOCK_COMMENT.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_UNTERMINATED_BLOCK_COMMENT.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_UNTERMINATED_BLOCK_COMMENT.example = proc main() -> int { give 0; }

LEX_E_UNTERMINATED_REGION_COMMENT.summary = region comment nao terminado.
LEX_E_UNTERMINATED_REGION_COMMENT.cause = The parser or lexer could not form the next valid source construct.
LEX_E_UNTERMINATED_REGION_COMMENT.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_UNTERMINATED_REGION_COMMENT.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_UNTERMINATED_REGION_COMMENT.example = proc main() -> int { give 0; }

LEX_E_UNEXPECTED_EOF.summary = end of arquivo inesperado.
LEX_E_UNEXPECTED_EOF.cause = The parser or lexer could not form the next valid source construct.
LEX_E_UNEXPECTED_EOF.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_UNEXPECTED_EOF.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_UNEXPECTED_EOF.example = proc main() -> int { give 0; }

LEX_E_TOKEN_TOO_LARGE.summary = token demais.
LEX_E_TOKEN_TOO_LARGE.cause = The parser or lexer could not form the next valid source construct.
LEX_E_TOKEN_TOO_LARGE.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_TOKEN_TOO_LARGE.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_TOKEN_TOO_LARGE.example = proc main() -> int { give 0; }

PLOOP.summary = o parser nao avancou.
PLOOP.cause = The parser or lexer could not form the next valid source construct.
PLOOP.step1 = Look at the highlighted token and complete or remove the construct around it.
PLOOP.fix = complete the syntax shape named by the parser label at the highlighted token
PLOOP.example = proc main() -> int { give 0; }

PPRIMARY999.summary = expressao token inesperado.
PPRIMARY999.cause = The parser or lexer could not form the next valid source construct.
PPRIMARY999.step1 = Look at the highlighted token and complete or remove the construct around it.
PPRIMARY999.fix = complete the syntax shape named by the parser label at the highlighted token
PPRIMARY999.example = proc main() -> int { give 0; }

PSTMT007.summary = atribuicao operador esperado.
PSTMT007.cause = The parser or lexer could not form the next valid source construct.
PSTMT007.step1 = Look at the highlighted token and complete or remove the construct around it.
PSTMT007.fix = complete the syntax shape named by the parser label at the highlighted token
PSTMT007.example = proc main() -> int { give 0; }

P0001.summary = top-level token inesperado.
P0001.cause = The parser or lexer could not form the next valid source construct.
P0001.step1 = Look at the highlighted token and complete or remove the construct around it.
P0001.fix = complete the syntax shape named by the parser label at the highlighted token
P0001.example = proc main() -> int { give 0; }

P000_UNBALANCED.summary = bloco nao fechado.
P000_UNBALANCED.cause = The parser or lexer could not form the next valid source construct.
P000_UNBALANCED.step1 = Look at the highlighted token and complete or remove the construct around it.
P000_UNBALANCED.fix = complete the syntax shape named by the parser label at the highlighted token
P000_UNBALANCED.example = proc main() -> int { give 0; }

LEX_E_INVALID_FLOAT.summary = ponto flutuante invalido.
LEX_E_INVALID_FLOAT.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_FLOAT.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_FLOAT.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_FLOAT.example = proc main() -> int { give 0; }

LEX_E_INVALID_BINARY.summary = binario invalido.
LEX_E_INVALID_BINARY.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_BINARY.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_BINARY.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_BINARY.example = proc main() -> int { give 0; }

LEX_E_INVALID_OCTAL.summary = octal invalido.
LEX_E_INVALID_OCTAL.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_OCTAL.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_OCTAL.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_OCTAL.example = proc main() -> int { give 0; }

LEX_E_INVALID_HEX.summary = hex invalido.
LEX_E_INVALID_HEX.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_HEX.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_HEX.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_HEX.example = proc main() -> int { give 0; }

LEX_E_INVALID_INDENTATION.summary = indentacao invalido.
LEX_E_INVALID_INDENTATION.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_INDENTATION.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_INDENTATION.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_INDENTATION.example = proc main() -> int { give 0; }

LEX_E_INVALID_TOKEN.summary = token invalido.
LEX_E_INVALID_TOKEN.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_TOKEN.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_TOKEN.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_TOKEN.example = proc main() -> int { give 0; }

LEX_E_INVALID_UNICODE.summary = unicode invalido.
LEX_E_INVALID_UNICODE.cause = The parser or lexer could not form the next valid source construct.
LEX_E_INVALID_UNICODE.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_INVALID_UNICODE.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_INVALID_UNICODE.example = proc main() -> int { give 0; }

LEX_E_UNTERMINATED_COMMENT.summary = comment nao terminado.
LEX_E_UNTERMINATED_COMMENT.cause = The parser or lexer could not form the next valid source construct.
LEX_E_UNTERMINATED_COMMENT.step1 = Look at the highlighted token and complete or remove the construct around it.
LEX_E_UNTERMINATED_COMMENT.fix = complete the syntax shape named by the parser label at the highlighted token
LEX_E_UNTERMINATED_COMMENT.example = proc main() -> int { give 0; }

PATTR003.summary = pattr003.
PATTR003.cause = The parser or lexer could not form the next valid source construct.
PATTR003.step1 = Look at the highlighted token and complete or remove the construct around it.
PATTR003.fix = complete the syntax shape named by the parser label at the highlighted token
PATTR003.example = proc main() -> int { give 0; }

PARSE_E_TOPLEVEL_DECL_EXPECTED.summary = top-level declaracao expected.
PARSE_E_TOPLEVEL_DECL_EXPECTED.cause = The parser or lexer could not form the next valid source construct.
PARSE_E_TOPLEVEL_DECL_EXPECTED.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_E_TOPLEVEL_DECL_EXPECTED.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_E_TOPLEVEL_DECL_EXPECTED.example = proc main() -> int { give 0; }

PARSE_E_INCOMPLETE_EXPR.summary = incomplete expressao.
PARSE_E_INCOMPLETE_EXPR.cause = The parser or lexer could not form the next valid source construct.
PARSE_E_INCOMPLETE_EXPR.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_E_INCOMPLETE_EXPR.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_E_INCOMPLETE_EXPR.example = proc main() -> int { give 0; }

PARSE_E_MISSING_RPAREN.summary = closing parenthesis ausente.
PARSE_E_MISSING_RPAREN.cause = The parser or lexer could not form the next valid source construct.
PARSE_E_MISSING_RPAREN.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_E_MISSING_RPAREN.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_E_MISSING_RPAREN.example = proc main() -> int { give 0; }

PARSE_E_MISSING_COMMA.summary = comma ausente.
PARSE_E_MISSING_COMMA.cause = The parser or lexer could not form the next valid source construct.
PARSE_E_MISSING_COMMA.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_E_MISSING_COMMA.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_E_MISSING_COMMA.example = proc main() -> int { give 0; }

PARSE_E_TYPE_EXPECTED.summary = tipo expected.
PARSE_E_TYPE_EXPECTED.cause = The parser or lexer could not form the next valid source construct.
PARSE_E_TYPE_EXPECTED.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_E_TYPE_EXPECTED.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_E_TYPE_EXPECTED.example = proc main() -> int { give 0; }

PARSE_E_PATTERN_EXPECTED.summary = padrao expected.
PARSE_E_PATTERN_EXPECTED.cause = The parser or lexer could not form the next valid source construct.
PARSE_E_PATTERN_EXPECTED.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_E_PATTERN_EXPECTED.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_E_PATTERN_EXPECTED.example = proc main() -> int { give 0; }

PARSE_E_BLOCK_EXPECTED.summary = bloco expected.
PARSE_E_BLOCK_EXPECTED.cause = The parser or lexer could not form the next valid source construct.
PARSE_E_BLOCK_EXPECTED.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_E_BLOCK_EXPECTED.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_E_BLOCK_EXPECTED.example = proc main() -> int { give 0; }

PARSE_E_UNCLOSED_BLOCK.summary = bloco nao fechado.
PARSE_E_UNCLOSED_BLOCK.cause = The parser or lexer could not form the next valid source construct.
PARSE_E_UNCLOSED_BLOCK.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_E_UNCLOSED_BLOCK.fix = add `}` to close the highlighted block, then run the checker again for its parent block
PARSE_E_UNCLOSED_BLOCK.example = proc main() -> int { give 0; }

PARSE_E_EXPECTED_TOKEN.summary = token esperado.
PARSE_E_EXPECTED_TOKEN.cause = The parser or lexer could not form the next valid source construct.
PARSE_E_EXPECTED_TOKEN.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_E_EXPECTED_TOKEN.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_E_EXPECTED_TOKEN.example = proc main() -> int { give 0; }

PARSE_E_PARAMETER_COLON_EXPECTED.summary = parameter colon expected.
PARSE_E_PARAMETER_COLON_EXPECTED.cause = The parser or lexer could not form the next valid source construct.
PARSE_E_PARAMETER_COLON_EXPECTED.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_E_PARAMETER_COLON_EXPECTED.fix = insert `:` between the parameter name and its type, for example `right: f64`
PARSE_E_PARAMETER_COLON_EXPECTED.example = proc main() -> int { give 0; }

PARSE_E_UNEXPECTED_TOKEN.summary = token inesperado.
PARSE_E_UNEXPECTED_TOKEN.cause = The parser or lexer could not form the next valid source construct.
PARSE_E_UNEXPECTED_TOKEN.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_E_UNEXPECTED_TOKEN.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_E_UNEXPECTED_TOKEN.example = proc main() -> int { give 0; }

PARSE_EXPECTED_EXPR.summary = parse expected expr.
PARSE_EXPECTED_EXPR.cause = The parser or lexer could not form the next valid source construct.
PARSE_EXPECTED_EXPR.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_EXPECTED_EXPR.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_EXPECTED_EXPR.example = proc main() -> int { give 0; }

PARSE_EXPECTED_TYPE.summary = parse expected tipo.
PARSE_EXPECTED_TYPE.cause = The parser or lexer could not form the next valid source construct.
PARSE_EXPECTED_TYPE.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_EXPECTED_TYPE.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_EXPECTED_TYPE.example = proc main() -> int { give 0; }

PARSE_EXPECTED_PATTERN.summary = parse expected padrao.
PARSE_EXPECTED_PATTERN.cause = The parser or lexer could not form the next valid source construct.
PARSE_EXPECTED_PATTERN.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_EXPECTED_PATTERN.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_EXPECTED_PATTERN.example = proc main() -> int { give 0; }

PARSE_EXPECTED_BLOCK.summary = parse expected bloco.
PARSE_EXPECTED_BLOCK.cause = The parser or lexer could not form the next valid source construct.
PARSE_EXPECTED_BLOCK.step1 = Look at the highlighted token and complete or remove the construct around it.
PARSE_EXPECTED_BLOCK.fix = complete the syntax shape named by the parser label at the highlighted token
PARSE_EXPECTED_BLOCK.example = proc main() -> int { give 0; }

PARSE_EXPECTED_IDENTIFIER.summary = parse expected identificador.
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

AST_E_INVALID_NODE.summary = no invalido.
AST_E_INVALID_NODE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
AST_E_INVALID_NODE.step1 = Fix the first span reported for this diagnostic, then run the command again.
AST_E_INVALID_NODE.fix = repair the highlighted compiler contract before checking later diagnostics
AST_E_INVALID_NODE.example = vitte check path/to/file.vit

AST_E_EMPTY_MODULE.summary = vazio modulo.
AST_E_EMPTY_MODULE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
AST_E_EMPTY_MODULE.step1 = Fix the first span reported for this diagnostic, then run the command again.
AST_E_EMPTY_MODULE.fix = repair the highlighted compiler contract before checking later diagnostics
AST_E_EMPTY_MODULE.example = vitte check path/to/file.vit

AST_E_INVALID_DECL.summary = decl invalido.
AST_E_INVALID_DECL.cause = The compiler pipeline phase found code that violates this diagnostic rule.
AST_E_INVALID_DECL.step1 = Fix the first span reported for this diagnostic, then run the command again.
AST_E_INVALID_DECL.fix = repair the highlighted compiler contract before checking later diagnostics
AST_E_INVALID_DECL.example = vitte check path/to/file.vit

AST_E_INVALID_EXPR.summary = expr invalido.
AST_E_INVALID_EXPR.cause = The compiler pipeline phase found code that violates this diagnostic rule.
AST_E_INVALID_EXPR.step1 = Fix the first span reported for this diagnostic, then run the command again.
AST_E_INVALID_EXPR.fix = repair the highlighted compiler contract before checking later diagnostics
AST_E_INVALID_EXPR.example = vitte check path/to/file.vit

AST_E_INVALID_PATTERN.summary = padrao invalido.
AST_E_INVALID_PATTERN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
AST_E_INVALID_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
AST_E_INVALID_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
AST_E_INVALID_PATTERN.example = vitte check path/to/file.vit

AST_E_INVALID_ATTRIBUTE.summary = attribute invalido.
AST_E_INVALID_ATTRIBUTE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
AST_E_INVALID_ATTRIBUTE.step1 = Fix the first span reported for this diagnostic, then run the command again.
AST_E_INVALID_ATTRIBUTE.fix = repair the highlighted compiler contract before checking later diagnostics
AST_E_INVALID_ATTRIBUTE.example = vitte check path/to/file.vit

AST_E_DUPLICATE_FIELD.summary = campo duplicado.
AST_E_DUPLICATE_FIELD.cause = The compiler pipeline phase found code that violates this diagnostic rule.
AST_E_DUPLICATE_FIELD.step1 = Fix the first span reported for this diagnostic, then run the command again.
AST_E_DUPLICATE_FIELD.fix = repair the highlighted compiler contract before checking later diagnostics
AST_E_DUPLICATE_FIELD.example = vitte check path/to/file.vit

AST_E_INVALID_VISIBILITY.summary = visibilidade invalido.
AST_E_INVALID_VISIBILITY.cause = The compiler pipeline phase found code that violates this diagnostic rule.
AST_E_INVALID_VISIBILITY.step1 = Fix the first span reported for this diagnostic, then run the command again.
AST_E_INVALID_VISIBILITY.fix = repair the highlighted compiler contract before checking later diagnostics
AST_E_INVALID_VISIBILITY.example = vitte check path/to/file.vit

AST_E_INVALID_ENTRY.summary = entry invalido.
AST_E_INVALID_ENTRY.cause = The compiler pipeline phase found code that violates this diagnostic rule.
AST_E_INVALID_ENTRY.step1 = Fix the first span reported for this diagnostic, then run the command again.
AST_E_INVALID_ENTRY.fix = repair the highlighted compiler contract before checking later diagnostics
AST_E_INVALID_ENTRY.example = vitte check path/to/file.vit

SEMA_E_DUPLICATE_SYMBOL.summary = simbolo duplicado.
SEMA_E_DUPLICATE_SYMBOL.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_DUPLICATE_SYMBOL.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_DUPLICATE_SYMBOL.fix = rename one declaration or remove the duplicate from the same scope
SEMA_E_DUPLICATE_SYMBOL.example = use vitte/core

SEMA_E_UNKNOWN_IDENTIFIER.summary = identificador desconhecido.
SEMA_E_UNKNOWN_IDENTIFIER.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_UNKNOWN_IDENTIFIER.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_UNKNOWN_IDENTIFIER.fix = declare the missing identifier in scope or import the module that exports it
SEMA_E_UNKNOWN_IDENTIFIER.example = use vitte/core

SEMA_E_AMBIGUOUS_SYMBOL.summary = ambiguous simbolo.
SEMA_E_AMBIGUOUS_SYMBOL.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_AMBIGUOUS_SYMBOL.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_AMBIGUOUS_SYMBOL.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_AMBIGUOUS_SYMBOL.example = use vitte/core

SEMA_E_SHADOWING_FORBIDDEN.summary = shadowing is forbidden.
SEMA_E_SHADOWING_FORBIDDEN.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_SHADOWING_FORBIDDEN.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_SHADOWING_FORBIDDEN.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_SHADOWING_FORBIDDEN.example = use vitte/core

SEMA_E_UNKNOWN_FIELD.summary = campo does not exist.
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

SEMA_E_UNKNOWN_SYMBOL.summary = simbolo desconhecido.
SEMA_E_UNKNOWN_SYMBOL.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_UNKNOWN_SYMBOL.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_UNKNOWN_SYMBOL.fix = declare the missing identifier in scope or import the module that exports it
SEMA_E_UNKNOWN_SYMBOL.example = use vitte/core

SEMA_E_INVALID_IMPORT.summary = importacao invalido.
SEMA_E_INVALID_IMPORT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_INVALID_IMPORT.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_INVALID_IMPORT.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_INVALID_IMPORT.example = use vitte/core

SEMA_E_INVALID_EXPORT.summary = export invalido.
SEMA_E_INVALID_EXPORT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_INVALID_EXPORT.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_INVALID_EXPORT.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_INVALID_EXPORT.example = use vitte/core

SEMA_E_INVALID_ATTRIBUTE.summary = attribute invalido.
SEMA_E_INVALID_ATTRIBUTE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_INVALID_ATTRIBUTE.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_INVALID_ATTRIBUTE.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_INVALID_ATTRIBUTE.example = use vitte/core

SEMA_E_INVALID_VISIBILITY.summary = visibilidade invalido.
SEMA_E_INVALID_VISIBILITY.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_INVALID_VISIBILITY.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_INVALID_VISIBILITY.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_INVALID_VISIBILITY.example = use vitte/core

SEMA_E_INVALID_CONTROL_FLOW.summary = control fluxo invalido.
SEMA_E_INVALID_CONTROL_FLOW.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_INVALID_CONTROL_FLOW.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_INVALID_CONTROL_FLOW.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_INVALID_CONTROL_FLOW.example = use vitte/core

SEMA_E_INVALID_MODULE.summary = modulo invalido.
SEMA_E_INVALID_MODULE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_INVALID_MODULE.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_INVALID_MODULE.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_INVALID_MODULE.example = use vitte/core

SEMA_E_INVALID_ASSIGN_TARGET.summary = assign destino invalido.
SEMA_E_INVALID_ASSIGN_TARGET.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_INVALID_ASSIGN_TARGET.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_INVALID_ASSIGN_TARGET.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_INVALID_ASSIGN_TARGET.example = use vitte/core

SEMA_E_UNDECLARED_TARGET.summary = undeclared destino.
SEMA_E_UNDECLARED_TARGET.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_UNDECLARED_TARGET.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_UNDECLARED_TARGET.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_UNDECLARED_TARGET.example = use vitte/core

SEMA_E_MISSING_BINDING.summary = vinculo ausente.
SEMA_E_MISSING_BINDING.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_MISSING_BINDING.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_MISSING_BINDING.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_MISSING_BINDING.example = use vitte/core

SEMA_E_DUPLICATE_BINDING.summary = vinculo duplicado.
SEMA_E_DUPLICATE_BINDING.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_DUPLICATE_BINDING.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_DUPLICATE_BINDING.fix = rename one declaration or remove the duplicate from the same scope
SEMA_E_DUPLICATE_BINDING.example = use vitte/core

SEMA_E_DUPLICATE_ITEM.summary = item duplicado.
SEMA_E_DUPLICATE_ITEM.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_DUPLICATE_ITEM.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_DUPLICATE_ITEM.fix = rename one declaration or remove the duplicate from the same scope
SEMA_E_DUPLICATE_ITEM.example = use vitte/core

SEMA_E_INVALID_HIR.summary = hir invalido.
SEMA_E_INVALID_HIR.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_INVALID_HIR.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_INVALID_HIR.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_INVALID_HIR.example = use vitte/core

SEMA_E_INTERNAL.summary = interno.
SEMA_E_INTERNAL.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
SEMA_E_INTERNAL.step1 = Check the spelling and the nearest import or declaration.
SEMA_E_INTERNAL.fix = resolve the exact symbol contract reported by semantic analysis
SEMA_E_INTERNAL.example = use vitte/core

TYPECK_E_UNKNOWN_NAME.summary = nome desconhecido.
TYPECK_E_UNKNOWN_NAME.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_UNKNOWN_NAME.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_UNKNOWN_NAME.fix = make the expression type match the type contract named by the type checker
TYPECK_E_UNKNOWN_NAME.example = let count: int = 1

TYPECK_E_UNKNOWN_TYPE.summary = tipo desconhecido.
TYPECK_E_UNKNOWN_TYPE.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_UNKNOWN_TYPE.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_UNKNOWN_TYPE.fix = make the expression type match the type contract named by the type checker
TYPECK_E_UNKNOWN_TYPE.example = let count: int = 1

TYPECK_E_UNKNOWN_ITEM.summary = item desconhecido.
TYPECK_E_UNKNOWN_ITEM.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_UNKNOWN_ITEM.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_UNKNOWN_ITEM.fix = make the expression type match the type contract named by the type checker
TYPECK_E_UNKNOWN_ITEM.example = let count: int = 1

TYPECK_W_UNRESOLVED_NAME.summary = unresolved nome.
TYPECK_W_UNRESOLVED_NAME.cause = The inferred type does not satisfy the type required at this location.
TYPECK_W_UNRESOLVED_NAME.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_W_UNRESOLVED_NAME.fix = make the expression type match the type contract named by the type checker
TYPECK_W_UNRESOLVED_NAME.example = let count: int = 1

TYPECK_E_INVALID_EXPR.summary = expr invalido.
TYPECK_E_INVALID_EXPR.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_INVALID_EXPR.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_INVALID_EXPR.fix = make the expression type match the type contract named by the type checker
TYPECK_E_INVALID_EXPR.example = let count: int = 1

TYPECK_E_INVALID_DEREF.summary = deref invalido.
TYPECK_E_INVALID_DEREF.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_INVALID_DEREF.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_INVALID_DEREF.fix = make the expression type match the type contract named by the type checker
TYPECK_E_INVALID_DEREF.example = let count: int = 1

TYPECK_E_BINARY_MISMATCH.summary = binario incompatibilidade.
TYPECK_E_BINARY_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_BINARY_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_BINARY_MISMATCH.fix = make both operands valid for the operator before MIR lowering
TYPECK_E_BINARY_MISMATCH.example = let count: int = 1

TYPECK_E_ASSIGN_MISMATCH.summary = atribuicao tipo incompatibilidade.
TYPECK_E_ASSIGN_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_ASSIGN_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_ASSIGN_MISMATCH.fix = assign a value of the declared binding type, or change the binding annotation at its declaration
TYPECK_E_ASSIGN_MISMATCH.example = let count: int = 1

TYPECK_E_RETURN_MISMATCH.summary = retorno tipo incompatibilidade.
TYPECK_E_RETURN_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_RETURN_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_RETURN_MISMATCH.fix = make the expression type match the type contract named by the type checker
TYPECK_E_RETURN_MISMATCH.example = let count: int = 1

TYPECK_E_CONDITION_TYPE.summary = condicao tipo.
TYPECK_E_CONDITION_TYPE.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_CONDITION_TYPE.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_CONDITION_TYPE.fix = make the condition produce bool, for example by adding an explicit comparison
TYPECK_E_CONDITION_TYPE.example = let count: int = 1

TYPECK_E_UNKNOWN_MEMBER.summary = membro desconhecido.
TYPECK_E_UNKNOWN_MEMBER.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_UNKNOWN_MEMBER.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_UNKNOWN_MEMBER.fix = make the expression type match the type contract named by the type checker
TYPECK_E_UNKNOWN_MEMBER.example = let count: int = 1

TYPECK_E_INDEX_TYPE.summary = indice tipo.
TYPECK_E_INDEX_TYPE.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_INDEX_TYPE.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_INDEX_TYPE.fix = make the expression type match the type contract named by the type checker
TYPECK_E_INDEX_TYPE.example = let count: int = 1

TYPECK_E_INVALID_INDEX_TARGET.summary = indice destino invalido.
TYPECK_E_INVALID_INDEX_TARGET.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_INVALID_INDEX_TARGET.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_INVALID_INDEX_TARGET.fix = make the expression type match the type contract named by the type checker
TYPECK_E_INVALID_INDEX_TARGET.example = let count: int = 1

TYPECK_E_IF_BRANCH_MISMATCH.summary = if branch incompatibilidade.
TYPECK_E_IF_BRANCH_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_IF_BRANCH_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_IF_BRANCH_MISMATCH.fix = make the expression type match the type contract named by the type checker
TYPECK_E_IF_BRANCH_MISMATCH.example = let count: int = 1

TYPECK_E_COMPARE_MISMATCH.summary = compare incompatibilidade.
TYPECK_E_COMPARE_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_COMPARE_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_COMPARE_MISMATCH.fix = make the expression type match the type contract named by the type checker
TYPECK_E_COMPARE_MISMATCH.example = let count: int = 1

TYPECK_E_INVALID_CAST.summary = conversao invalido.
TYPECK_E_INVALID_CAST.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_INVALID_CAST.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_INVALID_CAST.fix = make the expression type match the type contract named by the type checker
TYPECK_E_INVALID_CAST.example = let count: int = 1

TYPECK_E_MATCH_NON_EXHAUSTIVE.summary = match non exhaustive.
TYPECK_E_MATCH_NON_EXHAUSTIVE.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_MATCH_NON_EXHAUSTIVE.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_MATCH_NON_EXHAUSTIVE.fix = make the expression type match the type contract named by the type checker
TYPECK_E_MATCH_NON_EXHAUSTIVE.example = let count: int = 1

TYPECK_E_INVALID_CALL_TARGET.summary = chamada destino invalido.
TYPECK_E_INVALID_CALL_TARGET.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_INVALID_CALL_TARGET.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_INVALID_CALL_TARGET.fix = make the expression type match the type contract named by the type checker
TYPECK_E_INVALID_CALL_TARGET.example = let count: int = 1

TYPECK_E_ARGUMENT_MISMATCH.summary = chamada argumento tipo incompatibilidade.
TYPECK_E_ARGUMENT_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_ARGUMENT_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_ARGUMENT_MISMATCH.fix = make the expression type match the type contract named by the type checker
TYPECK_E_ARGUMENT_MISMATCH.example = let count: int = 1

TYPECK_E_CALL_ARITY.summary = wrong numero of chamada argumentos.
TYPECK_E_CALL_ARITY.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_CALL_ARITY.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_CALL_ARITY.fix = make the expression type match the type contract named by the type checker
TYPECK_E_CALL_ARITY.example = let count: int = 1

TYPECK_E_GENERIC_INFERENCE.summary = generico tipo could not be inferred.
TYPECK_E_GENERIC_INFERENCE.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_GENERIC_INFERENCE.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_GENERIC_INFERENCE.fix = make the expression type match the type contract named by the type checker
TYPECK_E_GENERIC_INFERENCE.example = let count: int = 1

TYPECK_E_TRAIT_BOUND.summary = trait constraint is not satisfied.
TYPECK_E_TRAIT_BOUND.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_TRAIT_BOUND.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_TRAIT_BOUND.fix = make the expression type match the type contract named by the type checker
TYPECK_E_TRAIT_BOUND.example = let count: int = 1

TYPECK_E_CAUSE_CHAIN_MISSING.summary = tipo diagnostic is ausente a cause chain.
TYPECK_E_CAUSE_CHAIN_MISSING.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_CAUSE_CHAIN_MISSING.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_CAUSE_CHAIN_MISSING.fix = make the expression type match the type contract named by the type checker
TYPECK_E_CAUSE_CHAIN_MISSING.example = let count: int = 1

TYPECK_E_CONFLICTING_IMPL.summary = conflicting trait implementation.
TYPECK_E_CONFLICTING_IMPL.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_CONFLICTING_IMPL.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_CONFLICTING_IMPL.fix = make the expression type match the type contract named by the type checker
TYPECK_E_CONFLICTING_IMPL.example = let count: int = 1

TYPECK_E_CONSTRAINT_CYCLE.summary = cyclic generico constraint.
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

TYPECK_E_USE_AFTER_MOVE.summary = use after movimento.
TYPECK_E_USE_AFTER_MOVE.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_USE_AFTER_MOVE.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_USE_AFTER_MOVE.fix = make the expression type match the type contract named by the type checker
TYPECK_E_USE_AFTER_MOVE.example = let count: int = 1

TYPECK_E_MALFORMED_UNARY.summary = malformed unary.
TYPECK_E_MALFORMED_UNARY.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_MALFORMED_UNARY.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_MALFORMED_UNARY.fix = make the expression type match the type contract named by the type checker
TYPECK_E_MALFORMED_UNARY.example = let count: int = 1

TYPECK_E_UNARY_MISMATCH.summary = unary incompatibilidade.
TYPECK_E_UNARY_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_UNARY_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_UNARY_MISMATCH.fix = make the expression type match the type contract named by the type checker
TYPECK_E_UNARY_MISMATCH.example = let count: int = 1

TYPECK_E_MALFORMED_BORROW.summary = malformed emprestimo.
TYPECK_E_MALFORMED_BORROW.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_MALFORMED_BORROW.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_MALFORMED_BORROW.fix = make the expression type match the type contract named by the type checker
TYPECK_E_MALFORMED_BORROW.example = let count: int = 1

TYPECK_E_MALFORMED_MEMBER.summary = malformed membro.
TYPECK_E_MALFORMED_MEMBER.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_MALFORMED_MEMBER.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_MALFORMED_MEMBER.fix = make the expression type match the type contract named by the type checker
TYPECK_E_MALFORMED_MEMBER.example = let count: int = 1

TYPECK_E_MALFORMED_INDEX.summary = malformed indice.
TYPECK_E_MALFORMED_INDEX.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_MALFORMED_INDEX.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_MALFORMED_INDEX.fix = make the expression type match the type contract named by the type checker
TYPECK_E_MALFORMED_INDEX.example = let count: int = 1

TYPECK_E_MALFORMED_BINARY.summary = malformed binario.
TYPECK_E_MALFORMED_BINARY.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_MALFORMED_BINARY.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_MALFORMED_BINARY.fix = make the expression type match the type contract named by the type checker
TYPECK_E_MALFORMED_BINARY.example = let count: int = 1

TYPECK_E_MALFORMED_CAST.summary = malformed conversao.
TYPECK_E_MALFORMED_CAST.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_MALFORMED_CAST.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_MALFORMED_CAST.fix = make the expression type match the type contract named by the type checker
TYPECK_E_MALFORMED_CAST.example = let count: int = 1

TYPECK_E_MALFORMED_IF.summary = malformed if.
TYPECK_E_MALFORMED_IF.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_MALFORMED_IF.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_MALFORMED_IF.fix = make the expression type match the type contract named by the type checker
TYPECK_E_MALFORMED_IF.example = let count: int = 1

TYPECK_E_INVALID_HIR.summary = hir invalido.
TYPECK_E_INVALID_HIR.cause = The inferred type does not satisfy the type required at this location.
TYPECK_E_INVALID_HIR.step1 = Compare the expected and found types in the diagnostic labels.
TYPECK_E_INVALID_HIR.fix = make the expression type match the type contract named by the type checker
TYPECK_E_INVALID_HIR.example = let count: int = 1

TYPECK_E_INTERNAL.summary = interno.
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

BORROWCK_E_MOVE_AFTER_MOVE.summary = movimento after movimento.
BORROWCK_E_MOVE_AFTER_MOVE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_MOVE_AFTER_MOVE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_MOVE_AFTER_MOVE.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_MOVE_AFTER_MOVE.example = let view = &value

BORROWCK_E_USE_AFTER_MOVE.summary = valor usado after movimento.
BORROWCK_E_USE_AFTER_MOVE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_USE_AFTER_MOVE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_USE_AFTER_MOVE.fix = borrow before the move, clone explicitly, or move the later use before ownership transfer
BORROWCK_E_USE_AFTER_MOVE.example = let view = &value

BORROWCK_E_BORROW_OF_MOVED_VALUE.summary = emprestimo of movido valor.
BORROWCK_E_BORROW_OF_MOVED_VALUE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_BORROW_OF_MOVED_VALUE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_BORROW_OF_MOVED_VALUE.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_BORROW_OF_MOVED_VALUE.example = let view = &value

BORROWCK_E_MUTABLE_BORROW_CONFLICT.summary = mutavel emprestimo conflito.
BORROWCK_E_MUTABLE_BORROW_CONFLICT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_MUTABLE_BORROW_CONFLICT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_MUTABLE_BORROW_CONFLICT.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_MUTABLE_BORROW_CONFLICT.example = let view = &value

BORROWCK_E_SHARED_BORROW_CONFLICT.summary = compartilhado emprestimo conflito.
BORROWCK_E_SHARED_BORROW_CONFLICT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_SHARED_BORROW_CONFLICT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_SHARED_BORROW_CONFLICT.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_SHARED_BORROW_CONFLICT.example = let view = &value

BORROWCK_E_WRITE_WHILE_BORROWED.summary = escrita enquanto emprestado.
BORROWCK_E_WRITE_WHILE_BORROWED.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_WRITE_WHILE_BORROWED.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_WRITE_WHILE_BORROWED.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_WRITE_WHILE_BORROWED.example = let view = &value

BORROWCK_E_MOVE_WHILE_BORROWED.summary = movimento enquanto emprestado.
BORROWCK_E_MOVE_WHILE_BORROWED.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_MOVE_WHILE_BORROWED.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_MOVE_WHILE_BORROWED.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_MOVE_WHILE_BORROWED.example = let view = &value

BORROWCK_E_DROP_WHILE_BORROWED.summary = descarte enquanto emprestado.
BORROWCK_E_DROP_WHILE_BORROWED.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_DROP_WHILE_BORROWED.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_DROP_WHILE_BORROWED.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_DROP_WHILE_BORROWED.example = let view = &value

BORROWCK_E_ASSIGN_WHILE_BORROWED.summary = assign enquanto emprestado.
BORROWCK_E_ASSIGN_WHILE_BORROWED.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_ASSIGN_WHILE_BORROWED.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_ASSIGN_WHILE_BORROWED.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_ASSIGN_WHILE_BORROWED.example = let view = &value

BORROWCK_E_RETURN_REF_TO_LOCAL.summary = retorno ref to local.
BORROWCK_E_RETURN_REF_TO_LOCAL.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_RETURN_REF_TO_LOCAL.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_RETURN_REF_TO_LOCAL.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_RETURN_REF_TO_LOCAL.example = let view = &value

BORROWCK_E_RETURN_BORROW_OF_LOCAL.summary = retorno emprestimo of local.
BORROWCK_E_RETURN_BORROW_OF_LOCAL.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_RETURN_BORROW_OF_LOCAL.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_RETURN_BORROW_OF_LOCAL.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_RETURN_BORROW_OF_LOCAL.example = let view = &value

BORROWCK_E_DANGLING_REFERENCE.summary = dangling referencia.
BORROWCK_E_DANGLING_REFERENCE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_DANGLING_REFERENCE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_DANGLING_REFERENCE.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_DANGLING_REFERENCE.example = let view = &value

BORROWCK_E_LIFETIME_TOO_SHORT.summary = tempo de vida demais short.
BORROWCK_E_LIFETIME_TOO_SHORT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_LIFETIME_TOO_SHORT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_LIFETIME_TOO_SHORT.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_LIFETIME_TOO_SHORT.example = let view = &value

BORROWCK_E_IMMUTABLE_ASSIGN.summary = immutable assign.
BORROWCK_E_IMMUTABLE_ASSIGN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_IMMUTABLE_ASSIGN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_IMMUTABLE_ASSIGN.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_IMMUTABLE_ASSIGN.example = let view = &value

BORROWCK_E_USE_AFTER_DROP.summary = use after descarte.
BORROWCK_E_USE_AFTER_DROP.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_USE_AFTER_DROP.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_USE_AFTER_DROP.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_USE_AFTER_DROP.example = let view = &value

BORROWCK_E_DOUBLE_DROP.summary = double descarte.
BORROWCK_E_DOUBLE_DROP.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_DOUBLE_DROP.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_DOUBLE_DROP.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_DOUBLE_DROP.example = let view = &value

BORROWCK_E_UNINITIALIZED_USE.summary = nao inicializado use.
BORROWCK_E_UNINITIALIZED_USE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_UNINITIALIZED_USE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_UNINITIALIZED_USE.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_UNINITIALIZED_USE.example = let view = &value

BORROWCK_E_MOVE_AFTER_BORROW.summary = movimento after emprestimo.
BORROWCK_E_MOVE_AFTER_BORROW.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_MOVE_AFTER_BORROW.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_MOVE_AFTER_BORROW.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_MOVE_AFTER_BORROW.example = let view = &value

BORROWCK_E_MUTABLE_ALIAS.summary = mutavel alias.
BORROWCK_E_MUTABLE_ALIAS.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_MUTABLE_ALIAS.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_MUTABLE_ALIAS.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_MUTABLE_ALIAS.example = let view = &value

BORROWCK_E_INTERNAL.summary = interno.
BORROWCK_E_INTERNAL.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_INTERNAL.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_INTERNAL.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_INTERNAL.example = let view = &value

BORROWCK_E_UNKNOWN.summary = desconhecido.
BORROWCK_E_UNKNOWN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
BORROWCK_E_UNKNOWN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
BORROWCK_E_UNKNOWN.fix = repair the ownership transition named by the borrow checker before the highlighted use
BORROWCK_E_UNKNOWN.example = let view = &value

CONST_EVAL_E_DIVISION_BY_ZERO.summary = divisao by zero in constante evaluation.
CONST_EVAL_E_DIVISION_BY_ZERO.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_EVAL_E_DIVISION_BY_ZERO.step1 = Reduce the constant expression at the reported span.
CONST_EVAL_E_DIVISION_BY_ZERO.fix = change the const divisor to a non-zero value or guard the expression before const evaluation
CONST_EVAL_E_DIVISION_BY_ZERO.example = const size: int = 4

CONST_EVAL_E_UNKNOWN_NAME.summary = nome desconhecido.
CONST_EVAL_E_UNKNOWN_NAME.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_EVAL_E_UNKNOWN_NAME.step1 = Reduce the constant expression at the reported span.
CONST_EVAL_E_UNKNOWN_NAME.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_EVAL_E_UNKNOWN_NAME.example = const size: int = 4

CONST_EVAL_E_NON_CONST_CALL.summary = non const chamada.
CONST_EVAL_E_NON_CONST_CALL.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_EVAL_E_NON_CONST_CALL.step1 = Reduce the constant expression at the reported span.
CONST_EVAL_E_NON_CONST_CALL.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_EVAL_E_NON_CONST_CALL.example = const size: int = 4

CONST_EVAL_E_MUTATION_IN_CONST.summary = mutation in const.
CONST_EVAL_E_MUTATION_IN_CONST.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_EVAL_E_MUTATION_IN_CONST.step1 = Reduce the constant expression at the reported span.
CONST_EVAL_E_MUTATION_IN_CONST.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_EVAL_E_MUTATION_IN_CONST.example = const size: int = 4

CONST_EVAL_E_UNSUPPORTED_EXPR.summary = expr nao suportado.
CONST_EVAL_E_UNSUPPORTED_EXPR.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_EVAL_E_UNSUPPORTED_EXPR.step1 = Reduce the constant expression at the reported span.
CONST_EVAL_E_UNSUPPORTED_EXPR.fix = replace the const expression with literals, supported arithmetic, or another compile-time value
CONST_EVAL_E_UNSUPPORTED_EXPR.example = const size: int = 4

CONST_EVAL_E_OVERFLOW.summary = estouro.
CONST_EVAL_E_OVERFLOW.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_EVAL_E_OVERFLOW.step1 = Reduce the constant expression at the reported span.
CONST_EVAL_E_OVERFLOW.fix = use a wider const type or reduce the arithmetic result before overflow
CONST_EVAL_E_OVERFLOW.example = const size: int = 4

CONST_EVAL_E_STATIC_ASSERT_FAILED.summary = static assercao falhou.
CONST_EVAL_E_STATIC_ASSERT_FAILED.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_EVAL_E_STATIC_ASSERT_FAILED.step1 = Reduce the constant expression at the reported span.
CONST_EVAL_E_STATIC_ASSERT_FAILED.fix = make the asserted const condition true or remove the invalid compile-time assumption
CONST_EVAL_E_STATIC_ASSERT_FAILED.example = const size: int = 4

CONST_EVAL_E_CYCLE.summary = ciclo.
CONST_EVAL_E_CYCLE.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_EVAL_E_CYCLE.step1 = Reduce the constant expression at the reported span.
CONST_EVAL_E_CYCLE.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_EVAL_E_CYCLE.example = const size: int = 4

CONST_EVAL_E_PARSE.summary = parse.
CONST_EVAL_E_PARSE.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_EVAL_E_PARSE.step1 = Reduce the constant expression at the reported span.
CONST_EVAL_E_PARSE.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_EVAL_E_PARSE.example = const size: int = 4

CONST_EVAL_E_UNKNOWN.summary = desconhecido.
CONST_EVAL_E_UNKNOWN.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_EVAL_E_UNKNOWN.step1 = Reduce the constant expression at the reported span.
CONST_EVAL_E_UNKNOWN.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_EVAL_E_UNKNOWN.example = const size: int = 4

MOD_E_MODULE_NOT_FOUND.summary = modulo ausente.
MOD_E_MODULE_NOT_FOUND.cause = The general phase found code that violates this diagnostic rule.
MOD_E_MODULE_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
MOD_E_MODULE_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
MOD_E_MODULE_NOT_FOUND.example = vitte check path/to/file.vit

MOD_E_IMPORT_CYCLE.summary = importacao ciclo detected.
MOD_E_IMPORT_CYCLE.cause = The general phase found code that violates this diagnostic rule.
MOD_E_IMPORT_CYCLE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MOD_E_IMPORT_CYCLE.fix = repair the highlighted compiler contract before checking later diagnostics
MOD_E_IMPORT_CYCLE.example = vitte check path/to/file.vit

MOD_E_SYMBOL_NOT_EXPORTED.summary = simbolo is nao exportado por modulo.
MOD_E_SYMBOL_NOT_EXPORTED.cause = The general phase found code that violates this diagnostic rule.
MOD_E_SYMBOL_NOT_EXPORTED.step1 = Fix the first span reported for this diagnostic, then run the command again.
MOD_E_SYMBOL_NOT_EXPORTED.fix = repair the highlighted compiler contract before checking later diagnostics
MOD_E_SYMBOL_NOT_EXPORTED.example = vitte check path/to/file.vit

MOD_E_IMPORT_NOT_FOUND.summary = importacao ausente.
MOD_E_IMPORT_NOT_FOUND.cause = The general phase found code that violates this diagnostic rule.
MOD_E_IMPORT_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
MOD_E_IMPORT_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
MOD_E_IMPORT_NOT_FOUND.example = vitte check path/to/file.vit

MOD_E_PACKAGE_MISSING.summary = package ausente.
MOD_E_PACKAGE_MISSING.cause = The general phase found code that violates this diagnostic rule.
MOD_E_PACKAGE_MISSING.step1 = Fix the first span reported for this diagnostic, then run the command again.
MOD_E_PACKAGE_MISSING.fix = repair the highlighted compiler contract before checking later diagnostics
MOD_E_PACKAGE_MISSING.example = vitte check path/to/file.vit

MOD_E_STDLIB_MISSING.summary = stdlib ausente.
MOD_E_STDLIB_MISSING.cause = The general phase found code that violates this diagnostic rule.
MOD_E_STDLIB_MISSING.step1 = Fix the first span reported for this diagnostic, then run the command again.
MOD_E_STDLIB_MISSING.fix = repair the highlighted compiler contract before checking later diagnostics
MOD_E_STDLIB_MISSING.example = vitte check path/to/file.vit

MOD_E_AMBIGUOUS_MODULE.summary = ambiguous modulo caminho.
MOD_E_AMBIGUOUS_MODULE.cause = The general phase found code that violates this diagnostic rule.
MOD_E_AMBIGUOUS_MODULE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MOD_E_AMBIGUOUS_MODULE.fix = repair the highlighted compiler contract before checking later diagnostics
MOD_E_AMBIGUOUS_MODULE.example = vitte check path/to/file.vit

HIR_E_INVALID_EXPR.summary = expr invalido.
HIR_E_INVALID_EXPR.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_EXPR.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_EXPR.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_EXPR.example = vitte check path/to/file.vit

HIR_E_INVALID_STMT.summary = stmt invalido.
HIR_E_INVALID_STMT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_STMT.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_STMT.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_STMT.example = vitte check path/to/file.vit

HIR_E_INVALID_PATTERN.summary = padrao invalido.
HIR_E_INVALID_PATTERN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_PATTERN.example = vitte check path/to/file.vit

HIR_E_INVALID_TYPE.summary = tipo invalido.
HIR_E_INVALID_TYPE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_TYPE.example = vitte check path/to/file.vit

HIR_E_MISSING_SYMBOL.summary = simbolo ausente.
HIR_E_MISSING_SYMBOL.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_MISSING_SYMBOL.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_MISSING_SYMBOL.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_MISSING_SYMBOL.example = vitte check path/to/file.vit

HIR_E_INVALID_CONTROL_FLOW.summary = control fluxo invalido.
HIR_E_INVALID_CONTROL_FLOW.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_CONTROL_FLOW.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_CONTROL_FLOW.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_CONTROL_FLOW.example = vitte check path/to/file.vit

HIR_E_LOWERING_FAILED.summary = lowering falhou.
HIR_E_LOWERING_FAILED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_LOWERING_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_LOWERING_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_LOWERING_FAILED.example = vitte check path/to/file.vit

MIR_E_INVALID_BLOCK.summary = bloco invalido.
MIR_E_INVALID_BLOCK.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_BLOCK.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_BLOCK.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_BLOCK.example = vitte check path/to/file.vit

MIR_E_INVALID_TERMINATOR.summary = terminador invalido.
MIR_E_INVALID_TERMINATOR.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_TERMINATOR.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_TERMINATOR.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_TERMINATOR.example = vitte check path/to/file.vit

MIR_E_UNREACHABLE_BLOCK.summary = inalcancavel bloco.
MIR_E_UNREACHABLE_BLOCK.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_UNREACHABLE_BLOCK.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_UNREACHABLE_BLOCK.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_UNREACHABLE_BLOCK.example = vitte check path/to/file.vit

MIR_E_INVALID_OPERAND.summary = operando invalido.
MIR_E_INVALID_OPERAND.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_OPERAND.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_OPERAND.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_OPERAND.example = vitte check path/to/file.vit

MIR_E_INVALID_PLACE.summary = place invalido.
MIR_E_INVALID_PLACE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_PLACE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_PLACE.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_PLACE.example = vitte check path/to/file.vit

MIR_E_DATAFLOW_CONFLICT.summary = dataflow conflito.
MIR_E_DATAFLOW_CONFLICT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_DATAFLOW_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_DATAFLOW_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_DATAFLOW_CONFLICT.example = vitte check path/to/file.vit

MIR_E_VERIFICATION_FAILED.summary = verification falhou.
MIR_E_VERIFICATION_FAILED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_VERIFICATION_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_VERIFICATION_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_VERIFICATION_FAILED.example = vitte check path/to/file.vit

IR_E_INVALID_MODULE.summary = modulo invalido.
IR_E_INVALID_MODULE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_MODULE.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_MODULE.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_MODULE.example = vitte check path/to/file.vit

IR_E_INVALID_FUNCTION.summary = function invalido.
IR_E_INVALID_FUNCTION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_FUNCTION.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_FUNCTION.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_FUNCTION.example = vitte check path/to/file.vit

IR_E_INVALID_BLOCK.summary = bloco invalido.
IR_E_INVALID_BLOCK.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_BLOCK.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_BLOCK.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_BLOCK.example = vitte check path/to/file.vit

IR_E_INVALID_INSTRUCTION.summary = instrucao invalido.
IR_E_INVALID_INSTRUCTION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_INSTRUCTION.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_INSTRUCTION.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_INSTRUCTION.example = vitte check path/to/file.vit

IR_E_TYPE_MISMATCH.summary = tipo incompatibilidade.
IR_E_TYPE_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_TYPE_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_TYPE_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_TYPE_MISMATCH.example = vitte check path/to/file.vit

IR_E_VERIFY_FAILED.summary = verify falhou.
IR_E_VERIFY_FAILED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_VERIFY_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_VERIFY_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_VERIFY_FAILED.example = vitte check path/to/file.vit

BACKEND_E_UNSUPPORTED_TARGET.summary = destino nao suportado.
BACKEND_E_UNSUPPORTED_TARGET.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_UNSUPPORTED_TARGET.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_UNSUPPORTED_TARGET.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_UNSUPPORTED_TARGET.example = vitte build app.vit -o app

BACKEND_E_UNSUPPORTED_FEATURE.summary = recurso nao suportado.
BACKEND_E_UNSUPPORTED_FEATURE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_UNSUPPORTED_FEATURE.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_UNSUPPORTED_FEATURE.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_UNSUPPORTED_FEATURE.example = vitte build app.vit -o app

BACKEND_E_CODEGEN_FAILED.summary = geracao de codigo falhou.
BACKEND_E_CODEGEN_FAILED.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_CODEGEN_FAILED.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_CODEGEN_FAILED.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_CODEGEN_FAILED.example = vitte build app.vit -o app

BACKEND_E_OBJECT_WRITE_FAILED.summary = objeto escrita falhou.
BACKEND_E_OBJECT_WRITE_FAILED.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_OBJECT_WRITE_FAILED.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_OBJECT_WRITE_FAILED.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_OBJECT_WRITE_FAILED.example = vitte build app.vit -o app

BACKEND_E_ASSEMBLER_FAILED.summary = assembler falhou.
BACKEND_E_ASSEMBLER_FAILED.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_ASSEMBLER_FAILED.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_ASSEMBLER_FAILED.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_ASSEMBLER_FAILED.example = vitte build app.vit -o app

BACKEND_E_ABI_MISMATCH.summary = abi incompatibilidade.
BACKEND_E_ABI_MISMATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_ABI_MISMATCH.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_ABI_MISMATCH.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_ABI_MISMATCH.example = vitte build app.vit -o app

BACKEND_E_NATIVE_TOOL_MISSING.summary = native ferramenta ausente.
BACKEND_E_NATIVE_TOOL_MISSING.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_NATIVE_TOOL_MISSING.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_NATIVE_TOOL_MISSING.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_NATIVE_TOOL_MISSING.example = vitte build app.vit -o app

LINK_E_UNDEFINED_SYMBOL.summary = indefinido simbolo.
LINK_E_UNDEFINED_SYMBOL.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_UNDEFINED_SYMBOL.step1 = Check the target triple and the first backend or linker note.
LINK_E_UNDEFINED_SYMBOL.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_UNDEFINED_SYMBOL.example = vitte build app.vit -o app

LINK_E_DUPLICATE_SYMBOL.summary = simbolo duplicado.
LINK_E_DUPLICATE_SYMBOL.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_DUPLICATE_SYMBOL.step1 = Check the target triple and the first backend or linker note.
LINK_E_DUPLICATE_SYMBOL.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_DUPLICATE_SYMBOL.example = vitte build app.vit -o app

LINK_E_LIBRARY_NOT_FOUND.summary = library ausente.
LINK_E_LIBRARY_NOT_FOUND.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_LIBRARY_NOT_FOUND.step1 = Check the target triple and the first backend or linker note.
LINK_E_LIBRARY_NOT_FOUND.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_LIBRARY_NOT_FOUND.example = vitte build app.vit -o app

LINK_E_OBJECT_NOT_FOUND.summary = objeto ausente.
LINK_E_OBJECT_NOT_FOUND.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_OBJECT_NOT_FOUND.step1 = Check the target triple and the first backend or linker note.
LINK_E_OBJECT_NOT_FOUND.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_OBJECT_NOT_FOUND.example = vitte build app.vit -o app

LINK_E_UNSUPPORTED_FORMAT.summary = formato nao suportado.
LINK_E_UNSUPPORTED_FORMAT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_UNSUPPORTED_FORMAT.step1 = Check the target triple and the first backend or linker note.
LINK_E_UNSUPPORTED_FORMAT.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_UNSUPPORTED_FORMAT.example = vitte build app.vit -o app

LINK_E_SYSTEM_LINKER_FAILED.summary = system linker falhou.
LINK_E_SYSTEM_LINKER_FAILED.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_SYSTEM_LINKER_FAILED.step1 = Check the target triple and the first backend or linker note.
LINK_E_SYSTEM_LINKER_FAILED.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_SYSTEM_LINKER_FAILED.example = vitte build app.vit -o app

RUNTIME_E_PANIC.summary = panico.
RUNTIME_E_PANIC.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_PANIC.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_PANIC.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_PANIC.example = vitte check path/to/file.vit

RUNTIME_E_ASSERT_FAILED.summary = assercao falhou.
RUNTIME_E_ASSERT_FAILED.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_ASSERT_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_ASSERT_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_ASSERT_FAILED.example = vitte check path/to/file.vit

RUNTIME_E_STACK_OVERFLOW.summary = pilha estouro.
RUNTIME_E_STACK_OVERFLOW.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_STACK_OVERFLOW.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_STACK_OVERFLOW.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_STACK_OVERFLOW.example = vitte check path/to/file.vit

RUNTIME_E_OUT_OF_MEMORY.summary = out of memory.
RUNTIME_E_OUT_OF_MEMORY.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_OUT_OF_MEMORY.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_OUT_OF_MEMORY.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_OUT_OF_MEMORY.example = vitte check path/to/file.vit

RUNTIME_E_DIVISION_BY_ZERO.summary = divisao by zero.
RUNTIME_E_DIVISION_BY_ZERO.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_DIVISION_BY_ZERO.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_DIVISION_BY_ZERO.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_DIVISION_BY_ZERO.example = vitte check path/to/file.vit

RUNTIME_E_BOUNDS_CHECK.summary = bounds check.
RUNTIME_E_BOUNDS_CHECK.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_BOUNDS_CHECK.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_BOUNDS_CHECK.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_BOUNDS_CHECK.example = vitte check path/to/file.vit

RUNTIME_E_NULL_DEREF.summary = nulo deref.
RUNTIME_E_NULL_DEREF.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_NULL_DEREF.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_NULL_DEREF.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_NULL_DEREF.example = vitte check path/to/file.vit

DRIVER_E_INVALID_ARGUMENT.summary = argumento invalido.
DRIVER_E_INVALID_ARGUMENT.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_ARGUMENT.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_ARGUMENT.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_ARGUMENT.example = vitte check src/main.vit --lang en

DRIVER_E_MISSING_INPUT.summary = entrada ausente.
DRIVER_E_MISSING_INPUT.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_MISSING_INPUT.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_MISSING_INPUT.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_MISSING_INPUT.example = vitte check src/main.vit --lang en

DRIVER_E_INPUT_NOT_FOUND.summary = entrada ausente.
DRIVER_E_INPUT_NOT_FOUND.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INPUT_NOT_FOUND.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INPUT_NOT_FOUND.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INPUT_NOT_FOUND.example = vitte check src/main.vit --lang en

DRIVER_E_OUTPUT_WRITE_FAILED.summary = saida escrita falhou.
DRIVER_E_OUTPUT_WRITE_FAILED.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_OUTPUT_WRITE_FAILED.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_OUTPUT_WRITE_FAILED.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_OUTPUT_WRITE_FAILED.example = vitte check src/main.vit --lang en

DRIVER_E_CACHE_READ_FAILED.summary = cache read falhou.
DRIVER_E_CACHE_READ_FAILED.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_CACHE_READ_FAILED.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_CACHE_READ_FAILED.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_CACHE_READ_FAILED.example = vitte check src/main.vit --lang en

DRIVER_E_CACHE_WRITE_FAILED.summary = cache escrita falhou.
DRIVER_E_CACHE_WRITE_FAILED.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_CACHE_WRITE_FAILED.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_CACHE_WRITE_FAILED.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_CACHE_WRITE_FAILED.example = vitte check src/main.vit --lang en

DRIVER_E_PROFILE_NOT_FOUND.summary = perfil ausente.
DRIVER_E_PROFILE_NOT_FOUND.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_PROFILE_NOT_FOUND.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_PROFILE_NOT_FOUND.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_PROFILE_NOT_FOUND.example = vitte check src/main.vit --lang en

DRIVER_E_TARGET_NOT_FOUND.summary = destino ausente.
DRIVER_E_TARGET_NOT_FOUND.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_TARGET_NOT_FOUND.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_TARGET_NOT_FOUND.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_TARGET_NOT_FOUND.example = vitte check src/main.vit --lang en

BOOTSTRAP_E_STAGE_FAILURE.summary = stage failure.
BOOTSTRAP_E_STAGE_FAILURE.cause = The bootstrap phase found code that violates this diagnostic rule.
BOOTSTRAP_E_STAGE_FAILURE.step1 = Fix the first span reported for this diagnostic, then run the command again.
BOOTSTRAP_E_STAGE_FAILURE.fix = repair the highlighted compiler contract before checking later diagnostics
BOOTSTRAP_E_STAGE_FAILURE.example = vitte check path/to/file.vit

BOOTSTRAP_E_SEED_MISSING.summary = seed ausente.
BOOTSTRAP_E_SEED_MISSING.cause = The bootstrap phase found code that violates this diagnostic rule.
BOOTSTRAP_E_SEED_MISSING.step1 = Fix the first span reported for this diagnostic, then run the command again.
BOOTSTRAP_E_SEED_MISSING.fix = repair the highlighted compiler contract before checking later diagnostics
BOOTSTRAP_E_SEED_MISSING.example = vitte check path/to/file.vit

BOOTSTRAP_E_COMPILER_MISSING.summary = compiler ausente.
BOOTSTRAP_E_COMPILER_MISSING.cause = The bootstrap phase found code that violates this diagnostic rule.
BOOTSTRAP_E_COMPILER_MISSING.step1 = Fix the first span reported for this diagnostic, then run the command again.
BOOTSTRAP_E_COMPILER_MISSING.fix = repair the highlighted compiler contract before checking later diagnostics
BOOTSTRAP_E_COMPILER_MISSING.example = vitte check path/to/file.vit

BOOTSTRAP_E_SELF_CHECK_FAILED.summary = self check falhou.
BOOTSTRAP_E_SELF_CHECK_FAILED.cause = The bootstrap phase found code that violates this diagnostic rule.
BOOTSTRAP_E_SELF_CHECK_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
BOOTSTRAP_E_SELF_CHECK_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
BOOTSTRAP_E_SELF_CHECK_FAILED.example = vitte check path/to/file.vit

BOOTSTRAP_E_ARTIFACT_INVALID.summary = artifact invalido.
BOOTSTRAP_E_ARTIFACT_INVALID.cause = The bootstrap phase found code that violates this diagnostic rule.
BOOTSTRAP_E_ARTIFACT_INVALID.step1 = Fix the first span reported for this diagnostic, then run the command again.
BOOTSTRAP_E_ARTIFACT_INVALID.fix = repair the highlighted compiler contract before checking later diagnostics
BOOTSTRAP_E_ARTIFACT_INVALID.example = vitte check path/to/file.vit

E_BOOTSTRAP_CONST_TYPE.summary = bootstrap constante has wrong tipo.
E_BOOTSTRAP_CONST_TYPE.cause = The general phase found code that violates this diagnostic rule.
E_BOOTSTRAP_CONST_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
E_BOOTSTRAP_CONST_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
E_BOOTSTRAP_CONST_TYPE.example = vitte check path/to/file.vit

E_BOOTSTRAP_DUP_PROC.summary = bootstrap procedure duplicado.
E_BOOTSTRAP_DUP_PROC.cause = The general phase found code that violates this diagnostic rule.
E_BOOTSTRAP_DUP_PROC.step1 = Fix the first span reported for this diagnostic, then run the command again.
E_BOOTSTRAP_DUP_PROC.fix = repair the highlighted compiler contract before checking later diagnostics
E_BOOTSTRAP_DUP_PROC.example = vitte check path/to/file.vit

E_BOOTSTRAP_UNKNOWN_CONST.summary = bootstrap constante desconhecido.
E_BOOTSTRAP_UNKNOWN_CONST.cause = The general phase found code that violates this diagnostic rule.
E_BOOTSTRAP_UNKNOWN_CONST.step1 = Fix the first span reported for this diagnostic, then run the command again.
E_BOOTSTRAP_UNKNOWN_CONST.fix = repair the highlighted compiler contract before checking later diagnostics
E_BOOTSTRAP_UNKNOWN_CONST.example = vitte check path/to/file.vit

E_BOOTSTRAP_UNKNOWN_PROC.summary = bootstrap procedure nao suportado.
E_BOOTSTRAP_UNKNOWN_PROC.cause = The general phase found code that violates this diagnostic rule.
E_BOOTSTRAP_UNKNOWN_PROC.step1 = Fix the first span reported for this diagnostic, then run the command again.
E_BOOTSTRAP_UNKNOWN_PROC.fix = repair the highlighted compiler contract before checking later diagnostics
E_BOOTSTRAP_UNKNOWN_PROC.example = vitte check path/to/file.vit

LIMIT_FILE_SIZE_MAX.summary = arquivo size max.
LIMIT_FILE_SIZE_MAX.cause = The input exceeded a configured compiler safety limit.
LIMIT_FILE_SIZE_MAX.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_FILE_SIZE_MAX.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_FILE_SIZE_MAX.example = vitte check src/main.vit

LIMIT_TOKEN_SIZE_MAX.summary = token size max.
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

LIMIT_IMPORT_DEPTH_MAX.summary = importacao depth max.
LIMIT_IMPORT_DEPTH_MAX.cause = The input exceeded a configured compiler safety limit.
LIMIT_IMPORT_DEPTH_MAX.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_IMPORT_DEPTH_MAX.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_IMPORT_DEPTH_MAX.example = vitte check src/main.vit

LIMIT_MODULE_COUNT_MAX.summary = modulo count max.
LIMIT_MODULE_COUNT_MAX.cause = The input exceeded a configured compiler safety limit.
LIMIT_MODULE_COUNT_MAX.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_MODULE_COUNT_MAX.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_MODULE_COUNT_MAX.example = vitte check src/main.vit

LIMIT_DIAGNOSTICS_MAX.summary = demais many diagnosticos emitted.
LIMIT_DIAGNOSTICS_MAX.cause = The input exceeded a configured compiler safety limit.
LIMIT_DIAGNOSTICS_MAX.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_DIAGNOSTICS_MAX.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_DIAGNOSTICS_MAX.example = vitte check src/main.vit

LIMIT_SYMBOL_COUNT_MAX.summary = simbolo count max.
LIMIT_SYMBOL_COUNT_MAX.cause = The input exceeded a configured compiler safety limit.
LIMIT_SYMBOL_COUNT_MAX.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_SYMBOL_COUNT_MAX.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_SYMBOL_COUNT_MAX.example = vitte check src/main.vit

LIMIT_PARSER_RECURSION_MAX.summary = parser recursion max.
LIMIT_PARSER_RECURSION_MAX.cause = The input exceeded a configured compiler safety limit.
LIMIT_PARSER_RECURSION_MAX.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_PARSER_RECURSION_MAX.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_PARSER_RECURSION_MAX.example = vitte check src/main.vit

LIMIT_MACRO_EXPANSION_MAX.summary = macro expansion max.
LIMIT_MACRO_EXPANSION_MAX.cause = The input exceeded a configured compiler safety limit.
LIMIT_MACRO_EXPANSION_MAX.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_MACRO_EXPANSION_MAX.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_MACRO_EXPANSION_MAX.example = vitte check src/main.vit

MACRO_E_EXPANSION_FAILED.summary = expansion falhou.
MACRO_E_EXPANSION_FAILED.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_EXPANSION_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_EXPANSION_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_EXPANSION_FAILED.example = vitte check path/to/file.vit

MACRO_E_RECURSION_LIMIT.summary = recursion limit.
MACRO_E_RECURSION_LIMIT.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_RECURSION_LIMIT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_RECURSION_LIMIT.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_RECURSION_LIMIT.example = vitte check path/to/file.vit

MACRO_E_INVALID_ARGUMENT.summary = argumento invalido.
MACRO_E_INVALID_ARGUMENT.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_ARGUMENT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_ARGUMENT.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_ARGUMENT.example = vitte check path/to/file.vit

MACRO_E_UNKNOWN_MACRO.summary = macro desconhecido.
MACRO_E_UNKNOWN_MACRO.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_UNKNOWN_MACRO.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_UNKNOWN_MACRO.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_UNKNOWN_MACRO.example = vitte check path/to/file.vit

MACRO_E_UNSTABLE_FEATURE.summary = unstable recurso.
MACRO_E_UNSTABLE_FEATURE.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_UNSTABLE_FEATURE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_UNSTABLE_FEATURE.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_UNSTABLE_FEATURE.example = vitte check path/to/file.vit

SYNTAX_E_EXPECTED_IDENTIFIER.summary = identificador esperado.
SYNTAX_E_EXPECTED_IDENTIFIER.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_EXPECTED_IDENTIFIER.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_EXPECTED_IDENTIFIER.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_EXPECTED_IDENTIFIER.example = proc main() -> int { give 0; }

SYNTAX_E_EXPECTED_EXPRESSION.summary = expressao esperado.
SYNTAX_E_EXPECTED_EXPRESSION.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_EXPECTED_EXPRESSION.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_EXPECTED_EXPRESSION.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_EXPECTED_EXPRESSION.example = proc main() -> int { give 0; }

SYNTAX_E_EXPECTED_TYPE.summary = tipo esperado.
SYNTAX_E_EXPECTED_TYPE.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_EXPECTED_TYPE.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_EXPECTED_TYPE.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_EXPECTED_TYPE.example = proc main() -> int { give 0; }

SYNTAX_E_EXPECTED_PATTERN.summary = padrao esperado.
SYNTAX_E_EXPECTED_PATTERN.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_EXPECTED_PATTERN.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_EXPECTED_PATTERN.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_EXPECTED_PATTERN.example = proc main() -> int { give 0; }

SYNTAX_E_EXPECTED_BLOCK.summary = bloco esperado.
SYNTAX_E_EXPECTED_BLOCK.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_EXPECTED_BLOCK.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_EXPECTED_BLOCK.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_EXPECTED_BLOCK.example = proc main() -> int { give 0; }

SYNTAX_E_EXPECTED_DELIMITER.summary = delimitador esperado.
SYNTAX_E_EXPECTED_DELIMITER.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_EXPECTED_DELIMITER.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_EXPECTED_DELIMITER.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_EXPECTED_DELIMITER.example = proc main() -> int { give 0; }

SYNTAX_E_UNEXPECTED_TOKEN.summary = token inesperado.
SYNTAX_E_UNEXPECTED_TOKEN.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_UNEXPECTED_TOKEN.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_UNEXPECTED_TOKEN.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_UNEXPECTED_TOKEN.example = proc main() -> int { give 0; }

SYNTAX_E_UNBALANCED_DELIMITER.summary = unbalanced delimitador.
SYNTAX_E_UNBALANCED_DELIMITER.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_UNBALANCED_DELIMITER.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_UNBALANCED_DELIMITER.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_UNBALANCED_DELIMITER.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_ATTRIBUTE.summary = attribute invalido.
SYNTAX_E_INVALID_ATTRIBUTE.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_ATTRIBUTE.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_ATTRIBUTE.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_ATTRIBUTE.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_DECLARATION.summary = declaracao invalido.
SYNTAX_E_INVALID_DECLARATION.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_DECLARATION.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_DECLARATION.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_DECLARATION.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_STATEMENT.summary = instrucao invalido.
SYNTAX_E_INVALID_STATEMENT.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_STATEMENT.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_STATEMENT.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_STATEMENT.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_EXPRESSION.summary = expressao invalido.
SYNTAX_E_INVALID_EXPRESSION.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_EXPRESSION.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_EXPRESSION.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_EXPRESSION.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_PATTERN.summary = padrao invalido.
SYNTAX_E_INVALID_PATTERN.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_PATTERN.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_PATTERN.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_PATTERN.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_LITERAL.summary = literal invalido.
SYNTAX_E_INVALID_LITERAL.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_LITERAL.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_LITERAL.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_LITERAL.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_OPERATOR.summary = operador invalido.
SYNTAX_E_INVALID_OPERATOR.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_OPERATOR.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_OPERATOR.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_OPERATOR.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_MODIFIER.summary = modifier invalido.
SYNTAX_E_INVALID_MODIFIER.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_MODIFIER.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_MODIFIER.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_MODIFIER.example = proc main() -> int { give 0; }

SYNTAX_E_MISSING_BODY.summary = body ausente.
SYNTAX_E_MISSING_BODY.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_MISSING_BODY.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_MISSING_BODY.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_MISSING_BODY.example = proc main() -> int { give 0; }

SYNTAX_E_MISSING_RETURN.summary = retorno ausente.
SYNTAX_E_MISSING_RETURN.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_MISSING_RETURN.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_MISSING_RETURN.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_MISSING_RETURN.example = proc main() -> int { give 0; }

SYNTAX_E_DUPLICATE_NAME.summary = nome duplicado.
SYNTAX_E_DUPLICATE_NAME.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_DUPLICATE_NAME.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_DUPLICATE_NAME.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_DUPLICATE_NAME.example = proc main() -> int { give 0; }

SYNTAX_E_UNKNOWN_NAME.summary = nome desconhecido.
SYNTAX_E_UNKNOWN_NAME.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_UNKNOWN_NAME.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_UNKNOWN_NAME.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_UNKNOWN_NAME.example = proc main() -> int { give 0; }

SYNTAX_E_UNKNOWN_TYPE.summary = tipo desconhecido.
SYNTAX_E_UNKNOWN_TYPE.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_UNKNOWN_TYPE.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_UNKNOWN_TYPE.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_UNKNOWN_TYPE.example = proc main() -> int { give 0; }

SYNTAX_E_UNKNOWN_MODULE.summary = modulo desconhecido.
SYNTAX_E_UNKNOWN_MODULE.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_UNKNOWN_MODULE.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_UNKNOWN_MODULE.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_UNKNOWN_MODULE.example = proc main() -> int { give 0; }

SYNTAX_E_UNKNOWN_MEMBER.summary = membro desconhecido.
SYNTAX_E_UNKNOWN_MEMBER.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_UNKNOWN_MEMBER.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_UNKNOWN_MEMBER.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_UNKNOWN_MEMBER.example = proc main() -> int { give 0; }

SYNTAX_E_AMBIGUOUS_NAME.summary = ambiguous nome.
SYNTAX_E_AMBIGUOUS_NAME.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_AMBIGUOUS_NAME.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_AMBIGUOUS_NAME.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_AMBIGUOUS_NAME.example = proc main() -> int { give 0; }

SYNTAX_E_PRIVATE_SYMBOL.summary = private simbolo.
SYNTAX_E_PRIVATE_SYMBOL.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_PRIVATE_SYMBOL.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_PRIVATE_SYMBOL.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_PRIVATE_SYMBOL.example = proc main() -> int { give 0; }

SYNTAX_E_IMPORT_NOT_FOUND.summary = importacao ausente.
SYNTAX_E_IMPORT_NOT_FOUND.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_IMPORT_NOT_FOUND.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_IMPORT_NOT_FOUND.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_IMPORT_NOT_FOUND.example = proc main() -> int { give 0; }

SYNTAX_E_IMPORT_CYCLE.summary = importacao ciclo.
SYNTAX_E_IMPORT_CYCLE.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_IMPORT_CYCLE.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_IMPORT_CYCLE.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_IMPORT_CYCLE.example = proc main() -> int { give 0; }

SYNTAX_E_EXPORT_CONFLICT.summary = export conflito.
SYNTAX_E_EXPORT_CONFLICT.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_EXPORT_CONFLICT.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_EXPORT_CONFLICT.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_EXPORT_CONFLICT.example = proc main() -> int { give 0; }

SYNTAX_E_ARITY_MISMATCH.summary = arity incompatibilidade.
SYNTAX_E_ARITY_MISMATCH.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_ARITY_MISMATCH.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_ARITY_MISMATCH.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_ARITY_MISMATCH.example = proc main() -> int { give 0; }

SYNTAX_E_ARGUMENT_MISMATCH.summary = argumento incompatibilidade.
SYNTAX_E_ARGUMENT_MISMATCH.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_ARGUMENT_MISMATCH.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_ARGUMENT_MISMATCH.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_ARGUMENT_MISMATCH.example = proc main() -> int { give 0; }

SYNTAX_E_ASSIGNMENT_MISMATCH.summary = atribuicao incompatibilidade.
SYNTAX_E_ASSIGNMENT_MISMATCH.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_ASSIGNMENT_MISMATCH.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_ASSIGNMENT_MISMATCH.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_ASSIGNMENT_MISMATCH.example = proc main() -> int { give 0; }

SYNTAX_E_BRANCH_MISMATCH.summary = branch incompatibilidade.
SYNTAX_E_BRANCH_MISMATCH.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_BRANCH_MISMATCH.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_BRANCH_MISMATCH.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_BRANCH_MISMATCH.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_CALL.summary = chamada invalido.
SYNTAX_E_INVALID_CALL.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_CALL.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_CALL.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_CALL.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_CAST.summary = conversao invalido.
SYNTAX_E_INVALID_CAST.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_CAST.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_CAST.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_CAST.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_INDEX.summary = indice invalido.
SYNTAX_E_INVALID_INDEX.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_INDEX.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_INDEX.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_INDEX.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_DEREF.summary = deref invalido.
SYNTAX_E_INVALID_DEREF.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_DEREF.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_DEREF.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_DEREF.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_BORROW.summary = emprestimo invalido.
SYNTAX_E_INVALID_BORROW.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_BORROW.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_BORROW.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_BORROW.example = proc main() -> int { give 0; }

SYNTAX_E_INVALID_MOVE.summary = movimento invalido.
SYNTAX_E_INVALID_MOVE.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_INVALID_MOVE.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_INVALID_MOVE.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_INVALID_MOVE.example = proc main() -> int { give 0; }

SYNTAX_E_USE_AFTER_MOVE.summary = use after movimento.
SYNTAX_E_USE_AFTER_MOVE.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_USE_AFTER_MOVE.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_USE_AFTER_MOVE.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_USE_AFTER_MOVE.example = proc main() -> int { give 0; }

SYNTAX_E_USE_AFTER_DROP.summary = use after descarte.
SYNTAX_E_USE_AFTER_DROP.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_USE_AFTER_DROP.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_USE_AFTER_DROP.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_USE_AFTER_DROP.example = proc main() -> int { give 0; }

SYNTAX_E_USE_BEFORE_INIT.summary = use before init.
SYNTAX_E_USE_BEFORE_INIT.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_USE_BEFORE_INIT.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_USE_BEFORE_INIT.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_USE_BEFORE_INIT.example = proc main() -> int { give 0; }

SYNTAX_E_DOUBLE_DROP.summary = double descarte.
SYNTAX_E_DOUBLE_DROP.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_DOUBLE_DROP.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_DOUBLE_DROP.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_DOUBLE_DROP.example = proc main() -> int { give 0; }

SYNTAX_E_BORROW_CONFLICT.summary = emprestimo conflito.
SYNTAX_E_BORROW_CONFLICT.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_BORROW_CONFLICT.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_BORROW_CONFLICT.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_BORROW_CONFLICT.example = proc main() -> int { give 0; }

SYNTAX_E_MUTABILITY_CONFLICT.summary = mutability conflito.
SYNTAX_E_MUTABILITY_CONFLICT.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_MUTABILITY_CONFLICT.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_MUTABILITY_CONFLICT.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_MUTABILITY_CONFLICT.example = proc main() -> int { give 0; }

SYNTAX_E_LIFETIME_TOO_SHORT.summary = tempo de vida demais short.
SYNTAX_E_LIFETIME_TOO_SHORT.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_LIFETIME_TOO_SHORT.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_LIFETIME_TOO_SHORT.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_LIFETIME_TOO_SHORT.example = proc main() -> int { give 0; }

SYNTAX_E_DANGLING_REFERENCE.summary = dangling referencia.
SYNTAX_E_DANGLING_REFERENCE.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_DANGLING_REFERENCE.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_DANGLING_REFERENCE.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_DANGLING_REFERENCE.example = proc main() -> int { give 0; }

SYNTAX_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
SYNTAX_E_NON_EXHAUSTIVE_MATCH.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_NON_EXHAUSTIVE_MATCH.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_NON_EXHAUSTIVE_MATCH.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_NON_EXHAUSTIVE_MATCH.example = proc main() -> int { give 0; }

SYNTAX_E_UNREACHABLE_PATTERN.summary = inalcancavel padrao.
SYNTAX_E_UNREACHABLE_PATTERN.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_UNREACHABLE_PATTERN.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_UNREACHABLE_PATTERN.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_UNREACHABLE_PATTERN.example = proc main() -> int { give 0; }

SYNTAX_E_CONST_REQUIRED.summary = const required.
SYNTAX_E_CONST_REQUIRED.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_CONST_REQUIRED.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_CONST_REQUIRED.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_CONST_REQUIRED.example = proc main() -> int { give 0; }

SYNTAX_E_CONST_OVERFLOW.summary = const estouro.
SYNTAX_E_CONST_OVERFLOW.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_CONST_OVERFLOW.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_CONST_OVERFLOW.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_CONST_OVERFLOW.example = proc main() -> int { give 0; }

SYNTAX_E_CONST_DIVISION_BY_ZERO.summary = const divisao by zero.
SYNTAX_E_CONST_DIVISION_BY_ZERO.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_CONST_DIVISION_BY_ZERO.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_CONST_DIVISION_BY_ZERO.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_CONST_DIVISION_BY_ZERO.example = proc main() -> int { give 0; }

SYNTAX_E_CONST_CYCLE.summary = const ciclo.
SYNTAX_E_CONST_CYCLE.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_CONST_CYCLE.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_CONST_CYCLE.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_CONST_CYCLE.example = proc main() -> int { give 0; }

SYNTAX_E_MACRO_NOT_FOUND.summary = macro ausente.
SYNTAX_E_MACRO_NOT_FOUND.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_MACRO_NOT_FOUND.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_MACRO_NOT_FOUND.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_MACRO_NOT_FOUND.example = proc main() -> int { give 0; }

SYNTAX_E_MACRO_RECURSION.summary = macro recursion.
SYNTAX_E_MACRO_RECURSION.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_MACRO_RECURSION.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_MACRO_RECURSION.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_MACRO_RECURSION.example = proc main() -> int { give 0; }

SYNTAX_E_MACRO_EXPANSION_FAILED.summary = macro expansion falhou.
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

SYNTAX_E_GENERIC_ARGUMENT_MISSING.summary = generico argumento ausente.
SYNTAX_E_GENERIC_ARGUMENT_MISSING.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_GENERIC_ARGUMENT_MISSING.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_GENERIC_ARGUMENT_MISSING.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_GENERIC_ARGUMENT_MISSING.example = proc main() -> int { give 0; }

SYNTAX_E_GENERIC_BOUND_FAILED.summary = generico bound falhou.
SYNTAX_E_GENERIC_BOUND_FAILED.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_GENERIC_BOUND_FAILED.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_GENERIC_BOUND_FAILED.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_GENERIC_BOUND_FAILED.example = proc main() -> int { give 0; }

SYNTAX_E_UNSUPPORTED_TARGET.summary = destino nao suportado.
SYNTAX_E_UNSUPPORTED_TARGET.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_UNSUPPORTED_TARGET.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_UNSUPPORTED_TARGET.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_UNSUPPORTED_TARGET.example = proc main() -> int { give 0; }

SYNTAX_E_ABI_MISMATCH.summary = abi incompatibilidade.
SYNTAX_E_ABI_MISMATCH.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_ABI_MISMATCH.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_ABI_MISMATCH.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_ABI_MISMATCH.example = proc main() -> int { give 0; }

SYNTAX_E_LINK_FAILED.summary = link falhou.
SYNTAX_E_LINK_FAILED.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_LINK_FAILED.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_LINK_FAILED.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_LINK_FAILED.example = proc main() -> int { give 0; }

SYNTAX_E_RUNTIME_PANIC.summary = runtime panico.
SYNTAX_E_RUNTIME_PANIC.cause = The parser or lexer could not form the next valid source construct.
SYNTAX_E_RUNTIME_PANIC.step1 = Look at the highlighted token and complete or remove the construct around it.
SYNTAX_E_RUNTIME_PANIC.fix = complete the syntax shape named by the parser label at the highlighted token
SYNTAX_E_RUNTIME_PANIC.example = proc main() -> int { give 0; }

NAME_E_EXPECTED_IDENTIFIER.summary = identificador esperado.
NAME_E_EXPECTED_IDENTIFIER.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_EXPECTED_IDENTIFIER.step1 = Check the spelling and the nearest import or declaration.
NAME_E_EXPECTED_IDENTIFIER.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_EXPECTED_IDENTIFIER.example = use vitte/core

NAME_E_EXPECTED_EXPRESSION.summary = expressao esperado.
NAME_E_EXPECTED_EXPRESSION.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_EXPECTED_EXPRESSION.step1 = Check the spelling and the nearest import or declaration.
NAME_E_EXPECTED_EXPRESSION.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_EXPECTED_EXPRESSION.example = use vitte/core

NAME_E_EXPECTED_TYPE.summary = tipo esperado.
NAME_E_EXPECTED_TYPE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_EXPECTED_TYPE.step1 = Check the spelling and the nearest import or declaration.
NAME_E_EXPECTED_TYPE.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_EXPECTED_TYPE.example = use vitte/core

NAME_E_EXPECTED_PATTERN.summary = padrao esperado.
NAME_E_EXPECTED_PATTERN.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_EXPECTED_PATTERN.step1 = Check the spelling and the nearest import or declaration.
NAME_E_EXPECTED_PATTERN.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_EXPECTED_PATTERN.example = use vitte/core

NAME_E_EXPECTED_BLOCK.summary = bloco esperado.
NAME_E_EXPECTED_BLOCK.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_EXPECTED_BLOCK.step1 = Check the spelling and the nearest import or declaration.
NAME_E_EXPECTED_BLOCK.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_EXPECTED_BLOCK.example = use vitte/core

NAME_E_EXPECTED_DELIMITER.summary = delimitador esperado.
NAME_E_EXPECTED_DELIMITER.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_EXPECTED_DELIMITER.step1 = Check the spelling and the nearest import or declaration.
NAME_E_EXPECTED_DELIMITER.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_EXPECTED_DELIMITER.example = use vitte/core

NAME_E_UNEXPECTED_TOKEN.summary = token inesperado.
NAME_E_UNEXPECTED_TOKEN.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_UNEXPECTED_TOKEN.step1 = Check the spelling and the nearest import or declaration.
NAME_E_UNEXPECTED_TOKEN.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_UNEXPECTED_TOKEN.example = use vitte/core

NAME_E_UNBALANCED_DELIMITER.summary = unbalanced delimitador.
NAME_E_UNBALANCED_DELIMITER.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_UNBALANCED_DELIMITER.step1 = Check the spelling and the nearest import or declaration.
NAME_E_UNBALANCED_DELIMITER.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_UNBALANCED_DELIMITER.example = use vitte/core

NAME_E_INVALID_ATTRIBUTE.summary = attribute invalido.
NAME_E_INVALID_ATTRIBUTE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_ATTRIBUTE.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_ATTRIBUTE.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_ATTRIBUTE.example = use vitte/core

NAME_E_INVALID_DECLARATION.summary = declaracao invalido.
NAME_E_INVALID_DECLARATION.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_DECLARATION.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_DECLARATION.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_DECLARATION.example = use vitte/core

NAME_E_INVALID_STATEMENT.summary = instrucao invalido.
NAME_E_INVALID_STATEMENT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_STATEMENT.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_STATEMENT.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_STATEMENT.example = use vitte/core

NAME_E_INVALID_EXPRESSION.summary = expressao invalido.
NAME_E_INVALID_EXPRESSION.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_EXPRESSION.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_EXPRESSION.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_EXPRESSION.example = use vitte/core

NAME_E_INVALID_PATTERN.summary = padrao invalido.
NAME_E_INVALID_PATTERN.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_PATTERN.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_PATTERN.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_PATTERN.example = use vitte/core

NAME_E_INVALID_LITERAL.summary = literal invalido.
NAME_E_INVALID_LITERAL.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_LITERAL.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_LITERAL.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_LITERAL.example = use vitte/core

NAME_E_INVALID_OPERATOR.summary = operador invalido.
NAME_E_INVALID_OPERATOR.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_OPERATOR.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_OPERATOR.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_OPERATOR.example = use vitte/core

NAME_E_INVALID_MODIFIER.summary = modifier invalido.
NAME_E_INVALID_MODIFIER.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_MODIFIER.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_MODIFIER.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_MODIFIER.example = use vitte/core

NAME_E_MISSING_BODY.summary = body ausente.
NAME_E_MISSING_BODY.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_MISSING_BODY.step1 = Check the spelling and the nearest import or declaration.
NAME_E_MISSING_BODY.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_MISSING_BODY.example = use vitte/core

NAME_E_MISSING_RETURN.summary = retorno ausente.
NAME_E_MISSING_RETURN.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_MISSING_RETURN.step1 = Check the spelling and the nearest import or declaration.
NAME_E_MISSING_RETURN.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_MISSING_RETURN.example = use vitte/core

NAME_E_DUPLICATE_NAME.summary = nome duplicado.
NAME_E_DUPLICATE_NAME.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_DUPLICATE_NAME.step1 = Check the spelling and the nearest import or declaration.
NAME_E_DUPLICATE_NAME.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_DUPLICATE_NAME.example = use vitte/core

NAME_E_UNKNOWN_NAME.summary = nome desconhecido.
NAME_E_UNKNOWN_NAME.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_UNKNOWN_NAME.step1 = Check the spelling and the nearest import or declaration.
NAME_E_UNKNOWN_NAME.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_UNKNOWN_NAME.example = use vitte/core

NAME_E_UNKNOWN_TYPE.summary = tipo desconhecido.
NAME_E_UNKNOWN_TYPE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_UNKNOWN_TYPE.step1 = Check the spelling and the nearest import or declaration.
NAME_E_UNKNOWN_TYPE.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_UNKNOWN_TYPE.example = use vitte/core

NAME_E_UNKNOWN_MODULE.summary = modulo desconhecido.
NAME_E_UNKNOWN_MODULE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_UNKNOWN_MODULE.step1 = Check the spelling and the nearest import or declaration.
NAME_E_UNKNOWN_MODULE.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_UNKNOWN_MODULE.example = use vitte/core

NAME_E_UNKNOWN_MEMBER.summary = membro desconhecido.
NAME_E_UNKNOWN_MEMBER.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_UNKNOWN_MEMBER.step1 = Check the spelling and the nearest import or declaration.
NAME_E_UNKNOWN_MEMBER.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_UNKNOWN_MEMBER.example = use vitte/core

NAME_E_AMBIGUOUS_NAME.summary = ambiguous nome.
NAME_E_AMBIGUOUS_NAME.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_AMBIGUOUS_NAME.step1 = Check the spelling and the nearest import or declaration.
NAME_E_AMBIGUOUS_NAME.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_AMBIGUOUS_NAME.example = use vitte/core

NAME_E_PRIVATE_SYMBOL.summary = private simbolo.
NAME_E_PRIVATE_SYMBOL.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_PRIVATE_SYMBOL.step1 = Check the spelling and the nearest import or declaration.
NAME_E_PRIVATE_SYMBOL.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_PRIVATE_SYMBOL.example = use vitte/core

NAME_E_IMPORT_NOT_FOUND.summary = importacao ausente.
NAME_E_IMPORT_NOT_FOUND.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_IMPORT_NOT_FOUND.step1 = Check the spelling and the nearest import or declaration.
NAME_E_IMPORT_NOT_FOUND.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_IMPORT_NOT_FOUND.example = use vitte/core

NAME_E_IMPORT_CYCLE.summary = importacao ciclo.
NAME_E_IMPORT_CYCLE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_IMPORT_CYCLE.step1 = Check the spelling and the nearest import or declaration.
NAME_E_IMPORT_CYCLE.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_IMPORT_CYCLE.example = use vitte/core

NAME_E_EXPORT_CONFLICT.summary = export conflito.
NAME_E_EXPORT_CONFLICT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_EXPORT_CONFLICT.step1 = Check the spelling and the nearest import or declaration.
NAME_E_EXPORT_CONFLICT.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_EXPORT_CONFLICT.example = use vitte/core

NAME_E_ARITY_MISMATCH.summary = arity incompatibilidade.
NAME_E_ARITY_MISMATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_ARITY_MISMATCH.step1 = Check the spelling and the nearest import or declaration.
NAME_E_ARITY_MISMATCH.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_ARITY_MISMATCH.example = use vitte/core

NAME_E_ARGUMENT_MISMATCH.summary = argumento incompatibilidade.
NAME_E_ARGUMENT_MISMATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_ARGUMENT_MISMATCH.step1 = Check the spelling and the nearest import or declaration.
NAME_E_ARGUMENT_MISMATCH.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_ARGUMENT_MISMATCH.example = use vitte/core

NAME_E_ASSIGNMENT_MISMATCH.summary = atribuicao incompatibilidade.
NAME_E_ASSIGNMENT_MISMATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_ASSIGNMENT_MISMATCH.step1 = Check the spelling and the nearest import or declaration.
NAME_E_ASSIGNMENT_MISMATCH.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_ASSIGNMENT_MISMATCH.example = use vitte/core

NAME_E_BRANCH_MISMATCH.summary = branch incompatibilidade.
NAME_E_BRANCH_MISMATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_BRANCH_MISMATCH.step1 = Check the spelling and the nearest import or declaration.
NAME_E_BRANCH_MISMATCH.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_BRANCH_MISMATCH.example = use vitte/core

NAME_E_INVALID_CALL.summary = chamada invalido.
NAME_E_INVALID_CALL.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_CALL.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_CALL.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_CALL.example = use vitte/core

NAME_E_INVALID_CAST.summary = conversao invalido.
NAME_E_INVALID_CAST.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_CAST.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_CAST.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_CAST.example = use vitte/core

NAME_E_INVALID_INDEX.summary = indice invalido.
NAME_E_INVALID_INDEX.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_INDEX.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_INDEX.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_INDEX.example = use vitte/core

NAME_E_INVALID_DEREF.summary = deref invalido.
NAME_E_INVALID_DEREF.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_DEREF.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_DEREF.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_DEREF.example = use vitte/core

NAME_E_INVALID_BORROW.summary = emprestimo invalido.
NAME_E_INVALID_BORROW.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_BORROW.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_BORROW.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_BORROW.example = use vitte/core

NAME_E_INVALID_MOVE.summary = movimento invalido.
NAME_E_INVALID_MOVE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_INVALID_MOVE.step1 = Check the spelling and the nearest import or declaration.
NAME_E_INVALID_MOVE.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_INVALID_MOVE.example = use vitte/core

NAME_E_USE_AFTER_MOVE.summary = use after movimento.
NAME_E_USE_AFTER_MOVE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_USE_AFTER_MOVE.step1 = Check the spelling and the nearest import or declaration.
NAME_E_USE_AFTER_MOVE.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_USE_AFTER_MOVE.example = use vitte/core

NAME_E_USE_AFTER_DROP.summary = use after descarte.
NAME_E_USE_AFTER_DROP.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_USE_AFTER_DROP.step1 = Check the spelling and the nearest import or declaration.
NAME_E_USE_AFTER_DROP.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_USE_AFTER_DROP.example = use vitte/core

NAME_E_USE_BEFORE_INIT.summary = use before init.
NAME_E_USE_BEFORE_INIT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_USE_BEFORE_INIT.step1 = Check the spelling and the nearest import or declaration.
NAME_E_USE_BEFORE_INIT.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_USE_BEFORE_INIT.example = use vitte/core

NAME_E_DOUBLE_DROP.summary = double descarte.
NAME_E_DOUBLE_DROP.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_DOUBLE_DROP.step1 = Check the spelling and the nearest import or declaration.
NAME_E_DOUBLE_DROP.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_DOUBLE_DROP.example = use vitte/core

NAME_E_BORROW_CONFLICT.summary = emprestimo conflito.
NAME_E_BORROW_CONFLICT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_BORROW_CONFLICT.step1 = Check the spelling and the nearest import or declaration.
NAME_E_BORROW_CONFLICT.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_BORROW_CONFLICT.example = use vitte/core

NAME_E_MUTABILITY_CONFLICT.summary = mutability conflito.
NAME_E_MUTABILITY_CONFLICT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_MUTABILITY_CONFLICT.step1 = Check the spelling and the nearest import or declaration.
NAME_E_MUTABILITY_CONFLICT.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_MUTABILITY_CONFLICT.example = use vitte/core

NAME_E_LIFETIME_TOO_SHORT.summary = tempo de vida demais short.
NAME_E_LIFETIME_TOO_SHORT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_LIFETIME_TOO_SHORT.step1 = Check the spelling and the nearest import or declaration.
NAME_E_LIFETIME_TOO_SHORT.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_LIFETIME_TOO_SHORT.example = use vitte/core

NAME_E_DANGLING_REFERENCE.summary = dangling referencia.
NAME_E_DANGLING_REFERENCE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_DANGLING_REFERENCE.step1 = Check the spelling and the nearest import or declaration.
NAME_E_DANGLING_REFERENCE.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_DANGLING_REFERENCE.example = use vitte/core

NAME_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
NAME_E_NON_EXHAUSTIVE_MATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_NON_EXHAUSTIVE_MATCH.step1 = Check the spelling and the nearest import or declaration.
NAME_E_NON_EXHAUSTIVE_MATCH.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_NON_EXHAUSTIVE_MATCH.example = use vitte/core

NAME_E_UNREACHABLE_PATTERN.summary = inalcancavel padrao.
NAME_E_UNREACHABLE_PATTERN.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_UNREACHABLE_PATTERN.step1 = Check the spelling and the nearest import or declaration.
NAME_E_UNREACHABLE_PATTERN.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_UNREACHABLE_PATTERN.example = use vitte/core

NAME_E_CONST_REQUIRED.summary = const required.
NAME_E_CONST_REQUIRED.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_CONST_REQUIRED.step1 = Check the spelling and the nearest import or declaration.
NAME_E_CONST_REQUIRED.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_CONST_REQUIRED.example = use vitte/core

NAME_E_CONST_OVERFLOW.summary = const estouro.
NAME_E_CONST_OVERFLOW.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_CONST_OVERFLOW.step1 = Check the spelling and the nearest import or declaration.
NAME_E_CONST_OVERFLOW.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_CONST_OVERFLOW.example = use vitte/core

NAME_E_CONST_DIVISION_BY_ZERO.summary = const divisao by zero.
NAME_E_CONST_DIVISION_BY_ZERO.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_CONST_DIVISION_BY_ZERO.step1 = Check the spelling and the nearest import or declaration.
NAME_E_CONST_DIVISION_BY_ZERO.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_CONST_DIVISION_BY_ZERO.example = use vitte/core

NAME_E_CONST_CYCLE.summary = const ciclo.
NAME_E_CONST_CYCLE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_CONST_CYCLE.step1 = Check the spelling and the nearest import or declaration.
NAME_E_CONST_CYCLE.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_CONST_CYCLE.example = use vitte/core

NAME_E_MACRO_NOT_FOUND.summary = macro ausente.
NAME_E_MACRO_NOT_FOUND.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_MACRO_NOT_FOUND.step1 = Check the spelling and the nearest import or declaration.
NAME_E_MACRO_NOT_FOUND.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_MACRO_NOT_FOUND.example = use vitte/core

NAME_E_MACRO_RECURSION.summary = macro recursion.
NAME_E_MACRO_RECURSION.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_MACRO_RECURSION.step1 = Check the spelling and the nearest import or declaration.
NAME_E_MACRO_RECURSION.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_MACRO_RECURSION.example = use vitte/core

NAME_E_MACRO_EXPANSION_FAILED.summary = macro expansion falhou.
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

NAME_E_GENERIC_ARGUMENT_MISSING.summary = generico argumento ausente.
NAME_E_GENERIC_ARGUMENT_MISSING.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_GENERIC_ARGUMENT_MISSING.step1 = Check the spelling and the nearest import or declaration.
NAME_E_GENERIC_ARGUMENT_MISSING.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_GENERIC_ARGUMENT_MISSING.example = use vitte/core

NAME_E_GENERIC_BOUND_FAILED.summary = generico bound falhou.
NAME_E_GENERIC_BOUND_FAILED.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_GENERIC_BOUND_FAILED.step1 = Check the spelling and the nearest import or declaration.
NAME_E_GENERIC_BOUND_FAILED.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_GENERIC_BOUND_FAILED.example = use vitte/core

NAME_E_UNSUPPORTED_TARGET.summary = destino nao suportado.
NAME_E_UNSUPPORTED_TARGET.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_UNSUPPORTED_TARGET.step1 = Check the spelling and the nearest import or declaration.
NAME_E_UNSUPPORTED_TARGET.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_UNSUPPORTED_TARGET.example = use vitte/core

NAME_E_ABI_MISMATCH.summary = abi incompatibilidade.
NAME_E_ABI_MISMATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_ABI_MISMATCH.step1 = Check the spelling and the nearest import or declaration.
NAME_E_ABI_MISMATCH.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_ABI_MISMATCH.example = use vitte/core

NAME_E_LINK_FAILED.summary = link falhou.
NAME_E_LINK_FAILED.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_LINK_FAILED.step1 = Check the spelling and the nearest import or declaration.
NAME_E_LINK_FAILED.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_LINK_FAILED.example = use vitte/core

NAME_E_RUNTIME_PANIC.summary = runtime panico.
NAME_E_RUNTIME_PANIC.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
NAME_E_RUNTIME_PANIC.step1 = Check the spelling and the nearest import or declaration.
NAME_E_RUNTIME_PANIC.fix = resolve the exact symbol contract reported by semantic analysis
NAME_E_RUNTIME_PANIC.example = use vitte/core

MODULE_E_EXPECTED_IDENTIFIER.summary = identificador esperado.
MODULE_E_EXPECTED_IDENTIFIER.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_EXPECTED_IDENTIFIER.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_EXPECTED_IDENTIFIER.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_EXPECTED_IDENTIFIER.example = use vitte/core

MODULE_E_EXPECTED_EXPRESSION.summary = expressao esperado.
MODULE_E_EXPECTED_EXPRESSION.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_EXPECTED_EXPRESSION.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_EXPECTED_EXPRESSION.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_EXPECTED_EXPRESSION.example = use vitte/core

MODULE_E_EXPECTED_TYPE.summary = tipo esperado.
MODULE_E_EXPECTED_TYPE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_EXPECTED_TYPE.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_EXPECTED_TYPE.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_EXPECTED_TYPE.example = use vitte/core

MODULE_E_EXPECTED_PATTERN.summary = padrao esperado.
MODULE_E_EXPECTED_PATTERN.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_EXPECTED_PATTERN.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_EXPECTED_PATTERN.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_EXPECTED_PATTERN.example = use vitte/core

MODULE_E_EXPECTED_BLOCK.summary = bloco esperado.
MODULE_E_EXPECTED_BLOCK.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_EXPECTED_BLOCK.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_EXPECTED_BLOCK.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_EXPECTED_BLOCK.example = use vitte/core

MODULE_E_EXPECTED_DELIMITER.summary = delimitador esperado.
MODULE_E_EXPECTED_DELIMITER.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_EXPECTED_DELIMITER.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_EXPECTED_DELIMITER.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_EXPECTED_DELIMITER.example = use vitte/core

MODULE_E_UNEXPECTED_TOKEN.summary = token inesperado.
MODULE_E_UNEXPECTED_TOKEN.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_UNEXPECTED_TOKEN.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_UNEXPECTED_TOKEN.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_UNEXPECTED_TOKEN.example = use vitte/core

MODULE_E_UNBALANCED_DELIMITER.summary = unbalanced delimitador.
MODULE_E_UNBALANCED_DELIMITER.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_UNBALANCED_DELIMITER.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_UNBALANCED_DELIMITER.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_UNBALANCED_DELIMITER.example = use vitte/core

MODULE_E_INVALID_ATTRIBUTE.summary = attribute invalido.
MODULE_E_INVALID_ATTRIBUTE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_ATTRIBUTE.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_ATTRIBUTE.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_ATTRIBUTE.example = use vitte/core

MODULE_E_INVALID_DECLARATION.summary = declaracao invalido.
MODULE_E_INVALID_DECLARATION.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_DECLARATION.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_DECLARATION.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_DECLARATION.example = use vitte/core

MODULE_E_INVALID_STATEMENT.summary = instrucao invalido.
MODULE_E_INVALID_STATEMENT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_STATEMENT.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_STATEMENT.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_STATEMENT.example = use vitte/core

MODULE_E_INVALID_EXPRESSION.summary = expressao invalido.
MODULE_E_INVALID_EXPRESSION.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_EXPRESSION.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_EXPRESSION.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_EXPRESSION.example = use vitte/core

MODULE_E_INVALID_PATTERN.summary = padrao invalido.
MODULE_E_INVALID_PATTERN.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_PATTERN.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_PATTERN.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_PATTERN.example = use vitte/core

MODULE_E_INVALID_LITERAL.summary = literal invalido.
MODULE_E_INVALID_LITERAL.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_LITERAL.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_LITERAL.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_LITERAL.example = use vitte/core

MODULE_E_INVALID_OPERATOR.summary = operador invalido.
MODULE_E_INVALID_OPERATOR.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_OPERATOR.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_OPERATOR.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_OPERATOR.example = use vitte/core

MODULE_E_INVALID_MODIFIER.summary = modifier invalido.
MODULE_E_INVALID_MODIFIER.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_MODIFIER.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_MODIFIER.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_MODIFIER.example = use vitte/core

MODULE_E_MISSING_BODY.summary = body ausente.
MODULE_E_MISSING_BODY.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_MISSING_BODY.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_MISSING_BODY.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_MISSING_BODY.example = use vitte/core

MODULE_E_MISSING_RETURN.summary = retorno ausente.
MODULE_E_MISSING_RETURN.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_MISSING_RETURN.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_MISSING_RETURN.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_MISSING_RETURN.example = use vitte/core

MODULE_E_DUPLICATE_NAME.summary = nome duplicado.
MODULE_E_DUPLICATE_NAME.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_DUPLICATE_NAME.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_DUPLICATE_NAME.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_DUPLICATE_NAME.example = use vitte/core

MODULE_E_UNKNOWN_NAME.summary = nome desconhecido.
MODULE_E_UNKNOWN_NAME.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_UNKNOWN_NAME.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_UNKNOWN_NAME.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_UNKNOWN_NAME.example = use vitte/core

MODULE_E_UNKNOWN_TYPE.summary = tipo desconhecido.
MODULE_E_UNKNOWN_TYPE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_UNKNOWN_TYPE.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_UNKNOWN_TYPE.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_UNKNOWN_TYPE.example = use vitte/core

MODULE_E_UNKNOWN_MODULE.summary = modulo desconhecido.
MODULE_E_UNKNOWN_MODULE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_UNKNOWN_MODULE.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_UNKNOWN_MODULE.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_UNKNOWN_MODULE.example = use vitte/core

MODULE_E_UNKNOWN_MEMBER.summary = membro desconhecido.
MODULE_E_UNKNOWN_MEMBER.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_UNKNOWN_MEMBER.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_UNKNOWN_MEMBER.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_UNKNOWN_MEMBER.example = use vitte/core

MODULE_E_AMBIGUOUS_NAME.summary = ambiguous nome.
MODULE_E_AMBIGUOUS_NAME.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_AMBIGUOUS_NAME.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_AMBIGUOUS_NAME.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_AMBIGUOUS_NAME.example = use vitte/core

MODULE_E_PRIVATE_SYMBOL.summary = private simbolo.
MODULE_E_PRIVATE_SYMBOL.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_PRIVATE_SYMBOL.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_PRIVATE_SYMBOL.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_PRIVATE_SYMBOL.example = use vitte/core

MODULE_E_IMPORT_NOT_FOUND.summary = importacao ausente.
MODULE_E_IMPORT_NOT_FOUND.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_IMPORT_NOT_FOUND.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_IMPORT_NOT_FOUND.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_IMPORT_NOT_FOUND.example = use vitte/core

MODULE_E_IMPORT_CYCLE.summary = importacao ciclo.
MODULE_E_IMPORT_CYCLE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_IMPORT_CYCLE.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_IMPORT_CYCLE.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_IMPORT_CYCLE.example = use vitte/core

MODULE_E_EXPORT_CONFLICT.summary = export conflito.
MODULE_E_EXPORT_CONFLICT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_EXPORT_CONFLICT.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_EXPORT_CONFLICT.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_EXPORT_CONFLICT.example = use vitte/core

MODULE_E_ARITY_MISMATCH.summary = arity incompatibilidade.
MODULE_E_ARITY_MISMATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_ARITY_MISMATCH.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_ARITY_MISMATCH.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_ARITY_MISMATCH.example = use vitte/core

MODULE_E_ARGUMENT_MISMATCH.summary = argumento incompatibilidade.
MODULE_E_ARGUMENT_MISMATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_ARGUMENT_MISMATCH.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_ARGUMENT_MISMATCH.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_ARGUMENT_MISMATCH.example = use vitte/core

MODULE_E_ASSIGNMENT_MISMATCH.summary = atribuicao incompatibilidade.
MODULE_E_ASSIGNMENT_MISMATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_ASSIGNMENT_MISMATCH.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_ASSIGNMENT_MISMATCH.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_ASSIGNMENT_MISMATCH.example = use vitte/core

MODULE_E_BRANCH_MISMATCH.summary = branch incompatibilidade.
MODULE_E_BRANCH_MISMATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_BRANCH_MISMATCH.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_BRANCH_MISMATCH.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_BRANCH_MISMATCH.example = use vitte/core

MODULE_E_INVALID_CALL.summary = chamada invalido.
MODULE_E_INVALID_CALL.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_CALL.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_CALL.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_CALL.example = use vitte/core

MODULE_E_INVALID_CAST.summary = conversao invalido.
MODULE_E_INVALID_CAST.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_CAST.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_CAST.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_CAST.example = use vitte/core

MODULE_E_INVALID_INDEX.summary = indice invalido.
MODULE_E_INVALID_INDEX.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_INDEX.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_INDEX.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_INDEX.example = use vitte/core

MODULE_E_INVALID_DEREF.summary = deref invalido.
MODULE_E_INVALID_DEREF.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_DEREF.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_DEREF.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_DEREF.example = use vitte/core

MODULE_E_INVALID_BORROW.summary = emprestimo invalido.
MODULE_E_INVALID_BORROW.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_BORROW.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_BORROW.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_BORROW.example = use vitte/core

MODULE_E_INVALID_MOVE.summary = movimento invalido.
MODULE_E_INVALID_MOVE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_INVALID_MOVE.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_INVALID_MOVE.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_INVALID_MOVE.example = use vitte/core

MODULE_E_USE_AFTER_MOVE.summary = use after movimento.
MODULE_E_USE_AFTER_MOVE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_USE_AFTER_MOVE.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_USE_AFTER_MOVE.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_USE_AFTER_MOVE.example = use vitte/core

MODULE_E_USE_AFTER_DROP.summary = use after descarte.
MODULE_E_USE_AFTER_DROP.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_USE_AFTER_DROP.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_USE_AFTER_DROP.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_USE_AFTER_DROP.example = use vitte/core

MODULE_E_USE_BEFORE_INIT.summary = use before init.
MODULE_E_USE_BEFORE_INIT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_USE_BEFORE_INIT.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_USE_BEFORE_INIT.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_USE_BEFORE_INIT.example = use vitte/core

MODULE_E_DOUBLE_DROP.summary = double descarte.
MODULE_E_DOUBLE_DROP.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_DOUBLE_DROP.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_DOUBLE_DROP.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_DOUBLE_DROP.example = use vitte/core

MODULE_E_BORROW_CONFLICT.summary = emprestimo conflito.
MODULE_E_BORROW_CONFLICT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_BORROW_CONFLICT.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_BORROW_CONFLICT.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_BORROW_CONFLICT.example = use vitte/core

MODULE_E_MUTABILITY_CONFLICT.summary = mutability conflito.
MODULE_E_MUTABILITY_CONFLICT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_MUTABILITY_CONFLICT.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_MUTABILITY_CONFLICT.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_MUTABILITY_CONFLICT.example = use vitte/core

MODULE_E_LIFETIME_TOO_SHORT.summary = tempo de vida demais short.
MODULE_E_LIFETIME_TOO_SHORT.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_LIFETIME_TOO_SHORT.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_LIFETIME_TOO_SHORT.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_LIFETIME_TOO_SHORT.example = use vitte/core

MODULE_E_DANGLING_REFERENCE.summary = dangling referencia.
MODULE_E_DANGLING_REFERENCE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_DANGLING_REFERENCE.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_DANGLING_REFERENCE.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_DANGLING_REFERENCE.example = use vitte/core

MODULE_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
MODULE_E_NON_EXHAUSTIVE_MATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_NON_EXHAUSTIVE_MATCH.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_NON_EXHAUSTIVE_MATCH.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_NON_EXHAUSTIVE_MATCH.example = use vitte/core

MODULE_E_UNREACHABLE_PATTERN.summary = inalcancavel padrao.
MODULE_E_UNREACHABLE_PATTERN.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_UNREACHABLE_PATTERN.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_UNREACHABLE_PATTERN.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_UNREACHABLE_PATTERN.example = use vitte/core

MODULE_E_CONST_REQUIRED.summary = const required.
MODULE_E_CONST_REQUIRED.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_CONST_REQUIRED.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_CONST_REQUIRED.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_CONST_REQUIRED.example = use vitte/core

MODULE_E_CONST_OVERFLOW.summary = const estouro.
MODULE_E_CONST_OVERFLOW.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_CONST_OVERFLOW.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_CONST_OVERFLOW.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_CONST_OVERFLOW.example = use vitte/core

MODULE_E_CONST_DIVISION_BY_ZERO.summary = const divisao by zero.
MODULE_E_CONST_DIVISION_BY_ZERO.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_CONST_DIVISION_BY_ZERO.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_CONST_DIVISION_BY_ZERO.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_CONST_DIVISION_BY_ZERO.example = use vitte/core

MODULE_E_CONST_CYCLE.summary = const ciclo.
MODULE_E_CONST_CYCLE.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_CONST_CYCLE.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_CONST_CYCLE.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_CONST_CYCLE.example = use vitte/core

MODULE_E_MACRO_NOT_FOUND.summary = macro ausente.
MODULE_E_MACRO_NOT_FOUND.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_MACRO_NOT_FOUND.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_MACRO_NOT_FOUND.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_MACRO_NOT_FOUND.example = use vitte/core

MODULE_E_MACRO_RECURSION.summary = macro recursion.
MODULE_E_MACRO_RECURSION.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_MACRO_RECURSION.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_MACRO_RECURSION.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_MACRO_RECURSION.example = use vitte/core

MODULE_E_MACRO_EXPANSION_FAILED.summary = macro expansion falhou.
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

MODULE_E_GENERIC_ARGUMENT_MISSING.summary = generico argumento ausente.
MODULE_E_GENERIC_ARGUMENT_MISSING.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_GENERIC_ARGUMENT_MISSING.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_GENERIC_ARGUMENT_MISSING.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_GENERIC_ARGUMENT_MISSING.example = use vitte/core

MODULE_E_GENERIC_BOUND_FAILED.summary = generico bound falhou.
MODULE_E_GENERIC_BOUND_FAILED.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_GENERIC_BOUND_FAILED.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_GENERIC_BOUND_FAILED.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_GENERIC_BOUND_FAILED.example = use vitte/core

MODULE_E_UNSUPPORTED_TARGET.summary = destino nao suportado.
MODULE_E_UNSUPPORTED_TARGET.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_UNSUPPORTED_TARGET.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_UNSUPPORTED_TARGET.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_UNSUPPORTED_TARGET.example = use vitte/core

MODULE_E_ABI_MISMATCH.summary = abi incompatibilidade.
MODULE_E_ABI_MISMATCH.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_ABI_MISMATCH.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_ABI_MISMATCH.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_ABI_MISMATCH.example = use vitte/core

MODULE_E_LINK_FAILED.summary = link falhou.
MODULE_E_LINK_FAILED.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_LINK_FAILED.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_LINK_FAILED.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_LINK_FAILED.example = use vitte/core

MODULE_E_RUNTIME_PANIC.summary = runtime panico.
MODULE_E_RUNTIME_PANIC.cause = A symbol, module path, binding, visibility rule, or import contract did not resolve.
MODULE_E_RUNTIME_PANIC.step1 = Check the spelling and the nearest import or declaration.
MODULE_E_RUNTIME_PANIC.fix = resolve the exact symbol contract reported by semantic analysis
MODULE_E_RUNTIME_PANIC.example = use vitte/core

TYPE_E_EXPECTED_IDENTIFIER.summary = identificador esperado.
TYPE_E_EXPECTED_IDENTIFIER.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_EXPECTED_IDENTIFIER.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_EXPECTED_IDENTIFIER.fix = make the expression type match the type contract named by the type checker
TYPE_E_EXPECTED_IDENTIFIER.example = let count: int = 1

TYPE_E_EXPECTED_EXPRESSION.summary = expressao esperado.
TYPE_E_EXPECTED_EXPRESSION.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_EXPECTED_EXPRESSION.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_EXPECTED_EXPRESSION.fix = make the expression type match the type contract named by the type checker
TYPE_E_EXPECTED_EXPRESSION.example = let count: int = 1

TYPE_E_EXPECTED_TYPE.summary = tipo esperado.
TYPE_E_EXPECTED_TYPE.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_EXPECTED_TYPE.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_EXPECTED_TYPE.fix = make the expression type match the type contract named by the type checker
TYPE_E_EXPECTED_TYPE.example = let count: int = 1

TYPE_E_EXPECTED_PATTERN.summary = padrao esperado.
TYPE_E_EXPECTED_PATTERN.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_EXPECTED_PATTERN.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_EXPECTED_PATTERN.fix = make the expression type match the type contract named by the type checker
TYPE_E_EXPECTED_PATTERN.example = let count: int = 1

TYPE_E_EXPECTED_BLOCK.summary = bloco esperado.
TYPE_E_EXPECTED_BLOCK.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_EXPECTED_BLOCK.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_EXPECTED_BLOCK.fix = make the expression type match the type contract named by the type checker
TYPE_E_EXPECTED_BLOCK.example = let count: int = 1

TYPE_E_EXPECTED_DELIMITER.summary = delimitador esperado.
TYPE_E_EXPECTED_DELIMITER.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_EXPECTED_DELIMITER.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_EXPECTED_DELIMITER.fix = make the expression type match the type contract named by the type checker
TYPE_E_EXPECTED_DELIMITER.example = let count: int = 1

TYPE_E_UNEXPECTED_TOKEN.summary = token inesperado.
TYPE_E_UNEXPECTED_TOKEN.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_UNEXPECTED_TOKEN.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_UNEXPECTED_TOKEN.fix = make the expression type match the type contract named by the type checker
TYPE_E_UNEXPECTED_TOKEN.example = let count: int = 1

TYPE_E_UNBALANCED_DELIMITER.summary = unbalanced delimitador.
TYPE_E_UNBALANCED_DELIMITER.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_UNBALANCED_DELIMITER.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_UNBALANCED_DELIMITER.fix = make the expression type match the type contract named by the type checker
TYPE_E_UNBALANCED_DELIMITER.example = let count: int = 1

TYPE_E_INVALID_ATTRIBUTE.summary = attribute invalido.
TYPE_E_INVALID_ATTRIBUTE.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_ATTRIBUTE.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_ATTRIBUTE.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_ATTRIBUTE.example = let count: int = 1

TYPE_E_INVALID_DECLARATION.summary = declaracao invalido.
TYPE_E_INVALID_DECLARATION.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_DECLARATION.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_DECLARATION.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_DECLARATION.example = let count: int = 1

TYPE_E_INVALID_STATEMENT.summary = instrucao invalido.
TYPE_E_INVALID_STATEMENT.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_STATEMENT.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_STATEMENT.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_STATEMENT.example = let count: int = 1

TYPE_E_INVALID_EXPRESSION.summary = expressao invalido.
TYPE_E_INVALID_EXPRESSION.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_EXPRESSION.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_EXPRESSION.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_EXPRESSION.example = let count: int = 1

TYPE_E_INVALID_PATTERN.summary = padrao invalido.
TYPE_E_INVALID_PATTERN.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_PATTERN.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_PATTERN.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_PATTERN.example = let count: int = 1

TYPE_E_INVALID_LITERAL.summary = literal invalido.
TYPE_E_INVALID_LITERAL.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_LITERAL.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_LITERAL.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_LITERAL.example = let count: int = 1

TYPE_E_INVALID_OPERATOR.summary = operador invalido.
TYPE_E_INVALID_OPERATOR.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_OPERATOR.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_OPERATOR.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_OPERATOR.example = let count: int = 1

TYPE_E_INVALID_MODIFIER.summary = modifier invalido.
TYPE_E_INVALID_MODIFIER.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_MODIFIER.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_MODIFIER.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_MODIFIER.example = let count: int = 1

TYPE_E_MISSING_BODY.summary = body ausente.
TYPE_E_MISSING_BODY.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_MISSING_BODY.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_MISSING_BODY.fix = make the expression type match the type contract named by the type checker
TYPE_E_MISSING_BODY.example = let count: int = 1

TYPE_E_MISSING_RETURN.summary = retorno ausente.
TYPE_E_MISSING_RETURN.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_MISSING_RETURN.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_MISSING_RETURN.fix = make the expression type match the type contract named by the type checker
TYPE_E_MISSING_RETURN.example = let count: int = 1

TYPE_E_DUPLICATE_NAME.summary = nome duplicado.
TYPE_E_DUPLICATE_NAME.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_DUPLICATE_NAME.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_DUPLICATE_NAME.fix = make the expression type match the type contract named by the type checker
TYPE_E_DUPLICATE_NAME.example = let count: int = 1

TYPE_E_UNKNOWN_NAME.summary = nome desconhecido.
TYPE_E_UNKNOWN_NAME.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_UNKNOWN_NAME.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_UNKNOWN_NAME.fix = make the expression type match the type contract named by the type checker
TYPE_E_UNKNOWN_NAME.example = let count: int = 1

TYPE_E_UNKNOWN_TYPE.summary = tipo desconhecido.
TYPE_E_UNKNOWN_TYPE.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_UNKNOWN_TYPE.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_UNKNOWN_TYPE.fix = make the expression type match the type contract named by the type checker
TYPE_E_UNKNOWN_TYPE.example = let count: int = 1

TYPE_E_UNKNOWN_MODULE.summary = modulo desconhecido.
TYPE_E_UNKNOWN_MODULE.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_UNKNOWN_MODULE.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_UNKNOWN_MODULE.fix = make the expression type match the type contract named by the type checker
TYPE_E_UNKNOWN_MODULE.example = let count: int = 1

TYPE_E_UNKNOWN_MEMBER.summary = membro desconhecido.
TYPE_E_UNKNOWN_MEMBER.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_UNKNOWN_MEMBER.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_UNKNOWN_MEMBER.fix = make the expression type match the type contract named by the type checker
TYPE_E_UNKNOWN_MEMBER.example = let count: int = 1

TYPE_E_AMBIGUOUS_NAME.summary = ambiguous nome.
TYPE_E_AMBIGUOUS_NAME.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_AMBIGUOUS_NAME.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_AMBIGUOUS_NAME.fix = make the expression type match the type contract named by the type checker
TYPE_E_AMBIGUOUS_NAME.example = let count: int = 1

TYPE_E_PRIVATE_SYMBOL.summary = private simbolo.
TYPE_E_PRIVATE_SYMBOL.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_PRIVATE_SYMBOL.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_PRIVATE_SYMBOL.fix = make the expression type match the type contract named by the type checker
TYPE_E_PRIVATE_SYMBOL.example = let count: int = 1

TYPE_E_IMPORT_NOT_FOUND.summary = importacao ausente.
TYPE_E_IMPORT_NOT_FOUND.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_IMPORT_NOT_FOUND.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_IMPORT_NOT_FOUND.fix = make the expression type match the type contract named by the type checker
TYPE_E_IMPORT_NOT_FOUND.example = let count: int = 1

TYPE_E_IMPORT_CYCLE.summary = importacao ciclo.
TYPE_E_IMPORT_CYCLE.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_IMPORT_CYCLE.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_IMPORT_CYCLE.fix = make the expression type match the type contract named by the type checker
TYPE_E_IMPORT_CYCLE.example = let count: int = 1

TYPE_E_EXPORT_CONFLICT.summary = export conflito.
TYPE_E_EXPORT_CONFLICT.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_EXPORT_CONFLICT.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_EXPORT_CONFLICT.fix = make the expression type match the type contract named by the type checker
TYPE_E_EXPORT_CONFLICT.example = let count: int = 1

TYPE_E_ARITY_MISMATCH.summary = arity incompatibilidade.
TYPE_E_ARITY_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_ARITY_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_ARITY_MISMATCH.fix = make the expression type match the type contract named by the type checker
TYPE_E_ARITY_MISMATCH.example = let count: int = 1

TYPE_E_ARGUMENT_MISMATCH.summary = argumento incompatibilidade.
TYPE_E_ARGUMENT_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_ARGUMENT_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_ARGUMENT_MISMATCH.fix = make the expression type match the type contract named by the type checker
TYPE_E_ARGUMENT_MISMATCH.example = let count: int = 1

TYPE_E_ASSIGNMENT_MISMATCH.summary = atribuicao incompatibilidade.
TYPE_E_ASSIGNMENT_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_ASSIGNMENT_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_ASSIGNMENT_MISMATCH.fix = make the expression type match the type contract named by the type checker
TYPE_E_ASSIGNMENT_MISMATCH.example = let count: int = 1

TYPE_E_BRANCH_MISMATCH.summary = branch incompatibilidade.
TYPE_E_BRANCH_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_BRANCH_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_BRANCH_MISMATCH.fix = make the expression type match the type contract named by the type checker
TYPE_E_BRANCH_MISMATCH.example = let count: int = 1

TYPE_E_INVALID_CALL.summary = chamada invalido.
TYPE_E_INVALID_CALL.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_CALL.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_CALL.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_CALL.example = let count: int = 1

TYPE_E_INVALID_CAST.summary = conversao invalido.
TYPE_E_INVALID_CAST.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_CAST.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_CAST.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_CAST.example = let count: int = 1

TYPE_E_INVALID_INDEX.summary = indice invalido.
TYPE_E_INVALID_INDEX.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_INDEX.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_INDEX.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_INDEX.example = let count: int = 1

TYPE_E_INVALID_DEREF.summary = deref invalido.
TYPE_E_INVALID_DEREF.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_DEREF.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_DEREF.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_DEREF.example = let count: int = 1

TYPE_E_INVALID_BORROW.summary = emprestimo invalido.
TYPE_E_INVALID_BORROW.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_BORROW.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_BORROW.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_BORROW.example = let count: int = 1

TYPE_E_INVALID_MOVE.summary = movimento invalido.
TYPE_E_INVALID_MOVE.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_INVALID_MOVE.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_INVALID_MOVE.fix = make the expression type match the type contract named by the type checker
TYPE_E_INVALID_MOVE.example = let count: int = 1

TYPE_E_USE_AFTER_MOVE.summary = use after movimento.
TYPE_E_USE_AFTER_MOVE.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_USE_AFTER_MOVE.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_USE_AFTER_MOVE.fix = make the expression type match the type contract named by the type checker
TYPE_E_USE_AFTER_MOVE.example = let count: int = 1

TYPE_E_USE_AFTER_DROP.summary = use after descarte.
TYPE_E_USE_AFTER_DROP.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_USE_AFTER_DROP.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_USE_AFTER_DROP.fix = make the expression type match the type contract named by the type checker
TYPE_E_USE_AFTER_DROP.example = let count: int = 1

TYPE_E_USE_BEFORE_INIT.summary = use before init.
TYPE_E_USE_BEFORE_INIT.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_USE_BEFORE_INIT.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_USE_BEFORE_INIT.fix = make the expression type match the type contract named by the type checker
TYPE_E_USE_BEFORE_INIT.example = let count: int = 1

TYPE_E_DOUBLE_DROP.summary = double descarte.
TYPE_E_DOUBLE_DROP.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_DOUBLE_DROP.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_DOUBLE_DROP.fix = make the expression type match the type contract named by the type checker
TYPE_E_DOUBLE_DROP.example = let count: int = 1

TYPE_E_BORROW_CONFLICT.summary = emprestimo conflito.
TYPE_E_BORROW_CONFLICT.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_BORROW_CONFLICT.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_BORROW_CONFLICT.fix = make the expression type match the type contract named by the type checker
TYPE_E_BORROW_CONFLICT.example = let count: int = 1

TYPE_E_MUTABILITY_CONFLICT.summary = mutability conflito.
TYPE_E_MUTABILITY_CONFLICT.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_MUTABILITY_CONFLICT.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_MUTABILITY_CONFLICT.fix = make the expression type match the type contract named by the type checker
TYPE_E_MUTABILITY_CONFLICT.example = let count: int = 1

TYPE_E_LIFETIME_TOO_SHORT.summary = tempo de vida demais short.
TYPE_E_LIFETIME_TOO_SHORT.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_LIFETIME_TOO_SHORT.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_LIFETIME_TOO_SHORT.fix = make the expression type match the type contract named by the type checker
TYPE_E_LIFETIME_TOO_SHORT.example = let count: int = 1

TYPE_E_DANGLING_REFERENCE.summary = dangling referencia.
TYPE_E_DANGLING_REFERENCE.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_DANGLING_REFERENCE.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_DANGLING_REFERENCE.fix = make the expression type match the type contract named by the type checker
TYPE_E_DANGLING_REFERENCE.example = let count: int = 1

TYPE_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
TYPE_E_NON_EXHAUSTIVE_MATCH.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_NON_EXHAUSTIVE_MATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_NON_EXHAUSTIVE_MATCH.fix = make the expression type match the type contract named by the type checker
TYPE_E_NON_EXHAUSTIVE_MATCH.example = let count: int = 1

TYPE_E_UNREACHABLE_PATTERN.summary = inalcancavel padrao.
TYPE_E_UNREACHABLE_PATTERN.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_UNREACHABLE_PATTERN.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_UNREACHABLE_PATTERN.fix = make the expression type match the type contract named by the type checker
TYPE_E_UNREACHABLE_PATTERN.example = let count: int = 1

TYPE_E_CONST_REQUIRED.summary = const required.
TYPE_E_CONST_REQUIRED.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_CONST_REQUIRED.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_CONST_REQUIRED.fix = make the expression type match the type contract named by the type checker
TYPE_E_CONST_REQUIRED.example = let count: int = 1

TYPE_E_CONST_OVERFLOW.summary = const estouro.
TYPE_E_CONST_OVERFLOW.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_CONST_OVERFLOW.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_CONST_OVERFLOW.fix = make the expression type match the type contract named by the type checker
TYPE_E_CONST_OVERFLOW.example = let count: int = 1

TYPE_E_CONST_DIVISION_BY_ZERO.summary = const divisao by zero.
TYPE_E_CONST_DIVISION_BY_ZERO.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_CONST_DIVISION_BY_ZERO.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_CONST_DIVISION_BY_ZERO.fix = make the expression type match the type contract named by the type checker
TYPE_E_CONST_DIVISION_BY_ZERO.example = let count: int = 1

TYPE_E_CONST_CYCLE.summary = const ciclo.
TYPE_E_CONST_CYCLE.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_CONST_CYCLE.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_CONST_CYCLE.fix = make the expression type match the type contract named by the type checker
TYPE_E_CONST_CYCLE.example = let count: int = 1

TYPE_E_MACRO_NOT_FOUND.summary = macro ausente.
TYPE_E_MACRO_NOT_FOUND.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_MACRO_NOT_FOUND.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_MACRO_NOT_FOUND.fix = make the expression type match the type contract named by the type checker
TYPE_E_MACRO_NOT_FOUND.example = let count: int = 1

TYPE_E_MACRO_RECURSION.summary = macro recursion.
TYPE_E_MACRO_RECURSION.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_MACRO_RECURSION.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_MACRO_RECURSION.fix = make the expression type match the type contract named by the type checker
TYPE_E_MACRO_RECURSION.example = let count: int = 1

TYPE_E_MACRO_EXPANSION_FAILED.summary = macro expansion falhou.
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

TYPE_E_GENERIC_ARGUMENT_MISSING.summary = generico argumento ausente.
TYPE_E_GENERIC_ARGUMENT_MISSING.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_GENERIC_ARGUMENT_MISSING.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_GENERIC_ARGUMENT_MISSING.fix = make the expression type match the type contract named by the type checker
TYPE_E_GENERIC_ARGUMENT_MISSING.example = let count: int = 1

TYPE_E_GENERIC_BOUND_FAILED.summary = generico bound falhou.
TYPE_E_GENERIC_BOUND_FAILED.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_GENERIC_BOUND_FAILED.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_GENERIC_BOUND_FAILED.fix = make the expression type match the type contract named by the type checker
TYPE_E_GENERIC_BOUND_FAILED.example = let count: int = 1

TYPE_E_UNSUPPORTED_TARGET.summary = destino nao suportado.
TYPE_E_UNSUPPORTED_TARGET.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_UNSUPPORTED_TARGET.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_UNSUPPORTED_TARGET.fix = make the expression type match the type contract named by the type checker
TYPE_E_UNSUPPORTED_TARGET.example = let count: int = 1

TYPE_E_ABI_MISMATCH.summary = abi incompatibilidade.
TYPE_E_ABI_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_ABI_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_ABI_MISMATCH.fix = make the expression type match the type contract named by the type checker
TYPE_E_ABI_MISMATCH.example = let count: int = 1

TYPE_E_LINK_FAILED.summary = link falhou.
TYPE_E_LINK_FAILED.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_LINK_FAILED.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_LINK_FAILED.fix = make the expression type match the type contract named by the type checker
TYPE_E_LINK_FAILED.example = let count: int = 1

TYPE_E_RUNTIME_PANIC.summary = runtime panico.
TYPE_E_RUNTIME_PANIC.cause = The inferred type does not satisfy the type required at this location.
TYPE_E_RUNTIME_PANIC.step1 = Compare the expected and found types in the diagnostic labels.
TYPE_E_RUNTIME_PANIC.fix = make the expression type match the type contract named by the type checker
TYPE_E_RUNTIME_PANIC.example = let count: int = 1

GENERIC_E_EXPECTED_IDENTIFIER.summary = identificador esperado.
GENERIC_E_EXPECTED_IDENTIFIER.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_EXPECTED_IDENTIFIER.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_EXPECTED_IDENTIFIER.fix = make the expression type match the type contract named by the type checker
GENERIC_E_EXPECTED_IDENTIFIER.example = let count: int = 1

GENERIC_E_EXPECTED_EXPRESSION.summary = expressao esperado.
GENERIC_E_EXPECTED_EXPRESSION.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_EXPECTED_EXPRESSION.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_EXPECTED_EXPRESSION.fix = make the expression type match the type contract named by the type checker
GENERIC_E_EXPECTED_EXPRESSION.example = let count: int = 1

GENERIC_E_EXPECTED_TYPE.summary = tipo esperado.
GENERIC_E_EXPECTED_TYPE.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_EXPECTED_TYPE.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_EXPECTED_TYPE.fix = make the expression type match the type contract named by the type checker
GENERIC_E_EXPECTED_TYPE.example = let count: int = 1

GENERIC_E_EXPECTED_PATTERN.summary = padrao esperado.
GENERIC_E_EXPECTED_PATTERN.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_EXPECTED_PATTERN.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_EXPECTED_PATTERN.fix = make the expression type match the type contract named by the type checker
GENERIC_E_EXPECTED_PATTERN.example = let count: int = 1

GENERIC_E_EXPECTED_BLOCK.summary = bloco esperado.
GENERIC_E_EXPECTED_BLOCK.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_EXPECTED_BLOCK.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_EXPECTED_BLOCK.fix = make the expression type match the type contract named by the type checker
GENERIC_E_EXPECTED_BLOCK.example = let count: int = 1

GENERIC_E_EXPECTED_DELIMITER.summary = delimitador esperado.
GENERIC_E_EXPECTED_DELIMITER.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_EXPECTED_DELIMITER.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_EXPECTED_DELIMITER.fix = make the expression type match the type contract named by the type checker
GENERIC_E_EXPECTED_DELIMITER.example = let count: int = 1

GENERIC_E_UNEXPECTED_TOKEN.summary = token inesperado.
GENERIC_E_UNEXPECTED_TOKEN.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_UNEXPECTED_TOKEN.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_UNEXPECTED_TOKEN.fix = make the expression type match the type contract named by the type checker
GENERIC_E_UNEXPECTED_TOKEN.example = let count: int = 1

GENERIC_E_UNBALANCED_DELIMITER.summary = unbalanced delimitador.
GENERIC_E_UNBALANCED_DELIMITER.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_UNBALANCED_DELIMITER.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_UNBALANCED_DELIMITER.fix = make the expression type match the type contract named by the type checker
GENERIC_E_UNBALANCED_DELIMITER.example = let count: int = 1

GENERIC_E_INVALID_ATTRIBUTE.summary = attribute invalido.
GENERIC_E_INVALID_ATTRIBUTE.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_ATTRIBUTE.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_ATTRIBUTE.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_ATTRIBUTE.example = let count: int = 1

GENERIC_E_INVALID_DECLARATION.summary = declaracao invalido.
GENERIC_E_INVALID_DECLARATION.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_DECLARATION.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_DECLARATION.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_DECLARATION.example = let count: int = 1

GENERIC_E_INVALID_STATEMENT.summary = instrucao invalido.
GENERIC_E_INVALID_STATEMENT.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_STATEMENT.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_STATEMENT.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_STATEMENT.example = let count: int = 1

GENERIC_E_INVALID_EXPRESSION.summary = expressao invalido.
GENERIC_E_INVALID_EXPRESSION.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_EXPRESSION.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_EXPRESSION.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_EXPRESSION.example = let count: int = 1

GENERIC_E_INVALID_PATTERN.summary = padrao invalido.
GENERIC_E_INVALID_PATTERN.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_PATTERN.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_PATTERN.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_PATTERN.example = let count: int = 1

GENERIC_E_INVALID_LITERAL.summary = literal invalido.
GENERIC_E_INVALID_LITERAL.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_LITERAL.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_LITERAL.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_LITERAL.example = let count: int = 1

GENERIC_E_INVALID_OPERATOR.summary = operador invalido.
GENERIC_E_INVALID_OPERATOR.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_OPERATOR.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_OPERATOR.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_OPERATOR.example = let count: int = 1

GENERIC_E_INVALID_MODIFIER.summary = modifier invalido.
GENERIC_E_INVALID_MODIFIER.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_MODIFIER.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_MODIFIER.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_MODIFIER.example = let count: int = 1

GENERIC_E_MISSING_BODY.summary = body ausente.
GENERIC_E_MISSING_BODY.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_MISSING_BODY.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_MISSING_BODY.fix = make the expression type match the type contract named by the type checker
GENERIC_E_MISSING_BODY.example = let count: int = 1

GENERIC_E_MISSING_RETURN.summary = retorno ausente.
GENERIC_E_MISSING_RETURN.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_MISSING_RETURN.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_MISSING_RETURN.fix = make the expression type match the type contract named by the type checker
GENERIC_E_MISSING_RETURN.example = let count: int = 1

GENERIC_E_DUPLICATE_NAME.summary = nome duplicado.
GENERIC_E_DUPLICATE_NAME.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_DUPLICATE_NAME.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_DUPLICATE_NAME.fix = make the expression type match the type contract named by the type checker
GENERIC_E_DUPLICATE_NAME.example = let count: int = 1

GENERIC_E_UNKNOWN_NAME.summary = nome desconhecido.
GENERIC_E_UNKNOWN_NAME.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_UNKNOWN_NAME.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_UNKNOWN_NAME.fix = make the expression type match the type contract named by the type checker
GENERIC_E_UNKNOWN_NAME.example = let count: int = 1

GENERIC_E_UNKNOWN_TYPE.summary = tipo desconhecido.
GENERIC_E_UNKNOWN_TYPE.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_UNKNOWN_TYPE.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_UNKNOWN_TYPE.fix = make the expression type match the type contract named by the type checker
GENERIC_E_UNKNOWN_TYPE.example = let count: int = 1

GENERIC_E_UNKNOWN_MODULE.summary = modulo desconhecido.
GENERIC_E_UNKNOWN_MODULE.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_UNKNOWN_MODULE.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_UNKNOWN_MODULE.fix = make the expression type match the type contract named by the type checker
GENERIC_E_UNKNOWN_MODULE.example = let count: int = 1

GENERIC_E_UNKNOWN_MEMBER.summary = membro desconhecido.
GENERIC_E_UNKNOWN_MEMBER.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_UNKNOWN_MEMBER.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_UNKNOWN_MEMBER.fix = make the expression type match the type contract named by the type checker
GENERIC_E_UNKNOWN_MEMBER.example = let count: int = 1

GENERIC_E_AMBIGUOUS_NAME.summary = ambiguous nome.
GENERIC_E_AMBIGUOUS_NAME.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_AMBIGUOUS_NAME.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_AMBIGUOUS_NAME.fix = make the expression type match the type contract named by the type checker
GENERIC_E_AMBIGUOUS_NAME.example = let count: int = 1

GENERIC_E_PRIVATE_SYMBOL.summary = private simbolo.
GENERIC_E_PRIVATE_SYMBOL.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_PRIVATE_SYMBOL.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_PRIVATE_SYMBOL.fix = make the expression type match the type contract named by the type checker
GENERIC_E_PRIVATE_SYMBOL.example = let count: int = 1

GENERIC_E_IMPORT_NOT_FOUND.summary = importacao ausente.
GENERIC_E_IMPORT_NOT_FOUND.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_IMPORT_NOT_FOUND.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_IMPORT_NOT_FOUND.fix = make the expression type match the type contract named by the type checker
GENERIC_E_IMPORT_NOT_FOUND.example = let count: int = 1

GENERIC_E_IMPORT_CYCLE.summary = importacao ciclo.
GENERIC_E_IMPORT_CYCLE.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_IMPORT_CYCLE.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_IMPORT_CYCLE.fix = make the expression type match the type contract named by the type checker
GENERIC_E_IMPORT_CYCLE.example = let count: int = 1

GENERIC_E_EXPORT_CONFLICT.summary = export conflito.
GENERIC_E_EXPORT_CONFLICT.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_EXPORT_CONFLICT.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_EXPORT_CONFLICT.fix = make the expression type match the type contract named by the type checker
GENERIC_E_EXPORT_CONFLICT.example = let count: int = 1

GENERIC_E_ARITY_MISMATCH.summary = arity incompatibilidade.
GENERIC_E_ARITY_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_ARITY_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_ARITY_MISMATCH.fix = make the expression type match the type contract named by the type checker
GENERIC_E_ARITY_MISMATCH.example = let count: int = 1

GENERIC_E_ARGUMENT_MISMATCH.summary = argumento incompatibilidade.
GENERIC_E_ARGUMENT_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_ARGUMENT_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_ARGUMENT_MISMATCH.fix = make the expression type match the type contract named by the type checker
GENERIC_E_ARGUMENT_MISMATCH.example = let count: int = 1

GENERIC_E_ASSIGNMENT_MISMATCH.summary = atribuicao incompatibilidade.
GENERIC_E_ASSIGNMENT_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_ASSIGNMENT_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_ASSIGNMENT_MISMATCH.fix = make the expression type match the type contract named by the type checker
GENERIC_E_ASSIGNMENT_MISMATCH.example = let count: int = 1

GENERIC_E_BRANCH_MISMATCH.summary = branch incompatibilidade.
GENERIC_E_BRANCH_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_BRANCH_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_BRANCH_MISMATCH.fix = make the expression type match the type contract named by the type checker
GENERIC_E_BRANCH_MISMATCH.example = let count: int = 1

GENERIC_E_INVALID_CALL.summary = chamada invalido.
GENERIC_E_INVALID_CALL.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_CALL.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_CALL.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_CALL.example = let count: int = 1

GENERIC_E_INVALID_CAST.summary = conversao invalido.
GENERIC_E_INVALID_CAST.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_CAST.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_CAST.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_CAST.example = let count: int = 1

GENERIC_E_INVALID_INDEX.summary = indice invalido.
GENERIC_E_INVALID_INDEX.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_INDEX.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_INDEX.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_INDEX.example = let count: int = 1

GENERIC_E_INVALID_DEREF.summary = deref invalido.
GENERIC_E_INVALID_DEREF.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_DEREF.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_DEREF.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_DEREF.example = let count: int = 1

GENERIC_E_INVALID_BORROW.summary = emprestimo invalido.
GENERIC_E_INVALID_BORROW.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_BORROW.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_BORROW.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_BORROW.example = let count: int = 1

GENERIC_E_INVALID_MOVE.summary = movimento invalido.
GENERIC_E_INVALID_MOVE.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_INVALID_MOVE.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_INVALID_MOVE.fix = make the expression type match the type contract named by the type checker
GENERIC_E_INVALID_MOVE.example = let count: int = 1

GENERIC_E_USE_AFTER_MOVE.summary = use after movimento.
GENERIC_E_USE_AFTER_MOVE.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_USE_AFTER_MOVE.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_USE_AFTER_MOVE.fix = make the expression type match the type contract named by the type checker
GENERIC_E_USE_AFTER_MOVE.example = let count: int = 1

GENERIC_E_USE_AFTER_DROP.summary = use after descarte.
GENERIC_E_USE_AFTER_DROP.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_USE_AFTER_DROP.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_USE_AFTER_DROP.fix = make the expression type match the type contract named by the type checker
GENERIC_E_USE_AFTER_DROP.example = let count: int = 1

GENERIC_E_USE_BEFORE_INIT.summary = use before init.
GENERIC_E_USE_BEFORE_INIT.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_USE_BEFORE_INIT.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_USE_BEFORE_INIT.fix = make the expression type match the type contract named by the type checker
GENERIC_E_USE_BEFORE_INIT.example = let count: int = 1

GENERIC_E_DOUBLE_DROP.summary = double descarte.
GENERIC_E_DOUBLE_DROP.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_DOUBLE_DROP.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_DOUBLE_DROP.fix = make the expression type match the type contract named by the type checker
GENERIC_E_DOUBLE_DROP.example = let count: int = 1

GENERIC_E_BORROW_CONFLICT.summary = emprestimo conflito.
GENERIC_E_BORROW_CONFLICT.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_BORROW_CONFLICT.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_BORROW_CONFLICT.fix = make the expression type match the type contract named by the type checker
GENERIC_E_BORROW_CONFLICT.example = let count: int = 1

GENERIC_E_MUTABILITY_CONFLICT.summary = mutability conflito.
GENERIC_E_MUTABILITY_CONFLICT.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_MUTABILITY_CONFLICT.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_MUTABILITY_CONFLICT.fix = make the expression type match the type contract named by the type checker
GENERIC_E_MUTABILITY_CONFLICT.example = let count: int = 1

GENERIC_E_LIFETIME_TOO_SHORT.summary = tempo de vida demais short.
GENERIC_E_LIFETIME_TOO_SHORT.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_LIFETIME_TOO_SHORT.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_LIFETIME_TOO_SHORT.fix = make the expression type match the type contract named by the type checker
GENERIC_E_LIFETIME_TOO_SHORT.example = let count: int = 1

GENERIC_E_DANGLING_REFERENCE.summary = dangling referencia.
GENERIC_E_DANGLING_REFERENCE.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_DANGLING_REFERENCE.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_DANGLING_REFERENCE.fix = make the expression type match the type contract named by the type checker
GENERIC_E_DANGLING_REFERENCE.example = let count: int = 1

GENERIC_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
GENERIC_E_NON_EXHAUSTIVE_MATCH.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_NON_EXHAUSTIVE_MATCH.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_NON_EXHAUSTIVE_MATCH.fix = make the expression type match the type contract named by the type checker
GENERIC_E_NON_EXHAUSTIVE_MATCH.example = let count: int = 1

GENERIC_E_UNREACHABLE_PATTERN.summary = inalcancavel padrao.
GENERIC_E_UNREACHABLE_PATTERN.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_UNREACHABLE_PATTERN.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_UNREACHABLE_PATTERN.fix = make the expression type match the type contract named by the type checker
GENERIC_E_UNREACHABLE_PATTERN.example = let count: int = 1

GENERIC_E_CONST_REQUIRED.summary = const required.
GENERIC_E_CONST_REQUIRED.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_CONST_REQUIRED.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_CONST_REQUIRED.fix = make the expression type match the type contract named by the type checker
GENERIC_E_CONST_REQUIRED.example = let count: int = 1

GENERIC_E_CONST_OVERFLOW.summary = const estouro.
GENERIC_E_CONST_OVERFLOW.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_CONST_OVERFLOW.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_CONST_OVERFLOW.fix = make the expression type match the type contract named by the type checker
GENERIC_E_CONST_OVERFLOW.example = let count: int = 1

GENERIC_E_CONST_DIVISION_BY_ZERO.summary = const divisao by zero.
GENERIC_E_CONST_DIVISION_BY_ZERO.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_CONST_DIVISION_BY_ZERO.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_CONST_DIVISION_BY_ZERO.fix = make the expression type match the type contract named by the type checker
GENERIC_E_CONST_DIVISION_BY_ZERO.example = let count: int = 1

GENERIC_E_CONST_CYCLE.summary = const ciclo.
GENERIC_E_CONST_CYCLE.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_CONST_CYCLE.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_CONST_CYCLE.fix = make the expression type match the type contract named by the type checker
GENERIC_E_CONST_CYCLE.example = let count: int = 1

GENERIC_E_MACRO_NOT_FOUND.summary = macro ausente.
GENERIC_E_MACRO_NOT_FOUND.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_MACRO_NOT_FOUND.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_MACRO_NOT_FOUND.fix = make the expression type match the type contract named by the type checker
GENERIC_E_MACRO_NOT_FOUND.example = let count: int = 1

GENERIC_E_MACRO_RECURSION.summary = macro recursion.
GENERIC_E_MACRO_RECURSION.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_MACRO_RECURSION.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_MACRO_RECURSION.fix = make the expression type match the type contract named by the type checker
GENERIC_E_MACRO_RECURSION.example = let count: int = 1

GENERIC_E_MACRO_EXPANSION_FAILED.summary = macro expansion falhou.
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

GENERIC_E_GENERIC_ARGUMENT_MISSING.summary = generico argumento ausente.
GENERIC_E_GENERIC_ARGUMENT_MISSING.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_GENERIC_ARGUMENT_MISSING.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_GENERIC_ARGUMENT_MISSING.fix = make the expression type match the type contract named by the type checker
GENERIC_E_GENERIC_ARGUMENT_MISSING.example = let count: int = 1

GENERIC_E_GENERIC_BOUND_FAILED.summary = generico bound falhou.
GENERIC_E_GENERIC_BOUND_FAILED.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_GENERIC_BOUND_FAILED.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_GENERIC_BOUND_FAILED.fix = make the expression type match the type contract named by the type checker
GENERIC_E_GENERIC_BOUND_FAILED.example = let count: int = 1

GENERIC_E_UNSUPPORTED_TARGET.summary = destino nao suportado.
GENERIC_E_UNSUPPORTED_TARGET.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_UNSUPPORTED_TARGET.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_UNSUPPORTED_TARGET.fix = make the expression type match the type contract named by the type checker
GENERIC_E_UNSUPPORTED_TARGET.example = let count: int = 1

GENERIC_E_ABI_MISMATCH.summary = abi incompatibilidade.
GENERIC_E_ABI_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_ABI_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_ABI_MISMATCH.fix = make the expression type match the type contract named by the type checker
GENERIC_E_ABI_MISMATCH.example = let count: int = 1

GENERIC_E_LINK_FAILED.summary = link falhou.
GENERIC_E_LINK_FAILED.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_LINK_FAILED.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_LINK_FAILED.fix = make the expression type match the type contract named by the type checker
GENERIC_E_LINK_FAILED.example = let count: int = 1

GENERIC_E_RUNTIME_PANIC.summary = runtime panico.
GENERIC_E_RUNTIME_PANIC.cause = The inferred type does not satisfy the type required at this location.
GENERIC_E_RUNTIME_PANIC.step1 = Compare the expected and found types in the diagnostic labels.
GENERIC_E_RUNTIME_PANIC.fix = make the expression type match the type contract named by the type checker
GENERIC_E_RUNTIME_PANIC.example = let count: int = 1

TRAIT_E_EXPECTED_IDENTIFIER.summary = identificador esperado.
TRAIT_E_EXPECTED_IDENTIFIER.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_EXPECTED_IDENTIFIER.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_EXPECTED_IDENTIFIER.fix = make the expression type match the type contract named by the type checker
TRAIT_E_EXPECTED_IDENTIFIER.example = let count: int = 1

TRAIT_E_EXPECTED_EXPRESSION.summary = expressao esperado.
TRAIT_E_EXPECTED_EXPRESSION.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_EXPECTED_EXPRESSION.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_EXPECTED_EXPRESSION.fix = make the expression type match the type contract named by the type checker
TRAIT_E_EXPECTED_EXPRESSION.example = let count: int = 1

TRAIT_E_EXPECTED_TYPE.summary = tipo esperado.
TRAIT_E_EXPECTED_TYPE.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_EXPECTED_TYPE.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_EXPECTED_TYPE.fix = make the expression type match the type contract named by the type checker
TRAIT_E_EXPECTED_TYPE.example = let count: int = 1

TRAIT_E_EXPECTED_PATTERN.summary = padrao esperado.
TRAIT_E_EXPECTED_PATTERN.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_EXPECTED_PATTERN.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_EXPECTED_PATTERN.fix = make the expression type match the type contract named by the type checker
TRAIT_E_EXPECTED_PATTERN.example = let count: int = 1

TRAIT_E_EXPECTED_BLOCK.summary = bloco esperado.
TRAIT_E_EXPECTED_BLOCK.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_EXPECTED_BLOCK.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_EXPECTED_BLOCK.fix = make the expression type match the type contract named by the type checker
TRAIT_E_EXPECTED_BLOCK.example = let count: int = 1

TRAIT_E_EXPECTED_DELIMITER.summary = delimitador esperado.
TRAIT_E_EXPECTED_DELIMITER.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_EXPECTED_DELIMITER.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_EXPECTED_DELIMITER.fix = make the expression type match the type contract named by the type checker
TRAIT_E_EXPECTED_DELIMITER.example = let count: int = 1

TRAIT_E_UNEXPECTED_TOKEN.summary = token inesperado.
TRAIT_E_UNEXPECTED_TOKEN.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_UNEXPECTED_TOKEN.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_UNEXPECTED_TOKEN.fix = make the expression type match the type contract named by the type checker
TRAIT_E_UNEXPECTED_TOKEN.example = let count: int = 1

TRAIT_E_UNBALANCED_DELIMITER.summary = unbalanced delimitador.
TRAIT_E_UNBALANCED_DELIMITER.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_UNBALANCED_DELIMITER.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_UNBALANCED_DELIMITER.fix = make the expression type match the type contract named by the type checker
TRAIT_E_UNBALANCED_DELIMITER.example = let count: int = 1

TRAIT_E_INVALID_ATTRIBUTE.summary = attribute invalido.
TRAIT_E_INVALID_ATTRIBUTE.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_ATTRIBUTE.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_ATTRIBUTE.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_ATTRIBUTE.example = let count: int = 1

TRAIT_E_INVALID_DECLARATION.summary = declaracao invalido.
TRAIT_E_INVALID_DECLARATION.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_DECLARATION.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_DECLARATION.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_DECLARATION.example = let count: int = 1

TRAIT_E_INVALID_STATEMENT.summary = instrucao invalido.
TRAIT_E_INVALID_STATEMENT.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_STATEMENT.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_STATEMENT.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_STATEMENT.example = let count: int = 1

TRAIT_E_INVALID_EXPRESSION.summary = expressao invalido.
TRAIT_E_INVALID_EXPRESSION.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_EXPRESSION.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_EXPRESSION.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_EXPRESSION.example = let count: int = 1

TRAIT_E_INVALID_PATTERN.summary = padrao invalido.
TRAIT_E_INVALID_PATTERN.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_PATTERN.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_PATTERN.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_PATTERN.example = let count: int = 1

TRAIT_E_INVALID_LITERAL.summary = literal invalido.
TRAIT_E_INVALID_LITERAL.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_LITERAL.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_LITERAL.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_LITERAL.example = let count: int = 1

TRAIT_E_INVALID_OPERATOR.summary = operador invalido.
TRAIT_E_INVALID_OPERATOR.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_OPERATOR.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_OPERATOR.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_OPERATOR.example = let count: int = 1

TRAIT_E_INVALID_MODIFIER.summary = modifier invalido.
TRAIT_E_INVALID_MODIFIER.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_MODIFIER.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_MODIFIER.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_MODIFIER.example = let count: int = 1

TRAIT_E_MISSING_BODY.summary = body ausente.
TRAIT_E_MISSING_BODY.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_MISSING_BODY.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_MISSING_BODY.fix = make the expression type match the type contract named by the type checker
TRAIT_E_MISSING_BODY.example = let count: int = 1

TRAIT_E_MISSING_RETURN.summary = retorno ausente.
TRAIT_E_MISSING_RETURN.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_MISSING_RETURN.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_MISSING_RETURN.fix = make the expression type match the type contract named by the type checker
TRAIT_E_MISSING_RETURN.example = let count: int = 1

TRAIT_E_DUPLICATE_NAME.summary = nome duplicado.
TRAIT_E_DUPLICATE_NAME.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_DUPLICATE_NAME.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_DUPLICATE_NAME.fix = make the expression type match the type contract named by the type checker
TRAIT_E_DUPLICATE_NAME.example = let count: int = 1

TRAIT_E_UNKNOWN_NAME.summary = nome desconhecido.
TRAIT_E_UNKNOWN_NAME.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_UNKNOWN_NAME.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_UNKNOWN_NAME.fix = make the expression type match the type contract named by the type checker
TRAIT_E_UNKNOWN_NAME.example = let count: int = 1

TRAIT_E_UNKNOWN_TYPE.summary = tipo desconhecido.
TRAIT_E_UNKNOWN_TYPE.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_UNKNOWN_TYPE.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_UNKNOWN_TYPE.fix = make the expression type match the type contract named by the type checker
TRAIT_E_UNKNOWN_TYPE.example = let count: int = 1

TRAIT_E_UNKNOWN_MODULE.summary = modulo desconhecido.
TRAIT_E_UNKNOWN_MODULE.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_UNKNOWN_MODULE.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_UNKNOWN_MODULE.fix = make the expression type match the type contract named by the type checker
TRAIT_E_UNKNOWN_MODULE.example = let count: int = 1

TRAIT_E_UNKNOWN_MEMBER.summary = membro desconhecido.
TRAIT_E_UNKNOWN_MEMBER.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_UNKNOWN_MEMBER.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_UNKNOWN_MEMBER.fix = make the expression type match the type contract named by the type checker
TRAIT_E_UNKNOWN_MEMBER.example = let count: int = 1

TRAIT_E_AMBIGUOUS_NAME.summary = ambiguous nome.
TRAIT_E_AMBIGUOUS_NAME.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_AMBIGUOUS_NAME.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_AMBIGUOUS_NAME.fix = make the expression type match the type contract named by the type checker
TRAIT_E_AMBIGUOUS_NAME.example = let count: int = 1

TRAIT_E_PRIVATE_SYMBOL.summary = private simbolo.
TRAIT_E_PRIVATE_SYMBOL.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_PRIVATE_SYMBOL.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_PRIVATE_SYMBOL.fix = make the expression type match the type contract named by the type checker
TRAIT_E_PRIVATE_SYMBOL.example = let count: int = 1

TRAIT_E_IMPORT_NOT_FOUND.summary = importacao ausente.
TRAIT_E_IMPORT_NOT_FOUND.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_IMPORT_NOT_FOUND.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_IMPORT_NOT_FOUND.fix = make the expression type match the type contract named by the type checker
TRAIT_E_IMPORT_NOT_FOUND.example = let count: int = 1

TRAIT_E_IMPORT_CYCLE.summary = importacao ciclo.
TRAIT_E_IMPORT_CYCLE.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_IMPORT_CYCLE.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_IMPORT_CYCLE.fix = make the expression type match the type contract named by the type checker
TRAIT_E_IMPORT_CYCLE.example = let count: int = 1

TRAIT_E_EXPORT_CONFLICT.summary = export conflito.
TRAIT_E_EXPORT_CONFLICT.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_EXPORT_CONFLICT.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_EXPORT_CONFLICT.fix = make the expression type match the type contract named by the type checker
TRAIT_E_EXPORT_CONFLICT.example = let count: int = 1

TRAIT_E_ARITY_MISMATCH.summary = arity incompatibilidade.
TRAIT_E_ARITY_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_ARITY_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_ARITY_MISMATCH.fix = make the expression type match the type contract named by the type checker
TRAIT_E_ARITY_MISMATCH.example = let count: int = 1

TRAIT_E_ARGUMENT_MISMATCH.summary = argumento incompatibilidade.
TRAIT_E_ARGUMENT_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_ARGUMENT_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_ARGUMENT_MISMATCH.fix = make the expression type match the type contract named by the type checker
TRAIT_E_ARGUMENT_MISMATCH.example = let count: int = 1

TRAIT_E_ASSIGNMENT_MISMATCH.summary = atribuicao incompatibilidade.
TRAIT_E_ASSIGNMENT_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_ASSIGNMENT_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_ASSIGNMENT_MISMATCH.fix = make the expression type match the type contract named by the type checker
TRAIT_E_ASSIGNMENT_MISMATCH.example = let count: int = 1

TRAIT_E_BRANCH_MISMATCH.summary = branch incompatibilidade.
TRAIT_E_BRANCH_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_BRANCH_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_BRANCH_MISMATCH.fix = make the expression type match the type contract named by the type checker
TRAIT_E_BRANCH_MISMATCH.example = let count: int = 1

TRAIT_E_INVALID_CALL.summary = chamada invalido.
TRAIT_E_INVALID_CALL.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_CALL.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_CALL.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_CALL.example = let count: int = 1

TRAIT_E_INVALID_CAST.summary = conversao invalido.
TRAIT_E_INVALID_CAST.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_CAST.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_CAST.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_CAST.example = let count: int = 1

TRAIT_E_INVALID_INDEX.summary = indice invalido.
TRAIT_E_INVALID_INDEX.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_INDEX.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_INDEX.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_INDEX.example = let count: int = 1

TRAIT_E_INVALID_DEREF.summary = deref invalido.
TRAIT_E_INVALID_DEREF.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_DEREF.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_DEREF.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_DEREF.example = let count: int = 1

TRAIT_E_INVALID_BORROW.summary = emprestimo invalido.
TRAIT_E_INVALID_BORROW.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_BORROW.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_BORROW.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_BORROW.example = let count: int = 1

TRAIT_E_INVALID_MOVE.summary = movimento invalido.
TRAIT_E_INVALID_MOVE.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_INVALID_MOVE.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_INVALID_MOVE.fix = make the expression type match the type contract named by the type checker
TRAIT_E_INVALID_MOVE.example = let count: int = 1

TRAIT_E_USE_AFTER_MOVE.summary = use after movimento.
TRAIT_E_USE_AFTER_MOVE.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_USE_AFTER_MOVE.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_USE_AFTER_MOVE.fix = make the expression type match the type contract named by the type checker
TRAIT_E_USE_AFTER_MOVE.example = let count: int = 1

TRAIT_E_USE_AFTER_DROP.summary = use after descarte.
TRAIT_E_USE_AFTER_DROP.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_USE_AFTER_DROP.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_USE_AFTER_DROP.fix = make the expression type match the type contract named by the type checker
TRAIT_E_USE_AFTER_DROP.example = let count: int = 1

TRAIT_E_USE_BEFORE_INIT.summary = use before init.
TRAIT_E_USE_BEFORE_INIT.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_USE_BEFORE_INIT.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_USE_BEFORE_INIT.fix = make the expression type match the type contract named by the type checker
TRAIT_E_USE_BEFORE_INIT.example = let count: int = 1

TRAIT_E_DOUBLE_DROP.summary = double descarte.
TRAIT_E_DOUBLE_DROP.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_DOUBLE_DROP.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_DOUBLE_DROP.fix = make the expression type match the type contract named by the type checker
TRAIT_E_DOUBLE_DROP.example = let count: int = 1

TRAIT_E_BORROW_CONFLICT.summary = emprestimo conflito.
TRAIT_E_BORROW_CONFLICT.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_BORROW_CONFLICT.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_BORROW_CONFLICT.fix = make the expression type match the type contract named by the type checker
TRAIT_E_BORROW_CONFLICT.example = let count: int = 1

TRAIT_E_MUTABILITY_CONFLICT.summary = mutability conflito.
TRAIT_E_MUTABILITY_CONFLICT.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_MUTABILITY_CONFLICT.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_MUTABILITY_CONFLICT.fix = make the expression type match the type contract named by the type checker
TRAIT_E_MUTABILITY_CONFLICT.example = let count: int = 1

TRAIT_E_LIFETIME_TOO_SHORT.summary = tempo de vida demais short.
TRAIT_E_LIFETIME_TOO_SHORT.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_LIFETIME_TOO_SHORT.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_LIFETIME_TOO_SHORT.fix = make the expression type match the type contract named by the type checker
TRAIT_E_LIFETIME_TOO_SHORT.example = let count: int = 1

TRAIT_E_DANGLING_REFERENCE.summary = dangling referencia.
TRAIT_E_DANGLING_REFERENCE.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_DANGLING_REFERENCE.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_DANGLING_REFERENCE.fix = make the expression type match the type contract named by the type checker
TRAIT_E_DANGLING_REFERENCE.example = let count: int = 1

TRAIT_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
TRAIT_E_NON_EXHAUSTIVE_MATCH.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_NON_EXHAUSTIVE_MATCH.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_NON_EXHAUSTIVE_MATCH.fix = make the expression type match the type contract named by the type checker
TRAIT_E_NON_EXHAUSTIVE_MATCH.example = let count: int = 1

TRAIT_E_UNREACHABLE_PATTERN.summary = inalcancavel padrao.
TRAIT_E_UNREACHABLE_PATTERN.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_UNREACHABLE_PATTERN.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_UNREACHABLE_PATTERN.fix = make the expression type match the type contract named by the type checker
TRAIT_E_UNREACHABLE_PATTERN.example = let count: int = 1

TRAIT_E_CONST_REQUIRED.summary = const required.
TRAIT_E_CONST_REQUIRED.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_CONST_REQUIRED.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_CONST_REQUIRED.fix = make the expression type match the type contract named by the type checker
TRAIT_E_CONST_REQUIRED.example = let count: int = 1

TRAIT_E_CONST_OVERFLOW.summary = const estouro.
TRAIT_E_CONST_OVERFLOW.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_CONST_OVERFLOW.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_CONST_OVERFLOW.fix = make the expression type match the type contract named by the type checker
TRAIT_E_CONST_OVERFLOW.example = let count: int = 1

TRAIT_E_CONST_DIVISION_BY_ZERO.summary = const divisao by zero.
TRAIT_E_CONST_DIVISION_BY_ZERO.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_CONST_DIVISION_BY_ZERO.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_CONST_DIVISION_BY_ZERO.fix = make the expression type match the type contract named by the type checker
TRAIT_E_CONST_DIVISION_BY_ZERO.example = let count: int = 1

TRAIT_E_CONST_CYCLE.summary = const ciclo.
TRAIT_E_CONST_CYCLE.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_CONST_CYCLE.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_CONST_CYCLE.fix = make the expression type match the type contract named by the type checker
TRAIT_E_CONST_CYCLE.example = let count: int = 1

TRAIT_E_MACRO_NOT_FOUND.summary = macro ausente.
TRAIT_E_MACRO_NOT_FOUND.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_MACRO_NOT_FOUND.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_MACRO_NOT_FOUND.fix = make the expression type match the type contract named by the type checker
TRAIT_E_MACRO_NOT_FOUND.example = let count: int = 1

TRAIT_E_MACRO_RECURSION.summary = macro recursion.
TRAIT_E_MACRO_RECURSION.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_MACRO_RECURSION.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_MACRO_RECURSION.fix = make the expression type match the type contract named by the type checker
TRAIT_E_MACRO_RECURSION.example = let count: int = 1

TRAIT_E_MACRO_EXPANSION_FAILED.summary = macro expansion falhou.
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

TRAIT_E_GENERIC_ARGUMENT_MISSING.summary = generico argumento ausente.
TRAIT_E_GENERIC_ARGUMENT_MISSING.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_GENERIC_ARGUMENT_MISSING.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_GENERIC_ARGUMENT_MISSING.fix = make the expression type match the type contract named by the type checker
TRAIT_E_GENERIC_ARGUMENT_MISSING.example = let count: int = 1

TRAIT_E_GENERIC_BOUND_FAILED.summary = generico bound falhou.
TRAIT_E_GENERIC_BOUND_FAILED.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_GENERIC_BOUND_FAILED.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_GENERIC_BOUND_FAILED.fix = make the expression type match the type contract named by the type checker
TRAIT_E_GENERIC_BOUND_FAILED.example = let count: int = 1

TRAIT_E_UNSUPPORTED_TARGET.summary = destino nao suportado.
TRAIT_E_UNSUPPORTED_TARGET.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_UNSUPPORTED_TARGET.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_UNSUPPORTED_TARGET.fix = make the expression type match the type contract named by the type checker
TRAIT_E_UNSUPPORTED_TARGET.example = let count: int = 1

TRAIT_E_ABI_MISMATCH.summary = abi incompatibilidade.
TRAIT_E_ABI_MISMATCH.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_ABI_MISMATCH.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_ABI_MISMATCH.fix = make the expression type match the type contract named by the type checker
TRAIT_E_ABI_MISMATCH.example = let count: int = 1

TRAIT_E_LINK_FAILED.summary = link falhou.
TRAIT_E_LINK_FAILED.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_LINK_FAILED.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_LINK_FAILED.fix = make the expression type match the type contract named by the type checker
TRAIT_E_LINK_FAILED.example = let count: int = 1

TRAIT_E_RUNTIME_PANIC.summary = runtime panico.
TRAIT_E_RUNTIME_PANIC.cause = The inferred type does not satisfy the type required at this location.
TRAIT_E_RUNTIME_PANIC.step1 = Compare the expected and found types in the diagnostic labels.
TRAIT_E_RUNTIME_PANIC.fix = make the expression type match the type contract named by the type checker
TRAIT_E_RUNTIME_PANIC.example = let count: int = 1

OWNERSHIP_E_EXPECTED_IDENTIFIER.summary = identificador esperado.
OWNERSHIP_E_EXPECTED_IDENTIFIER.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_EXPECTED_IDENTIFIER.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_EXPECTED_IDENTIFIER.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_EXPECTED_IDENTIFIER.example = let view = &value

OWNERSHIP_E_EXPECTED_EXPRESSION.summary = expressao esperado.
OWNERSHIP_E_EXPECTED_EXPRESSION.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_EXPECTED_EXPRESSION.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_EXPECTED_EXPRESSION.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_EXPECTED_EXPRESSION.example = let view = &value

OWNERSHIP_E_EXPECTED_TYPE.summary = tipo esperado.
OWNERSHIP_E_EXPECTED_TYPE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_EXPECTED_TYPE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_EXPECTED_TYPE.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_EXPECTED_TYPE.example = let view = &value

OWNERSHIP_E_EXPECTED_PATTERN.summary = padrao esperado.
OWNERSHIP_E_EXPECTED_PATTERN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_EXPECTED_PATTERN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_EXPECTED_PATTERN.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_EXPECTED_PATTERN.example = let view = &value

OWNERSHIP_E_EXPECTED_BLOCK.summary = bloco esperado.
OWNERSHIP_E_EXPECTED_BLOCK.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_EXPECTED_BLOCK.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_EXPECTED_BLOCK.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_EXPECTED_BLOCK.example = let view = &value

OWNERSHIP_E_EXPECTED_DELIMITER.summary = delimitador esperado.
OWNERSHIP_E_EXPECTED_DELIMITER.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_EXPECTED_DELIMITER.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_EXPECTED_DELIMITER.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_EXPECTED_DELIMITER.example = let view = &value

OWNERSHIP_E_UNEXPECTED_TOKEN.summary = token inesperado.
OWNERSHIP_E_UNEXPECTED_TOKEN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_UNEXPECTED_TOKEN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_UNEXPECTED_TOKEN.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_UNEXPECTED_TOKEN.example = let view = &value

OWNERSHIP_E_UNBALANCED_DELIMITER.summary = unbalanced delimitador.
OWNERSHIP_E_UNBALANCED_DELIMITER.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_UNBALANCED_DELIMITER.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_UNBALANCED_DELIMITER.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_UNBALANCED_DELIMITER.example = let view = &value

OWNERSHIP_E_INVALID_ATTRIBUTE.summary = attribute invalido.
OWNERSHIP_E_INVALID_ATTRIBUTE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_ATTRIBUTE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_ATTRIBUTE.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_ATTRIBUTE.example = let view = &value

OWNERSHIP_E_INVALID_DECLARATION.summary = declaracao invalido.
OWNERSHIP_E_INVALID_DECLARATION.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_DECLARATION.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_DECLARATION.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_DECLARATION.example = let view = &value

OWNERSHIP_E_INVALID_STATEMENT.summary = instrucao invalido.
OWNERSHIP_E_INVALID_STATEMENT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_STATEMENT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_STATEMENT.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_STATEMENT.example = let view = &value

OWNERSHIP_E_INVALID_EXPRESSION.summary = expressao invalido.
OWNERSHIP_E_INVALID_EXPRESSION.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_EXPRESSION.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_EXPRESSION.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_EXPRESSION.example = let view = &value

OWNERSHIP_E_INVALID_PATTERN.summary = padrao invalido.
OWNERSHIP_E_INVALID_PATTERN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_PATTERN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_PATTERN.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_PATTERN.example = let view = &value

OWNERSHIP_E_INVALID_LITERAL.summary = literal invalido.
OWNERSHIP_E_INVALID_LITERAL.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_LITERAL.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_LITERAL.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_LITERAL.example = let view = &value

OWNERSHIP_E_INVALID_OPERATOR.summary = operador invalido.
OWNERSHIP_E_INVALID_OPERATOR.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_OPERATOR.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_OPERATOR.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_OPERATOR.example = let view = &value

OWNERSHIP_E_INVALID_MODIFIER.summary = modifier invalido.
OWNERSHIP_E_INVALID_MODIFIER.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_MODIFIER.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_MODIFIER.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_MODIFIER.example = let view = &value

OWNERSHIP_E_MISSING_BODY.summary = body ausente.
OWNERSHIP_E_MISSING_BODY.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_MISSING_BODY.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_MISSING_BODY.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_MISSING_BODY.example = let view = &value

OWNERSHIP_E_MISSING_RETURN.summary = retorno ausente.
OWNERSHIP_E_MISSING_RETURN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_MISSING_RETURN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_MISSING_RETURN.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_MISSING_RETURN.example = let view = &value

OWNERSHIP_E_DUPLICATE_NAME.summary = nome duplicado.
OWNERSHIP_E_DUPLICATE_NAME.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_DUPLICATE_NAME.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_DUPLICATE_NAME.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_DUPLICATE_NAME.example = let view = &value

OWNERSHIP_E_UNKNOWN_NAME.summary = nome desconhecido.
OWNERSHIP_E_UNKNOWN_NAME.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_UNKNOWN_NAME.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_UNKNOWN_NAME.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_UNKNOWN_NAME.example = let view = &value

OWNERSHIP_E_UNKNOWN_TYPE.summary = tipo desconhecido.
OWNERSHIP_E_UNKNOWN_TYPE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_UNKNOWN_TYPE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_UNKNOWN_TYPE.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_UNKNOWN_TYPE.example = let view = &value

OWNERSHIP_E_UNKNOWN_MODULE.summary = modulo desconhecido.
OWNERSHIP_E_UNKNOWN_MODULE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_UNKNOWN_MODULE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_UNKNOWN_MODULE.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_UNKNOWN_MODULE.example = let view = &value

OWNERSHIP_E_UNKNOWN_MEMBER.summary = membro desconhecido.
OWNERSHIP_E_UNKNOWN_MEMBER.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_UNKNOWN_MEMBER.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_UNKNOWN_MEMBER.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_UNKNOWN_MEMBER.example = let view = &value

OWNERSHIP_E_AMBIGUOUS_NAME.summary = ambiguous nome.
OWNERSHIP_E_AMBIGUOUS_NAME.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_AMBIGUOUS_NAME.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_AMBIGUOUS_NAME.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_AMBIGUOUS_NAME.example = let view = &value

OWNERSHIP_E_PRIVATE_SYMBOL.summary = private simbolo.
OWNERSHIP_E_PRIVATE_SYMBOL.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_PRIVATE_SYMBOL.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_PRIVATE_SYMBOL.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_PRIVATE_SYMBOL.example = let view = &value

OWNERSHIP_E_IMPORT_NOT_FOUND.summary = importacao ausente.
OWNERSHIP_E_IMPORT_NOT_FOUND.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_IMPORT_NOT_FOUND.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_IMPORT_NOT_FOUND.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_IMPORT_NOT_FOUND.example = let view = &value

OWNERSHIP_E_IMPORT_CYCLE.summary = importacao ciclo.
OWNERSHIP_E_IMPORT_CYCLE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_IMPORT_CYCLE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_IMPORT_CYCLE.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_IMPORT_CYCLE.example = let view = &value

OWNERSHIP_E_EXPORT_CONFLICT.summary = export conflito.
OWNERSHIP_E_EXPORT_CONFLICT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_EXPORT_CONFLICT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_EXPORT_CONFLICT.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_EXPORT_CONFLICT.example = let view = &value

OWNERSHIP_E_ARITY_MISMATCH.summary = arity incompatibilidade.
OWNERSHIP_E_ARITY_MISMATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_ARITY_MISMATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_ARITY_MISMATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_ARITY_MISMATCH.example = let view = &value

OWNERSHIP_E_ARGUMENT_MISMATCH.summary = argumento incompatibilidade.
OWNERSHIP_E_ARGUMENT_MISMATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_ARGUMENT_MISMATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_ARGUMENT_MISMATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_ARGUMENT_MISMATCH.example = let view = &value

OWNERSHIP_E_ASSIGNMENT_MISMATCH.summary = atribuicao incompatibilidade.
OWNERSHIP_E_ASSIGNMENT_MISMATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_ASSIGNMENT_MISMATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_ASSIGNMENT_MISMATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_ASSIGNMENT_MISMATCH.example = let view = &value

OWNERSHIP_E_BRANCH_MISMATCH.summary = branch incompatibilidade.
OWNERSHIP_E_BRANCH_MISMATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_BRANCH_MISMATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_BRANCH_MISMATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_BRANCH_MISMATCH.example = let view = &value

OWNERSHIP_E_INVALID_CALL.summary = chamada invalido.
OWNERSHIP_E_INVALID_CALL.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_CALL.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_CALL.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_CALL.example = let view = &value

OWNERSHIP_E_INVALID_CAST.summary = conversao invalido.
OWNERSHIP_E_INVALID_CAST.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_CAST.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_CAST.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_CAST.example = let view = &value

OWNERSHIP_E_INVALID_INDEX.summary = indice invalido.
OWNERSHIP_E_INVALID_INDEX.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_INDEX.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_INDEX.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_INDEX.example = let view = &value

OWNERSHIP_E_INVALID_DEREF.summary = deref invalido.
OWNERSHIP_E_INVALID_DEREF.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_DEREF.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_DEREF.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_DEREF.example = let view = &value

OWNERSHIP_E_INVALID_BORROW.summary = emprestimo invalido.
OWNERSHIP_E_INVALID_BORROW.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_BORROW.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_BORROW.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_BORROW.example = let view = &value

OWNERSHIP_E_INVALID_MOVE.summary = movimento invalido.
OWNERSHIP_E_INVALID_MOVE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_INVALID_MOVE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_INVALID_MOVE.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_INVALID_MOVE.example = let view = &value

OWNERSHIP_E_USE_AFTER_MOVE.summary = use after movimento.
OWNERSHIP_E_USE_AFTER_MOVE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_USE_AFTER_MOVE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_USE_AFTER_MOVE.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_USE_AFTER_MOVE.example = let view = &value

OWNERSHIP_E_USE_AFTER_DROP.summary = use after descarte.
OWNERSHIP_E_USE_AFTER_DROP.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_USE_AFTER_DROP.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_USE_AFTER_DROP.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_USE_AFTER_DROP.example = let view = &value

OWNERSHIP_E_USE_BEFORE_INIT.summary = use before init.
OWNERSHIP_E_USE_BEFORE_INIT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_USE_BEFORE_INIT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_USE_BEFORE_INIT.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_USE_BEFORE_INIT.example = let view = &value

OWNERSHIP_E_DOUBLE_DROP.summary = double descarte.
OWNERSHIP_E_DOUBLE_DROP.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_DOUBLE_DROP.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_DOUBLE_DROP.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_DOUBLE_DROP.example = let view = &value

OWNERSHIP_E_BORROW_CONFLICT.summary = emprestimo conflito.
OWNERSHIP_E_BORROW_CONFLICT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_BORROW_CONFLICT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_BORROW_CONFLICT.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_BORROW_CONFLICT.example = let view = &value

OWNERSHIP_E_MUTABILITY_CONFLICT.summary = mutability conflito.
OWNERSHIP_E_MUTABILITY_CONFLICT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_MUTABILITY_CONFLICT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_MUTABILITY_CONFLICT.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_MUTABILITY_CONFLICT.example = let view = &value

OWNERSHIP_E_LIFETIME_TOO_SHORT.summary = tempo de vida demais short.
OWNERSHIP_E_LIFETIME_TOO_SHORT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_LIFETIME_TOO_SHORT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_LIFETIME_TOO_SHORT.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_LIFETIME_TOO_SHORT.example = let view = &value

OWNERSHIP_E_DANGLING_REFERENCE.summary = dangling referencia.
OWNERSHIP_E_DANGLING_REFERENCE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_DANGLING_REFERENCE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_DANGLING_REFERENCE.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_DANGLING_REFERENCE.example = let view = &value

OWNERSHIP_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
OWNERSHIP_E_NON_EXHAUSTIVE_MATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_NON_EXHAUSTIVE_MATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_NON_EXHAUSTIVE_MATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_NON_EXHAUSTIVE_MATCH.example = let view = &value

OWNERSHIP_E_UNREACHABLE_PATTERN.summary = inalcancavel padrao.
OWNERSHIP_E_UNREACHABLE_PATTERN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_UNREACHABLE_PATTERN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_UNREACHABLE_PATTERN.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_UNREACHABLE_PATTERN.example = let view = &value

OWNERSHIP_E_CONST_REQUIRED.summary = const required.
OWNERSHIP_E_CONST_REQUIRED.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_CONST_REQUIRED.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_CONST_REQUIRED.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_CONST_REQUIRED.example = let view = &value

OWNERSHIP_E_CONST_OVERFLOW.summary = const estouro.
OWNERSHIP_E_CONST_OVERFLOW.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_CONST_OVERFLOW.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_CONST_OVERFLOW.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_CONST_OVERFLOW.example = let view = &value

OWNERSHIP_E_CONST_DIVISION_BY_ZERO.summary = const divisao by zero.
OWNERSHIP_E_CONST_DIVISION_BY_ZERO.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_CONST_DIVISION_BY_ZERO.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_CONST_DIVISION_BY_ZERO.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_CONST_DIVISION_BY_ZERO.example = let view = &value

OWNERSHIP_E_CONST_CYCLE.summary = const ciclo.
OWNERSHIP_E_CONST_CYCLE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_CONST_CYCLE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_CONST_CYCLE.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_CONST_CYCLE.example = let view = &value

OWNERSHIP_E_MACRO_NOT_FOUND.summary = macro ausente.
OWNERSHIP_E_MACRO_NOT_FOUND.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_MACRO_NOT_FOUND.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_MACRO_NOT_FOUND.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_MACRO_NOT_FOUND.example = let view = &value

OWNERSHIP_E_MACRO_RECURSION.summary = macro recursion.
OWNERSHIP_E_MACRO_RECURSION.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_MACRO_RECURSION.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_MACRO_RECURSION.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_MACRO_RECURSION.example = let view = &value

OWNERSHIP_E_MACRO_EXPANSION_FAILED.summary = macro expansion falhou.
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

OWNERSHIP_E_GENERIC_ARGUMENT_MISSING.summary = generico argumento ausente.
OWNERSHIP_E_GENERIC_ARGUMENT_MISSING.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_GENERIC_ARGUMENT_MISSING.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_GENERIC_ARGUMENT_MISSING.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_GENERIC_ARGUMENT_MISSING.example = let view = &value

OWNERSHIP_E_GENERIC_BOUND_FAILED.summary = generico bound falhou.
OWNERSHIP_E_GENERIC_BOUND_FAILED.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_GENERIC_BOUND_FAILED.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_GENERIC_BOUND_FAILED.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_GENERIC_BOUND_FAILED.example = let view = &value

OWNERSHIP_E_UNSUPPORTED_TARGET.summary = destino nao suportado.
OWNERSHIP_E_UNSUPPORTED_TARGET.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_UNSUPPORTED_TARGET.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_UNSUPPORTED_TARGET.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_UNSUPPORTED_TARGET.example = let view = &value

OWNERSHIP_E_ABI_MISMATCH.summary = abi incompatibilidade.
OWNERSHIP_E_ABI_MISMATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_ABI_MISMATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_ABI_MISMATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_ABI_MISMATCH.example = let view = &value

OWNERSHIP_E_LINK_FAILED.summary = link falhou.
OWNERSHIP_E_LINK_FAILED.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_LINK_FAILED.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_LINK_FAILED.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_LINK_FAILED.example = let view = &value

OWNERSHIP_E_RUNTIME_PANIC.summary = runtime panico.
OWNERSHIP_E_RUNTIME_PANIC.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
OWNERSHIP_E_RUNTIME_PANIC.step1 = Find the earlier move or borrow mentioned by the diagnostic.
OWNERSHIP_E_RUNTIME_PANIC.fix = repair the ownership transition named by the borrow checker before the highlighted use
OWNERSHIP_E_RUNTIME_PANIC.example = let view = &value

LIFETIME_E_EXPECTED_IDENTIFIER.summary = identificador esperado.
LIFETIME_E_EXPECTED_IDENTIFIER.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_EXPECTED_IDENTIFIER.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_EXPECTED_IDENTIFIER.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_EXPECTED_IDENTIFIER.example = let view = &value

LIFETIME_E_EXPECTED_EXPRESSION.summary = expressao esperado.
LIFETIME_E_EXPECTED_EXPRESSION.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_EXPECTED_EXPRESSION.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_EXPECTED_EXPRESSION.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_EXPECTED_EXPRESSION.example = let view = &value

LIFETIME_E_EXPECTED_TYPE.summary = tipo esperado.
LIFETIME_E_EXPECTED_TYPE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_EXPECTED_TYPE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_EXPECTED_TYPE.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_EXPECTED_TYPE.example = let view = &value

LIFETIME_E_EXPECTED_PATTERN.summary = padrao esperado.
LIFETIME_E_EXPECTED_PATTERN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_EXPECTED_PATTERN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_EXPECTED_PATTERN.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_EXPECTED_PATTERN.example = let view = &value

LIFETIME_E_EXPECTED_BLOCK.summary = bloco esperado.
LIFETIME_E_EXPECTED_BLOCK.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_EXPECTED_BLOCK.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_EXPECTED_BLOCK.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_EXPECTED_BLOCK.example = let view = &value

LIFETIME_E_EXPECTED_DELIMITER.summary = delimitador esperado.
LIFETIME_E_EXPECTED_DELIMITER.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_EXPECTED_DELIMITER.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_EXPECTED_DELIMITER.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_EXPECTED_DELIMITER.example = let view = &value

LIFETIME_E_UNEXPECTED_TOKEN.summary = token inesperado.
LIFETIME_E_UNEXPECTED_TOKEN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_UNEXPECTED_TOKEN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_UNEXPECTED_TOKEN.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_UNEXPECTED_TOKEN.example = let view = &value

LIFETIME_E_UNBALANCED_DELIMITER.summary = unbalanced delimitador.
LIFETIME_E_UNBALANCED_DELIMITER.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_UNBALANCED_DELIMITER.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_UNBALANCED_DELIMITER.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_UNBALANCED_DELIMITER.example = let view = &value

LIFETIME_E_INVALID_ATTRIBUTE.summary = attribute invalido.
LIFETIME_E_INVALID_ATTRIBUTE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_ATTRIBUTE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_ATTRIBUTE.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_ATTRIBUTE.example = let view = &value

LIFETIME_E_INVALID_DECLARATION.summary = declaracao invalido.
LIFETIME_E_INVALID_DECLARATION.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_DECLARATION.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_DECLARATION.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_DECLARATION.example = let view = &value

LIFETIME_E_INVALID_STATEMENT.summary = instrucao invalido.
LIFETIME_E_INVALID_STATEMENT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_STATEMENT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_STATEMENT.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_STATEMENT.example = let view = &value

LIFETIME_E_INVALID_EXPRESSION.summary = expressao invalido.
LIFETIME_E_INVALID_EXPRESSION.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_EXPRESSION.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_EXPRESSION.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_EXPRESSION.example = let view = &value

LIFETIME_E_INVALID_PATTERN.summary = padrao invalido.
LIFETIME_E_INVALID_PATTERN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_PATTERN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_PATTERN.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_PATTERN.example = let view = &value

LIFETIME_E_INVALID_LITERAL.summary = literal invalido.
LIFETIME_E_INVALID_LITERAL.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_LITERAL.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_LITERAL.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_LITERAL.example = let view = &value

LIFETIME_E_INVALID_OPERATOR.summary = operador invalido.
LIFETIME_E_INVALID_OPERATOR.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_OPERATOR.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_OPERATOR.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_OPERATOR.example = let view = &value

LIFETIME_E_INVALID_MODIFIER.summary = modifier invalido.
LIFETIME_E_INVALID_MODIFIER.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_MODIFIER.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_MODIFIER.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_MODIFIER.example = let view = &value

LIFETIME_E_MISSING_BODY.summary = body ausente.
LIFETIME_E_MISSING_BODY.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_MISSING_BODY.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_MISSING_BODY.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_MISSING_BODY.example = let view = &value

LIFETIME_E_MISSING_RETURN.summary = retorno ausente.
LIFETIME_E_MISSING_RETURN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_MISSING_RETURN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_MISSING_RETURN.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_MISSING_RETURN.example = let view = &value

LIFETIME_E_DUPLICATE_NAME.summary = nome duplicado.
LIFETIME_E_DUPLICATE_NAME.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_DUPLICATE_NAME.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_DUPLICATE_NAME.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_DUPLICATE_NAME.example = let view = &value

LIFETIME_E_UNKNOWN_NAME.summary = nome desconhecido.
LIFETIME_E_UNKNOWN_NAME.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_UNKNOWN_NAME.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_UNKNOWN_NAME.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_UNKNOWN_NAME.example = let view = &value

LIFETIME_E_UNKNOWN_TYPE.summary = tipo desconhecido.
LIFETIME_E_UNKNOWN_TYPE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_UNKNOWN_TYPE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_UNKNOWN_TYPE.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_UNKNOWN_TYPE.example = let view = &value

LIFETIME_E_UNKNOWN_MODULE.summary = modulo desconhecido.
LIFETIME_E_UNKNOWN_MODULE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_UNKNOWN_MODULE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_UNKNOWN_MODULE.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_UNKNOWN_MODULE.example = let view = &value

LIFETIME_E_UNKNOWN_MEMBER.summary = membro desconhecido.
LIFETIME_E_UNKNOWN_MEMBER.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_UNKNOWN_MEMBER.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_UNKNOWN_MEMBER.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_UNKNOWN_MEMBER.example = let view = &value

LIFETIME_E_AMBIGUOUS_NAME.summary = ambiguous nome.
LIFETIME_E_AMBIGUOUS_NAME.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_AMBIGUOUS_NAME.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_AMBIGUOUS_NAME.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_AMBIGUOUS_NAME.example = let view = &value

LIFETIME_E_PRIVATE_SYMBOL.summary = private simbolo.
LIFETIME_E_PRIVATE_SYMBOL.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_PRIVATE_SYMBOL.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_PRIVATE_SYMBOL.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_PRIVATE_SYMBOL.example = let view = &value

LIFETIME_E_IMPORT_NOT_FOUND.summary = importacao ausente.
LIFETIME_E_IMPORT_NOT_FOUND.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_IMPORT_NOT_FOUND.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_IMPORT_NOT_FOUND.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_IMPORT_NOT_FOUND.example = let view = &value

LIFETIME_E_IMPORT_CYCLE.summary = importacao ciclo.
LIFETIME_E_IMPORT_CYCLE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_IMPORT_CYCLE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_IMPORT_CYCLE.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_IMPORT_CYCLE.example = let view = &value

LIFETIME_E_EXPORT_CONFLICT.summary = export conflito.
LIFETIME_E_EXPORT_CONFLICT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_EXPORT_CONFLICT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_EXPORT_CONFLICT.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_EXPORT_CONFLICT.example = let view = &value

LIFETIME_E_ARITY_MISMATCH.summary = arity incompatibilidade.
LIFETIME_E_ARITY_MISMATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_ARITY_MISMATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_ARITY_MISMATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_ARITY_MISMATCH.example = let view = &value

LIFETIME_E_ARGUMENT_MISMATCH.summary = argumento incompatibilidade.
LIFETIME_E_ARGUMENT_MISMATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_ARGUMENT_MISMATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_ARGUMENT_MISMATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_ARGUMENT_MISMATCH.example = let view = &value

LIFETIME_E_ASSIGNMENT_MISMATCH.summary = atribuicao incompatibilidade.
LIFETIME_E_ASSIGNMENT_MISMATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_ASSIGNMENT_MISMATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_ASSIGNMENT_MISMATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_ASSIGNMENT_MISMATCH.example = let view = &value

LIFETIME_E_BRANCH_MISMATCH.summary = branch incompatibilidade.
LIFETIME_E_BRANCH_MISMATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_BRANCH_MISMATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_BRANCH_MISMATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_BRANCH_MISMATCH.example = let view = &value

LIFETIME_E_INVALID_CALL.summary = chamada invalido.
LIFETIME_E_INVALID_CALL.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_CALL.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_CALL.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_CALL.example = let view = &value

LIFETIME_E_INVALID_CAST.summary = conversao invalido.
LIFETIME_E_INVALID_CAST.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_CAST.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_CAST.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_CAST.example = let view = &value

LIFETIME_E_INVALID_INDEX.summary = indice invalido.
LIFETIME_E_INVALID_INDEX.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_INDEX.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_INDEX.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_INDEX.example = let view = &value

LIFETIME_E_INVALID_DEREF.summary = deref invalido.
LIFETIME_E_INVALID_DEREF.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_DEREF.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_DEREF.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_DEREF.example = let view = &value

LIFETIME_E_INVALID_BORROW.summary = emprestimo invalido.
LIFETIME_E_INVALID_BORROW.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_BORROW.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_BORROW.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_BORROW.example = let view = &value

LIFETIME_E_INVALID_MOVE.summary = movimento invalido.
LIFETIME_E_INVALID_MOVE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_INVALID_MOVE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_INVALID_MOVE.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_INVALID_MOVE.example = let view = &value

LIFETIME_E_USE_AFTER_MOVE.summary = use after movimento.
LIFETIME_E_USE_AFTER_MOVE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_USE_AFTER_MOVE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_USE_AFTER_MOVE.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_USE_AFTER_MOVE.example = let view = &value

LIFETIME_E_USE_AFTER_DROP.summary = use after descarte.
LIFETIME_E_USE_AFTER_DROP.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_USE_AFTER_DROP.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_USE_AFTER_DROP.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_USE_AFTER_DROP.example = let view = &value

LIFETIME_E_USE_BEFORE_INIT.summary = use before init.
LIFETIME_E_USE_BEFORE_INIT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_USE_BEFORE_INIT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_USE_BEFORE_INIT.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_USE_BEFORE_INIT.example = let view = &value

LIFETIME_E_DOUBLE_DROP.summary = double descarte.
LIFETIME_E_DOUBLE_DROP.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_DOUBLE_DROP.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_DOUBLE_DROP.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_DOUBLE_DROP.example = let view = &value

LIFETIME_E_BORROW_CONFLICT.summary = emprestimo conflito.
LIFETIME_E_BORROW_CONFLICT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_BORROW_CONFLICT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_BORROW_CONFLICT.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_BORROW_CONFLICT.example = let view = &value

LIFETIME_E_MUTABILITY_CONFLICT.summary = mutability conflito.
LIFETIME_E_MUTABILITY_CONFLICT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_MUTABILITY_CONFLICT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_MUTABILITY_CONFLICT.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_MUTABILITY_CONFLICT.example = let view = &value

LIFETIME_E_LIFETIME_TOO_SHORT.summary = tempo de vida demais short.
LIFETIME_E_LIFETIME_TOO_SHORT.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_LIFETIME_TOO_SHORT.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_LIFETIME_TOO_SHORT.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_LIFETIME_TOO_SHORT.example = let view = &value

LIFETIME_E_DANGLING_REFERENCE.summary = dangling referencia.
LIFETIME_E_DANGLING_REFERENCE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_DANGLING_REFERENCE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_DANGLING_REFERENCE.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_DANGLING_REFERENCE.example = let view = &value

LIFETIME_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
LIFETIME_E_NON_EXHAUSTIVE_MATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_NON_EXHAUSTIVE_MATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_NON_EXHAUSTIVE_MATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_NON_EXHAUSTIVE_MATCH.example = let view = &value

LIFETIME_E_UNREACHABLE_PATTERN.summary = inalcancavel padrao.
LIFETIME_E_UNREACHABLE_PATTERN.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_UNREACHABLE_PATTERN.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_UNREACHABLE_PATTERN.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_UNREACHABLE_PATTERN.example = let view = &value

LIFETIME_E_CONST_REQUIRED.summary = const required.
LIFETIME_E_CONST_REQUIRED.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_CONST_REQUIRED.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_CONST_REQUIRED.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_CONST_REQUIRED.example = let view = &value

LIFETIME_E_CONST_OVERFLOW.summary = const estouro.
LIFETIME_E_CONST_OVERFLOW.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_CONST_OVERFLOW.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_CONST_OVERFLOW.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_CONST_OVERFLOW.example = let view = &value

LIFETIME_E_CONST_DIVISION_BY_ZERO.summary = const divisao by zero.
LIFETIME_E_CONST_DIVISION_BY_ZERO.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_CONST_DIVISION_BY_ZERO.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_CONST_DIVISION_BY_ZERO.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_CONST_DIVISION_BY_ZERO.example = let view = &value

LIFETIME_E_CONST_CYCLE.summary = const ciclo.
LIFETIME_E_CONST_CYCLE.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_CONST_CYCLE.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_CONST_CYCLE.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_CONST_CYCLE.example = let view = &value

LIFETIME_E_MACRO_NOT_FOUND.summary = macro ausente.
LIFETIME_E_MACRO_NOT_FOUND.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_MACRO_NOT_FOUND.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_MACRO_NOT_FOUND.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_MACRO_NOT_FOUND.example = let view = &value

LIFETIME_E_MACRO_RECURSION.summary = macro recursion.
LIFETIME_E_MACRO_RECURSION.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_MACRO_RECURSION.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_MACRO_RECURSION.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_MACRO_RECURSION.example = let view = &value

LIFETIME_E_MACRO_EXPANSION_FAILED.summary = macro expansion falhou.
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

LIFETIME_E_GENERIC_ARGUMENT_MISSING.summary = generico argumento ausente.
LIFETIME_E_GENERIC_ARGUMENT_MISSING.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_GENERIC_ARGUMENT_MISSING.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_GENERIC_ARGUMENT_MISSING.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_GENERIC_ARGUMENT_MISSING.example = let view = &value

LIFETIME_E_GENERIC_BOUND_FAILED.summary = generico bound falhou.
LIFETIME_E_GENERIC_BOUND_FAILED.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_GENERIC_BOUND_FAILED.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_GENERIC_BOUND_FAILED.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_GENERIC_BOUND_FAILED.example = let view = &value

LIFETIME_E_UNSUPPORTED_TARGET.summary = destino nao suportado.
LIFETIME_E_UNSUPPORTED_TARGET.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_UNSUPPORTED_TARGET.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_UNSUPPORTED_TARGET.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_UNSUPPORTED_TARGET.example = let view = &value

LIFETIME_E_ABI_MISMATCH.summary = abi incompatibilidade.
LIFETIME_E_ABI_MISMATCH.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_ABI_MISMATCH.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_ABI_MISMATCH.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_ABI_MISMATCH.example = let view = &value

LIFETIME_E_LINK_FAILED.summary = link falhou.
LIFETIME_E_LINK_FAILED.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_LINK_FAILED.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_LINK_FAILED.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_LINK_FAILED.example = let view = &value

LIFETIME_E_RUNTIME_PANIC.summary = runtime panico.
LIFETIME_E_RUNTIME_PANIC.cause = A value was moved, borrowed, assigned, or dropped in an invalid order.
LIFETIME_E_RUNTIME_PANIC.step1 = Find the earlier move or borrow mentioned by the diagnostic.
LIFETIME_E_RUNTIME_PANIC.fix = repair the ownership transition named by the borrow checker before the highlighted use
LIFETIME_E_RUNTIME_PANIC.example = let view = &value

CONST_E_EXPECTED_IDENTIFIER.summary = identificador esperado.
CONST_E_EXPECTED_IDENTIFIER.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_EXPECTED_IDENTIFIER.step1 = Reduce the constant expression at the reported span.
CONST_E_EXPECTED_IDENTIFIER.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_EXPECTED_IDENTIFIER.example = const size: int = 4

CONST_E_EXPECTED_EXPRESSION.summary = expressao esperado.
CONST_E_EXPECTED_EXPRESSION.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_EXPECTED_EXPRESSION.step1 = Reduce the constant expression at the reported span.
CONST_E_EXPECTED_EXPRESSION.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_EXPECTED_EXPRESSION.example = const size: int = 4

CONST_E_EXPECTED_TYPE.summary = tipo esperado.
CONST_E_EXPECTED_TYPE.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_EXPECTED_TYPE.step1 = Reduce the constant expression at the reported span.
CONST_E_EXPECTED_TYPE.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_EXPECTED_TYPE.example = const size: int = 4

CONST_E_EXPECTED_PATTERN.summary = padrao esperado.
CONST_E_EXPECTED_PATTERN.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_EXPECTED_PATTERN.step1 = Reduce the constant expression at the reported span.
CONST_E_EXPECTED_PATTERN.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_EXPECTED_PATTERN.example = const size: int = 4

CONST_E_EXPECTED_BLOCK.summary = bloco esperado.
CONST_E_EXPECTED_BLOCK.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_EXPECTED_BLOCK.step1 = Reduce the constant expression at the reported span.
CONST_E_EXPECTED_BLOCK.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_EXPECTED_BLOCK.example = const size: int = 4

CONST_E_EXPECTED_DELIMITER.summary = delimitador esperado.
CONST_E_EXPECTED_DELIMITER.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_EXPECTED_DELIMITER.step1 = Reduce the constant expression at the reported span.
CONST_E_EXPECTED_DELIMITER.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_EXPECTED_DELIMITER.example = const size: int = 4

CONST_E_UNEXPECTED_TOKEN.summary = token inesperado.
CONST_E_UNEXPECTED_TOKEN.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_UNEXPECTED_TOKEN.step1 = Reduce the constant expression at the reported span.
CONST_E_UNEXPECTED_TOKEN.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_UNEXPECTED_TOKEN.example = const size: int = 4

CONST_E_UNBALANCED_DELIMITER.summary = unbalanced delimitador.
CONST_E_UNBALANCED_DELIMITER.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_UNBALANCED_DELIMITER.step1 = Reduce the constant expression at the reported span.
CONST_E_UNBALANCED_DELIMITER.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_UNBALANCED_DELIMITER.example = const size: int = 4

CONST_E_INVALID_ATTRIBUTE.summary = attribute invalido.
CONST_E_INVALID_ATTRIBUTE.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_ATTRIBUTE.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_ATTRIBUTE.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_ATTRIBUTE.example = const size: int = 4

CONST_E_INVALID_DECLARATION.summary = declaracao invalido.
CONST_E_INVALID_DECLARATION.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_DECLARATION.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_DECLARATION.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_DECLARATION.example = const size: int = 4

CONST_E_INVALID_STATEMENT.summary = instrucao invalido.
CONST_E_INVALID_STATEMENT.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_STATEMENT.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_STATEMENT.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_STATEMENT.example = const size: int = 4

CONST_E_INVALID_EXPRESSION.summary = expressao invalido.
CONST_E_INVALID_EXPRESSION.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_EXPRESSION.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_EXPRESSION.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_EXPRESSION.example = const size: int = 4

CONST_E_INVALID_PATTERN.summary = padrao invalido.
CONST_E_INVALID_PATTERN.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_PATTERN.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_PATTERN.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_PATTERN.example = const size: int = 4

CONST_E_INVALID_LITERAL.summary = literal invalido.
CONST_E_INVALID_LITERAL.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_LITERAL.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_LITERAL.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_LITERAL.example = const size: int = 4

CONST_E_INVALID_OPERATOR.summary = operador invalido.
CONST_E_INVALID_OPERATOR.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_OPERATOR.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_OPERATOR.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_OPERATOR.example = const size: int = 4

CONST_E_INVALID_MODIFIER.summary = modifier invalido.
CONST_E_INVALID_MODIFIER.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_MODIFIER.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_MODIFIER.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_MODIFIER.example = const size: int = 4

CONST_E_MISSING_BODY.summary = body ausente.
CONST_E_MISSING_BODY.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_MISSING_BODY.step1 = Reduce the constant expression at the reported span.
CONST_E_MISSING_BODY.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_MISSING_BODY.example = const size: int = 4

CONST_E_MISSING_RETURN.summary = retorno ausente.
CONST_E_MISSING_RETURN.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_MISSING_RETURN.step1 = Reduce the constant expression at the reported span.
CONST_E_MISSING_RETURN.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_MISSING_RETURN.example = const size: int = 4

CONST_E_DUPLICATE_NAME.summary = nome duplicado.
CONST_E_DUPLICATE_NAME.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_DUPLICATE_NAME.step1 = Reduce the constant expression at the reported span.
CONST_E_DUPLICATE_NAME.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_DUPLICATE_NAME.example = const size: int = 4

CONST_E_UNKNOWN_NAME.summary = nome desconhecido.
CONST_E_UNKNOWN_NAME.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_UNKNOWN_NAME.step1 = Reduce the constant expression at the reported span.
CONST_E_UNKNOWN_NAME.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_UNKNOWN_NAME.example = const size: int = 4

CONST_E_UNKNOWN_TYPE.summary = tipo desconhecido.
CONST_E_UNKNOWN_TYPE.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_UNKNOWN_TYPE.step1 = Reduce the constant expression at the reported span.
CONST_E_UNKNOWN_TYPE.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_UNKNOWN_TYPE.example = const size: int = 4

CONST_E_UNKNOWN_MODULE.summary = modulo desconhecido.
CONST_E_UNKNOWN_MODULE.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_UNKNOWN_MODULE.step1 = Reduce the constant expression at the reported span.
CONST_E_UNKNOWN_MODULE.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_UNKNOWN_MODULE.example = const size: int = 4

CONST_E_UNKNOWN_MEMBER.summary = membro desconhecido.
CONST_E_UNKNOWN_MEMBER.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_UNKNOWN_MEMBER.step1 = Reduce the constant expression at the reported span.
CONST_E_UNKNOWN_MEMBER.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_UNKNOWN_MEMBER.example = const size: int = 4

CONST_E_AMBIGUOUS_NAME.summary = ambiguous nome.
CONST_E_AMBIGUOUS_NAME.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_AMBIGUOUS_NAME.step1 = Reduce the constant expression at the reported span.
CONST_E_AMBIGUOUS_NAME.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_AMBIGUOUS_NAME.example = const size: int = 4

CONST_E_PRIVATE_SYMBOL.summary = private simbolo.
CONST_E_PRIVATE_SYMBOL.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_PRIVATE_SYMBOL.step1 = Reduce the constant expression at the reported span.
CONST_E_PRIVATE_SYMBOL.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_PRIVATE_SYMBOL.example = const size: int = 4

CONST_E_IMPORT_NOT_FOUND.summary = importacao ausente.
CONST_E_IMPORT_NOT_FOUND.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_IMPORT_NOT_FOUND.step1 = Reduce the constant expression at the reported span.
CONST_E_IMPORT_NOT_FOUND.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_IMPORT_NOT_FOUND.example = const size: int = 4

CONST_E_IMPORT_CYCLE.summary = importacao ciclo.
CONST_E_IMPORT_CYCLE.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_IMPORT_CYCLE.step1 = Reduce the constant expression at the reported span.
CONST_E_IMPORT_CYCLE.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_IMPORT_CYCLE.example = const size: int = 4

CONST_E_EXPORT_CONFLICT.summary = export conflito.
CONST_E_EXPORT_CONFLICT.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_EXPORT_CONFLICT.step1 = Reduce the constant expression at the reported span.
CONST_E_EXPORT_CONFLICT.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_EXPORT_CONFLICT.example = const size: int = 4

CONST_E_ARITY_MISMATCH.summary = arity incompatibilidade.
CONST_E_ARITY_MISMATCH.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_ARITY_MISMATCH.step1 = Reduce the constant expression at the reported span.
CONST_E_ARITY_MISMATCH.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_ARITY_MISMATCH.example = const size: int = 4

CONST_E_ARGUMENT_MISMATCH.summary = argumento incompatibilidade.
CONST_E_ARGUMENT_MISMATCH.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_ARGUMENT_MISMATCH.step1 = Reduce the constant expression at the reported span.
CONST_E_ARGUMENT_MISMATCH.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_ARGUMENT_MISMATCH.example = const size: int = 4

CONST_E_ASSIGNMENT_MISMATCH.summary = atribuicao incompatibilidade.
CONST_E_ASSIGNMENT_MISMATCH.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_ASSIGNMENT_MISMATCH.step1 = Reduce the constant expression at the reported span.
CONST_E_ASSIGNMENT_MISMATCH.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_ASSIGNMENT_MISMATCH.example = const size: int = 4

CONST_E_BRANCH_MISMATCH.summary = branch incompatibilidade.
CONST_E_BRANCH_MISMATCH.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_BRANCH_MISMATCH.step1 = Reduce the constant expression at the reported span.
CONST_E_BRANCH_MISMATCH.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_BRANCH_MISMATCH.example = const size: int = 4

CONST_E_INVALID_CALL.summary = chamada invalido.
CONST_E_INVALID_CALL.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_CALL.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_CALL.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_CALL.example = const size: int = 4

CONST_E_INVALID_CAST.summary = conversao invalido.
CONST_E_INVALID_CAST.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_CAST.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_CAST.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_CAST.example = const size: int = 4

CONST_E_INVALID_INDEX.summary = indice invalido.
CONST_E_INVALID_INDEX.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_INDEX.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_INDEX.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_INDEX.example = const size: int = 4

CONST_E_INVALID_DEREF.summary = deref invalido.
CONST_E_INVALID_DEREF.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_DEREF.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_DEREF.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_DEREF.example = const size: int = 4

CONST_E_INVALID_BORROW.summary = emprestimo invalido.
CONST_E_INVALID_BORROW.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_BORROW.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_BORROW.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_BORROW.example = const size: int = 4

CONST_E_INVALID_MOVE.summary = movimento invalido.
CONST_E_INVALID_MOVE.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_INVALID_MOVE.step1 = Reduce the constant expression at the reported span.
CONST_E_INVALID_MOVE.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_INVALID_MOVE.example = const size: int = 4

CONST_E_USE_AFTER_MOVE.summary = use after movimento.
CONST_E_USE_AFTER_MOVE.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_USE_AFTER_MOVE.step1 = Reduce the constant expression at the reported span.
CONST_E_USE_AFTER_MOVE.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_USE_AFTER_MOVE.example = const size: int = 4

CONST_E_USE_AFTER_DROP.summary = use after descarte.
CONST_E_USE_AFTER_DROP.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_USE_AFTER_DROP.step1 = Reduce the constant expression at the reported span.
CONST_E_USE_AFTER_DROP.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_USE_AFTER_DROP.example = const size: int = 4

CONST_E_USE_BEFORE_INIT.summary = use before init.
CONST_E_USE_BEFORE_INIT.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_USE_BEFORE_INIT.step1 = Reduce the constant expression at the reported span.
CONST_E_USE_BEFORE_INIT.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_USE_BEFORE_INIT.example = const size: int = 4

CONST_E_DOUBLE_DROP.summary = double descarte.
CONST_E_DOUBLE_DROP.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_DOUBLE_DROP.step1 = Reduce the constant expression at the reported span.
CONST_E_DOUBLE_DROP.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_DOUBLE_DROP.example = const size: int = 4

CONST_E_BORROW_CONFLICT.summary = emprestimo conflito.
CONST_E_BORROW_CONFLICT.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_BORROW_CONFLICT.step1 = Reduce the constant expression at the reported span.
CONST_E_BORROW_CONFLICT.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_BORROW_CONFLICT.example = const size: int = 4

CONST_E_MUTABILITY_CONFLICT.summary = mutability conflito.
CONST_E_MUTABILITY_CONFLICT.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_MUTABILITY_CONFLICT.step1 = Reduce the constant expression at the reported span.
CONST_E_MUTABILITY_CONFLICT.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_MUTABILITY_CONFLICT.example = const size: int = 4

CONST_E_LIFETIME_TOO_SHORT.summary = tempo de vida demais short.
CONST_E_LIFETIME_TOO_SHORT.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_LIFETIME_TOO_SHORT.step1 = Reduce the constant expression at the reported span.
CONST_E_LIFETIME_TOO_SHORT.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_LIFETIME_TOO_SHORT.example = const size: int = 4

CONST_E_DANGLING_REFERENCE.summary = dangling referencia.
CONST_E_DANGLING_REFERENCE.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_DANGLING_REFERENCE.step1 = Reduce the constant expression at the reported span.
CONST_E_DANGLING_REFERENCE.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_DANGLING_REFERENCE.example = const size: int = 4

CONST_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
CONST_E_NON_EXHAUSTIVE_MATCH.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_NON_EXHAUSTIVE_MATCH.step1 = Reduce the constant expression at the reported span.
CONST_E_NON_EXHAUSTIVE_MATCH.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_NON_EXHAUSTIVE_MATCH.example = const size: int = 4

CONST_E_UNREACHABLE_PATTERN.summary = inalcancavel padrao.
CONST_E_UNREACHABLE_PATTERN.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_UNREACHABLE_PATTERN.step1 = Reduce the constant expression at the reported span.
CONST_E_UNREACHABLE_PATTERN.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_UNREACHABLE_PATTERN.example = const size: int = 4

CONST_E_CONST_REQUIRED.summary = const required.
CONST_E_CONST_REQUIRED.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_CONST_REQUIRED.step1 = Reduce the constant expression at the reported span.
CONST_E_CONST_REQUIRED.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_CONST_REQUIRED.example = const size: int = 4

CONST_E_CONST_OVERFLOW.summary = const estouro.
CONST_E_CONST_OVERFLOW.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_CONST_OVERFLOW.step1 = Reduce the constant expression at the reported span.
CONST_E_CONST_OVERFLOW.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_CONST_OVERFLOW.example = const size: int = 4

CONST_E_CONST_DIVISION_BY_ZERO.summary = const divisao by zero.
CONST_E_CONST_DIVISION_BY_ZERO.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_CONST_DIVISION_BY_ZERO.step1 = Reduce the constant expression at the reported span.
CONST_E_CONST_DIVISION_BY_ZERO.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_CONST_DIVISION_BY_ZERO.example = const size: int = 4

CONST_E_CONST_CYCLE.summary = const ciclo.
CONST_E_CONST_CYCLE.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_CONST_CYCLE.step1 = Reduce the constant expression at the reported span.
CONST_E_CONST_CYCLE.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_CONST_CYCLE.example = const size: int = 4

CONST_E_MACRO_NOT_FOUND.summary = macro ausente.
CONST_E_MACRO_NOT_FOUND.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_MACRO_NOT_FOUND.step1 = Reduce the constant expression at the reported span.
CONST_E_MACRO_NOT_FOUND.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_MACRO_NOT_FOUND.example = const size: int = 4

CONST_E_MACRO_RECURSION.summary = macro recursion.
CONST_E_MACRO_RECURSION.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_MACRO_RECURSION.step1 = Reduce the constant expression at the reported span.
CONST_E_MACRO_RECURSION.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_MACRO_RECURSION.example = const size: int = 4

CONST_E_MACRO_EXPANSION_FAILED.summary = macro expansion falhou.
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

CONST_E_GENERIC_ARGUMENT_MISSING.summary = generico argumento ausente.
CONST_E_GENERIC_ARGUMENT_MISSING.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_GENERIC_ARGUMENT_MISSING.step1 = Reduce the constant expression at the reported span.
CONST_E_GENERIC_ARGUMENT_MISSING.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_GENERIC_ARGUMENT_MISSING.example = const size: int = 4

CONST_E_GENERIC_BOUND_FAILED.summary = generico bound falhou.
CONST_E_GENERIC_BOUND_FAILED.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_GENERIC_BOUND_FAILED.step1 = Reduce the constant expression at the reported span.
CONST_E_GENERIC_BOUND_FAILED.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_GENERIC_BOUND_FAILED.example = const size: int = 4

CONST_E_UNSUPPORTED_TARGET.summary = destino nao suportado.
CONST_E_UNSUPPORTED_TARGET.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_UNSUPPORTED_TARGET.step1 = Reduce the constant expression at the reported span.
CONST_E_UNSUPPORTED_TARGET.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_UNSUPPORTED_TARGET.example = const size: int = 4

CONST_E_ABI_MISMATCH.summary = abi incompatibilidade.
CONST_E_ABI_MISMATCH.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_ABI_MISMATCH.step1 = Reduce the constant expression at the reported span.
CONST_E_ABI_MISMATCH.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_ABI_MISMATCH.example = const size: int = 4

CONST_E_LINK_FAILED.summary = link falhou.
CONST_E_LINK_FAILED.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_LINK_FAILED.step1 = Reduce the constant expression at the reported span.
CONST_E_LINK_FAILED.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_LINK_FAILED.example = const size: int = 4

CONST_E_RUNTIME_PANIC.summary = runtime panico.
CONST_E_RUNTIME_PANIC.cause = A compile-time expression used an operation that cannot be evaluated safely.
CONST_E_RUNTIME_PANIC.step1 = Reduce the constant expression at the reported span.
CONST_E_RUNTIME_PANIC.fix = rewrite the highlighted constant expression so const evaluation can prove it safely
CONST_E_RUNTIME_PANIC.example = const size: int = 4

MACRO_E_EXPECTED_IDENTIFIER.summary = identificador esperado.
MACRO_E_EXPECTED_IDENTIFIER.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_EXPECTED_IDENTIFIER.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_EXPECTED_IDENTIFIER.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_EXPECTED_IDENTIFIER.example = vitte check path/to/file.vit

MACRO_E_EXPECTED_EXPRESSION.summary = expressao esperado.
MACRO_E_EXPECTED_EXPRESSION.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_EXPECTED_EXPRESSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_EXPECTED_EXPRESSION.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_EXPECTED_EXPRESSION.example = vitte check path/to/file.vit

MACRO_E_EXPECTED_TYPE.summary = tipo esperado.
MACRO_E_EXPECTED_TYPE.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_EXPECTED_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_EXPECTED_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_EXPECTED_TYPE.example = vitte check path/to/file.vit

MACRO_E_EXPECTED_PATTERN.summary = padrao esperado.
MACRO_E_EXPECTED_PATTERN.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_EXPECTED_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_EXPECTED_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_EXPECTED_PATTERN.example = vitte check path/to/file.vit

MACRO_E_EXPECTED_BLOCK.summary = bloco esperado.
MACRO_E_EXPECTED_BLOCK.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_EXPECTED_BLOCK.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_EXPECTED_BLOCK.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_EXPECTED_BLOCK.example = vitte check path/to/file.vit

MACRO_E_EXPECTED_DELIMITER.summary = delimitador esperado.
MACRO_E_EXPECTED_DELIMITER.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_EXPECTED_DELIMITER.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_EXPECTED_DELIMITER.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_EXPECTED_DELIMITER.example = vitte check path/to/file.vit

MACRO_E_UNEXPECTED_TOKEN.summary = token inesperado.
MACRO_E_UNEXPECTED_TOKEN.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_UNEXPECTED_TOKEN.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_UNEXPECTED_TOKEN.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_UNEXPECTED_TOKEN.example = vitte check path/to/file.vit

MACRO_E_UNBALANCED_DELIMITER.summary = unbalanced delimitador.
MACRO_E_UNBALANCED_DELIMITER.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_UNBALANCED_DELIMITER.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_UNBALANCED_DELIMITER.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_UNBALANCED_DELIMITER.example = vitte check path/to/file.vit

MACRO_E_INVALID_ATTRIBUTE.summary = attribute invalido.
MACRO_E_INVALID_ATTRIBUTE.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_ATTRIBUTE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_ATTRIBUTE.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_ATTRIBUTE.example = vitte check path/to/file.vit

MACRO_E_INVALID_DECLARATION.summary = declaracao invalido.
MACRO_E_INVALID_DECLARATION.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_DECLARATION.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_DECLARATION.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_DECLARATION.example = vitte check path/to/file.vit

MACRO_E_INVALID_STATEMENT.summary = instrucao invalido.
MACRO_E_INVALID_STATEMENT.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_STATEMENT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_STATEMENT.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_STATEMENT.example = vitte check path/to/file.vit

MACRO_E_INVALID_EXPRESSION.summary = expressao invalido.
MACRO_E_INVALID_EXPRESSION.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_EXPRESSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_EXPRESSION.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_EXPRESSION.example = vitte check path/to/file.vit

MACRO_E_INVALID_PATTERN.summary = padrao invalido.
MACRO_E_INVALID_PATTERN.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_PATTERN.example = vitte check path/to/file.vit

MACRO_E_INVALID_LITERAL.summary = literal invalido.
MACRO_E_INVALID_LITERAL.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_LITERAL.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_LITERAL.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_LITERAL.example = vitte check path/to/file.vit

MACRO_E_INVALID_OPERATOR.summary = operador invalido.
MACRO_E_INVALID_OPERATOR.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_OPERATOR.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_OPERATOR.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_OPERATOR.example = vitte check path/to/file.vit

MACRO_E_INVALID_MODIFIER.summary = modifier invalido.
MACRO_E_INVALID_MODIFIER.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_MODIFIER.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_MODIFIER.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_MODIFIER.example = vitte check path/to/file.vit

MACRO_E_MISSING_BODY.summary = body ausente.
MACRO_E_MISSING_BODY.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_MISSING_BODY.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_MISSING_BODY.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_MISSING_BODY.example = vitte check path/to/file.vit

MACRO_E_MISSING_RETURN.summary = retorno ausente.
MACRO_E_MISSING_RETURN.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_MISSING_RETURN.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_MISSING_RETURN.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_MISSING_RETURN.example = vitte check path/to/file.vit

MACRO_E_DUPLICATE_NAME.summary = nome duplicado.
MACRO_E_DUPLICATE_NAME.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_DUPLICATE_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_DUPLICATE_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_DUPLICATE_NAME.example = vitte check path/to/file.vit

MACRO_E_UNKNOWN_NAME.summary = nome desconhecido.
MACRO_E_UNKNOWN_NAME.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_UNKNOWN_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_UNKNOWN_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_UNKNOWN_NAME.example = vitte check path/to/file.vit

MACRO_E_UNKNOWN_TYPE.summary = tipo desconhecido.
MACRO_E_UNKNOWN_TYPE.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_UNKNOWN_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_UNKNOWN_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_UNKNOWN_TYPE.example = vitte check path/to/file.vit

MACRO_E_UNKNOWN_MODULE.summary = modulo desconhecido.
MACRO_E_UNKNOWN_MODULE.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_UNKNOWN_MODULE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_UNKNOWN_MODULE.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_UNKNOWN_MODULE.example = vitte check path/to/file.vit

MACRO_E_UNKNOWN_MEMBER.summary = membro desconhecido.
MACRO_E_UNKNOWN_MEMBER.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_UNKNOWN_MEMBER.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_UNKNOWN_MEMBER.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_UNKNOWN_MEMBER.example = vitte check path/to/file.vit

MACRO_E_AMBIGUOUS_NAME.summary = ambiguous nome.
MACRO_E_AMBIGUOUS_NAME.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_AMBIGUOUS_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_AMBIGUOUS_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_AMBIGUOUS_NAME.example = vitte check path/to/file.vit

MACRO_E_PRIVATE_SYMBOL.summary = private simbolo.
MACRO_E_PRIVATE_SYMBOL.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_PRIVATE_SYMBOL.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_PRIVATE_SYMBOL.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_PRIVATE_SYMBOL.example = vitte check path/to/file.vit

MACRO_E_IMPORT_NOT_FOUND.summary = importacao ausente.
MACRO_E_IMPORT_NOT_FOUND.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_IMPORT_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_IMPORT_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_IMPORT_NOT_FOUND.example = vitte check path/to/file.vit

MACRO_E_IMPORT_CYCLE.summary = importacao ciclo.
MACRO_E_IMPORT_CYCLE.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_IMPORT_CYCLE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_IMPORT_CYCLE.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_IMPORT_CYCLE.example = vitte check path/to/file.vit

MACRO_E_EXPORT_CONFLICT.summary = export conflito.
MACRO_E_EXPORT_CONFLICT.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_EXPORT_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_EXPORT_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_EXPORT_CONFLICT.example = vitte check path/to/file.vit

MACRO_E_ARITY_MISMATCH.summary = arity incompatibilidade.
MACRO_E_ARITY_MISMATCH.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_ARITY_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_ARITY_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_ARITY_MISMATCH.example = vitte check path/to/file.vit

MACRO_E_ARGUMENT_MISMATCH.summary = argumento incompatibilidade.
MACRO_E_ARGUMENT_MISMATCH.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_ARGUMENT_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_ARGUMENT_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_ARGUMENT_MISMATCH.example = vitte check path/to/file.vit

MACRO_E_ASSIGNMENT_MISMATCH.summary = atribuicao incompatibilidade.
MACRO_E_ASSIGNMENT_MISMATCH.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_ASSIGNMENT_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_ASSIGNMENT_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_ASSIGNMENT_MISMATCH.example = vitte check path/to/file.vit

MACRO_E_BRANCH_MISMATCH.summary = branch incompatibilidade.
MACRO_E_BRANCH_MISMATCH.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_BRANCH_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_BRANCH_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_BRANCH_MISMATCH.example = vitte check path/to/file.vit

MACRO_E_INVALID_CALL.summary = chamada invalido.
MACRO_E_INVALID_CALL.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_CALL.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_CALL.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_CALL.example = vitte check path/to/file.vit

MACRO_E_INVALID_CAST.summary = conversao invalido.
MACRO_E_INVALID_CAST.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_CAST.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_CAST.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_CAST.example = vitte check path/to/file.vit

MACRO_E_INVALID_INDEX.summary = indice invalido.
MACRO_E_INVALID_INDEX.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_INDEX.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_INDEX.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_INDEX.example = vitte check path/to/file.vit

MACRO_E_INVALID_DEREF.summary = deref invalido.
MACRO_E_INVALID_DEREF.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_DEREF.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_DEREF.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_DEREF.example = vitte check path/to/file.vit

MACRO_E_INVALID_BORROW.summary = emprestimo invalido.
MACRO_E_INVALID_BORROW.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_BORROW.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_BORROW.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_BORROW.example = vitte check path/to/file.vit

MACRO_E_INVALID_MOVE.summary = movimento invalido.
MACRO_E_INVALID_MOVE.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_INVALID_MOVE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_INVALID_MOVE.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_INVALID_MOVE.example = vitte check path/to/file.vit

MACRO_E_USE_AFTER_MOVE.summary = use after movimento.
MACRO_E_USE_AFTER_MOVE.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_USE_AFTER_MOVE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_USE_AFTER_MOVE.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_USE_AFTER_MOVE.example = vitte check path/to/file.vit

MACRO_E_USE_AFTER_DROP.summary = use after descarte.
MACRO_E_USE_AFTER_DROP.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_USE_AFTER_DROP.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_USE_AFTER_DROP.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_USE_AFTER_DROP.example = vitte check path/to/file.vit

MACRO_E_USE_BEFORE_INIT.summary = use before init.
MACRO_E_USE_BEFORE_INIT.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_USE_BEFORE_INIT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_USE_BEFORE_INIT.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_USE_BEFORE_INIT.example = vitte check path/to/file.vit

MACRO_E_DOUBLE_DROP.summary = double descarte.
MACRO_E_DOUBLE_DROP.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_DOUBLE_DROP.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_DOUBLE_DROP.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_DOUBLE_DROP.example = vitte check path/to/file.vit

MACRO_E_BORROW_CONFLICT.summary = emprestimo conflito.
MACRO_E_BORROW_CONFLICT.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_BORROW_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_BORROW_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_BORROW_CONFLICT.example = vitte check path/to/file.vit

MACRO_E_MUTABILITY_CONFLICT.summary = mutability conflito.
MACRO_E_MUTABILITY_CONFLICT.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_MUTABILITY_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_MUTABILITY_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_MUTABILITY_CONFLICT.example = vitte check path/to/file.vit

MACRO_E_LIFETIME_TOO_SHORT.summary = tempo de vida demais short.
MACRO_E_LIFETIME_TOO_SHORT.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_LIFETIME_TOO_SHORT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_LIFETIME_TOO_SHORT.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_LIFETIME_TOO_SHORT.example = vitte check path/to/file.vit

MACRO_E_DANGLING_REFERENCE.summary = dangling referencia.
MACRO_E_DANGLING_REFERENCE.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_DANGLING_REFERENCE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_DANGLING_REFERENCE.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_DANGLING_REFERENCE.example = vitte check path/to/file.vit

MACRO_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
MACRO_E_NON_EXHAUSTIVE_MATCH.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_NON_EXHAUSTIVE_MATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_NON_EXHAUSTIVE_MATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_NON_EXHAUSTIVE_MATCH.example = vitte check path/to/file.vit

MACRO_E_UNREACHABLE_PATTERN.summary = inalcancavel padrao.
MACRO_E_UNREACHABLE_PATTERN.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_UNREACHABLE_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_UNREACHABLE_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_UNREACHABLE_PATTERN.example = vitte check path/to/file.vit

MACRO_E_CONST_REQUIRED.summary = const required.
MACRO_E_CONST_REQUIRED.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_CONST_REQUIRED.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_CONST_REQUIRED.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_CONST_REQUIRED.example = vitte check path/to/file.vit

MACRO_E_CONST_OVERFLOW.summary = const estouro.
MACRO_E_CONST_OVERFLOW.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_CONST_OVERFLOW.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_CONST_OVERFLOW.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_CONST_OVERFLOW.example = vitte check path/to/file.vit

MACRO_E_CONST_DIVISION_BY_ZERO.summary = const divisao by zero.
MACRO_E_CONST_DIVISION_BY_ZERO.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_CONST_DIVISION_BY_ZERO.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_CONST_DIVISION_BY_ZERO.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_CONST_DIVISION_BY_ZERO.example = vitte check path/to/file.vit

MACRO_E_CONST_CYCLE.summary = const ciclo.
MACRO_E_CONST_CYCLE.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_CONST_CYCLE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_CONST_CYCLE.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_CONST_CYCLE.example = vitte check path/to/file.vit

MACRO_E_MACRO_NOT_FOUND.summary = macro ausente.
MACRO_E_MACRO_NOT_FOUND.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_MACRO_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_MACRO_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_MACRO_NOT_FOUND.example = vitte check path/to/file.vit

MACRO_E_MACRO_RECURSION.summary = macro recursion.
MACRO_E_MACRO_RECURSION.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_MACRO_RECURSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_MACRO_RECURSION.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_MACRO_RECURSION.example = vitte check path/to/file.vit

MACRO_E_MACRO_EXPANSION_FAILED.summary = macro expansion falhou.
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

MACRO_E_GENERIC_ARGUMENT_MISSING.summary = generico argumento ausente.
MACRO_E_GENERIC_ARGUMENT_MISSING.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_GENERIC_ARGUMENT_MISSING.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_GENERIC_ARGUMENT_MISSING.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_GENERIC_ARGUMENT_MISSING.example = vitte check path/to/file.vit

MACRO_E_GENERIC_BOUND_FAILED.summary = generico bound falhou.
MACRO_E_GENERIC_BOUND_FAILED.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_GENERIC_BOUND_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_GENERIC_BOUND_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_GENERIC_BOUND_FAILED.example = vitte check path/to/file.vit

MACRO_E_UNSUPPORTED_TARGET.summary = destino nao suportado.
MACRO_E_UNSUPPORTED_TARGET.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_UNSUPPORTED_TARGET.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_UNSUPPORTED_TARGET.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_UNSUPPORTED_TARGET.example = vitte check path/to/file.vit

MACRO_E_ABI_MISMATCH.summary = abi incompatibilidade.
MACRO_E_ABI_MISMATCH.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_ABI_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_ABI_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_ABI_MISMATCH.example = vitte check path/to/file.vit

MACRO_E_LINK_FAILED.summary = link falhou.
MACRO_E_LINK_FAILED.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_LINK_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_LINK_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_LINK_FAILED.example = vitte check path/to/file.vit

MACRO_E_RUNTIME_PANIC.summary = runtime panico.
MACRO_E_RUNTIME_PANIC.cause = The macro expansion phase found code that violates this diagnostic rule.
MACRO_E_RUNTIME_PANIC.step1 = Fix the first span reported for this diagnostic, then run the command again.
MACRO_E_RUNTIME_PANIC.fix = repair the highlighted compiler contract before checking later diagnostics
MACRO_E_RUNTIME_PANIC.example = vitte check path/to/file.vit

HIR_E_EXPECTED_IDENTIFIER.summary = identificador esperado.
HIR_E_EXPECTED_IDENTIFIER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_EXPECTED_IDENTIFIER.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_EXPECTED_IDENTIFIER.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_EXPECTED_IDENTIFIER.example = vitte check path/to/file.vit

HIR_E_EXPECTED_EXPRESSION.summary = expressao esperado.
HIR_E_EXPECTED_EXPRESSION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_EXPECTED_EXPRESSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_EXPECTED_EXPRESSION.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_EXPECTED_EXPRESSION.example = vitte check path/to/file.vit

HIR_E_EXPECTED_TYPE.summary = tipo esperado.
HIR_E_EXPECTED_TYPE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_EXPECTED_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_EXPECTED_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_EXPECTED_TYPE.example = vitte check path/to/file.vit

HIR_E_EXPECTED_PATTERN.summary = padrao esperado.
HIR_E_EXPECTED_PATTERN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_EXPECTED_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_EXPECTED_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_EXPECTED_PATTERN.example = vitte check path/to/file.vit

HIR_E_EXPECTED_BLOCK.summary = bloco esperado.
HIR_E_EXPECTED_BLOCK.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_EXPECTED_BLOCK.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_EXPECTED_BLOCK.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_EXPECTED_BLOCK.example = vitte check path/to/file.vit

HIR_E_EXPECTED_DELIMITER.summary = delimitador esperado.
HIR_E_EXPECTED_DELIMITER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_EXPECTED_DELIMITER.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_EXPECTED_DELIMITER.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_EXPECTED_DELIMITER.example = vitte check path/to/file.vit

HIR_E_UNEXPECTED_TOKEN.summary = token inesperado.
HIR_E_UNEXPECTED_TOKEN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_UNEXPECTED_TOKEN.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_UNEXPECTED_TOKEN.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_UNEXPECTED_TOKEN.example = vitte check path/to/file.vit

HIR_E_UNBALANCED_DELIMITER.summary = unbalanced delimitador.
HIR_E_UNBALANCED_DELIMITER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_UNBALANCED_DELIMITER.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_UNBALANCED_DELIMITER.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_UNBALANCED_DELIMITER.example = vitte check path/to/file.vit

HIR_E_INVALID_ATTRIBUTE.summary = attribute invalido.
HIR_E_INVALID_ATTRIBUTE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_ATTRIBUTE.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_ATTRIBUTE.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_ATTRIBUTE.example = vitte check path/to/file.vit

HIR_E_INVALID_DECLARATION.summary = declaracao invalido.
HIR_E_INVALID_DECLARATION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_DECLARATION.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_DECLARATION.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_DECLARATION.example = vitte check path/to/file.vit

HIR_E_INVALID_STATEMENT.summary = instrucao invalido.
HIR_E_INVALID_STATEMENT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_STATEMENT.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_STATEMENT.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_STATEMENT.example = vitte check path/to/file.vit

HIR_E_INVALID_EXPRESSION.summary = expressao invalido.
HIR_E_INVALID_EXPRESSION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_EXPRESSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_EXPRESSION.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_EXPRESSION.example = vitte check path/to/file.vit

HIR_E_INVALID_LITERAL.summary = literal invalido.
HIR_E_INVALID_LITERAL.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_LITERAL.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_LITERAL.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_LITERAL.example = vitte check path/to/file.vit

HIR_E_INVALID_OPERATOR.summary = operador invalido.
HIR_E_INVALID_OPERATOR.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_OPERATOR.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_OPERATOR.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_OPERATOR.example = vitte check path/to/file.vit

HIR_E_INVALID_MODIFIER.summary = modifier invalido.
HIR_E_INVALID_MODIFIER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_MODIFIER.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_MODIFIER.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_MODIFIER.example = vitte check path/to/file.vit

HIR_E_MISSING_BODY.summary = body ausente.
HIR_E_MISSING_BODY.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_MISSING_BODY.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_MISSING_BODY.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_MISSING_BODY.example = vitte check path/to/file.vit

HIR_E_MISSING_RETURN.summary = retorno ausente.
HIR_E_MISSING_RETURN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_MISSING_RETURN.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_MISSING_RETURN.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_MISSING_RETURN.example = vitte check path/to/file.vit

HIR_E_DUPLICATE_NAME.summary = nome duplicado.
HIR_E_DUPLICATE_NAME.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_DUPLICATE_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_DUPLICATE_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_DUPLICATE_NAME.example = vitte check path/to/file.vit

HIR_E_UNKNOWN_NAME.summary = nome desconhecido.
HIR_E_UNKNOWN_NAME.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_UNKNOWN_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_UNKNOWN_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_UNKNOWN_NAME.example = vitte check path/to/file.vit

HIR_E_UNKNOWN_TYPE.summary = tipo desconhecido.
HIR_E_UNKNOWN_TYPE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_UNKNOWN_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_UNKNOWN_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_UNKNOWN_TYPE.example = vitte check path/to/file.vit

HIR_E_UNKNOWN_MODULE.summary = modulo desconhecido.
HIR_E_UNKNOWN_MODULE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_UNKNOWN_MODULE.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_UNKNOWN_MODULE.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_UNKNOWN_MODULE.example = vitte check path/to/file.vit

HIR_E_UNKNOWN_MEMBER.summary = membro desconhecido.
HIR_E_UNKNOWN_MEMBER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_UNKNOWN_MEMBER.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_UNKNOWN_MEMBER.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_UNKNOWN_MEMBER.example = vitte check path/to/file.vit

HIR_E_AMBIGUOUS_NAME.summary = ambiguous nome.
HIR_E_AMBIGUOUS_NAME.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_AMBIGUOUS_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_AMBIGUOUS_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_AMBIGUOUS_NAME.example = vitte check path/to/file.vit

HIR_E_PRIVATE_SYMBOL.summary = private simbolo.
HIR_E_PRIVATE_SYMBOL.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_PRIVATE_SYMBOL.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_PRIVATE_SYMBOL.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_PRIVATE_SYMBOL.example = vitte check path/to/file.vit

HIR_E_IMPORT_NOT_FOUND.summary = importacao ausente.
HIR_E_IMPORT_NOT_FOUND.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_IMPORT_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_IMPORT_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_IMPORT_NOT_FOUND.example = vitte check path/to/file.vit

HIR_E_IMPORT_CYCLE.summary = importacao ciclo.
HIR_E_IMPORT_CYCLE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_IMPORT_CYCLE.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_IMPORT_CYCLE.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_IMPORT_CYCLE.example = vitte check path/to/file.vit

HIR_E_EXPORT_CONFLICT.summary = export conflito.
HIR_E_EXPORT_CONFLICT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_EXPORT_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_EXPORT_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_EXPORT_CONFLICT.example = vitte check path/to/file.vit

HIR_E_ARITY_MISMATCH.summary = arity incompatibilidade.
HIR_E_ARITY_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_ARITY_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_ARITY_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_ARITY_MISMATCH.example = vitte check path/to/file.vit

HIR_E_ARGUMENT_MISMATCH.summary = argumento incompatibilidade.
HIR_E_ARGUMENT_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_ARGUMENT_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_ARGUMENT_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_ARGUMENT_MISMATCH.example = vitte check path/to/file.vit

HIR_E_ASSIGNMENT_MISMATCH.summary = atribuicao incompatibilidade.
HIR_E_ASSIGNMENT_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_ASSIGNMENT_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_ASSIGNMENT_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_ASSIGNMENT_MISMATCH.example = vitte check path/to/file.vit

HIR_E_BRANCH_MISMATCH.summary = branch incompatibilidade.
HIR_E_BRANCH_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_BRANCH_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_BRANCH_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_BRANCH_MISMATCH.example = vitte check path/to/file.vit

HIR_E_INVALID_CALL.summary = chamada invalido.
HIR_E_INVALID_CALL.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_CALL.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_CALL.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_CALL.example = vitte check path/to/file.vit

HIR_E_INVALID_CAST.summary = conversao invalido.
HIR_E_INVALID_CAST.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_CAST.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_CAST.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_CAST.example = vitte check path/to/file.vit

HIR_E_INVALID_INDEX.summary = indice invalido.
HIR_E_INVALID_INDEX.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_INDEX.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_INDEX.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_INDEX.example = vitte check path/to/file.vit

HIR_E_INVALID_DEREF.summary = deref invalido.
HIR_E_INVALID_DEREF.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_DEREF.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_DEREF.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_DEREF.example = vitte check path/to/file.vit

HIR_E_INVALID_BORROW.summary = emprestimo invalido.
HIR_E_INVALID_BORROW.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_BORROW.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_BORROW.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_BORROW.example = vitte check path/to/file.vit

HIR_E_INVALID_MOVE.summary = movimento invalido.
HIR_E_INVALID_MOVE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_INVALID_MOVE.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_INVALID_MOVE.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_INVALID_MOVE.example = vitte check path/to/file.vit

HIR_E_USE_AFTER_MOVE.summary = use after movimento.
HIR_E_USE_AFTER_MOVE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_USE_AFTER_MOVE.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_USE_AFTER_MOVE.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_USE_AFTER_MOVE.example = vitte check path/to/file.vit

HIR_E_USE_AFTER_DROP.summary = use after descarte.
HIR_E_USE_AFTER_DROP.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_USE_AFTER_DROP.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_USE_AFTER_DROP.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_USE_AFTER_DROP.example = vitte check path/to/file.vit

HIR_E_USE_BEFORE_INIT.summary = use before init.
HIR_E_USE_BEFORE_INIT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_USE_BEFORE_INIT.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_USE_BEFORE_INIT.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_USE_BEFORE_INIT.example = vitte check path/to/file.vit

HIR_E_DOUBLE_DROP.summary = double descarte.
HIR_E_DOUBLE_DROP.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_DOUBLE_DROP.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_DOUBLE_DROP.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_DOUBLE_DROP.example = vitte check path/to/file.vit

HIR_E_BORROW_CONFLICT.summary = emprestimo conflito.
HIR_E_BORROW_CONFLICT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_BORROW_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_BORROW_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_BORROW_CONFLICT.example = vitte check path/to/file.vit

HIR_E_MUTABILITY_CONFLICT.summary = mutability conflito.
HIR_E_MUTABILITY_CONFLICT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_MUTABILITY_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_MUTABILITY_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_MUTABILITY_CONFLICT.example = vitte check path/to/file.vit

HIR_E_LIFETIME_TOO_SHORT.summary = tempo de vida demais short.
HIR_E_LIFETIME_TOO_SHORT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_LIFETIME_TOO_SHORT.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_LIFETIME_TOO_SHORT.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_LIFETIME_TOO_SHORT.example = vitte check path/to/file.vit

HIR_E_DANGLING_REFERENCE.summary = dangling referencia.
HIR_E_DANGLING_REFERENCE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_DANGLING_REFERENCE.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_DANGLING_REFERENCE.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_DANGLING_REFERENCE.example = vitte check path/to/file.vit

HIR_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
HIR_E_NON_EXHAUSTIVE_MATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_NON_EXHAUSTIVE_MATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_NON_EXHAUSTIVE_MATCH.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_NON_EXHAUSTIVE_MATCH.example = vitte check path/to/file.vit

HIR_E_UNREACHABLE_PATTERN.summary = inalcancavel padrao.
HIR_E_UNREACHABLE_PATTERN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_UNREACHABLE_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_UNREACHABLE_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_UNREACHABLE_PATTERN.example = vitte check path/to/file.vit

HIR_E_CONST_REQUIRED.summary = const required.
HIR_E_CONST_REQUIRED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_CONST_REQUIRED.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_CONST_REQUIRED.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_CONST_REQUIRED.example = vitte check path/to/file.vit

HIR_E_CONST_OVERFLOW.summary = const estouro.
HIR_E_CONST_OVERFLOW.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_CONST_OVERFLOW.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_CONST_OVERFLOW.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_CONST_OVERFLOW.example = vitte check path/to/file.vit

HIR_E_CONST_DIVISION_BY_ZERO.summary = const divisao by zero.
HIR_E_CONST_DIVISION_BY_ZERO.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_CONST_DIVISION_BY_ZERO.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_CONST_DIVISION_BY_ZERO.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_CONST_DIVISION_BY_ZERO.example = vitte check path/to/file.vit

HIR_E_CONST_CYCLE.summary = const ciclo.
HIR_E_CONST_CYCLE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_CONST_CYCLE.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_CONST_CYCLE.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_CONST_CYCLE.example = vitte check path/to/file.vit

HIR_E_MACRO_NOT_FOUND.summary = macro ausente.
HIR_E_MACRO_NOT_FOUND.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_MACRO_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_MACRO_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_MACRO_NOT_FOUND.example = vitte check path/to/file.vit

HIR_E_MACRO_RECURSION.summary = macro recursion.
HIR_E_MACRO_RECURSION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_MACRO_RECURSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_MACRO_RECURSION.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_MACRO_RECURSION.example = vitte check path/to/file.vit

HIR_E_MACRO_EXPANSION_FAILED.summary = macro expansion falhou.
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

HIR_E_GENERIC_ARGUMENT_MISSING.summary = generico argumento ausente.
HIR_E_GENERIC_ARGUMENT_MISSING.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_GENERIC_ARGUMENT_MISSING.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_GENERIC_ARGUMENT_MISSING.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_GENERIC_ARGUMENT_MISSING.example = vitte check path/to/file.vit

HIR_E_GENERIC_BOUND_FAILED.summary = generico bound falhou.
HIR_E_GENERIC_BOUND_FAILED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_GENERIC_BOUND_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_GENERIC_BOUND_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_GENERIC_BOUND_FAILED.example = vitte check path/to/file.vit

HIR_E_UNSUPPORTED_TARGET.summary = destino nao suportado.
HIR_E_UNSUPPORTED_TARGET.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_UNSUPPORTED_TARGET.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_UNSUPPORTED_TARGET.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_UNSUPPORTED_TARGET.example = vitte check path/to/file.vit

HIR_E_ABI_MISMATCH.summary = abi incompatibilidade.
HIR_E_ABI_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_ABI_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_ABI_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_ABI_MISMATCH.example = vitte check path/to/file.vit

HIR_E_LINK_FAILED.summary = link falhou.
HIR_E_LINK_FAILED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_LINK_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_LINK_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_LINK_FAILED.example = vitte check path/to/file.vit

HIR_E_RUNTIME_PANIC.summary = runtime panico.
HIR_E_RUNTIME_PANIC.cause = The compiler pipeline phase found code that violates this diagnostic rule.
HIR_E_RUNTIME_PANIC.step1 = Fix the first span reported for this diagnostic, then run the command again.
HIR_E_RUNTIME_PANIC.fix = repair the highlighted compiler contract before checking later diagnostics
HIR_E_RUNTIME_PANIC.example = vitte check path/to/file.vit

MIR_E_EXPECTED_IDENTIFIER.summary = identificador esperado.
MIR_E_EXPECTED_IDENTIFIER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_EXPECTED_IDENTIFIER.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_EXPECTED_IDENTIFIER.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_EXPECTED_IDENTIFIER.example = vitte check path/to/file.vit

MIR_E_EXPECTED_EXPRESSION.summary = expressao esperado.
MIR_E_EXPECTED_EXPRESSION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_EXPECTED_EXPRESSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_EXPECTED_EXPRESSION.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_EXPECTED_EXPRESSION.example = vitte check path/to/file.vit

MIR_E_EXPECTED_TYPE.summary = tipo esperado.
MIR_E_EXPECTED_TYPE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_EXPECTED_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_EXPECTED_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_EXPECTED_TYPE.example = vitte check path/to/file.vit

MIR_E_EXPECTED_PATTERN.summary = padrao esperado.
MIR_E_EXPECTED_PATTERN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_EXPECTED_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_EXPECTED_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_EXPECTED_PATTERN.example = vitte check path/to/file.vit

MIR_E_EXPECTED_BLOCK.summary = bloco esperado.
MIR_E_EXPECTED_BLOCK.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_EXPECTED_BLOCK.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_EXPECTED_BLOCK.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_EXPECTED_BLOCK.example = vitte check path/to/file.vit

MIR_E_EXPECTED_DELIMITER.summary = delimitador esperado.
MIR_E_EXPECTED_DELIMITER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_EXPECTED_DELIMITER.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_EXPECTED_DELIMITER.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_EXPECTED_DELIMITER.example = vitte check path/to/file.vit

MIR_E_UNEXPECTED_TOKEN.summary = token inesperado.
MIR_E_UNEXPECTED_TOKEN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_UNEXPECTED_TOKEN.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_UNEXPECTED_TOKEN.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_UNEXPECTED_TOKEN.example = vitte check path/to/file.vit

MIR_E_UNBALANCED_DELIMITER.summary = unbalanced delimitador.
MIR_E_UNBALANCED_DELIMITER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_UNBALANCED_DELIMITER.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_UNBALANCED_DELIMITER.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_UNBALANCED_DELIMITER.example = vitte check path/to/file.vit

MIR_E_INVALID_ATTRIBUTE.summary = attribute invalido.
MIR_E_INVALID_ATTRIBUTE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_ATTRIBUTE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_ATTRIBUTE.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_ATTRIBUTE.example = vitte check path/to/file.vit

MIR_E_INVALID_DECLARATION.summary = declaracao invalido.
MIR_E_INVALID_DECLARATION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_DECLARATION.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_DECLARATION.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_DECLARATION.example = vitte check path/to/file.vit

MIR_E_INVALID_STATEMENT.summary = instrucao invalido.
MIR_E_INVALID_STATEMENT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_STATEMENT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_STATEMENT.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_STATEMENT.example = vitte check path/to/file.vit

MIR_E_INVALID_EXPRESSION.summary = expressao invalido.
MIR_E_INVALID_EXPRESSION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_EXPRESSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_EXPRESSION.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_EXPRESSION.example = vitte check path/to/file.vit

MIR_E_INVALID_PATTERN.summary = padrao invalido.
MIR_E_INVALID_PATTERN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_PATTERN.example = vitte check path/to/file.vit

MIR_E_INVALID_LITERAL.summary = literal invalido.
MIR_E_INVALID_LITERAL.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_LITERAL.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_LITERAL.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_LITERAL.example = vitte check path/to/file.vit

MIR_E_INVALID_OPERATOR.summary = operador invalido.
MIR_E_INVALID_OPERATOR.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_OPERATOR.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_OPERATOR.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_OPERATOR.example = vitte check path/to/file.vit

MIR_E_INVALID_MODIFIER.summary = modifier invalido.
MIR_E_INVALID_MODIFIER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_MODIFIER.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_MODIFIER.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_MODIFIER.example = vitte check path/to/file.vit

MIR_E_MISSING_BODY.summary = body ausente.
MIR_E_MISSING_BODY.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_MISSING_BODY.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_MISSING_BODY.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_MISSING_BODY.example = vitte check path/to/file.vit

MIR_E_MISSING_RETURN.summary = retorno ausente.
MIR_E_MISSING_RETURN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_MISSING_RETURN.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_MISSING_RETURN.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_MISSING_RETURN.example = vitte check path/to/file.vit

MIR_E_DUPLICATE_NAME.summary = nome duplicado.
MIR_E_DUPLICATE_NAME.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_DUPLICATE_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_DUPLICATE_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_DUPLICATE_NAME.example = vitte check path/to/file.vit

MIR_E_UNKNOWN_NAME.summary = nome desconhecido.
MIR_E_UNKNOWN_NAME.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_UNKNOWN_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_UNKNOWN_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_UNKNOWN_NAME.example = vitte check path/to/file.vit

MIR_E_UNKNOWN_TYPE.summary = tipo desconhecido.
MIR_E_UNKNOWN_TYPE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_UNKNOWN_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_UNKNOWN_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_UNKNOWN_TYPE.example = vitte check path/to/file.vit

MIR_E_UNKNOWN_MODULE.summary = modulo desconhecido.
MIR_E_UNKNOWN_MODULE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_UNKNOWN_MODULE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_UNKNOWN_MODULE.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_UNKNOWN_MODULE.example = vitte check path/to/file.vit

MIR_E_UNKNOWN_MEMBER.summary = membro desconhecido.
MIR_E_UNKNOWN_MEMBER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_UNKNOWN_MEMBER.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_UNKNOWN_MEMBER.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_UNKNOWN_MEMBER.example = vitte check path/to/file.vit

MIR_E_AMBIGUOUS_NAME.summary = ambiguous nome.
MIR_E_AMBIGUOUS_NAME.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_AMBIGUOUS_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_AMBIGUOUS_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_AMBIGUOUS_NAME.example = vitte check path/to/file.vit

MIR_E_PRIVATE_SYMBOL.summary = private simbolo.
MIR_E_PRIVATE_SYMBOL.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_PRIVATE_SYMBOL.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_PRIVATE_SYMBOL.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_PRIVATE_SYMBOL.example = vitte check path/to/file.vit

MIR_E_IMPORT_NOT_FOUND.summary = importacao ausente.
MIR_E_IMPORT_NOT_FOUND.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_IMPORT_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_IMPORT_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_IMPORT_NOT_FOUND.example = vitte check path/to/file.vit

MIR_E_IMPORT_CYCLE.summary = importacao ciclo.
MIR_E_IMPORT_CYCLE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_IMPORT_CYCLE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_IMPORT_CYCLE.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_IMPORT_CYCLE.example = vitte check path/to/file.vit

MIR_E_EXPORT_CONFLICT.summary = export conflito.
MIR_E_EXPORT_CONFLICT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_EXPORT_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_EXPORT_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_EXPORT_CONFLICT.example = vitte check path/to/file.vit

MIR_E_ARITY_MISMATCH.summary = arity incompatibilidade.
MIR_E_ARITY_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_ARITY_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_ARITY_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_ARITY_MISMATCH.example = vitte check path/to/file.vit

MIR_E_ARGUMENT_MISMATCH.summary = argumento incompatibilidade.
MIR_E_ARGUMENT_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_ARGUMENT_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_ARGUMENT_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_ARGUMENT_MISMATCH.example = vitte check path/to/file.vit

MIR_E_ASSIGNMENT_MISMATCH.summary = atribuicao incompatibilidade.
MIR_E_ASSIGNMENT_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_ASSIGNMENT_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_ASSIGNMENT_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_ASSIGNMENT_MISMATCH.example = vitte check path/to/file.vit

MIR_E_BRANCH_MISMATCH.summary = branch incompatibilidade.
MIR_E_BRANCH_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_BRANCH_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_BRANCH_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_BRANCH_MISMATCH.example = vitte check path/to/file.vit

MIR_E_INVALID_CALL.summary = chamada invalido.
MIR_E_INVALID_CALL.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_CALL.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_CALL.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_CALL.example = vitte check path/to/file.vit

MIR_E_INVALID_CAST.summary = conversao invalido.
MIR_E_INVALID_CAST.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_CAST.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_CAST.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_CAST.example = vitte check path/to/file.vit

MIR_E_INVALID_INDEX.summary = indice invalido.
MIR_E_INVALID_INDEX.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_INDEX.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_INDEX.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_INDEX.example = vitte check path/to/file.vit

MIR_E_INVALID_DEREF.summary = deref invalido.
MIR_E_INVALID_DEREF.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_DEREF.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_DEREF.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_DEREF.example = vitte check path/to/file.vit

MIR_E_INVALID_BORROW.summary = emprestimo invalido.
MIR_E_INVALID_BORROW.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_BORROW.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_BORROW.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_BORROW.example = vitte check path/to/file.vit

MIR_E_INVALID_MOVE.summary = movimento invalido.
MIR_E_INVALID_MOVE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_INVALID_MOVE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_INVALID_MOVE.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_INVALID_MOVE.example = vitte check path/to/file.vit

MIR_E_USE_AFTER_MOVE.summary = use after movimento.
MIR_E_USE_AFTER_MOVE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_USE_AFTER_MOVE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_USE_AFTER_MOVE.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_USE_AFTER_MOVE.example = vitte check path/to/file.vit

MIR_E_USE_AFTER_DROP.summary = use after descarte.
MIR_E_USE_AFTER_DROP.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_USE_AFTER_DROP.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_USE_AFTER_DROP.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_USE_AFTER_DROP.example = vitte check path/to/file.vit

MIR_E_USE_BEFORE_INIT.summary = use before init.
MIR_E_USE_BEFORE_INIT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_USE_BEFORE_INIT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_USE_BEFORE_INIT.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_USE_BEFORE_INIT.example = vitte check path/to/file.vit

MIR_E_DOUBLE_DROP.summary = double descarte.
MIR_E_DOUBLE_DROP.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_DOUBLE_DROP.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_DOUBLE_DROP.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_DOUBLE_DROP.example = vitte check path/to/file.vit

MIR_E_BORROW_CONFLICT.summary = emprestimo conflito.
MIR_E_BORROW_CONFLICT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_BORROW_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_BORROW_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_BORROW_CONFLICT.example = vitte check path/to/file.vit

MIR_E_MUTABILITY_CONFLICT.summary = mutability conflito.
MIR_E_MUTABILITY_CONFLICT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_MUTABILITY_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_MUTABILITY_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_MUTABILITY_CONFLICT.example = vitte check path/to/file.vit

MIR_E_LIFETIME_TOO_SHORT.summary = tempo de vida demais short.
MIR_E_LIFETIME_TOO_SHORT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_LIFETIME_TOO_SHORT.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_LIFETIME_TOO_SHORT.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_LIFETIME_TOO_SHORT.example = vitte check path/to/file.vit

MIR_E_DANGLING_REFERENCE.summary = dangling referencia.
MIR_E_DANGLING_REFERENCE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_DANGLING_REFERENCE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_DANGLING_REFERENCE.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_DANGLING_REFERENCE.example = vitte check path/to/file.vit

MIR_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
MIR_E_NON_EXHAUSTIVE_MATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_NON_EXHAUSTIVE_MATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_NON_EXHAUSTIVE_MATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_NON_EXHAUSTIVE_MATCH.example = vitte check path/to/file.vit

MIR_E_UNREACHABLE_PATTERN.summary = inalcancavel padrao.
MIR_E_UNREACHABLE_PATTERN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_UNREACHABLE_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_UNREACHABLE_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_UNREACHABLE_PATTERN.example = vitte check path/to/file.vit

MIR_E_CONST_REQUIRED.summary = const required.
MIR_E_CONST_REQUIRED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_CONST_REQUIRED.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_CONST_REQUIRED.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_CONST_REQUIRED.example = vitte check path/to/file.vit

MIR_E_CONST_OVERFLOW.summary = const estouro.
MIR_E_CONST_OVERFLOW.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_CONST_OVERFLOW.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_CONST_OVERFLOW.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_CONST_OVERFLOW.example = vitte check path/to/file.vit

MIR_E_CONST_DIVISION_BY_ZERO.summary = const divisao by zero.
MIR_E_CONST_DIVISION_BY_ZERO.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_CONST_DIVISION_BY_ZERO.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_CONST_DIVISION_BY_ZERO.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_CONST_DIVISION_BY_ZERO.example = vitte check path/to/file.vit

MIR_E_CONST_CYCLE.summary = const ciclo.
MIR_E_CONST_CYCLE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_CONST_CYCLE.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_CONST_CYCLE.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_CONST_CYCLE.example = vitte check path/to/file.vit

MIR_E_MACRO_NOT_FOUND.summary = macro ausente.
MIR_E_MACRO_NOT_FOUND.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_MACRO_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_MACRO_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_MACRO_NOT_FOUND.example = vitte check path/to/file.vit

MIR_E_MACRO_RECURSION.summary = macro recursion.
MIR_E_MACRO_RECURSION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_MACRO_RECURSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_MACRO_RECURSION.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_MACRO_RECURSION.example = vitte check path/to/file.vit

MIR_E_MACRO_EXPANSION_FAILED.summary = macro expansion falhou.
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

MIR_E_GENERIC_ARGUMENT_MISSING.summary = generico argumento ausente.
MIR_E_GENERIC_ARGUMENT_MISSING.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_GENERIC_ARGUMENT_MISSING.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_GENERIC_ARGUMENT_MISSING.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_GENERIC_ARGUMENT_MISSING.example = vitte check path/to/file.vit

MIR_E_GENERIC_BOUND_FAILED.summary = generico bound falhou.
MIR_E_GENERIC_BOUND_FAILED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_GENERIC_BOUND_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_GENERIC_BOUND_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_GENERIC_BOUND_FAILED.example = vitte check path/to/file.vit

MIR_E_UNSUPPORTED_TARGET.summary = destino nao suportado.
MIR_E_UNSUPPORTED_TARGET.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_UNSUPPORTED_TARGET.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_UNSUPPORTED_TARGET.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_UNSUPPORTED_TARGET.example = vitte check path/to/file.vit

MIR_E_ABI_MISMATCH.summary = abi incompatibilidade.
MIR_E_ABI_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_ABI_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_ABI_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_ABI_MISMATCH.example = vitte check path/to/file.vit

MIR_E_LINK_FAILED.summary = link falhou.
MIR_E_LINK_FAILED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_LINK_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_LINK_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_LINK_FAILED.example = vitte check path/to/file.vit

MIR_E_RUNTIME_PANIC.summary = runtime panico.
MIR_E_RUNTIME_PANIC.cause = The compiler pipeline phase found code that violates this diagnostic rule.
MIR_E_RUNTIME_PANIC.step1 = Fix the first span reported for this diagnostic, then run the command again.
MIR_E_RUNTIME_PANIC.fix = repair the highlighted compiler contract before checking later diagnostics
MIR_E_RUNTIME_PANIC.example = vitte check path/to/file.vit

IR_E_EXPECTED_IDENTIFIER.summary = identificador esperado.
IR_E_EXPECTED_IDENTIFIER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_EXPECTED_IDENTIFIER.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_EXPECTED_IDENTIFIER.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_EXPECTED_IDENTIFIER.example = vitte check path/to/file.vit

IR_E_EXPECTED_EXPRESSION.summary = expressao esperado.
IR_E_EXPECTED_EXPRESSION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_EXPECTED_EXPRESSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_EXPECTED_EXPRESSION.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_EXPECTED_EXPRESSION.example = vitte check path/to/file.vit

IR_E_EXPECTED_TYPE.summary = tipo esperado.
IR_E_EXPECTED_TYPE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_EXPECTED_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_EXPECTED_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_EXPECTED_TYPE.example = vitte check path/to/file.vit

IR_E_EXPECTED_PATTERN.summary = padrao esperado.
IR_E_EXPECTED_PATTERN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_EXPECTED_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_EXPECTED_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_EXPECTED_PATTERN.example = vitte check path/to/file.vit

IR_E_EXPECTED_BLOCK.summary = bloco esperado.
IR_E_EXPECTED_BLOCK.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_EXPECTED_BLOCK.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_EXPECTED_BLOCK.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_EXPECTED_BLOCK.example = vitte check path/to/file.vit

IR_E_EXPECTED_DELIMITER.summary = delimitador esperado.
IR_E_EXPECTED_DELIMITER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_EXPECTED_DELIMITER.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_EXPECTED_DELIMITER.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_EXPECTED_DELIMITER.example = vitte check path/to/file.vit

IR_E_UNEXPECTED_TOKEN.summary = token inesperado.
IR_E_UNEXPECTED_TOKEN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_UNEXPECTED_TOKEN.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_UNEXPECTED_TOKEN.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_UNEXPECTED_TOKEN.example = vitte check path/to/file.vit

IR_E_UNBALANCED_DELIMITER.summary = unbalanced delimitador.
IR_E_UNBALANCED_DELIMITER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_UNBALANCED_DELIMITER.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_UNBALANCED_DELIMITER.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_UNBALANCED_DELIMITER.example = vitte check path/to/file.vit

IR_E_INVALID_ATTRIBUTE.summary = attribute invalido.
IR_E_INVALID_ATTRIBUTE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_ATTRIBUTE.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_ATTRIBUTE.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_ATTRIBUTE.example = vitte check path/to/file.vit

IR_E_INVALID_DECLARATION.summary = declaracao invalido.
IR_E_INVALID_DECLARATION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_DECLARATION.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_DECLARATION.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_DECLARATION.example = vitte check path/to/file.vit

IR_E_INVALID_STATEMENT.summary = instrucao invalido.
IR_E_INVALID_STATEMENT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_STATEMENT.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_STATEMENT.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_STATEMENT.example = vitte check path/to/file.vit

IR_E_INVALID_EXPRESSION.summary = expressao invalido.
IR_E_INVALID_EXPRESSION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_EXPRESSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_EXPRESSION.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_EXPRESSION.example = vitte check path/to/file.vit

IR_E_INVALID_PATTERN.summary = padrao invalido.
IR_E_INVALID_PATTERN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_PATTERN.example = vitte check path/to/file.vit

IR_E_INVALID_LITERAL.summary = literal invalido.
IR_E_INVALID_LITERAL.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_LITERAL.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_LITERAL.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_LITERAL.example = vitte check path/to/file.vit

IR_E_INVALID_OPERATOR.summary = operador invalido.
IR_E_INVALID_OPERATOR.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_OPERATOR.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_OPERATOR.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_OPERATOR.example = vitte check path/to/file.vit

IR_E_INVALID_MODIFIER.summary = modifier invalido.
IR_E_INVALID_MODIFIER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_MODIFIER.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_MODIFIER.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_MODIFIER.example = vitte check path/to/file.vit

IR_E_MISSING_BODY.summary = body ausente.
IR_E_MISSING_BODY.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_MISSING_BODY.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_MISSING_BODY.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_MISSING_BODY.example = vitte check path/to/file.vit

IR_E_MISSING_RETURN.summary = retorno ausente.
IR_E_MISSING_RETURN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_MISSING_RETURN.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_MISSING_RETURN.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_MISSING_RETURN.example = vitte check path/to/file.vit

IR_E_DUPLICATE_NAME.summary = nome duplicado.
IR_E_DUPLICATE_NAME.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_DUPLICATE_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_DUPLICATE_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_DUPLICATE_NAME.example = vitte check path/to/file.vit

IR_E_UNKNOWN_NAME.summary = nome desconhecido.
IR_E_UNKNOWN_NAME.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_UNKNOWN_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_UNKNOWN_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_UNKNOWN_NAME.example = vitte check path/to/file.vit

IR_E_UNKNOWN_TYPE.summary = tipo desconhecido.
IR_E_UNKNOWN_TYPE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_UNKNOWN_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_UNKNOWN_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_UNKNOWN_TYPE.example = vitte check path/to/file.vit

IR_E_UNKNOWN_MODULE.summary = modulo desconhecido.
IR_E_UNKNOWN_MODULE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_UNKNOWN_MODULE.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_UNKNOWN_MODULE.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_UNKNOWN_MODULE.example = vitte check path/to/file.vit

IR_E_UNKNOWN_MEMBER.summary = membro desconhecido.
IR_E_UNKNOWN_MEMBER.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_UNKNOWN_MEMBER.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_UNKNOWN_MEMBER.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_UNKNOWN_MEMBER.example = vitte check path/to/file.vit

IR_E_AMBIGUOUS_NAME.summary = ambiguous nome.
IR_E_AMBIGUOUS_NAME.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_AMBIGUOUS_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_AMBIGUOUS_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_AMBIGUOUS_NAME.example = vitte check path/to/file.vit

IR_E_PRIVATE_SYMBOL.summary = private simbolo.
IR_E_PRIVATE_SYMBOL.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_PRIVATE_SYMBOL.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_PRIVATE_SYMBOL.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_PRIVATE_SYMBOL.example = vitte check path/to/file.vit

IR_E_IMPORT_NOT_FOUND.summary = importacao ausente.
IR_E_IMPORT_NOT_FOUND.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_IMPORT_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_IMPORT_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_IMPORT_NOT_FOUND.example = vitte check path/to/file.vit

IR_E_IMPORT_CYCLE.summary = importacao ciclo.
IR_E_IMPORT_CYCLE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_IMPORT_CYCLE.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_IMPORT_CYCLE.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_IMPORT_CYCLE.example = vitte check path/to/file.vit

IR_E_EXPORT_CONFLICT.summary = export conflito.
IR_E_EXPORT_CONFLICT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_EXPORT_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_EXPORT_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_EXPORT_CONFLICT.example = vitte check path/to/file.vit

IR_E_ARITY_MISMATCH.summary = arity incompatibilidade.
IR_E_ARITY_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_ARITY_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_ARITY_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_ARITY_MISMATCH.example = vitte check path/to/file.vit

IR_E_ARGUMENT_MISMATCH.summary = argumento incompatibilidade.
IR_E_ARGUMENT_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_ARGUMENT_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_ARGUMENT_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_ARGUMENT_MISMATCH.example = vitte check path/to/file.vit

IR_E_ASSIGNMENT_MISMATCH.summary = atribuicao incompatibilidade.
IR_E_ASSIGNMENT_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_ASSIGNMENT_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_ASSIGNMENT_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_ASSIGNMENT_MISMATCH.example = vitte check path/to/file.vit

IR_E_BRANCH_MISMATCH.summary = branch incompatibilidade.
IR_E_BRANCH_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_BRANCH_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_BRANCH_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_BRANCH_MISMATCH.example = vitte check path/to/file.vit

IR_E_INVALID_CALL.summary = chamada invalido.
IR_E_INVALID_CALL.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_CALL.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_CALL.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_CALL.example = vitte check path/to/file.vit

IR_E_INVALID_CAST.summary = conversao invalido.
IR_E_INVALID_CAST.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_CAST.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_CAST.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_CAST.example = vitte check path/to/file.vit

IR_E_INVALID_INDEX.summary = indice invalido.
IR_E_INVALID_INDEX.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_INDEX.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_INDEX.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_INDEX.example = vitte check path/to/file.vit

IR_E_INVALID_DEREF.summary = deref invalido.
IR_E_INVALID_DEREF.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_DEREF.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_DEREF.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_DEREF.example = vitte check path/to/file.vit

IR_E_INVALID_BORROW.summary = emprestimo invalido.
IR_E_INVALID_BORROW.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_BORROW.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_BORROW.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_BORROW.example = vitte check path/to/file.vit

IR_E_INVALID_MOVE.summary = movimento invalido.
IR_E_INVALID_MOVE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_INVALID_MOVE.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_INVALID_MOVE.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_INVALID_MOVE.example = vitte check path/to/file.vit

IR_E_USE_AFTER_MOVE.summary = use after movimento.
IR_E_USE_AFTER_MOVE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_USE_AFTER_MOVE.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_USE_AFTER_MOVE.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_USE_AFTER_MOVE.example = vitte check path/to/file.vit

IR_E_USE_AFTER_DROP.summary = use after descarte.
IR_E_USE_AFTER_DROP.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_USE_AFTER_DROP.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_USE_AFTER_DROP.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_USE_AFTER_DROP.example = vitte check path/to/file.vit

IR_E_USE_BEFORE_INIT.summary = use before init.
IR_E_USE_BEFORE_INIT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_USE_BEFORE_INIT.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_USE_BEFORE_INIT.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_USE_BEFORE_INIT.example = vitte check path/to/file.vit

IR_E_DOUBLE_DROP.summary = double descarte.
IR_E_DOUBLE_DROP.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_DOUBLE_DROP.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_DOUBLE_DROP.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_DOUBLE_DROP.example = vitte check path/to/file.vit

IR_E_BORROW_CONFLICT.summary = emprestimo conflito.
IR_E_BORROW_CONFLICT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_BORROW_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_BORROW_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_BORROW_CONFLICT.example = vitte check path/to/file.vit

IR_E_MUTABILITY_CONFLICT.summary = mutability conflito.
IR_E_MUTABILITY_CONFLICT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_MUTABILITY_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_MUTABILITY_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_MUTABILITY_CONFLICT.example = vitte check path/to/file.vit

IR_E_LIFETIME_TOO_SHORT.summary = tempo de vida demais short.
IR_E_LIFETIME_TOO_SHORT.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_LIFETIME_TOO_SHORT.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_LIFETIME_TOO_SHORT.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_LIFETIME_TOO_SHORT.example = vitte check path/to/file.vit

IR_E_DANGLING_REFERENCE.summary = dangling referencia.
IR_E_DANGLING_REFERENCE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_DANGLING_REFERENCE.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_DANGLING_REFERENCE.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_DANGLING_REFERENCE.example = vitte check path/to/file.vit

IR_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
IR_E_NON_EXHAUSTIVE_MATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_NON_EXHAUSTIVE_MATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_NON_EXHAUSTIVE_MATCH.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_NON_EXHAUSTIVE_MATCH.example = vitte check path/to/file.vit

IR_E_UNREACHABLE_PATTERN.summary = inalcancavel padrao.
IR_E_UNREACHABLE_PATTERN.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_UNREACHABLE_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_UNREACHABLE_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_UNREACHABLE_PATTERN.example = vitte check path/to/file.vit

IR_E_CONST_REQUIRED.summary = const required.
IR_E_CONST_REQUIRED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_CONST_REQUIRED.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_CONST_REQUIRED.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_CONST_REQUIRED.example = vitte check path/to/file.vit

IR_E_CONST_OVERFLOW.summary = const estouro.
IR_E_CONST_OVERFLOW.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_CONST_OVERFLOW.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_CONST_OVERFLOW.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_CONST_OVERFLOW.example = vitte check path/to/file.vit

IR_E_CONST_DIVISION_BY_ZERO.summary = const divisao by zero.
IR_E_CONST_DIVISION_BY_ZERO.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_CONST_DIVISION_BY_ZERO.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_CONST_DIVISION_BY_ZERO.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_CONST_DIVISION_BY_ZERO.example = vitte check path/to/file.vit

IR_E_CONST_CYCLE.summary = const ciclo.
IR_E_CONST_CYCLE.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_CONST_CYCLE.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_CONST_CYCLE.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_CONST_CYCLE.example = vitte check path/to/file.vit

IR_E_MACRO_NOT_FOUND.summary = macro ausente.
IR_E_MACRO_NOT_FOUND.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_MACRO_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_MACRO_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_MACRO_NOT_FOUND.example = vitte check path/to/file.vit

IR_E_MACRO_RECURSION.summary = macro recursion.
IR_E_MACRO_RECURSION.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_MACRO_RECURSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_MACRO_RECURSION.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_MACRO_RECURSION.example = vitte check path/to/file.vit

IR_E_MACRO_EXPANSION_FAILED.summary = macro expansion falhou.
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

IR_E_GENERIC_ARGUMENT_MISSING.summary = generico argumento ausente.
IR_E_GENERIC_ARGUMENT_MISSING.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_GENERIC_ARGUMENT_MISSING.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_GENERIC_ARGUMENT_MISSING.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_GENERIC_ARGUMENT_MISSING.example = vitte check path/to/file.vit

IR_E_GENERIC_BOUND_FAILED.summary = generico bound falhou.
IR_E_GENERIC_BOUND_FAILED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_GENERIC_BOUND_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_GENERIC_BOUND_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_GENERIC_BOUND_FAILED.example = vitte check path/to/file.vit

IR_E_UNSUPPORTED_TARGET.summary = destino nao suportado.
IR_E_UNSUPPORTED_TARGET.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_UNSUPPORTED_TARGET.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_UNSUPPORTED_TARGET.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_UNSUPPORTED_TARGET.example = vitte check path/to/file.vit

IR_E_ABI_MISMATCH.summary = abi incompatibilidade.
IR_E_ABI_MISMATCH.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_ABI_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_ABI_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_ABI_MISMATCH.example = vitte check path/to/file.vit

IR_E_LINK_FAILED.summary = link falhou.
IR_E_LINK_FAILED.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_LINK_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_LINK_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_LINK_FAILED.example = vitte check path/to/file.vit

IR_E_RUNTIME_PANIC.summary = runtime panico.
IR_E_RUNTIME_PANIC.cause = The compiler pipeline phase found code that violates this diagnostic rule.
IR_E_RUNTIME_PANIC.step1 = Fix the first span reported for this diagnostic, then run the command again.
IR_E_RUNTIME_PANIC.fix = repair the highlighted compiler contract before checking later diagnostics
IR_E_RUNTIME_PANIC.example = vitte check path/to/file.vit

BACKEND_E_EXPECTED_IDENTIFIER.summary = identificador esperado.
BACKEND_E_EXPECTED_IDENTIFIER.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_EXPECTED_IDENTIFIER.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_EXPECTED_IDENTIFIER.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_EXPECTED_IDENTIFIER.example = vitte build app.vit -o app

BACKEND_E_EXPECTED_EXPRESSION.summary = expressao esperado.
BACKEND_E_EXPECTED_EXPRESSION.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_EXPECTED_EXPRESSION.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_EXPECTED_EXPRESSION.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_EXPECTED_EXPRESSION.example = vitte build app.vit -o app

BACKEND_E_EXPECTED_TYPE.summary = tipo esperado.
BACKEND_E_EXPECTED_TYPE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_EXPECTED_TYPE.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_EXPECTED_TYPE.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_EXPECTED_TYPE.example = vitte build app.vit -o app

BACKEND_E_EXPECTED_PATTERN.summary = padrao esperado.
BACKEND_E_EXPECTED_PATTERN.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_EXPECTED_PATTERN.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_EXPECTED_PATTERN.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_EXPECTED_PATTERN.example = vitte build app.vit -o app

BACKEND_E_EXPECTED_BLOCK.summary = bloco esperado.
BACKEND_E_EXPECTED_BLOCK.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_EXPECTED_BLOCK.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_EXPECTED_BLOCK.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_EXPECTED_BLOCK.example = vitte build app.vit -o app

BACKEND_E_EXPECTED_DELIMITER.summary = delimitador esperado.
BACKEND_E_EXPECTED_DELIMITER.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_EXPECTED_DELIMITER.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_EXPECTED_DELIMITER.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_EXPECTED_DELIMITER.example = vitte build app.vit -o app

BACKEND_E_UNEXPECTED_TOKEN.summary = token inesperado.
BACKEND_E_UNEXPECTED_TOKEN.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_UNEXPECTED_TOKEN.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_UNEXPECTED_TOKEN.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_UNEXPECTED_TOKEN.example = vitte build app.vit -o app

BACKEND_E_UNBALANCED_DELIMITER.summary = unbalanced delimitador.
BACKEND_E_UNBALANCED_DELIMITER.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_UNBALANCED_DELIMITER.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_UNBALANCED_DELIMITER.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_UNBALANCED_DELIMITER.example = vitte build app.vit -o app

BACKEND_E_INVALID_ATTRIBUTE.summary = attribute invalido.
BACKEND_E_INVALID_ATTRIBUTE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_ATTRIBUTE.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_ATTRIBUTE.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_ATTRIBUTE.example = vitte build app.vit -o app

BACKEND_E_INVALID_DECLARATION.summary = declaracao invalido.
BACKEND_E_INVALID_DECLARATION.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_DECLARATION.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_DECLARATION.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_DECLARATION.example = vitte build app.vit -o app

BACKEND_E_INVALID_STATEMENT.summary = instrucao invalido.
BACKEND_E_INVALID_STATEMENT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_STATEMENT.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_STATEMENT.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_STATEMENT.example = vitte build app.vit -o app

BACKEND_E_INVALID_EXPRESSION.summary = expressao invalido.
BACKEND_E_INVALID_EXPRESSION.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_EXPRESSION.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_EXPRESSION.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_EXPRESSION.example = vitte build app.vit -o app

BACKEND_E_INVALID_PATTERN.summary = padrao invalido.
BACKEND_E_INVALID_PATTERN.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_PATTERN.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_PATTERN.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_PATTERN.example = vitte build app.vit -o app

BACKEND_E_INVALID_LITERAL.summary = literal invalido.
BACKEND_E_INVALID_LITERAL.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_LITERAL.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_LITERAL.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_LITERAL.example = vitte build app.vit -o app

BACKEND_E_INVALID_OPERATOR.summary = operador invalido.
BACKEND_E_INVALID_OPERATOR.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_OPERATOR.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_OPERATOR.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_OPERATOR.example = vitte build app.vit -o app

BACKEND_E_INVALID_MODIFIER.summary = modifier invalido.
BACKEND_E_INVALID_MODIFIER.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_MODIFIER.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_MODIFIER.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_MODIFIER.example = vitte build app.vit -o app

BACKEND_E_MISSING_BODY.summary = body ausente.
BACKEND_E_MISSING_BODY.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_MISSING_BODY.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_MISSING_BODY.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_MISSING_BODY.example = vitte build app.vit -o app

BACKEND_E_MISSING_RETURN.summary = retorno ausente.
BACKEND_E_MISSING_RETURN.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_MISSING_RETURN.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_MISSING_RETURN.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_MISSING_RETURN.example = vitte build app.vit -o app

BACKEND_E_DUPLICATE_NAME.summary = nome duplicado.
BACKEND_E_DUPLICATE_NAME.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_DUPLICATE_NAME.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_DUPLICATE_NAME.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_DUPLICATE_NAME.example = vitte build app.vit -o app

BACKEND_E_UNKNOWN_NAME.summary = nome desconhecido.
BACKEND_E_UNKNOWN_NAME.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_UNKNOWN_NAME.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_UNKNOWN_NAME.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_UNKNOWN_NAME.example = vitte build app.vit -o app

BACKEND_E_UNKNOWN_TYPE.summary = tipo desconhecido.
BACKEND_E_UNKNOWN_TYPE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_UNKNOWN_TYPE.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_UNKNOWN_TYPE.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_UNKNOWN_TYPE.example = vitte build app.vit -o app

BACKEND_E_UNKNOWN_MODULE.summary = modulo desconhecido.
BACKEND_E_UNKNOWN_MODULE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_UNKNOWN_MODULE.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_UNKNOWN_MODULE.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_UNKNOWN_MODULE.example = vitte build app.vit -o app

BACKEND_E_UNKNOWN_MEMBER.summary = membro desconhecido.
BACKEND_E_UNKNOWN_MEMBER.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_UNKNOWN_MEMBER.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_UNKNOWN_MEMBER.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_UNKNOWN_MEMBER.example = vitte build app.vit -o app

BACKEND_E_AMBIGUOUS_NAME.summary = ambiguous nome.
BACKEND_E_AMBIGUOUS_NAME.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_AMBIGUOUS_NAME.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_AMBIGUOUS_NAME.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_AMBIGUOUS_NAME.example = vitte build app.vit -o app

BACKEND_E_PRIVATE_SYMBOL.summary = private simbolo.
BACKEND_E_PRIVATE_SYMBOL.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_PRIVATE_SYMBOL.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_PRIVATE_SYMBOL.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_PRIVATE_SYMBOL.example = vitte build app.vit -o app

BACKEND_E_IMPORT_NOT_FOUND.summary = importacao ausente.
BACKEND_E_IMPORT_NOT_FOUND.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_IMPORT_NOT_FOUND.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_IMPORT_NOT_FOUND.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_IMPORT_NOT_FOUND.example = vitte build app.vit -o app

BACKEND_E_IMPORT_CYCLE.summary = importacao ciclo.
BACKEND_E_IMPORT_CYCLE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_IMPORT_CYCLE.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_IMPORT_CYCLE.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_IMPORT_CYCLE.example = vitte build app.vit -o app

BACKEND_E_EXPORT_CONFLICT.summary = export conflito.
BACKEND_E_EXPORT_CONFLICT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_EXPORT_CONFLICT.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_EXPORT_CONFLICT.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_EXPORT_CONFLICT.example = vitte build app.vit -o app

BACKEND_E_ARITY_MISMATCH.summary = arity incompatibilidade.
BACKEND_E_ARITY_MISMATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_ARITY_MISMATCH.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_ARITY_MISMATCH.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_ARITY_MISMATCH.example = vitte build app.vit -o app

BACKEND_E_ARGUMENT_MISMATCH.summary = argumento incompatibilidade.
BACKEND_E_ARGUMENT_MISMATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_ARGUMENT_MISMATCH.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_ARGUMENT_MISMATCH.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_ARGUMENT_MISMATCH.example = vitte build app.vit -o app

BACKEND_E_ASSIGNMENT_MISMATCH.summary = atribuicao incompatibilidade.
BACKEND_E_ASSIGNMENT_MISMATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_ASSIGNMENT_MISMATCH.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_ASSIGNMENT_MISMATCH.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_ASSIGNMENT_MISMATCH.example = vitte build app.vit -o app

BACKEND_E_BRANCH_MISMATCH.summary = branch incompatibilidade.
BACKEND_E_BRANCH_MISMATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_BRANCH_MISMATCH.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_BRANCH_MISMATCH.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_BRANCH_MISMATCH.example = vitte build app.vit -o app

BACKEND_E_INVALID_CALL.summary = chamada invalido.
BACKEND_E_INVALID_CALL.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_CALL.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_CALL.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_CALL.example = vitte build app.vit -o app

BACKEND_E_INVALID_CAST.summary = conversao invalido.
BACKEND_E_INVALID_CAST.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_CAST.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_CAST.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_CAST.example = vitte build app.vit -o app

BACKEND_E_INVALID_INDEX.summary = indice invalido.
BACKEND_E_INVALID_INDEX.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_INDEX.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_INDEX.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_INDEX.example = vitte build app.vit -o app

BACKEND_E_INVALID_DEREF.summary = deref invalido.
BACKEND_E_INVALID_DEREF.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_DEREF.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_DEREF.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_DEREF.example = vitte build app.vit -o app

BACKEND_E_INVALID_BORROW.summary = emprestimo invalido.
BACKEND_E_INVALID_BORROW.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_BORROW.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_BORROW.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_BORROW.example = vitte build app.vit -o app

BACKEND_E_INVALID_MOVE.summary = movimento invalido.
BACKEND_E_INVALID_MOVE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_INVALID_MOVE.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_INVALID_MOVE.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_INVALID_MOVE.example = vitte build app.vit -o app

BACKEND_E_USE_AFTER_MOVE.summary = use after movimento.
BACKEND_E_USE_AFTER_MOVE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_USE_AFTER_MOVE.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_USE_AFTER_MOVE.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_USE_AFTER_MOVE.example = vitte build app.vit -o app

BACKEND_E_USE_AFTER_DROP.summary = use after descarte.
BACKEND_E_USE_AFTER_DROP.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_USE_AFTER_DROP.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_USE_AFTER_DROP.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_USE_AFTER_DROP.example = vitte build app.vit -o app

BACKEND_E_USE_BEFORE_INIT.summary = use before init.
BACKEND_E_USE_BEFORE_INIT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_USE_BEFORE_INIT.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_USE_BEFORE_INIT.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_USE_BEFORE_INIT.example = vitte build app.vit -o app

BACKEND_E_DOUBLE_DROP.summary = double descarte.
BACKEND_E_DOUBLE_DROP.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_DOUBLE_DROP.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_DOUBLE_DROP.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_DOUBLE_DROP.example = vitte build app.vit -o app

BACKEND_E_BORROW_CONFLICT.summary = emprestimo conflito.
BACKEND_E_BORROW_CONFLICT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_BORROW_CONFLICT.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_BORROW_CONFLICT.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_BORROW_CONFLICT.example = vitte build app.vit -o app

BACKEND_E_MUTABILITY_CONFLICT.summary = mutability conflito.
BACKEND_E_MUTABILITY_CONFLICT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_MUTABILITY_CONFLICT.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_MUTABILITY_CONFLICT.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_MUTABILITY_CONFLICT.example = vitte build app.vit -o app

BACKEND_E_LIFETIME_TOO_SHORT.summary = tempo de vida demais short.
BACKEND_E_LIFETIME_TOO_SHORT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_LIFETIME_TOO_SHORT.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_LIFETIME_TOO_SHORT.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_LIFETIME_TOO_SHORT.example = vitte build app.vit -o app

BACKEND_E_DANGLING_REFERENCE.summary = dangling referencia.
BACKEND_E_DANGLING_REFERENCE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_DANGLING_REFERENCE.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_DANGLING_REFERENCE.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_DANGLING_REFERENCE.example = vitte build app.vit -o app

BACKEND_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
BACKEND_E_NON_EXHAUSTIVE_MATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_NON_EXHAUSTIVE_MATCH.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_NON_EXHAUSTIVE_MATCH.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_NON_EXHAUSTIVE_MATCH.example = vitte build app.vit -o app

BACKEND_E_UNREACHABLE_PATTERN.summary = inalcancavel padrao.
BACKEND_E_UNREACHABLE_PATTERN.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_UNREACHABLE_PATTERN.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_UNREACHABLE_PATTERN.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_UNREACHABLE_PATTERN.example = vitte build app.vit -o app

BACKEND_E_CONST_REQUIRED.summary = const required.
BACKEND_E_CONST_REQUIRED.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_CONST_REQUIRED.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_CONST_REQUIRED.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_CONST_REQUIRED.example = vitte build app.vit -o app

BACKEND_E_CONST_OVERFLOW.summary = const estouro.
BACKEND_E_CONST_OVERFLOW.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_CONST_OVERFLOW.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_CONST_OVERFLOW.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_CONST_OVERFLOW.example = vitte build app.vit -o app

BACKEND_E_CONST_DIVISION_BY_ZERO.summary = const divisao by zero.
BACKEND_E_CONST_DIVISION_BY_ZERO.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_CONST_DIVISION_BY_ZERO.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_CONST_DIVISION_BY_ZERO.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_CONST_DIVISION_BY_ZERO.example = vitte build app.vit -o app

BACKEND_E_CONST_CYCLE.summary = const ciclo.
BACKEND_E_CONST_CYCLE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_CONST_CYCLE.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_CONST_CYCLE.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_CONST_CYCLE.example = vitte build app.vit -o app

BACKEND_E_MACRO_NOT_FOUND.summary = macro ausente.
BACKEND_E_MACRO_NOT_FOUND.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_MACRO_NOT_FOUND.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_MACRO_NOT_FOUND.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_MACRO_NOT_FOUND.example = vitte build app.vit -o app

BACKEND_E_MACRO_RECURSION.summary = macro recursion.
BACKEND_E_MACRO_RECURSION.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_MACRO_RECURSION.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_MACRO_RECURSION.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_MACRO_RECURSION.example = vitte build app.vit -o app

BACKEND_E_MACRO_EXPANSION_FAILED.summary = macro expansion falhou.
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

BACKEND_E_GENERIC_ARGUMENT_MISSING.summary = generico argumento ausente.
BACKEND_E_GENERIC_ARGUMENT_MISSING.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_GENERIC_ARGUMENT_MISSING.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_GENERIC_ARGUMENT_MISSING.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_GENERIC_ARGUMENT_MISSING.example = vitte build app.vit -o app

BACKEND_E_GENERIC_BOUND_FAILED.summary = generico bound falhou.
BACKEND_E_GENERIC_BOUND_FAILED.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_GENERIC_BOUND_FAILED.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_GENERIC_BOUND_FAILED.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_GENERIC_BOUND_FAILED.example = vitte build app.vit -o app

BACKEND_E_LINK_FAILED.summary = link falhou.
BACKEND_E_LINK_FAILED.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_LINK_FAILED.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_LINK_FAILED.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_LINK_FAILED.example = vitte build app.vit -o app

BACKEND_E_RUNTIME_PANIC.summary = runtime panico.
BACKEND_E_RUNTIME_PANIC.cause = The selected target, linker, ABI, object file, or native toolchain failed.
BACKEND_E_RUNTIME_PANIC.step1 = Check the target triple and the first backend or linker note.
BACKEND_E_RUNTIME_PANIC.fix = install the missing native tool, change target, or fix undefined symbols
BACKEND_E_RUNTIME_PANIC.example = vitte build app.vit -o app

LINK_E_EXPECTED_IDENTIFIER.summary = identificador esperado.
LINK_E_EXPECTED_IDENTIFIER.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_EXPECTED_IDENTIFIER.step1 = Check the target triple and the first backend or linker note.
LINK_E_EXPECTED_IDENTIFIER.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_EXPECTED_IDENTIFIER.example = vitte build app.vit -o app

LINK_E_EXPECTED_EXPRESSION.summary = expressao esperado.
LINK_E_EXPECTED_EXPRESSION.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_EXPECTED_EXPRESSION.step1 = Check the target triple and the first backend or linker note.
LINK_E_EXPECTED_EXPRESSION.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_EXPECTED_EXPRESSION.example = vitte build app.vit -o app

LINK_E_EXPECTED_TYPE.summary = tipo esperado.
LINK_E_EXPECTED_TYPE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_EXPECTED_TYPE.step1 = Check the target triple and the first backend or linker note.
LINK_E_EXPECTED_TYPE.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_EXPECTED_TYPE.example = vitte build app.vit -o app

LINK_E_EXPECTED_PATTERN.summary = padrao esperado.
LINK_E_EXPECTED_PATTERN.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_EXPECTED_PATTERN.step1 = Check the target triple and the first backend or linker note.
LINK_E_EXPECTED_PATTERN.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_EXPECTED_PATTERN.example = vitte build app.vit -o app

LINK_E_EXPECTED_BLOCK.summary = bloco esperado.
LINK_E_EXPECTED_BLOCK.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_EXPECTED_BLOCK.step1 = Check the target triple and the first backend or linker note.
LINK_E_EXPECTED_BLOCK.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_EXPECTED_BLOCK.example = vitte build app.vit -o app

LINK_E_EXPECTED_DELIMITER.summary = delimitador esperado.
LINK_E_EXPECTED_DELIMITER.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_EXPECTED_DELIMITER.step1 = Check the target triple and the first backend or linker note.
LINK_E_EXPECTED_DELIMITER.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_EXPECTED_DELIMITER.example = vitte build app.vit -o app

LINK_E_UNEXPECTED_TOKEN.summary = token inesperado.
LINK_E_UNEXPECTED_TOKEN.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_UNEXPECTED_TOKEN.step1 = Check the target triple and the first backend or linker note.
LINK_E_UNEXPECTED_TOKEN.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_UNEXPECTED_TOKEN.example = vitte build app.vit -o app

LINK_E_UNBALANCED_DELIMITER.summary = unbalanced delimitador.
LINK_E_UNBALANCED_DELIMITER.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_UNBALANCED_DELIMITER.step1 = Check the target triple and the first backend or linker note.
LINK_E_UNBALANCED_DELIMITER.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_UNBALANCED_DELIMITER.example = vitte build app.vit -o app

LINK_E_INVALID_ATTRIBUTE.summary = attribute invalido.
LINK_E_INVALID_ATTRIBUTE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_ATTRIBUTE.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_ATTRIBUTE.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_ATTRIBUTE.example = vitte build app.vit -o app

LINK_E_INVALID_DECLARATION.summary = declaracao invalido.
LINK_E_INVALID_DECLARATION.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_DECLARATION.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_DECLARATION.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_DECLARATION.example = vitte build app.vit -o app

LINK_E_INVALID_STATEMENT.summary = instrucao invalido.
LINK_E_INVALID_STATEMENT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_STATEMENT.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_STATEMENT.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_STATEMENT.example = vitte build app.vit -o app

LINK_E_INVALID_EXPRESSION.summary = expressao invalido.
LINK_E_INVALID_EXPRESSION.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_EXPRESSION.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_EXPRESSION.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_EXPRESSION.example = vitte build app.vit -o app

LINK_E_INVALID_PATTERN.summary = padrao invalido.
LINK_E_INVALID_PATTERN.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_PATTERN.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_PATTERN.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_PATTERN.example = vitte build app.vit -o app

LINK_E_INVALID_LITERAL.summary = literal invalido.
LINK_E_INVALID_LITERAL.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_LITERAL.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_LITERAL.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_LITERAL.example = vitte build app.vit -o app

LINK_E_INVALID_OPERATOR.summary = operador invalido.
LINK_E_INVALID_OPERATOR.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_OPERATOR.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_OPERATOR.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_OPERATOR.example = vitte build app.vit -o app

LINK_E_INVALID_MODIFIER.summary = modifier invalido.
LINK_E_INVALID_MODIFIER.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_MODIFIER.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_MODIFIER.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_MODIFIER.example = vitte build app.vit -o app

LINK_E_MISSING_BODY.summary = body ausente.
LINK_E_MISSING_BODY.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_MISSING_BODY.step1 = Check the target triple and the first backend or linker note.
LINK_E_MISSING_BODY.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_MISSING_BODY.example = vitte build app.vit -o app

LINK_E_MISSING_RETURN.summary = retorno ausente.
LINK_E_MISSING_RETURN.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_MISSING_RETURN.step1 = Check the target triple and the first backend or linker note.
LINK_E_MISSING_RETURN.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_MISSING_RETURN.example = vitte build app.vit -o app

LINK_E_DUPLICATE_NAME.summary = nome duplicado.
LINK_E_DUPLICATE_NAME.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_DUPLICATE_NAME.step1 = Check the target triple and the first backend or linker note.
LINK_E_DUPLICATE_NAME.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_DUPLICATE_NAME.example = vitte build app.vit -o app

LINK_E_UNKNOWN_NAME.summary = nome desconhecido.
LINK_E_UNKNOWN_NAME.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_UNKNOWN_NAME.step1 = Check the target triple and the first backend or linker note.
LINK_E_UNKNOWN_NAME.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_UNKNOWN_NAME.example = vitte build app.vit -o app

LINK_E_UNKNOWN_TYPE.summary = tipo desconhecido.
LINK_E_UNKNOWN_TYPE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_UNKNOWN_TYPE.step1 = Check the target triple and the first backend or linker note.
LINK_E_UNKNOWN_TYPE.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_UNKNOWN_TYPE.example = vitte build app.vit -o app

LINK_E_UNKNOWN_MODULE.summary = modulo desconhecido.
LINK_E_UNKNOWN_MODULE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_UNKNOWN_MODULE.step1 = Check the target triple and the first backend or linker note.
LINK_E_UNKNOWN_MODULE.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_UNKNOWN_MODULE.example = vitte build app.vit -o app

LINK_E_UNKNOWN_MEMBER.summary = membro desconhecido.
LINK_E_UNKNOWN_MEMBER.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_UNKNOWN_MEMBER.step1 = Check the target triple and the first backend or linker note.
LINK_E_UNKNOWN_MEMBER.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_UNKNOWN_MEMBER.example = vitte build app.vit -o app

LINK_E_AMBIGUOUS_NAME.summary = ambiguous nome.
LINK_E_AMBIGUOUS_NAME.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_AMBIGUOUS_NAME.step1 = Check the target triple and the first backend or linker note.
LINK_E_AMBIGUOUS_NAME.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_AMBIGUOUS_NAME.example = vitte build app.vit -o app

LINK_E_PRIVATE_SYMBOL.summary = private simbolo.
LINK_E_PRIVATE_SYMBOL.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_PRIVATE_SYMBOL.step1 = Check the target triple and the first backend or linker note.
LINK_E_PRIVATE_SYMBOL.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_PRIVATE_SYMBOL.example = vitte build app.vit -o app

LINK_E_IMPORT_NOT_FOUND.summary = importacao ausente.
LINK_E_IMPORT_NOT_FOUND.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_IMPORT_NOT_FOUND.step1 = Check the target triple and the first backend or linker note.
LINK_E_IMPORT_NOT_FOUND.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_IMPORT_NOT_FOUND.example = vitte build app.vit -o app

LINK_E_IMPORT_CYCLE.summary = importacao ciclo.
LINK_E_IMPORT_CYCLE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_IMPORT_CYCLE.step1 = Check the target triple and the first backend or linker note.
LINK_E_IMPORT_CYCLE.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_IMPORT_CYCLE.example = vitte build app.vit -o app

LINK_E_EXPORT_CONFLICT.summary = export conflito.
LINK_E_EXPORT_CONFLICT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_EXPORT_CONFLICT.step1 = Check the target triple and the first backend or linker note.
LINK_E_EXPORT_CONFLICT.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_EXPORT_CONFLICT.example = vitte build app.vit -o app

LINK_E_ARITY_MISMATCH.summary = arity incompatibilidade.
LINK_E_ARITY_MISMATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_ARITY_MISMATCH.step1 = Check the target triple and the first backend or linker note.
LINK_E_ARITY_MISMATCH.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_ARITY_MISMATCH.example = vitte build app.vit -o app

LINK_E_ARGUMENT_MISMATCH.summary = argumento incompatibilidade.
LINK_E_ARGUMENT_MISMATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_ARGUMENT_MISMATCH.step1 = Check the target triple and the first backend or linker note.
LINK_E_ARGUMENT_MISMATCH.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_ARGUMENT_MISMATCH.example = vitte build app.vit -o app

LINK_E_ASSIGNMENT_MISMATCH.summary = atribuicao incompatibilidade.
LINK_E_ASSIGNMENT_MISMATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_ASSIGNMENT_MISMATCH.step1 = Check the target triple and the first backend or linker note.
LINK_E_ASSIGNMENT_MISMATCH.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_ASSIGNMENT_MISMATCH.example = vitte build app.vit -o app

LINK_E_BRANCH_MISMATCH.summary = branch incompatibilidade.
LINK_E_BRANCH_MISMATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_BRANCH_MISMATCH.step1 = Check the target triple and the first backend or linker note.
LINK_E_BRANCH_MISMATCH.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_BRANCH_MISMATCH.example = vitte build app.vit -o app

LINK_E_INVALID_CALL.summary = chamada invalido.
LINK_E_INVALID_CALL.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_CALL.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_CALL.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_CALL.example = vitte build app.vit -o app

LINK_E_INVALID_CAST.summary = conversao invalido.
LINK_E_INVALID_CAST.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_CAST.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_CAST.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_CAST.example = vitte build app.vit -o app

LINK_E_INVALID_INDEX.summary = indice invalido.
LINK_E_INVALID_INDEX.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_INDEX.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_INDEX.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_INDEX.example = vitte build app.vit -o app

LINK_E_INVALID_DEREF.summary = deref invalido.
LINK_E_INVALID_DEREF.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_DEREF.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_DEREF.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_DEREF.example = vitte build app.vit -o app

LINK_E_INVALID_BORROW.summary = emprestimo invalido.
LINK_E_INVALID_BORROW.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_BORROW.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_BORROW.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_BORROW.example = vitte build app.vit -o app

LINK_E_INVALID_MOVE.summary = movimento invalido.
LINK_E_INVALID_MOVE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_INVALID_MOVE.step1 = Check the target triple and the first backend or linker note.
LINK_E_INVALID_MOVE.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_INVALID_MOVE.example = vitte build app.vit -o app

LINK_E_USE_AFTER_MOVE.summary = use after movimento.
LINK_E_USE_AFTER_MOVE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_USE_AFTER_MOVE.step1 = Check the target triple and the first backend or linker note.
LINK_E_USE_AFTER_MOVE.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_USE_AFTER_MOVE.example = vitte build app.vit -o app

LINK_E_USE_AFTER_DROP.summary = use after descarte.
LINK_E_USE_AFTER_DROP.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_USE_AFTER_DROP.step1 = Check the target triple and the first backend or linker note.
LINK_E_USE_AFTER_DROP.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_USE_AFTER_DROP.example = vitte build app.vit -o app

LINK_E_USE_BEFORE_INIT.summary = use before init.
LINK_E_USE_BEFORE_INIT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_USE_BEFORE_INIT.step1 = Check the target triple and the first backend or linker note.
LINK_E_USE_BEFORE_INIT.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_USE_BEFORE_INIT.example = vitte build app.vit -o app

LINK_E_DOUBLE_DROP.summary = double descarte.
LINK_E_DOUBLE_DROP.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_DOUBLE_DROP.step1 = Check the target triple and the first backend or linker note.
LINK_E_DOUBLE_DROP.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_DOUBLE_DROP.example = vitte build app.vit -o app

LINK_E_BORROW_CONFLICT.summary = emprestimo conflito.
LINK_E_BORROW_CONFLICT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_BORROW_CONFLICT.step1 = Check the target triple and the first backend or linker note.
LINK_E_BORROW_CONFLICT.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_BORROW_CONFLICT.example = vitte build app.vit -o app

LINK_E_MUTABILITY_CONFLICT.summary = mutability conflito.
LINK_E_MUTABILITY_CONFLICT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_MUTABILITY_CONFLICT.step1 = Check the target triple and the first backend or linker note.
LINK_E_MUTABILITY_CONFLICT.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_MUTABILITY_CONFLICT.example = vitte build app.vit -o app

LINK_E_LIFETIME_TOO_SHORT.summary = tempo de vida demais short.
LINK_E_LIFETIME_TOO_SHORT.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_LIFETIME_TOO_SHORT.step1 = Check the target triple and the first backend or linker note.
LINK_E_LIFETIME_TOO_SHORT.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_LIFETIME_TOO_SHORT.example = vitte build app.vit -o app

LINK_E_DANGLING_REFERENCE.summary = dangling referencia.
LINK_E_DANGLING_REFERENCE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_DANGLING_REFERENCE.step1 = Check the target triple and the first backend or linker note.
LINK_E_DANGLING_REFERENCE.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_DANGLING_REFERENCE.example = vitte build app.vit -o app

LINK_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
LINK_E_NON_EXHAUSTIVE_MATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_NON_EXHAUSTIVE_MATCH.step1 = Check the target triple and the first backend or linker note.
LINK_E_NON_EXHAUSTIVE_MATCH.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_NON_EXHAUSTIVE_MATCH.example = vitte build app.vit -o app

LINK_E_UNREACHABLE_PATTERN.summary = inalcancavel padrao.
LINK_E_UNREACHABLE_PATTERN.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_UNREACHABLE_PATTERN.step1 = Check the target triple and the first backend or linker note.
LINK_E_UNREACHABLE_PATTERN.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_UNREACHABLE_PATTERN.example = vitte build app.vit -o app

LINK_E_CONST_REQUIRED.summary = const required.
LINK_E_CONST_REQUIRED.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_CONST_REQUIRED.step1 = Check the target triple and the first backend or linker note.
LINK_E_CONST_REQUIRED.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_CONST_REQUIRED.example = vitte build app.vit -o app

LINK_E_CONST_OVERFLOW.summary = const estouro.
LINK_E_CONST_OVERFLOW.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_CONST_OVERFLOW.step1 = Check the target triple and the first backend or linker note.
LINK_E_CONST_OVERFLOW.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_CONST_OVERFLOW.example = vitte build app.vit -o app

LINK_E_CONST_DIVISION_BY_ZERO.summary = const divisao by zero.
LINK_E_CONST_DIVISION_BY_ZERO.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_CONST_DIVISION_BY_ZERO.step1 = Check the target triple and the first backend or linker note.
LINK_E_CONST_DIVISION_BY_ZERO.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_CONST_DIVISION_BY_ZERO.example = vitte build app.vit -o app

LINK_E_CONST_CYCLE.summary = const ciclo.
LINK_E_CONST_CYCLE.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_CONST_CYCLE.step1 = Check the target triple and the first backend or linker note.
LINK_E_CONST_CYCLE.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_CONST_CYCLE.example = vitte build app.vit -o app

LINK_E_MACRO_NOT_FOUND.summary = macro ausente.
LINK_E_MACRO_NOT_FOUND.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_MACRO_NOT_FOUND.step1 = Check the target triple and the first backend or linker note.
LINK_E_MACRO_NOT_FOUND.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_MACRO_NOT_FOUND.example = vitte build app.vit -o app

LINK_E_MACRO_RECURSION.summary = macro recursion.
LINK_E_MACRO_RECURSION.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_MACRO_RECURSION.step1 = Check the target triple and the first backend or linker note.
LINK_E_MACRO_RECURSION.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_MACRO_RECURSION.example = vitte build app.vit -o app

LINK_E_MACRO_EXPANSION_FAILED.summary = macro expansion falhou.
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

LINK_E_GENERIC_ARGUMENT_MISSING.summary = generico argumento ausente.
LINK_E_GENERIC_ARGUMENT_MISSING.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_GENERIC_ARGUMENT_MISSING.step1 = Check the target triple and the first backend or linker note.
LINK_E_GENERIC_ARGUMENT_MISSING.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_GENERIC_ARGUMENT_MISSING.example = vitte build app.vit -o app

LINK_E_GENERIC_BOUND_FAILED.summary = generico bound falhou.
LINK_E_GENERIC_BOUND_FAILED.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_GENERIC_BOUND_FAILED.step1 = Check the target triple and the first backend or linker note.
LINK_E_GENERIC_BOUND_FAILED.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_GENERIC_BOUND_FAILED.example = vitte build app.vit -o app

LINK_E_UNSUPPORTED_TARGET.summary = destino nao suportado.
LINK_E_UNSUPPORTED_TARGET.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_UNSUPPORTED_TARGET.step1 = Check the target triple and the first backend or linker note.
LINK_E_UNSUPPORTED_TARGET.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_UNSUPPORTED_TARGET.example = vitte build app.vit -o app

LINK_E_ABI_MISMATCH.summary = abi incompatibilidade.
LINK_E_ABI_MISMATCH.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_ABI_MISMATCH.step1 = Check the target triple and the first backend or linker note.
LINK_E_ABI_MISMATCH.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_ABI_MISMATCH.example = vitte build app.vit -o app

LINK_E_LINK_FAILED.summary = link falhou.
LINK_E_LINK_FAILED.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_LINK_FAILED.step1 = Check the target triple and the first backend or linker note.
LINK_E_LINK_FAILED.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_LINK_FAILED.example = vitte build app.vit -o app

LINK_E_RUNTIME_PANIC.summary = runtime panico.
LINK_E_RUNTIME_PANIC.cause = The selected target, linker, ABI, object file, or native toolchain failed.
LINK_E_RUNTIME_PANIC.step1 = Check the target triple and the first backend or linker note.
LINK_E_RUNTIME_PANIC.fix = install the missing native tool, change target, or fix undefined symbols
LINK_E_RUNTIME_PANIC.example = vitte build app.vit -o app

RUNTIME_E_EXPECTED_IDENTIFIER.summary = identificador esperado.
RUNTIME_E_EXPECTED_IDENTIFIER.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_EXPECTED_IDENTIFIER.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_EXPECTED_IDENTIFIER.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_EXPECTED_IDENTIFIER.example = vitte check path/to/file.vit

RUNTIME_E_EXPECTED_EXPRESSION.summary = expressao esperado.
RUNTIME_E_EXPECTED_EXPRESSION.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_EXPECTED_EXPRESSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_EXPECTED_EXPRESSION.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_EXPECTED_EXPRESSION.example = vitte check path/to/file.vit

RUNTIME_E_EXPECTED_TYPE.summary = tipo esperado.
RUNTIME_E_EXPECTED_TYPE.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_EXPECTED_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_EXPECTED_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_EXPECTED_TYPE.example = vitte check path/to/file.vit

RUNTIME_E_EXPECTED_PATTERN.summary = padrao esperado.
RUNTIME_E_EXPECTED_PATTERN.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_EXPECTED_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_EXPECTED_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_EXPECTED_PATTERN.example = vitte check path/to/file.vit

RUNTIME_E_EXPECTED_BLOCK.summary = bloco esperado.
RUNTIME_E_EXPECTED_BLOCK.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_EXPECTED_BLOCK.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_EXPECTED_BLOCK.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_EXPECTED_BLOCK.example = vitte check path/to/file.vit

RUNTIME_E_EXPECTED_DELIMITER.summary = delimitador esperado.
RUNTIME_E_EXPECTED_DELIMITER.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_EXPECTED_DELIMITER.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_EXPECTED_DELIMITER.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_EXPECTED_DELIMITER.example = vitte check path/to/file.vit

RUNTIME_E_UNEXPECTED_TOKEN.summary = token inesperado.
RUNTIME_E_UNEXPECTED_TOKEN.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_UNEXPECTED_TOKEN.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_UNEXPECTED_TOKEN.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_UNEXPECTED_TOKEN.example = vitte check path/to/file.vit

RUNTIME_E_UNBALANCED_DELIMITER.summary = unbalanced delimitador.
RUNTIME_E_UNBALANCED_DELIMITER.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_UNBALANCED_DELIMITER.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_UNBALANCED_DELIMITER.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_UNBALANCED_DELIMITER.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_ATTRIBUTE.summary = attribute invalido.
RUNTIME_E_INVALID_ATTRIBUTE.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_ATTRIBUTE.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_ATTRIBUTE.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_ATTRIBUTE.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_DECLARATION.summary = declaracao invalido.
RUNTIME_E_INVALID_DECLARATION.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_DECLARATION.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_DECLARATION.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_DECLARATION.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_STATEMENT.summary = instrucao invalido.
RUNTIME_E_INVALID_STATEMENT.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_STATEMENT.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_STATEMENT.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_STATEMENT.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_EXPRESSION.summary = expressao invalido.
RUNTIME_E_INVALID_EXPRESSION.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_EXPRESSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_EXPRESSION.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_EXPRESSION.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_PATTERN.summary = padrao invalido.
RUNTIME_E_INVALID_PATTERN.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_PATTERN.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_LITERAL.summary = literal invalido.
RUNTIME_E_INVALID_LITERAL.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_LITERAL.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_LITERAL.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_LITERAL.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_OPERATOR.summary = operador invalido.
RUNTIME_E_INVALID_OPERATOR.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_OPERATOR.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_OPERATOR.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_OPERATOR.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_MODIFIER.summary = modifier invalido.
RUNTIME_E_INVALID_MODIFIER.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_MODIFIER.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_MODIFIER.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_MODIFIER.example = vitte check path/to/file.vit

RUNTIME_E_MISSING_BODY.summary = body ausente.
RUNTIME_E_MISSING_BODY.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_MISSING_BODY.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_MISSING_BODY.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_MISSING_BODY.example = vitte check path/to/file.vit

RUNTIME_E_MISSING_RETURN.summary = retorno ausente.
RUNTIME_E_MISSING_RETURN.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_MISSING_RETURN.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_MISSING_RETURN.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_MISSING_RETURN.example = vitte check path/to/file.vit

RUNTIME_E_DUPLICATE_NAME.summary = nome duplicado.
RUNTIME_E_DUPLICATE_NAME.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_DUPLICATE_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_DUPLICATE_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_DUPLICATE_NAME.example = vitte check path/to/file.vit

RUNTIME_E_UNKNOWN_NAME.summary = nome desconhecido.
RUNTIME_E_UNKNOWN_NAME.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_UNKNOWN_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_UNKNOWN_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_UNKNOWN_NAME.example = vitte check path/to/file.vit

RUNTIME_E_UNKNOWN_TYPE.summary = tipo desconhecido.
RUNTIME_E_UNKNOWN_TYPE.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_UNKNOWN_TYPE.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_UNKNOWN_TYPE.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_UNKNOWN_TYPE.example = vitte check path/to/file.vit

RUNTIME_E_UNKNOWN_MODULE.summary = modulo desconhecido.
RUNTIME_E_UNKNOWN_MODULE.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_UNKNOWN_MODULE.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_UNKNOWN_MODULE.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_UNKNOWN_MODULE.example = vitte check path/to/file.vit

RUNTIME_E_UNKNOWN_MEMBER.summary = membro desconhecido.
RUNTIME_E_UNKNOWN_MEMBER.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_UNKNOWN_MEMBER.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_UNKNOWN_MEMBER.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_UNKNOWN_MEMBER.example = vitte check path/to/file.vit

RUNTIME_E_AMBIGUOUS_NAME.summary = ambiguous nome.
RUNTIME_E_AMBIGUOUS_NAME.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_AMBIGUOUS_NAME.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_AMBIGUOUS_NAME.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_AMBIGUOUS_NAME.example = vitte check path/to/file.vit

RUNTIME_E_PRIVATE_SYMBOL.summary = private simbolo.
RUNTIME_E_PRIVATE_SYMBOL.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_PRIVATE_SYMBOL.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_PRIVATE_SYMBOL.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_PRIVATE_SYMBOL.example = vitte check path/to/file.vit

RUNTIME_E_IMPORT_NOT_FOUND.summary = importacao ausente.
RUNTIME_E_IMPORT_NOT_FOUND.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_IMPORT_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_IMPORT_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_IMPORT_NOT_FOUND.example = vitte check path/to/file.vit

RUNTIME_E_IMPORT_CYCLE.summary = importacao ciclo.
RUNTIME_E_IMPORT_CYCLE.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_IMPORT_CYCLE.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_IMPORT_CYCLE.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_IMPORT_CYCLE.example = vitte check path/to/file.vit

RUNTIME_E_EXPORT_CONFLICT.summary = export conflito.
RUNTIME_E_EXPORT_CONFLICT.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_EXPORT_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_EXPORT_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_EXPORT_CONFLICT.example = vitte check path/to/file.vit

RUNTIME_E_ARITY_MISMATCH.summary = arity incompatibilidade.
RUNTIME_E_ARITY_MISMATCH.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_ARITY_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_ARITY_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_ARITY_MISMATCH.example = vitte check path/to/file.vit

RUNTIME_E_ARGUMENT_MISMATCH.summary = argumento incompatibilidade.
RUNTIME_E_ARGUMENT_MISMATCH.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_ARGUMENT_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_ARGUMENT_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_ARGUMENT_MISMATCH.example = vitte check path/to/file.vit

RUNTIME_E_ASSIGNMENT_MISMATCH.summary = atribuicao incompatibilidade.
RUNTIME_E_ASSIGNMENT_MISMATCH.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_ASSIGNMENT_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_ASSIGNMENT_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_ASSIGNMENT_MISMATCH.example = vitte check path/to/file.vit

RUNTIME_E_BRANCH_MISMATCH.summary = branch incompatibilidade.
RUNTIME_E_BRANCH_MISMATCH.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_BRANCH_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_BRANCH_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_BRANCH_MISMATCH.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_CALL.summary = chamada invalido.
RUNTIME_E_INVALID_CALL.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_CALL.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_CALL.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_CALL.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_CAST.summary = conversao invalido.
RUNTIME_E_INVALID_CAST.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_CAST.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_CAST.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_CAST.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_INDEX.summary = indice invalido.
RUNTIME_E_INVALID_INDEX.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_INDEX.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_INDEX.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_INDEX.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_DEREF.summary = deref invalido.
RUNTIME_E_INVALID_DEREF.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_DEREF.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_DEREF.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_DEREF.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_BORROW.summary = emprestimo invalido.
RUNTIME_E_INVALID_BORROW.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_BORROW.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_BORROW.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_BORROW.example = vitte check path/to/file.vit

RUNTIME_E_INVALID_MOVE.summary = movimento invalido.
RUNTIME_E_INVALID_MOVE.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_INVALID_MOVE.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_INVALID_MOVE.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_INVALID_MOVE.example = vitte check path/to/file.vit

RUNTIME_E_USE_AFTER_MOVE.summary = use after movimento.
RUNTIME_E_USE_AFTER_MOVE.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_USE_AFTER_MOVE.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_USE_AFTER_MOVE.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_USE_AFTER_MOVE.example = vitte check path/to/file.vit

RUNTIME_E_USE_AFTER_DROP.summary = use after descarte.
RUNTIME_E_USE_AFTER_DROP.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_USE_AFTER_DROP.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_USE_AFTER_DROP.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_USE_AFTER_DROP.example = vitte check path/to/file.vit

RUNTIME_E_USE_BEFORE_INIT.summary = use before init.
RUNTIME_E_USE_BEFORE_INIT.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_USE_BEFORE_INIT.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_USE_BEFORE_INIT.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_USE_BEFORE_INIT.example = vitte check path/to/file.vit

RUNTIME_E_DOUBLE_DROP.summary = double descarte.
RUNTIME_E_DOUBLE_DROP.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_DOUBLE_DROP.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_DOUBLE_DROP.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_DOUBLE_DROP.example = vitte check path/to/file.vit

RUNTIME_E_BORROW_CONFLICT.summary = emprestimo conflito.
RUNTIME_E_BORROW_CONFLICT.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_BORROW_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_BORROW_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_BORROW_CONFLICT.example = vitte check path/to/file.vit

RUNTIME_E_MUTABILITY_CONFLICT.summary = mutability conflito.
RUNTIME_E_MUTABILITY_CONFLICT.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_MUTABILITY_CONFLICT.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_MUTABILITY_CONFLICT.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_MUTABILITY_CONFLICT.example = vitte check path/to/file.vit

RUNTIME_E_LIFETIME_TOO_SHORT.summary = tempo de vida demais short.
RUNTIME_E_LIFETIME_TOO_SHORT.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_LIFETIME_TOO_SHORT.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_LIFETIME_TOO_SHORT.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_LIFETIME_TOO_SHORT.example = vitte check path/to/file.vit

RUNTIME_E_DANGLING_REFERENCE.summary = dangling referencia.
RUNTIME_E_DANGLING_REFERENCE.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_DANGLING_REFERENCE.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_DANGLING_REFERENCE.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_DANGLING_REFERENCE.example = vitte check path/to/file.vit

RUNTIME_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
RUNTIME_E_NON_EXHAUSTIVE_MATCH.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_NON_EXHAUSTIVE_MATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_NON_EXHAUSTIVE_MATCH.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_NON_EXHAUSTIVE_MATCH.example = vitte check path/to/file.vit

RUNTIME_E_UNREACHABLE_PATTERN.summary = inalcancavel padrao.
RUNTIME_E_UNREACHABLE_PATTERN.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_UNREACHABLE_PATTERN.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_UNREACHABLE_PATTERN.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_UNREACHABLE_PATTERN.example = vitte check path/to/file.vit

RUNTIME_E_CONST_REQUIRED.summary = const required.
RUNTIME_E_CONST_REQUIRED.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_CONST_REQUIRED.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_CONST_REQUIRED.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_CONST_REQUIRED.example = vitte check path/to/file.vit

RUNTIME_E_CONST_OVERFLOW.summary = const estouro.
RUNTIME_E_CONST_OVERFLOW.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_CONST_OVERFLOW.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_CONST_OVERFLOW.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_CONST_OVERFLOW.example = vitte check path/to/file.vit

RUNTIME_E_CONST_DIVISION_BY_ZERO.summary = const divisao by zero.
RUNTIME_E_CONST_DIVISION_BY_ZERO.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_CONST_DIVISION_BY_ZERO.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_CONST_DIVISION_BY_ZERO.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_CONST_DIVISION_BY_ZERO.example = vitte check path/to/file.vit

RUNTIME_E_CONST_CYCLE.summary = const ciclo.
RUNTIME_E_CONST_CYCLE.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_CONST_CYCLE.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_CONST_CYCLE.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_CONST_CYCLE.example = vitte check path/to/file.vit

RUNTIME_E_MACRO_NOT_FOUND.summary = macro ausente.
RUNTIME_E_MACRO_NOT_FOUND.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_MACRO_NOT_FOUND.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_MACRO_NOT_FOUND.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_MACRO_NOT_FOUND.example = vitte check path/to/file.vit

RUNTIME_E_MACRO_RECURSION.summary = macro recursion.
RUNTIME_E_MACRO_RECURSION.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_MACRO_RECURSION.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_MACRO_RECURSION.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_MACRO_RECURSION.example = vitte check path/to/file.vit

RUNTIME_E_MACRO_EXPANSION_FAILED.summary = macro expansion falhou.
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

RUNTIME_E_GENERIC_ARGUMENT_MISSING.summary = generico argumento ausente.
RUNTIME_E_GENERIC_ARGUMENT_MISSING.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_GENERIC_ARGUMENT_MISSING.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_GENERIC_ARGUMENT_MISSING.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_GENERIC_ARGUMENT_MISSING.example = vitte check path/to/file.vit

RUNTIME_E_GENERIC_BOUND_FAILED.summary = generico bound falhou.
RUNTIME_E_GENERIC_BOUND_FAILED.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_GENERIC_BOUND_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_GENERIC_BOUND_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_GENERIC_BOUND_FAILED.example = vitte check path/to/file.vit

RUNTIME_E_UNSUPPORTED_TARGET.summary = destino nao suportado.
RUNTIME_E_UNSUPPORTED_TARGET.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_UNSUPPORTED_TARGET.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_UNSUPPORTED_TARGET.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_UNSUPPORTED_TARGET.example = vitte check path/to/file.vit

RUNTIME_E_ABI_MISMATCH.summary = abi incompatibilidade.
RUNTIME_E_ABI_MISMATCH.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_ABI_MISMATCH.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_ABI_MISMATCH.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_ABI_MISMATCH.example = vitte check path/to/file.vit

RUNTIME_E_LINK_FAILED.summary = link falhou.
RUNTIME_E_LINK_FAILED.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_LINK_FAILED.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_LINK_FAILED.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_LINK_FAILED.example = vitte check path/to/file.vit

RUNTIME_E_RUNTIME_PANIC.summary = runtime panico.
RUNTIME_E_RUNTIME_PANIC.cause = The runtime phase found code that violates this diagnostic rule.
RUNTIME_E_RUNTIME_PANIC.step1 = Fix the first span reported for this diagnostic, then run the command again.
RUNTIME_E_RUNTIME_PANIC.fix = repair the highlighted compiler contract before checking later diagnostics
RUNTIME_E_RUNTIME_PANIC.example = vitte check path/to/file.vit

DRIVER_E_EXPECTED_IDENTIFIER.summary = identificador esperado.
DRIVER_E_EXPECTED_IDENTIFIER.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_EXPECTED_IDENTIFIER.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_EXPECTED_IDENTIFIER.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_EXPECTED_IDENTIFIER.example = vitte check src/main.vit --lang en

DRIVER_E_EXPECTED_EXPRESSION.summary = expressao esperado.
DRIVER_E_EXPECTED_EXPRESSION.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_EXPECTED_EXPRESSION.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_EXPECTED_EXPRESSION.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_EXPECTED_EXPRESSION.example = vitte check src/main.vit --lang en

DRIVER_E_EXPECTED_TYPE.summary = tipo esperado.
DRIVER_E_EXPECTED_TYPE.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_EXPECTED_TYPE.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_EXPECTED_TYPE.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_EXPECTED_TYPE.example = vitte check src/main.vit --lang en

DRIVER_E_EXPECTED_PATTERN.summary = padrao esperado.
DRIVER_E_EXPECTED_PATTERN.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_EXPECTED_PATTERN.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_EXPECTED_PATTERN.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_EXPECTED_PATTERN.example = vitte check src/main.vit --lang en

DRIVER_E_EXPECTED_BLOCK.summary = bloco esperado.
DRIVER_E_EXPECTED_BLOCK.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_EXPECTED_BLOCK.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_EXPECTED_BLOCK.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_EXPECTED_BLOCK.example = vitte check src/main.vit --lang en

DRIVER_E_EXPECTED_DELIMITER.summary = delimitador esperado.
DRIVER_E_EXPECTED_DELIMITER.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_EXPECTED_DELIMITER.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_EXPECTED_DELIMITER.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_EXPECTED_DELIMITER.example = vitte check src/main.vit --lang en

DRIVER_E_UNEXPECTED_TOKEN.summary = token inesperado.
DRIVER_E_UNEXPECTED_TOKEN.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_UNEXPECTED_TOKEN.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_UNEXPECTED_TOKEN.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_UNEXPECTED_TOKEN.example = vitte check src/main.vit --lang en

DRIVER_E_UNBALANCED_DELIMITER.summary = unbalanced delimitador.
DRIVER_E_UNBALANCED_DELIMITER.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_UNBALANCED_DELIMITER.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_UNBALANCED_DELIMITER.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_UNBALANCED_DELIMITER.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_ATTRIBUTE.summary = attribute invalido.
DRIVER_E_INVALID_ATTRIBUTE.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_ATTRIBUTE.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_ATTRIBUTE.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_ATTRIBUTE.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_DECLARATION.summary = declaracao invalido.
DRIVER_E_INVALID_DECLARATION.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_DECLARATION.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_DECLARATION.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_DECLARATION.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_STATEMENT.summary = instrucao invalido.
DRIVER_E_INVALID_STATEMENT.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_STATEMENT.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_STATEMENT.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_STATEMENT.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_EXPRESSION.summary = expressao invalido.
DRIVER_E_INVALID_EXPRESSION.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_EXPRESSION.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_EXPRESSION.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_EXPRESSION.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_PATTERN.summary = padrao invalido.
DRIVER_E_INVALID_PATTERN.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_PATTERN.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_PATTERN.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_PATTERN.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_LITERAL.summary = literal invalido.
DRIVER_E_INVALID_LITERAL.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_LITERAL.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_LITERAL.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_LITERAL.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_OPERATOR.summary = operador invalido.
DRIVER_E_INVALID_OPERATOR.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_OPERATOR.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_OPERATOR.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_OPERATOR.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_MODIFIER.summary = modifier invalido.
DRIVER_E_INVALID_MODIFIER.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_MODIFIER.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_MODIFIER.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_MODIFIER.example = vitte check src/main.vit --lang en

DRIVER_E_MISSING_BODY.summary = body ausente.
DRIVER_E_MISSING_BODY.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_MISSING_BODY.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_MISSING_BODY.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_MISSING_BODY.example = vitte check src/main.vit --lang en

DRIVER_E_MISSING_RETURN.summary = retorno ausente.
DRIVER_E_MISSING_RETURN.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_MISSING_RETURN.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_MISSING_RETURN.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_MISSING_RETURN.example = vitte check src/main.vit --lang en

DRIVER_E_DUPLICATE_NAME.summary = nome duplicado.
DRIVER_E_DUPLICATE_NAME.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_DUPLICATE_NAME.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_DUPLICATE_NAME.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_DUPLICATE_NAME.example = vitte check src/main.vit --lang en

DRIVER_E_UNKNOWN_NAME.summary = nome desconhecido.
DRIVER_E_UNKNOWN_NAME.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_UNKNOWN_NAME.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_UNKNOWN_NAME.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_UNKNOWN_NAME.example = vitte check src/main.vit --lang en

DRIVER_E_UNKNOWN_TYPE.summary = tipo desconhecido.
DRIVER_E_UNKNOWN_TYPE.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_UNKNOWN_TYPE.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_UNKNOWN_TYPE.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_UNKNOWN_TYPE.example = vitte check src/main.vit --lang en

DRIVER_E_UNKNOWN_MODULE.summary = modulo desconhecido.
DRIVER_E_UNKNOWN_MODULE.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_UNKNOWN_MODULE.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_UNKNOWN_MODULE.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_UNKNOWN_MODULE.example = vitte check src/main.vit --lang en

DRIVER_E_UNKNOWN_MEMBER.summary = membro desconhecido.
DRIVER_E_UNKNOWN_MEMBER.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_UNKNOWN_MEMBER.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_UNKNOWN_MEMBER.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_UNKNOWN_MEMBER.example = vitte check src/main.vit --lang en

DRIVER_E_AMBIGUOUS_NAME.summary = ambiguous nome.
DRIVER_E_AMBIGUOUS_NAME.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_AMBIGUOUS_NAME.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_AMBIGUOUS_NAME.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_AMBIGUOUS_NAME.example = vitte check src/main.vit --lang en

DRIVER_E_PRIVATE_SYMBOL.summary = private simbolo.
DRIVER_E_PRIVATE_SYMBOL.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_PRIVATE_SYMBOL.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_PRIVATE_SYMBOL.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_PRIVATE_SYMBOL.example = vitte check src/main.vit --lang en

DRIVER_E_IMPORT_NOT_FOUND.summary = importacao ausente.
DRIVER_E_IMPORT_NOT_FOUND.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_IMPORT_NOT_FOUND.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_IMPORT_NOT_FOUND.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_IMPORT_NOT_FOUND.example = vitte check src/main.vit --lang en

DRIVER_E_IMPORT_CYCLE.summary = importacao ciclo.
DRIVER_E_IMPORT_CYCLE.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_IMPORT_CYCLE.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_IMPORT_CYCLE.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_IMPORT_CYCLE.example = vitte check src/main.vit --lang en

DRIVER_E_EXPORT_CONFLICT.summary = export conflito.
DRIVER_E_EXPORT_CONFLICT.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_EXPORT_CONFLICT.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_EXPORT_CONFLICT.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_EXPORT_CONFLICT.example = vitte check src/main.vit --lang en

DRIVER_E_ARITY_MISMATCH.summary = arity incompatibilidade.
DRIVER_E_ARITY_MISMATCH.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_ARITY_MISMATCH.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_ARITY_MISMATCH.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_ARITY_MISMATCH.example = vitte check src/main.vit --lang en

DRIVER_E_ARGUMENT_MISMATCH.summary = argumento incompatibilidade.
DRIVER_E_ARGUMENT_MISMATCH.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_ARGUMENT_MISMATCH.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_ARGUMENT_MISMATCH.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_ARGUMENT_MISMATCH.example = vitte check src/main.vit --lang en

DRIVER_E_ASSIGNMENT_MISMATCH.summary = atribuicao incompatibilidade.
DRIVER_E_ASSIGNMENT_MISMATCH.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_ASSIGNMENT_MISMATCH.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_ASSIGNMENT_MISMATCH.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_ASSIGNMENT_MISMATCH.example = vitte check src/main.vit --lang en

DRIVER_E_BRANCH_MISMATCH.summary = branch incompatibilidade.
DRIVER_E_BRANCH_MISMATCH.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_BRANCH_MISMATCH.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_BRANCH_MISMATCH.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_BRANCH_MISMATCH.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_CALL.summary = chamada invalido.
DRIVER_E_INVALID_CALL.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_CALL.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_CALL.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_CALL.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_CAST.summary = conversao invalido.
DRIVER_E_INVALID_CAST.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_CAST.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_CAST.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_CAST.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_INDEX.summary = indice invalido.
DRIVER_E_INVALID_INDEX.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_INDEX.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_INDEX.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_INDEX.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_DEREF.summary = deref invalido.
DRIVER_E_INVALID_DEREF.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_DEREF.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_DEREF.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_DEREF.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_BORROW.summary = emprestimo invalido.
DRIVER_E_INVALID_BORROW.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_BORROW.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_BORROW.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_BORROW.example = vitte check src/main.vit --lang en

DRIVER_E_INVALID_MOVE.summary = movimento invalido.
DRIVER_E_INVALID_MOVE.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_INVALID_MOVE.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_INVALID_MOVE.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_INVALID_MOVE.example = vitte check src/main.vit --lang en

DRIVER_E_USE_AFTER_MOVE.summary = use after movimento.
DRIVER_E_USE_AFTER_MOVE.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_USE_AFTER_MOVE.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_USE_AFTER_MOVE.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_USE_AFTER_MOVE.example = vitte check src/main.vit --lang en

DRIVER_E_USE_AFTER_DROP.summary = use after descarte.
DRIVER_E_USE_AFTER_DROP.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_USE_AFTER_DROP.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_USE_AFTER_DROP.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_USE_AFTER_DROP.example = vitte check src/main.vit --lang en

DRIVER_E_USE_BEFORE_INIT.summary = use before init.
DRIVER_E_USE_BEFORE_INIT.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_USE_BEFORE_INIT.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_USE_BEFORE_INIT.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_USE_BEFORE_INIT.example = vitte check src/main.vit --lang en

DRIVER_E_DOUBLE_DROP.summary = double descarte.
DRIVER_E_DOUBLE_DROP.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_DOUBLE_DROP.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_DOUBLE_DROP.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_DOUBLE_DROP.example = vitte check src/main.vit --lang en

DRIVER_E_BORROW_CONFLICT.summary = emprestimo conflito.
DRIVER_E_BORROW_CONFLICT.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_BORROW_CONFLICT.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_BORROW_CONFLICT.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_BORROW_CONFLICT.example = vitte check src/main.vit --lang en

DRIVER_E_MUTABILITY_CONFLICT.summary = mutability conflito.
DRIVER_E_MUTABILITY_CONFLICT.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_MUTABILITY_CONFLICT.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_MUTABILITY_CONFLICT.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_MUTABILITY_CONFLICT.example = vitte check src/main.vit --lang en

DRIVER_E_LIFETIME_TOO_SHORT.summary = tempo de vida demais short.
DRIVER_E_LIFETIME_TOO_SHORT.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_LIFETIME_TOO_SHORT.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_LIFETIME_TOO_SHORT.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_LIFETIME_TOO_SHORT.example = vitte check src/main.vit --lang en

DRIVER_E_DANGLING_REFERENCE.summary = dangling referencia.
DRIVER_E_DANGLING_REFERENCE.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_DANGLING_REFERENCE.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_DANGLING_REFERENCE.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_DANGLING_REFERENCE.example = vitte check src/main.vit --lang en

DRIVER_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
DRIVER_E_NON_EXHAUSTIVE_MATCH.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_NON_EXHAUSTIVE_MATCH.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_NON_EXHAUSTIVE_MATCH.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_NON_EXHAUSTIVE_MATCH.example = vitte check src/main.vit --lang en

DRIVER_E_UNREACHABLE_PATTERN.summary = inalcancavel padrao.
DRIVER_E_UNREACHABLE_PATTERN.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_UNREACHABLE_PATTERN.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_UNREACHABLE_PATTERN.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_UNREACHABLE_PATTERN.example = vitte check src/main.vit --lang en

DRIVER_E_CONST_REQUIRED.summary = const required.
DRIVER_E_CONST_REQUIRED.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_CONST_REQUIRED.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_CONST_REQUIRED.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_CONST_REQUIRED.example = vitte check src/main.vit --lang en

DRIVER_E_CONST_OVERFLOW.summary = const estouro.
DRIVER_E_CONST_OVERFLOW.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_CONST_OVERFLOW.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_CONST_OVERFLOW.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_CONST_OVERFLOW.example = vitte check src/main.vit --lang en

DRIVER_E_CONST_DIVISION_BY_ZERO.summary = const divisao by zero.
DRIVER_E_CONST_DIVISION_BY_ZERO.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_CONST_DIVISION_BY_ZERO.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_CONST_DIVISION_BY_ZERO.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_CONST_DIVISION_BY_ZERO.example = vitte check src/main.vit --lang en

DRIVER_E_CONST_CYCLE.summary = const ciclo.
DRIVER_E_CONST_CYCLE.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_CONST_CYCLE.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_CONST_CYCLE.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_CONST_CYCLE.example = vitte check src/main.vit --lang en

DRIVER_E_MACRO_NOT_FOUND.summary = macro ausente.
DRIVER_E_MACRO_NOT_FOUND.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_MACRO_NOT_FOUND.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_MACRO_NOT_FOUND.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_MACRO_NOT_FOUND.example = vitte check src/main.vit --lang en

DRIVER_E_MACRO_RECURSION.summary = macro recursion.
DRIVER_E_MACRO_RECURSION.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_MACRO_RECURSION.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_MACRO_RECURSION.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_MACRO_RECURSION.example = vitte check src/main.vit --lang en

DRIVER_E_MACRO_EXPANSION_FAILED.summary = macro expansion falhou.
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

DRIVER_E_GENERIC_ARGUMENT_MISSING.summary = generico argumento ausente.
DRIVER_E_GENERIC_ARGUMENT_MISSING.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_GENERIC_ARGUMENT_MISSING.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_GENERIC_ARGUMENT_MISSING.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_GENERIC_ARGUMENT_MISSING.example = vitte check src/main.vit --lang en

DRIVER_E_GENERIC_BOUND_FAILED.summary = generico bound falhou.
DRIVER_E_GENERIC_BOUND_FAILED.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_GENERIC_BOUND_FAILED.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_GENERIC_BOUND_FAILED.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_GENERIC_BOUND_FAILED.example = vitte check src/main.vit --lang en

DRIVER_E_UNSUPPORTED_TARGET.summary = destino nao suportado.
DRIVER_E_UNSUPPORTED_TARGET.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_UNSUPPORTED_TARGET.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_UNSUPPORTED_TARGET.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_UNSUPPORTED_TARGET.example = vitte check src/main.vit --lang en

DRIVER_E_ABI_MISMATCH.summary = abi incompatibilidade.
DRIVER_E_ABI_MISMATCH.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_ABI_MISMATCH.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_ABI_MISMATCH.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_ABI_MISMATCH.example = vitte check src/main.vit --lang en

DRIVER_E_LINK_FAILED.summary = link falhou.
DRIVER_E_LINK_FAILED.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_LINK_FAILED.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_LINK_FAILED.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_LINK_FAILED.example = vitte check src/main.vit --lang en

DRIVER_E_RUNTIME_PANIC.summary = runtime panico.
DRIVER_E_RUNTIME_PANIC.cause = The driver could not use the provided input, option, cache, output, or profile.
DRIVER_E_RUNTIME_PANIC.step1 = Re-run the command with --help and verify paths and option values.
DRIVER_E_RUNTIME_PANIC.fix = provide an existing input, writable output path, and supported target/profile
DRIVER_E_RUNTIME_PANIC.example = vitte check src/main.vit --lang en

LIMIT_E_EXPECTED_IDENTIFIER.summary = identificador esperado.
LIMIT_E_EXPECTED_IDENTIFIER.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_EXPECTED_IDENTIFIER.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_EXPECTED_IDENTIFIER.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_EXPECTED_IDENTIFIER.example = vitte check src/main.vit

LIMIT_E_EXPECTED_EXPRESSION.summary = expressao esperado.
LIMIT_E_EXPECTED_EXPRESSION.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_EXPECTED_EXPRESSION.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_EXPECTED_EXPRESSION.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_EXPECTED_EXPRESSION.example = vitte check src/main.vit

LIMIT_E_EXPECTED_TYPE.summary = tipo esperado.
LIMIT_E_EXPECTED_TYPE.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_EXPECTED_TYPE.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_EXPECTED_TYPE.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_EXPECTED_TYPE.example = vitte check src/main.vit

LIMIT_E_EXPECTED_PATTERN.summary = padrao esperado.
LIMIT_E_EXPECTED_PATTERN.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_EXPECTED_PATTERN.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_EXPECTED_PATTERN.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_EXPECTED_PATTERN.example = vitte check src/main.vit

LIMIT_E_EXPECTED_BLOCK.summary = bloco esperado.
LIMIT_E_EXPECTED_BLOCK.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_EXPECTED_BLOCK.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_EXPECTED_BLOCK.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_EXPECTED_BLOCK.example = vitte check src/main.vit

LIMIT_E_EXPECTED_DELIMITER.summary = delimitador esperado.
LIMIT_E_EXPECTED_DELIMITER.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_EXPECTED_DELIMITER.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_EXPECTED_DELIMITER.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_EXPECTED_DELIMITER.example = vitte check src/main.vit

LIMIT_E_UNEXPECTED_TOKEN.summary = token inesperado.
LIMIT_E_UNEXPECTED_TOKEN.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_UNEXPECTED_TOKEN.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_UNEXPECTED_TOKEN.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_UNEXPECTED_TOKEN.example = vitte check src/main.vit

LIMIT_E_UNBALANCED_DELIMITER.summary = unbalanced delimitador.
LIMIT_E_UNBALANCED_DELIMITER.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_UNBALANCED_DELIMITER.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_UNBALANCED_DELIMITER.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_UNBALANCED_DELIMITER.example = vitte check src/main.vit

LIMIT_E_INVALID_ATTRIBUTE.summary = attribute invalido.
LIMIT_E_INVALID_ATTRIBUTE.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_ATTRIBUTE.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_ATTRIBUTE.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_ATTRIBUTE.example = vitte check src/main.vit

LIMIT_E_INVALID_DECLARATION.summary = declaracao invalido.
LIMIT_E_INVALID_DECLARATION.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_DECLARATION.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_DECLARATION.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_DECLARATION.example = vitte check src/main.vit

LIMIT_E_INVALID_STATEMENT.summary = instrucao invalido.
LIMIT_E_INVALID_STATEMENT.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_STATEMENT.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_STATEMENT.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_STATEMENT.example = vitte check src/main.vit

LIMIT_E_INVALID_EXPRESSION.summary = expressao invalido.
LIMIT_E_INVALID_EXPRESSION.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_EXPRESSION.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_EXPRESSION.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_EXPRESSION.example = vitte check src/main.vit

LIMIT_E_INVALID_PATTERN.summary = padrao invalido.
LIMIT_E_INVALID_PATTERN.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_PATTERN.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_PATTERN.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_PATTERN.example = vitte check src/main.vit

LIMIT_E_INVALID_LITERAL.summary = literal invalido.
LIMIT_E_INVALID_LITERAL.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_LITERAL.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_LITERAL.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_LITERAL.example = vitte check src/main.vit

LIMIT_E_INVALID_OPERATOR.summary = operador invalido.
LIMIT_E_INVALID_OPERATOR.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_OPERATOR.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_OPERATOR.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_OPERATOR.example = vitte check src/main.vit

LIMIT_E_INVALID_MODIFIER.summary = modifier invalido.
LIMIT_E_INVALID_MODIFIER.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_MODIFIER.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_MODIFIER.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_MODIFIER.example = vitte check src/main.vit

LIMIT_E_MISSING_BODY.summary = body ausente.
LIMIT_E_MISSING_BODY.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_MISSING_BODY.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_MISSING_BODY.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_MISSING_BODY.example = vitte check src/main.vit

LIMIT_E_MISSING_RETURN.summary = retorno ausente.
LIMIT_E_MISSING_RETURN.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_MISSING_RETURN.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_MISSING_RETURN.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_MISSING_RETURN.example = vitte check src/main.vit

LIMIT_E_DUPLICATE_NAME.summary = nome duplicado.
LIMIT_E_DUPLICATE_NAME.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_DUPLICATE_NAME.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_DUPLICATE_NAME.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_DUPLICATE_NAME.example = vitte check src/main.vit

LIMIT_E_UNKNOWN_NAME.summary = nome desconhecido.
LIMIT_E_UNKNOWN_NAME.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_UNKNOWN_NAME.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_UNKNOWN_NAME.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_UNKNOWN_NAME.example = vitte check src/main.vit

LIMIT_E_UNKNOWN_TYPE.summary = tipo desconhecido.
LIMIT_E_UNKNOWN_TYPE.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_UNKNOWN_TYPE.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_UNKNOWN_TYPE.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_UNKNOWN_TYPE.example = vitte check src/main.vit

LIMIT_E_UNKNOWN_MODULE.summary = modulo desconhecido.
LIMIT_E_UNKNOWN_MODULE.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_UNKNOWN_MODULE.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_UNKNOWN_MODULE.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_UNKNOWN_MODULE.example = vitte check src/main.vit

LIMIT_E_UNKNOWN_MEMBER.summary = membro desconhecido.
LIMIT_E_UNKNOWN_MEMBER.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_UNKNOWN_MEMBER.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_UNKNOWN_MEMBER.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_UNKNOWN_MEMBER.example = vitte check src/main.vit

LIMIT_E_AMBIGUOUS_NAME.summary = ambiguous nome.
LIMIT_E_AMBIGUOUS_NAME.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_AMBIGUOUS_NAME.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_AMBIGUOUS_NAME.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_AMBIGUOUS_NAME.example = vitte check src/main.vit

LIMIT_E_PRIVATE_SYMBOL.summary = private simbolo.
LIMIT_E_PRIVATE_SYMBOL.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_PRIVATE_SYMBOL.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_PRIVATE_SYMBOL.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_PRIVATE_SYMBOL.example = vitte check src/main.vit

LIMIT_E_IMPORT_NOT_FOUND.summary = importacao ausente.
LIMIT_E_IMPORT_NOT_FOUND.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_IMPORT_NOT_FOUND.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_IMPORT_NOT_FOUND.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_IMPORT_NOT_FOUND.example = vitte check src/main.vit

LIMIT_E_IMPORT_CYCLE.summary = importacao ciclo.
LIMIT_E_IMPORT_CYCLE.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_IMPORT_CYCLE.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_IMPORT_CYCLE.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_IMPORT_CYCLE.example = vitte check src/main.vit

LIMIT_E_EXPORT_CONFLICT.summary = export conflito.
LIMIT_E_EXPORT_CONFLICT.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_EXPORT_CONFLICT.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_EXPORT_CONFLICT.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_EXPORT_CONFLICT.example = vitte check src/main.vit

LIMIT_E_ARITY_MISMATCH.summary = arity incompatibilidade.
LIMIT_E_ARITY_MISMATCH.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_ARITY_MISMATCH.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_ARITY_MISMATCH.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_ARITY_MISMATCH.example = vitte check src/main.vit

LIMIT_E_ARGUMENT_MISMATCH.summary = argumento incompatibilidade.
LIMIT_E_ARGUMENT_MISMATCH.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_ARGUMENT_MISMATCH.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_ARGUMENT_MISMATCH.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_ARGUMENT_MISMATCH.example = vitte check src/main.vit

LIMIT_E_ASSIGNMENT_MISMATCH.summary = atribuicao incompatibilidade.
LIMIT_E_ASSIGNMENT_MISMATCH.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_ASSIGNMENT_MISMATCH.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_ASSIGNMENT_MISMATCH.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_ASSIGNMENT_MISMATCH.example = vitte check src/main.vit

LIMIT_E_BRANCH_MISMATCH.summary = branch incompatibilidade.
LIMIT_E_BRANCH_MISMATCH.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_BRANCH_MISMATCH.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_BRANCH_MISMATCH.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_BRANCH_MISMATCH.example = vitte check src/main.vit

LIMIT_E_INVALID_CALL.summary = chamada invalido.
LIMIT_E_INVALID_CALL.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_CALL.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_CALL.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_CALL.example = vitte check src/main.vit

LIMIT_E_INVALID_CAST.summary = conversao invalido.
LIMIT_E_INVALID_CAST.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_CAST.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_CAST.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_CAST.example = vitte check src/main.vit

LIMIT_E_INVALID_INDEX.summary = indice invalido.
LIMIT_E_INVALID_INDEX.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_INDEX.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_INDEX.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_INDEX.example = vitte check src/main.vit

LIMIT_E_INVALID_DEREF.summary = deref invalido.
LIMIT_E_INVALID_DEREF.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_DEREF.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_DEREF.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_DEREF.example = vitte check src/main.vit

LIMIT_E_INVALID_BORROW.summary = emprestimo invalido.
LIMIT_E_INVALID_BORROW.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_BORROW.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_BORROW.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_BORROW.example = vitte check src/main.vit

LIMIT_E_INVALID_MOVE.summary = movimento invalido.
LIMIT_E_INVALID_MOVE.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_INVALID_MOVE.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_INVALID_MOVE.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_INVALID_MOVE.example = vitte check src/main.vit

LIMIT_E_USE_AFTER_MOVE.summary = use after movimento.
LIMIT_E_USE_AFTER_MOVE.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_USE_AFTER_MOVE.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_USE_AFTER_MOVE.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_USE_AFTER_MOVE.example = vitte check src/main.vit

LIMIT_E_USE_AFTER_DROP.summary = use after descarte.
LIMIT_E_USE_AFTER_DROP.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_USE_AFTER_DROP.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_USE_AFTER_DROP.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_USE_AFTER_DROP.example = vitte check src/main.vit

LIMIT_E_USE_BEFORE_INIT.summary = use before init.
LIMIT_E_USE_BEFORE_INIT.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_USE_BEFORE_INIT.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_USE_BEFORE_INIT.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_USE_BEFORE_INIT.example = vitte check src/main.vit

LIMIT_E_DOUBLE_DROP.summary = double descarte.
LIMIT_E_DOUBLE_DROP.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_DOUBLE_DROP.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_DOUBLE_DROP.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_DOUBLE_DROP.example = vitte check src/main.vit

LIMIT_E_BORROW_CONFLICT.summary = emprestimo conflito.
LIMIT_E_BORROW_CONFLICT.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_BORROW_CONFLICT.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_BORROW_CONFLICT.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_BORROW_CONFLICT.example = vitte check src/main.vit

LIMIT_E_MUTABILITY_CONFLICT.summary = mutability conflito.
LIMIT_E_MUTABILITY_CONFLICT.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_MUTABILITY_CONFLICT.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_MUTABILITY_CONFLICT.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_MUTABILITY_CONFLICT.example = vitte check src/main.vit

LIMIT_E_LIFETIME_TOO_SHORT.summary = tempo de vida demais short.
LIMIT_E_LIFETIME_TOO_SHORT.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_LIFETIME_TOO_SHORT.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_LIFETIME_TOO_SHORT.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_LIFETIME_TOO_SHORT.example = vitte check src/main.vit

LIMIT_E_DANGLING_REFERENCE.summary = dangling referencia.
LIMIT_E_DANGLING_REFERENCE.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_DANGLING_REFERENCE.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_DANGLING_REFERENCE.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_DANGLING_REFERENCE.example = vitte check src/main.vit

LIMIT_E_NON_EXHAUSTIVE_MATCH.summary = non exhaustive match.
LIMIT_E_NON_EXHAUSTIVE_MATCH.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_NON_EXHAUSTIVE_MATCH.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_NON_EXHAUSTIVE_MATCH.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_NON_EXHAUSTIVE_MATCH.example = vitte check src/main.vit

LIMIT_E_UNREACHABLE_PATTERN.summary = inalcancavel padrao.
LIMIT_E_UNREACHABLE_PATTERN.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_UNREACHABLE_PATTERN.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_UNREACHABLE_PATTERN.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_UNREACHABLE_PATTERN.example = vitte check src/main.vit

LIMIT_E_CONST_REQUIRED.summary = const required.
LIMIT_E_CONST_REQUIRED.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_CONST_REQUIRED.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_CONST_REQUIRED.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_CONST_REQUIRED.example = vitte check src/main.vit

LIMIT_E_CONST_OVERFLOW.summary = const estouro.
LIMIT_E_CONST_OVERFLOW.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_CONST_OVERFLOW.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_CONST_OVERFLOW.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_CONST_OVERFLOW.example = vitte check src/main.vit

LIMIT_E_CONST_DIVISION_BY_ZERO.summary = const divisao by zero.
LIMIT_E_CONST_DIVISION_BY_ZERO.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_CONST_DIVISION_BY_ZERO.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_CONST_DIVISION_BY_ZERO.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_CONST_DIVISION_BY_ZERO.example = vitte check src/main.vit

LIMIT_E_CONST_CYCLE.summary = const ciclo.
LIMIT_E_CONST_CYCLE.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_CONST_CYCLE.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_CONST_CYCLE.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_CONST_CYCLE.example = vitte check src/main.vit

LIMIT_E_MACRO_NOT_FOUND.summary = macro ausente.
LIMIT_E_MACRO_NOT_FOUND.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_MACRO_NOT_FOUND.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_MACRO_NOT_FOUND.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_MACRO_NOT_FOUND.example = vitte check src/main.vit

LIMIT_E_MACRO_RECURSION.summary = macro recursion.
LIMIT_E_MACRO_RECURSION.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_MACRO_RECURSION.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_MACRO_RECURSION.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_MACRO_RECURSION.example = vitte check src/main.vit

LIMIT_E_MACRO_EXPANSION_FAILED.summary = macro expansion falhou.
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

LIMIT_E_GENERIC_ARGUMENT_MISSING.summary = generico argumento ausente.
LIMIT_E_GENERIC_ARGUMENT_MISSING.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_GENERIC_ARGUMENT_MISSING.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_GENERIC_ARGUMENT_MISSING.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_GENERIC_ARGUMENT_MISSING.example = vitte check src/main.vit

LIMIT_E_GENERIC_BOUND_FAILED.summary = generico bound falhou.
LIMIT_E_GENERIC_BOUND_FAILED.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_GENERIC_BOUND_FAILED.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_GENERIC_BOUND_FAILED.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_GENERIC_BOUND_FAILED.example = vitte check src/main.vit

LIMIT_E_UNSUPPORTED_TARGET.summary = destino nao suportado.
LIMIT_E_UNSUPPORTED_TARGET.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_UNSUPPORTED_TARGET.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_UNSUPPORTED_TARGET.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_UNSUPPORTED_TARGET.example = vitte check src/main.vit

LIMIT_E_ABI_MISMATCH.summary = abi incompatibilidade.
LIMIT_E_ABI_MISMATCH.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_ABI_MISMATCH.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_ABI_MISMATCH.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_ABI_MISMATCH.example = vitte check src/main.vit

LIMIT_E_LINK_FAILED.summary = link falhou.
LIMIT_E_LINK_FAILED.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_LINK_FAILED.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_LINK_FAILED.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_LINK_FAILED.example = vitte check src/main.vit

LIMIT_E_RUNTIME_PANIC.summary = runtime panico.
LIMIT_E_RUNTIME_PANIC.cause = The input exceeded a configured compiler safety limit.
LIMIT_E_RUNTIME_PANIC.step1 = Split the file, expression, import graph, token, or macro expansion named by the code.
LIMIT_E_RUNTIME_PANIC.fix = reduce the input size or raise the limit only in a trusted build profile
LIMIT_E_RUNTIME_PANIC.example = vitte check src/main.vit
