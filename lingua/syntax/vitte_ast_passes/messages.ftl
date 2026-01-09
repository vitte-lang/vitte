# lingua/syntax/vitte_ast_passes/messages.ftl
# Diagnostic messages for the AST front-end pipeline orchestrator.
# Language: English
#
# Conventions:
# - Message IDs are kebab-case with prefix "passes-".
# - Use { $var } placeables for dynamic content.
# - Keep primary messages concise; add actionable hints in *.help.

passes-stage-name =
    AST Passes

passes-mode-fast =
    Pipeline mode: fast
passes-mode-normal =
    Pipeline mode: normal
passes-mode-strict =
    Pipeline mode: strict


### Pipeline input / setup

passes-input-missing-file =
    Pipeline input is missing the file node.
passes-input-missing-file.help =
    Provide a valid file node produced by the parser (or a compatible adapter node).

passes-input-invalid-file-kind =
    File node has invalid kind: expected { $expected }, got { $got }.
passes-input-invalid-file-kind.help =
    Ensure the root node is a file-level node containing only top-level items and doc nodes.

passes-input-missing-cfg-env =
    Pipeline input is missing the cfg environment.
passes-input-missing-cfg-env.help =
    Provide a cfg environment (even empty) to allow cfg pruning and feature gates.

passes-input-diags-null =
    Pipeline input diagnostics list is missing.
passes-input-diags-null.help =
    Provide a diagnostics sink list to collect errors and warnings.


### Pipeline configuration

passes-config-invalid =
    Pipeline configuration is invalid.
passes-config-invalid.help =
    Verify pass toggles and strictness settings.

passes-config-inconsistent =
    Pipeline configuration is inconsistent: { $reason }.
passes-config-inconsistent.help =
    Fix the configuration: for example, disabling expansion while keeping expansion-only features enabled.

passes-config-unknown-mode =
    Unknown pipeline mode: { $mode }.

passes-config-stop-on-first-error =
    stop_on_first_error is enabled; pipeline will halt after the first error.

passes-config-warnings-as-errors =
    warnings-as-errors is enabled; warnings will be promoted to errors.


### Comment / doc handling

passes-doc-store-disabled =
    Doc store is disabled; documentation will not be collected.

passes-doc-store-enabled =
    Doc store is enabled; documentation will be collected.

passes-doc-attach-disabled =
    Doc attachment is disabled; docs will not be attached to nodes.

passes-doc-attach-enabled =
    Doc attachment is enabled; docs may be attached to nodes.

passes-docs-imported-from-expansion =
    Imported documentation from expansion results.

passes-doc-zone-unclosed =
    Unclosed doc zone (missing >>>).
passes-doc-zone-unclosed.help =
    Close the doc zone with a line containing >>>.

passes-doc-zone-nested =
    Nested doc zones are not allowed.
passes-doc-zone-nested.help =
    Close the outer doc zone (>>>) before opening another.

passes-doc-zone-empty =
    Doc zone is empty.

passes-doc-line-empty =
    Doc line is empty.

passes-doc-chunk-merge =
    Merged adjacent doc chunks.

passes-doc-chunk-orphan =
    Orphan doc chunk could not be attached.
passes-doc-chunk-orphan.help =
    Ensure documentation precedes a valid declaration, or treat it as file-level docs.

passes-docs-summary =
    Docs: { $units } unit(s), { $chunks } chunk(s).


### Attribute normalization

passes-attr-normalization-disabled =
    Attribute normalization is disabled.

passes-attr-normalization-enabled =
    Attribute normalization is enabled.

passes-attr-registry-missing =
    Attribute registry is missing.
passes-attr-registry-missing.help =
    Provide a registry so attributes can be validated and normalized.

passes-attr-normalized =
    Normalized attributes for { $target }.

passes-attr-normalization-failed =
    Failed to normalize attributes for { $target }.

passes-attr-summary =
    Attributes: normalized { $count } time(s).


### Expansion stage

passes-expand-disabled =
    Expansion is disabled.

passes-expand-enabled =
    Expansion is enabled.

passes-expand-start =
    Starting expansion.

passes-expand-finished =
    Finished expansion.

passes-expand-failed =
    Expansion failed.
passes-expand-failed.help =
    Inspect diagnostics produced by expansion; enable tracing for more details.

passes-expand-trace-enabled =
    Expansion trace enabled.

passes-expand-trace-disabled =
    Expansion trace disabled.

passes-expand-strict-shape-enabled =
    Expansion strict shape enabled.

passes-expand-strict-shape-disabled =
    Expansion strict shape disabled.

passes-expand-strict-cfg-enabled =
    Expansion strict cfg enabled.

passes-expand-strict-cfg-disabled =
    Expansion strict cfg disabled.

passes-expand-cfg-disabled =
    cfg pruning is disabled; cfg gates will be ignored.

passes-expand-cfg-enabled =
    cfg pruning is enabled.

passes-expand-cfg-default-true =
    cfg default is true.

passes-expand-cfg-default-false =
    cfg default is false.

passes-expand-desugar-loop-until-enabled =
    Desugaring enabled: loop until -> loop while not.

passes-expand-desugar-each-enabled =
    Desugaring enabled: each -> loop each.

passes-expand-desugar-select-enabled =
    Desugaring enabled: select normalization enabled.

passes-expand-desugar-pack-enabled =
    Desugaring enabled: pack normalization enabled.

passes-expand-removed-docs =
    Removed { $count } doc node(s) during expansion.

passes-expand-removed-cfg =
    Removed { $count } cfg-gated node(s) during expansion.

passes-expand-rewritten =
    Rewrote { $count } node(s) during expansion.

passes-expand-attr-normalized =
    Normalized attributes on { $count } node(s) during expansion.

passes-expand-stats =
    Expansion stats: nodes { $in } -> { $out }, rewritten { $rewritten }, cfg removed { $cfg }, docs removed { $docs }.


### Lowering stage

passes-lowering-disabled =
    Lowering is disabled.

passes-lowering-enabled =
    Lowering is enabled.

passes-lowering-start =
    Starting lowering (AST -> AST-IR).

passes-lowering-finished =
    Finished lowering.

passes-lowering-failed =
    Lowering failed.
passes-lowering-failed.help =
    Inspect diagnostics produced by lowering; ensure the AST shape matches the lowering adapter expectations.

passes-lowering-bridge-failed =
    Failed to bridge expanded AST to lowering AST.

passes-lowering-strict-enabled =
    Lowering strict mode enabled.

passes-lowering-strict-disabled =
    Lowering strict mode disabled.

passes-lowering-desugar-loop-until-enabled =
    Lowering desugaring enabled: loop until -> loop while not.

passes-lowering-desugar-each-enabled =
    Lowering desugaring enabled: each -> loop each.

passes-lowering-desugar-select-enabled =
    Lowering desugaring enabled: select normalization enabled.

passes-lowering-stats =
    Lowering stats: items { $items }, blocks { $blocks }, stmts { $stmts }, exprs { $exprs }.


### Diagnostics and control flow

passes-stop-early =
    Stopping pipeline early due to configuration.

passes-stop-early-error =
    Stopping pipeline early due to errors.
passes-stop-early-error.help =
    Disable stop_on_first_error to collect more errors in one run.

passes-warnings-promoted =
    Warnings were promoted to errors.

passes-diags-summary =
    Diagnostics: { $errors } error(s), { $warnings } warning(s).

passes-stage-summary =
    Pipeline summary: expand { $expand_in }->{ $expand_out }, lower items { $items }, errors { $errors }, warnings { $warnings }.


### General / internal

passes-internal-error =
    Internal pipeline error: { $detail }.

passes-internal-missing-stage-result =
    Internal error: missing stage result: { $stage }.

passes-internal-unsupported =
    Unsupported pipeline operation: { $op }.

passes-hint-enable-trace =
    Hint: enable expansion tracing to see rewrite steps.
passes-hint-enable-trace.help =
    Set expand_trace=true in PassConfig to get per-node expansion logs.

passes-hint-run-strict =
    Hint: run strict mode to catch structural errors early.
passes-hint-run-strict.help =
    Use pass_config_strict() for CI or compiler regression tests.

passes-hint-disable-stage =
    Hint: disable a stage to isolate issues.
passes-hint-disable-stage.help =
    For example, disable expansion to test lowering-only behavior (or vice versa).
