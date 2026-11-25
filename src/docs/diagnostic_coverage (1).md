# Diagnostic coverage: grammar and semantic rules

This audit uses [`grammar/vitte.ebnf`](../../grammar/vitte.ebnf) as the canonical syntax inventory, the bootstrap C frontend under `src/bootstrap`, and the self-hosted frontend under `src/compiler`. It covers **148/148 named productions** plus semantic constraints that the grammar intentionally does not encode. The diagnostic contract suite maps all **117 nonterminal syntax productions** (before the lexical-forms section) to an executed invalid-input regression case.

Status means the implementation emits a source diagnostic for the invalid form. `Implemented` may still use a family-level parser diagnostic; `Missing` means the rule accepts invalid input or lacks its promised dedicated diagnostic; `Intentionally unsupported` means the current compiler scope explicitly defers the feature. A parser fixture proves valid syntax/AST construction, not semantic validity.

## Stable diagnostic identity

The bootstrap C registry and Vitte registry currently have exact parity across **264 public entries**: public code, internal symbolic name, severity, origin, category, title, and explanation. Preserve those identities and metadata when adding an emitter. The runtime format may serialize phase spelling differently (`type-check`/`type_check`); the registry origin remains the canonical phase owner.

## Phase classification and cancellation

| Failure kind | Owning phase | Diagnostic family | Meaning |
| --- | --- | --- | --- |
| Malformed source syntax | Lexer or parser | E0100–E0199, E0200–E0299, E0308–E0311 | The source cannot be tokenized or parsed according to the supported grammar. |
| Recognized but unsupported syntax | Parser | E0210 | The parser reached a declaration position whose source form is not implemented. This is a user-facing unsupported-syntax error, not a compiler phase failure. |
| Invalid source semantics | Import, name resolution, type checking, flow, contracts, generics or memory checks | E0300–E0799, E1100–E1299, E1500–E1799 | The source is grammatically valid but violates a language rule. The owning phase reports the source error and later phases stop. |
| Failed lowering | HIR or IR lowering | E1002, E1011 | A valid source form could not be represented by the selected intermediate lowering implementation. |
| Lowering/verifier limitation | HIR or IR lowering | E1004, E1012 | The source form is valid, but that lowerer does not implement it. Keep this separate from malformed syntax and failed compiler invariants. |
| Backend limitation/configuration | Selected backend | E0903, E0909 | The backend cannot emit a valid source construct, or its configuration is invalid. External C compilation, linking and execution retain their own E0904/E0905/E0906 identities. |
| Internal compiler failure | Compiler infrastructure or verifier | E0001, E0002, E0004, E1001, E1003, E1010 | The compiler failed internally, exhausted a resource, or produced an invalid intermediate representation. These failures are fatal and must not be presented as user syntax or type errors. |

Both frontends stop after a source-level parser or semantic error. They may report multiple independent source errors from that same phase, but must not add a lowering or backend diagnostic whose cause is only a consequence of an already-invalid construct. If a phase fails without having emitted a source diagnostic, report its classified infrastructure/lowering failure rather than silently returning or relabeling it as malformed source. Public title/explanation/help come from the paired diagnostic catalogs; phase-specific messages are kept byte-for-byte aligned between bootstrap and self-hosted adapters.

Lexer primary spans use half-open UTF-8 byte offsets. Invalid characters are shown with escaped source bytes and their byte range; malformed UTF-8 reports the offending bytes, encoding failure (truncated, overlong, surrogate, out-of-range or malformed), and byte range. A valid but disallowed identifier-start scalar is identified by its `U+` value and byte range. When a reserved word appears where a name is required, both frontends emit E0211 on that token and advise renaming it. Terminal underline placement converts source byte offsets to display columns so multi-byte characters are underlined as one glyph.

Literal diagnostics distinguish malformed integers (E0109), floating-point forms (E0110), and character literals (E0111). Numeric separators, based digits/prefixes, suffixes, and exponents have their own codes (E0112–E0115); integer overflow reports the literal, supported `u64` range, and byte range. String literals retain their dedicated unterminated-string and invalid-escape diagnostics (E0102/E0103). Boolean and null spellings are keywords in the current grammar, not separate quoted or numeric literal forms.

Known near-miss operator compounds use E0116 and identify the full sequence; unknown single punctuation characters retain E0101. Recoverable lexical errors are accumulated in source order (up to the lexer’s diagnostic limit); malformed literal contents are skipped through their closing delimiter so errors later in the file remain visible without treating the remainder of the literal as independent tokens.

| Public code | Internal symbolic name | Severity | Origin | Category | Title |
| --- | --- | --- | --- | --- | --- |
| E0101 | `VITTE_LEXER_E_INVALID_CHARACTER` | error | Lexer | lexer | invalid character |
| E0102 | `VITTE_LEXER_E_UNTERMINATED_STRING` | error | Lexer | lexer | unterminated string literal |
| E0103 | `VITTE_LEXER_E_INVALID_ESCAPE` | error | Lexer | lexer | invalid escape sequence |
| E0104 | `VITTE_LEXER_E_INVALID_NUMBER` | error | Lexer | lexer | invalid numeric literal |
| E0105 | `VITTE_LEXER_E_NUMBER_OVERFLOW` | error | Lexer | lexer | numeric literal overflow |
| E0106 | `VITTE_LEXER_E_UNTERMINATED_COMMENT` | error | Lexer | lexer | unterminated comment |
| E0107 | `VITTE_LEXER_E_INVALID_IDENTIFIER` | error | Lexer | lexer | invalid identifier |
| E0108 | `VITTE_LEXER_E_INVALID_UNICODE` | error | Lexer | unicode | invalid Unicode sequence |
| E0109 | `VITTE_LEXER_E_INVALID_INTEGER` | error | Lexer | lexer | malformed integer literal |
| E0110 | `VITTE_LEXER_E_INVALID_FLOAT` | error | Lexer | lexer | malformed floating-point literal |
| E0111 | `VITTE_LEXER_E_INVALID_CHARACTER_LITERAL` | error | Lexer | lexer | malformed character literal |
| E0112 | `VITTE_LEXER_E_INVALID_NUMERIC_SEPARATOR` | error | Lexer | lexer | invalid numeric separator |
| E0113 | `VITTE_LEXER_E_INVALID_NUMERIC_BASE` | error | Lexer | lexer | invalid integer base or digit |
| E0114 | `VITTE_LEXER_E_INVALID_NUMERIC_SUFFIX` | error | Lexer | lexer | unsupported numeric suffix |
| E0115 | `VITTE_LEXER_E_INVALID_EXPONENT` | error | Lexer | lexer | malformed exponent |
| E0116 | `VITTE_LEXER_E_INVALID_OPERATOR` | error | Lexer | lexer | invalid or incomplete operator |
| E0201 | `VITTE_PARSER_E_EXPECTED_TOKEN` | error | Parser | syntax | expected token |
| E0202 | `VITTE_PARSER_E_UNEXPECTED_TOKEN` | error | Parser | syntax | unexpected token |
| E0203 | `VITTE_PARSER_E_EXPECTED_EXPRESSION` | error | Parser | syntax | expected expression |
| E0204 | `VITTE_PARSER_E_EXPECTED_DECLARATION` | error | Parser | syntax | expected declaration |
| E0205 | `VITTE_PARSER_E_EXPECTED_TYPE` | error | Parser | syntax | expected type |
| E0206 | `VITTE_PARSER_E_EXPECTED_PATTERN` | error | Parser | syntax | expected pattern |
| E0207 | `VITTE_PARSER_E_UNCLOSED_DELIMITER` | error | Parser | syntax | unclosed delimiter |
| E0208 | `VITTE_PARSER_E_MISMATCHED_DELIMITER` | error | Parser | syntax | mismatched delimiter |
| E0209 | `VITTE_PARSER_E_INVALID_MODIFIER` | error | Parser | syntax | invalid declaration modifier |
| E0210 | `VITTE_PARSER_E_UNSUPPORTED_SYNTAX` | error | Parser | syntax | unsupported syntax |
| E0211 | `VITTE_PARSER_E_RESERVED_WORD_AS_NAME` | error | Parser | syntax | reserved word used as a name |
| E0212 | `VITTE_PARSER_E_INVALID_MODULE_PATH` | error | Parser | syntax | malformed module path |
| E0213 | `VITTE_PARSER_E_DUPLICATE_MODULE_HEADER` | error | Parser | syntax | duplicate module header |
| E0214 | `VITTE_PARSER_E_MISPLACED_MODULE_HEADER` | error | Parser | syntax | misplaced module header |
| E0215 | `VITTE_PARSER_E_INVALID_USE_DECLARATION` | error | Parser | syntax | malformed use declaration |
| E0216 | `VITTE_PARSER_E_INVALID_USE_ALIAS` | error | Parser | syntax | invalid import alias |
| E0217 | `VITTE_PARSER_E_INVALID_IMPORT_GROUP` | error | Parser | syntax | malformed import group |
| E0218 | `VITTE_PARSER_E_INVALID_GLOB_IMPORT` | error | Parser | syntax | invalid glob import |
| E0219 | `VITTE_PARSER_E_INVALID_EXPORT_CLAUSE` | error | Parser | syntax | malformed export clause |
| E0220 | `VITTE_PARSER_E_EMPTY_EXPORT_GROUP` | error | Parser | syntax | empty export group |
| E0221 | `VITTE_PARSER_E_INVALID_EXPORT_ALIAS` | error | Parser | syntax | invalid export alias |
| E0222 | `VITTE_PARSER_E_TRAILING_TOKENS` | error | Parser | syntax | unexpected trailing tokens |
| E0223 | `VITTE_PARSER_E_MISSING_MODULE_HEADER` | error | Parser | syntax | missing module header |
| E0224 | `VITTE_PARSER_E_INVALID_VALUE_DECLARATION` | error | Parser | syntax | malformed const or static declaration |
| E0225 | `VITTE_PARSER_E_MISSING_VALUE_INITIALIZER` | error | Parser | syntax | missing const or static initializer |
| E0226 | `VITTE_PARSER_E_INVALID_TYPE_DECLARATION` | error | Parser | syntax | malformed type declaration |
| E0227 | `VITTE_PARSER_E_INVALID_OPAQUE_DECLARATION` | error | Parser | syntax | malformed opaque declaration |
| E0228 | `VITTE_PARSER_E_INVALID_FORM_DECLARATION` | error | Parser | syntax | malformed form declaration |
| E0229 | `VITTE_PARSER_E_DUPLICATE_FORM_FIELD` | error | Parser | syntax | duplicate form field |
| E0230 | `VITTE_PARSER_E_INVALID_FORM_FIELD_SEPARATOR` | error | Parser | syntax | missing form field separator |
| E0231 | `VITTE_PARSER_E_INVALID_PICK_DECLARATION` | error | Parser | syntax | malformed pick declaration |
| E0232 | `VITTE_PARSER_E_DUPLICATE_PICK_VARIANT` | error | Parser | syntax | duplicate pick variant |
| E0233 | `VITTE_PARSER_E_INVALID_PICK_VARIANT_PAYLOAD` | error | Parser | syntax | invalid pick variant payload |
| E0234 | `VITTE_PARSER_E_INVALID_TRAIT_DECLARATION` | error | Parser | syntax | malformed trait declaration |
| E0235 | `VITTE_PARSER_E_INVALID_IMPL_DECLARATION` | error | Parser | syntax | malformed implementation declaration |
| E0236 | `VITTE_PARSER_E_MISSING_IMPL_TARGET_TYPE` | error | Parser | syntax | missing implementation target type |
| E0237 | `VITTE_PARSER_E_INVALID_IMPL_FOR_CLAUSE` | error | Parser | syntax | invalid implementation `for` clause |
| E0238 | `VITTE_PARSER_E_INVALID_PROCEDURE_DECLARATION` | error | Parser | syntax | malformed procedure declaration |
| E0239 | `VITTE_PARSER_E_INVALID_PROCEDURE_PARAMETERS` | error | Parser | syntax | malformed procedure parameter list |
| E0240 | `VITTE_PARSER_E_INVALID_PROCEDURE_RETURN_TYPE` | error | Parser | syntax | invalid procedure return type |
| E0241 | `VITTE_PARSER_E_INVALID_PROCEDURE_MODIFIER` | error | Parser | syntax | invalid procedure modifier |
| E0242 | `VITTE_PARSER_E_INVALID_PROCEDURE_BODY_FORM` | error | Parser | syntax | invalid procedure body form |
| E0243 | `VITTE_PARSER_E_INVALID_EXTERNAL_PROCEDURE` | error | Parser | syntax | malformed external procedure declaration |
| E0244 | `VITTE_PARSER_E_INVALID_INTRINSIC_DECLARATION` | error | Parser | syntax | malformed intrinsic declaration |
| E0245 | `VITTE_PARSER_E_INVALID_ABI_STRING_LITERAL` | error | Parser | syntax | invalid ABI string literal |
| E0246 | `VITTE_PARSER_E_INVALID_GENERIC_PARAMETER_LIST` | error | Parser | syntax | malformed generic parameter list |
| E0247 | `VITTE_PARSER_E_INVALID_CONST_GENERIC_PARAMETER` | error | Parser | syntax | malformed const generic parameter |
| E0248 | `VITTE_PARSER_E_INVALID_GENERIC_BOUND` | error | Parser | syntax | malformed generic bound |
| E0249 | `VITTE_PARSER_E_INVALID_WHERE_CLAUSE` | error | Parser | syntax | malformed where clause |
| E0250 | `VITTE_PARSER_E_INVALID_REQUIRES_CLAUSE` | error | Parser | syntax | malformed requires clause |
| E0251 | `VITTE_PARSER_E_INVALID_ENSURES_CLAUSE` | error | Parser | syntax | malformed ensures clause |
| E0252 | `VITTE_PARSER_E_INVALID_MACRO_DECLARATION` | error | Parser | syntax | malformed macro declaration |
| E0253 | `VITTE_PARSER_E_INVALID_MACRO_PARAMETERS` | error | Parser | syntax | malformed macro parameter list |
| E0254 | `VITTE_PARSER_E_INVALID_TEST_DECLARATION` | error | Parser | syntax | malformed test declaration |
| E0255 | `VITTE_PARSER_E_INVALID_DECLARATION_MODIFIER_ORDER` | error | Parser | syntax | invalid declaration modifier order or combination |
| E0256 | `VITTE_PARSER_E_INVALID_ATTRIBUTE` | error | Parser | syntax | malformed attribute |
| E0257 | `VITTE_PARSER_E_INVALID_ATTRIBUTE_NAME` | error | Parser | syntax | missing or invalid attribute name |
| E0258 | `VITTE_PARSER_E_INVALID_ATTRIBUTE_ARGUMENTS` | error | Parser | syntax | malformed attribute arguments |
| E0259 | `VITTE_PARSER_E_MISSING_ATTRIBUTE_VALUE` | error | Parser | syntax | missing attribute value |
| E0260 | `VITTE_PARSER_E_INVALID_ATTRIBUTE_LITERAL` | error | Parser | syntax | invalid attribute literal |
| E0261 | `VITTE_PARSER_E_UNEXPECTED_DECLARATION` | error | Parser | syntax | unexpected declaration |
| E0262 | `VITTE_PARSER_E_UNEXPECTED_STATEMENT` | error | Parser | syntax | unexpected statement |
| E0263 | `VITTE_PARSER_E_UNEXPECTED_EXPRESSION` | error | Parser | syntax | unexpected expression |
| E0264 | `VITTE_PARSER_E_INVALID_PROCEDURE_TYPE` | error | Parser | syntax | malformed procedure type |
| E0265 | `VITTE_PARSER_E_INVALID_REFERENCE_TYPE` | error | Parser | syntax | malformed reference type |
| E0266 | `VITTE_PARSER_E_INVALID_POINTER_TYPE` | error | Parser | syntax | malformed pointer type |
| E0267 | `VITTE_PARSER_E_INVALID_ARRAY_TYPE` | error | Parser | syntax | malformed array type |
| E0268 | `VITTE_PARSER_E_INVALID_SLICE_TYPE` | error | Parser | syntax | malformed slice type |
| E0269 | `VITTE_PARSER_E_INVALID_OPTIONAL_TYPE` | error | Parser | syntax | malformed optional type |
| E0270 | `VITTE_PARSER_E_INVALID_TYPE_PATH` | error | Parser | syntax | malformed type path |
| E0271 | `VITTE_PARSER_E_INVALID_GENERIC_ARGUMENTS` | error | Parser | syntax | malformed generic arguments |
| E0272 | `VITTE_PARSER_E_INVALID_LET_STATEMENT` | error | Parser | syntax | malformed let binding statement |
| E0273 | `VITTE_PARSER_E_INVALID_SET_STATEMENT` | error | Parser | syntax | malformed set statement |
| E0274 | `VITTE_PARSER_E_INVALID_ASSIGNMENT_OPERATOR` | error | Parser | syntax | invalid assignment operator |
| E0275 | `VITTE_PARSER_E_INVALID_RETURN_STATEMENT` | error | Parser | syntax | malformed return statement |
| E0276 | `VITTE_PARSER_E_INVALID_JUMP_STATEMENT` | error | Parser | syntax | malformed jump statement |
| E0277 | `VITTE_PARSER_E_INVALID_DEFER_STATEMENT` | error | Parser | syntax | malformed defer statement |
| E0278 | `VITTE_PARSER_E_INVALID_ASSERT_STATEMENT` | error | Parser | syntax | malformed assert statement |
| E0279 | `VITTE_PARSER_E_INVALID_UNSAFE_STATEMENT` | error | Parser | syntax | malformed unsafe statement |
| E0280 | `VITTE_PARSER_E_INVALID_ASM_STATEMENT` | error | Parser | syntax | malformed assembly statement |
| E0281 | `VITTE_PARSER_E_INVALID_CONTROL_FLOW_STATEMENT` | error | Parser | syntax | malformed control-flow statement |
| E0282 | `VITTE_PARSER_E_INVALID_MATCH_STATEMENT` | error | Parser | syntax | malformed match statement |
| E0283 | `VITTE_PARSER_E_INVALID_MATCH_ARM` | error | Parser | syntax | malformed match arm |
| E0284 | `VITTE_PARSER_E_INVALID_PATTERN` | error | Parser | syntax | malformed pattern |
| E0285 | `VITTE_PARSER_E_INVALID_RANGE_PATTERN` | error | Parser | syntax | malformed range pattern |
| E0286 | `VITTE_PARSER_E_INVALID_ASSIGNMENT_EXPRESSION` | error | Parser | syntax | malformed assignment expression |
| E0287 | `VITTE_PARSER_E_INVALID_COALESCING_EXPRESSION` | error | Parser | syntax | malformed coalescing expression |
| E0288 | `VITTE_PARSER_E_INVALID_LOGICAL_EXPRESSION` | error | Parser | syntax | malformed logical expression |
| E0289 | `VITTE_PARSER_E_INVALID_BITWISE_EXPRESSION` | error | Parser | syntax | malformed bitwise expression |
| E0290 | `VITTE_PARSER_E_INVALID_COMPARISON_EXPRESSION` | error | Parser | syntax | malformed comparison expression |
| E0291 | `VITTE_PARSER_E_INVALID_SHIFT_EXPRESSION` | error | Parser | syntax | malformed shift expression |
| E0292 | `VITTE_PARSER_E_INVALID_ARITHMETIC_EXPRESSION` | error | Parser | syntax | malformed arithmetic expression |
| E0293 | `VITTE_PARSER_E_INVALID_RANGE_EXPRESSION` | error | Parser | syntax | malformed range expression |
| E0294 | `VITTE_PARSER_E_INVALID_UNARY_EXPRESSION` | error | Parser | syntax | malformed unary expression |
| E0295 | `VITTE_PARSER_E_INVALID_CAST_EXPRESSION` | error | Parser | syntax | malformed cast expression |
| E0296 | `VITTE_PARSER_E_INVALID_POSTFIX_EXPRESSION` | error | Parser | syntax | malformed postfix expression |
| E0297 | `VITTE_PARSER_E_INVALID_IF_EXPRESSION` | error | Parser | syntax | malformed if expression |
| E0298 | `VITTE_PARSER_E_INVALID_BLOCK_EXPRESSION` | error | Parser | syntax | malformed block expression |
| E0299 | `VITTE_PARSER_E_INVALID_ARRAY_EXPRESSION` | error | Parser | syntax | malformed array expression |
| E0300 | `VITTE_IMPORT_E_MODULE_PATH_MISMATCH` | error | Import | module | module name does not match import path |
| E0301 | `VITTE_IMPORT_E_MODULE_NOT_FOUND` | error | Import | module | module not found |
| E0302 | `VITTE_IMPORT_E_PRIVATE_SYMBOL` | error | Import | module | private symbol imported |
| E0303 | `VITTE_IMPORT_E_CYCLE` | error | Import | module | cyclic module dependency |
| E0304 | `VITTE_IMPORT_E_DUPLICATE` | error | Import | module | duplicate import |
| E0305 | `VITTE_IMPORT_E_PACKAGE_NOT_FOUND` | error | Import | package | package not found |
| E0308 | `VITTE_PARSER_E_INVALID_SIZEOF_EXPRESSION` | error | Parser | syntax | malformed sizeof expression |
| E0309 | `VITTE_PARSER_E_INVALID_ALIGNOF_EXPRESSION` | error | Parser | syntax | malformed alignof expression |
| E0310 | `VITTE_PARSER_E_INVALID_NULL_CONSTRUCTION` | error | Parser | syntax | malformed null construction |
| E0311 | `VITTE_PARSER_E_INVALID_EXPRESSION_LIST` | error | Parser | syntax | malformed expression list |
| E0401 | `VITTE_RESOLVE_E_UNKNOWN_SYMBOL` | error | NameResolution | name-resolution | unknown symbol |
| E0402 | `VITTE_RESOLVE_E_REDECLARATION` | error | NameResolution | name-resolution | duplicate declaration |
| E0403 | `VITTE_RESOLVE_E_AMBIGUOUS_SYMBOL` | error | NameResolution | name-resolution | ambiguous symbol |
| E0404 | `VITTE_RESOLVE_E_PRIVATE` | error | NameResolution | visibility | symbol is not visible |
| E0405 | `VITTE_RESOLVE_E_INVALID_SCOPE` | error | NameResolution | scope | invalid scope |
| E0406 | `VITTE_RESOLVE_E_DUPLICATE_BINDING` | error | NameResolution | scope | duplicate binding |
| E0407 | `VITTE_RESOLVE_E_INVALID_EXPORT` | error | NameResolution | export | invalid export target |
| E0501 | `VITTE_TYPE_E_MISMATCH` | error | TypeCheck | type | type mismatch |
| E0502 | `VITTE_TYPE_E_CANNOT_INFER` | error | TypeCheck | type | type cannot be inferred |
| E0503 | `VITTE_TYPE_E_INVALID_CAST` | error | TypeCheck | type | invalid cast |
| E0510 | `VITTE_TYPE_E_UNKNOWN_TYPE` | error | TypeCheck | type | unknown type |
| E0511 | `VITTE_TYPE_E_NOT_A_TYPE` | error | TypeCheck | type | name is not a type |
| E0516 | `VITTE_TYPE_E_NOT_A_VALUE` | error | TypeCheck | type | type name used as a value |
| E0504 | `VITTE_TYPE_E_NOT_CALLABLE` | error | TypeCheck | type | value is not callable |
| E0505 | `VITTE_TYPE_E_NOT_INDEXABLE` | error | TypeCheck | type | value is not indexable |
| E0506 | `VITTE_TYPE_E_NO_MEMBER` | error | TypeCheck | type | member does not exist |
| E0510 | `VITTE_TYPE_E_UNKNOWN_TYPE` | error | TypeCheck | type | unknown type |
| E0511 | `VITTE_TYPE_E_NOT_A_TYPE` | error | TypeCheck | type | name is not a type |
| E0512 | `VITTE_TYPE_E_INVALID_OPERAND` | error | TypeCheck | type | invalid operand type |
| E0513 | `VITTE_TYPE_E_NOT_ASSIGNABLE` | error | TypeCheck | type | value is not assignable |
| E0514 | `VITTE_TYPE_E_INVALID_INDEX` | error | TypeCheck | type | invalid index type |
| E0515 | `VITTE_TYPE_E_CONDITION_NOT_BOOL` | error | TypeCheck | type | condition is not boolean |
| E0601 | `VITTE_CALL_E_ARITY` | error | TypeCheck | call | incorrect argument count |
| E0602 | `VITTE_CALL_E_ARGUMENT_TYPE` | error | TypeCheck | call | invalid argument type |
| E0603 | `VITTE_CALL_E_NO_CANDIDATE` | error | TypeCheck | call | no matching procedure |
| E0604 | `VITTE_CALL_E_AMBIGUOUS` | error | TypeCheck | call | ambiguous call |
| E0605 | `VITTE_CALL_E_RETURN_TYPE` | error | TypeCheck | procedure | invalid return type |
| E0606 | `VITTE_CALL_E_MODIFIER` | error | TypeCheck | procedure | invalid procedure modifier |
| E0607 | `VITTE_CALL_E_RETURN_OUTSIDE_PROC` | error | TypeCheck | procedure | return outside procedure |
| E0608 | `VITTE_CALL_E_PARAMETER` | error | TypeCheck | parameter | invalid procedure parameter |
| E0701 | `VITTE_CONTRACT_E_PRECONDITION` | error | Contract | contract | precondition not satisfied |
| E0702 | `VITTE_CONTRACT_E_POSTCONDITION` | error | Contract | contract | postcondition failed |
| E0704 | `VITTE_CONTRACT_E_REQUIRES` | error | Contract | contract | invalid requires clause |
| E0705 | `VITTE_CONTRACT_E_ENSURES` | error | Contract | contract | invalid ensures clause |
| E0707 | `VITTE_CONTRACT_E_RESULT` | error | Contract | contract | invalid result reference |
| E0709 | `VITTE_CONTRACT_E_CONTRADICTION` | error | Contract | contract | contradictory contract |
| E0710 | `VITTE_CONTRACT_E_EFFECT` | error | Contract | contract | invalid contract effect |
| E0801 | `VITTE_GENERIC_E_BOUND` | error | TypeCheck | generic | generic bound not satisfied |
| E0802 | `VITTE_GENERIC_E_ARGUMENT_COUNT` | error | TypeCheck | generic | incorrect generic argument count |
| E0810 | `VITTE_TRAIT_E_NOT_IMPLEMENTED` | error | TypeCheck | trait | trait not implemented |
| E0811 | `VITTE_TRAIT_E_AMBIGUOUS_IMPL` | error | TypeCheck | trait | ambiguous implementation |
| E0812 | `VITTE_TRAIT_E_CONFLICTING_IMPL` | error | TypeCheck | trait | conflicting implementation |
| E1101 | `VITTE_CONTROL_E_MISSING_RETURN` | error | TypeCheck | control-flow | missing return |
| E1102 | `VITTE_CONTROL_E_INVALID_BREAK` | error | TypeCheck | control-flow | invalid break |
| E1103 | `VITTE_CONTROL_E_INVALID_CONTINUE` | error | TypeCheck | control-flow | invalid continue |
| E1701 | `VITTE_ASYNC_E_AWAIT_CONTEXT` | error | TypeCheck | async | await outside async procedure |
| E1702 | `VITTE_ASYNC_E_TASK_TYPE` | error | TypeCheck | async | await operand is not a task |
| E1201 | `VITTE_MEMORY_E_INVALID_ACCESS` | error | TypeCheck | memory | invalid memory access |
| E1205 | `VITTE_MEMORY_E_REFERENCE_ESCAPE` | error | TypeCheck | memory | reference lifetime cannot be verified |
| E1206 | `VITTE_MEMORY_E_BORROW_CONFLICT` | error | TypeCheck | memory | conflicting reference borrow |
| E1207 | `VITTE_MEMORY_E_BORROWED_MUTATION` | error | TypeCheck | memory | access conflicts with a live reference |
| E1208 | `VITTE_MEMORY_E_MOVE_BORROWED` | error | TypeCheck | memory | cannot move a borrowed value |
| E1209 | `VITTE_MEMORY_E_USE_AFTER_MOVE` | error | TypeCheck | memory | use of moved value |
| E1301 | `VITTE_UNSAFE_E_REQUIRED` | error | TypeCheck | unsafe | unsafe context required |
| E1502 | `VITTE_CONST_E_OVERFLOW` | error | ConstantEval | const-eval | constant evaluation overflow |
| E1503 | `VITTE_CONST_E_DIVIDE_BY_ZERO` | error | ConstantEval | const-eval | division by zero |
| E1504 | `VITTE_CONST_E_CYCLE` | error | ConstantEval | const-eval | constant evaluation cycle |
| E1601 | `VITTE_PATTERN_E_NON_EXHAUSTIVE` | error | TypeCheck | pattern | non-exhaustive match |
| E1602 | `VITTE_PATTERN_E_UNREACHABLE` | error | TypeCheck | pattern | unreachable pattern |
| E1603 | `VITTE_PATTERN_E_TYPE` | error | TypeCheck | pattern | pattern type mismatch |
| E1604 | `VITTE_PATTERN_E_BINDING` | error | TypeCheck | pattern | inconsistent pattern binding |

Explanations are the canonical strings in the registries: `src/bootstrap/diagnostic/registry.c` and `src/compiler/diagnostic/registry.vitte`. They are compared in the registry audit; do not copy or fork their prose when adding an emitter.

## Grammar production ownership and coverage

For syntax errors, the parser owns productions before the lexical section; the lexer owns the lexical section. The parser currently stores the expected/found token and source span, recovers at synchronization points, and retains partial trees. The production-to-code column names the code family used for failures; it does not claim a distinct code per production.

| Production | Invalid-form owner | Public codes | Bootstrap | Vitte | Notes |
| --- | --- | --- | --- | --- | --- |
| `translation_unit` | Parser | E0204, E0202, E0223 | Implemented | Implemented | A `use` declaration or export clause without a module header reports E0223 at EOF; standalone source files may omit the header. |
| `declaration` | Parser | E0204, E0201, E0202, E0262, E0263 | Implemented | Implemented | Statements and expressions in a declaration position get distinct diagnostics; unrecognized syntax keeps the declaration/unsupported-syntax fallback. |
| `declaration_modifier` | Parser | E0241, E0255 | Implemented | Implemented | Procedure-only modifiers outside procedures, before export clauses, or repeated use E0241; duplicate/conflicting `pub`/`export` visibility modifiers, visibility modifiers placed after procedure effects, and visibility applied to an export clause use E0255. E0209 remains an unused generic fallback. |
| `declaration_body` | Parser | E0204, E0201, E0202 | Implemented (family fallback) | Implemented (family fallback) | Malformed form is reported by the parser family; no production-specific code is assigned. |
| `attribute` | Parser | E0256, E0257 | Implemented | Implemented | Missing opening/closing brackets use E0256; missing attribute names and qualified-name segments use E0257. |
| `attribute_arguments` | Parser | E0258 | Implemented | Implemented | Malformed separators, argument forms and parentheses use E0258; invalid or missing named values have dedicated codes. |
| `attribute_argument` | Parser | E0258, E0259, E0260 | Implemented | Implemented | Bare names/literals and `name = literal` are distinguished; a missing value uses E0259 and a nonliteral value uses E0260. |
| `attribute_name` | Parser | E0257 | Implemented | Implemented | Attribute names accept identifiers or keywords; missing names after `#[`, `#![` or `::` are reported at the missing segment. |
| `attribute_literal` | Parser | E0260 | Implemented | Implemented | Values are restricted to integer, float, string, character, boolean and `null` literals; lexical literal failures remain lexer diagnostics. |
| `space_declaration` | Parser | E0212, E0213, E0214, E0223, E0201 | Implemented | Implemented | `space` without a path and malformed path separators use E0212; a second top-level `space path` uses E0213, a header after another declaration uses E0214, and module imports/exports without a header use E0223. |
| `space_body` | Parser | E0204, E0201, E0202 | Implemented (family fallback) | Implemented (family fallback) | Malformed form is reported by the parser family; no production-specific code is assigned. |
| `use_declaration` | Parser | E0212, E0215, E0216, E0217, E0218, E0222 | Implemented | Implemented | Missing path, malformed tails, malformed aliases/groups, misplaced globs and tokens before `;` have stable diagnostics. |
| `import_items` | Parser | E0217, E0218 | Implemented | Implemented | Groups must be nonempty and comma-separated; nested groups are accepted and flattened to qualified paths in the AST; globs inside groups are rejected. |
| `import_item` | Parser | E0217, E0218 | Implemented | Implemented | A nested group follows a path with `::{...}`; malformed paths, missing braces, misplaced globs and trailing separators are diagnosed. |
| `path_alias` | Parser | E0212, E0216, E0217 | Implemented | Implemented | Path separators and aliases are checked; aliases require an identifier after `as`. |
| `export_clause` | Parser | E0219, E0220, E0221, E0222 | Implemented | Implemented | Simple and grouped exports accept path aliases; empty groups, invalid aliases, malformed forms and tokens before `;` are diagnosed. |
| `export_items` | Parser | E0219, E0220, E0221, E0222 | Implemented | Implemented | Export groups require at least one path and allow comma-separated aliases with an optional trailing comma. |
| `value_declaration` | Parser | E0224, E0225 | Implemented | Implemented | Missing names, type annotations, `=`, initializer expressions or `;` receive declaration-specific diagnostics; a missing initializer has its own code. |
| `type_declaration` | Parser | E0226, E0246, E0247, E0248, E0249 | Implemented | Implemented | Missing names, malformed aliases and missing terminators use E0226; generic parameters, bounds and `where` predicates use their dedicated codes. |
| `opaque_declaration` | Parser | E0227, E0246, E0247, E0248, E0249 | Implemented | Implemented | Missing names or aliased types use E0227; malformed generic parameters, bounds and `where` predicates have dedicated codes. Opaque bounds remain outside the grammar. |
| `form_declaration` | Parser | E0228, E0229, E0246, E0247, E0248, E0249 | Implemented | Implemented | Missing names/braces, malformed fields and duplicate fields are reported separately from generic and `where` errors. |
| `form_field` | Parser | E0228, E0229, E0230 | Implemented | Implemented | Fields require `name: Type`; adjacent fields require `,` or `;`, and duplicate names point at the second field. |
| `pick_declaration` | Parser | E0231, E0246, E0247, E0248, E0249 | Implemented | Implemented | Missing names, braces and variant separators use E0231; generic parameters, bounds and `where` predicates retain their specific diagnostics. |
| `pick_variant` | Parser | E0231, E0232, E0233 | Implemented | Implemented | Variant names must be unique; payloads are optional parenthesized comma-separated types, including the valid empty payload `()`. Duplicate variants underline the second name and relate the first declaration. |
| `trait_declaration` | Parser | E0234, E0246, E0247, E0248, E0249 | Implemented | Implemented | Missing trait names and declaration-block delimiters use E0234; generic parameter, bound and `where` errors retain their dedicated codes. Invalid members retain their own diagnostics. |
| `implementation_declaration` | Parser | E0235, E0236, E0237, E0246, E0247, E0248, E0249 | Implemented | Implemented | Supports inherent `impl Type` and trait `impl Trait for Type`; distinguishes missing target types and malformed `for` clauses from generic and `where` errors. |
| `declaration_block` | Parser | E0234, E0235 | Implemented | Implemented | Trait and implementation blocks require `{` and `}`; malformed member declarations retain their original cause during recovery. |
| `procedure_declaration` | Parser | E0238, E0240, E0241, E0242, E0246, E0247, E0248, E0249, E0250, E0251 | Implemented | Implemented | Requires a name and parameter list; reports incomplete return types, modifiers, generic constraints, contracts, and invalid body versus semicolon forms. |
| `external_procedure_declaration` | Parser | E0243, E0245, E0239, E0240, E0242, E0246, E0247, E0248, E0249, E0250, E0251 | Implemented | Implemented | Requires `extern`, an optional nonempty ABI string literal, `proc`, a valid signature, optional constraints/contracts and `;`; external bodies use the procedure body-form diagnostic. |
| `intrinsic_declaration` | Parser | E0244, E0239, E0240, E0242, E0246, E0247, E0248, E0249, E0250, E0251 | Implemented | Implemented | Requires an intrinsic name and valid signature, optional constraints/contracts and `;`; ABI literals and bodies are not accepted. |
| `procedure_parameters` | Parser | E0239 | Implemented | Implemented | Requires parentheses, typed patterns, comma separators, and a closing `)`; the offending token or EOF is primary. |
| `contract_clause` | Parser | E0250, E0251 | Implemented | Implemented | `requires` and `ensures` require an expression and a terminating `;`; the code identifies which clause failed. |
| `macro_declaration` | Parser | E0252, E0253 | Implemented | Implemented | Requires an identifier, an optional procedure-style typed parameter list, and a block; the declaration and parameter-list failures have distinct codes. |
| `test_declaration` | Parser | E0254 | Implemented | Implemented | Accepts no name, an identifier, or a string literal, followed by a block; malformed names or missing/malformed bodies point to the offending token. |
| `generic_parameters` | Parser | E0246, E0247, E0248 | Implemented | Implemented | Requires balanced angle brackets, comma-separated parameters, named const parameters and valid bounds after `:`/`+`; trailing commas are accepted. |
| `generic_parameter` | Parser | E0246, E0247, E0248 | Implemented | Implemented | Missing parameter names, malformed const parameters and incomplete or invalid bounds receive distinct codes. |
| `where_clause` | Parser | E0249 | Implemented | Implemented | Requires one or more complete comma-separated predicates and reports missing predicates, relations, bounds or separators. |
| `where_predicate` | Parser | E0249 | Implemented | Implemented | Supports type bounds and relational predicates; missing right-hand expressions or types identify the where clause. |
| `type_bound` | Parser | E0248, E0249 | Implemented | Implemented | Generic bound types use E0248; malformed bounds inside a `where` predicate use E0249. |
| `relational_operator` | Parser | E0204, E0201, E0202 | Implemented (family fallback) | Implemented (family fallback) | Malformed form is reported by the parser family; no production-specific code is assigned. |
| `procedure_parameter` | Parser | E0239 | Implemented | Implemented | Each parameter requires a valid pattern, a colon, and a type; supported passing modifiers are retained in the AST. |
| `type` | Parser | E0205, E0201, E0202, E0264–E0271 | Implemented | Implemented | Missing operands are reported as expected types; malformed type prefixes, paths and generic arguments retain their specific production code. |
| `procedure_type` | Parser | E0264 | Implemented | Implemented | Reports missing parentheses, parameter types, commas, closing delimiters and malformed return types. |
| `procedure_type_parameters` | Parser | E0264 | Implemented | Implemented | Procedure type parameters are comma-separated and report the missing comma or mismatched closing delimiter. |
| `procedure_type_parameter` | Parser | E0264, E0265–E0270 | Implemented | Implemented | A named parameter requires `name: Type`; its nested type keeps the more specific type diagnostic. |
| `reference_type` | Parser | E0265 | Implemented | Implemented | `&`/`ref` and optional `mut` require a complete pointee type. |
| `pointer_type` | Parser | E0266 | Implemented | Implemented | Pointer qualifiers and the pointee type are checked as one type production. |
| `array_or_slice_type` | Parser | E0267, E0268 | Implemented | Implemented | Fixed arrays and slices receive separate diagnostics for missing element types, lengths or closing brackets. |
| `optional_type` | Parser | E0269 | Implemented | Implemented | `?` requires a complete following type. |
| `path_type` | Parser | E0270, E0271 | Implemented | Implemented | Reports single-colon, empty, repeated or trailing qualification separators and malformed arguments in either bracket form. |
| `generic_arguments` | Parser | E0271 | Implemented | Implemented | Both `<Type, ...>` and `[Type, ...]` report invalid arguments, missing commas and mismatched or missing closers. |
| `module_path` | Parser | E0212, E0270 | Implemented | Implemented | Module paths use E0212; malformed paths used as types use E0270. A separator requires a segment on each side. |
| `value_path` | Parser | E0201, E0202 | Implemented (family fallback) | Implemented (family fallback) | Malformed form is reported by the parser family; no production-specific code is assigned. |
| `path_segment` | Parser | E0201, E0202, E0212, E0270 | Implemented | Implemented | Missing or extra path components are diagnosed at the segment or separator, with the expected identifier and found token. |
| `block` | Parser | E0201, E0202, E0261 | Implemented | Implemented | Declarations are rejected where the canonical grammar permits only statements, with E0261 on the declaration token. |
| `statement` | Parser | E0201, E0202, E0262, E0263 | Implemented | Implemented | Statements in declaration positions use E0262; expressions at declaration positions use E0263; statements in expression positions use E0262. |
| `let_statement` | Parser | E0272 | Implemented | Implemented | Missing patterns, incomplete optional types or initializers, and missing semicolons use a statement-specific diagnostic; missing inferable type information remains a semantic error. |
| `set_statement` | Parser | E0273, E0274 | Implemented | Implemented | Reports incomplete assignment expressions and terminators; an unsupported operator is underlined and shown with an expected assignment operator. |
| `return_statement` | Parser | E0275 | Implemented | Implemented | Reports malformed optional expressions and return at declaration scope; procedure placement and return-type compatibility are semantic checks. |
| `jump_statement` | Parser | E0276 | Implemented | Implemented | Reports malformed `break`/`continue` syntax, missing semicolons, and jumps at declaration scope; loop placement is validated by semantic analysis. |
| `defer_statement` | Parser | E0277 | Implemented | Implemented | Reports missing deferred block/expression and the expression-form semicolon; help shows both accepted forms. |
| `assert_statement` | Parser | E0278 | Implemented | Implemented | Requires `assert(expression);`; malformed delimiters, condition and terminator point to the expected form. |
| `if_statement` | Parser | E0281 | Implemented | Implemented | `if` and `elif` require a condition followed by a block; `else` requires a block or chained conditional. |
| `while_statement` | Parser | E0281 | Implemented | Implemented | Requires a complete condition and body block. |
| `loop_statement` | Parser | E0281 | Implemented | Implemented | Requires `loop { ... }` with a complete block. |
| `for_statement` | Parser | E0281 | Implemented | Implemented | Requires a binding pattern, `in`, an iterable expression and a body block. |
| `match_statement` | Parser | E0282 | Implemented | Implemented | Requires a subject expression, braces and complete arms. |
| `match_arm` | Parser | E0283, E0284, E0285 | Implemented | Implemented | Missing pattern/guard/arrow/body/separator uses E0283; malformed patterns preserve E0284/E0285. |
| `match_arm_body` | Parser | E0283 | Implemented | Implemented | Requires a complete expression, statement or block after `=>`. |
| `unsafe_statement` | Parser | E0279 | Implemented | Implemented | Requires the block form `unsafe { ... }`. |
| `assembly_statement` | Parser | E0280 | Implemented | Implemented | Requires an operand list or template string, balanced delimiters and a terminating semicolon. |
| `expression_statement` | Parser | E0201, E0202 | Implemented (family fallback) | Implemented (family fallback) | Malformed form is reported by the parser family; no production-specific code is assigned. |
| `pattern` | Parser | E0284, E0285 | Implemented | Implemented | Invalid path, literal, tuple, array, variant and struct pattern forms receive a pattern diagnostic; range errors retain their dedicated code. |
| `pattern_atom` | Parser | E0284, E0285 | Implemented | Implemented | Reports malformed pattern atoms while preserving invalid range endpoint diagnostics. |
| `pattern_list` | Parser | E0284 | Implemented | Implemented | Tuple, array and variant payload items require valid comma-separated patterns and matching delimiters. |
| `range_pattern` | Parser | E0285 | Implemented | Implemented | Supports exclusive `..` and inclusive `..=`; endpoints must be same-kind integer, float or character literals. |
| `literal_pattern` | Parser | E0284 | Implemented | Implemented | Requires a complete literal pattern rather than an arbitrary expression. |
| `pattern_literal` | Parser | E0284 | Implemented | Implemented | Malformed or unsupported literal patterns receive the pattern-specific diagnostic. |
| `generic_variant_pattern` | Parser | E0284 | Implemented | Implemented | Variant payloads and generic arguments must be complete and correctly separated. |
| `path_pattern` | Parser | E0284 | Implemented | Implemented | Path patterns require nonempty qualification segments and a valid terminal binding/variant. |
| `pattern_path` | Parser | E0284 | Implemented | Implemented | Missing or malformed path segments are reported at the offending token. |
| `struct_pattern_items` | Parser | E0284 | Implemented | Implemented | Struct fields and shorthand/rest items require valid names, patterns and separators. |
| `struct_pattern_item` | Parser | E0284 | Implemented | Implemented | Each field item requires a valid name and, when explicit, a complete nested pattern. |
| `expression` | Parser | E0286–E0299, E0308–E0311, E0203, E0201, E0202 | Implemented | Implemented | Operator families, postfix suffixes, arrays, type queries, null construction and expression lists have targeted diagnostics; all syntax productions map to invalid-input contract cases. |
| `assignment_expression` | Parser | E0286 | Implemented | Implemented | Assignment requires a complete target, supported operator and right-hand expression. |
| `assignment_operator` | Parser | E0203, E0201, E0202 | Implemented (family fallback) | Implemented (family fallback) | Malformed form is reported by the parser family; no production-specific code is assigned. |
| `coalesce_expression` | Parser | E0287 | Implemented | Implemented | Both sides of `??` must be complete expressions. |
| `logical_or_expression` | Parser | E0288 | Implemented | Implemented | Logical OR requires a complete operand before and after each operator. |
| `logical_and_expression` | Parser | E0288 | Implemented | Implemented | Logical AND requires a complete operand before and after each operator. |
| `bitwise_or_expression` | Parser | E0289 | Implemented | Implemented | Bitwise OR requires a complete operand before and after `|`. |
| `bitwise_xor_expression` | Parser | E0289 | Implemented | Implemented | Bitwise XOR requires a complete operand before and after `^`. |
| `bitwise_and_expression` | Parser | E0289 | Implemented | Implemented | Bitwise AND requires a complete operand before and after `&`. |
| `equality_expression` | Parser | E0290 | Implemented | Implemented | Equality operators require complete expressions on both sides. |
| `comparison_expression` | Parser | E0290 | Implemented | Implemented | Relational operators require complete expressions on both sides. |
| `shift_expression` | Parser | E0291 | Implemented | Implemented | Shift operators require a value and a complete shift-count expression. |
| `additive_expression` | Parser | E0292 | Implemented | Implemented | Addition and subtraction require complete expressions on both sides. |
| `multiplicative_expression` | Parser | E0292 | Implemented | Implemented | Multiplication, division and remainder require complete expressions on both sides. |
| `range_expression` | Parser | E0293 | Implemented | Implemented | Exclusive `..` and inclusive `..=` ranges require complete endpoints. |
| `unary_expression` | Parser | E0294 | Implemented | Implemented | Every unary operator requires one complete operand. |
| `unary_operator` | Parser | E0294 | Implemented | Implemented | A recognized unary operator without its operand reports a unary-expression diagnostic. |
| `cast_expression` | Parser | E0295 | Implemented | Implemented | A cast requires a complete type after `as`; type-specific failures retain their type diagnostic. |
| `postfix_expression` | Parser | E0296 | Implemented | Implemented | Malformed call, index, member, optional postfix, generic suffix or form initializer is reported at the invalid suffix; inner operator and generic-argument failures retain their own code. |
| `ordinary_postfix_suffix` | Parser | E0296 | Implemented | Implemented | Calls and indexes require complete argument/index expressions and matching delimiters; field access requires an identifier, and `?` follows a complete expression. |
| `generic_form_or_variant_suffix` | Parser | E0296, E0271 | Implemented | Implemented | Generic suffixes require nonempty, comma-separated type arguments and a complete form initializer or `::Variant` suffix. |
| `primary_expression` | Parser | E0299, E0308–E0311, E0203, E0201, E0202 | Implemented | Implemented | Arrays, `sizeof`, `alignof`, null construction and expression-list errors retain their construct-specific identity. |
| `sizeof_expression` | Parser | E0308 | Implemented | Implemented | Requires a complete type or expression operand in `sizeof(...)`. |
| `alignof_expression` | Parser | E0309 | Implemented | Implemented | Requires a complete type operand in `alignof(...)`. |
| `if_expression` | Parser | E0297 | Implemented | Implemented | Requires a condition, a block and an else branch; failures inside the condition preserve their operator/type diagnostic. |
| `if_expression_tail` | Parser | E0297 | Implemented | Implemented | Requires a complete `elif` chain or an `else` block/expression branch. |
| `block_expression` | Parser | E0298 | Implemented | Implemented | Requires complete block items, an optional final expression and a matching `}`. |
| `block_expression_statement` | Parser | E0203, E0201, E0202 | Implemented (family fallback) | Implemented (family fallback) | Malformed form is reported by the parser family; no production-specific code is assigned. |
| `if_statement_without_else` | Parser | E0203, E0201, E0202 | Implemented (family fallback) | Implemented (family fallback) | Malformed form is reported by the parser family; no production-specific code is assigned. |
| `array_expression` | Parser | E0299, E0311 | Implemented | Implemented | Requires complete elements, comma separators where a new element begins, and a closing `]`; `[]` and trailing commas remain valid. |
| `form_initializer` | Parser | E0296 | Implemented | Implemented | Requires a type/path, braces and complete comma-separated fields. |
| `form_initializer_field` | Parser | E0296 | Implemented | Implemented | Each field requires an identifier, `:` and a complete value expression. |
| `expression_list` | Parser | E0311 | Implemented | Implemented | Empty elements and missing commas are rejected; an insertion fix is attached only when the next token clearly starts another expression. |
| `literal` | Parser | E0203, E0201, E0202 | Implemented (family fallback) | Implemented (family fallback) | Malformed form is reported by the parser family; no production-specific code is assigned. |
| `EOF` | parser | E0202 | Implemented | Implemented | Unexpected tokens after the source boundary are a parser error. |
| `CR` | lexer | — | Implemented | Implemented | Accepted as line trivia. |
| `LF` | lexer | — | Implemented | Implemented | Accepted as line trivia. |
| `valid_utf8_scalar` | lexer | E0108 | Implemented | Implemented | Malformed UTF-8 is rejected; valid scalars are accepted. |
| `identifier` | lexer | E0107, E0108 | Implemented | Implemented | Invalid identifier start/continuation and malformed UTF-8. |
| `identifier_start` | lexer | E0107, E0108 | Implemented | Implemented | Invalid identifier start or malformed UTF-8. |
| `identifier_continue` | lexer | E0107, E0108 | Implemented | Implemented | Invalid identifier continuation or malformed UTF-8. |
| `vitte_keyword_token` | lexer/parser | E0202 | Implemented | Implemented | Tokenization is implemented; unsupported keyword use falls through to the parser. |
| `legacy_lexer_keyword` | lexer | — | Implemented (legacy token only) | Intentionally unsupported | The C bootstrap lexer recognizes legacy `give`; it is not Vitte syntax. |
| `lexer_only_operator` | parser | E0202 | Intentionally unsupported | Intentionally unsupported | `@` and `$` are tokens without canonical parser productions. |
| `integer_literal` | lexer | E0104, E0105 | Implemented | Implemented | Malformed digits/separators and integer range overflow. |
| `float_literal` | lexer | E0104 | Implemented | Implemented | Malformed decimal or hexadecimal floating literal. |
| `decimal_float` | lexer | E0104 | Implemented | Implemented | Malformed decimal mantissa or exponent. |
| `decimal_exponent` | lexer | E0104 | Implemented | Implemented | Missing or malformed exponent digits. |
| `hexadecimal_float` | lexer | E0104 | Implemented | Implemented | Malformed hexadecimal mantissa or missing `p` exponent. |
| `digit_sequence` | lexer | E0104 | Implemented | Implemented | Invalid decimal digits or separator placement. |
| `binary_digit_sequence` | lexer | E0104 | Implemented | Implemented | Invalid binary digits or separator placement. |
| `octal_digit_sequence` | lexer | E0104 | Implemented | Implemented | Invalid octal digits or separator placement. |
| `hexadecimal_digit_sequence` | lexer | E0104 | Implemented | Implemented | Invalid hexadecimal digits or separator placement. |
| `string_literal` | lexer | E0101, E0102, E0103, E0108 | Implemented | Implemented | Invalid character, missing quote, escape, or encoding. |
| `character_literal` | lexer | E0101, E0103, E0108 | Implemented | Implemented | Invalid character count/content, escape, or encoding. |
| `escape_sequence` | lexer | E0103 | Implemented | Implemented | Escape must be one of the canonical forms. |
| `string_character` | lexer | E0101, E0108 | Implemented | Implemented | Disallowed control/newline character or malformed encoding. |
| `character` | lexer | E0101, E0108 | Implemented | Implemented | Disallowed control/newline character or malformed encoding. |
| `BACKSLASH` | lexer | E0103 | Implemented | Implemented | Backslash must begin a recognized escape. |
| `digit` | lexer | E0104 | Implemented | Implemented | Out-of-base digits are rejected by the containing number scanner. |
| `binary_digit` | lexer | E0104 | Implemented | Implemented | Out-of-base digit is rejected by the number scanner. |
| `octal_digit` | lexer | E0104 | Implemented | Implemented | Out-of-base digit is rejected by the number scanner. |
| `hexadecimal_digit` | lexer | E0104 | Implemented | Implemented | Invalid digit is rejected by the number scanner. |
| `non_ascii_identifier_start` | lexer | E0107, E0108 | Implemented | Implemented | Unicode XID_Start membership and UTF-8 validity. |
| `non_ascii_identifier_continue` | lexer | E0107, E0108 | Implemented | Implemented | Unicode XID_Continue membership and UTF-8 validity. |

### Grammar-wide parser gaps

- Pattern failures now use E0284/E0285, operator families use E0286–E0295, postfix forms use E0296, if/block expressions use E0297/E0298, and arrays/type queries/null construction/expression lists use E0299 and E0308–E0311. E0206 remains an unused generic expected-pattern identity; E0207/E0208 are not yet emitted consistently for delimiter failures, which can still use E0201 or a production-specific diagnostic. E0209 overlaps with active procedure/modifier diagnostics E0241/E0242/E0255 and has no dedicated producer.
- E0285 diagnoses missing, non-literal, mismatched-kind and incomplete range-pattern endpoints. The pattern classifier ignores `..` inside tuple, array and struct delimiters so a struct-rest marker cannot turn an unrelated malformed field into a range error.
- Expected-identifier remains a family fallback, but unexpected declarations, statements and expressions now have separate E0261–E0263 identities at declaration, statement and expression boundaries.
- Both lexers expose a single last-error slot and stop lexing at the first lexical failure. Later independent lexical errors are not reported in the same pass.
- Syntax recovery is implemented in both parsers; the bootstrap and native parser READMEs describe synchronization and partial-tree retention. Error recovery does not yet guarantee a targeted diagnostic for every production.

## Semantic rules and responsible phase

These checks are outside EBNF. The table gives each rule family a single owner, its productions, and the active public code family. “Partial” means the listed subset is implemented; it does not imply a full language proof.

| Semantic rule / invalid form | Productions | Owner | Bootstrap C | Vitte frontend | Public diagnostic identity |
| --- | --- | --- | --- | --- | --- |
| Module lookup, package lookup, module-name/path agreement, duplicate imports, import cycles, and imported-symbol visibility | `space_declaration`, `use_declaration`, `import_items`, `path_alias`, `export_clause`, `export_items` | Implemented by the bootstrap CLI resolver: malformed paths use E0212/E0215; missing modules E0301 include candidate forms and ordered search roots; missing declared package roots use E0305; a resolved file with a mismatched `space` header uses E0300 with the import as a related location; duplicate imports and cycles include related import locations; private names report through E0404 | Missing in the direct native frontend: it does not resolve source imports or produce E0300/E0301/E0303/E0304/E0305; E0302 remains registered without a producer | E0212 `VITTE_PARSER_E_INVALID_MODULE_PATH`; E0215 `VITTE_PARSER_E_INVALID_USE_DECLARATION`; E0300 `VITTE_IMPORT_E_MODULE_PATH_MISMATCH`; E0301 `VITTE_IMPORT_E_MODULE_NOT_FOUND`; E0302 `VITTE_IMPORT_E_PRIVATE_SYMBOL` (missing); E0303 `VITTE_IMPORT_E_CYCLE`; E0304 `VITTE_IMPORT_E_DUPLICATE`; E0305 `VITTE_IMPORT_E_PACKAGE_NOT_FOUND`; E0404 `VITTE_RESOLVE_E_PRIVATE` |
| Name resolution, visibility, duplicate declarations, shadowing | declaration, paths, parameters, bindings, patterns, imports | NameResolution | Implemented | Implemented | E0401 `VITTE_RESOLVE_E_UNKNOWN_SYMBOL`; E0402 `VITTE_RESOLVE_E_REDECLARATION` includes the prior declaration; E0404 `VITTE_RESOLVE_E_PRIVATE` includes the private declaration plus `pub`/`export` guidance; W0030 `VITTE_WARNING_SHADOWING` includes the shadowed declaration. Nested local shadowing is allowed; duplicate bindings in one scope are errors. |
| Ambiguous names, invalid scopes, repeated pattern bindings, export targets | declarations, `pattern`, `pattern_list`, `struct_pattern_item`, `export_clause`, `export_items` | NameResolution / TypeCheck | Implemented | Implemented | E0403 `VITTE_RESOLVE_E_AMBIGUOUS_SYMBOL` reports candidates; E0405 `VITTE_RESOLVE_E_INVALID_SCOPE` rejects export clauses outside root/module scope; E0406 `VITTE_RESOLVE_E_DUPLICATE_BINDING` reports the new binding and previous binding; E0407 `VITTE_RESOLVE_E_INVALID_EXPORT` rejects missing targets and re-exports. `export` clauses address declarations in the current module only. |
| Type resolution, type/value mismatch, operator operands, assignability, indexing, members, conditions | `type`, type productions, expressions, `set_statement`, loops, matches, form fields | TypeCheck | Implemented for core types and operators | Implemented for core types and operators | E0501 `VITTE_TYPE_E_MISMATCH`; E0504–E0506; E0510–E0516 |
| Type inference failure, invalid cast, incomplete/recursive type diagnostics | `type`, `cast_expression`, declarations, expressions | TypeCheck | E0502/E0503 implemented; E0508 reports opaque types used where layout or members are required; E0509 reports recursive alias expansion | E0502/E0503 implemented; E0508 reports opaque types used where layout or members are required; E0509 reports recursive alias expansion | E0502 `VITTE_TYPE_E_CANNOT_INFER`; E0503 `VITTE_TYPE_E_INVALID_CAST`; E0508 `VITTE_TYPE_E_INCOMPLETE`; E0509 `VITTE_TYPE_E_RECURSIVE` |
| Calls, argument count/type, return context and return type | procedure declarations, parameters, call suffix, return statement | TypeCheck | Implemented; E0605 reports missing values in non-void procedures and incompatible values (including values returned from void procedures), with expected/found types and the declared return type location when explicit; E1101 reports reachable fallthrough; a second unreachable return is covered by W0020; unresolved names and types suppress secondary return mismatches and missing-return cascades | Partial: argument and return mismatch diagnostics; no missing-return analysis | E0601 `VITTE_CALL_E_ARITY`; E0602 `VITTE_CALL_E_ARGUMENT_TYPE`; E0605 `VITTE_CALL_E_RETURN_TYPE`; E0607 `VITTE_CALL_E_RETURN_OUTSIDE_PROC`; E1101 `VITTE_CONTROL_E_MISSING_RETURN`; W0020 `VITTE_WARNING_UNREACHABLE_CODE` |
| Overload resolution and procedure modifier/calling-context validation | generic calls, `procedure_declaration`, `external_procedure_declaration`, `intrinsic_declaration`, modifiers | TypeCheck | Implemented for non-generic overload signatures, async/comptime context and extern/intrinsic modifier conflicts | Missing these dedicated checks in the native semantic emitter | E0603 `VITTE_CALL_E_NO_CANDIDATE`; E0604 `VITTE_CALL_E_AMBIGUOUS`; E0606 `VITTE_CALL_E_MODIFIER`; E1701 `VITTE_ASYNC_E_AWAIT_CONTEXT`; E1702 `VITTE_ASYNC_E_TASK_TYPE` |
| Parameter binding patterns, modes and mutability | `procedure_parameter`, patterns, `set_statement` | TypeCheck / Ownership | E0608 rejects refutable parameter patterns and the `owned`/`borrow` modes whose calling convention is not implemented; `mut` is honored; immutable assignment help suggests `mut` or a mutable local | E0608 producer and return-error recovery are present; parity is not yet validated by the native build | E0608 `VITTE_CALL_E_PARAMETER`; E0513 `VITTE_TYPE_E_NOT_ASSIGNABLE` |
| Contract typing, result availability, pre/postcondition proof and forbidden effects | `contract_clause`, procedure declarations, calls, return paths | Contract checker | Implemented for the documented subset | Missing: current native semantic diagnostic model has no contract rules/codes and accepts these invalid cases | E0701/E0702/E0704/E0705/E0707/E0709/E0710 (`VITTE_CONTRACT_E_*`) |
| Generic argument substitution/arity and trait bounds/coherence | generic parameters/arguments, where clauses, traits, impls, generic calls | TypeCheck | Partial: E0801 reports failed trait bounds at the call with concrete type and bound location; E0802/E0803 distinguish arity and const-versus-type argument errors; E0804 reports conflicting inferred substitutions; E0812 detects repeated concrete and alpha-equivalent generic impl headers; E0813–E0816 cover missing/invalid trait members, impl targets and where predicates; E0817 reports incompatible same-name methods across generic bounds and relates another candidate. Non-identical overlapping impl patterns and ambiguous impl candidates are not resolved. | Partial: E0801–E0804 and E0812–E0817 are present; E0812 currently detects repeated concrete impls only. Generic impl overlap/coherence and full candidate resolution remain incomplete. Const generic values in type applications remain unsupported and receive E0803 when supplied as a type. | E0801 `VITTE_GENERIC_E_BOUND`; E0802 `VITTE_GENERIC_E_ARGUMENT_COUNT`; E0803 `VITTE_GENERIC_E_ARGUMENT_KIND`; E0804 `VITTE_GENERIC_E_INFERENCE_CONFLICT`; E0812–E0817 `VITTE_TRAIT_E_*` |
| Return completeness, loop control legality, unreachable statements | procedures, blocks, returns, jumps, loops, if, match | Control-flow analysis | Implemented (E1101–E1103, W0020) | Implemented (E1102/E1103 and W0020); missing E1101 | E1101 `VITTE_CONTROL_E_MISSING_RETURN`; E1102 `VITTE_CONTROL_E_INVALID_BREAK`; E1103 `VITTE_CONTROL_E_INVALID_CONTINUE`; W0020 `VITTE_WARNING_UNREACHABLE_CODE` |
| Constant overflow, divide-by-zero, dependency cycles, statically known bounds | constants, array types, literals, arithmetic, indexes | Constant evaluator / TypeCheck | Implemented (E1502–E1504, E1201) | Missing: current native frontend accepts the audited invalid examples | E1502 `VITTE_CONST_E_OVERFLOW`; E1503 `VITTE_CONST_E_DIVIDE_BY_ZERO`; E1504 `VITTE_CONST_E_CYCLE`; E1201 `VITTE_MEMORY_E_INVALID_ACCESS` |
| Pattern typing, exhaustiveness, unreachable arms and consistent bindings | patterns, match arms, match statement | TypeCheck / pattern analysis | Implemented (partial: bool exhaustiveness, known duplicate/unreachable checks and type mismatch); intentionally unsupported non-bool constructor/range coverage | Implemented (E1604 inconsistent-binding code); missing E1601–E1603 | E1601 `VITTE_PATTERN_E_NON_EXHAUSTIVE`; E1602 `VITTE_PATTERN_E_UNREACHABLE`; E1603 `VITTE_PATTERN_E_TYPE`; E1604 `VITTE_PATTERN_E_BINDING` |
| Borrow conflicts, use-after-move, reference escape and owner lifetime | reference/pointer types, let/set, move/ref unary forms, calls, returns | Ownership / borrow analysis | Implemented (partial, experimental; direct roots and selected aliases only); intentionally unsupported complete alias/lifetime proof | Implemented (partial E1205–E1209 code paths); intentionally unsupported complete alias/lifetime proof | E1205 `VITTE_MEMORY_E_REFERENCE_ESCAPE`; E1206 `VITTE_MEMORY_E_BORROW_CONFLICT`; E1207 `VITTE_MEMORY_E_BORROWED_MUTATION`; E1208 `VITTE_MEMORY_E_MOVE_BORROWED`; E1209 `VITTE_MEMORY_E_USE_AFTER_MOVE` |
| Unsafe-context requirement and assembly/intrinsic constraints | unsafe statement, assembly, intrinsic declaration, pointers | TypeCheck / target validation | E1301 unsafe-required path exists; assembly/intrinsic validation incomplete | Missing: native semantic code model has no unsafe/asm diagnostic | E1301 `VITTE_UNSAFE_E_REQUIRED`; E1302 `VITTE_UNSAFE_E_INVALID_ASM`; E1303 `VITTE_UNSAFE_E_INTRINSIC` |
| FFI types, ABI, variadic calls and external symbol names | extern procedures, procedure types, calls | TypeCheck / ABI validation | Missing active dedicated producer | Missing active dedicated producer | E1401–E1404 `VITTE_FFI_E_*` |
| Async context/task validity | async modifiers, await, task-valued calls | TypeCheck | Implemented: await outside async is E1701; a non-task await operand in async code is E1702 | Missing dedicated E1701/E1702 producer | E1701 `VITTE_ASYNC_E_AWAIT_CONTEXT`; E1702 `VITTE_ASYNC_E_TASK_TYPE` |
| Macro expansion and recursion | macro declarations (the canonical grammar has no invocation production) | Macro expansion | Intentionally unsupported | Intentionally unsupported | E1801 `VITTE_MACRO_E_EXPANSION`; E1802 `VITTE_MACRO_E_RECURSION` |

### Confirmed generic, misleading, duplicate, or swallowed failures

- **Generic parser identities:** E0201/E0202 still absorb some expected-identifier and token failures; E0206–E0209 have no dedicated producer. Defer/assert/unsafe/assembly, control-flow, match arms, patterns, operator families, postfix forms and if/block expressions now use E0277–E0298; let/set/assignment/return/jump statements use E0272–E0276, while unexpected declarations/statements/expressions at grammar boundaries use E0261–E0263.
- **Misleading bootstrap type detail:** invalid `1 && 2` was rendered as `E0512` with `expected i64, found i64`. The Vitte frontend produced the useful `expected bool, found i64`. The bootstrap now renders operand-type wording instead of a false expected/found pair.
- **Generic API fallback:** E0014 `VITTE_FRONTEND_E_SOURCE` is intentionally a last-resort API/driver diagnostic when a native API rejects source without returning parser/sema details. If used after a detailed error exists, it would hide that cause; preserve the detailed diagnostic first.
- **Single lexer error:** lexer APIs preserve one `last_error`; a first lexical failure prevents later independent lexer errors from surfacing in the same pass.
- **No semantic cascade suppression parity guarantee:** bootstrap tests explicitly pin “unknown name suppresses follow-on operand/type errors”; the native frontend should gain the same regression coverage for its separate type-check paths.
- **Unsupported coverage is not a parse failure:** native currently accepts contract, missing-return, constant-cycle, and non-exhaustive boolean-match examples without a semantic diagnostic; these are missing semantic checks, not intentionally unsupported syntax.
- **Catalog entries without producers or parity:** besides E0206–E0209, E0302, E0507, native overload/modifier/async checks E0603/E0604/E0606/E1701/E1702, E0810/E0811 and E1401–E1404, these diagnostics lack an active producer in both frontends or a matching producer in each frontend. Generic impl overlap, fully ambiguous trait implementation selection and complete signature compatibility checks for trait members remain unsupported; the active diagnostics cover only the cases described in the table.

## Evidence and limitations

- Bootstrap semantic coverage and documented limitations: `src/bootstrap/sema/README.md`, `src/tests/bootstrap_semantic_diagnostics_test.py`, and `src/tests/diagnostic_contract_test.py`.
- Native parser recovery/grammar claims: `src/compiler/parser/README.md`; native parser diagnostics and codes: `src/compiler/parser/parser.vitte` and `src/compiler/driver/pipeline.vitte`.
- Native semantic diagnostic enum and emitters: `src/compiler/sema/sema.vitte`; frontend pipeline behavior was compared with the bootstrap binary for representative contract, constant, flow, pattern, type and name failures.
- The existing canonical grammar parser fixture validates AST construction but not all invalid forms or semantic rules. `src/docs/language_coverage.md` describes an older compiler profile and is not used as current diagnostic coverage evidence.
- Later-phase invalid IR, C17, ABI, and target cases are owned by the origins represented by E0900–E0999, E1000–E1099, E1400–E1499, and E1900–E1999; their producer coverage is not rated by this frontend audit.

Last audited against the working tree on 2026-11-18.
