

# ============================================================================
# Muffin manifest – Vitte bootstrap pipeline
#
# Ce fichier décrit la pipeline logique utilisée pour le bootstrap de Vitte :
#   - découpage en phases (front/middle/back),
#   - artefacts intermédiaires (AST, HIR, MIR, IR, bytecode),
#   - relations entre modules du repo `vitte-core`,
#   - rôles des outils (compilateur, runner, tests).
#
# Il ne contient aucun code exécutable : uniquement des métadonnées pour la
# toolchain Vitte, les scripts de bootstrap et les outils (LSP, indexers, etc.).
# ============================================================================

[pipeline]
name        = "vitte-bootstrap"
edition     = "2025"
description = "Logical description of the Vitte compiler bootstrap pipeline."
version     = "0.1.0"

# Profil par défaut de la pipeline (dev/release/ci/local…)
default_profile = "dev"

# ---------------------------------------------------------------------------
# Vues globales sur les phases de pipeline
# ---------------------------------------------------------------------------

# Ordre logique des grandes phases de compilation Vitte.
# Chaque clé correspond à une section [pipeline.phase."…"] plus bas.
phases = [
  "front-end",
  "middle-end",
  "back-end",
  "runtime",
  "tooling",
]

# ---------------------------------------------------------------------------
# Phase : front-end
# ---------------------------------------------------------------------------

[pipeline.phase."front-end"]
description = "Lexing, parsing, AST, scope, name resolution and early diagnostics."
order       = 10

modules = [
  "vitte.compiler.lexer",
  "vitte.compiler.parser",
  "vitte.compiler.ast",
  "vitte.compiler.span",
  "vitte.compiler.diagnostics",
  "vitte.compiler.scope",
  "vitte.compiler.symbols",
]

# Artefacts produits par cette phase.
artifacts = [
  "ast",
  "scopes",
  "symbol-table",
  "early-diagnostics",
]

[pipeline.phase."front-end".artifact."ast"]
kind        = "tree"
format      = "vitte-ast"
description = "Syntax tree (AST) for each source file."

[pipeline.phase."front-end".artifact."scopes"]
kind        = "table"
format      = "vitte-scope"
description = "Lexical scope hierarchy and bindings."

[pipeline.phase."front-end".artifact."symbol-table"]
kind        = "table"
format      = "vitte-symbols"
description = "Symbol table enriched with resolution info."

[pipeline.phase."front-end".artifact."early-diagnostics"]
kind        = "list"
format      = "vitte-diag"
description = "Diagnostics emitted during early stages (lex/parse/scope)."

# ---------------------------------------------------------------------------
# Phase : middle-end
# ---------------------------------------------------------------------------

[pipeline.phase."middle-end"]
description = "Type checking, HIR/MIR transformations, and semantic passes."
order       = 20

modules = [
  "vitte.compiler.types",
  "vitte.compiler.hir",
  "vitte.compiler.mir",
  "vitte.compiler.typecheck",
  "vitte.compiler.constant_fold",
  "vitte.compiler.flow_analysis",
  "vitte.compiler.optimize",
]

artifacts = [
  "hir",
  "mir",
  "type-info",
  "optimized-mir",
]

[pipeline.phase."middle-end".artifact."hir"]
kind        = "tree"
format      = "vitte-hir"
description = "High-level IR (HIR) derived from AST."

[pipeline.phase."middle-end".artifact."mir"]
kind        = "graph"
format      = "vitte-mir"
description = "Medium-level IR (MIR) for control-flow and data-flow analyses."

[pipeline.phase."middle-end".artifact."type-info"]
kind        = "table"
format      = "vitte-types"
description = "Type information (inferred, explicit, constraints)."

[pipeline.phase."middle-end".artifact."optimized-mir"]
kind        = "graph"
format      = "vitte-mir"
description = "MIR after basic optimizations (DCE, constant folding, simplifications)."

# ---------------------------------------------------------------------------
# Phase : back-end
# ---------------------------------------------------------------------------

[pipeline.phase."back-end"]
description = "Lowering to IR/bytecode, code generation and emission."
order       = 30

modules = [
  "vitte.compiler.ir",
  "vitte.compiler.codegen.bytecode",
  "vitte.compiler.codegen.text",
  "vitte.compiler.link",
]

artifacts = [
  "ir-program",
  "bytecode",
  "text-ir",
]

[pipeline.phase."back-end".artifact."ir-program"]
kind        = "graph"
format      = "vitte-ir"
description = "Canonical compiler IR program before emission."

[pipeline.phase."back-end".artifact."bytecode"]
kind        = "binary"
format      = "vitte-bytecode"
description = "Bytecode suitable for the Vitte VM/runtime."

[pipeline.phase."back-end".artifact."text-ir"]
kind        = "text"
format      = "vitte-ir-text"
description = "Human-readable textual IR, useful for debugging and tests."

# ---------------------------------------------------------------------------
# Phase : runtime
# ---------------------------------------------------------------------------

[pipeline.phase."runtime"]
description = "Runtime and VM components required to execute compiled programs."
order       = 40

modules = [
  "vitte.runtime.vm",
  "vitte.runtime.bytecode",
  "vitte.runtime.gc",
  "vitte.runtime.std_hooks",
]

artifacts = [
  "vm-image",
  "runtime-metadata",
]

[pipeline.phase."runtime".artifact."vm-image"]
kind        = "binary"
format      = "vitte-vm-image"
description = "Compiled VM/runtime image or configuration required to run bytecode."

[pipeline.phase."runtime".artifact."runtime-metadata"]
kind        = "table"
format      = "vitte-runtime-meta"
description = "Logical description of runtime capabilities and configuration."

# ---------------------------------------------------------------------------
# Phase : tooling
# ---------------------------------------------------------------------------

[pipeline.phase."tooling"]
description = "Auxiliary tools: formatter, LSP, symbol browser, test harness."
order       = 50

modules = [
  "vitte.tools.format",
  "vitte.tools.lsp",
  "vitte.tools.symbols",
  "vitte.tools.test_runner",
]

artifacts = [
  "format-report",
  "lsp-index",
  "symbol-index",
  "test-report",
]

[pipeline.phase."tooling".artifact."format-report"]
kind        = "text"
format      = "vitte-format-report"
description = "Reports produced by the formatter for diffing and tests."

[pipeline.phase."tooling".artifact."lsp-index"]
kind        = "index"
format      = "vitte-lsp-index"
description = "Index used by the LSP (definitions, references, workspace symbols)."

[pipeline.phase."tooling".artifact."symbol-index"]
kind        = "index"
format      = "vitte-symbol-index"
description = "Global index of symbols, used by tooling and diagnostics."

[pipeline.phase."tooling".artifact."test-report"]
kind        = "text"
format      = "vitte-test-report"
description = "Aggregated report for compiler/runtime/tests pipelines."

# ---------------------------------------------------------------------------
# Profils de pipeline
# ---------------------------------------------------------------------------

[pipeline.profile."dev"]
description = "Development pipeline: all phases enabled, extra diagnostics, no heavy optimizations."
phases      = ["front-end", "middle-end", "back-end", "runtime", "tooling"]

[pipeline.profile."release"]
description = "Release pipeline: optimizations enabled, tooling optional."
phases      = ["front-end", "middle-end", "back-end", "runtime"]

[pipeline.profile."ci"]
description = "Continuous integration pipeline: full pipeline plus extended tests."
phases      = ["front-end", "middle-end", "back-end", "runtime", "tooling"]

# ---------------------------------------------------------------------------
# Tooling hints – bootstrap scripts et toolchain Vitte
# ---------------------------------------------------------------------------

[tool.vitte.bootstrap]
# Ordre logique pour les scripts de bootstrap (stage0/stage1/stage2…).
# Ces noms n'ont pas de sémantique imposée au langage, ils servent uniquement
# de contrat pour les scripts shell et les runners.
stages = [
  "stage0-host-tools",
  "stage1-bootstrap-compiler",
  "stage2-self-hosted-compiler",
]

[tool.vitte.bootstrap.stage."stage0-host-tools"]
description = "Use a minimal host toolchain to build the first Vitte compiler."
depends_on  = []

# Modules / cibles buildés à ce stade (purement déclaratif).
targets = [
  "vitte.tools.format",
  "vitte.tools.symbols",
]

[tool.vitte.bootstrap.stage."stage1-bootstrap-compiler"]
description = "Build a Vitte compiler using the stage0 tools and run basic tests."
depends_on  = ["stage0-host-tools"]

targets = [
  "vitte.compiler.front_end",
  "vitte.compiler.middle_end",
  "vitte.compiler.back_end",
]

[tool.vitte.bootstrap.stage."stage2-self-hosted-compiler"]
description = "Rebuild the compiler with itself (self-host), run full test suite."
depends_on  = ["stage1-bootstrap-compiler"]

targets = [
  "vitte.compiler.full",
  "vitte.runtime.vm",
  "vitte.tools.test_runner",
]

[tool.vitte.bootstrap.test]
# Indications pour les tests automatisés du bootstrap.
expect_stage0_success = true
expect_stage1_success = true
expect_stage2_success = true
allow_experimental    = true