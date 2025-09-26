\
    #!/usr/bin/env bash
    # benchmarks/scripts/prepare.sh
    # Prépare les artefacts de bench pour Vitte :
    # - construit (optionnel) le binaire `vitte` (CLI)
    # - pack tous les .vit → .vitbc sous benchmarks/out/<tier>/
    # - (optionnel) exécute chaque .vitbc pour sanity
    #
    # Usage rapide :
    #   ./benchmarks/scripts/prepare.sh
    #
    # Options :
    #   --bin <path>       : utilise un binaire vitte spécifique (sinon auto-lookup/BUILD)
    #   --profile <p>      : debug | release (def: release)
    #   --no-build         : n’essaie pas de builder si le binaire est absent
    #   --run              : exécute chaque .vitbc une fois (sanity)
    #   --data <dir>       : répertoire des sources .vit (def: benchmarks/data)
    #   --out  <dir>       : répertoire de sortie .vitbc (def: benchmarks/out)
    #   --clean            : supprime le répertoire de sortie avant génération
    #   -h|--help          : affiche cette aide
    #
    # Variables d’env utiles :
    #   VITTE_BIN          : chemin vers le binaire vitte (prioritaire sur --profile)
    #   PACK_FLAGS         : flags supplémentaires passés à `vitte pack` (ex: "-O2")
    #   RUSTFLAGS          : flags de build cargo si build nécessaire
    #
    set -Eeuo pipefail

    # ──────────────────────────────────────────────────────────────────────────
    # Utils
    # ──────────────────────────────────────────────────────────────────────────
    info() { printf '\033[1;34m[i]\033[0m %s\n' "$*"; }
    warn() { printf '\033[1;33m[!]\033[0m %s\n' "$*"; }
    die()  { printf '\033[1;31m[x]\033[0m %s\n' "$*" >&2; exit 1; }

    have() { command -v "$1" >/dev/null 2>&1; }

    sha256() {
      if have sha256sum; then sha256sum "$1" | awk '{print $1}';
      elif have shasum; then shasum -a 256 "$1" | awk '{print $1}';
      else echo "unknown"; fi
    }

    normpath() { python - "$1" <<'PY'
    import os, sys
    p = sys.argv[1]
    print(os.path.normpath(os.path.abspath(p)))
    PY
    }

    script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
    repo_root="$(cd -- "$script_dir/../.." && pwd -P)"

    data_dir="$repo_root/benchmarks/data"
    out_dir="$repo_root/benchmarks/out"
    profile="release"
    do_build=1
    do_run=0
    vitte_bin="${VITTE_BIN:-}"

    # parse args
    while [[ $# -gt 0 ]]; do
      case "$1" in
        --bin)       shift; vitte_bin="${1:-}"; [[ -n "${vitte_bin:-}" ]] || die "--bin requiert un chemin";;
        --profile)   shift; profile="${1:-}";;
        --no-build)  do_build=0;;
        --run)       do_run=1;;
        --data)      shift; data_dir="$(normpath "${1:-}")";;
        --out)       shift; out_dir="$(normpath "${1:-}")";;
        --clean)     rm -rf -- "$out_dir";;
        -h|--help)
          grep -E '^#( |$)' "$0" | sed -E 's/^# ?//'; exit 0;;
        *) die "Option inconnue: $1";;
      esac
      shift || true
    done

    # ──────────────────────────────────────────────────────────────────────────
    # Résolution du binaire vitte
    # ──────────────────────────────────────────────────────────────────────────
    exe_ext=""
    case "${OS:-}" in
      Windows_NT) exe_ext=".exe" ;;
    esac
    case "${MSYSTEM:-}" in
      MINGW*|MSYS*) exe_ext=".exe" ;;
    esac

    if [[ -z "${vitte_bin:-}" ]]; then
      # heuristiques locales
      if [[ -n "${VITTE_BIN:-}" ]]; then
        vitte_bin="$VITTE_BIN"
      else
        cand1="$repo_root/target/${profile}/vitte${exe_ext}"
        cand2="$repo_root/target/${profile}/vitte-tools${exe_ext}" # au cas où
        if [[ -x "$cand1" ]]; then vitte_bin="$cand1"
        elif [[ -x "$cand2" ]]; then vitte_bin="$cand2"
        else vitte_bin="" ; fi
      fi
    fi

    if [[ -z "${vitte_bin:-}" || ! -x "${vitte_bin:-/nonexistent}" ]]; then
      if [[ $do_build -eq 1 ]]; then
        info "Binaire Vitte introuvable — build en cours (profile=$profile)…"
        if [[ "$profile" == "release" ]]; then
          cargo build -p vitte-tools --features cli --release --manifest-path "$repo_root/Cargo.toml"
        else
          cargo build -p vitte-tools --features cli --manifest-path "$repo_root/Cargo.toml"
        fi
        vitte_bin="$repo_root/target/${profile}/vitte${exe_ext}"
        [[ -x "$vitte_bin" ]] || die "Échec build : binaire $vitte_bin introuvable"
      else
        die "Binaire Vitte introuvable et --no-build activé"
      fi
    fi

    info "Vitte: $vitte_bin"
    info "Data : $data_dir"
    info "Out  : $out_dir"

    mkdir -p "$out_dir"

    # ──────────────────────────────────────────────────────────────────────────
    # Pack de tous les .vit → .vitbc (miroir d'arborescence)
    # ──────────────────────────────────────────────────────────────────────────
    declare -i ok=0 fail=0 total=0
    manifest="$out_dir/manifest.txt"
    : > "$manifest"

    mapfile -t files < <(find "$data_dir" -type f -name '*.vit' | sort)
    total="${#files[@]}"
    if [[ $total -eq 0 ]]; then
      warn "Aucun fichier .vit sous $data_dir"
    fi

    for src in "${files[@]}"; do
      rel="${src#$data_dir/}"
      base="${rel%.vit}"
      out="$out_dir/${base}.vitbc"
      out_parent="$(dirname "$out")"
      mkdir -p "$out_parent"

      info "pack: $rel → ${base}.vitbc"
      if "$vitte_bin" pack ${PACK_FLAGS:-} "$src" -o "$out"; then
        sum="$(sha256 "$out")"
        bytes="$(stat -c '%s' "$out" 2>/dev/null || stat -f '%z' "$out")"
        printf '%s\t%s\t%s bytes\t%s\n' "$rel" "${base}.vitbc" "$bytes" "$sum" >> "$manifest"
        ok+=1
      else
        warn "échec pack: $rel"
        fail+=1
      fi

      if [[ $do_run -eq 1 && -f "$out" ]]; then
        info "run : ${base}.vitbc"
        if ! "$vitte_bin" run "$out"; then
          warn "échec run: ${base}.vitbc"
        fi
      fi
    done

    info "Terminé: $ok ok, $fail échecs, $total total"
    info "Manifest: $manifest"
