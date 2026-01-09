## File: C:\Users\gogin\Documents\GitHub\vitte\lingua\types\vitte_hir_analysis\messages.ftl
## Vitte — HIR analysis diagnostics/messages (MAX)
##
## Conventions:
## - vitte-hir-analysis-<severity>-<topic>[-<detail>]
## - Variables: { $name }, { $detail }, { $node }, { $block }

# -----------------------------------------------------------------------------
# Generic
# -----------------------------------------------------------------------------

vitte-hir-analysis-kind = hir-analysis

vitte-hir-analysis-e-internal =
    internal HIR analysis error: { $detail }

vitte-hir-analysis-help-report-bug =
    This may be a compiler bug. Re-run with `--trace-hir-analysis` and report the issue.

# -----------------------------------------------------------------------------
# CFG
# -----------------------------------------------------------------------------

vitte-hir-analysis-e-cfg =
    failed to build control-flow graph: { $detail }

vitte-hir-analysis-w-cfg-incomplete =
    CFG is incomplete; results may be conservative

vitte-hir-analysis-note-cfg-node =
    CFG node { $node } for block { $block }

# -----------------------------------------------------------------------------
# Dominators
# -----------------------------------------------------------------------------

vitte-hir-analysis-e-dominators =
    failed to compute dominators: { $detail }

vitte-hir-analysis-w-dominators-approx =
    dominators were approximated due to incomplete CFG

# -----------------------------------------------------------------------------
# Liveness
# -----------------------------------------------------------------------------

vitte-hir-analysis-e-liveness =
    failed to compute liveness: { $detail }

vitte-hir-analysis-w-liveness-approx =
    liveness was approximated due to incomplete CFG

# -----------------------------------------------------------------------------
# Moves
# -----------------------------------------------------------------------------

vitte-hir-analysis-e-moves =
    failed to compute move analysis: { $detail }

vitte-hir-analysis-w-moves-conservative =
    move analysis is conservative due to missing place/projection info

# -----------------------------------------------------------------------------
# Call graph
# -----------------------------------------------------------------------------

vitte-hir-analysis-e-callgraph =
    failed to build call graph: { $detail }

vitte-hir-analysis-w-callgraph-incomplete =
    call graph may be incomplete (dynamic calls or unresolved paths)
