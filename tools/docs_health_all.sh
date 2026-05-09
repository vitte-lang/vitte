#!/usr/bin/env bash
set -euo pipefail
python3 tools/build_docs_site.py
python3 tools/build_grammar_extras.py
python3 tools/sync_ebnf_memory_pages.py
python3 tools/build_static_extras.py
python3 tools/docs_doctor.py
python3 tools/check_search_indexes.py
python3 tools/check_docs_perf.py
python3 tools/check_no_duplicate_scripts.py
python3 tools/check_broken_internal_links.py
python3 tools/check_html_page_sizes.py
python3 tools/check_book_pedagogy.py
python3 tools/check_no_duplicate_css.py
echo "docs health all ok"
