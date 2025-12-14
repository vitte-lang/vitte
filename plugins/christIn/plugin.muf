# Muffin plugin manifest (placeholder)

[plugin]
id = "org.vitte.plugin.regex"
name = "regex"
version = "0.1.0"
kind = "runtime"
entry = "vitte_plugin_entry"
compat = ">=0.1,<0.2"

[caps]
needs_text = true
needs_bytes = true
needs_fs = false
needs_net = false
needs_unsafe = false

[build]
sources = [
  "mod.vitte",
  "api/mod.vitte",
  "api/types.vitte",
  "api/flags.vitte",
  "api/pattern.vitte",
  "api/exec.vitte",
  "api/iter.vitte",
  "api/bytes.vitte",
  "api/text.vitte",
  "api/builder.vitte",
  "api/diagnostics.vitte",
  "api/version.vitte",
  "engine/mod.vitte",
  "engine/parser.vitte",
  "engine/ast.vitte",
  "engine/hir.vitte",
  "engine/nfa.vitte",
  "engine/dfa.vitte",
  "engine/backtrack.vitte",
  "engine/optim/mod.vitte",
  "engine/optim/literal_prefix.vitte",
  "engine/optim/charclass_simplify.vitte",
  "engine/optim/nfa_prune.vitte",
  "engine/optim/cache.vitte",
  "runtime/mod.vitte",
  "runtime/matcher.vitte",
  "runtime/input.vitte",
  "runtime/unicode.vitte",
  "runtime/rope_stub.vitte",
  "runtime/limits.vitte",
  "data/mod.vitte",
  "data/ascii_tables.vitte",
  "data/unicode_tables.vitte",
  "tests/t_compile.vitte",
  "tests/t_match.vitte",
  "tests/t_captures.vitte",
  "tests/t_split_replace.vitte",
  "tests/t_bytes.vitte",
  "tests/t_unicode.vitte",
  "tests/t_limits.vitte",
  "benches/b_literal_prefix.vitte",
  "benches/b_thompson.vitte",
  "benches/b_backtrack.vitte",
]
