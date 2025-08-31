#!/usr/bin/env bash
# ============================================================
# Vitte — scripts/fuzz.sh
# Orchestration de fuzzing (Rust): cargo-fuzz (libFuzzer) principal,
# avec bonus honggfuzz / AFL++ si présents.
#
# Prérequis:
#   rustup + nightly, cargo-fuzz  (cargo install cargo-fuzz)
# Optionnels:
#   cargo-llvm-cov (coverage), honggfuzz, afl++
#
# Exemples:
#   scripts/fuzz.sh init
#   scripts/fuzz.sh run my_target --seconds 30 --jobs 4
#   scripts/fuzz.sh minimize my_target
#   scripts/fuzz.sh cmin my_target
#   scripts/fuzz.sh repro my_target crash-123456
#   scripts/fuzz.sh coverage my_target
#   scripts/fuzz.sh honggfuzz my_target
#   scripts/fuzz.sh afl my_target
#   scripts/fuzz.sh list
#
# Dirs:
#   fuzz/                (créé par cargo-fuzz init)
#   fuzz/fuzz_targets/   (fichiers *.rs des cibles)
#   fuzz/corpus/<tgt>/   (corpus)
#   fuzz/artifacts/<tgt> (crashers)
#   fuzz/dict/<tgt>.dict (optionnel, dictionnaire)
# ============================================================

set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

# ----- couleurs -----
if [ -t 1 ]; then
  C_GRN="\033[32m"; C_YLW="\033[33m"; C_RED="\033[31m"; C_BLU="\033[34m"; C_DIM="\033[2m"; C_RST="\033[0m"
else
  C_GRN=""; C_YLW=""; C_RED=""; C_BLU=""; C_DIM=""; C_RST=""
fi
log(){ echo -e "${C_BLU}[fuzz]${C_RST} $*"; }
ok(){  echo -e "${C_GRN}[ok]${C_RST} $*"; }
warn(){echo -e "${C_YLW}[!]${C_RST} $*"; }
err(){ echo -e "${C_RED}[err]${C_RST} $*"; }

cmd_exists(){ command -v "$1" >/dev/null 2>&1; }

# ----- sous-commandes -----
usage() {
  sed -n '1,/^set -euo pipefail/p' "$0" | sed 's/^# \{0,1\}//'
  cat <<'EOF'
Usage:
  scripts/fuzz.sh init
  scripts/fuzz.sh add <target_name>           # crée fuzz_targets/<name>.rs stub
  scripts/fuzz.sh list                        # liste les targets (cargo-fuzz)
  scripts/fuzz.sh run <target> [opts]         # lance libFuzzer
      --seconds N           durée (s)  ex: 60
      --jobs N              workers     ex: 4
      --dict PATH           dictionnaire
      --seed PATH           ajoute un fichier seed au corpus
      --san <asan|ubsan|none>
      --memlimit MB         limite mémoire (0=illimitée)
      --rsslimit MB         limite RSS (libFuzzer)
      --timeout SEC         timeout par input
      --release             build release
  scripts/fuzz.sh cmin <target>               # corpus minification
  scripts/fuzz.sh tmin <target> <crash>       # testcase minimization
  scripts/fuzz.sh repro <target> <artifact>   # reproduit un crash
  scripts/fuzz.sh coverage <target>           # génère lcov (coverage/)
  scripts/fuzz.sh honggfuzz <target> [opts]   # si honggfuzz dispo
  scripts/fuzz.sh afl <target> [opts]         # si afl-fuzz dispo

Tips:
  - Place tes seeds dans fuzz/corpus/<target>/seed-* ou via --seed.
  - Place ton dict dans fuzz/dict/<target>.dict et passe --dict automatiquement.
EOF
}

# ----- helpers -----
ensure_cargo_fuzz() {
  if ! cmd_exists cargo-fuzz; then
    err "cargo-fuzz introuvable. Installe: cargo install cargo-fuzz"
    exit 1
  fi
}

ensure_nightly() {
  if ! rustc -Vv | grep -qi 'release: .*nightly'; then
    warn "Toolchain non-nightly; cargo-fuzz utilisera nightly automatiquement."
  fi
}

target_corpus_dir(){ echo "fuzz/corpus/$1"; }
target_art_dir(){ echo "fuzz/artifacts/$1"; }
target_dict_path(){ echo "fuzz/dict/$1.dict"; }

# ----- init -----
init() {
  ensure_cargo_fuzz
  if [ -d fuzz ]; then
    warn "fuzz/ existe déjà — skip cargo-fuzz init"
  else
    log "Initialisation cargo-fuzz…"
    cargo fuzz init
  fi
  mkdir -p fuzz/dict
  ok "Init terminé."
}

# ----- add target stub -----
add_target() {
  local name="${1:?nom de target}"
  ensure_cargo_fuzz
  local path="fuzz/fuzz_targets/${name}.rs"
  if [ -f "$path" ]; then
    warn "Target existe déjà: $path"
  else
    cat > "$path" <<'RS'
#![no_main]
use libfuzzer_sys::fuzz_target;

// TODO: importe tes fonctions à fuzzer depuis ta lib
fn fuzz_entry(data: &[u8]) {
    // Exemple: valide du UTF-8 et appelle un parseur
    let _ = std::str::from_utf8(data).ok().map(|s| s.len());
    // place ton code testé ici
}

fuzz_target!(|data: &[u8]| {
    fuzz_entry(data);
});
RS
    ok "Créé: $path"
  fi
  mkdir -p "fuzz/corpus/$name"
  ok "Corpus: fuzz/corpus/$name"
}

# ----- list -----
list() {
  ensure_cargo_fuzz
  log "Targets disponibles:"
  cargo fuzz list || true
}

# ----- run (libFuzzer) -----
run_libfuzzer() {
  ensure_cargo_fuzz
  ensure_nightly
  local target="${1:?target manquante}"; shift

  local seconds=0 jobs=0 dict="" seed="" san="asan" release=false memlimit=0 rss=0 timeout=0
  while (( "$#" )); do
    case "$1" in
      --seconds) seconds="${2:?}"; shift 2;;
      --jobs)    jobs="${2:?}"; shift 2;;
      --dict)    dict="${2:?}"; shift 2;;
      --seed)    seed="${2:?}"; shift 2;;
      --san)     san="${2:?}"; shift 2;;
      --memlimit) memlimit="${2:?}"; shift 2;;
      --rsslimit) rss="${2:?}"; shift 2;;
      --timeout) timeout="${2:?}"; shift 2;;
      --release) release=true; shift;;
      *) err "Option inconnue: $1"; exit 1;;
    esac
  done

  local corpus="$(target_corpus_dir "$target")"
  mkdir -p "$corpus" "$(target_art_dir "$target")"

  # Ajout seed si fourni
  if [[ -n "$seed" ]]; then
    cp -n "$seed" "$corpus/seed-$(date +%s)-$(basename "$seed")" || true
  fi

  # Dictionnaire: auto si present
  local dict_auto="$(target_dict_path "$target")"
  if [[ -z "$dict" && -f "$dict_auto" ]]; then
    dict="$dict_auto"
  fi

  # Sanitizers
  case "$san" in
    asan)   export RUSTFLAGS="${RUSTFLAGS:-} -Zsanitizer=address";;
    ubsan)  export RUSTFLAGS="${RUSTFLAGS:-} -Zsanitizer=undefined";;
    none)   : ;;
    *) warn "sanitizer inconnu: $san (utilise asan/ubsan/none)";;
  esac

  local FUZZ_ARGS=()
  [[ $seconds -gt 0 ]] && FUZZ_ARGS+=("-max_total_time=$seconds")
  [[ $jobs -gt 0    ]] && FUZZ_ARGS+=("-workers=$jobs" "-jobs=$jobs")
  [[ $memlimit -gt 0 ]] && FUZZ_ARGS+=("-rss_limit_mb=$memlimit")
  [[ $rss -gt 0      ]] && FUZZ_ARGS+=("-rss_limit_mb=$rss")
  [[ $timeout -gt 0  ]] && FUZZ_ARGS+=("-timeout=$timeout")
  [[ -n "$dict"      ]] && FUZZ_ARGS+=("-dict=$dict")

  local mode=("--dev")
  $release && mode=("--release")

  log "libFuzzer → target=$target time=${seconds}s jobs=$jobs san=$san dict=${dict:-none}"
  # shellcheck disable=SC2068
  cargo fuzz run "$target" "$corpus" --fuzz-dir fuzz ${mode[@]} -- ${FUZZ_ARGS[@]}
}

# ----- cmin/tmin -----
cmin() {
  ensure_cargo_fuzz
  local target="${1:?target manquante}"
  cargo fuzz cmin "$target" --fuzz-dir fuzz
  ok "Corpus minimisé: fuzz/corpus/$target"
}

tmin() {
  ensure_cargo_fuzz
  local target="${1:?}"; local testcase="${2:?path crash/testcase}"
  cargo fuzz tmin "$target" "$testcase" --fuzz-dir fuzz
  ok "Testcase minimisé (voir fuzz/artifacts/$target)"
}

# ----- repro -----
repro() {
  ensure_cargo_fuzz
  local target="${1:?}"; local art="${2:?artifact file}"
  if [ ! -f "$art" ]; then
    # tente artefact par nom simple
    local guess="fuzz/artifacts/$target/$art"
    [ -f "$guess" ] && art="$guess"
  fi
  log "Reproduction crash: $art"
  cargo fuzz run "$target" --fuzz-dir fuzz -- "$art"
}

# ----- coverage (llvm-cov) -----
coverage() {
  local target="${1:?target manquante}"
  if ! cmd_exists cargo-llvm-cov; then
    err "cargo-llvm-cov introuvable. Installe: cargo install cargo-llvm-cov"
  fi
  # Build with coverage instrumentation + run on corpus
  local corpus="$(target_corpus_dir "$target")"
  [ -d "$corpus" ] || { err "Corpus introuvable: $corpus"; }

  log "Coverage instrumentation…"
  # On génère un binaire runner minimal qui lit les fichiers du corpus
  local runfile="target/coverage_fuzz_runner.rs"
  mkdir -p target
  cat > "$runfile" <<'RS'
fn main() {
    let args: Vec<_> = std::env::args().skip(1).collect();
    for path in args {
        if let Ok(data) = std::fs::read(&path) {
            vitte_fuzz_entry(&data); // à exposer via ta crate si besoin
        }
    }
}
// NOTE: crée une fonction publique dans ta crate pour déléguer à la même logique
// que fuzz_target!, ex: pub fn vitte_fuzz_entry(data: &[u8]) { ... }
RS
  warn "NB: assure-toi d'exposer `vitte_fuzz_entry` dans ta lib pour coverage."
  mkdir -p coverage
  cargo llvm-cov clean --workspace
  cargo llvm-cov run --bin coverage_fuzz_dummy --ignore-filename-regex fuzz || true
  # En pratique, préférer un petit binaire dédié dans examples/ plutôt que ce stub.
  cargo llvm-cov --workspace --lcov --output-path coverage/coverage.lcov
  ok "Coverage → coverage/coverage.lcov"
}

# ----- honggfuzz / afl++ (best-effort) -----
honggfuzz() {
  local target="${1:?target manquante}"; shift || true
  if ! cmd_exists hfuzz-run; then
    err "honggfuzz introuvable (hfuzz-run). Installe le paquet honggfuzz de ta distro."
  fi
  local corpus="$(target_corpus_dir "$target")"
  mkdir -p "$corpus" "$(target_art_dir "$target")"
  warn "honggfuzz intégration générique: tu dois fournir un binaire cible."
  echo "Voir: https://github.com/rust-fuzz/honggfuzz-rs"
}

afl() {
  local target="${1:?target manquante}"; shift || true
  if ! cmd_exists afl-fuzz; then
    err "afl-fuzz introuvable. Installe AFL++."
  fi
  local corpus="$(target_corpus_dir "$target")"
  mkdir -p "$corpus" "$(target_art_dir "$target")"
  warn "AFL++ intégration générique: nécessite wrapper main() spécifique."
  echo "Voir: https://github.com/rust-fuzz/afl.rs"
}

# ----- dispatcher -----
sub="${1:-help}"; shift || true
case "$sub" in
  help|-h|--help) usage ;;
  init) init ;;
  add)  add_target "$@" ;;
  list) list ;;
  run)  run_libfuzzer "$@" ;;
  cmin) cmin "$@" ;;
  tmin) tmin "$@" ;;
  repro) repro "$@" ;;
  coverage) coverage "$@" ;;
  honggfuzz) honggfuzz "$@" ;;
  afl) afl "$@" ;;
  *) usage; err "Sous-commande inconnue: $sub"; exit 1;;
esac

ok "Done."
