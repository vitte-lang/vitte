; no injections yet
; Tree-sitter injection queries for Vitte
; File: /Users/vincent/Documents/Github/vitte/editors/tree-sitter/queries/injections.scm

; Zone comments / contracts
((contract_block) @injection.content
  (#set! injection.language "markdown"))

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
