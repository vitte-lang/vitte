#!/usr/bin/env bash
set -euo pipefail

PHASE="${1:-phase1}"

echo "[docs] build core"
python3 tools/build_docs_site.py
python3 tools/generate_stdlib_reference_pages.py

echo "[docs] build grammar extras"
python3 tools/build_grammar_extras.py

echo "[docs] sync ebnf memory"
python3 tools/sync_ebnf_memory_pages.py

echo "[docs] build static extras"
python3 tools/build_static_extras.py

echo "[docs] sanitize generated html scripts"
python3 tools/fix_broken_script_tags.py

echo "[docs] validate"
python3 tools/docs_doctor.py
python3 tools/docs/check_assets_policy.py
python3 tests/docs/test_ebnf_memory_sync.py
python3 tests/docs/test_grammar_sync.py
python3 tools/check_search_indexes.py
python3 tools/check_docs_perf.py
python3 tools/check_no_duplicate_scripts.py
python3 tools/check_no_duplicate_css.py
python3 tools/check_docs_frontend.py
python3 tools/check_broken_internal_links.py
python3 tools/check_html_page_sizes.py
python3 tools/check_docs_en_only.py
python3 tools/check_book_pedagogy.py --phase "$PHASE"
tools/docs/verify_local_pages.sh

echo "[docs] done (phase=$PHASE)"
