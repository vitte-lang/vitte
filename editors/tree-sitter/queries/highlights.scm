(comment) @comment
(block_comment) @comment

(string_literal) @string
(character_literal) @character
(integer_literal) @number
(float_literal) @number.float

[
  "true"
  "false"
] @boolean
"null" @constant.builtin

[
  "space" "use" "export"
] @keyword.import

[
  "const" "static" "let" "mut" "set"
] @keyword.storage

[
  "form" "pick" "trait" "impl" "type" "opaque"
] @keyword.type

[
  "proc" "extern" "intrinsic" "macro" "test"
] @keyword.function

[
  "return" "give" "break" "continue" "defer"
] @keyword.control

[
  "if" "else" "while" "loop" "for" "in" "match"
] @keyword.control

[
  "pub" "async" "unsafe" "comptime" "where" "requires" "ensures"
] @keyword.modifier

[
  "not" "and" "or" "await" "move" "ref" "as"
] @keyword.operator

[
  "asm" "assert" "self"
] @keyword

(procedure_declaration "proc" (identifier) @function)
(intrinsic_declaration "intrinsic" (identifier) @function)
(macro_declaration "macro" (identifier) @function.macro)
(test_declaration "test" (identifier) @function)
(form_declaration "form" (identifier) @type)
(pick_declaration "pick" (identifier) @type)
(trait_declaration "trait" (identifier) @type)
(type_declaration "type" (identifier) @type)
(opaque_declaration "opaque" (identifier) @type)

(parameter (identifier) @variable.parameter)
(let_statement (identifier) @variable)
(for_statement (identifier) @variable)
(generic_parameter (identifier) @type)
(field (identifier) @property)
(variant (identifier) @constant)
(path (identifier) @variable)
(module_path) @module
(identifier) @variable

[
  "+" "-" "*" "/" "%" "=" "+=" "-=" "*=" "/=" "%="
  "&=" "|=" "^=" "<<=" ">>=" "??" "||" "&&" "|"
  "^" "&" "==" "!=" "<" "<=" ">" ">=" "<<" ">>"
  ".." "..=" "!" "~" "?" "->" "=>"
] @operator

[
  "(" ")" "{" "}" "[" "]"
] @punctuation.bracket

[
  "," ":" ";" "." "::"
] @punctuation.delimiter

((identifier) @type.builtin
  (#match? @type.builtin "^(void|never|unit|bool|char|rune|str|string|bytes|cstr|int|i8|i16|i32|i64|i128|isize|u8|u16|u32|u64|u128|usize|intptr|uintptr|f16|f32|f64|f128|c_char|c_int|c_uint|c_long|c_ulong|c_void)$"))
