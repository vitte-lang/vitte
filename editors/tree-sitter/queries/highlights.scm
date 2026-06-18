; Tree-sitter highlight queries for Vitte
; File: queries/vitte/highlights.scm

; ============================================================
; Comments
; ============================================================

(line_comment) @comment
(block_comment) @comment
(contract_block) @comment.documentation

((contract_block) @comment.documentation
  (#match? @comment.documentation "^<<<\\s*ROLE-CONTRACT"))

; ============================================================
; Literals
; ============================================================

(string_literal) @string
(raw_string_literal) @string.special
(char_literal) @character
(number_literal) @number
(float_literal) @number.float
(boolean_literal) @boolean
(null_literal) @constant.builtin

[
  "true"
  "false"
] @boolean

; ============================================================
; Keywords
; ============================================================

[
  "space"
  "pull"
  "use"
  "share"
  "all"
] @keyword.import

[
  "const"
  "let"
  "make"
  "set"
] @keyword.storage

[
  "form"
  "trait"
  "pick"
  "case"
  "macro"
] @keyword.type

[
  "proc"
  "entry"
  "at"
] @keyword.function

[
  "give"
  "return"
  "emit"
] @keyword.return

[
  "if"
  "else"
  "otherwise"
  "select"
  "when"
  "match"
] @keyword.conditional

[
  "loop"
  "for"
  "in"
  "break"
  "continue"
] @keyword.repeat

[
  "unsafe"
] @keyword.directive

[
  "not"
  "and"
  "or"
  "is"
  "as"
] @keyword.operator

; ============================================================
; Builtin types
; ============================================================

[
  "bool"
  "string"
  "int"
  "char"
  "i8"
  "i16"
  "i32"
  "i64"
  "i128"
  "isize"
  "u8"
  "u16"
  "u32"
  "u64"
  "u128"
  "usize"
  "f32"
  "f64"
] @type.builtin

; ============================================================
; Declarations
; ============================================================

(proc_decl name: (identifier) @function)
(proc_decl name: (api_identifier) @function.special)

(entry_decl name: (identifier) @function)
(entry_decl target: (scoped_identifier) @module)

(form_decl name: (identifier) @type)
(class_decl name: (identifier) @type)
(trait_decl name: (identifier) @type)
(pick_decl name: (identifier) @type)
(enum_decl name: (identifier) @type)
(union_decl name: (identifier) @type)

(macro_decl name: (identifier) @function.macro)

; ============================================================
; Calls / constructors
; ============================================================

(call_expr
  callee: (identifier) @function.call)

(call_expr
  callee: (api_identifier) @function.special)

(call_expr
  callee: (scoped_identifier) @function.call)

; ============================================================
; Modules / imports
; ============================================================

(space_decl path: (scoped_identifier) @module)
(use_stmt path: (scoped_identifier) @module)
(share_stmt path: (scoped_identifier) @module)

(use_stmt alias: (identifier) @variable.special)

(scoped_identifier) @variable

; ============================================================
; Diagnostics / compiler API naming
; ============================================================

(diag_code) @diagnostic.error

((identifier) @function.special
  (#match? @function.special "^(diagnostics_|quickfix_|doctor_)[A-Za-z0-9_]*$"))

((api_identifier) @function.special
  (#match? @function.special "^(diagnostics_|quickfix_|doctor_)[A-Za-z0-9_]*$"))

((string_literal) @diagnostic.error
  (#match? @diagnostic.error "VITTE-[A-Z]+[0-9]{4}"))

; ============================================================
; Variables / parameters
; ============================================================

(parameter name: (identifier) @variable.parameter)
(let_stmt name: (pattern (identifier) @variable))
(const_decl name: (identifier) @constant)

(identifier) @variable

; ============================================================
; Operators / punctuation
; ============================================================

[
  "+"
  "-"
  "*"
  "/"
  "%"
  "="
  "=="
  "!="
  "<"
  "<="
  ">"
  ">="
  "=>"
  "->"
  "."
  "::"
] @operator

[
  "("
  ")"
  "{"
  "}"
  "["
  "]"
] @punctuation.bracket

[
  ","
  ":"
] @punctuation.delimiter
