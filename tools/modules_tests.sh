#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"

log() { printf "[modules-tests] %s\n" "$*"; }
die() { printf "[modules-tests][error] %s\n" "$*" >&2; exit 1; }

[ -x "$BIN" ] || die "missing binary: $BIN"

graph_src="$ROOT_DIR/tests/modules/mod_graph/main.vit"
doctor_src="$ROOT_DIR/tests/modules/mod_doctor/main.vit"
api_old="$ROOT_DIR/tests/modules/api_diff/old_case/main.vit"
api_new="$ROOT_DIR/tests/modules/api_diff/new_case/main.vit"
exp_src="$ROOT_DIR/tests/modules/experimental/main.vit"
internal_src="$ROOT_DIR/tests/modules/internal/main.vit"

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

log "dump module index"
out_idx="$("$BIN" check --lang=en --dump-module-index "$graph_src" 2>&1)"
grep -Fq "\"modules\"" <<<"$out_idx" || die "missing modules JSON key"
grep -Fq "\"profile\"" <<<"$out_idx" || die "missing profile JSON key"

log "cache report"
out_cache="$("$BIN" check --lang=en --cache-report "$graph_src" 2>&1)"
grep -Fq "[cache] parse=" <<<"$out_cache" || die "missing cache report output"

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

log "clean cache"
out_clean="$("$BIN" clean-cache 2>&1)"
grep -Eq "\\[clean-cache\\] (removed|nothing to clean)" <<<"$out_clean" || die "clean-cache output mismatch"

log "OK"
