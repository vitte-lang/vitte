#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"

log() { printf "[modules-tests] %s\n" "$*"; }
die() { printf "[modules-tests][error] %s\n" "$*" >&2; exit 1; }

[ -x "$BIN" ] || die "missing binary: $BIN"
DENY_LEGACY_SELF_LEAF="${DENY_LEGACY_SELF_LEAF:-0}"
LEGACY_SELF_LEAF_WARN_ONLY="${LEGACY_SELF_LEAF_WARN_ONLY:-0}"

graph_src="$ROOT_DIR/tests/modules/mod_graph/main.vit"
doctor_src="$ROOT_DIR/tests/modules/mod_doctor/main.vit"
api_old="$ROOT_DIR/tests/modules/api_diff/old_case/main.vit"
api_new="$ROOT_DIR/tests/modules/api_diff/new_case/main.vit"
exp_src="$ROOT_DIR/tests/modules/experimental/main.vit"
internal_src="$ROOT_DIR/tests/modules/internal/main.vit"
reexport_src="$ROOT_DIR/tests/modules/reexport_conflict/main.vit"
cross_pkg_src="$ROOT_DIR/tests/modules/contracts/cross_package/main.vit"
legacy_runtime_src="$ROOT_DIR/tests/modules/mod_doctor_legacy/main.vit"
legacy_write_expected="$ROOT_DIR/tests/modules/snapshots/mod_doctor_fix_write.rewritten.must"
doctor_write_cases_dir="$ROOT_DIR/tests/modules/mod_doctor_write_cases"

log "mod graph"
out_graph="$("$BIN" mod graph --lang=en "$graph_src" 2>&1)"
grep -Fq "module_graph:" <<<"$out_graph" || die "missing module_graph output"
grep -Fq "cycles:" <<<"$out_graph" || die "missing cycles output"
grep -Fq -- "->" <<<"$out_graph" || die "missing graph edges"

log "mod graph json"
out_graph_json="$("$BIN" mod graph --lang=en --json --from __root__ "$graph_src" 2>&1)"
grep -Fq "\"nodes\"" <<<"$out_graph_json" || die "missing graph json nodes"
grep -Fq "\"edges\"" <<<"$out_graph_json" || die "missing graph json edges"
grep -Fq "\"from\": \"__root__\"" <<<"$out_graph_json" || die "missing graph from filter"

log "mod doctor"
set +e
out_doctor="$("$BIN" mod doctor --lang=en "$doctor_src" 2>&1)"
rc=$?
set -e
[ "$rc" -ne 0 ] || die "mod doctor should report issues for fixture"
grep -Fq "non-canonical import path" <<<"$out_doctor" || die "missing non-canonical diagnostic"
grep -Fq "missing alias in use" <<<"$out_doctor" || die "missing alias diagnostic"
grep -Fq "unused alias" <<<"$out_doctor" || die "missing unused alias diagnostic"

log "mod doctor fix"
set +e
out_doctor_fix="$("$BIN" mod doctor --lang=en --fix --max-imports=1 "$doctor_src" 2>&1)"
rc_fix=$?
set -e
[ "$rc_fix" -ne 0 ] || die "mod doctor --fix should still fail on fixture"
grep -Fq "fix: use doctor/util as util" <<<"$out_doctor_fix" || die "missing canonical fix suggestion"

log "legacy import runtime compat"
set +e
legacy_args=()
if [ "$LEGACY_SELF_LEAF_WARN_ONLY" = "1" ]; then
  legacy_args+=(--legacy-self-leaf-warn-only)
elif [ "$DENY_LEGACY_SELF_LEAF" = "1" ]; then
  legacy_args+=(--deny-legacy-self-leaf)
fi
out_legacy_runtime="$("$BIN" check --lang=en "${legacy_args[@]}" "$legacy_runtime_src" 2>&1)"
rc_legacy_runtime=$?
set -e
if [ "$LEGACY_SELF_LEAF_WARN_ONLY" = "1" ]; then
  [ "$rc_legacy_runtime" -eq 0 ] || die "legacy self-leaf warn-only mode should keep runtime compatibility"
  grep -Fq "[driver] mir ok" <<<"$out_legacy_runtime" || die "legacy warn-only fixture should reach mir ok"
  grep -Fq "warning[E1020]" <<<"$out_legacy_runtime" || die "missing E1020 warning in warn-only runtime mode"
elif [ "$DENY_LEGACY_SELF_LEAF" = "1" ]; then
  if [ "$rc_legacy_runtime" -eq 0 ]; then
    grep -Fq "[driver] mir ok" <<<"$out_legacy_runtime" || die "legacy fixture should either pass cleanly or fail with diagnostics"
  else
    grep -Fq "error[E" <<<"$out_legacy_runtime" || die "legacy deny mode failed without diagnostic"
  fi
else
  [ "$rc_legacy_runtime" -eq 0 ] || die "legacy self-leaf import should remain runtime-compatible in phase B"
  grep -Fq "[driver] mir ok" <<<"$out_legacy_runtime" || die "legacy runtime fixture should reach mir ok"
  set +e
  out_legacy_warn="$("$BIN" check --lang=en --fail-on-warning "$legacy_runtime_src" 2>&1)"
  rc_legacy_warn=$?
  set -e
  [ "$rc_legacy_warn" -ne 0 ] || die "legacy import should fail with --fail-on-warning due to E1020"
  grep -Fq "warning[E1020]" <<<"$out_legacy_warn" || die "missing E1020 warning on legacy import with --fail-on-warning"
fi

log "legacy self-leaf warn-only rollout flag"
set +e
out_legacy_warn_only="$("$BIN" check --lang=en --legacy-self-leaf-warn-only --fail-on-warning "$legacy_runtime_src" 2>&1)"
rc_legacy_warn_only=$?
set -e
[ "$rc_legacy_warn_only" -ne 0 ] || die "--legacy-self-leaf-warn-only should still produce warnings under --fail-on-warning"
grep -Fq "warning[E1020]" <<<"$out_legacy_warn_only" || die "missing E1020 warning in warn-only rollout mode"

log "mod doctor fix write"
tmp_legacy="$(mktemp "${TMPDIR:-/tmp}/vitte-mod-doctor-write-XXXXXX.vit")"
cp "$legacy_runtime_src" "$tmp_legacy"
set +e
out_doctor_write="$("$BIN" mod doctor --lang=en --fix --write "$tmp_legacy" 2>&1)"
rc_doctor_write=$?
set -e
[ "$rc_doctor_write" -ne 0 ] || die "mod doctor --fix --write should still report remaining issues on fixture"
grep -Fq "[doctor] write: rewrote 1 import(s)" <<<"$out_doctor_write" || die "missing write summary"
if ! diff -u "$legacy_write_expected" "$tmp_legacy" >/dev/null 2>&1; then
  diff -u "$legacy_write_expected" "$tmp_legacy" || true
  rm -f "$tmp_legacy"
  die "rewritten file does not match snapshot"
fi
rm -f "$tmp_legacy"

log "mod doctor fix write (use legacy)"
tmp_use_legacy="$(mktemp "${TMPDIR:-/tmp}/vitte-mod-doctor-use-legacy-XXXXXX.vit")"
cp "$doctor_write_cases_dir/use_legacy.vit" "$tmp_use_legacy"
set +e
out_doctor_write_use="$("$BIN" mod doctor --lang=en --fix --write "$tmp_use_legacy" 2>&1)"
rc_doctor_write_use=$?
set -e
[ "$rc_doctor_write_use" -ne 0 ] || die "mod doctor --fix --write should report issues on use_legacy fixture"
grep -Fq "[doctor] write: rewrote 1 import(s)" <<<"$out_doctor_write_use" || die "missing use_legacy rewrite summary"
if ! diff -u "$doctor_write_cases_dir/use_legacy.rewritten.must" "$tmp_use_legacy" >/dev/null 2>&1; then
  diff -u "$doctor_write_cases_dir/use_legacy.rewritten.must" "$tmp_use_legacy" || true
  rm -f "$tmp_use_legacy"
  die "use_legacy rewrite snapshot mismatch"
fi
rm -f "$tmp_use_legacy"

log "mod doctor fix write (pull legacy)"
tmp_pull_legacy="$(mktemp "${TMPDIR:-/tmp}/vitte-mod-doctor-pull-legacy-XXXXXX.vit")"
cp "$doctor_write_cases_dir/pull_legacy.vit" "$tmp_pull_legacy"
set +e
out_doctor_write_pull="$("$BIN" mod doctor --lang=en --fix --write "$tmp_pull_legacy" 2>&1)"
rc_doctor_write_pull=$?
set -e
[ "$rc_doctor_write_pull" -ne 0 ] || die "mod doctor --fix --write should report issues on pull_legacy fixture"
grep -Fq "[doctor] write: rewrote 1 import(s)" <<<"$out_doctor_write_pull" || die "missing pull_legacy rewrite summary"
if ! diff -u "$doctor_write_cases_dir/pull_legacy.rewritten.must" "$tmp_pull_legacy" >/dev/null 2>&1; then
  diff -u "$doctor_write_cases_dir/pull_legacy.rewritten.must" "$tmp_pull_legacy" || true
  rm -f "$tmp_pull_legacy"
  die "pull_legacy rewrite snapshot mismatch"
fi
rm -f "$tmp_pull_legacy"

log "mod doctor fix write (glob/alias-missing/relative fixtures)"
for fixture in glob_conflict.vit alias_missing.vit relative_path.vit; do
  set +e
  out_case="$("$BIN" mod doctor --lang=en --fix --write "$doctor_write_cases_dir/$fixture" 2>&1)"
  rc_case=$?
  set -e
  [ "$rc_case" -ne 0 ] || die "mod doctor --fix --write should report issues on $fixture"
  grep -Fq "[doctor]" <<<"$out_case" || die "missing doctor output for $fixture"
done

log "mod doctor fix check"
set +e
out_doctor_check_legacy="$("$BIN" mod doctor --lang=en --fix --check "$legacy_runtime_src" 2>&1)"
rc_doctor_check_legacy=$?
set -e
[ "$rc_doctor_check_legacy" -ne 0 ] || die "mod doctor --fix --check should fail when rewrites are available"
grep -Fq "[doctor] check: rewritable imports=" <<<"$out_doctor_check_legacy" || die "missing doctor check summary"

set +e
out_doctor_check_clean="$("$BIN" mod doctor --lang=en --fix --check "$ROOT_DIR/tests/modules/reexport_conflict/pkg/a.vit" 2>&1)"
rc_doctor_check_clean=$?
set -e
[ "$rc_doctor_check_clean" -eq 0 ] || die "mod doctor --fix --check should pass when no rewrites are available"

set +e
out_doctor_write_check="$("$BIN" mod doctor --lang=en --fix --write --check "$legacy_runtime_src" 2>&1)"
rc_doctor_write_check=$?
set -e
[ "$rc_doctor_write_check" -ne 0 ] || die "mod doctor --fix --write --check should fail when rewrites are available"
grep -Fq "[doctor] check: rewritable imports=" <<<"$out_doctor_write_check" || die "missing doctor write+check summary"

log "mod migrate-imports (check + write)"
tmp_migrate_root="$(mktemp -d "${TMPDIR:-/tmp}/vitte-migrate-imports-XXXXXX")"
cp "$legacy_runtime_src" "$tmp_migrate_root/main.vit"
set +e
out_migrate_check="$("$BIN" mod migrate-imports --lang=en --roots "$tmp_migrate_root" 2>&1)"
rc_migrate_check=$?
set -e
[ "$rc_migrate_check" -ne 0 ] || die "mod migrate-imports check mode should fail when candidates exist"
grep -Fq "[migrate-imports] candidate:" <<<"$out_migrate_check" || die "missing migrate candidate output"

set +e
out_migrate_write="$("$BIN" mod migrate-imports --lang=en --write --roots "$tmp_migrate_root" 2>&1)"
rc_migrate_write=$?
set -e
[ "$rc_migrate_write" -eq 0 ] || die "mod migrate-imports --write should succeed"
grep -Fq "rewritten_files=1" <<<"$out_migrate_write" || die "missing migrate rewritten_files=1 summary"
grep -Fq "use vitte/abi as legacy_abi" "$tmp_migrate_root/main.vit" || die "migrate --write did not rewrite legacy import"
[ -f "$tmp_migrate_root/main.vit.bak" ] || die "migrate --write should create .bak by default"
rm -rf "$tmp_migrate_root"

log "mod doctor multi-file roots (check + write)"
tmp_doctor_roots="$(mktemp -d "${TMPDIR:-/tmp}/vitte-doctor-roots-XXXXXX")"
cp "$legacy_runtime_src" "$tmp_doctor_roots/main.vit"
set +e
out_doctor_roots_check="$("$BIN" mod doctor --lang=en --fix --check --roots "$tmp_doctor_roots" 2>&1)"
rc_doctor_roots_check=$?
set -e
[ "$rc_doctor_roots_check" -ne 0 ] || die "mod doctor --fix --check --roots should fail when candidates exist"
grep -Fq "[migrate-imports] candidate:" <<<"$out_doctor_roots_check" || die "missing doctor roots candidate output"

set +e
out_doctor_roots_write="$("$BIN" mod doctor --lang=en --fix --write --roots "$tmp_doctor_roots" 2>&1)"
rc_doctor_roots_write=$?
set -e
[ "$rc_doctor_roots_write" -eq 0 ] || die "mod doctor --fix --write --roots should succeed"
grep -Fq "rewritten_files=1" <<<"$out_doctor_roots_write" || die "missing doctor roots rewritten_files=1 summary"
grep -Fq "use vitte/abi as legacy_abi" "$tmp_doctor_roots/main.vit" || die "doctor roots --write did not rewrite import"
[ -f "$tmp_doctor_roots/main.vit.bak" ] || die "doctor roots --write should create .bak by default"
rm -rf "$tmp_doctor_roots"

log "dump module index"
out_idx="$("$BIN" check --lang=en --dump-module-index "$graph_src" 2>&1)"
grep -Fq "\"modules\"" <<<"$out_idx" || die "missing modules JSON key"
grep -Fq "\"profile\"" <<<"$out_idx" || die "missing profile JSON key"

log "cache report"
out_cache="$("$BIN" check --lang=en --cache-report "$graph_src" 2>&1)"
grep -Fq "[cache] parse=" <<<"$out_cache" || die "missing cache report output"

log "cross-package (collections + abi + channel)"
out_cross="$("$BIN" check --lang=en "$cross_pkg_src" 2>&1)"
grep -Fq "[driver] mir ok" <<<"$out_cross" || die "cross-package fixture should reach mir ok"

log "mod contract-diff"
set +e
out_diff="$("$BIN" mod contract-diff --lang=en --old "$api_old" --new "$api_new" 2>&1)"
rc_diff=$?
set -e
[ "$rc_diff" -ne 0 ] || die "contract-diff should detect breaking removal"
grep -Fq "[contract-diff] removed symbol" <<<"$out_diff" || die "missing removed symbol in contract-diff"
grep -Fq "[contract-diff] BREAKING" <<<"$out_diff" || die "missing BREAKING in contract-diff"

log "warn experimental"
set +e
out_warn="$("$BIN" check --lang=en --warn-experimental --fail-on-warning "$exp_src" 2>&1)"
rc_warn=$?
set -e
[ "$rc_warn" -ne 0 ] || die "warn-experimental + fail-on-warning should fail"
grep -Fq "warning[E1015]" <<<"$out_warn" || die "missing experimental warning in warn mode"

log "deny/allow internal"
set +e
out_internal_deny="$("$BIN" check --lang=en --deny-internal "$internal_src" 2>&1)"
rc_internal_deny=$?
set -e
[ "$rc_internal_deny" -ne 0 ] || die "deny-internal should fail internal import"
grep -Fq "error[E1016]" <<<"$out_internal_deny" || die "missing internal deny diagnostic"

out_internal_allow="$("$BIN" check --lang=en --allow-internal "$internal_src" 2>&1)"
grep -Fq "mir ok" <<<"$out_internal_allow" || die "allow-internal should pass"

log "re-export conflict"
set +e
out_reexport="$("$BIN" check --lang=en "$reexport_src" 2>&1)"
rc_reexport=$?
set -e
[ "$rc_reexport" -ne 0 ] || die "re-export conflict fixture should fail"
grep -Fq "error[E1017]" <<<"$out_reexport" || die "missing re-export conflict code"
grep -Fq "fix: replace one glob import with explicit symbols" <<<"$out_reexport" || die "missing re-export fix suggestion"

log "clean cache"
out_clean="$("$BIN" clean-cache 2>&1)"
grep -Eq "\\[clean-cache\\] (removed|nothing to clean)" <<<"$out_clean" || die "clean-cache output mismatch"

log "OK"
