#!/usr/bin/env bash
set -euo pipefail

MODE="${1:-all}"
PHASE="${2:-${DOCS_MAX_PHASE:-phase3}}"
BOOK_QA_MODE="${DOCS_MAX_BOOK_QA_MODE:-warn}"
CONTENT_MODE="${DOCS_MAX_CONTENT_MODE:-warn}"

log() { printf '[docs-maximal] %s\n' "$1"; }
die() { printf '[docs-maximal][error] %s\n' "$1" >&2; exit 1; }

run() {
  log "$1"
  shift
  "$@"
}

run_warn() {
  log "$1"
  shift
  if "$@"; then
    return 0
  fi
  printf '[docs-maximal][warn] command failed but is non-blocking in current mode\n' >&2
}

run_content() {
  local label="$1"
  shift
  case "$CONTENT_MODE" in
    strict)
      run "$label" "$@"
      ;;
    warn)
      run_warn "$label" "$@"
      ;;
    off)
      log "$label skipped"
      ;;
    *)
      die "unsupported DOCS_MAX_CONTENT_MODE=$CONTENT_MODE (expected off, warn, or strict)"
      ;;
  esac
}

build_docs() {
  run "sync grammar" python3 docs/book/grammar/scripts/sync_grammar.py
  run "sync precedence" python3 docs/book/grammar/scripts/sync_precedence.py
  run "sync grammar surface" python3 docs/book/scripts/sync_grammar_surface.py
  run "build railroad diagrams" python3 docs/book/grammar/scripts/build_railroad.py
  run "build docs site" python3 tools/build_docs_site.py
  run "build stdlib reference pages" python3 tools/generate_stdlib_reference_pages.py
  run "build stdlib family docs" python3 tools/generate_stdlib_family_docs.py
  run "build book learning layer" python3 tools/build_book_learning_layer.py
  run "build grammar extras" python3 tools/build_grammar_extras.py
  run "build grammar practical page" python3 tools/build_grammar_practical_page.py
  run "sync ebnf memory pages" python3 tools/sync_ebnf_memory_pages.py
  run "build static extras" python3 tools/build_static_extras.py
  run "render markdown-backed docs html" python3 tools/render_site_html.py --root docs
  run "generate package index" python3 tools/generate_package_index.py
  run "sanitize generated script tags" python3 tools/fix_broken_script_tags.py
  run "index generated reports" python3 tools/reports_index.py
}

validate_docs() {
  run "docs doctor" python3 tools/docs_doctor.py
  run "docs assets policy" python3 tools/docs/check_assets_policy.py
  run "grammar sync tests" python3 tests/docs/test_grammar_sync.py
  run "ebnf memory sync tests" python3 tests/docs/test_ebnf_memory_sync.py
  run "search indexes" python3 tools/check_search_indexes.py
  run "docs perf" python3 tools/check_docs_perf.py
  run "duplicate scripts" python3 tools/check_no_duplicate_scripts.py
  run "duplicate css" python3 tools/check_no_duplicate_css.py
  run "broken internal links" python3 tools/check_broken_internal_links.py
  run "html page sizes" python3 tools/check_html_page_sizes.py
  run "english-only policy" python3 tools/check_docs_en_only.py
  run "book pedagogy" python3 tools/check_book_pedagogy.py --phase "$PHASE"
  run "docs spacing tokens" python3 tools/lint_docs_tokens.py
  case "$BOOK_QA_MODE" in
    strict)
      run "book qa strict" python3 docs/book/scripts/qa_book.py --strict
      ;;
    normal)
      run "book qa" python3 docs/book/scripts/qa_book.py
      ;;
    warn)
      run_warn "book qa" python3 docs/book/scripts/qa_book.py
      ;;
    off)
      log "book qa skipped"
      ;;
    *)
      die "unsupported DOCS_MAX_BOOK_QA_MODE=$BOOK_QA_MODE (expected off, warn, normal, or strict)"
      ;;
  esac
  run "book links" python3 docs/book/scripts/check_links.py --book-root docs/book --report target/reports/book_links_report.txt
  run_content "book structure" python3 docs/book/scripts/check_structure.py --book-root docs/book
  run_content "book length" python3 docs/book/scripts/check_chapter_length.py --chapters-dir docs/book/chapters --min-pages 10 --max-pages 30 --words-per-page 350 --report target/reports/chapter_length_report.txt
  run_content "keywords style" python3 docs/book/scripts/lint_keywords_style.py
  run_content "keywords references" python3 docs/book/chapters/keywords/scripts/lint_keywords.py
  run "docs paths" python3 tools/docs_paths_check.py
  run "docs sync gate" python3 tools/docs_sync_gate.py
  run_content "architecture docs coverage" python3 tools/architecture_docs_check.py
  run "local pages verification" tools/docs/verify_local_pages.sh
}

case "$MODE" in
  all)
    build_docs
    validate_docs
    ;;
  build)
    build_docs
    ;;
  validate)
    validate_docs
    ;;
  *)
    die "usage: tools/docs_maximal.sh [all|build|validate] [phase1|phase2|phase3]"
    ;;
esac

log "done mode=$MODE phase=$PHASE book_qa=$BOOK_QA_MODE content=$CONTENT_MODE"
