; no injections yet
; Tree-sitter injection queries for Vitte
; File: /Users/vincent/Documents/Github/vitte/editors/tree-sitter/queries/injections.scm

; Zone comments / contracts
((contract_block) @injection.content
  (#set! injection.language "markdown"))

((zone_comment) @injection.content
  (#set! injection.language "markdown"))

; Inline assembly
((asm_block) @injection.content
  (#set! injection.language "asm"))

; Foreign C blocks
((foreign_block) @injection.content
  (#set! injection.language "c"))

; Documentation strings
((raw_string_literal) @injection.content
  (#match? @injection.content "^r?\"\"\"")
  (#set! injection.language "markdown"))

; Diagnostic JSON strings
((string_literal) @injection.content
  (#match? @injection.content "^\"\\s*\\{")
  (#set! injection.language "json"))

; Regex-like strings
((string_literal) @injection.content
  (#match? @injection.content "^\"/.*/\"")
  (#set! injection.language "regex"))

; Shell command strings
((call_expr
  function: (identifier) @_fn
  arguments: (argument_list (string_literal) @injection.content))
  (#match? @_fn "^(sh|shell|cmd|run_command|exec)$")
  (#set! injection.language "bash"))

; SQL strings
((call_expr
  function: (identifier) @_fn
  arguments: (argument_list (string_literal) @injection.content))
  (#match? @_fn "^(sql|query|db_query|execute_sql)$")
  (#set! injection.language "sql"))

; HTML template strings
((call_expr
  function: (identifier) @_fn
  arguments: (argument_list (string_literal) @injection.content))
  (#match? @_fn "^(html|template_html|render_html)$")
  (#set! injection.language "html"))

; CSS template strings
((call_expr
  function: (identifier) @_fn
  arguments: (argument_list (string_literal) @injection.content))
  (#match? @_fn "^(css|style|stylesheet)$")
  (#set! injection.language "css"))
