#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MOD = ROOT / 'src' / 'vitte' / 'tools' / 'lsp' / 'mod.vit'
SMOKE = ROOT / 'src' / 'vitte' / 'tools' / 'lsp' / 'tests' / 'smoke.vit'
STABILITY = ROOT / 'src' / 'vitte' / 'tools' / 'lsp' / 'tests' / 'stability_suite.vit'

REQUIRED_SYMBOLS = [
    'hover_provider',
    'completion_provider',
    'diagnostics_provider',
    'diagnostics_status_health',
    'diagnostics_status_health_for_source',
    'publish_diagnostics_for_source',
    'code_actions_for_source',
    'diagnostic_hover_for_source',
    'diagnostic_details_by_index_for_source',
    'diagnostic_details_by_code_for_source',
    'diagnostic_details_by_span_for_source',
    'diagnostic_hover_for_phase_source',
    'diagnostic_details_by_phase_index_for_source',
    'diagnostic_details_by_phase_code_for_source',
    'diagnostic_details_by_phase_span_for_source',
    'code_actions_for_phase_source',
    'diagnostic_phase_statuses_for_source',
    'diagnostic_panel_for_source',
    'diagnostic_panel_json',
    'diagnostic_panel_json_for_source',
    'status_health_json',
    'goto_definition',
    'find_references',
    'semantic_tokens_provider',
    'lsp_capabilities',
    'semantic_tokens: true',
    'rename: true',
    'formatting: true',
]

if not MOD.exists():
    print(f'[lsp][error] missing {MOD}', file=sys.stderr)
    raise SystemExit(1)

if not SMOKE.exists():
    print(f'[lsp][error] missing {SMOKE}', file=sys.stderr)
    raise SystemExit(1)

if not STABILITY.exists():
    print(f'[lsp][error] missing {STABILITY}', file=sys.stderr)
    raise SystemExit(1)

with open(MOD, 'r', encoding='utf-8') as f:
    content = f.read()
    for symbol in REQUIRED_SYMBOLS:
        if symbol not in content:
            print(f'[lsp][error] missing symbol {symbol}', file=sys.stderr)
            raise SystemExit(1)

with open(STABILITY, 'r', encoding='utf-8') as f:
    stability_content = f.read()
    stability_symbols = [
        'test_incremental_parsing_live_diagnostics',
        'test_concurrent_edits_stability',
        'test_malformed_files_are_safe',
        'test_workspace_reload_and_symbol_rename_stress',
        'test_status_health_granularity_is_stable',
        'test_status_health_real_pipeline_is_stable',
        'test_publish_diagnostics_real_pipeline_is_stable',
        'test_code_actions_real_pipeline_is_stable',
        'test_diagnostic_hover_real_pipeline_is_stable',
        'test_diagnostic_details_real_pipeline_is_stable',
        'test_phase_diagnostic_selection_is_stable',
        'test_phase_statuses_are_stable',
        'test_diagnostic_panel_is_stable',
        'test_diagnostic_panel_json_is_stable',
    ]
    for symbol in stability_symbols:
        if symbol not in stability_content:
            print(f'[lsp][error] missing stability symbol {symbol}', file=sys.stderr)
            raise SystemExit(1)

print('[lsp] checks passed')
