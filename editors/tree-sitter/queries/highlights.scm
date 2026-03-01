; Tree-sitter highlight queries for VITTE

(line_comment) @comment
(block_comment) @comment
(contract_block) @comment.block

(string_literal) @string
(number_literal) @number
(boolean_literal) @boolean
(null_literal) @constant.builtin

[
  "space" "pull" "use" "share" "const" "let" "make" "type"
  "form" "trait" "pick" "case" "proc" "entry" "at" "macro"
  "give" "emit" "asm" "unsafe" "if" "else" "otherwise"
  "loop" "for" "in" "break" "continue" "select" "when" "match"
  "return" "not" "and" "or" "is"
] @keyword

[
  "bool" "string" "int" "char"
  "i8" "i16" "i32" "i64" "isize"
  "u8" "u16" "u32" "u64" "usize"
  "f32" "f64"
] @type.builtin

(proc_decl name: (identifier) @function)
(proc_decl name: (api_identifier) @function.special)
(form_decl name: (identifier) @type)
(pick_decl name: (identifier) @type)
(trait_decl name: (identifier) @type)
(entry_decl name: (identifier) @function)

(use_stmt alias: (alias_pkg) @variable.special)
(alias_pkg) @variable.special
(diag_code) @diagnostic.error
(api_identifier) @function.special

(use_stmt path: (scoped_identifier) @module)
(scoped_identifier) @variable
(identifier) @variable
