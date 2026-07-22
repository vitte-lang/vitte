#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / "target" / "reports" / "package_lsp_format_96_119.json"


@dataclass(frozen=True)
class Requirement:
    number: int
    title: str
    files: tuple[str, ...]
    terms: tuple[str, ...]


REQUIREMENTS = [
    Requirement(96, "package graph explain", ("src/vitte/tools/package_manager/mod.vit", "target/package_manager/package_graph_explain.json"), ("package_graph_explain", "edge_count")),
    Requirement(97, "workspace build selective", ("src/vitte/tools/package_manager/mod.vit", "target/package_manager/workspace_build_selective.json"), ("workspace_build_selective", "selected_members")),
    Requirement(98, "workspace test all", ("src/vitte/tools/package_manager/mod.vit", "target/package_manager/workspace_test_all.json"), ("workspace_test_all", "test_all")),
    Requirement(99, "workspace publish dry-run", ("src/vitte/tools/package_manager/mod.vit", "target/package_manager/workspace_publish_dry_run.json"), ("workspace_publish_dry_run", "would_publish")),
    Requirement(100, "LSP references complete", ("src/vitte/tools/lsp/mod.vit", "target/lsp/references_complete_demo.json"), ("lsp_references_complete", "references")),
    Requirement(101, "LSP code actions fix-it", ("src/vitte/tools/lsp/mod.vit", "target/lsp/code_actions_fixit_demo.json"), ("code_actions_fixit_provider", "quickfix")),
    Requirement(102, "LSP diagnostics streaming", ("src/vitte/tools/lsp/mod.vit", "target/lsp/diagnostics_streaming_demo.json"), ("diagnostics_streaming_batches", "final_batch")),
    Requirement(103, "LSP semantic tokens snapshots", ("src/vitte/tools/lsp/mod.vit", "target/lsp/semantic_tokens.snapshot.txt"), ("semantic_tokens_snapshot_text", "semantic-token-snapshot")),
    Requirement(104, "LSP workspace symbols", ("src/vitte/tools/lsp/mod.vit", "target/lsp/workspace_symbols_demo.json"), ("workspace_symbols_provider", "symbols")),
    Requirement(105, "LSP incremental document sync real", ("src/vitte/tools/lsp/mod.vit", "target/lsp/incremental_sync_demo.json"), ("incremental_document_sync_apply", "incremental")),
    Requirement(106, "LSP multi-root workspace", ("src/vitte/tools/lsp/mod.vit", "target/lsp/multi_root_workspace_demo.json"), ("multi_root_workspace_enabled", "roots")),
    Requirement(107, "LSP stress tests large project", ("src/vitte/tools/lsp/mod.vit", "target/lsp/large_project_stress_demo.json"), ("lsp_large_project_stress_profile", "file_count")),
    Requirement(108, "LSP VSCode/Neovim compatibility", ("src/vitte/tools/lsp/mod.vit", "target/lsp/client_compat_vscode_neovim.json"), ("vscode_neovim_compatibility_matrix", "neovim")),
    Requirement(109, "formatter stable by edition", ("tools/vitte_format.py", "target/formatter/edition-2026.snapshot.txt"), ("SUPPORTED_EDITIONS", "--edition")),
    Requirement(110, "formatter preserve comments", ("tools/vitte_format.py", "target/formatter/comments.snapshot.txt"), ("preserve_comments", "keep comment")),
    Requirement(111, "formatter normalize imports", ("tools/vitte_format.py", "target/formatter/imports.snapshot.txt"), ("normalize_imports", "use alpha")),
    Requirement(112, "formatter text snapshots", ("tools/formatter/generate_snapshots.py", "target/reports/formatter_coverage.md"), ("snapshot.txt", "Text snapshots")),
    Requirement(113, "formatter CI check obligatory", ("Makefile", ".github/workflows/ci.yml"), ("formatter-gate", "--check")),
    Requirement(114, "formatter migration guide", ("docs/formatter-migration-guide.md",), ("Formatter Migration Guide", "--edition 2026")),
    Requirement(115, "official package manager docs", ("docs/package-manager.md",), ("Vitte Package Manager", "workspace build")),
    Requirement(116, "official LSP docs", ("docs/lsp.md",), ("Vitte LSP", "VSCode")),
    Requirement(117, "official formatter docs", ("docs/formatter.md",), ("Vitte Formatter", "make formatter-gate")),
    Requirement(118, "real package/workspace examples", ("examples/package-workspace/vitte-workspace.json", "examples/package-workspace/packages/app/src/main.vit", "examples/package-workspace/packages/lib/src/lib.vit"), ("packages/app", "proc main", "proc answer")),
    Requirement(119, "release gate 90-119", ("Makefile", ".github/workflows/release-stability-gate.yml"), ("release-gate-90-119", "package-lsp-format-96-119")),
]


def read_combined(files: tuple[str, ...]) -> tuple[bool, str, list[str]]:
    missing: list[str] = []
    parts: list[str] = []
    for rel in files:
        path = ROOT / rel
        if not path.exists():
            missing.append(rel)
            continue
        parts.append(path.read_text(encoding="utf-8"))
    return not missing, "\n".join(parts), missing


def main() -> int:
    results: list[dict[str, object]] = []
    failures = 0

    for req in REQUIREMENTS:
        files_ok, content, missing = read_combined(req.files)
        missing_terms = [term for term in req.terms if term not in content]
        ok = files_ok and not missing_terms
        if not ok:
            failures += 1
        results.append(
            {
                "number": req.number,
                "title": req.title,
                "ok": ok,
                "missing_files": missing,
                "missing_terms": missing_terms,
                "files": list(req.files),
            }
        )

    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(
        json.dumps({"ok": failures == 0, "requirements": results}, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )

    if failures:
        for result in results:
            if not result["ok"]:
                print(
                    f"[release-96-119][error] {result['number']} {result['title']} "
                    f"missing_files={result['missing_files']} missing_terms={result['missing_terms']}",
                    file=sys.stderr,
                )
        return 1

    print("[release-96-119] checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
