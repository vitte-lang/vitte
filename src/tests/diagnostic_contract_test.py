"""Pin the public contract of errors emitted by the C bootstrap frontend."""

import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile


binary = Path(sys.argv[1]).resolve()
root = Path(__file__).resolve().parents[2]

# Each span is the exact source fragment that the compiler must underline.
# Byte offsets and human-readable line/column coordinates are checked
# separately, including a multibyte UTF-8 character before a later error.
CASES = (
    ("requires-not-bool", "compile",
     "proc f(x: int) -> int requires x + 1; { return x; }",
     ("E0704",), "contract", "x + 1"),
    ("ensures-not-bool", "compile",
     "proc f(x: int) -> int ensures result + 1; { return x; }",
     ("E0705",), "contract", "result + 1"),
    ("requires-false", "compile",
     "proc f() -> int requires false; { return 0; }",
     ("E0709",), "contract", "false"),
    ("ensures-false", "compile",
     "proc f() -> int ensures false; { return 0; }",
     ("E0702",), "contract", "false"),
    ("requires-result", "compile",
     "proc f() -> int requires result > 0; { return 1; }",
     ("E0707",), "contract", "result"),
    ("ensures-void-result", "compile",
     "proc f() ensures result > 0; { }",
     ("E0707",), "contract", "result"),
    ("ensures-old-unavailable", "compile",
     "proc f(x: int) -> int ensures old(x) == x; { return x; }",
     ("E0706",), "contract", "old(x)"),
    ("requires-old-unavailable", "compile",
     "proc f(x: int) -> int requires old(x) > 0; { return x; }",
     ("E0706",), "contract", "old(x)"),
    ("ensures-local-unavailable", "compile",
     "proc f() -> int ensures local > 0; { let local = 1; return local; }",
     ("E0401",), "name-resolution", "local"),
    ("multiple-contract-errors", "compile",
     'proc f() -> int requires 1; ensures 2; { return "bad"; }',
     ("E0704", "E0705", "E0605"), "contract", "1",
     {"E0705": "2", "E0605": '"bad"'}),
    ("unreachable-after-branches", "compile",
     "proc f(x: bool) -> int { if x { return 1; } else { return 2; } let y = 3; }",
     ("W0020",), "type-check", "let y = 3;"),
    ("unreachable-constant-if-branch", "compile",
     "proc f() -> i64 { if false { return 1; } else { return 2; } }",
     ("W0020",), "type-check", "{ return 1; }"),
    ("unreachable-constant-while-body", "compile",
     "proc f() { while false { let value = 1; } }",
     ("W0020",), "type-check", "{ let value = 1; }"),
    ("unreachable-constant-expression-if-branch", "compile",
     "proc f() -> i64 { if 1 < 2 { return 1; } else { return 2; } }",
     ("W0020",), "type-check", "{ return 2; }"),
    ("unreachable-constant-expression-while-body", "compile",
     "proc f() { while 2 > 3 { let value = 1; } }",
     ("W0020",), "type-check", "{ let value = 1; }"),
    ("unreachable-constant-elif-branch", "compile",
     "proc f(x: bool) { if x { let a = 0; } elif true { let b = 0; } else { let c = 0; } }",
     ("W0020",), "type-check", "{ let c = 0; }"),
    ("missing-return-empty", "compile",
     "proc f() -> int { }", ("E1101",), "type-check", "}"),
    ("missing-return-local", "compile",
     "proc f() -> int { let x = 1; }", ("E1101",), "type-check", "}"),
    ("missing-return-branch", "compile",
     "proc f(x: bool) -> int { if x { return 1; } }",
     ("E1101",), "type-check", ("}", 1)),
    ("constant-divide-zero", "compile",
     "proc main() -> int { return 8 / (2 - 2); }",
     ("E1503",), "constant-eval", "8 / (2 - 2)"),
    ("constant-remainder-zero", "compile",
     "proc main() -> int { return 8 % 0; }",
     ("E1506",), "constant-eval", "8 % 0"),
    ("constant-overflow", "compile",
     "proc main() -> int { return 9223372036854775807 + 1; }",
     ("E1502",), "constant-eval", "9223372036854775807 + 1"),
    ("constant-call-unsupported", "compile",
     "proc identity(x: int) -> int { return x; } const VALUE: int = identity(1); proc main() -> int { return VALUE; }",
     ("E1501",), "constant-eval", "identity(1)"),
    ("constant-invalid-shift", "compile",
     "proc main() -> int { return 1 << 64; }",
     ("E0512",), "type-check", "1 << 64"),
    ("array-index-out-of-bounds", "compile",
     "proc main() -> int { let a = [1, 2]; return a[2]; }",
     ("E1201",), "type-check", "a[2]"),
    ("array-negative-index", "compile",
     "proc main() -> int { let a = [1, 2]; return a[-1]; }",
     ("E1201",), "type-check", "a[-1]"),
    ("call-precondition", "compile",
     "proc f(x: int) -> int requires x > 0; { return x; } "
     "proc main() -> int { return f(-1); }",
     ("E0701",), "contract", "f(-1)"),
    ("return-postcondition", "compile",
     "proc f() -> int ensures result > 0; { return -1; }",
     ("E0702",), "contract", "return -1;"),
    ("requires-computed-false", "compile",
     "proc f() -> int requires 2 > 3; { return 0; }",
     ("E0709",), "contract", "2 > 3"),
    ("missing-return-loop-break", "compile",
     "proc f() -> int { loop { break; } }",
     ("E1101",), "type-check", ("}", 1)),
    ("bool-match-missing-arm", "compile",
     "proc f(x: bool) -> int { match x { true => { return 1; }, } }",
     ("E1601",), "type-check", "match x { true => { return 1; }, }"),
    ("bool-match-unreachable", "compile",
     "proc f(x: bool) -> int { match x { _ => { return 0; }, true => { return 1; }, } }",
     ("E1602",), "type-check", "true"),
    ("match-duplicate-pattern-location", "compile",
     "proc f(value: i64) { match value { 1 => { return; }, 1 => { return; }, _ => { return; } } }",
     ("E1602",), "type-check", ("1", 1)),
    ("match-false-guard-location", "compile",
     "proc f(value: i64) { match value { _ if false => { return; }, _ => { return; } } }",
     ("E1602",), "type-check", "false"),
    ("contract-await-effect", "compile",
     "async proc ready() -> bool { return true; } "
     "async proc f() -> int requires await ready(); { return 0; }",
     ("E0710",), "contract", "await ready()"),
    ("constant-self-cycle", "compile",
     "const VALUE: int = VALUE; proc main() -> int { return 0; }",
     ("E1504",), "constant-eval", ("VALUE", 1)),
    ("match-pattern-type", "compile",
     "proc f(x: bool) -> int { match x { 1 => 1, _ => 0, } }",
     ("E1603",), "type-check", ("1", 0)),
    ("pick-match-missing-variant", "compile",
     "pick Maybe { None, Some(i64) } "
     "proc f(value: Maybe) -> i64 { match value { Maybe::None => 0, } }",
     ("E1601",), "type-check", "match value { Maybe::None => 0, }"),
    ("pick-match-duplicate-arm", "compile",
     "pick Maybe { None, Some(i64) } "
     "proc f(value: Maybe) -> i64 { match value { "
     "Maybe::None => 0, Maybe::None => 1, Maybe::Some(item) => item, } }",
     ("E1602",), "type-check", "Maybe::None", {"E1602": ("Maybe::None", 1)}),
    ("match-invalid-or-bindings", "compile",
     "pick Maybe { None, Some(i64) } "
     "proc f(value: Maybe) -> i64 { match value { "
     "Maybe::Some(item) | Maybe::None => 0, else => 1, } }",
     ("E1604",), "type-check", "item"),
    ("match-overlapping-ranges", "compile",
     "proc f(value: i64) -> i64 { match value { "
     "1..=5 => 0, 3..=8 => 1, _ => 2, } }",
     ("W0052",), "type-check", "3..=8"),
    ("defer-return-exit", "compile",
     "proc f() -> i64 { defer { return 1; } return 0; }",
     ("E1105",), "type-check", "return 1;"),
    ("defer-enclosing-loop-exit", "compile",
     "proc f() { loop { defer { break; } break; } }",
     ("E1105",), "type-check", "break;"),
    ("defer-captured-value-move", "compile",
     "proc f() { let value = 1; defer { let copy = value; } "
     "let moved = move value; }",
     ("E1208",), "type-check", "move value"),
    ("lexer-malformed-character-literal", "lex",
     "proc main() -> int { let x = 'a; return 0; }",
     ("E0111",), "lexer", "'a; return 0; }"),
    ("lexer-unterminated-string", "check",
     'proc main() -> int { let x = "unfinished; return 0; }',
     ("E0102",), "lexer", '"'),
    ("lexer-invalid-escape", "check",
     'proc main() -> int { let x = "bad\\q"; return 0; }',
     ("E0103",), "lexer", "\\q"),
    ("lexer-invalid-number", "check",
     "proc main() -> int { let x = 123abc; return 0; }",
     ("E0114",), "lexer", "123abc"),
    ("lexer-integer-overflow", "lex",
     "proc main() -> int { let x = 18446744073709551616; return 0; }",
     ("E0105",), "lexer", "18446744073709551616"),
    ("lexer-unterminated-comment", "lex",
     "proc main() -> int { /* never closed", ("E0106",), "lexer", "/*"),
    ("parser-expected-type", "check",
     "proc main() -> int { let x: = 1; return 0; }",
     ("E0272",), "parser", "="),
    ("parser-expected-expression", "check",
     "proc main() -> int { let x = ; return 0; }",
     ("E0272",), "parser", ";"),
    ("parser-unsupported-feature-keyword", "parse",
     "class Widget {}", ("E0210",), "parser", "class"),
    ("parser-unsupported-operator", "parse",
     "proc main() { @; }", ("E0210",), "parser", "@"),
    ("parser-return-outside-procedure", "parse",
     "return 1;", ("E0275",), "parser", "return"),
    ("parser-jump-outside-procedure", "parse",
     "break;", ("E0276",), "parser", "break"),
    ("parser-missing-module-path", "parse",
     "space ;", ("E0212",), "parser", ";"),
    ("parser-malformed-module-path", "parse",
     "space core::;", ("E0212",), "parser", ";"),
    ("parser-space-body-malformed-declaration", "parse",
     "space sample { proc () {} }", ("E0238",), "parser", "("),
    ("parser-missing-module-header", "parse",
     "use core;", ("E0223",), "parser", "<EOF>"),
    ("parser-missing-header-export", "parse",
     "export item;", ("E0223",), "parser", "<EOF>"),
    ("parser-duplicate-module-header", "parse",
     "space first;\nspace second;", ("E0213",), "parser", ("space", 1)),
    ("parser-misplaced-module-header", "parse",
     "proc before() {}\nspace late;", ("E0214",), "parser", "space"),
    ("parser-malformed-use", "parse",
     "use ;", ("E0215",), "parser", ";"),
    ("parser-invalid-use-alias", "parse",
     "use core as ;", ("E0216",), "parser", ";"),
    ("parser-malformed-import-group", "parse",
     "use core::{first second};", ("E0217",), "parser", "second"),
    ("parser-invalid-nested-glob", "parse",
     "use core::{outer::{*}};", ("E0218",), "parser", "*"),
    ("parser-invalid-top-level-glob", "parse",
     "use core::* as alias;", ("E0218",), "parser", "as"),
    ("parser-malformed-export", "parse",
     "export ;", ("E0219",), "parser", ";"),
    ("parser-empty-export-group", "parse",
     "export {};", ("E0220",), "parser", "}"),
    ("parser-invalid-export-alias", "parse",
     "export item as ;", ("E0221",), "parser", ";"),
    ("parser-trailing-use-tokens", "parse",
     "use core extra;", ("E0222",), "parser", "extra"),
    ("parser-trailing-export-tokens", "parse",
     "export item extra;", ("E0222",), "parser", "extra"),
    ("parser-const-missing-name", "parse",
     "const = 1;", ("E0224",), "parser", "="),
    ("parser-expected-declaration", "parse",
     "}", ("E0204",), "parser", "}"),
    ("parser-static-missing-terminator", "parse",
     "static mut COUNT = 1", ("E0224",), "parser", "<EOF>"),
    ("parser-const-missing-initializer", "parse",
     "const LIMIT = ;", ("E0225",), "parser", ";"),
    ("parser-type-missing-alias-target", "parse",
     "type Alias = ;", ("E0226",), "parser", ";"),
    ("parser-type-invalid-bound", "parse",
     "type Item: Trait + = int;", ("E0226",), "parser", "="),
    ("parser-opaque-missing-alias-target", "parse",
     "opaque type Handle = ;", ("E0227",), "parser", ";"),
    ("parser-opaque-invalid-bound", "parse",
     "opaque type Handle: Trait;", ("E0227",), "parser", ":"),
    ("parser-form-malformed-field", "parse",
     "form Point { value int }", ("E0228",), "parser", ("int", 1)),
    ("parser-form-duplicate-field", "parse",
     "form Point { value: int, value: bool }",
     ("E0229",), "parser", ("value", 1)),
    ("parser-form-missing-field-separator", "parse",
     "form Point { x: int y: int }", ("E0230",), "parser", "y"),
    ("parser-pick-malformed", "parse",
     "pick { A }", ("E0231",), "parser", "{"),
    ("parser-pick-duplicate-variant", "parse",
     "pick Choice { Some, Some }", ("E0232",), "parser", ("Some", 1)),
    ("parser-pick-invalid-payload", "parse",
     "pick Choice { Some(,) }", ("E0233",), "parser", ","),
    ("parser-trait-malformed", "parse",
     "trait Display", ("E0234",), "parser", "<EOF>"),
    ("parser-impl-malformed", "parse",
     "impl Point", ("E0235",), "parser", "<EOF>"),
    ("parser-impl-missing-target", "parse",
     "impl Display for {}", ("E0236",), "parser", "{"),
    ("parser-impl-invalid-for-clause", "parse",
     "impl Display for Point extra {}", ("E0237",), "parser", "extra"),
    ("parser-procedure-malformed", "parse",
     "proc () {}", ("E0238",), "parser", "("),
    ("parser-procedure-parameters", "parse",
     "proc f(x int) {}", ("E0239",), "parser", "int"),
    ("parser-procedure-missing-parameter-closer", "parse",
     "proc f(x: int {}", ("E0239",), "parser", "{"),
    ("parser-procedure-return-type", "parse",
     "proc f() -> {}", ("E0240",), "parser", "{"),
    ("parser-invalid-attribute-name", "parse",
     "#[] proc f() {}", ("E0257",), "parser", "]"),
    ("parser-invalid-attribute-arguments", "parse",
     "#[cfg(,)] proc f() {}", ("E0258",), "parser", ","),
    ("parser-missing-attribute-value", "parse",
     "#[cfg(enabled =)] proc f() {}", ("E0259",), "parser", ")"),
    ("parser-invalid-attribute-literal", "parse",
     "#[cfg(enabled = name)] proc f() {}", ("E0260",), "parser", "name"),
    ("parser-generic-parameter-list-missing-comma", "parse",
     "proc bad<T U>() {}", ("E0246",), "parser", "U"),
    ("parser-generic-bound-missing-type", "parse",
     "proc bad<T:>() {}", ("E0248",), "parser", ">"),
    ("parser-where-clause-missing-predicate", "parse",
     "proc bad<T>() where {}", ("E0249",), "parser", "{"),
    ("parser-where-invalid-relational-operator", "parse",
     "proc bad<T>() where T = 1 {}", ("E0249",), "parser", "="),
    ("parser-requires-missing-expression", "parse",
     "proc bad() requires ; {}", ("E0250",), "parser", ";"),
    ("parser-ensures-missing-terminator", "parse",
     "proc bad() ensures true {}", ("E0251",), "parser", "{"),
    ("parser-external-empty-abi", "parse",
     'extern "" proc bad();', ("E0245",), "parser", '""'),
    ("parser-intrinsic-missing-name", "parse",
     "intrinsic ();", ("E0244",), "parser", "("),
    ("parser-malformed-macro", "parse",
     "macro () {}", ("E0252",), "parser", "("),
    ("parser-malformed-macro-parameters", "parse",
     "macro bad(value int) {}", ("E0253",), "parser", "int"),
    ("parser-malformed-test-declaration", "parse",
     "test 1 {}", ("E0254",), "parser", "1"),
    ("parser-malformed-procedure-type", "parse",
     "type Bad = proc(int bool);", ("E0264",), "parser", "bool"),
    ("parser-malformed-reference-type", "parse",
     "type Bad = &;", ("E0265",), "parser", ";"),
    ("parser-malformed-pointer-type", "parse",
     "type Bad = *;", ("E0266",), "parser", ";"),
    ("parser-malformed-array-type", "parse",
     "type Bad = [int; ];", ("E0267",), "parser", "]"),
    ("parser-malformed-slice-type", "parse",
     "type Bad = [];", ("E0268",), "parser", "]"),
    ("parser-malformed-optional-type", "parse",
     "type Bad = ?;", ("E0269",), "parser", ";"),
    ("parser-malformed-type-path", "parse",
     "type Bad = Item::;", ("E0270",), "parser", ";"),
    ("parser-single-colon-type-path", "parse",
     "type Bad = Item: Other;", ("E0270",), "parser", ":"),
    ("parser-malformed-angle-type-arguments", "parse",
     "type Bad = Map<int bool>;", ("E0271",), "parser", "bool"),
    ("parser-malformed-bracket-type-arguments", "parse",
     "type Bad = Map[int bool];", ("E0271",), "parser", "bool"),
    ("parser-let-missing-pattern", "parse",
     "proc bad() { let = 1; }", ("E0272",), "parser", "="),
    ("parser-let-missing-type", "parse",
     "proc bad() { let value: ; }", ("E0272",), "parser", ";"),
    ("parser-let-missing-initializer", "parse",
     "proc bad() { let value = ; }", ("E0272",), "parser", ";"),
    ("parser-set-missing-operator", "parse",
     "proc bad() { set value; }", ("E0274",), "parser", ";"),
    ("parser-set-invalid-operator", "parse",
     "proc bad() { set value == 1; }", ("E0274",), "parser", "=="),
    ("parser-set-missing-value", "parse",
     "proc bad() { set value = ; }", ("E0273",), "parser", ";"),
    ("parser-return-malformed", "parse",
     "proc bad() { return 1 2; }", ("E0275",), "parser", "2"),
    ("parser-return-missing-terminator", "parse",
     "proc bad() { return 1 return 2; }",
     ("E0275",), "parser", ("return", 1)),
    ("parser-jump-malformed", "parse",
     "proc bad() { break value; }", ("E0276",), "parser", "value"),
    ("parser-let-missing-terminator", "parse",
     "proc bad() { let value = 1 }", ("E0272",), "parser", "}"),
    ("parser-set-missing-terminator", "parse",
     "proc bad() { set value = 1 }", ("E0273",), "parser", "}"),
    ("parser-jump-missing-terminator", "parse",
     "proc bad() { break }", ("E0276",), "parser", "}"),
    ("parser-malformed-defer", "parse",
     "proc bad() { defer; }", ("E0277",), "parser", ";"),
    ("parser-malformed-assert", "parse",
     "proc bad() { assert true; }", ("E0278",), "parser", "true"),
    ("parser-malformed-unsafe", "parse",
     "proc bad() { unsafe value; }", ("E0279",), "parser", "value"),
    ("parser-malformed-assembly", "parse",
     "proc bad() { asm; }", ("E0280",), "parser", ";"),
    ("parser-malformed-assembly-operands", "parse",
     "proc bad() { asm(,); }", ("E0311",), "parser", ","),
    ("parser-malformed-if", "parse",
     "proc bad() { if; }", ("E0281",), "parser", ";"),
    ("parser-malformed-elif", "parse",
     "proc bad() { if true {} elif; }", ("E0281",), "parser", ";"),
    ("parser-malformed-else", "parse",
     "proc bad() { if true {} else value; }", ("E0281",), "parser", "value"),
    ("parser-else-without-if", "parse",
     "proc bad() { else {} }", ("E0281",), "parser", "else"),
    ("parser-malformed-while", "parse",
     "proc bad() { while true; }", ("E0281",), "parser", ";"),
    ("parser-malformed-loop", "parse",
     "proc bad() { loop; }", ("E0281",), "parser", ";"),
    ("parser-malformed-for", "parse",
     "proc bad() { for item values {} }", ("E0281",), "parser", "values"),
    ("parser-malformed-match", "parse",
     "proc bad() { match; }", ("E0282",), "parser", ";"),
    ("parser-match-arm-missing-arrow", "parse",
     "proc bad() { match true { true false => 0, } }",
     ("E0283",), "parser", "false"),
    ("parser-match-arm-missing-guard", "parse",
     "proc bad() { match true { true if => 1, } }",
     ("E0283",), "parser", "=>"),
    ("parser-match-arm-missing-body", "parse",
     "proc bad() { match true { true => , } }",
     ("E0283",), "parser", ","),
    ("parser-match-arm-missing-separator", "parse",
     "proc bad() { match true { true => 1 false => 0, } }",
     ("E0283",), "parser", "false"),
    ("parser-malformed-tuple-pattern", "parse",
     "proc bad() { match true { (true,, false) => 1, } }",
     ("E0284",), "parser", (",", 1)),
    ("parser-malformed-array-pattern", "parse",
     "proc bad() { match true { [true,, false] => 1, } }",
     ("E0284",), "parser", (",", 1)),
    ("parser-malformed-variant-pattern", "parse",
     "proc bad() { match true { Some(,) => 1, } }",
     ("E0284",), "parser", ","),
    ("parser-malformed-path-pattern", "parse",
     "proc bad() { match true { Path:: => 1, } }",
     ("E0284",), "parser", "=>"),
    ("parser-malformed-struct-pattern", "parse",
     "proc bad() { match true { Point { x: } => 1, } }",
     ("E0284",), "parser", ("}", 0)),
    ("parser-malformed-struct-pattern-with-rest", "parse",
     "proc bad() { match true { Point { .., x: } => 1, } }",
     ("E0284",), "parser", ("}", 0)),
    ("parser-range-pattern-invalid-upper", "parse",
     "proc bad() { match value { 1..false => 0, } }",
     ("E0285",), "parser", "false"),
    ("parser-range-pattern-missing-upper", "parse",
     "proc bad() { match value { 1.. => 0, } }",
     ("E0285",), "parser", "=>"),
    ("parser-range-pattern-invalid-lower", "parse",
     "proc bad() { match value { item..4 => 0, } }",
     ("E0285",), "parser", "item"),
    ("parser-malformed-assignment-expression", "parse",
     "proc bad() { let value = target = ; }", ("E0286",), "parser", ";"),
    ("parser-malformed-coalescing-expression", "parse",
     "proc bad() { let value = target ?? ; }", ("E0287",), "parser", ";"),
    ("parser-malformed-logical-expression", "parse",
     "proc bad() { let value = target and ; }", ("E0288",), "parser", ";"),
    ("parser-malformed-bitwise-expression", "parse",
     "proc bad() { let value = target | ; }", ("E0289",), "parser", ";"),
    ("parser-malformed-comparison-expression", "parse",
     "proc bad() { let value = target == ; }", ("E0290",), "parser", ";"),
    ("parser-malformed-shift-expression", "parse",
     "proc bad() { let value = target << ; }", ("E0291",), "parser", ";"),
    ("parser-malformed-arithmetic-expression", "parse",
     "proc bad() { let value = target + ; }", ("E0292",), "parser", ";"),
    ("parser-malformed-range-expression", "parse",
     "proc bad() { let value = target .. ; }", ("E0293",), "parser", ";"),
    ("parser-malformed-unary-expression", "parse",
     "proc bad() { let value = -; }", ("E0294",), "parser", ";"),
    ("parser-malformed-cast-expression", "parse",
     "proc bad() { let value = target as ; }", ("E0295",), "parser", ";"),
    ("parser-cast-missing-value", "parse",
     "proc bad() { let value = as int; }", ("E0295",), "parser", "as"),
    ("parser-operator-placement-preserves-precedence", "parse",
     "proc bad() { let value = left + right * / divisor; }",
     ("E0292",), "parser", "/"),
    ("parser-malformed-call-suffix", "parse",
     "proc bad() { let value = invoke(1; }", ("E0296",), "parser", ";"),
    ("parser-malformed-index-suffix", "parse",
     "proc bad() { let value = items[]; }", ("E0296",), "parser", "]"),
    ("parser-malformed-field-suffix", "parse",
     "proc bad() { let value = item.; }", ("E0296",), "parser", ";"),
    ("parser-malformed-optional-postfix", "parse",
     "proc bad() { let value = ?item; }", ("E0296",), "parser", "?"),
    ("parser-empty-generic-suffix", "parse",
     "proc bad() { let value = Box[] { item: 1 }; }",
     ("E0271",), "parser", "]"),
    ("parser-malformed-form-initializer", "parse",
     "proc bad() { let value = Point { x: }; }",
     ("E0296",), "parser", "}"),
    ("parser-if-expression-requires-else", "parse",
     "proc bad() { let value = if ready { 1 }; }",
     ("E0297",), "parser", ";"),
    ("parser-if-expression-preserves-condition-error", "parse",
     "proc bad() { let value = if ready + ; { 1 } else { 0 }; }",
     ("E0292",), "parser", ";"),
    ("parser-block-expression-missing-closer", "parse",
     "proc bad() { let value = { 1 2 }; }", ("E0298",), "parser", "2"),
    ("parser-array-expression-missing-comma", "parse",
     "proc bad() { let value = [1 2]; }",
     ("E0311",), "parser", "2"),
    ("parser-array-expression-missing-closer", "parse",
     "const VALUES = [1",
     ("E0299", "E0224"), "parser", "<EOF>"),
    ("parser-sizeof-malformed", "parse",
     "proc bad() { let value = sizeof(); }",
     ("E0308",), "parser", (")", 1)),
    ("parser-alignof-malformed", "parse",
     "proc bad() { let value = alignof(); }",
     ("E0309",), "parser", (")", 1)),
    ("parser-null-construction-missing-closer", "parse",
     "proc bad() { let value = null(1; }",
     ("E0310",), "parser", ";"),
    ("parser-null-construction-malformed-list", "parse",
     "proc bad() { let value = null(,); }",
     ("E0311",), "parser", ","),
    ("parser-expression-list-empty-item", "parse",
     "proc bad() { let value = invoke(,); }",
     ("E0311",), "parser", ","),
    ("parser-expression-statement-missing-operand", "parse",
     "proc bad() { value + ; }", ("E0292",), "parser", ";"),
    ("let-without-type-or-initializer", "compile",
     "proc bad() { let value; }", ("E0502",), "type-check", "value"),
    ("empty-array-cannot-infer", "compile",
     "proc bad() { let values = []; }", ("E0502",), "type-check", "[]"),
    ("typed-let-without-initializer", "compile",
     "proc ok() { let value: int; }", (), "type-check", "let value;"),
    ("return-without-value-or-semicolon", "compile",
     "proc ok() { return }", (), "type-check", "return"),
    ("parser-procedure-modifier", "parse",
     "async async proc f() {}", ("E0241",), "parser", ("async", 1)),
    ("parser-misplaced-procedure-modifier", "parse",
     "async form Point {}", ("E0241",), "parser", "async"),
    ("parser-procedure-body-form", "parse",
     "proc f() return 1;", ("E0242",), "parser", "return"),
    ("parser-external-procedure-body-form", "parse",
     "extern proc f() {}", ("E0242",), "parser", "{"),
    ("parser-external-procedure-missing-semicolon", "parse",
     "extern proc f()", ("E0242",), "parser", "<EOF>"),
    ("lexer-invalid-character", "lex",
     "proc main() -> int { return 0; ` }", ("E0101",), "lexer", "`"),
    ("missing-module", "compile",
     "space contract::main;\nuse contract::absent;\n"
     "proc main() -> int { return 0; }\n",
     ("E0301",), "import", "contract::absent"),
    ("cli-malformed-import-path", "compile",
     "space contract::main;\nuse contract..absent;\n"
     "proc main() -> int { return 0; }\n",
     ("E0212",), "parser", "."),
    ("cli-reserved-import-path-segment", "compile",
     "space contract::main;\nuse return;\n"
     "proc main() -> int { return 0; }\n",
     ("E0212",), "parser", "return"),
    ("unknown-name", "compile",
     "proc main() -> int { return missing; }", ("E0401",), "name-resolution", "missing"),
    ("duplicate-declaration", "compile",
     "space contract::main;\nconst value: i64 = 1;\nconst value: i64 = 2;\n",
     ("E0402",), "name-resolution", ("value", 1)),
    ("duplicate-binding", "compile",
     "proc main() -> i64 { let x = 1; let x = 2; return x; }",
     ("E0402",), "name-resolution", ("x", 1)),
    ("duplicate-pattern-binding", "compile",
     "form Point { a: i64, b: i64 } "
     "proc main(point: Point) { let Point { a: x, b: x } = point; }",
     ("E0406",), "name-resolution", ("x", 1)),
    ("invalid-export-target", "compile",
     "space contract::main;\nexport missing as public_name;\n"
     "proc main() -> i64 { return 0; }\n",
     ("E0407",), "name-resolution", "missing"),
    ("export-outside-module-scope", "compile",
     "space contract::main;\nform Hidden {}\n"
     "impl Hidden { pub use core; }\n"
     "proc main() -> i64 { return 0; }\n",
     ("E0405",), "name-resolution", "use core;"),
    ("shadowed-global", "compile",
     "const VALUE: i64 = 1;\n"
     "proc main(VALUE: i64) -> i64 { return VALUE; }\n",
     ("W0030",), "name-resolution", ("VALUE: i64", 1)),
    ("incomplete-opaque-sizeof", "compile",
     "opaque Hidden; proc size(value: Hidden) -> i64 { return sizeof(value); }",
     ("E0508",), "type-check", ("value", 1)),
    ("recursive-type-alias", "compile",
     "type Loop = Loop;",
     ("E0509",), "type-check", ("Loop", 1)),
    ("unknown-member", "compile",
     "form Item { value: int; } proc main(item: Item) -> int { return item.missing; }",
     ("E0506",), "type-check", "missing"),
    ("local-type-mismatch", "compile",
     'proc main() -> int { let x: int = "bad"; return 0; }',
     ("E0501",), "type-check", '"bad"'),
    ("assignment-type-mismatch", "compile",
     'proc main() -> int { let mut x: int = 1; set x = "bad"; return 0; }',
     ("E0501",), "type-check", '"bad"'),
    ("invalid-cast", "compile",
     'proc main() -> int { let value = "text" as bool; return 0; }',
     ("E0503",), "type-check", '"text"'),
    ("type-name-used-as-value", "compile",
     "proc main() -> int { let value = i64; return 0; }",
     ("E0516",), "type-check", "i64"),
    ("generic-inference", "compile",
     "proc unused<T>() -> int { return 1; } "
     "proc main() -> int { return unused(); }",
     ("E0502",), "type-check", ("unused", 1)),
    ("generic-argument-count", "compile",
     "form Box<T> { value: T; } extern proc inspect(value: Box) -> i64;",
     ("E0802",), "type-check", ("Box", 1)),
    ("generic-argument-kind", "compile",
     "form Buffer<const N: usize> { value: i64; } "
     "extern proc inspect(value: Buffer<i64>) -> i64;",
     ("E0803",), "type-check", ("i64", 1)),
    ("generic-inference-conflict", "compile",
     "proc same<T>(left: T, right: T) { } "
     "proc main() { same(1, true); }",
     ("E0804",), "type-check", "true"),
    ("generic-bound-unsatisfied", "compile",
     "trait Marker { proc mark(); } "
     "proc needs<T: Marker>(value: T) { } "
     "proc main() { needs(1); }",
     ("E0801",), "type-check", "needs(1)"),
    ("missing-trait-member", "compile",
     "trait Marker { proc mark(); } form Item {} "
     "impl Marker for Item {}",
     ("E0813",), "type-check", "impl Marker for Item {}"),
    ("invalid-trait-member", "compile",
     "trait Marker { form Nested {} }",
     ("E0814",), "type-check", "form Nested {}"),
    ("invalid-impl-target", "compile",
     "form Item {} impl Item for Item {}",
     ("E0815",), "type-check", ("Item", 1)),
    ("invalid-where-predicate", "compile",
     "proc bounded<T>() where T == 1 { }",
     ("E0816",), "type-check", "T == 1"),
    ("conflicting-trait-impl", "compile",
     "trait Marker { proc mark(); } form Item {} "
     "impl Marker for Item { proc mark() {} } "
     "impl Marker for Item { proc mark() {} }",
     ("E0812",), "type-check", ("impl Marker for Item { proc mark() {} }", 1)),
    ("conflicting-generic-trait-impl", "compile",
     "trait Marker { proc mark(); } form Box<T> { value: T; } "
     "impl<T> Marker for Box<T> { proc mark() {} } "
     "impl<U> Marker for Box<U> { proc mark() {} }",
     ("E0812",), "type-check", "impl<U> Marker for Box<U> { proc mark() {} }"),
    ("ambiguous-generic-method", "compile",
     "trait ReadNumber { proc read() -> i64; } "
     "trait ReadFlag { proc read() -> bool; } "
     "proc inspect<T: ReadNumber + ReadFlag>(value: T) { "
     "let result = value.read(); }",
     ("E0817",), "type-check", ("read", 2)),
    ("not-callable", "compile",
     "proc main() -> int { let x = 1; return x(); }", ("E0504",), "type-check", ("x", 1)),
    ("not-indexable", "compile",
     "proc main() -> int { return 1[0]; }", ("E0505",), "type-check", "1"),
    ("unknown-type", "compile",
     "proc main(x: Missing) -> int { return 0; }", ("E0510",), "type-check", "Missing"),
    ("not-a-type", "compile",
     "const Value: int = 1; proc main(x: Value) -> int { return 0; }",
     ("E0511",), "type-check", ("Value", 1)),
    ("invalid-operands", "compile",
     "proc main() -> int { if 1 && 2 { return 1; } return 0; }",
     ("E0512",), "type-check", "1 && 2"),
    ("not-assignable", "compile",
     "proc main() -> int { let x = 1; set x = 2; return x; }",
     ("E0513",), "type-check", ("x", 1)),
    ("invalid-index-type", "compile",
     'proc main() -> int { let x = [1, 2]; return x["x"]; }',
     ("E0514",), "type-check", '"x"'),
    ("condition-not-bool", "compile",
     "proc main() -> int { if 1 { return 1; } return 0; }",
     ("E0515",), "type-check", "1"),
    ("condition-elif-not-bool", "compile",
     "proc check(flag: bool, count: i64) { if flag { let left = 0; } elif count { let right = 0; } }",
     ("E0515",), "type-check", ("count", 1)),
    ("condition-while-not-bool", "compile",
     "proc check() { while 1 { let value = 0; } }",
     ("E0515",), "type-check", "1"),
    ("condition-guard-not-bool", "compile",
     "proc check(value: i64) { match value { _ if 1 => { return; }, _ => { return; } } }",
     ("E0515",), "type-check", "1"),
    ("condition-assert-not-bool", "compile",
     "proc check() { assert(1); }", ("E0515",), "type-check", "1"),
    ("condition-if-expression-not-bool", "compile",
     "proc check() -> i64 { return if 1 { 1 } else { 2 }; }",
     ("E0515",), "type-check", "1"),
    ("call-arity", "compile",
     "proc f(x: int) -> int { return x; } proc main() -> int { return f(); }",
     ("E0601",), "type-check", "f()"),
    ("call-argument-type", "compile",
     'proc f(x: int) -> int { return x; } proc main() -> int { return f("x"); }',
     ("E0602",), "type-check", '"x"'),
    ("return-type", "compile",
     'proc main() -> i64 { return "x"; }', ("E0605",), "type-check", '"x"'),
    ("return-missing-value", "compile",
     'proc main() -> i64 { return; }', ("E0605",), "type-check", "return;"),
    ("return-value-from-void", "compile",
     'proc main() { return 1; }', ("E0605",), "type-check", "1"),
    ("return-unresolved-value", "compile",
     'proc main() -> int { return missing; }', ("E0401",), "name-resolution", "missing"),
    ("return-unresolved-type-no-missing", "compile",
     'proc main() -> Missing {}', ("E0510",), "type-check", "Missing"),
    ("return-unresolved-parameter-type", "compile",
     'proc main(value: Missing) -> i64 { return value; }',
     ("E0510",), "type-check", "Missing"),
    ("duplicate-return-unreachable", "compile",
     'proc main() -> i64 { return 1; return 2; }',
     ("W0020",), "type-check", "return 2;"),
    ("missing-return-declared-type", "compile",
     'proc value() -> i64 {}', ("E1101",), "type-check", "}"),
    ("parameter-refutable-pattern", "compile",
     'proc f(1: i64) {}', ("E0608",), "type-check", "1"),
    ("parameter-borrow-mode-unsupported", "compile",
     'proc f(borrow value: i64) {}', ("E0608",), "type-check", "borrow"),
    ("parameter-owned-mode-unsupported", "compile",
     'proc f(owned value: i64) {}', ("E0608",), "type-check", "owned"),
    ("immutable-parameter-assignment", "compile",
     'proc f(value: i64) { set value = 1; }', ("E0513",), "type-check", ("value", 1)),
    ("return-outside-procedure", "compile",
     'test "return outside procedure" { return 0; }',
     ("E0607",), "type-check", "return 0;"),
    ("utf8-before-error", "compile",
     'proc main() -> int {\n    let label = "é";\n    return "bad";\n}',
     ("E0605",), "type-check", '"bad"'),
    ("break-outside-loop", "compile",
     "proc main() -> int { break; return 0; }", ("E1102", "W0020"), "type-check", "break;",
     {"W0020": "return 0;"}),
    ("continue-outside-loop", "compile",
     "proc main() -> int { continue; return 0; }",
     ("E1103", "W0020"), "type-check", "continue;", {"W0020": "return 0;"}),
    ("unreachable-code-warning", "compile",
     "proc main() -> int { return 1; let x = 2; }",
     ("W0020",), "type-check", "let x = 2;"),
)

# Every nonterminal in the grammar's syntax section must have a malformed
# source witness in CASES. A witness can cover nested productions as well as
# its target production; the named case itself is still executed with an exact
# public code and source span by this contract test.
GRAMMAR_INVALID_PRODUCTION_CASES = {
    "translation_unit": "parser-missing-module-path",
    "declaration": "parser-const-missing-name",
    "declaration_modifier": "parser-procedure-modifier",
    "declaration_body": "parser-procedure-malformed",
    "attribute": "parser-invalid-attribute-name",
    "attribute_arguments": "parser-invalid-attribute-arguments",
    "attribute_argument": "parser-missing-attribute-value",
    "attribute_name": "parser-invalid-attribute-name",
    "attribute_literal": "parser-invalid-attribute-literal",
    "space_declaration": "parser-malformed-module-path",
    "space_body": "parser-space-body-malformed-declaration",
    "use_declaration": "parser-malformed-use",
    "import_items": "parser-malformed-import-group",
    "import_item": "parser-invalid-nested-glob",
    "path_alias": "parser-invalid-use-alias",
    "export_clause": "parser-malformed-export",
    "export_items": "parser-empty-export-group",
    "value_declaration": "parser-const-missing-initializer",
    "type_declaration": "parser-type-missing-alias-target",
    "opaque_declaration": "parser-opaque-missing-alias-target",
    "form_declaration": "parser-form-malformed-field",
    "form_field": "parser-form-malformed-field",
    "pick_declaration": "parser-pick-malformed",
    "pick_variant": "parser-pick-invalid-payload",
    "trait_declaration": "parser-trait-malformed",
    "implementation_declaration": "parser-impl-malformed",
    "declaration_block": "parser-trait-malformed",
    "procedure_declaration": "parser-procedure-malformed",
    "external_procedure_declaration": "parser-external-procedure-body-form",
    "intrinsic_declaration": "parser-intrinsic-missing-name",
    "contract_clause": "parser-requires-missing-expression",
    "macro_declaration": "parser-malformed-macro",
    "test_declaration": "parser-malformed-test-declaration",
    "generic_parameters": "parser-generic-parameter-list-missing-comma",
    "generic_parameter": "parser-generic-bound-missing-type",
    "where_clause": "parser-where-clause-missing-predicate",
    "where_predicate": "parser-where-invalid-relational-operator",
    "type_bound": "parser-generic-bound-missing-type",
    "relational_operator": "parser-where-invalid-relational-operator",
    "procedure_parameters": "parser-procedure-missing-parameter-closer",
    "procedure_parameter": "parser-procedure-parameters",
    "type": "parser-malformed-type-path",
    "procedure_type": "parser-malformed-procedure-type",
    "procedure_type_parameters": "parser-malformed-procedure-type",
    "procedure_type_parameter": "parser-malformed-procedure-type",
    "reference_type": "parser-malformed-reference-type",
    "pointer_type": "parser-malformed-pointer-type",
    "array_or_slice_type": "parser-malformed-array-type",
    "optional_type": "parser-malformed-optional-type",
    "path_type": "parser-malformed-type-path",
    "generic_arguments": "parser-malformed-angle-type-arguments",
    "module_path": "parser-malformed-module-path",
    "value_path": "parser-malformed-path-pattern",
    "path_segment": "parser-malformed-module-path",
    "block": "parser-malformed-if",
    "statement": "parser-malformed-if",
    "let_statement": "parser-let-missing-pattern",
    "set_statement": "parser-set-missing-value",
    "return_statement": "parser-return-malformed",
    "jump_statement": "parser-jump-malformed",
    "defer_statement": "parser-malformed-defer",
    "assert_statement": "parser-malformed-assert",
    "if_statement": "parser-malformed-if",
    "while_statement": "parser-malformed-while",
    "loop_statement": "parser-malformed-loop",
    "for_statement": "parser-malformed-for",
    "match_statement": "parser-malformed-match",
    "match_arm": "parser-match-arm-missing-arrow",
    "match_arm_body": "parser-match-arm-missing-body",
    "unsafe_statement": "parser-malformed-unsafe",
    "assembly_statement": "parser-malformed-assembly",
    "expression_statement": "parser-expression-statement-missing-operand",
    "pattern": "parser-malformed-tuple-pattern",
    "pattern_atom": "parser-malformed-array-pattern",
    "pattern_list": "parser-malformed-tuple-pattern",
    "range_pattern": "parser-range-pattern-missing-upper",
    "literal_pattern": "parser-range-pattern-invalid-upper",
    "pattern_literal": "parser-range-pattern-invalid-upper",
    "generic_variant_pattern": "parser-malformed-variant-pattern",
    "path_pattern": "parser-malformed-path-pattern",
    "pattern_path": "parser-malformed-path-pattern",
    "struct_pattern_items": "parser-malformed-struct-pattern-with-rest",
    "struct_pattern_item": "parser-malformed-struct-pattern",
    "expression": "parser-malformed-assignment-expression",
    "assignment_expression": "parser-malformed-assignment-expression",
    "assignment_operator": "parser-set-invalid-operator",
    "coalesce_expression": "parser-malformed-coalescing-expression",
    "logical_or_expression": "parser-malformed-logical-expression",
    "logical_and_expression": "parser-malformed-logical-expression",
    "bitwise_or_expression": "parser-malformed-bitwise-expression",
    "bitwise_xor_expression": "parser-malformed-bitwise-expression",
    "bitwise_and_expression": "parser-malformed-bitwise-expression",
    "equality_expression": "parser-malformed-comparison-expression",
    "comparison_expression": "parser-malformed-comparison-expression",
    "shift_expression": "parser-malformed-shift-expression",
    "additive_expression": "parser-malformed-arithmetic-expression",
    "multiplicative_expression": "parser-malformed-arithmetic-expression",
    "range_expression": "parser-malformed-range-expression",
    "unary_expression": "parser-malformed-unary-expression",
    "unary_operator": "parser-malformed-unary-expression",
    "cast_expression": "parser-malformed-cast-expression",
    "postfix_expression": "parser-malformed-call-suffix",
    "ordinary_postfix_suffix": "parser-malformed-index-suffix",
    "generic_form_or_variant_suffix": "parser-empty-generic-suffix",
    "primary_expression": "parser-array-expression-missing-closer",
    "sizeof_expression": "parser-sizeof-malformed",
    "alignof_expression": "parser-alignof-malformed",
    "if_expression": "parser-if-expression-requires-else",
    "if_expression_tail": "parser-if-expression-requires-else",
    "block_expression": "parser-block-expression-missing-closer",
    "block_expression_statement": "parser-block-expression-missing-closer",
    "if_statement_without_else": "parser-malformed-if",
    "array_expression": "parser-array-expression-missing-closer",
    "form_initializer": "parser-malformed-form-initializer",
    "form_initializer_field": "parser-malformed-form-initializer",
    "expression_list": "parser-expression-list-empty-item",
    "literal": "lexer-malformed-character-literal",
}


if len(sys.argv) == 2 and sys.argv[1] == "--export-fixtures":
    records = {
        case[0]: {
            "name": case[0],
            "command": case[1],
            "source": case[2],
            "codes": case[3],
            "phase": case[4],
            "marked": case[5],
        }
        for case in CASES
    }
    print(json.dumps({
        "schema": "vitte-diagnostic-fixtures@1",
        "positive": [
            {"name": "positive-client-main", "path": "vscode/VitteLangVsCode-main/test/fixtures/diagnostics/positive.vit"},
            {"name": "grammar-bindings", "path": "src/tests/grammar_alignment/let_mut_ok.vit"},
            {"name": "valid-contracts", "path": "src/tests/contracts_ok.vit"},
            {"name": "valid-imports", "path": "src/tests/import_visibility/module_ok.vit"},
            {"name": "native-runtime", "path": "src/tests/core_native_smoke_runtime.vit"},
        ],
        "grammar": [
            {"production": production, "case": case_name}
            for production, case_name in GRAMMAR_INVALID_PRODUCTION_CASES.items()
        ],
        "negative": list(records.values()),
    }, ensure_ascii=False, indent=2))
    sys.exit(0)


def check_grammar_invalid_production_coverage():
    grammar = (root / "grammar" / "vitte.ebnf").read_text(encoding="utf-8")
    syntax_grammar = grammar.split("(* Lexical forms *)", 1)[0]
    productions = set(re.findall(r"^\s*([A-Za-z_][A-Za-z0-9_]*)\s*::=", syntax_grammar, re.M))
    assert productions == set(GRAMMAR_INVALID_PRODUCTION_CASES), (
        "grammar production inventory changed",
        sorted(productions - set(GRAMMAR_INVALID_PRODUCTION_CASES)),
        sorted(set(GRAMMAR_INVALID_PRODUCTION_CASES) - productions),
    )
    records = {case[0]: case for case in CASES}
    missing_cases = set(GRAMMAR_INVALID_PRODUCTION_CASES.values()) - set(records)
    assert not missing_cases, ("missing invalid-input witnesses", sorted(missing_cases))
    for production, case_name in GRAMMAR_INVALID_PRODUCTION_CASES.items():
        case = records[case_name]
        assert case[1] in ("parse", "lex", "check", "compile"), (production, case)
        assert any(code.startswith("E") for code in case[3]), (production, case)


def check_grammar_lexical_production_mapping():
    grammar = (root / "grammar" / "vitte.ebnf").read_text(encoding="utf-8")
    lexical_grammar = grammar.split("(* Lexical forms *)", 1)[1]
    lexical_productions = set(
        re.findall(r"^\s*([A-Za-z_][A-Za-z0-9_]*)\s*::=", lexical_grammar, re.M)
    )
    audit = (root / "src" / "docs" / "diagnostic_coverage.md").read_text(encoding="utf-8")
    audit_rows = {
        match.group(1): match.group(0)
        for match in re.finditer(r"^\| `([^`]+)` \|[^\n]*$", audit, re.M)
    }
    assert lexical_productions <= set(audit_rows), (
        "canonical lexical productions need a diagnostic/status row",
        sorted(lexical_productions - set(audit_rows)),
    )
    for production in lexical_productions:
        cells = [cell.strip() for cell in audit_rows[production].strip("|").split("|")]
        assert len(cells) == 6 and cells[1] and cells[2] and cells[3] and cells[4], (
            production,
            audit_rows[production],
        )
        if cells[2] == "—":
            assert "accepted" in cells[5].lower() or "legacy" in cells[5].lower(), (
                production,
                audit_rows[production],
            )

TYPE_CONTEXTS = {
    "let-without-type-or-initializer": (None, None, "available information does not determine a unique type"),
    "empty-array-cannot-infer": (None, None, "available information does not determine a unique type"),
    "requires-not-bool": ("bool", "i64", "the contract expression must be valid and have type bool"),
    "ensures-not-bool": ("bool", "i64", "the contract expression must be valid and have type bool"),
    "local-type-mismatch": ("i64", "string", "the expression's actual type does not satisfy the expected type"),
    "assignment-type-mismatch": ("i64", "string", "the assigned value does not satisfy the target type"),
    "invalid-operands": ("bool", "i64", "the operator does not support these operand types"),
    "invalid-cast": ("bool", "string", "the source type cannot be converted to the requested target type"),
    "type-name-used-as-value": (None, "i64", "the name resolves to a type, which cannot be evaluated as a value"),
    "generic-inference": (None, None, "available information does not determine a unique type"),
    "invalid-index-type": (None, "string", "the index must have an integer type"),
    "condition-not-bool": ("bool", "i64", "the condition must be boolean"),
    "call-argument-type": ("i64", "string", "the argument does not satisfy the parameter type"),
    "return-type": ("i64", "string", "the returned value does not satisfy the declared return type"),
    "return-missing-value": ("i64", "void", "the returned value does not satisfy the declared return type"),
    "return-value-from-void": ("void", "i64", "the returned value does not satisfy the declared return type"),
}

# This inventory is the executable source-diagnostic surface of the C
# bootstrap frontend today. E0108 is exercised through the public C API in
# diagnostic_render_test.c because the CLI deliberately reports malformed
# files as the file-level E0006. Keeping this inventory explicit means a new
# source diagnostic cannot silently arrive without an exact source witness.
ACTIVE_BOOTSTRAP_SOURCE_CODES = {
    "E0006",  # malformed file encoding through the CLI
    "E0101", "E0102", "E0103", "E0104", "E0105", "E0106", "E0107", "E0108",
    "E0111", "E0114",
    "E0204", "E0210", "E0211",
    "E0212", "E0213", "E0214", "E0215", "E0216", "E0217",
    "E0218", "E0219", "E0220", "E0221", "E0222", "E0223",
    "E0224", "E0225", "E0226", "E0227", "E0228", "E0229", "E0230",
    "E0231", "E0232", "E0233", "E0234", "E0235", "E0236", "E0237",
    "E0238", "E0239", "E0240", "E0241", "E0242", "E0244", "E0245",
    "E0246", "E0248", "E0249", "E0250", "E0251", "E0252", "E0253", "E0254",
    "E0257", "E0258", "E0259", "E0260",
    "E0264", "E0265", "E0266", "E0267", "E0268", "E0269", "E0270", "E0271",
    "E0272", "E0273", "E0274", "E0275", "E0276",
    "E0277", "E0278", "E0279", "E0280", "E0281", "E0282", "E0283",
    "E0284", "E0285", "E0286", "E0287", "E0288", "E0289", "E0290",
    "E0291", "E0292", "E0293", "E0294", "E0295", "E0296", "E0297", "E0298", "E0299",
    "E0300", "E0301", "E0303", "E0304", "E0305",
    "E0308", "E0309", "E0310", "E0311",
    "E0401", "E0402", "E0403", "E0404", "E0405", "E0406", "E0407",
    "E0501", "E0502", "E0503", "E0504", "E0505", "E0506", "E0510", "E0511", "E0512",
    "E0508", "E0509", "E0513", "E0514", "E0515", "E0516",
    "E0601", "E0602", "E0605", "E0607", "E0608",
    "E0801", "E0802", "E0803", "E0804", "E0812", "E0813", "E0814", "E0815", "E0816", "E0817",
    "E0701", "E0702", "E0704", "E0705", "E0706", "E0707", "E0709", "E0710",
    "E1201", "E1501", "E1502", "E1503", "E1504", "E1506", "E1601", "E1602", "E1603",
    "E1101", "E1102", "E1103", "E1105", "E1208", "E1604",
    "W0020", "W0030", "W0052",
}


def invoke(path: Path, command: str, output_format: str):
    args = [
        str(binary), command, str(path), "--quiet", "--no-color",
        f"--diagnostic={output_format}",
    ]
    if command == "compile":
        args.append("--stop-after-sema")
    return subprocess.run(args, capture_output=True, text=True, timeout=20)


def check_case(directory: Path, case):
    name, command, source, expected_codes, expected_phase, marked = case[:6]
    additional_spans = case[6] if len(case) > 6 else {}
    source_prefix_length = 0
    if not source.lstrip().startswith("space ") and name not in (
        "parser-missing-module-header",
        "parser-missing-header-export",
        "parser-misplaced-module-header",
    ):
        source = "space diagnostic_contract_case\n" + source
        source_prefix_length = len("space diagnostic_contract_case\n")
    path = directory / f"{name}.vit"
    # Keep the bytes used by the span assertions identical on every host.
    # Path.write_text(..., newline=None) translates LF to CRLF on Windows.
    path.write_bytes(source.encode("utf-8"))
    result = invoke(path, command, "json")
    expects_error = any(code.startswith("E") for code in expected_codes)
    expected_status = 1 if expects_error else 0
    assert result.returncode == expected_status, (
        name,
        result.returncode,
        result.stdout,
        result.stderr,
    )
    if not expected_codes:
        assert not result.stdout and not result.stderr, (name, result.stdout, result.stderr)
        return
    assert not result.stdout, (name, result.stdout)
    try:
        document = json.loads(result.stderr)
    except json.JSONDecodeError as error:
        raise AssertionError((name, "compiler did not return JSON", result.stderr)) from error
    diagnostics = document["diagnostics"]
    assert tuple(item["code"] for item in diagnostics) == expected_codes, (
        name,
        expected_codes,
        diagnostics,
    )
    for diagnostic in diagnostics:
        expected_severity = "warning" if diagnostic["code"].startswith("W") else "error"
        diagnostic_marked = additional_spans.get(diagnostic["code"], marked)
        marker, occurrence = (
            diagnostic_marked if isinstance(diagnostic_marked, tuple)
            else (diagnostic_marked, 0)
        )
        if marker == "<EOF>":
            expected_start = expected_end = len(source.encode("utf-8"))
            expected_start_line = expected_end_line = source.count("\n")
            expected_start_column = expected_end_column = len(
                source.rsplit("\n", 1)[-1]
            )
        else:
            marker_positions = [
                source_prefix_length + match.start()
                for match in re.finditer(
                    re.escape(marker), source[source_prefix_length:]
                )
            ]
            assert occurrence < len(marker_positions), (name, marker, marker_positions)
            expected_start = len(source[:marker_positions[occurrence]].encode("utf-8"))
            expected_end = expected_start + len(marker.encode("utf-8"))
            expected_start_line = source[:marker_positions[occurrence]].count("\n")
            expected_start_column = len(
                source[:marker_positions[occurrence]].rsplit("\n", 1)[-1]
            )
            marker_end = marker_positions[occurrence] + len(marker)
            expected_end_line = source[:marker_end].count("\n")
            expected_end_column = len(source[:marker_end].rsplit("\n", 1)[-1])
        position = diagnostic["position"]
        assert position["has_span"], (name, diagnostic)
        assert position["source"] == str(path), (name, position)
        assert (position["start_byte"], position["end_byte"]) == (
            expected_start,
            expected_end,
        ), (name, expected_start, expected_end, position)
        assert (position["start_line"], position["start_column"]) == (
            expected_start_line,
            expected_start_column,
        ), (name, expected_start_line, expected_start_column, position)
        assert (position["end_line"], position["end_column"]) == (
            expected_end_line,
            expected_end_column,
        ), (name, expected_end_line, expected_end_column, position)
        phase = "type-check" if name == "multiple-contract-errors" and diagnostic["code"] == "E0605" else expected_phase
        assert diagnostic["phase"] == phase, (name, diagnostic)
        assert diagnostic["severity"] == expected_severity, (name, diagnostic)
        assert diagnostic["internal_code"] and diagnostic["category"], (name, diagnostic)
        assert diagnostic["message"].strip(), (name, diagnostic)
        if name in ("parser-trailing-use-tokens", "parser-trailing-export-tokens"):
            assert not diagnostic.get("suggestions"), (name, diagnostic["suggestions"])
        if name == "parser-array-expression-missing-comma":
            suggestions = diagnostic["suggestions"]
            assert len(suggestions) == 1, diagnostic
            suggestion = suggestions[0]
            assert suggestion["replacement"] == ",", diagnostic
            assert suggestion["applicability"] == "maybe-incorrect", diagnostic
            assert suggestion["span"]["start_byte"] == suggestion["span"]["end_byte"] == expected_start, diagnostic
        if name.startswith(("requires-", "ensures-")) or name in ("call-precondition", "return-postcondition"):
            contract = diagnostic["contract"]
            assert contract and contract["procedure"] == "f", (name, contract)
            assert contract["kind"] == ("ensures" if name.startswith("ensures-") or name == "return-postcondition" else "requires"), (name, contract)
            assert contract["expression"] in source, (name, contract)
            assert diagnostic["context"]["procedure"] == "f", (name, diagnostic)
            if name == "ensures-local-unavailable":
                help_text = " ".join(annotation["message"] for annotation in diagnostic["annotations"] if annotation["kind"] == "help")
                assert "local variables from the procedure body are unavailable" in help_text, diagnostic
        if name in TYPE_CONTEXTS:
            expected, actual, cause = TYPE_CONTEXTS[name]
            context = diagnostic["context"]
            expected_value = expected if expected is not None else ""
            actual_value = actual if actual is not None else ""
            assert context["expected_type"] == context["expected"] == expected_value, (
                name,
                context,
            )
            assert context["actual_type"] == context["found"] == actual_value, (name, context)
            assert context["probable_cause"] == cause, (name, context)
            if name == "call-argument-type":
                assert "argument 1" in context["context"], (name, context)
                assert "parameter `x`" in context["context"], (name, context)
            if name == "generic-inference":
                assert context["context"] == "type inference / unresolved expression or binding", (name, context)
        if name == "missing-module":
            assert "search roots checked in order" in diagnostic["details"], diagnostic
            assert "<module path>.vit" in diagnostic["details"], diagnostic

        primary = [label for label in diagnostic["labels"] if label["style"] == "primary"]
        assert len(primary) == 1, (name, diagnostic["labels"])
        primary_span = primary[0]["span"]
        assert primary[0]["message"].strip(), (name, primary[0])
        if name == "missing-module":
            assert primary[0]["message"] == (
                "module path `contract::absent` was not found"
            ), primary[0]
        assert primary_span["source"] == str(path), (name, primary_span)
        assert (primary_span["start_byte"], primary_span["end_byte"]) == (
            expected_start,
            expected_end,
        ), (name, primary_span)

        if name in ("parser-duplicate-module-header", "parser-misplaced-module-header"):
            related = diagnostic["related_locations"]
            assert len(related) == 1, (name, diagnostic)
            expected_label, related_text = (
                ("first module header declared here", "space first;")
                if name == "parser-duplicate-module-header"
                else ("preceding top-level declaration is here", "proc before() {}")
            )
            related_start = len(source[:source.index(related_text)].encode("utf-8"))
            related_end = related_start + len(related_text.encode("utf-8"))
            assert related[0]["label"] == expected_label, (name, related)
            assert related[0]["span"]["source"] == str(path), (name, related)
            assert (
                related[0]["span"]["start_byte"],
                related[0]["span"]["end_byte"],
            ) == (related_start, related_end), (name, related)

        if name == "parser-pick-duplicate-variant":
            related = diagnostic["related_locations"]
            assert len(related) == 1, (name, diagnostic)
            assert related[0]["label"] == "first variant declared here", related
            related_start = len(source[:source.index("Some")].encode("utf-8"))
            related_end = related_start + len("Some".encode("utf-8"))
            assert related[0]["span"]["source"] == str(path), related
            assert (
                related[0]["span"]["start_byte"],
                related[0]["span"]["end_byte"],
            ) == (related_start, related_end), (name, related)

        if name == "parser-form-duplicate-field":
            related = diagnostic["related_locations"]
            assert len(related) == 1, (name, diagnostic)
            assert related[0]["label"] == "first form field declared here", related
            related_start = len(source[:source.index("value")].encode("utf-8"))
            related_end = related_start + len("value".encode("utf-8"))
            assert related[0]["span"]["source"] == str(path), related
            assert (
                related[0]["span"]["start_byte"],
                related[0]["span"]["end_byte"],
            ) == (related_start, related_end), (name, related)

        related_expectations = {
            "pick-match-missing-variant": (
                "all variants are declared here", "pick Maybe { None, Some(i64) }"),
            "pick-match-duplicate-arm": (
                "earlier arm already covers this pattern", ("Maybe::None", 0)),
            "match-invalid-or-bindings": (
                "first alternative establishes the bindings", "Maybe::Some"),
            "match-overlapping-ranges": (
                "earlier pattern overlaps this range", "1..=5"),
            "defer-return-exit": (
                "deferred action is registered here", "defer { return 1; }"),
            "defer-enclosing-loop-exit": (
                "deferred action is registered here", "defer { break; }"),
            "defer-captured-value-move": (
                "deferred action captures this value here",
                "defer { let copy = value; }"),
            "return-type": (
                "declared return type is here", "i64"),
            "return-missing-value": (
                "declared return type is here", "i64"),
            "missing-return-declared-type": (
                "declared return type is here", "i64"),
            "parameter-borrow-mode-unsupported": (
                "parameter type declared here", "i64"),
            "incomplete-opaque-sizeof": (
                "opaque type declared here", "opaque Hidden;"),
            "unknown-member": (
                "receiver type declared here", "form Item { value: int; }"),
            "generic-argument-count": (
                "generic parameters declared here",
                "form Box<T> { value: T; }"),
            "generic-argument-kind": (
                "const generic parameter declared here", "const N: usize"),
            "generic-inference-conflict": (
                "generic parameter whose substitution conflicts", "T"),
            "generic-bound-unsatisfied": (
                "required trait bound introduced here", ("Marker", 1)),
            "missing-trait-member": (
                "required trait method declared here", "proc mark();"),
            "conflicting-trait-impl": (
                "conflicting implementation declared here",
                ("impl Marker for Item { proc mark() {} }", 0)),
            "conflicting-generic-trait-impl": (
                "conflicting implementation declared here",
                "impl<T> Marker for Box<T> { proc mark() {} }"),
            "ambiguous-generic-method": (
                "other applicable method candidate declared here",
                "proc read() -> i64;"),
        }
        if name in related_expectations:
            related = diagnostic["related_locations"]
            expected_label, related_marker = related_expectations[name]
            related_marker, related_occurrence = (
                related_marker if isinstance(related_marker, tuple)
                else (related_marker, 0)
            )
            related_positions = [
                match.start() for match in re.finditer(
                    re.escape(related_marker), source[source_prefix_length:]
                )
            ]
            assert len(related) == 1, (name, diagnostic)
            assert related[0]["label"] == expected_label, (name, related)
            assert related[0]["span"]["source"] == str(path), (name, related)
            related_start = len(source[:source_prefix_length + related_positions[
                related_occurrence
            ]].encode("utf-8"))
            related_end = related_start + len(related_marker.encode("utf-8"))
            assert (
                related[0]["span"]["start_byte"],
                related[0]["span"]["end_byte"],
            ) == (related_start, related_end), (name, related)

        control_related_expectations = {
            "return-outside-procedure": (
                "E0607", "enclosing test is here; return requires a procedure",
                'test "return outside procedure" { return 0; }', 0),
            "break-outside-loop": (
                "E1102", "enclosing procedure is here; place a loop here",
                "proc main() -> int { break; return 0; }", 0),
            "continue-outside-loop": (
                "E1103", "enclosing procedure is here; place a loop here",
                "proc main() -> int { continue; return 0; }", 0),
            "unreachable-after-branches": (
                "W0020", "preceding statement exits control flow here",
                "if x { return 1; } else { return 2; }", 0),
            "duplicate-return-unreachable": (
                "W0020", "preceding statement exits control flow here",
                "return 1;", 0),
            "unreachable-code-warning": (
                "W0020", "preceding statement exits control flow here",
                "return 1;", 0),
            "unreachable-constant-if-branch": (
                "W0020", "this condition makes the branch unreachable",
                "false", 0),
            "unreachable-constant-while-body": (
                "W0020", "this condition makes the loop body unreachable",
                "false", 0),
            "unreachable-constant-expression-if-branch": (
                "W0020", "this condition makes the branch unreachable",
                "1 < 2", 0),
            "unreachable-constant-expression-while-body": (
                "W0020", "this condition makes the loop body unreachable",
                "2 > 3", 0),
            "unreachable-constant-elif-branch": (
                "W0020", "this condition makes the branch unreachable",
                "true", 0),
            "bool-match-unreachable": (
                "E1602", "earlier arm already covers this pattern", "_", 0),
            "match-duplicate-pattern-location": (
                "E1602", "earlier arm already covers this pattern", "1", 0),
            "match-false-guard-location": (
                "E1602", "this pattern is disabled by the guard", "_", 0),
        }
        if name in control_related_expectations:
            expected_code, expected_label, marker, occurrence = \
                control_related_expectations[name]
            if diagnostic["code"] == expected_code:
                related = diagnostic["related_locations"]
                assert len(related) == 1, (name, diagnostic)
                assert related[0]["label"] == expected_label, (name, related)
                assert related[0]["span"]["source"] == str(path), (name, related)
                related_positions = [
                    source_prefix_length + match.start()
                    for match in re.finditer(
                        re.escape(marker), source[source_prefix_length:]
                    )
                ]
                assert occurrence < len(related_positions), (
                    name, marker, related_positions)
                related_start = len(
                    source[:related_positions[occurrence]].encode("utf-8"))
                related_end = related_start + len(marker.encode("utf-8"))
                assert (
                    related[0]["span"]["start_byte"],
                    related[0]["span"]["end_byte"],
                ) == (related_start, related_end), (name, related)

        help_annotations = [
            annotation for annotation in diagnostic["annotations"]
            if annotation["kind"] == "help"
        ]
        assert len(help_annotations) == 1, (name, diagnostic["annotations"])
        help_text = help_annotations[0]["message"]
        assert len(help_text.strip()) >= 12, (name, help_text)
        if name == "generic-inference":
            assert "generic arguments" in help_text, (name, help_text)
        if name in ("return-type", "return-missing-value"):
            assert "declared return type `i64`" in help_text, (name, help_text)
        if name == "missing-return-declared-type":
            assert "declares return type `i64`" in help_text, (name, help_text)
        if name == "immutable-parameter-assignment":
            assert "Declare it as `mut value`" in help_text, (name, help_text)
        if name == "generic-argument-count":
            assert "expects 1 generic argument" in help_text, (name, help_text)
        if name == "generic-argument-kind":
            assert "const parameter" in help_text and "type argument" in help_text, (name, help_text)
        if name == "generic-inference-conflict":
            assert "earlier argument" in help_text and "different type" in help_text, (name, help_text)
        if name == "generic-bound-unsatisfied":
            assert "Marker" in diagnostic["details"] and "i64" in diagnostic["details"], diagnostic
            assert diagnostic["context"]["expected_type"] == "Marker", diagnostic
            assert diagnostic["context"]["actual_type"] == "i64", diagnostic
            assert "concrete type" in help_text, (name, help_text)
        if name == "missing-trait-member":
            assert "mark" in help_text, (name, help_text)
        if name in ("conflicting-trait-impl", "conflicting-generic-trait-impl"):
            assert "already have an implementation" in help_text, (name, help_text)
        if name == "ambiguous-generic-method":
            assert "multiple applicable trait bounds" in help_text, (name, help_text)
        if name == "empty-array-cannot-infer":
            assert "ElementType" in help_text, (name, help_text)


def check_valid_contracts_and_returns(directory: Path):
    sources = (
        "proc f(x: int) -> int requires x > 0; ensures result > 0; { return x; }",
        "proc f() requires true; ensures true; { }",
        "proc f(x: bool) -> int { if x { return 1; } else { return 2; } }",
        "proc f() -> int { if true { return 1; } }",
        "proc f() -> int { loop { return 1; } }",
        "proc f() -> int { while true { return 1; } }",
        "proc f() -> int { { return 1; } }",
        "proc f(x: bool) -> int { if x { return 1; } return 2; }",
        "extern proc f() -> int;",
        "proc f() -> int { let result = 1; return result; }",
        "proc f(mut value: i64) { set value = 1; }",
        "proc f(move value: i64) { }",
        "trait ReadA { proc read() -> i64; } "
        "trait ReadB { proc read() -> i64; } "
        "proc inspect<T: ReadA + ReadB>(value: T) { let result = value.read(); }",
    )
    for index, source in enumerate(sources):
        path = directory / f"valid-contract-flow-{index}.vit"
        path.write_text(f"space diagnostics::valid_contract_flow_{index}\n" + source, encoding="utf-8")
        result = invoke(path, "compile", "json")
        assert result.returncode == 0, (source, result.stderr)
        assert not result.stdout and not result.stderr, (source, result)


def check_invalid_utf8(directory: Path):
    path = directory / "invalid-utf8.vit"
    valid_prefix = b"proc main() -> int { return 0; }\n"
    path.write_bytes(valid_prefix + b"\xff")
    result = invoke(path, "lex", "json")
    assert result.returncode == 1, (result.returncode, result.stdout, result.stderr)
    document = json.loads(result.stderr)
    assert len(document["diagnostics"]) == 1, document
    diagnostic = document["diagnostics"][0]
    assert diagnostic["code"] == "E0006", diagnostic
    assert diagnostic["phase"] == "lexer", diagnostic
    assert diagnostic["severity"] == "error", diagnostic
    assert f"byte {len(valid_prefix)}" in diagnostic["details"], diagnostic
    position = diagnostic["position"]
    assert position["source"] == str(path), position
    assert position["has_span"], position
    assert (position["start_byte"], position["end_byte"]) == (
        len(valid_prefix), len(valid_prefix) + 1,
    ), position
    assert (position["start_line"], position["start_column"]) == (1, 0), position
    assert (position["end_line"], position["end_column"]) == (1, 1), position
    assert diagnostic["context"]["context"] == "source encoding", diagnostic
    assert diagnostic["context"]["procedure"] == "source file", diagnostic
    primary = [label for label in diagnostic["labels"] if label["style"] == "primary"]
    assert len(primary) == 1, diagnostic["labels"]
    assert (primary[0]["span"]["start_byte"], primary[0]["span"]["end_byte"]) == (
        len(valid_prefix), len(valid_prefix) + 1,
    ), primary
    helps = [item for item in diagnostic["annotations"] if item["kind"] == "help"]
    assert len(helps) == 1 and len(helps[0]["message"].strip()) >= 12, helps


def check_invalid_unicode_module_paths(directory: Path):
    path = directory / "invalid-module-path-utf8.vit"
    source = (
        b"space import_unicode::main;\n"
        b"use import_unicode::bad\xffname;\n"
        b"proc main() -> i64 { return 0; }\n"
    )
    path.write_bytes(source)
    result = invoke(path, "compile", "json")
    assert result.returncode == 1, (result.returncode, result.stdout, result.stderr)
    document = json.loads(result.stderr)
    assert [item["code"] for item in document["diagnostics"]] == ["E0006"], document
    diagnostic = document["diagnostics"][0]
    assert diagnostic["phase"] == "lexer", diagnostic
    start = source.index(b"\xff")
    assert f"byte {start}" in diagnostic["details"], diagnostic
    assert diagnostic["position"]["source"] == str(path), diagnostic
    assert (diagnostic["position"]["start_byte"], diagnostic["position"]["end_byte"]) == (
        start, start + 1,
    ), diagnostic
    primary = [label for label in diagnostic["labels"] if label["style"] == "primary"]
    assert len(primary) == 1 and "invalid UTF-8 sequence" in primary[0]["message"], diagnostic

    scalar_source = (
        "space import_unicode::main;\n"
        "use import_unicode::bad😀name;\n"
        "proc main() -> i64 { return 0; }\n"
    ).encode("utf-8")
    path.write_bytes(scalar_source)
    result = invoke(path, "compile", "json")
    assert result.returncode == 1, (result.returncode, result.stdout, result.stderr)
    document = json.loads(result.stderr)
    assert [item["code"] for item in document["diagnostics"]] == ["E0107"], document
    diagnostic = document["diagnostics"][0]
    assert diagnostic["phase"] == "lexer", diagnostic
    assert "U+1F600" in diagnostic["details"], diagnostic
    scalar = "😀".encode("utf-8")
    start = scalar_source.index(scalar)
    assert (diagnostic["position"]["start_byte"], diagnostic["position"]["end_byte"]) == (
        start, start + len(scalar),
    ), diagnostic


def check_private_import(directory: Path):
    module_dir = directory / "imports" / "sample"
    module_dir.mkdir(parents=True)
    library = module_dir / "lib.vit"
    library_source = (
        "space sample::lib;\n"
        "const private_value: i64 = 1;\n"
        "export const public_value: i64 = 2;\n"
    )
    library.write_bytes(library_source.encode("utf-8"))
    path = module_dir.parent / "main.vit"
    source = (
        "space sample::main;\n"
        "use sample::lib;\n"
        "proc main() -> i64 { return lib::private_value; }\n"
    )
    path.write_bytes(source.encode("utf-8"))
    result = invoke(path, "compile", "json")
    assert result.returncode == 1, (result.returncode, result.stdout, result.stderr)
    document = json.loads(result.stderr)
    assert [item["code"] for item in document["diagnostics"]] == ["E0404"], document
    diagnostic = document["diagnostics"][0]
    assert diagnostic["phase"] == "name-resolution", diagnostic
    assert diagnostic["severity"] == "error", diagnostic
    assert diagnostic["internal_code"] and diagnostic["category"], diagnostic
    start = len(source[:source.index("private_value")].encode("utf-8"))
    end = start + len("private_value".encode("utf-8"))
    assert (diagnostic["position"]["start_byte"], diagnostic["position"]["end_byte"]) == (
        start,
        end,
    ), diagnostic
    primary = [label for label in diagnostic["labels"] if label["style"] == "primary"]
    assert len(primary) == 1, diagnostic["labels"]
    assert (primary[0]["span"]["start_byte"], primary[0]["span"]["end_byte"]) == (
        start,
        end,
    ), primary
    assert len(diagnostic["related_locations"]) == 1, diagnostic
    related = diagnostic["related_locations"][0]
    assert related["label"] == "private declaration is here", related
    assert related["span"]["source"] == str(library), related
    declaration = "const private_value: i64 = 1;"
    assert (related["span"]["start_byte"], related["span"]["end_byte"]) == (
        len("space sample::lib;\n".encode("utf-8")),
        len(("space sample::lib;\n" + declaration).encode("utf-8")),
    ), related
    helps = [item for item in diagnostic["annotations"] if item["kind"] == "help"]
    assert len(helps) == 1 and "private_value" in helps[0]["message"], helps
    assert "pub" in helps[0]["message"] and "export" in helps[0]["message"], helps


def check_declaration_origins(directory: Path):
    root = directory / "declaration-origins"
    root.mkdir()
    cases = [
        (
            "duplicate-declaration",
            "space sample::main;\nconst ITEM: i64 = 1;\nconst ITEM: i64 = 2;\n",
            "E0402",
            "conflicting declaration is here",
            "const ITEM: i64 = 1;",
        ),
        (
            "duplicate-binding",
            "proc main() -> i64 { let value = 1; let value = 2; return value; }\n",
            "E0402",
            "conflicting declaration is here",
            "let value = 1;",
        ),
        (
            "shadowed-global",
            "const VALUE: i64 = 1;\nproc main(VALUE: i64) -> i64 { return VALUE; }\n",
            "W0030",
            "earlier shadowed declaration is here",
            "const VALUE: i64 = 1;",
        ),
    ]
    for name, source, code, label, previous in cases:
        path = root / f"{name}.vit"
        path.write_text(source, encoding="utf-8")
        result = invoke(path, "compile", "json")
        expected_status = 1 if code.startswith("E") else 0
        assert result.returncode == expected_status, (result.returncode, result.stdout, result.stderr)
        document = json.loads(result.stderr)
        diagnostic = next(item for item in document["diagnostics"] if item["code"] == code)
        related = diagnostic["related_locations"]
        assert len(related) == 1 and related[0]["label"] == label, diagnostic
        assert related[0]["span"]["source"] == str(path), diagnostic
        previous_start = len(source[:source.index(previous)].encode("utf-8"))
        previous_length = len(previous.encode("utf-8"))
        assert (
            related[0]["span"]["start_byte"],
            related[0]["span"]["end_byte"],
        ) == (previous_start, previous_start + previous_length), diagnostic


def check_invalid_reexport(directory: Path):
    root = directory / "invalid-reexport"
    module_dir = root / "sample"
    module_dir.mkdir(parents=True)
    library = module_dir / "lib.vit"
    library.write_text(
        "space sample::lib;\nexport const VALUE: i64 = 1;\n",
        encoding="utf-8",
    )
    path = root / "main.vit"
    path.write_text(
        "space sample::main;\n"
        "use sample::lib::{VALUE};\n"
        "export VALUE as public_value;\n"
        "proc main() -> i64 { return 0; }\n",
        encoding="utf-8",
    )
    result = invoke(path, "compile", "json")
    assert result.returncode == 1, (result.returncode, result.stdout, result.stderr)
    document = json.loads(result.stderr)
    diagnostic = next(
        item for item in document["diagnostics"] if item["code"] == "E0407")
    assert diagnostic["internal_code"] == "VITTE_RESOLVE_E_INVALID_EXPORT", diagnostic
    assert diagnostic["phase"] == "name-resolution", diagnostic
    assert diagnostic["position"]["source"] == str(path), diagnostic
    assert diagnostic["related_locations"], diagnostic
    related = diagnostic["related_locations"][0]
    assert related["label"] == "imported declaration originates here", related
    assert related["span"]["source"] == str(library), related
    help_messages = [
        item["message"] for item in diagnostic["annotations"]
        if item["kind"] == "help"
    ]
    assert len(help_messages) == 1 and "re-exporting" in help_messages[0].lower(), diagnostic
    assert "use module::{name};" in help_messages[0], help_messages

    qualified_source = (
        "space sample::main;\n"
        "use sample::lib;\n"
        "export lib::MISSING;\n"
        "proc main() -> i64 { return 0; }\n"
    )
    path.write_text(qualified_source, encoding="utf-8")
    result = invoke(path, "compile", "json")
    assert result.returncode == 1, (result.returncode, result.stdout, result.stderr)
    document = json.loads(result.stderr)
    diagnostic = next(
        item for item in document["diagnostics"] if item["code"] == "E0407")
    assert diagnostic["position"]["start_byte"] == len(
        "space sample::main;\nuse sample::lib;\nexport lib::".encode("utf-8")
    ), diagnostic
    related = diagnostic["related_locations"][0]
    assert related["label"] == "imported module is bound here", diagnostic
    import_name_start = len(
        qualified_source[:qualified_source.index("lib;", qualified_source.index("use"))].encode("utf-8")
    )
    assert (related["span"]["start_byte"], related["span"]["end_byte"]) == (
        import_name_start,
        import_name_start + len("lib"),
    ), diagnostic
    help_messages = [
        item["message"] for item in diagnostic["annotations"]
        if item["kind"] == "help"
    ]
    assert len(help_messages) == 1 and "no declaration with that name" in help_messages[0], diagnostic


def check_conflicting_import_alias(directory: Path):
    root = directory / "conflicting-imports"
    modules = root / "sample"
    modules.mkdir(parents=True)
    (modules / "lib.vit").write_text(
        "space sample::lib;\nexport const VALUE: i64 = 1;\n",
        encoding="utf-8",
    )
    (modules / "other.vit").write_text(
        "space sample::other;\nexport const VALUE: i64 = 2;\n",
        encoding="utf-8",
    )
    source = (
        "space sample::main;\n"
        "use sample::lib as shared;\n"
        "use sample::other as shared;\n"
        "proc main() -> i64 { return 0; }\n"
    )
    path = root / "main.vit"
    path.write_text(source, encoding="utf-8")
    result = invoke(path, "compile", "json")
    assert result.returncode == 1, (result.returncode, result.stdout, result.stderr)
    assert not result.stdout, result.stdout
    document = json.loads(result.stderr)
    assert [item["code"] for item in document["diagnostics"]] == ["E0304"], document
    diagnostic = document["diagnostics"][0]
    assert diagnostic["internal_code"] == "VITTE_IMPORT_E_DUPLICATE", diagnostic
    assert diagnostic["phase"] == "import" and diagnostic["severity"] == "error", diagnostic
    assert diagnostic["message"] == "conflicting import alias", diagnostic
    assert diagnostic["context"]["module"] == "sample::other", diagnostic
    assert diagnostic["context"]["symbol"] == "shared", diagnostic
    marker = "use sample::other as shared;"
    alias_start = source.index(marker) + marker.index("shared")
    start = len(source[:alias_start].encode("utf-8"))
    end = start + len("shared".encode("utf-8"))
    position = diagnostic["position"]
    assert position["source"] == str(path), position
    assert (position["start_byte"], position["end_byte"]) == (start, end), position
    primary = [label for label in diagnostic["labels"] if label["style"] == "primary"]
    assert len(primary) == 1, diagnostic["labels"]
    assert primary[0]["message"] == "alias `shared` is already bound here", primary
    assert (primary[0]["span"]["start_byte"], primary[0]["span"]["end_byte"]) == (
        start,
        end,
    ), primary
    related = diagnostic["related_locations"]
    assert len(related) == 1, diagnostic
    assert related[0]["label"] == "earlier import binds this alias", related
    previous = "use sample::lib as shared;"
    previous_start = len(source[:source.index(previous)].encode("utf-8"))
    previous_end = previous_start + len(previous.encode("utf-8"))
    assert related[0]["span"]["source"] == str(path), related
    assert (related[0]["span"]["start_byte"], related[0]["span"]["end_byte"]) == (
        previous_start,
        previous_end,
    ), related
    helps = [item for item in diagnostic["annotations"] if item["kind"] == "help"]
    assert len(helps) == 1 and "distinct aliases" in helps[0]["message"], helps


def check_duplicate_group_import(directory: Path):
    root = directory / "duplicate-symbol-imports"
    module_dir = root / "sample"
    module_dir.mkdir(parents=True)
    (module_dir / "lib.vit").write_text(
        "space sample::lib;\nexport const VALUE: i64 = 1;\n",
        encoding="utf-8",
    )
    path = root / "main.vit"
    path.write_text(
        "space sample::main;\n"
        "use sample::lib::{VALUE, VALUE};\n"
        "proc main() -> i64 { return VALUE; }\n",
        encoding="utf-8",
    )
    result = invoke(path, "compile", "json")
    assert result.returncode == 1, (result.returncode, result.stdout, result.stderr)
    document = json.loads(result.stderr)
    diagnostic = next(
        item for item in document["diagnostics"] if item["code"] == "E0304")
    assert diagnostic["phase"] == "import", diagnostic
    assert diagnostic["message"] == "duplicate import", diagnostic
    assert diagnostic["related_locations"], diagnostic
    assert diagnostic["related_locations"][0]["label"] == "earlier identical import is here", diagnostic
    help_messages = [
        item["message"] for item in diagnostic["annotations"]
        if item["kind"] == "help"
    ]
    assert len(help_messages) == 1 and "imported more than once" in help_messages[0], diagnostic


def check_glob_import_ambiguity(directory: Path):
    root = directory / "ambiguous-glob-imports"
    module_dir = root / "sample"
    module_dir.mkdir(parents=True)
    first = module_dir / "one.vit"
    second = module_dir / "two.vit"
    first.write_text(
        "space sample::one;\nexport const VALUE: i64 = 1;\n",
        encoding="utf-8",
    )
    second.write_text(
        "space sample::two;\nexport const VALUE: i64 = 2;\n",
        encoding="utf-8",
    )
    path = root / "main.vit"
    path.write_text(
        "space sample::main;\n"
        "use sample::one::*;\n"
        "use sample::two::*;\n"
        "proc main() -> i64 { return 0; }\n",
        encoding="utf-8",
    )
    result = invoke(path, "compile", "json")
    assert result.returncode == 1, (result.returncode, result.stdout, result.stderr)
    document = json.loads(result.stderr)
    diagnostic = next(
        item for item in document["diagnostics"] if item["code"] == "E0403")
    assert diagnostic["phase"] == "name-resolution", diagnostic
    assert diagnostic["related_locations"], diagnostic
    candidates = {
        diagnostic["position"]["source"],
        diagnostic["related_locations"][0]["span"]["source"],
    }
    assert candidates == {str(first), str(second)}, diagnostic
    evidence = diagnostic.get("details", "") + " " + " ".join(
        item["message"] for item in diagnostic["annotations"]
    )
    assert "sample::one" in evidence and "sample::two" in evidence, diagnostic


def check_group_import_ambiguity(directory: Path):
    root = directory / "ambiguous-group-imports"
    module_dir = root / "sample"
    module_dir.mkdir(parents=True)
    first = module_dir / "one.vit"
    second = module_dir / "two.vit"
    first.write_text(
        "space sample::one;\nexport const VALUE: i64 = 1;\n",
        encoding="utf-8",
    )
    second.write_text(
        "space sample::two;\nexport const VALUE: i64 = 2;\n",
        encoding="utf-8",
    )
    path = root / "main.vit"
    path.write_text(
        "space sample::main;\n"
        "use sample::one::{VALUE};\n"
        "use sample::two::{VALUE};\n"
        "proc main() -> i64 { return 0; }\n",
        encoding="utf-8",
    )
    result = invoke(path, "compile", "json")
    assert result.returncode == 1, (result.returncode, result.stdout, result.stderr)
    document = json.loads(result.stderr)
    diagnostic = next(
        item for item in document["diagnostics"] if item["code"] == "E0403")
    candidates = {
        diagnostic["position"]["source"],
        diagnostic["related_locations"][0]["span"]["source"],
    }
    assert candidates == {str(first), str(second)}, diagnostic
    help_messages = [
        item["message"] for item in diagnostic["annotations"]
        if item["kind"] == "help"
    ]
    assert len(help_messages) == 1 and "as local" in help_messages[0], diagnostic


def check_module_member_import_help(directory: Path):
    root = directory / "module-member-import"
    module_dir = root / "sample"
    module_dir.mkdir(parents=True)
    (module_dir / "lib.vit").write_text(
        "space sample::lib;\nexport const public_value: i64 = 1;\n",
        encoding="utf-8",
    )
    path = root / "main.vit"
    path.write_text(
        "space sample::main;\n"
        "use sample::lib;\n"
        "proc main() -> i64 { return public_value; }\n",
        encoding="utf-8",
    )
    result = invoke(path, "compile", "json")
    assert result.returncode == 1, (result.returncode, result.stdout, result.stderr)
    document = json.loads(result.stderr)
    diagnostic = next(
        item for item in document["diagnostics"] if item["code"] == "E0401")
    help_messages = [
        item["message"] for item in diagnostic["annotations"]
        if item["kind"] == "help"
    ]
    assert len(help_messages) == 1, diagnostic
    assert "lib::public_value" in help_messages[0], help_messages[0]
    assert "use sample::lib::{public_value};" in help_messages[0], help_messages[0]
    assert "Did you mean" not in help_messages[0], help_messages[0]


def check_visible_name_suggestions(directory: Path):
    root = directory / "visible-name-suggestions"
    root.mkdir()
    unique = root / "unique.vit"
    unique.write_text(
        "proc main() -> i64 { let counter = 1; return countr; }\n",
        encoding="utf-8",
    )
    unique_result = invoke(unique, "compile", "json")
    unique_document = json.loads(unique_result.stderr)
    unique_diagnostic = next(
        item for item in unique_document["diagnostics"] if item["code"] == "E0401")
    unique_help = " ".join(
        item["message"] for item in unique_diagnostic["annotations"]
        if item["kind"] == "help"
    )
    assert "Did you mean `counter`" in unique_help, unique_diagnostic

    tied = root / "tied.vit"
    tied.write_text(
        "proc main() -> i64 { let counter = 1; let counte = 2; return countr; }\n",
        encoding="utf-8",
    )
    tied_result = invoke(tied, "compile", "json")
    tied_document = json.loads(tied_result.stderr)
    tied_diagnostic = next(
        item for item in tied_document["diagnostics"] if item["code"] == "E0401")
    tied_help = " ".join(
        item["message"] for item in tied_diagnostic["annotations"]
        if item["kind"] == "help"
    )
    assert "Did you mean" not in tied_help, tied_diagnostic


def check_import_cycle(directory: Path):
    module_dir = directory / "cycle" / "imports" / "sample"
    module_dir.mkdir(parents=True)
    first = module_dir / "a.vit"
    first_source = "space sample::a;\nuse sample::b;\nconst A: i64 = 1;\n"
    first.write_text(first_source, encoding="utf-8")
    second = module_dir / "b.vit"
    second_source = "space sample::b;\nuse sample::a;\nconst B: i64 = 2;\n"
    second.write_text(second_source, encoding="utf-8")
    path = module_dir.parent / "main.vit"
    path.write_text(
        "space sample::main;\nuse sample::a;\n"
        "proc main() -> i64 { return a::A; }\n",
        encoding="utf-8",
    )
    result = invoke(path, "compile", "json")
    assert result.returncode == 1, (result.returncode, result.stdout, result.stderr)
    document = json.loads(result.stderr)
    assert [item["code"] for item in document["diagnostics"]] == ["E0303"], document
    diagnostic = document["diagnostics"][0]
    assert diagnostic["phase"] == "import" and diagnostic["severity"] == "error", diagnostic
    assert diagnostic["context"]["module"] == "sample::b", diagnostic
    assert diagnostic["context"]["procedure"] == "import graph", diagnostic
    assert str(first) in diagnostic["details"] and str(second) in diagnostic["details"], diagnostic
    assert diagnostic["details"].count(" -> ") == 2, diagnostic["details"]
    start = len(first_source[:first_source.index("use sample::b;")].encode("utf-8"))
    end = start + len("use sample::b;".encode("utf-8"))
    assert diagnostic["position"]["source"] == str(first), diagnostic
    assert (diagnostic["position"]["start_byte"], diagnostic["position"]["end_byte"]) == (
        start,
        end,
    ), diagnostic
    primary = [label for label in diagnostic["labels"] if label["style"] == "primary"]
    assert len(primary) == 1, diagnostic["labels"]
    assert primary[0]["message"].startswith(
        "this import participates in the cycle; import chain:"
    ), primary
    assert (primary[0]["span"]["start_byte"], primary[0]["span"]["end_byte"]) == (
        start,
        end,
    ), primary
    related = diagnostic["related_locations"]
    assert len(related) == 1, diagnostic
    assert related[0]["label"] == "another import in this cycle is here", related
    assert related[0]["span"]["source"] == str(second), related
    other_import_start = len(second_source[:second_source.index("use sample::a;")].encode("utf-8"))
    other_import_end = other_import_start + len("use sample::a;".encode("utf-8"))
    assert (
        related[0]["span"]["start_byte"],
        related[0]["span"]["end_byte"],
    ) == (other_import_start, other_import_end), related
    helps = [item for item in diagnostic["annotations"] if item["kind"] == "help"]
    assert len(helps) == 1 and "Break the reported import cycle" in helps[0]["message"], helps


def check_missing_package(directory: Path):
    root_dir = directory / "missing-package-project"
    source_dir = root_dir / "src"
    source_dir.mkdir(parents=True)
    package = "diagnostic_contract_absent_pkg"
    (root_dir / "vitte.packages.toml").write_text(
        f'dependency.{package} = "1.2.3"\n', encoding="utf-8")
    source = (
        "space package_test::main;\n"
        f"use {package}::api;\n"
        "proc main() -> i64 { return 0; }\n"
    )
    path = source_dir / "main.vit"
    path.write_text(source, encoding="utf-8")
    result = invoke(path, "compile", "json")
    assert result.returncode == 1, (result.returncode, result.stdout, result.stderr)
    document = json.loads(result.stderr)
    assert [item["code"] for item in document["diagnostics"]] == ["E0305"], document
    diagnostic = document["diagnostics"][0]
    assert diagnostic["internal_code"] == "VITTE_IMPORT_E_PACKAGE_NOT_FOUND", diagnostic
    assert diagnostic["phase"] == "import", diagnostic
    assert package in diagnostic["details"] and "1.2.3" in diagnostic["details"], diagnostic
    assert "search roots checked in order" in diagnostic["details"], diagnostic
    assert diagnostic["labels"][0]["message"] == (
        f"package `{package}` source directory was not found"
    ), diagnostic
    help_messages = [
        item["message"] for item in diagnostic["annotations"]
        if item["kind"] == "help"
    ]
    assert len(help_messages) == 1 and "VITTE_MODULE_PATH" in help_messages[0], diagnostic
    marker = f"{package}::api"
    start = len(source[:source.index(marker)].encode("utf-8"))
    end = start + len(marker.encode("utf-8"))
    assert diagnostic["position"]["source"] == str(path), diagnostic
    assert (diagnostic["position"]["start_byte"], diagnostic["position"]["end_byte"]) == (
        start, end,
    ), diagnostic


def check_missing_module_in_installed_package(directory: Path):
    root_dir = directory / "installed-package-project"
    source_dir = root_dir / "src"
    package_name = "diagnostic_contract_installed_pkg"
    package_root = root_dir / ".vitte" / "modules" / package_name
    source_dir.mkdir(parents=True)
    package_root.mkdir(parents=True)
    (root_dir / "vitte.packages.toml").write_text(
        f'dependency.{package_name} = "2.0.0"\n', encoding="utf-8")
    source = (
        "space package_test::main;\n"
        f"use {package_name}::absent;\n"
        "proc main() -> i64 { return 0; }\n"
    )
    path = source_dir / "main.vit"
    path.write_text(source, encoding="utf-8")
    result = invoke(path, "compile", "json")
    assert result.returncode == 1, (result.returncode, result.stdout, result.stderr)
    document = json.loads(result.stderr)
    assert [item["code"] for item in document["diagnostics"]] == ["E0301"], document
    diagnostic = document["diagnostics"][0]
    assert diagnostic["internal_code"] == "VITTE_IMPORT_E_MODULE_NOT_FOUND", diagnostic
    assert package_name in diagnostic["details"], diagnostic
    assert "search roots checked in order" in diagnostic["details"], diagnostic


def check_module_path_mismatch(directory: Path):
    module_dir = directory / "module-path-mismatch" / "demo"
    module_dir.mkdir(parents=True)
    library = module_dir / "wanted.vit"
    library_source = "space demo::actual;\nexport const VALUE: i64 = 1;\n"
    library.write_text(library_source, encoding="utf-8")
    source = (
        "space demo::main;\n"
        "use demo::wanted;\n"
        "proc main() -> i64 { return 0; }\n"
    )
    path = module_dir.parent / "main.vit"
    path.write_text(source, encoding="utf-8")
    result = invoke(path, "compile", "json")
    assert result.returncode == 1, (result.returncode, result.stdout, result.stderr)
    document = json.loads(result.stderr)
    assert [item["code"] for item in document["diagnostics"]] == ["E0300"], document
    diagnostic = document["diagnostics"][0]
    assert diagnostic["internal_code"] == "VITTE_IMPORT_E_MODULE_PATH_MISMATCH", diagnostic
    assert diagnostic["phase"] == "import", diagnostic
    primary = [label for label in diagnostic["labels"] if label["style"] == "primary"]
    assert len(primary) == 1 and "declares `demo::actual`" in primary[0]["message"], diagnostic
    help_messages = [
        item["message"] for item in diagnostic["annotations"]
        if item["kind"] == "help"
    ]
    assert len(help_messages) == 1 and "space" in help_messages[0], diagnostic
    declared = "demo::actual"
    declared_start = len(library_source[:library_source.index(declared)].encode("utf-8"))
    declared_end = declared_start + len(declared.encode("utf-8"))
    assert diagnostic["position"]["source"] == str(library), diagnostic
    assert (diagnostic["position"]["start_byte"], diagnostic["position"]["end_byte"]) == (
        declared_start, declared_end,
    ), diagnostic
    related = diagnostic["related_locations"]
    assert len(related) == 1, diagnostic
    assert related[0]["label"] == "import requests this module path", related
    requested = "demo::wanted"
    request_start = len(source[:source.index(requested)].encode("utf-8"))
    request_end = request_start + len(requested.encode("utf-8"))
    assert related[0]["span"]["source"] == str(path), related
    assert (related[0]["span"]["start_byte"], related[0]["span"]["end_byte"]) == (
        request_start, request_end,
    ), related


def check_source_code_inventory():
    inline_codes = {code for case in CASES for code in case[3]}
    cli_witness_codes = {"E0006", "E0104", "E0107", "E0108", "E0211", "E0300", "E0303", "E0304", "E0305", "E0403", "E0404"}
    api_witness_codes = {"E0108"}
    observed = inline_codes | cli_witness_codes | api_witness_codes
    assert observed == ACTIVE_BOOTSTRAP_SOURCE_CODES, {
        "missing_witness": sorted(ACTIVE_BOOTSTRAP_SOURCE_CODES - observed),
        "unclassified_witness": sorted(observed - ACTIVE_BOOTSTRAP_SOURCE_CODES),
    }

    # Keep E0108 wired into the public C API renderer contract as well as its
    # source CLI witness, which compiler-power-gate executes.
    c_test = (root / "src/tests/diagnostic_render_test.c").read_text(encoding="utf-8")
    assert "test_api_invalid_utf8_source_diagnostic" in c_test
    assert 'strstr(diagnostics, "E0108")' in c_test


with tempfile.TemporaryDirectory(prefix="vitte-diagnostic-contract-") as temporary:
    directory = Path(temporary)
    check_source_code_inventory()
    for case in CASES:
        check_case(directory, case)
    check_grammar_invalid_production_coverage()
    check_grammar_lexical_production_mapping()
    check_valid_contracts_and_returns(directory)
    check_invalid_utf8(directory)
    check_invalid_unicode_module_paths(directory)
    check_private_import(directory)
    check_declaration_origins(directory)
    check_conflicting_import_alias(directory)
    check_duplicate_group_import(directory)
    check_glob_import_ambiguity(directory)
    check_group_import_ambiguity(directory)
    check_module_member_import_help(directory)
    check_visible_name_suggestions(directory)
    check_invalid_reexport(directory)
    check_import_cycle(directory)
    check_missing_package(directory)
    check_missing_module_in_installed_package(directory)
    check_module_path_mismatch(directory)

print(
    "Bootstrap diagnostic contracts passed: "
    f"{len(CASES) + 1} exact source cases, 117 invalid syntax productions, "
    "31 lexical productions mapped, "
    "invalid UTF-8, duplicate and ambiguous imports, import privacy, "
    "package lookup, module paths and cycles."
)
