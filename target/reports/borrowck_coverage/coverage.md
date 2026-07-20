# Borrowck Coverage

| Surface | Status | Owner | Source | Tests | Evidence |
| --- | --- | --- | --- | --- | --- |
| analysis/errors.BorrowDiagnosticResult | supported | diagnostics | src/vitte/compiler/analysis/borrowck/errors.vit | test_borrow_diagnostic_helpers_build_reports | BorrowDiagnosticResult is covered by test_borrow_diagnostic_helpers_build_reports |
| analysis/errors.BorrowErrorKind | supported | diagnostics | src/vitte/compiler/analysis/borrowck/errors.vit | test_borrow_diagnostic_helpers_build_reports | BorrowErrorKind is covered by test_borrow_diagnostic_helpers_build_reports |
| analysis/errors.BorrowPhase | supported | diagnostics | src/vitte/compiler/analysis/borrowck/errors.vit | test_borrow_diagnostic_helpers_build_reports | BorrowPhase is covered by test_borrow_diagnostic_helpers_build_reports |
| analysis/lifetimes.LifetimeEdge | supported | borrowck | src/vitte/compiler/analysis/borrowck/lifetimes.vit | test_lifetime_and_region_helpers_summarize_failures | LifetimeEdge is covered by test_lifetime_and_region_helpers_summarize_failures |
| analysis/lifetimes.LifetimeEscapeKind | supported | borrowck | src/vitte/compiler/analysis/borrowck/lifetimes.vit | test_lifetime_and_region_helpers_summarize_failures | LifetimeEscapeKind is covered by test_lifetime_and_region_helpers_summarize_failures |
| analysis/lifetimes.LifetimeFact | supported | borrowck | src/vitte/compiler/analysis/borrowck/lifetimes.vit | test_lifetime_and_region_helpers_summarize_failures | LifetimeFact is covered by test_lifetime_and_region_helpers_summarize_failures |
| analysis/lifetimes.LifetimeFactKind | supported | borrowck | src/vitte/compiler/analysis/borrowck/lifetimes.vit | test_lifetime_and_region_helpers_summarize_failures | LifetimeFactKind is covered by test_lifetime_and_region_helpers_summarize_failures |
| analysis/lifetimes.LifetimeResult | supported | borrowck | src/vitte/compiler/analysis/borrowck/lifetimes.vit | test_lifetime_and_region_helpers_summarize_failures | LifetimeResult is covered by test_lifetime_and_region_helpers_summarize_failures |
| analysis/lifetimes.LifetimeValidity | supported | borrowck | src/vitte/compiler/analysis/borrowck/lifetimes.vit | test_lifetime_and_region_helpers_summarize_failures | LifetimeValidity is covered by test_lifetime_and_region_helpers_summarize_failures |
| analysis/lifetimes.TemporalOwnershipWindow | supported | borrowck | src/vitte/compiler/analysis/borrowck/lifetimes.vit | test_temporal_ownership_windows_enforce_alias_duration | TemporalOwnershipWindow is covered by test_temporal_ownership_windows_enforce_alias_duration |
| analysis/loans.Loan | supported | borrowck | src/vitte/compiler/analysis/borrowck/loans.vit | test_loan_table_helpers_track_aliases_and_conflicts | Loan is covered by test_loan_table_helpers_track_aliases_and_conflicts |
| analysis/loans.LoanConflict | supported | borrowck | src/vitte/compiler/analysis/borrowck/loans.vit | test_loan_table_helpers_track_aliases_and_conflicts | LoanConflict is covered by test_loan_table_helpers_track_aliases_and_conflicts |
| analysis/loans.LoanConflictKind | supported | borrowck | src/vitte/compiler/analysis/borrowck/loans.vit | test_loan_table_helpers_track_aliases_and_conflicts | LoanConflictKind is covered by test_loan_table_helpers_track_aliases_and_conflicts |
| analysis/loans.LoanKind | supported | borrowck | src/vitte/compiler/analysis/borrowck/loans.vit | test_loan_table_helpers_track_aliases_and_conflicts | LoanKind is covered by test_loan_table_helpers_track_aliases_and_conflicts |
| analysis/loans.LoanState | supported | borrowck | src/vitte/compiler/analysis/borrowck/loans.vit | test_loan_table_helpers_track_aliases_and_conflicts | LoanState is covered by test_loan_table_helpers_track_aliases_and_conflicts |
| analysis/loans.LoanTable | supported | borrowck | src/vitte/compiler/analysis/borrowck/loans.vit | test_loan_table_helpers_track_aliases_and_conflicts | LoanTable is covered by test_loan_table_helpers_track_aliases_and_conflicts |
| analysis/mod.BorrowBlockState | supported | borrowck | src/vitte/compiler/analysis/borrowck/mod.vit | test_branch_move_after_join | BorrowBlockState is covered by test_branch_move_after_join |
| analysis/mod.BorrowCheckResult | supported | borrowck | src/vitte/compiler/analysis/borrowck/mod.vit | test_hir_and_lowered_mir_agree_on_copy_and_scope | BorrowCheckResult is covered by test_hir_and_lowered_mir_agree_on_copy_and_scope |
| analysis/mod.BorrowFunctionInfo | supported | borrowck | src/vitte/compiler/analysis/borrowck/mod.vit | test_impl_borrow_param_call_uses_canonical_mir_signature | BorrowFunctionInfo is covered by test_impl_borrow_param_call_uses_canonical_mir_signature |
| analysis/mod.BorrowFunctionParam | supported | borrowck | src/vitte/compiler/analysis/borrowck/mod.vit | test_borrow_param_call_does_not_move_source | BorrowFunctionParam is covered by test_borrow_param_call_does_not_move_source |
| analysis/mod.BorrowMetadata | supported | borrowck | src/vitte/compiler/analysis/borrowck/mod.vit | test_hir_and_lowered_mir_agree_on_copy_and_scope | BorrowMetadata is covered by test_hir_and_lowered_mir_agree_on_copy_and_scope |
| analysis/mod.BorrowScopeInfo | supported | borrowck | src/vitte/compiler/analysis/borrowck/mod.vit | test_branch_loan_expires_at_scope_end | BorrowScopeInfo is covered by test_branch_loan_expires_at_scope_end |
| analysis/moves.MoveConflict | supported | borrowck | src/vitte/compiler/analysis/borrowck/moves.vit | test_move_tracker_helpers_report_state_and_conflicts | MoveConflict is covered by test_move_tracker_helpers_report_state_and_conflicts |
| analysis/moves.MoveConflictKind | supported | borrowck | src/vitte/compiler/analysis/borrowck/moves.vit | test_move_tracker_helpers_report_state_and_conflicts | MoveConflictKind is covered by test_move_tracker_helpers_report_state_and_conflicts |
| analysis/moves.MoveEvent | supported | borrowck | src/vitte/compiler/analysis/borrowck/moves.vit | test_move_tracker_helpers_report_state_and_conflicts | MoveEvent is covered by test_move_tracker_helpers_report_state_and_conflicts |
| analysis/moves.MoveEventKind | supported | borrowck | src/vitte/compiler/analysis/borrowck/moves.vit | test_move_tracker_helpers_report_state_and_conflicts | MoveEventKind is covered by test_move_tracker_helpers_report_state_and_conflicts |
| analysis/moves.MoveState | supported | borrowck | src/vitte/compiler/analysis/borrowck/moves.vit | test_move_tracker_helpers_report_state_and_conflicts | MoveState is covered by test_move_tracker_helpers_report_state_and_conflicts |
| analysis/moves.MoveTracker | supported | borrowck | src/vitte/compiler/analysis/borrowck/moves.vit | test_move_tracker_helpers_report_state_and_conflicts | MoveTracker is covered by test_move_tracker_helpers_report_state_and_conflicts |
| analysis/ownership.LocalState | supported | borrowck | src/vitte/compiler/analysis/borrowck/ownership.vit | test_ownership_helpers_track_places_and_borrows | LocalState is covered by test_ownership_helpers_track_places_and_borrows |
| analysis/ownership.LocalStateKind | supported | borrowck | src/vitte/compiler/analysis/borrowck/ownership.vit | test_ownership_helpers_track_places_and_borrows | LocalStateKind is covered by test_ownership_helpers_track_places_and_borrows |
| analysis/ownership.OwnershipFact | supported | borrowck | src/vitte/compiler/analysis/borrowck/ownership.vit | test_ownership_helpers_track_places_and_borrows | OwnershipFact is covered by test_ownership_helpers_track_places_and_borrows |
| analysis/ownership.OwnershipStateKind | supported | borrowck | src/vitte/compiler/analysis/borrowck/ownership.vit | test_ownership_helpers_track_places_and_borrows | OwnershipStateKind is covered by test_ownership_helpers_track_places_and_borrows |
| analysis/ownership.OwnershipTable | supported | borrowck | src/vitte/compiler/analysis/borrowck/ownership.vit | test_ownership_helpers_track_places_and_borrows | OwnershipTable is covered by test_ownership_helpers_track_places_and_borrows |
| analysis/ownership.Place | supported | borrowck | src/vitte/compiler/analysis/borrowck/ownership.vit | test_ownership_helpers_track_places_and_borrows | Place is covered by test_ownership_helpers_track_places_and_borrows |
| analysis/ownership.PlaceKind | supported | borrowck | src/vitte/compiler/analysis/borrowck/ownership.vit | test_ownership_helpers_track_places_and_borrows | PlaceKind is covered by test_ownership_helpers_track_places_and_borrows |
| analysis/regions.MemoryRegionBinding | supported | borrowck | src/vitte/compiler/analysis/borrowck/regions.vit | test_first_class_memory_regions_bind_places_to_declared_regions | MemoryRegionBinding is covered by test_first_class_memory_regions_bind_places_to_declared_regions |
| analysis/regions.MemoryRegionDecl | supported | borrowck | src/vitte/compiler/analysis/borrowck/regions.vit | test_first_class_memory_regions_bind_places_to_declared_regions | MemoryRegionDecl is covered by test_first_class_memory_regions_bind_places_to_declared_regions |
| analysis/regions.MemoryRegionKind | supported | borrowck | src/vitte/compiler/analysis/borrowck/regions.vit | test_first_class_memory_regions_bind_places_to_declared_regions | MemoryRegionKind is covered by test_first_class_memory_regions_bind_places_to_declared_regions |
| analysis/regions.MemoryRegionModel | supported | borrowck | src/vitte/compiler/analysis/borrowck/regions.vit | test_first_class_memory_regions_bind_places_to_declared_regions | MemoryRegionModel is covered by test_first_class_memory_regions_bind_places_to_declared_regions |
| analysis/regions.Region | supported | borrowck | src/vitte/compiler/analysis/borrowck/regions.vit | test_lifetime_and_region_helpers_summarize_failures | Region is covered by test_lifetime_and_region_helpers_summarize_failures |
| analysis/regions.RegionConstraint | supported | borrowck | src/vitte/compiler/analysis/borrowck/regions.vit | test_lifetime_and_region_helpers_summarize_failures | RegionConstraint is covered by test_lifetime_and_region_helpers_summarize_failures |
| analysis/regions.RegionConstraintKind | supported | borrowck | src/vitte/compiler/analysis/borrowck/regions.vit | test_lifetime_and_region_helpers_summarize_failures | RegionConstraintKind is covered by test_lifetime_and_region_helpers_summarize_failures |
| analysis/regions.RegionSolution | supported | borrowck | src/vitte/compiler/analysis/borrowck/regions.vit | test_lifetime_and_region_helpers_summarize_failures | RegionSolution is covered by test_lifetime_and_region_helpers_summarize_failures |
| middle/checks.BorrowCheckKind | planned | mir | src/vitte/compiler/middle/borrow/checks.vit | borrowck-fixtures | BorrowCheckKind is covered by borrowck fixture presence |
| middle/checks.BorrowCheckResult | planned | mir | src/vitte/compiler/middle/borrow/checks.vit | borrowck-fixtures | BorrowCheckResult is covered by borrowck fixture presence |
| middle/checks.BorrowContext | planned | mir | src/vitte/compiler/middle/borrow/checks.vit | borrowck-fixtures | BorrowContext is covered by borrowck fixture presence |
| middle/checks.BorrowDiagnostic | planned | diagnostics | src/vitte/compiler/middle/borrow/checks.vit | borrowck-fixtures | BorrowDiagnostic is covered by borrowck fixture presence |
| middle/checks.BorrowLoan | planned | mir | src/vitte/compiler/middle/borrow/checks.vit | borrowck-fixtures | BorrowLoan is covered by borrowck fixture presence |
| middle/checks.BorrowPlace | planned | mir | src/vitte/compiler/middle/borrow/checks.vit | borrowck-fixtures | BorrowPlace is covered by borrowck fixture presence |
| middle/checks.BorrowRegion | planned | mir | src/vitte/compiler/middle/borrow/checks.vit | borrowck-fixtures | BorrowRegion is covered by borrowck fixture presence |
| middle/checks.BorrowSeverity | planned | diagnostics | src/vitte/compiler/middle/borrow/checks.vit | borrowck-fixtures | BorrowSeverity is covered by borrowck fixture presence |
| middle/checks.BorrowSpan | planned | mir | src/vitte/compiler/middle/borrow/checks.vit | borrowck-fixtures | BorrowSpan is covered by borrowck fixture presence |
| middle/checks.BorrowState | planned | mir | src/vitte/compiler/middle/borrow/checks.vit | borrowck-fixtures | BorrowState is covered by borrowck fixture presence |
| middle/checks.BorrowStateEntry | planned | mir | src/vitte/compiler/middle/borrow/checks.vit | borrowck-fixtures | BorrowStateEntry is covered by borrowck fixture presence |
| middle/regions.LoanKind | planned | mir | src/vitte/compiler/middle/borrow/regions.vit | borrowck-fixtures | LoanKind is covered by borrowck fixture presence |
| middle/regions.Region | planned | mir | src/vitte/compiler/middle/borrow/regions.vit | borrowck-fixtures | Region is covered by borrowck fixture presence |
| middle/regions.RegionContext | planned | mir | src/vitte/compiler/middle/borrow/regions.vit | borrowck-fixtures | RegionContext is covered by borrowck fixture presence |
| middle/regions.RegionGraphEdge | planned | mir | src/vitte/compiler/middle/borrow/regions.vit | borrowck-fixtures | RegionGraphEdge is covered by borrowck fixture presence |
| middle/regions.RegionId | planned | mir | src/vitte/compiler/middle/borrow/regions.vit | borrowck-fixtures | RegionId is covered by borrowck fixture presence |
| middle/regions.RegionKind | planned | mir | src/vitte/compiler/middle/borrow/regions.vit | borrowck-fixtures | RegionKind is covered by borrowck fixture presence |
| middle/regions.RegionLoan | planned | mir | src/vitte/compiler/middle/borrow/regions.vit | borrowck-fixtures | RegionLoan is covered by borrowck fixture presence |
| middle/regions.RegionSpan | planned | mir | src/vitte/compiler/middle/borrow/regions.vit | borrowck-fixtures | RegionSpan is covered by borrowck fixture presence |
| middle/regions.RegionState | planned | mir | src/vitte/compiler/middle/borrow/regions.vit | borrowck-fixtures | RegionState is covered by borrowck fixture presence |
| middle/regions.RegionSummary | planned | mir | src/vitte/compiler/middle/borrow/regions.vit | borrowck-fixtures | RegionSummary is covered by borrowck fixture presence |
| middle/regions.RegionVariable | planned | mir | src/vitte/compiler/middle/borrow/regions.vit | borrowck-fixtures | RegionVariable is covered by borrowck fixture presence |

## Uncovered Surfaces

- None.
