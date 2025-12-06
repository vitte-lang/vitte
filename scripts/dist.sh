#!/usr/bin/env bash
# Build a release-friendly archive for vittec and emit a SHA256 signature.
set -euo pipefail

this_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
workspace_root="$(cd "${this_dir}/.." && pwd)"

version="${VITTE_VERSION:-$(awk -F '\"' '/^version/ {print $2; exit}' "${workspace_root}/vitte.project.muf")}"
os="$(uname -s | tr '[:upper:]' '[:lower:]')"
arch="$(uname -m)"

debug_bin="${workspace_root}/target/debug/vittec"
release_dir="${workspace_root}/target/release"
release_bin="${release_dir}/vittec"

dist_dir="${workspace_root}/dist"
tmp_root=""

cleanup() {
    [[ -n "${tmp_root}" && -d "${tmp_root}" ]] && rm -rf "${tmp_root}"
}
trap cleanup EXIT

log() {
    printf '[vitte][dist][INFO] %s\n' "$*"
}

log_error() {
    printf '[vitte][dist][ERROR] %s\n' "$*" >&2
}

if [[ ! -x "${debug_bin}" ]]; then
    log_error "Binaire debug introuvable (${debug_bin}). Lance d'abord make bootstrap-stage1."
    exit 1
fi

mkdir -p "${release_dir}" "${dist_dir}"
install -m 0755 "${debug_bin}" "${release_bin}"

tmp_root="$(mktemp -d "${TMPDIR:-/tmp}/vittec-dist-XXXXXX")"
install -m 0755 "${release_bin}" "${tmp_root}/vittec"
[[ -f "${workspace_root}/LICENSE" ]] && cp "${workspace_root}/LICENSE" "${tmp_root}/"
[[ -f "${workspace_root}/README.md" ]] && cp "${workspace_root}/README.md" "${tmp_root}/"

archive_name="vittec-${version}-${os}-${arch}.tar.gz"
archive_path="${dist_dir}/${archive_name}"
sha_path="${archive_path}.sha256"

tar -C "${tmp_root}" -czf "${archive_path}" .
shasum -a 256 "${archive_path}" >"${sha_path}"

log "Binaire release : ${release_bin}"
log "Archive créée   : ${archive_path}"
log "Signature SHA   : ${sha_path}"
