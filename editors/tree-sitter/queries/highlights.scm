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
  "module"
  "mod"
  "pull"
  "import"
  "use"
  "share"
  "export"
  "all"
] @keyword.import

[
  "const"
  "global"
  "static"
  "let"
  "make"
  "set"
] @keyword.storage

[
  "form"
  "struct"
  "class"
  "trait"
  "interface"
  "pick"
  "enum"
  "union"
  "case"
  "macro"
  "impl"
] @keyword.type

[
  "proc"
  "fn"
  "flow"
  "entry"
  "program"
  "prog"
  "scenario"
  "scn"
  "at"
] @keyword.function

[
  "give"
  "return"
  "ret"
  "emit"
  "defer"
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
  "each"
  "in"
  "from"
  "step"
  "while"
  "until"
  "break"
  "continue"
] @keyword.repeat

[
  "unsafe"
  "async"
  "extern"
] @keyword.directive

[
  "not"
  "and"
  "or"
  "is"
  "as"
  "await"
] @keyword.operator

[
  "do"
  "say"
] @keyword

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
  "Float"
  "Str"
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
(flow_decl name: (identifier) @function)
(class_decl name: (identifier) @type)
(trait_decl name: (identifier) @type)
(impl_decl target: (type_expression (identifier) @type))

; ============================================================
; Calls / constructors
; ============================================================

(call_expr
  callee: (identifier) @function.call)

(call_expr
  callee: (api_identifier) @function.special)

(call_expr
  callee: (scoped_identifier) @function.call)

(member_expr
  property: (identifier) @property)

; ============================================================
; Modules / imports
; ============================================================

(space_decl path: (scoped_identifier) @module)
(use_stmt path: (scoped_identifier) @module)
(share_stmt path: (scoped_identifier) @module)

(use_stmt alias: (identifier) @variable.special)
(share_stmt path: (identifier) @variable.special)

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

((identifier) @variable.special
  (#match? @variable.special "_pkg$"))

; ============================================================
; Variables / parameters
; ============================================================

(parameter name: (identifier) @variable.parameter)
(let_stmt name: (pattern (identifier) @variable))
(set_stmt target: (identifier) @variable)
(for_stmt (pattern (identifier) @variable))
(match_arm (pattern (identifier) @variable))
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
