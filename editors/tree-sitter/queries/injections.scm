; Tree-sitter injection queries for Vitte.

; Diagnostic JSON strings
((string_literal) @injection.content
  (#match? @injection.content "^\"\\s*\\{")
  (#set! injection.language "json"))

; Regex-like strings
((string_literal) @injection.content
  (#match? @injection.content "^\"/.*/\"")
  (#set! injection.language "regex"))
