#!/usr/bin/env bash
# Minimal golden-file check for the vittec placeholder compiler.

set -euo pipefail

this_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
workspace_root="$(cd "${this_dir}/.." && pwd)"

vittec_bin="${workspace_root}/target/debug/vittec"
golden_dir="${workspace_root}/tests/goldens"

log() {
    printf '[vitte][golden][INFO] %s\n' "$*"
}

log_error() {
    printf '[vitte][golden][ERROR] %s\n' "$*" >&2
}

if [[ ! -x "${vittec_bin}" ]]; then
    log_error "vittec introuvable à ${vittec_bin} (run make bootstrap-stage1 pour générer le placeholder)."
    exit 1
fi

cases=(
    "tests/data/mini_project/muffin.muf|tests/goldens/mini_project_manifest.golden"
    "tests/data/mini_project/src/app/main.vitte|tests/goldens/mini_project_main.golden"
    "tests/data/resolver/scope_rules.vitte|tests/goldens/resolver_scope_rules.golden"
)

status=0
for entry in "${cases[@]}"; do
    IFS="|" read -r rel_input rel_golden <<<"${entry}"

    input_path="${workspace_root}/${rel_input}"
    golden_path="${workspace_root}/${rel_golden}"

    if [[ ! -f "${golden_path}" ]]; then
        log_error "Golden manquant: ${golden_path}"
        status=1
        continue
    fi

    tmp_out="$(mktemp "${TMPDIR:-/tmp}/vittec-golden-XXXXXX")"
    if ! "${vittec_bin}" "${input_path}" >"${tmp_out}" 2>&1; then
        # Les entrées peuvent volontairement déclencher des diagnostics; on compare tout de même la sortie.
        log_error "Execution échec sur ${rel_input} (voir ${tmp_out}, statut ignoré pour la comparaison)"
    fi

    if ! diff -u "${golden_path}" "${tmp_out}"; then
        log_error "Mismatch pour ${rel_input} (voir diff ci-dessus)"
        status=1
    else
        log "OK ${rel_input}"
    fi

    rm -f "${tmp_out}"
done

exit "${status}"
